/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sstream>
#include <fstream>

extern "C"
{
#include <lib_rtos/lib_rtos.h>
#include <lib_common_enc/EncBuffers.h>
}

#include "QPGenerator.h"
#include "ROIMngr.h"

using namespace std;

/****************************************************************************/
static AL_INLINE int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) & (~(iRnd - 1));
}

/****************************************************************************/
static int FromHex2(char a, char b)
{
  int A = ((a >= 'a') && (a <= 'f')) ? (a - 'a') + 10 :
          ((a >= 'A') && (a <= 'F')) ? (a - 'A') + 10 :
          ((a >= '0') && (a <= '9')) ? (a - '0') : 0;

  int B = ((b >= 'a') && (b <= 'f')) ? (b - 'a') + 10 :
          ((b >= 'A') && (b <= 'F')) ? (b - 'A') + 10 :
          ((b >= '0') && (b <= '9')) ? (b - '0') : 0;

  return (A << 4) + B;
}

/****************************************************************************/
static int FromHex4(char a, char b, char c, char d)
{
  return (FromHex2(a, b) << 8) + FromHex2(c, d);
}

/****************************************************************************/
void Generate_RampQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iMinQP, int iMaxQP)
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
    pQPs[iLCU] = (s_iCurSeg % 8);
    s_iCurSeg++;
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
void Generate_RandomQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;
  int iConvSliceQP = iSliceQP % 52;
  uint32_t iSeed = iNumLCUs * iConvSliceQP - (0xEFFACE << (iConvSliceQP >> 1)) + iRandQP++;
  uint32_t iRand = iSeed;
  int iRange = iMaxQP - iMinQP + 1;

  int16_t* pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; iSeg++)
  {
    iRand = (1103515245 * iRand + 12345); // Unix
    pSeg[iSeg] = (iMinQP + (iRand % iRange));
  }

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    iRand = (1103515245 * iRand + 12345); // Unix
    pQPs[iLCU] = (iRand % 8);
  }
}

/****************************************************************************/
void Generate_RandomQP(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;

  uint32_t iSeed = iNumLCUs * iSliceQP - (0xEFFACE << (iSliceQP >> 1)) + iRandQP++;
  uint32_t iRand = iSeed;

  int iRange = iMaxQP - iMinQP + 1;

  for(int iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      iRand = (1103515245 * iRand + 12345); // Unix
      pQPs[iFirst + iQP] = (iMinQP + (iRand % iRange)) & MASK_QP;
    }
  }
}

/****************************************************************************/
void Generate_BorderQP(uint8_t* pQPs, int iNumLCUs, int iLCUWidth, int iLCUHeight, int iNumQPPerLCU, int iNumBytesPerLCU, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  int iQP0 = bRelative ? 0 : iSliceQP;
  int iQP2 = iQP0 + 2;
  int iQP1;

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUWidth - 1;
  const int iLastX1 = iLCUWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUHeight - 1;
  const int iLastY1 = iLCUHeight - 2;

  int iFirstLCU = 0;
  int iLastLCU = iNumLCUs - 1;

  if(iQP2 > iMaxQP)
    iQP2 = iMaxQP;
  iQP1 = iQP0 + 1;

  if(iQP1 > iMaxQP)
    iQP1 = iMaxQP;

  for(int iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    int X = iLCU % iLCUWidth;
    int Y = iLCU / iLCUWidth;

    int iFirst = iNumBytesPerLCU * iLCU;

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
void Generate_BorderQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iLCUWidth, int iLCUHeight, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  int iQP0 = bRelative ? 0 : iSliceQP;
  int iQP2 = iQP0 + 10;
  int iQP1;

  int iFirstLCU = 0;
  int iLastLCU = iNumLCUs - 1;

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUWidth - 1;
  const int iLastX1 = iLCUWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUHeight - 1;
  const int iLastY1 = iLCUHeight - 2;
  int16_t* pSeg = (int16_t*)pSegs;

  if(iQP2 > iMaxQP)
    iQP2 = iMaxQP;
  iQP1 = iQP0 + 5;

  if(iQP1 > iMaxQP)
    iQP1 = iMaxQP;

  Rtos_Memset(pSeg, 0, 8 * sizeof(int16_t));
  pSeg[0] = iQP0;
  pSeg[1] = iQP1;
  pSeg[2] = iQP2;

  // write Map
  for(int iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    int X = iLCU % iLCUWidth;
    int Y = iLCU / iLCUWidth;

    if(X == iFirstX2 || Y == iFirstY2 || X == iLastX2 || Y >= iLastY2)
      pQPs[iLCU] = 2;
    else if(X == iFirstX1 || Y == iFirstY1 || X == iLastX1 || Y == iLastY1)
      pQPs[iLCU] = 1;
    else
      pQPs[iLCU] = 0;
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
#include <stdarg.h>
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

#define DEBUG_PATH "."

string createFileNameWithID(int iFrameID, string motif)
{
  ostringstream filename;
  filename << DEBUG_PATH << "/" << motif << "_" << iFrameID << ".hex";
  return filename.str();
}

string createQPFileName(string motif)
{
  ostringstream filename;
  filename << DEBUG_PATH << "/" << motif << ".hex";
  return filename.str();
}

void freeQPFileName(char* filename)
{
  free(filename);
}

/****************************************************************************/
static bool OpenFile(int iFrameID, string motif, ifstream& File)
{
  auto qpFileName = createFileNameWithID(iFrameID, motif);
  File.open(qpFileName);

  if(!File.is_open())
  {
    // Use default file (motif + "s" for backward compatibility)
    qpFileName = createQPFileName(motif + "s");
    File.open(qpFileName);
  }

  return File.is_open();
}

/****************************************************************************/
bool Load_QPTable_FromFile_Vp9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iFrameID, bool bRelative)
{
  ifstream file;

  if(!OpenFile(iFrameID, "QP", file))
    return false;

  char sLine[256];
  int16_t* pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; ++iSeg)
  {
    int idx = (iSeg & 0x01) << 2;

    if(idx == 0)
      file.read(sLine, 256);
    pSeg[iSeg] = FromHex4(sLine[4 - idx], sLine[5 - idx], sLine[6 - idx], sLine[7 - idx]);

    if(!bRelative && (pSeg[iSeg] < 0 || pSeg[iSeg] > 255))
      pSeg[iSeg] = 255;
  }

  // read QPs
  ReadQPs(file, pQPs, iNumLCUs, 1, 1);

  return true;
}

/****************************************************************************/
bool Load_QPTable_FromFile(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iFrameID)
{
  ifstream file;

  if(!OpenFile(iFrameID, "QP", file))
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
  int iID = atoi(sLine + iPos);
  return iID;
}

/****************************************************************************/
static bool check_frame_id(char* sLine, int iFrameID)
{
  int iPos;

  if(get_motif(sLine, "frame", iPos))
  {
    int iID = get_id(sLine, iPos);

    if(iID == iFrameID)
      return true;
  }
  return false;
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

  return AL_ROI_QUALITY_MAX_ENUM;
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
  {
  }

  ++iPos;

  iValue2 = atoi(sLine + iPos);

  while(sLine[++iPos] != ',')
  {
  }

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
void Generate_BorderSkip(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iLCUWidth, int iLCUHeight)
{
  int H = iLCUHeight * 2 / 6;
  int W = iLCUWidth * 2 / 6;

  H *= H;
  W *= W;

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int X = (iLCU % iLCUWidth) - (iLCUWidth >> 1);
    int Y = (iLCU / iLCUWidth) - (iLCUHeight >> 1);

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
  int iLimitQP = iSliceQP % 52;
  int iSeed = iNumLCUs * iLimitQP - (0xEFFACE << (iLimitQP >> 1)) + iRandFlag;
  int iRand = iSeed;

  for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    if(!(pQPs[iFirst] & MASK_FORCE))
    {
      for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      {
        if((pQPs[iFirst + iQP] & MASK_FORCE) != (pQPs[iFirst] & MASK_FORCE)) // remove existing flag if different from depth 0
          pQPs[iFirst + iQP] &= ~MASK_FORCE;

        iRand = (1103515245 * iRand + 12345); // Random formula from Unix

        if(abs(iRand) % 100 <= iPercent)
          pQPs[iFirst + iQP] |= uFORCE;
      }
    }
  }
}

/****************************************************************************/
static void GetQPBufferParameters(int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int& iNumQPPerLCU, int& iNumBytesPerLCU, int& iNumLCUs, uint8_t* pQPs)
{
  (void)eProf;

#if AL_BLK16X16_QP_TABLE
  iNumQPPerLCU = AL_IS_HEVC(eProf) ? 5 : 1;
  iNumBytesPerLCU = AL_IS_HEVC(eProf) ? 8 : 1;
#else
  iNumQPPerLCU = 1;
  iNumBytesPerLCU = 1;
#endif

  iNumLCUs = iLCUWidth * iLCUHeight;
  int iSize = RoundUp(iNumLCUs * iNumBytesPerLCU, 128);

  assert(pQPs);
  Rtos_Memset(pQPs, 0, iSize);
}

/****************************************************************************/
bool GenerateROIBuffer(AL_TRoiMngrCtx* pRoiCtx, string const& sRoiFileName, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs)
{
  int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
  GetQPBufferParameters(iLCUWidth, iLCUHeight, eProf, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
  return Load_QPTable_FromRoiFile(pRoiCtx, sRoiFileName, pQPs, iFrameID, iNumQPPerLCU, iNumBytesPerLCU);
}


/****************************************************************************/
bool GenerateQPBuffer(AL_EQpCtrlMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs, uint8_t* pSegs)
{
  bool bRet = false;
  int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
  static int iRandFlag = 0;
  bool bIsVp9 = false;

  if(bIsVp9)
    Rtos_Memset(pSegs, 0, 8 * sizeof(int16_t));

  int iQPMode = eMode & 0x0F; // exclusive mode
  bool bRelative = (eMode & RELATIVE_QP) ? true : false;

  if(bRelative)
  {
    int iMinus = bIsVp9 ? 128 : 32;
    int iPlus = bIsVp9 ? 127 : 31;

    if(iQPMode == RANDOM_QP)
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
  GetQPBufferParameters(iLCUWidth, iLCUHeight, eProf, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
  /////////////////////////////////  QPs  /////////////////////////////////////
  switch(iQPMode)
  {
  case RAMP_QP:
  {
    bIsVp9 ? Generate_RampQP_VP9(pSegs, pQPs, iNumLCUs, iMinQP, iMaxQP) :
    Generate_RampQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case RANDOM_QP:
  {
    bIsVp9 ? Generate_RandomQP_VP9(pSegs, pQPs, iNumLCUs, iMinQP, iMaxQP, iSliceQP) :
    Generate_RandomQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP, iSliceQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case BORDER_QP:
  {
    bIsVp9 ? Generate_BorderQP_VP9(pSegs, pQPs, iNumLCUs, iLCUWidth, iLCUHeight, iMaxQP, iSliceQP, bRelative) :
    Generate_BorderQP(pQPs, iNumLCUs, iLCUWidth, iLCUHeight, iNumQPPerLCU, iNumBytesPerLCU, iMaxQP, iSliceQP, bRelative);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case LOAD_QP:
  {
    bRet = bIsVp9 ? Load_QPTable_FromFile_Vp9(pSegs, pQPs, iNumLCUs, iFrameID, bRelative) :
           Load_QPTable_FromFile(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iFrameID);
  } break;
  }

  // ------------------------------------------------------------------------
  if((eMode != UNIFORM_QP) && (iQPMode == UNIFORM_QP) && !bRelative)
  {
    int s;

    if(bIsVp9)
      for(s = 0; s < 8; ++s)
        pSegs[2 * s] = iSliceQP;

    else
      for(int iLCU = 0; iLCU < iNumLCUs; iLCU++)
      {
        int iFirst = iLCU * iNumBytesPerLCU;

        for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
          pQPs[iFirst + iQP] = iSliceQP;
      }
  }
  // ------------------------------------------------------------------------

  if(eMode & RANDOM_I_ONLY)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 20, MASK_FORCE_INTRA); // 20 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & RANDOM_SKIP)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 30, MASK_FORCE_MV0); // 30 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & FULL_SKIP)
  {
    Generate_FullSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);
    bRet = true;
  }
  else if(eMode & BORDER_SKIP)
  {
    Generate_BorderSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iLCUWidth, iLCUHeight);
    bRet = true;
  }

  return bRet;
}

/****************************************************************************/

