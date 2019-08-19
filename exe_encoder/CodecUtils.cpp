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

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <cassert>
#include <climits>

extern "C"
{
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "CodecUtils.h"
#include "lib_app/utils.h"

/******************************************************************************/
void DisplayFrameStatus(int iFrameNum)
{
#if VERBOSE_MODE
  LogVerbose("\n\n> % 3d", iFrameNum);
#else
  LogVerbose("\r  Encoding picture #%-6d - ", iFrameNum);
#endif
  fflush(stdout);
}

/*****************************************************************************/
bool IsConversionNeeded(TFourCC const& FourCC, AL_TPicFormat const& picFmt)
{
  return FourCC != AL_GetFourCC(picFmt);
}

/*****************************************************************************/
unsigned int ReadNextFrame(std::ifstream& File)
{
  std::string sLine;

  getline(File, sLine);

  if(File.fail())
    return UINT_MAX;

  return atoi(sLine.c_str());
}

/*****************************************************************************/
#include <sstream>
unsigned int ReadNextFrameMV(std::ifstream& File, int& iX, int& iY)
{
  std::string sLine, sVal;
  int iFrame = 0;
  iX = iY = 0;
  getline(File, sLine);
  std::stringstream ss(sLine);
  ss >> sVal;

  do
  {
    if(sVal == "fr:")
    {
      ss >> sVal;
      iFrame = stoi(sVal);
    }
    else if(sVal == "x_d:")
    {
      ss >> sVal;
      iX = stoi(sVal);
    }
    else if(sVal == "y_d:")
    {
      ss >> sVal;
      iY = stoi(sVal);
    }
    ss >> sVal;
  }
  while(!(ss.rdbuf()->in_avail() == 0));

  if(File.fail())
    return UINT_MAX;

  return iFrame - 1;
}


/*****************************************************************************/
void WriteOneSection(std::ofstream& File, AL_TBuffer* pStream, int iSection, const AL_TEncChanParam* pChannelParam)
{
  (void)pChannelParam;

  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  AL_TStreamSection* pCurSection = &pStreamMeta->pSections[iSection];
  uint8_t* pData = AL_Buffer_GetData(pStream);

  if(pCurSection->uLength)
  {

    uint32_t uRemSize = pStream->zSize - pCurSection->uOffset;

    if(uRemSize < pCurSection->uLength)
    {
      File.write((char*)(pData + pCurSection->uOffset), uRemSize);
      File.write((char*)pData, pCurSection->uLength - uRemSize);
    }
    else
    {
      File.write((char*)(pData + pCurSection->uOffset), pCurSection->uLength);
    }
  }
}

/*****************************************************************************/

/*****************************************************************************/
static void FillSectionFillerData(AL_TBuffer* pStream, int iSection, const AL_TEncChanParam* pChannelParam)
{
  assert(AL_IS_AVC(pChannelParam->eProfile) || AL_IS_HEVC(pChannelParam->eProfile));

  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  AL_TStreamSection* pSection = &pStreamMeta->pSections[iSection];

  uint32_t uLength = pSection->uLength;
  assert(uLength > 4);

  uint8_t* pData = AL_Buffer_GetData(pStream) + pSection->uOffset;

  while(--uLength && (*pData != 0xFF))
    ++pData;

  if(uLength > 0)
    Rtos_Memset(pData, 0xFF, uLength);

  assert(pData[uLength] == 0x80);
}

/*****************************************************************************/
int WriteStream(std::ofstream& HEVCFile, AL_TBuffer* pStream, const AL_TEncSettings* pSettings)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  auto& tChParam = pSettings->tChParam[0];

  int iNumFrame = 0;

  for(int curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection)
  {
    if(pStreamMeta->pSections[curSection].uFlags & AL_SECTION_END_FRAME_FLAG)
      ++iNumFrame;

    if(pStreamMeta->pSections[curSection].uFlags & AL_SECTION_APP_FILLER_FLAG)
      FillSectionFillerData(pStream, curSection, &tChParam);

    WriteOneSection(HEVCFile, pStream, curSection, &tChParam);
  }

  return iNumFrame;
}

/*****************************************************************************/
void GetImageStreamSize(AL_TBuffer* pStream, std::deque<ImageSize>& imageSizes)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  if(imageSizes.empty())
    throw std::runtime_error("You need at least one empty image size structure to begin the first frame");

  for(int curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection)
  {
    AL_TStreamSection& section = pStreamMeta->pSections[curSection];

    if(section.uFlags & AL_SECTION_END_FRAME_FLAG)
    {
      imageSizes.back().finished = true;
      imageSizes.push_back(ImageSize { 0, false });
    }

    imageSizes.back().size += section.uLength;
  }
}

