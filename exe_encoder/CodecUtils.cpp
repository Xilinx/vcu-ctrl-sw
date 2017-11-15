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

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <assert.h>
#include <limits.h>

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
  Message("\n\n> % 3d", iFrameNum);
#else
  Message("\r  Encoding picture #%-6d - ", iFrameNum);
#endif
  fflush(stdout);
}

/*****************************************************************************/
static int PictureSize(TYUVFileInfo FI)
{
  std::ifstream::pos_type iSize;
  switch(AL_GetChromaMode(FI.FourCC))
  {
  case CHROMA_MONO:
    iSize = FI.PictWidth * FI.PictHeight;
    break;
  case CHROMA_4_2_0:
    iSize = (FI.PictWidth * FI.PictHeight * 3) / 2;
    break;
  case CHROMA_4_2_2:
    iSize = FI.PictWidth * FI.PictHeight * 2;
    break;
  default:
    iSize = 0;
  }

  auto iPixSize = GetPixelSize(AL_GetBitDepth(FI.FourCC));

  return iSize * iPixSize;
}

/*****************************************************************************/
void GotoFirstPicture(TYUVFileInfo const& FI, std::ifstream& File, unsigned int iFirstPict)
{
  auto const iPictLen = PictureSize(FI);
  File.seekg(iPictLen * iFirstPict);
}

static uint32_t GetLumaRowSize(TFourCC fourCC, uint32_t uWidth)
{
  uint32_t uRowSizeLuma;
  uint8_t uBitDepth = AL_GetBitDepth(fourCC);

  if(AL_Is10bitPacked(fourCC))
    uRowSizeLuma = (uWidth + 2) / 3 * 4;
  else
    uRowSizeLuma = uWidth * GetPixelSize(uBitDepth);
  return uRowSizeLuma;
}

/*****************************************************************************/
static uint32_t ReadFileLumaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf));

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  assert((uint32_t)pSrcMeta->tPitches.iLuma >= uFileRowSize);

  if((uint32_t)pSrcMeta->tPitches.iLuma == uFileRowSize)
  {
    uint32_t uSize = uFileNumRow * uFileRowSize;
    File.read(pTmp, uSize);
    pTmp += uSize;
  }
  else
  {
    for(uint32_t h = 0; h < uFileNumRow; h++)
    {
      File.read(pTmp, uFileRowSize);
      Rtos_Memset(pTmp + uFileRowSize, 0x00, pSrcMeta->tPitches.iLuma - uFileRowSize);
      pTmp += pSrcMeta->tPitches.iLuma;
    }
  }

  if(bPadding && (pSrcMeta->tDim.iHeight & 15))
  {
    uint32_t uRowPadding = ((pSrcMeta->tDim.iHeight + 15) & ~15) - pSrcMeta->tDim.iHeight;
    Rtos_Memset(pTmp, 0x00, uRowPadding * pSrcMeta->tPitches.iLuma);
    uFileNumRow += uRowPadding;
  }
  return pSrcMeta->tPitches.iLuma * uFileNumRow;
}

/*****************************************************************************/
static uint32_t ReadFileChromaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf) + uOffset);
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uNumRowC = (AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;
  uint32_t uRowSizeC = uFileRowSize >> 1;

  assert((uint32_t)pSrcMeta->tPitches.iChroma >= uRowSizeC);

  if((uint32_t)pSrcMeta->tPitches.iChroma == uRowSizeC)
  {
    uint32_t uSize = uNumRowC * uRowSizeC;
    File.read(pTmp, uSize);
    pTmp += uSize;
  }
  else
  {
    for(uint32_t h = 0; h < uNumRowC; h++)
    {
      File.read(pTmp, uRowSizeC);
      Rtos_Memset(pTmp + uRowSizeC, 0x80, (pSrcMeta->tPitches.iChroma - uRowSizeC));
      pTmp += pSrcMeta->tPitches.iChroma;
    }
  }

  if(bPadding && (pSrcMeta->tDim.iHeight & 15))
  {
    uint32_t uRowPadding;

    if(AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0)
      uRowPadding = (((pSrcMeta->tDim.iHeight >> 1) + 7) & ~7) - (pSrcMeta->tDim.iHeight >> 1);
    else
      uRowPadding = ((pSrcMeta->tDim.iHeight + 15) & ~15) - pSrcMeta->tDim.iHeight;
    Rtos_Memset(pTmp, 0x80, uRowPadding * pSrcMeta->tPitches.iChroma);
    uNumRowC += uRowPadding;
  }

  return pSrcMeta->tPitches.iChroma * uNumRowC;
}

/*****************************************************************************/
static void ReadFileChromaSemiPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf) + uOffset);
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
  uint32_t uNumRowC = (AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;

  assert((uint32_t)pSrcMeta->tPitches.iChroma >= uFileRowSize);

  if((uint32_t)pSrcMeta->tPitches.iChroma == uFileRowSize)
  {
    File.read(pTmp, uFileRowSize * uNumRowC);
  }
  else
  {
    assert(!AL_Is10bitPacked(pSrcMeta->tFourCC)); // TODO padding for 10bit packed format

    for(uint32_t h = 0; h < uNumRowC; h++)
    {
      File.read(pTmp, uFileRowSize);
      Rtos_Memset(pTmp + uFileRowSize, 0x80, (pSrcMeta->tPitches.iChroma - uFileRowSize));
      pTmp += pSrcMeta->tPitches.iChroma;
    }
  }
}

/*****************************************************************************/
static void ReadFile(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  uint32_t uOffset = ReadFileLumaPlanar(File, pBuf, uFileRowSize, uFileNumRow);

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  if(AL_IsSemiPlanar(pSrcMeta->tFourCC))
  {
    ReadFileChromaSemiPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow);
  }
  else if(AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0 || AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_2)
  {
    uOffset += ReadFileChromaPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow); // Cb
    ReadFileChromaPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow); // Cr
  }
}

/*****************************************************************************/
bool IsConversionNeeded(TFourCC const& FourCC, AL_TPicFormat const& picFmt)
{
  return FourCC != AL_EncGetSrcFourCC(picFmt);
}

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop)
{
  if(!pBuf || !File.is_open())
    throw std::runtime_error("invalid argument");

  if((File.peek() == EOF) && !bLoop)
    return false;

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uRowSizeLuma = GetLumaRowSize(pSrcMeta->tFourCC, pSrcMeta->tDim.iWidth);

  ReadFile(File, pBuf, uRowSizeLuma, pSrcMeta->tDim.iHeight);

  if((File.rdstate() & std::ios::failbit) && bLoop)
  {
    File.clear();
    File.seekg(0, std::ios::beg);
    ReadFile(File, pBuf, uRowSizeLuma, pSrcMeta->tDim.iHeight);
  }

  if(File.rdstate() & std::ios::failbit)
    throw std::runtime_error("not enough data for a complete frame");

  return true;
}

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, const AL_TBuffer* pBuf, int iWidth, int iHeight)
{
  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  assert(iWidth <= pBufMeta->tDim.iWidth);
  assert(iHeight <= pBufMeta->tDim.iHeight);

  if(!File.is_open())
    return false;

  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf));
  int iSizePix = GetPixelSize(AL_GetBitDepth(pBufMeta->tFourCC));

  if(pBufMeta->tPitches.iLuma == iWidth * iSizePix)
  {
    uint32_t uSizeY = iWidth * iHeight * iSizePix;
    File.write(pTmp, uSizeY);
    pTmp += uSizeY;
  }
  else
  {
    for(int h = 0; h < iHeight; h++)
    {
      File.write(pTmp, iWidth * iSizePix);
      pTmp += pBufMeta->tPitches.iLuma;
    }
  }

  // 1 Interleaved Chroma plane
  if(AL_IsSemiPlanar(pBufMeta->tFourCC))
  {
    int iHeightC = (pBufMeta->tFourCC == FOURCC(NV12)) ? iHeight >> 1 : iHeight;

    if(pBufMeta->tPitches.iChroma == iWidth * iSizePix)
      File.write(pTmp, iWidth * iHeightC * iSizePix);
    else
    {
      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, iWidth * iSizePix);
        pTmp += pBufMeta->tPitches.iChroma;
      }
    }
  }
  // 2 Separated chroma plane
  else if(AL_GetChromaMode(pBufMeta->tFourCC) == CHROMA_4_2_0 || AL_GetChromaMode(pBufMeta->tFourCC) == CHROMA_4_2_2)
  {
    int iWidthC = iWidth >> 1;
    int iHeightC = (AL_GetChromaMode(pBufMeta->tFourCC) == CHROMA_4_2_0) ? iHeight >> 1 : iHeight;

    if(pBufMeta->tPitches.iChroma == iWidthC * iSizePix)
    {
      uint32_t uSizeC = iWidthC * iHeightC * iSizePix;
      File.write(pTmp, uSizeC);
      pTmp += uSizeC;
      File.write(pTmp, uSizeC);
    }
    else
    {
      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, iWidthC * iSizePix);
        pTmp += pBufMeta->tPitches.iChroma;
      }

      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, iWidthC * iSizePix);
        pTmp += pBufMeta->tPitches.iChroma;
      }
    }
  }
  return true;
}


/*****************************************************************************/
bool WriteCompFrame(std::ofstream& File, std::ofstream& MapFile, std::ofstream& MapLogFile, AL_TBuffer* pBuf, AL_EProfile eProfile)
{
  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  if(!File.is_open() || !MapFile.is_open())
    return false;

  uint32_t uWidth = pBufMeta->tDim.iWidth;
  uint32_t uHeight = pBufMeta->tDim.iHeight;

  if(AL_IS_AVC(eProfile))
  {
    uWidth = (uWidth + 15) & ~0xF;
    uHeight = (uHeight + 15) & ~0xF;
  }
  int iOffset = 0;
  uint8_t uBitDepth = AL_GetBitDepth(pBufMeta->tFourCC);
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pBufMeta->tFourCC);
  // Luma
  {
    int iNumRowY = (uHeight + 3) >> 2;
    int iRndNumRowY = ((uHeight + 63) & ~63) >> 2;
    int iRowSizeY = (((uWidth + 63) & ~63) << 2) * uBitDepth / 8;
    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf));

    for(int r = 0; r < iNumRowY; ++r)
    {
      File.write(pTmp, iRowSizeY);
      pTmp += iRowSizeY;
    }

    iOffset += iRndNumRowY * pBufMeta->tPitches.iLuma;
  }

  // Chroma
  if(eChromaMode == CHROMA_4_2_0)
  {
    int iNumRowC = (uHeight + 3) >> 3;
    int iRndNumRowC = ((uHeight + 63) & ~63) >> 3;
    int iRowSizeC = (((uWidth + 63) & ~63) << 2) * uBitDepth / 8;
    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf) + iOffset);

    for(int r = 0; r < iNumRowC; ++r)
    {
      File.write(pTmp, iRowSizeC);
      pTmp += iRowSizeC;
    }

    iOffset += iRndNumRowC * pBufMeta->tPitches.iChroma;
  }
  else if(eChromaMode == CHROMA_4_2_2)
  {
    int iNumRowC = (uHeight + 3) >> 2;
    int iRndNumRowC = ((uHeight + 63) & ~63) >> 2;
    int iRowSizeC = (((uWidth + 63) & ~63) << 2) * uBitDepth / 8;
    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf) + iOffset);

    for(int r = 0; r < iNumRowC; ++r)
    {
      File.write(pTmp, iRowSizeC);
      pTmp += iRowSizeC;
    }

    iOffset += iRndNumRowC * pBufMeta->tPitches.iChroma;
  }

  // Map
  {
    int iNumRowMap = (uHeight + 3) >> 2;
    int iRowSize = 32 * ((uWidth + 4095) >> 12);

    // Luma
    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf) + iOffset);
    MapFile.write(pTmp, iNumRowMap * iRowSize);

    // Chroma
    if(eChromaMode == CHROMA_4_2_0)
      MapFile.write(pTmp + iNumRowMap * iRowSize, iNumRowMap * iRowSize / 2);
    else if(eChromaMode == CHROMA_4_2_2)
      MapFile.write(pTmp + iNumRowMap * iRowSize, iNumRowMap * iRowSize);

    (void)MapLogFile;
  }

  return true;
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
      iX = stoi(sVal) * -1;
    }
    else if(sVal == "y_d:")
    {
      ss >> sVal;
      iY = stoi(sVal) * -1;
    }
    ss >> sVal;
  }
  while(!ss.rdbuf()->in_avail() == 0);

  if(File.fail())
    return UINT_MAX;

  return iFrame - 1;
}

/*****************************************************************************/
void WriteOneSection(std::ofstream& File, AL_TBuffer* pStream, int iSection)
{
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
int WriteStream(std::ofstream& HEVCFile, AL_TBuffer* pStream)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int iNumSectionWritten = 0;
  int iNumFrame = 0;
  int i = 0;

  while(i < pStreamMeta->uNumSection)
  {
    if(pStreamMeta->pSections[i].uFlags & SECTION_END_FRAME_FLAG)
      ++iNumFrame;
    WriteOneSection(HEVCFile, pStream, i++);
    ++iNumSectionWritten;
  }

  return iNumFrame;
}

