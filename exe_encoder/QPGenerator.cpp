/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>

extern "C"
{
#include <lib_rtos/lib_rtos.h>
#include <lib_common_enc/EncBuffers.h>
#include "lib_common/Error.h"
}

#include "QPGenerator.h"
#include "ROIMngr.h"

#include "lib_app/FileUtils.h"

using namespace std;

/****************************************************************************/
static inline int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) & (~(iRnd - 1));
}

/****************************************************************************/
static AL_ERR ReadQPs(ifstream& qpFile, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iQPTableDepth)
{
  string sLine;
  int iNumQPPerLine;
  (void)iQPTableDepth;
  iNumQPPerLine = (iNumQPPerLCU == 5) ? 5 : 1;
  int iNumDigit = iNumQPPerLine * 2;

  int iIdx = 0;

  for(int iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iNumBytesPerLCU * iLCU;
    int iNumLine = 0;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      if(iIdx == 0)
      {
        getline(qpFile, sLine);

        if(sLine.size() < uint32_t(iNumDigit))
          return AL_ERR_QPLOAD_NOT_ENOUGH_DATA;

        ++iNumLine;
      }

      int iHexVal = FromHex2(sLine[iNumDigit - 2 * iIdx - 2], sLine[iNumDigit - 2 * iIdx - 1]);

      if(iHexVal == FROM_HEX_ERROR)
        return AL_ERR_QPLOAD_DATA;

      pQPs[iFirst + iQP] = iHexVal;

      iIdx = (iIdx + 1) % iNumQPPerLine;
    }

  }

  return AL_SUCCESS;
}

#ifdef _MSC_VER
#include <cstdarg>
static inline
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
AL_ERR Load_QPTable_FromFile_AOM(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iQPTableDepth, const string& sQPTablesFolder, int iFrameID, bool bRelative)
{
  string sLine;
  ifstream file;

  if(!OpenFile(sQPTablesFolder, iFrameID, QPTablesMotif, file))
    return AL_ERR_CANNOT_OPEN_FILE;

  int16_t* pSeg = (int16_t*)pSegs;

  for(int iSeg = 0; iSeg < 8; ++iSeg)
  {
    int idx = (iSeg & 0x01) << 2;

    if(idx == 0)
    {
      getline(file, sLine);

      if(sLine.size() < 8)
        return AL_ERR_QPLOAD_NOT_ENOUGH_DATA;
    }

    int iHexVal = FromHex4(sLine[4 - idx], sLine[5 - idx], sLine[6 - idx], sLine[7 - idx]);

    if(iHexVal == FROM_HEX_ERROR)
      return AL_ERR_QPLOAD_DATA;

    pSeg[iSeg] = iHexVal;

    int16_t iMinQP = 1;
    int16_t iMaxQP = 255;

    if(bRelative)
    {
      int16_t iMaxDelta = iMaxQP - iMinQP;
      iMinQP = -iMaxDelta;
      iMaxQP = +iMaxDelta;
    }

    pSeg[iSeg] = max(iMinQP, min(iMaxQP, pSeg[iSeg]));
  }

  return ReadQPs(file, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iQPTableDepth);
}

/****************************************************************************/
AL_ERR Load_QPTable_FromFile(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iQPTableDepth, const string& sQPTablesFolder, int iFrameID)
{
  ifstream file;

  if(!OpenFile(sQPTablesFolder, iFrameID, QPTablesMotif, file))
    return AL_ERR_CANNOT_OPEN_FILE;

  // Warning: the LOAD_QP is not backward compatible
  return ReadQPs(file, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iQPTableDepth);
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
static string getStringOnKeyword(const string& sLine, int iPos)
{
  int iPos1 = sLine.find_first_not_of("\t= ", iPos);
  int iPos2 = sLine.find_first_of(", \n\r\0\t", iPos1);
  return sLine.substr(iPos1, iPos2 - iPos1);
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
static bool ReadRoiHdr(ifstream& RoiFile, int iFrameID, AL_ERoiQuality& eBkgQuality, AL_ERoiOrder& eRoiOrder, bool& bRoiDisable)
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

      bRoiDisable = get_motif(sLine, "RoiDisable", iPos);
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
static int GetLcuQpOffset(int iQPTableDepth)
{
  int iLcuQpOffset = 0;
  (void)iQPTableDepth;

  return iLcuQpOffset;
}

/****************************************************************************/
AL_ERR Load_QPTable_FromRoiFile(AL_TRoiMngrCtx* pCtx, string const& sRoiFileName, uint8_t* pQPs, int iFrameID, int iNumQPPerLCU, int iNumBytesPerLCU, int iQPTableDepth)
{
  ifstream file(sRoiFileName);

  if(!file.is_open())
    return AL_ERR_CANNOT_OPEN_FILE;

  bool bRoiDisable = false;
  bool bHasData = ReadRoiHdr(file, iFrameID, pCtx->eBkgQuality, pCtx->eOrder, bRoiDisable);

  if(bRoiDisable)
    return AL_ERR_ROI_DISABLE;

  if(bHasData)
  {
    AL_RoiMngr_Clear(pCtx);
    int iPosX = 0, iPosY = 0, iWidth = 0, iHeight = 0;
    AL_ERoiQuality eQuality;

    while(get_new_roi(file, iPosX, iPosY, iWidth, iHeight, eQuality))
      AL_RoiMngr_AddROI(pCtx, iPosX, iPosY, iWidth, iHeight, eQuality);
  }

  auto const iLcuQpOffset = GetLcuQpOffset(iQPTableDepth);
  AL_RoiMngr_FillBuff(pCtx, iNumQPPerLCU, iNumBytesPerLCU, pQPs, iLcuQpOffset);
  return AL_SUCCESS;
}

/****************************************************************************/
static void Set_Block_Feature(uint8_t* pQPs, int iNumLCUs, int iNumBytesPerLCU, int iQPTableDepth)
{
  uint8_t const uLambdaFactor = DEFAULT_LAMBDA_FACT; // fixed point with 5 decimal bits

  if(iQPTableDepth > 1)
    for(int iLcuIdx = 0; iLcuIdx < iNumLCUs * iNumBytesPerLCU; iLcuIdx += iNumBytesPerLCU)
      pQPs[iLcuIdx + 3] = uLambdaFactor;
}

/****************************************************************************/
static void GetQPBufferParameters(int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, uint8_t uLog2MaxCuSize, int iQPTableDepth, int& iNumQPPerLCU, int& iNumBytesPerLCU, int& iNumLCUs, uint8_t* pQPs)
{
  (void)eProf;
  (void)uLog2MaxCuSize;
  (void)iQPTableDepth;

  if(pQPs == nullptr)
    throw runtime_error("pQPs buffer must exist");

  iNumQPPerLCU = 1;
  iNumBytesPerLCU = 1;

  iNumLCUs = iLCUPicWidth * iLCUPicHeight;
  int const iSize = RoundUp(iNumLCUs * iNumBytesPerLCU, 128);

  Rtos_Memset(pQPs, 0, iSize);
}

/****************************************************************************/
AL_ERR GenerateROIBuffer(AL_TRoiMngrCtx* pRoiCtx, string const& sRoiFileName, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, uint8_t uLog2MaxCuSize, int iQPTableDepth, int iFrameID, uint8_t* pQPs)
{
  int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
  GetQPBufferParameters(iLCUPicWidth, iLCUPicHeight, eProf, uLog2MaxCuSize, iQPTableDepth, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
  return Load_QPTable_FromRoiFile(pRoiCtx, sRoiFileName, pQPs, iFrameID, iNumQPPerLCU, iNumBytesPerLCU, iQPTableDepth);
}

/****************************************************************************/
AL_ERR GenerateQPBuffer(AL_EGenerateQpMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, uint8_t uLog2MaxCuSize, int iQPTableDepth, const string& sQPTablesFolder, int iFrameID, uint8_t* pQPTable)
{
  (void)iSliceQP;
  (void)iMinQP;
  (void)iMaxQP;
  bool bRelative = (eMode & AL_GENERATE_RELATIVE_QP);

  if(((AL_EGenerateQpMode)(eMode & AL_GENERATE_QP_TABLE_MASK)) == AL_GENERATE_LOAD_QP)
  {
    uint8_t* pSegs = pQPTable;
    uint8_t* pQPs = pQPTable + AL_QPTABLE_SEGMENTS_SIZE;
    bool bIsAOM = false;

    int iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs;
    GetQPBufferParameters(iLCUPicWidth, iLCUPicHeight, eProf, uLog2MaxCuSize, iQPTableDepth, iNumQPPerLCU, iNumBytesPerLCU, iNumLCUs, pQPs);
    Set_Block_Feature(pQPs, iNumLCUs, iNumBytesPerLCU, iQPTableDepth);

    AL_ERR Err = bIsAOM ? Load_QPTable_FromFile_AOM(pSegs, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iQPTableDepth, sQPTablesFolder, iFrameID, bRelative) :
                 Load_QPTable_FromFile(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iQPTableDepth, sQPTablesFolder, iFrameID);

    return Err;
  }

  return AL_SUCCESS;
}

