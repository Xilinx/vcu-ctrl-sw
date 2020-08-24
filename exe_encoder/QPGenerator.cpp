/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>

extern "C"
{
#include <lib_rtos/lib_rtos.h>
#include <lib_common_enc/EncBuffers.h>
}

#include "QPGenerator.h"
#include "ROIMngr.h"

#include "FileUtils.h"

using namespace std;

/****************************************************************************/
// We need to have a deterministic seed to compare soft and hard
static int CreateSeed(int iNumLCUs, int iSliceQP, int iRandQP)
{
  return iNumLCUs * iSliceQP - (0xEFFACE << (iSliceQP >> 1)) + iRandQP;
}

/****************************************************************************/
static int random_int(int& iSeed, int iMin, int iMax)
{
  iSeed = 1103515245u * iSeed + 12345;
  int iRange = iMax - iMin + 1;
  return iMin + abs(iSeed % iRange);
}

/****************************************************************************/
static AL_INLINE int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) & (~(iRnd - 1));
}

/****************************************************************************/
void Generate_RampQP_AOM(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP)
{
  static int16_t s_iQP = 0;
  static uint8_t s_iCurSeg = 0;
  int iStepQP;
  int16_t* pSeg;

  if(s_iQP < iMinQP)
    s_iQP = iMinQP;

  iStepQP = (iMaxQP - iMinQP) >> 3;
  pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; iSeg++)
  {
    pSeg[iSeg] = (s_iQP += iStepQP) & 0xFF;

    if(s_iQP > iMaxQP)
      s_iQP = iMinQP;
  }

  s_iCurSeg = 0;

  for(int iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      pQPs[iFirst + iQP] = (s_iCurSeg % 8);
      s_iCurSeg++;
    }
  }
}

/****************************************************************************/
void Generate_RampQP(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP)
{
  static int8_t s_iQP = 0;

  if(s_iQP < iMinQP)
    s_iQP = iMinQP;

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = s_iQP & MASK_QP;

    if(++s_iQP > iMaxQP)
      s_iQP = iMinQP;
  }
}

/****************************************************************************/
void Generate_RandomQP_AOM(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;
  int iSeed = CreateSeed(iNumLCUs, iSliceQP % 52, iRandQP);
  ++iRandQP;

  int16_t* pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; iSeg++)
    pSeg[iSeg] = random_int(iSeed, iMinQP, iMaxQP);

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = random_int(iSeed, 0, 7);
  }
}

/****************************************************************************/
void Generate_RandomQP(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;
  int iSeed = CreateSeed(iNumLCUs, iSliceQP, iRandQP);
  ++iRandQP;

  for(int iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = random_int(iSeed, iMinQP, iMaxQP) & MASK_QP;
  }
}

/****************************************************************************/
void Generate_BorderQP(uint8_t* pQPs, int iNumLCUs, int iLCUPicWidth, int iLCUPicHeight, int iNumQPPerLCU, int iNumBytesPerLCU, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  const int iQP0 = bRelative ? 0 : iSliceQP;
  const int iQP2 = min(iQP0 + 2, iMaxQP);
  const int iQP1 = min(iQP0 + 1, iMaxQP);

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUPicWidth - 1;
  const int iLastX1 = iLCUPicWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUPicHeight - 1;
  const int iLastY1 = iLCUPicHeight - 2;

  const int iFirstLCU = 0;
  const int iLastLCU = iNumLCUs - 1;

  for(int iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    const int X = iLCU % iLCUPicWidth;
    const int Y = iLCU / iLCUPicWidth;

    const int iFirst = iNumBytesPerLCU * iLCU;

    if(X == iFirstX2 || Y == iFirstY2 || X == iLastX2 || Y >= iLastY2)
      pQPs[iFirst] = iQP2;
    else if(X == iFirstX1 || Y == iFirstY1 || X == iLastX1 || Y == iLastY1)
      pQPs[iFirst] = iQP1;
    else
      pQPs[iFirst] = iQP0;

    for(int iQP = 1; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = pQPs[iFirst];
  }
}

/****************************************************************************/
void Generate_BorderQP_AOM(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iLCUPicWidth, int iLCUPicHeight, int iNumQPPerLCU, int iNumBytesPerLCU, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  const int iQP0 = bRelative ? 0 : iSliceQP;
  const int iQP2 = min(iQP0 + 10, iMaxQP);
  const int iQP1 = min(iQP0 + 5, iMaxQP);

  const int iFirstLCU = 0;
  const int iLastLCU = iNumLCUs - 1;

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUPicWidth - 1;
  const int iLastX1 = iLCUPicWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUPicHeight - 1;
  const int iLastY1 = iLCUPicHeight - 2;
  int16_t* pSeg = (int16_t*)pSegs;

  Rtos_Memset(pSeg, 0, 8 * sizeof(int16_t));
  pSeg[0] = iQP0;
  pSeg[1] = iQP1;
  pSeg[2] = iQP2;

  // write Map
  for(int iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    const int X = iLCU % iLCUPicWidth;
    const int Y = iLCU / iLCUPicWidth;

    const int iFirst = iNumBytesPerLCU * iLCU;

    if(X == iFirstX2 || Y == iFirstY2 || X == iLastX2 || Y >= iLastY2)
      pQPs[iLCU] = 2;
    else if(X == iFirstX1 || Y == iFirstY1 || X == iLastX1 || Y == iLastY1)
      pQPs[iLCU] = 1;
    else
      pQPs[iLCU] = 0;

    for(int iQP = 1; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = pQPs[iFirst];
  }
}

/****************************************************************************/
static void ReadQPs(ifstream& qpFile, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU)
{
  string sLine;

  int iNumQPPerLine = (iNumQPPerLCU == 5) ? 5 : 1;
  int iNumDigit = iNumQPPerLine * 2;

  int iIdx = 0;

  for(int iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      if(iIdx == 0)
        getline(qpFile, sLine);

      pQPs[iFirst + iQP] = FromHex2(sLine[iNumDigit - 2 * iIdx - 2], sLine[iNumDigit - 2 * iIdx - 1]);

      iIdx = (iIdx + 1) % iNumQPPerLine;
    }
  }
}

#ifdef _MSC_VER
#include <cstdarg>
static AL_INLINE
int LIBSYS_SNPRINTF(char* str, size_t size, const char* format, ...)
{
  int retval;
  va_list ap;
  va_start(ap, format);
  retval = _vsnprintf(str, size, format, ap);
  va_end(ap);
  return retval;
}

#define snprintf LIBSYS_SNPRINTF
#endif

static const string DefaultQPTablesFolder = ".";
static const string QPTablesMotif = "QP";
static const string QPTablesLegacyMotif = "QPs"; // Use default file (motif + "s" for backward compatibility)
static const string QPTablesExtension = ".hex";

string createQPFileName(const string& folder, const string& motif)
{
  return combinePath(folder, motif + QPTablesExtension);
}

/****************************************************************************/
static bool OpenFile(const string& sQPTablesFolder, int iFrameID, string motif, ifstream& File)
{
  string sFileFolder = sQPTablesFolder.empty() ? DefaultQPTablesFolder : sQPTablesFolder;

  auto qpFileName = createFileNameWithID(sFileFolder, motif, QPTablesExtension, iFrameID);
  File.open(qpFileName);

  if(!File.is_open())
  {
    qpFileName = createQPFileName(sFileFolder, QPTablesLegacyMotif);
    File.open(qpFileName);
  }

  return File.is_open();
}

/****************************************************************************/
bool Load_QPTable_FromFile_AOM(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, const string& sQPTablesFolder, int iFrameID, bool bRelative)
{
  string sLine;
  ifstream file;

  if(!OpenFile(sQPTablesFolder, iFrameID, QPTablesMotif, file))
    return false;

  int16_t* pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; ++iSeg)
  {
    int idx = (iSeg & 0x01) << 2;

    if(idx == 0)
      getline(file, sLine);

    pSeg[iSeg] = FromHex4(sLine[4 - idx], sLine[5 - idx], sLine[6 - idx], sLine[7 - idx]);

    if(!bRelative && (pSeg[iSeg] < 0 || pSeg[iSeg] > 255))
      pSeg[iSeg] = 255;
  }

  // read QPs
  ReadQPs(file, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);

  return true;
}

/****************************************************************************/
bool Load_QPTable_FromFile(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, const string& sQPTablesFolder, int iFrameID)
{
  ifstream file;

  if(!OpenFile(sQPTablesFolder, iFrameID, QPTablesMotif, file))
    return false;

  // Warning : the LOAD_QP is not backward compatible
  ReadQPs(file, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);

  return true;
}

/****************************************************************************/
static bool get_motif(char* sLine, string motif, int& iPos)
{
  int iState = 0;
  int iNumState = motif.length();
  iPos = 0;

  while(iState < iNumState && sLine[iPos] != '\0')
  {
    if(sLine[iPos] == motif[iState])
      ++iState;
    else
      iState = 0;

    ++iPos;
  }

  return iState == iNumState;
}

/****************************************************************************/
static int get_id(char* sLine, int iPos)
{
  return atoi(sLine + iPos);
}

/****************************************************************************/
static bool check_frame_id(char* sLine, int iFrameID)
{
  int iPos;

  if(!get_motif(sLine, "frame", iPos))
    return false;
  int iID = get_id(sLine, iPos);

  return iID == iFrameID;
}

/****************************************************************************/
string getStringOnKeyword(char* sLine, int iPos)
{
  while(sLine[iPos] == ' ' || sLine[iPos] == '\t' || sLine[iPos] == '=')
    ++iPos;

  int iLength = 0;

  while(sLine[iPos + iLength] != ',' && sLine[iPos + iLength] != '\0')
    ++iLength;

  return string(sLine + iPos, iLength);
}

/****************************************************************************/
static AL_ERoiQuality get_roi_quality(char* sLine, int iPos)
{
  auto s = getStringOnKeyword(sLine, iPos);

  if(s == "HIGH_QUALITY")
    return AL_ROI_QUALITY_HIGH;

  if(s == "MEDIUM_QUALITY")
    return AL_ROI_QUALITY_MEDIUM;

  if(s == "LOW_QUALITY")
    return AL_ROI_QUALITY_LOW;

  if(s == "NO_QUALITY")
    return AL_ROI_QUALITY_DONT_CARE;

  if(s == "INTRA_QUALITY")
    return AL_ROI_QUALITY_INTRA;

  std::stringstream ss {};
  ss.str(s);
  int i;
  ss >> i;
  return static_cast<AL_ERoiQuality>(i);
}

/****************************************************************************/
static AL_ERoiOrder get_roi_order(char* sLine, int iPos)
{
  auto s = getStringOnKeyword(sLine, iPos);

  if(s.compare("INCOMING_ORDER") == 0)
    return AL_ROI_INCOMING_ORDER;
  return AL_ROI_QUALITY_ORDER;
}

/****************************************************************************/
static bool ReadRoiHdr(ifstream& RoiFile, int iFrameID, AL_ERoiQuality& eBkgQuality, AL_ERoiOrder& eRoiOrder)
{
  char sLine[256];
  bool bFind = false;

  while(!bFind && !RoiFile.eof())
  {
    RoiFile.getline(sLine, 256);
    bFind = check_frame_id(sLine, iFrameID);

    if(bFind)
    {
      int iPos;

      if(get_motif(sLine, "BkgQuality", iPos))
        eBkgQuality = get_roi_quality(sLine, iPos);

      if(get_motif(sLine, "Order", iPos))
        eRoiOrder = get_roi_order(sLine, iPos);
    }
  }

  return bFind;
}

/****************************************************************************/
static bool line_is_empty(char* sLine)
{
  int iPos = 0;

  while(sLine[iPos] != '\0')
  {
    if((sLine[iPos] >= 'a' && sLine[iPos] <= 'z') || (sLine[iPos] >= 'A' && sLine[iPos] <= 'Z') || (sLine[iPos] >= '0' && sLine[iPos] <= '9'))
      return false;
    ++iPos;
  }

  return true;
}

/****************************************************************************/
static void get_dual_value(char* sLine, char separator, int& iPos, int& iValue1, int& iValue2)
{
  iValue1 = atoi(sLine + iPos);

  while(sLine[++iPos] != separator)
    ;

  ++iPos;

  iValue2 = atoi(sLine + iPos);

  while(sLine[++iPos] != ',')
    ;

  ++iPos;
}

/****************************************************************************/
static bool get_new_roi(ifstream& RoiFile, int& iPosX, int& iPosY, int& iWidth, int& iHeight, AL_ERoiQuality& eQuality)
{
  char sLine[256];
  bool bFind = false;

  while(!RoiFile.eof())
  {
    RoiFile.getline(sLine, 256);
    int iPos;

    if(get_motif(sLine, "frame", iPos))
      return bFind;

    if(!line_is_empty(sLine))
    {
      iPos = 0;
      get_dual_value(sLine, ':', iPos, iPosX, iPosY);
      get_dual_value(sLine, 'x', iPos, iWidth, iHeight);
      eQuality = get_roi_quality(sLine, iPos);
      return true;
    }
  }

  return false;
}

/****************************************************************************/
bool Load_QPTable_FromRoiFile(AL_TRoiMngrCtx* pCtx, string const& sRoiFileName, uint8_t* pQPs, int iFrameID, int iNumQPPerLCU, int iNumBytesPerLCU)
{
  ifstream file(sRoiFileName);

  if(!file.is_open())
    return false;

  bool bHasData = ReadRoiHdr(file, iFrameID, pCtx->eBkgQuality, pCtx->eOrder);

  if(bHasData)
  {
    AL_RoiMngr_Clear(pCtx);
    int iPosX = 0, iPosY = 0, iWidth = 0, iHeight = 0;
    AL_ERoiQuality eQuality;

    while(get_new_roi(file, iPosX, iPosY, iWidth, iHeight, eQuality))
      AL_RoiMngr_AddROI(pCtx, iPosX, iPosY, iWidth, iHeight, eQuality);
  }
  AL_RoiMngr_FillBuff(pCtx, iNumQPPerLCU, iNumBytesPerLCU, pQPs);
  return true;
}

/****************************************************************************/
void Generate_FullSkip(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU)
{
  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      pQPs[iFirst + iQP] &= ~MASK_FORCE;
      pQPs[iFirst + iQP] |= MASK_FORCE_MV0;
    }
  }
}

/****************************************************************************/
void Generate_BorderSkip(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iLCUPicWidth, int iLCUPicHeight)
{
  int H = iLCUPicHeight * 2 / 6;
  int W = iLCUPicWidth * 2 / 6;

  H *= H;
  W *= W;

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int X = (iLCU % iLCUPicWidth) - (iLCUPicWidth >> 1);
    int Y = (iLCU / iLCUPicWidth) - (iLCUPicHeight >> 1);

    int iFirst = iNumBytesPerLCU * iLCU;

    if(100 * X * X / W + 100 * Y * Y / H > 100)
    {
      pQPs[iFirst] &= ~MASK_FORCE;
      pQPs[iFirst] |= MASK_FORCE_MV0;
    }

    for(int iQP = 1; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = pQPs[iFirst];
  }
}

/****************************************************************************/
void Generate_Random_WithFlag(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int16_t iSliceQP, int iRandFlag, int iPercent, uint8_t uFORCE)
{
  int iSeed = CreateSeed(iNumLCUs, iSliceQP % 52, iRandFlag);

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    if(!(pQPs[iFirst] & MASK_FORCE))
    {
      for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      {
        if((pQPs[iFirst + iQP] & MASK_FORCE) != (pQPs[iFirst] & MASK_FORCE)) // remove existing flag if different from depth 0
          pQPs[iFirst + iQP] &= ~MASK_FORCE;

        if(random_int(iSeed, 0, 99) <= iPercent)
          pQPs[iFirst + iQP] |= uFORCE;
      }
    }
  }
}

/****************************************************************************/
static void GetQPBufferParameters(int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, int& iNumQPPerLCU, int& iNumBytesPerLCU, int& iNumLCUs, uint8_t* pQPs)
{
  (void)eProf;
  iNumQPPerLCU = 1;
  iNumBytesPerLCU = 1;

#if AL_BLK16X16_QP_TABLE

  if(!AL_IS_AVC(eProf))
  {
    iNumQPPerLCU = 5;
    iNumBytesPerLCU = 8;
  }
#endif

  iNumLCUs = iLCUPicWidth * iLCUPicHeight;
  int iSize = RoundUp(iNumLCUs * iNumBytesPerLCU, 128);

  assert(pQPs);
  Rtos_Memset(pQPs, 0, iSize);
}

/****************************************************************************/
bool GenerateROIBuffer(AL_TRoiMngrCtx* pRoiCtx, string const& sRoiFileName, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs)
{
  int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
  GetQPBufferParameters(iLCUPicWidth, iLCUPicHeight, eProf, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
  return Load_QPTable_FromRoiFile(pRoiCtx, sRoiFileName, pQPs, iFrameID, iNumQPPerLCU, iNumBytesPerLCU);
}

/****************************************************************************/
bool GenerateQPBuffer(AL_EGenerateQpMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, const string& sQPTablesFolder, int iFrameID, uint8_t* pQPs, uint8_t* pSegs)
{
  bool bRet = false;
  static int iRandFlag = 0;
  bool bIsAOM = false;

  AL_EGenerateQpMode eQPMode = (AL_EGenerateQpMode)(eMode & AL_GENERATE_MASK_QP_TABLE);
  bool bRelative = (eMode & AL_GENERATE_RELATIVE_QP);

  if(bRelative)
  {
    int iMinus = bIsAOM ? 128 : 32;
    int iPlus = bIsAOM ? 127 : 31;

    if(eQPMode == AL_GENERATE_RANDOM_QP && !bIsAOM)
    {
      iMinQP = -iMinus;
      iMaxQP = iPlus;
    }
    else
    {
      iMinQP = (iSliceQP - iMinus < iMinQP) ? iMinQP - iSliceQP : -iMinus;
      iMaxQP = (iSliceQP + iPlus > iMaxQP) ? iMaxQP - iSliceQP : iPlus;
    }
  }

  int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
  GetQPBufferParameters(iLCUPicWidth, iLCUPicHeight, eProf, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
  /////////////////////////////////  QPs  /////////////////////////////////////
  switch(eQPMode)
  {
  case AL_GENERATE_RAMP_QP:
  {
    bIsAOM ? Generate_RampQP_AOM(pSegs, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP) :
    Generate_RampQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case AL_GENERATE_RANDOM_QP:
  {
    bIsAOM ? Generate_RandomQP_AOM(pSegs, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP, iSliceQP) :
    Generate_RandomQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP, iSliceQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case AL_GENERATE_BORDER_QP:
  {
    bIsAOM ? Generate_BorderQP_AOM(pSegs, pQPs, iNumLCUs, iLCUPicWidth, iLCUPicHeight, iNumQPPerLCU, iNumBytesPerLCU, iMaxQP, iSliceQP, bRelative) :
    Generate_BorderQP(pQPs, iNumLCUs, iLCUPicWidth, iLCUPicHeight, iNumQPPerLCU, iNumBytesPerLCU, iMaxQP, iSliceQP, bRelative);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case AL_GENERATE_LOAD_QP:
  {
    bRet = bIsAOM ? Load_QPTable_FromFile_AOM(pSegs, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, sQPTablesFolder, iFrameID, bRelative) :
           Load_QPTable_FromFile(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, sQPTablesFolder, iFrameID);
  } break;
  default: break;
  }

  // ------------------------------------------------------------------------
  if((eMode != AL_GENERATE_UNIFORM_QP) && (eQPMode == AL_GENERATE_UNIFORM_QP) && !bRelative)
  {
    if(bIsAOM)
    {
      for(int s = 0; s < 8; ++s)
        pSegs[2 * s] = iSliceQP;
    }
    else
    {
      for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
      {
        int iFirst = iLCU * iNumBytesPerLCU;

        for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
          pQPs[iFirst + iQP] = iSliceQP;
      }
    }
  }
  // ------------------------------------------------------------------------

  if(eMode & AL_GENERATE_RANDOM_I_ONLY)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 20, MASK_FORCE_INTRA); // 20 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & AL_GENERATE_RANDOM_SKIP)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 30, MASK_FORCE_MV0); // 30 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & AL_GENERATE_FULL_SKIP)
  {
    Generate_FullSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);
    bRet = true;
  }
  else if(eMode & AL_GENERATE_BORDER_SKIP)
  {
    Generate_BorderSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iLCUPicWidth, iLCUPicHeight);
    bRet = true;
  }

  return bRet;
}

/****************************************************************************/

