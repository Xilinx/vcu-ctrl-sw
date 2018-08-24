/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "EncCmdMngr.h"

using namespace std;

/****************************************************************************/
class CCmdTokenizer
{
public:
  CCmdTokenizer(string& sLine)
    : m_sLine(sLine),
    m_zBeg(0),
    m_zEnd(0),
    m_zNext(0),
    m_fVal(0.0)
  {}

  bool GetNext()
  {
    m_fVal = 0.0;

    if(m_zNext == string::npos)
      return false;

    m_zBeg = m_sLine.find_first_not_of(m_Separators, m_zNext);

    if(m_zBeg == string::npos)
      return false;

    m_zEnd = m_sLine.find_first_of(m_Separators, m_zBeg);

    if(m_zEnd != string::npos && m_sLine[m_zEnd] == '=')
    {
      size_t zPos1 = m_sLine.find_first_not_of(m_Separators, m_zEnd + 1);

      if(zPos1 == string::npos)
        return false;

      size_t zPos2 = m_sLine.find_first_of(m_Separators, zPos1 + 1);

      m_fVal = atof(m_sLine.substr(zPos1, zPos2 - zPos1).c_str());

      m_zNext = zPos2;
    }
    else
      m_zNext = m_zEnd;

    return true;
  }

  double GetValue() { return m_fVal; }

  bool operator == (const char* S) const { return m_sLine.substr(m_zBeg, m_zEnd - m_zBeg) == S; }
  int atoi()
  {
    return ::atoi(m_sLine.substr(m_zBeg, m_zEnd - m_zBeg).c_str());
  }

private:
  const char* m_Separators = ":,= \t";

  string const& m_sLine;
  size_t m_zBeg;
  size_t m_zEnd;
  size_t m_zNext;
  double m_fVal;
};

/****************************************************************************
 * Class CEncCmdMngr                                                         *
 *****************************************************************************/
CEncCmdMngr::CEncCmdMngr(istream& CmdInput, int iLookAhead, int iFreqLT)
  : m_CmdInput(CmdInput),
  m_iLookAhead(iLookAhead),
  m_iFreqLT(iFreqLT),
  m_bHasLT(false)
{
  Refill(0);
}

/****************************************************************************/
void CEncCmdMngr::Refill(int iCurFrame)
{
  while(m_Cmds.size() && m_Cmds.front().iFrame < iCurFrame)
    m_Cmds.pop_front();

  while((int)m_Cmds.size() < m_iLookAhead && (m_Cmds.empty() || m_Cmds.back().iFrame < iCurFrame + m_iLookAhead))
  {
    TFrmCmd Cmd {};

    if(!ReadNextCmd(Cmd))
      break;

    m_Cmds.push_back(Cmd);
  }
}

/****************************************************************************/
bool CEncCmdMngr::ReadNextCmd(TFrmCmd& Cmd)
{
  std::string sLine;
  bool bRet = false;

  for(;;)
  {
    if(!GetNextLine(sLine))
      break;

    if(sLine.empty())
      continue;

    if(ParseCmd(sLine, Cmd, bRet))
    {
      bRet = true;
    }
    else
    {
      m_sBufferedLine = sLine;
      break;
    }
  }

  return bRet;
}

/****************************************************************************/
bool CEncCmdMngr::GetNextLine(string& sNextLine)
{
  bool bSucceed = true;

  if(!m_sBufferedLine.empty())
  {
    sNextLine = m_sBufferedLine;
    m_sBufferedLine.clear();
  }
  else
  {
    getline(m_CmdInput, sNextLine);
    bSucceed = !m_CmdInput.fail();
  }

  return bSucceed;
}

/****************************************************************************/
bool CEncCmdMngr::ParseCmd(std::string sLine, TFrmCmd& Cmd, bool bSameFrame)
{
  CCmdTokenizer Tok(sLine);

  if(!Tok.GetNext())
    return bSameFrame;

  int iFrame = Tok.atoi();

  if(bSameFrame && iFrame != Cmd.iFrame)
    return false;

  Cmd.iFrame = iFrame;

  while(Tok.GetNext())
  {
    if(Tok == "SC")
      Cmd.bSceneChange = true;
    else if(Tok == "LT")
      Cmd.bIsLongTerm = true;
    else if(Tok == "UseLT")
      Cmd.bUseLongTerm = true;


    else if(Tok == "KF")
      Cmd.bKeyFrame = true;
    else if(Tok == "GopLen")
    {
      Cmd.bChangeGopLength = true;
      Cmd.iGopLength = int(Tok.GetValue());
    }
    else if(Tok == "NumB")
    {
      Cmd.bChangeGopNumB = true;
      Cmd.iGopNumB = int(Tok.GetValue());
    }
    else if(Tok == "BR")
    {
      Cmd.bChangeBitRate = true;
      Cmd.iBitRate = int(Tok.GetValue()) * 1000;
    }
    else if(Tok == "Fps")
    {
      Cmd.bChangeFrameRate = true;
      Cmd.iFrameRate = int(Tok.GetValue());
      Cmd.iClkRatio = 1000;
      int iFrac = int(Tok.GetValue() * 1000) % 1000;

      if(iFrac)
        Cmd.iClkRatio += (1000 - iFrac) / ++Cmd.iFrameRate;
    }
    else if(Tok == "QP")
    {
      Cmd.bChangeQP = true;
      Cmd.iQP = int(Tok.GetValue());
    }
  }

  return true;
}

/****************************************************************************/
void CEncCmdMngr::Process(ICommandsSender* sender, int iFrame)
{
  if(!m_Cmds.empty())
  {
    bool bRefill = false;

    // Look ahead command
    if(iFrame + m_iLookAhead == m_Cmds.back().iFrame)
    {
      bRefill = true;

      if(m_Cmds.back().bSceneChange)
        sender->notifySceneChange(m_iLookAhead);
    }

    // On time command
    if(iFrame == m_Cmds.front().iFrame)
    {
      bRefill = true;

      if(m_Cmds.front().bUseLongTerm && (m_iFreqLT || m_bHasLT))
        sender->notifyUseLongTerm();

      if(m_Cmds.front().bIsLongTerm)
      {
        sender->notifyIsLongTerm();
        m_bHasLT = true;
      }


      if(m_Cmds.front().bKeyFrame)
        sender->restartGop();

      if(m_Cmds.front().bChangeGopLength)
        sender->setGopLength(m_Cmds.front().iGopLength);

      if(m_Cmds.front().bChangeGopNumB)
        sender->setNumB(m_Cmds.front().iGopNumB);

      if(m_Cmds.front().bChangeFrameRate)
        sender->setFrameRate(m_Cmds.front().iFrameRate, m_Cmds.front().iClkRatio);

      if(m_Cmds.front().bChangeBitRate)
        sender->setBitRate(m_Cmds.front().iBitRate);

      if(m_Cmds.front().bChangeQP)
        sender->setQP(m_Cmds.front().iQP);
    }

    if(bRefill)
      Refill(iFrame + 1);
  }
}

