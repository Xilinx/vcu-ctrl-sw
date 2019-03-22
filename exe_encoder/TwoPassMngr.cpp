/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include "TwoPassMngr.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <iostream>

#define SEQUENCE_SIZE_MAX 1000

using namespace std;

/***************************************************************************/
/*Shared Methods for LookAhead and offline Twopass*/
/***************************************************************************/
static bool SceneChangeDetected(AL_TLookAheadMetaData* pPrevMeta, AL_TLookAheadMetaData* pCurrentMeta)
{
  if(!pPrevMeta || !pCurrentMeta)
    return false;

  auto iPercent = 100 * pCurrentMeta->iPercentIntra[0];
  auto iIntraRatio = (pPrevMeta->iPercentIntra[0] != 0) ? iPercent / pPrevMeta->iPercentIntra[0] : iPercent;
  return (pCurrentMeta->iPercentIntra[0] >= 95 && iIntraRatio > 135) || (pCurrentMeta->iPercentIntra[0] >= 80 && iIntraRatio > 200);
}

/***************************************************************************/
static bool SceneChangeDetected_Crop(AL_TLookAheadMetaData* pPrevMeta, AL_TLookAheadMetaData* pCurrentMeta)
{
  if(!pPrevMeta || !pCurrentMeta)
    return false;

  int iOk = 0, iKo = 0;

  for(int8_t i = 0; i < 5; i++)
  {
    auto iPercent = 100 * pCurrentMeta->iPercentIntra[i];
    auto iIntraRatio = (pPrevMeta->iPercentIntra[i] != 0) ? iPercent / pPrevMeta->iPercentIntra[i] : iPercent;

    if((pCurrentMeta->iPercentIntra[i] >= 95 && iIntraRatio > 135) || (pCurrentMeta->iPercentIntra[i] >= 80 && iIntraRatio > 200))
      iOk++;
    else if(pCurrentMeta->iPercentIntra[i] < 40)
      iKo++;
  }

  return iOk >= 3 && iKo == 0;
}

/***************************************************************************/
static int32_t GetIPRatio(AL_TLookAheadMetaData* pCurrentMeta, AL_TLookAheadMetaData* pNextMeta)
{
  if(!pCurrentMeta || !pNextMeta || !pNextMeta->iPictureSize)
    return 1000;

  return max(static_cast<int64_t>(100), 1000 * static_cast<int64_t>(pCurrentMeta->iPictureSize) / pNextMeta->iPictureSize);
}

/***************************************************************************/
AL_TLookAheadMetaData* AL_TwoPassMngr_CreateAndAttachTwoPassMetaData(AL_TBuffer* Src)
{
  auto pPictureMetaTP = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(Src, AL_META_TYPE_LOOKAHEAD));

  if(!pPictureMetaTP)
  {
    pPictureMetaTP = AL_LookAheadMetaData_Create();
    bool success = AL_Buffer_AddMetaData(Src, reinterpret_cast<AL_TMetaData*>(pPictureMetaTP));
    assert(success);
  }
  AL_LookAheadMetaData_Reset(pPictureMetaTP);
  return pPictureMetaTP;
}

/***************************************************************************/
bool AL_TwoPassMngr_HasLookAhead(AL_TEncSettings const& settings)
{
  return settings.LookAhead > 0;
}

/***************************************************************************/
void AL_TwoPassMngr_SetPass1Settings(AL_TEncSettings& settings)
{
  settings.NumLayer = 1;
  auto& channel = settings.tChParam[0];
  channel.tRCParam.eRCMode = AL_RC_CONST_QP;
  channel.tRCParam.iInitialQP = 20;
  channel.tGopParam.eMode = AL_GOP_MODE_LOW_DELAY_P;
  channel.tGopParam.uGopLength = 0;
  channel.tGopParam.uNumB = 0;
  channel.bSubframeLatency = false;
  channel.eLdaCtrlMode = DEFAULT_LDA;

  if(settings.bEnableFirstPassSceneChangeDetection)
  {
    channel.eEncOptions = (AL_EChEncOption)(channel.eEncOptions | AL_OPT_SCENE_CHANGE_DETECTION);
    channel.uNumSlices = 1;
  }
}

/***************************************************************************/
static bool DetectPatternTwoFrames(vector<int> v)
{
  if(v.size() < 5)
    return false;

  int nb_zero = 0, ecart = 0, ecart_max = 0;

  for(int i = 1; i < (int)v.size(); i++)
  {
    if(v[i] == 0)
    {
      nb_zero++;
      ecart_max = max(ecart_max, ecart);
      ecart = 0;
    }
    else
      ecart++;
  }

  return ecart_max == 1 && nb_zero >= ((int)v.size() - 1) / 2;
}

/***************************************************************************/
/*Offline TwoPass methods*/
/***************************************************************************/
TwoPassMngr::TwoPassMngr(std::string p_FileName, int p_iPass, bool p_bEnabledFirstPassSceneChangeDetection, int p_iGopSize, int p_iCpbLevel, int p_iInitialLevel, int p_iFrameRate) :
  iPass(p_iPass), bEnableFirstPassSceneChangeDetection(p_bEnabledFirstPassSceneChangeDetection), iGopSize(p_iGopSize),
  iCpbLevel(p_iCpbLevel), iInitialLevel(p_iInitialLevel), iFrameRate(p_iFrameRate)
{
  FileName = { p_FileName };
  tFrames.clear();
}

/***************************************************************************/
TwoPassMngr::~TwoPassMngr()
{
  tFrames.clear();
  CloseLog();
}

/***************************************************************************/
void TwoPassMngr::OpenLog()
{
  if(iPass == 1)
  {
    outputFile.open(FileName);

    if(!outputFile.is_open())
      throw runtime_error("Can't open TwoPass LogFile");
  }

  if(iPass == 2)
  {
    inputFile.open(FileName);

    if(!inputFile.is_open())
      throw runtime_error("Can't open TwoPass LogFile");
  }
}

/***************************************************************************/
void TwoPassMngr::CloseLog()
{
  inputFile.close();
  outputFile.close();
}

/***************************************************************************/
void TwoPassMngr::EmptyLog()
{
  if(!inputFile.is_open())
    OpenLog();

  tFrames.clear();

  char sLine[256];
  bool bFind = true;
  int i = 0;

  while(bFind && !inputFile.eof() && i < SEQUENCE_SIZE_MAX)
  {
    inputFile.getline(sLine, 256);

    auto str_PicSize = strtok(sLine, " ");
    auto str_PercentIntra = strtok(NULL, " ");

    bFind = (str_PicSize != NULL && str_PercentIntra != NULL);

    if(!bFind)
      break;

    AddNewFrame(atoi(str_PicSize), atoi(str_PercentIntra));
    i++;
  }

  ComputeTwoPass();
}

/***************************************************************************/
void TwoPassMngr::FillLog()
{
  if(!outputFile.is_open())
    OpenLog();

  for(auto frame: tFrames)
    outputFile << frame.iPictureSize << " " << static_cast<int>(frame.iPercentIntra[0]) << endl;

  tFrames.clear();
}

/***************************************************************************/
void TwoPassMngr::AddNewFrame(int iPictureSize, int iPercentIntra)
{
  AL_TLookAheadMetaData tParams;
  tParams.iPictureSize = iPictureSize;
  tParams.iPercentIntra[0] = iPercentIntra;
  tParams.iComplexity = 0;
  tParams.eSceneChange = AL_SC_NONE;
  tParams.iIPRatio = 1000;
  tFrames.push_back(tParams);
}

/***************************************************************************/
void TwoPassMngr::AddFrame(AL_TLookAheadMetaData* pMetaData)
{
  if(!pMetaData)
    throw runtime_error("[Pass 1] : No twopass Metadata during pass1, can't create logfile");

  tFrames.push_back(*pMetaData);

  if(static_cast<int>(tFrames.size()) >= SEQUENCE_SIZE_MAX)
    FillLog();
}

/***************************************************************************/
void TwoPassMngr::Flush()
{
  FillLog();
}

/***************************************************************************/
void TwoPassMngr::GetFrame(AL_TLookAheadMetaData* pMetaData)
{
  if(iCurrentFrame >= static_cast<int>(tFrames.size()))
  {
    EmptyLog();
    iCurrentFrame = 0;

    if(static_cast<int>(tFrames.size()) == 0)
      throw runtime_error("[Pass 2] : Not enough frames from pass 1 Logfile");
  }
  AL_LookAheadMetaData_Copy(&tFrames[iCurrentFrame], pMetaData);
  iCurrentFrame++;
}

/***************************************************************************/
void TwoPassMngr::ComputeTwoPass()
{
  auto iSequenceSize = static_cast<int>(tFrames.size());

  if(HasPatternTwoFrames())
  {
    for(int i = 0; i < iSequenceSize - 1; i++)
      tFrames[i].iPictureSize = 0;

    return;
  }

  for(int i = 0; i < iSequenceSize - 1; i++)
    tFrames[i].eSceneChange = SceneChangeDetected(&tFrames[i], &tFrames[i + 1]) ? AL_SC_NEXT : AL_SC_NONE;

  for(int i = 0; i < iSequenceSize - 1; i++)
  {
    tFrames[i].iIPRatio = GetIPRatio(&tFrames[i], &tFrames[i + 1]);

    for(int k = i + 2; k < min(iSequenceSize, i + 4) && !SceneChangeDetected(&tFrames[k - 1], &tFrames[k]); k++)
      tFrames[i].iIPRatio = min(tFrames[i].iIPRatio, GetIPRatio(&tFrames[i], &tFrames[k]));
  }

  ComputeComplexity();
}

/***************************************************************************/
void TwoPassMngr::ComputeComplexity()
{
  auto iSequenceSize = static_cast<int>(tFrames.size());
  assert(iSequenceSize > 0);
  assert(iSequenceSize <= SEQUENCE_SIZE_MAX);
  assert(iCpbLevel >= iInitialLevel);

  if(iGopSize == 0 || iSequenceSize == 0)
    return;

  size_t uSumCompGops = 0;
  int iIndex = 0, iNbGop = 0;

  while(iIndex < iSequenceSize)
  {
    int iLength = 0;
    size_t uSumComp = 0;

    while(iLength < iGopSize && iIndex + iLength < iSequenceSize)
    {
      uSumComp += tFrames[iIndex + iLength].iPictureSize;
      iLength++;

      if(tFrames[iIndex + iLength - 1].eSceneChange == AL_SC_NEXT)
        break;
    }

    int iComp = iLength ? uSumComp / iLength : uSumComp;

    for(int k = 0; k < iLength; k++)
      tFrames[iIndex + k].iComplexity = iComp;

    uSumCompGops += iComp;
    iNbGop++;
    iIndex += iLength;
  }

  int iMeanComp = uSumCompGops / iNbGop;

  int iLevel = 0, iLevelMax = 0, iLevelMin = 0;
  int iLimitMin = -iInitialLevel;
  int iLimitMax = (iCpbLevel - iInitialLevel);

  for(int i = 0; i < iSequenceSize; i++)
  {
    tFrames[i].iComplexity = (tFrames[i].iComplexity * 1000 / iMeanComp) - 1000;
    iLevel -= tFrames[i].iComplexity / iFrameRate;
    iLevelMax = max(iLevelMax, iLevel);
    iLevelMin = min(iLevelMin, iLevel);
  }

  int iCoeff = 1000;

  if(iLevelMax > 0)
    iCoeff = min(iCoeff, iLimitMax * 1000 / iLevelMax);

  if(iLevelMin < 0)
    iCoeff = min(iCoeff, iLimitMin * 900 / iLevelMin);

  for(int i = 0; i < iSequenceSize; i++)
    tFrames[i].iComplexity = (tFrames[i].iComplexity * iCoeff / 1000) + 1000;

  iIndex = 0;
  iLevel = iInitialLevel;

  while(iIndex < iSequenceSize)
  {
    int iLength = 0;

    while(iLength < iGopSize && iIndex + iLength < iSequenceSize)
    {
      iLength++;

      if(tFrames[iIndex + iLength - 1].eSceneChange == AL_SC_NEXT)
        break;
    }

    int iTarget = iLevel - iGopSize * (tFrames[iIndex].iComplexity - 1000) / iFrameRate;
    iLevel -= iLength * (tFrames[iIndex].iComplexity - 1000) / iFrameRate;

    for(int k = 0; k < iLength; k++)
      tFrames[iIndex + k].iTargetLevel = iTarget;

    iIndex += iLength;
  }
}

/***************************************************************************/
bool TwoPassMngr::HasPatternTwoFrames()
{
  vector<int> v = {};

  for(auto i = tFrames.begin(); i < tFrames.end(); i++)
    v.push_back(i->iPercentIntra[0]);

  return DetectPatternTwoFrames(v);
}

/***************************************************************************/
/*LookAhead structures and methods*/
/***************************************************************************/
LookAheadMngr::LookAheadMngr(int p_iLookAhead, bool p_bEnableFirstPassSceneChangeDetection) : uLookAheadSize(p_iLookAhead), bEnableFirstPassSceneChangeDetection(p_bEnableFirstPassSceneChangeDetection)
{
  iComplexity = 1000;
  iFrameCount = 0;
  iComplexityDiff = 0;
  bUseComplexity = (uLookAheadSize >= 10 && !bEnableFirstPassSceneChangeDetection);
  m_fifo.clear();

  for(int8_t i = 0; i < 5; i++)
    tPrevMetaData.iPercentIntra[i] = 100;

}

/***************************************************************************/
LookAheadMngr::~LookAheadMngr()
{
  m_fifo.clear();

}

/***************************************************************************/
bool LookAheadMngr::ComputeSceneChange(AL_TBuffer* pPrevSrc, AL_TBuffer* pCurrentSrc)
{
  if(!pPrevSrc || !pCurrentSrc)
    return false;

  auto pPreviousMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pPrevSrc, AL_META_TYPE_LOOKAHEAD));
  auto pCurrentMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pCurrentSrc, AL_META_TYPE_LOOKAHEAD));

  if(bEnableFirstPassSceneChangeDetection)
    return SceneChangeDetected_Crop(pPreviousMeta, pCurrentMeta);

  return SceneChangeDetected(pPreviousMeta, pCurrentMeta);
}

/***************************************************************************/
bool LookAheadMngr::ComputeSceneChange_LA1(AL_TBuffer* pCurrentSrc)
{
  if(!pCurrentSrc)
    return false;

  auto pCurrentMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pCurrentSrc, AL_META_TYPE_LOOKAHEAD));
  bool bDetected = false;

  if(bEnableFirstPassSceneChangeDetection)
    bDetected = SceneChangeDetected_Crop(&tPrevMetaData, pCurrentMeta);
  else
    bDetected = SceneChangeDetected(&tPrevMetaData, pCurrentMeta);

  tPrevMetaData = *pCurrentMeta;
  return bDetected;
}

/***************************************************************************/
int32_t LookAheadMngr::ComputeIPRatio(AL_TBuffer* pCurrentSrc, AL_TBuffer* pNextSrc)
{
  auto pCurrentMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pCurrentSrc, AL_META_TYPE_LOOKAHEAD));
  auto pNextMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pNextSrc, AL_META_TYPE_LOOKAHEAD));

  return GetIPRatio(pCurrentMeta, pNextMeta);
}

/***************************************************************************/
int LookAheadMngr::GetNextSceneChange()
{
  int iFifoSize = static_cast<int>(m_fifo.size());
  int iIndex = 0;

  while((iIndex + 1 < iFifoSize) && !ComputeSceneChange(m_fifo[iIndex], m_fifo[iIndex + 1]))
    iIndex++;

  if(iFifoSize < 2 || iIndex + 1 == iFifoSize)
    return iFifoSize;
  return iIndex + 1;
}

/***************************************************************************/
void LookAheadMngr::ProcessLookAheadParams()
{
  int iFifoSize = static_cast<int>(m_fifo.size());
  assert(iFifoSize > 0);

  auto pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(m_fifo[0], AL_META_TYPE_LOOKAHEAD);

  if(!pPictureMetaLA)
    return;

  if(uLookAheadSize == 1)
  {
    pPictureMetaLA->eSceneChange = ComputeSceneChange_LA1(m_fifo[0]) ? AL_SC_CURRENT : AL_SC_NONE;
    pPictureMetaLA->iPictureSize = 0;
    return;
  }

  if(bUseComplexity)
  {
    ComputeComplexity();
    pPictureMetaLA->iComplexity = iComplexity;
  }

  if(iFifoSize < 2)
    return;

  pPictureMetaLA->eSceneChange = ComputeSceneChange(m_fifo[0], m_fifo[1]) ? AL_SC_NEXT : AL_SC_NONE;

  if(bEnableFirstPassSceneChangeDetection)
  {
    pPictureMetaLA->iPictureSize = 0;
    return;
  }

  pPictureMetaLA->iIPRatio = ComputeIPRatio(m_fifo[0], m_fifo[1]);
  int iNextSceneChange = GetNextSceneChange();

  for(int i = 2; i < min(iNextSceneChange, 4); i++)
    pPictureMetaLA->iIPRatio = min(pPictureMetaLA->iIPRatio, ComputeIPRatio(m_fifo[0], m_fifo[i]));
}

/***************************************************************************/
void LookAheadMngr::ComputeComplexity()
{
  int iFifoSize = static_cast<int>(m_fifo.size());

  if(iFrameCount % 5 == 0)
  {
    iFrameCount = 0;
    iComplexity = 1000;

    if(iFifoSize >= 5 && AL_Buffer_GetMetaData(m_fifo.front(), AL_META_TYPE_LOOKAHEAD))
    {
      intmax_t iComp[2] = { 0, 0 };

      for(int i = 0; i < iFifoSize; i++)
      {
        auto pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(m_fifo[i], AL_META_TYPE_LOOKAHEAD);
        iComp[(i < 5) ? 0 : 1] += pPictureMetaLA->iPictureSize;
      }

      iComplexity = ((1000 * iFifoSize / 5) + iComplexityDiff) * iComp[0] / (iComp[0] + iComp[1]);
      iComplexity = min(3000, max(100, iComplexity));
      iComplexityDiff += (1000 - iComplexity);
    }
  }

  iFrameCount++;

  if(iFifoSize >= 2 && ComputeSceneChange(m_fifo[0], m_fifo[1]))
    iFrameCount = 0;
}

/***************************************************************************/
bool LookAheadMngr::HasPatternTwoFrames()
{
  vector<int> v = {};

  for(auto i = m_fifo.begin(); i < m_fifo.end(); i++)
  {
    auto pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(*i, AL_META_TYPE_LOOKAHEAD);
    v.push_back(pPictureMetaLA->iPercentIntra[0]);
  }

  return DetectPatternTwoFrames(v);
}


