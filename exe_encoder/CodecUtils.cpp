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

static uint32_t GetFileLumaSize(AL_TSrcMetaData* pSrcMeta, uint32_t uRowSize)
{
  return pSrcMeta->iHeight * uRowSize;
}

static uint32_t GetFileChromaSize(AL_TSrcMetaData* pSrcMeta, uint32_t uRowSize)
{
  uint32_t uFileNumRow = pSrcMeta->iHeight;
  uint32_t uNumRowC = (AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;
  return uRowSize * uNumRowC;
}

bool IsLastFrameInFile(std::ifstream& File, AL_TBuffer* pBuf)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
  uint32_t uRowSize = GetLumaRowSize(pSrcMeta->tFourCC, pSrcMeta->iWidth);
  uint32_t uSize = GetFileLumaSize(pSrcMeta, uRowSize);
  uSize += GetFileChromaSize(pSrcMeta, uRowSize);

  auto curPos = File.tellg();

  File.seekg(uSize - 1, std::ios::cur);
  File.peek();
  bool eof = File.eof();
  File.seekg(curPos, std::ios::beg);
  return eof;
}

/*****************************************************************************/
static uint32_t ReadFileLumaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(pBuf->pData);

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

  if(bPadding && (pSrcMeta->iHeight & 15))
  {
    uint32_t uRowPadding = ((pSrcMeta->iHeight + 15) & ~15) - pSrcMeta->iHeight;
    Rtos_Memset(pTmp, 0x00, uRowPadding * pSrcMeta->tPitches.iLuma);
    uFileNumRow += uRowPadding;
  }
  return pSrcMeta->tPitches.iLuma * uFileNumRow;
}

/*****************************************************************************/
static uint32_t ReadFileChromaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(pBuf->pData + uOffset);
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

  if(bPadding && (pSrcMeta->iHeight & 15))
  {
    uint32_t uRowPadding;

    if(AL_GetChromaMode(pSrcMeta->tFourCC) == CHROMA_4_2_0)
      uRowPadding = (((pSrcMeta->iHeight >> 1) + 7) & ~7) - (pSrcMeta->iHeight >> 1);
    else
      uRowPadding = ((pSrcMeta->iHeight + 15) & ~15) - pSrcMeta->iHeight;
    Rtos_Memset(pTmp, 0x80, uRowPadding * pSrcMeta->tPitches.iChroma);
    uNumRowC += uRowPadding;
  }

  return pSrcMeta->tPitches.iChroma * uNumRowC;
}

/*****************************************************************************/
static void ReadFileChromaSemiPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  char* pTmp = reinterpret_cast<char*>(pBuf->pData + uOffset);
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
bool IsConversionNeeded(TFourCC const& FourCC, AL_EChromaMode eEncChromaMode, uint8_t uBitDepth)
{
  return FourCC != AL_GetSrcFourCC(eEncChromaMode, uBitDepth);
}

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop)
{
  if(!pBuf || !File.is_open())
    return false;

  if((File.peek() == EOF) && !bLoop)
    return false;

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uRowSizeLuma = GetLumaRowSize(pSrcMeta->tFourCC, pSrcMeta->iWidth);

  ReadFile(File, pBuf, uRowSizeLuma, pSrcMeta->iHeight);

  if((File.rdstate() & std::ios::failbit) && bLoop)
  {
    File.clear();
    File.seekg(0, std::ios::beg);
    ReadFile(File, pBuf, uRowSizeLuma, pSrcMeta->iHeight);
  }

  bool bRet = ((File.rdstate() & std::ios::failbit) == 0);

  if(!bRet)
    throw std::runtime_error("not enough data for a complete frame");

  return bRet;
}

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, const AL_TBuffer* pBuf, int iWidth, int iHeight)
{
  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  assert(iWidth <= pBufMeta->iWidth);
  assert(iHeight <= pBufMeta->iHeight);

  if(!File.is_open())
    return false;

  char* pTmp = reinterpret_cast<char*>(pBuf->pData);
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

#if AL_ENABLE_FBC_LOG
/*****************************************************************************/
static void ReadMap(uint8_t* pMap, uint32_t uBlockNum, uint8_t& uBurstSize)
{
  uBurstSize = (pMap[uBlockNum >> 1] >> ((uBlockNum & 1) << 2)) & 15; // 4 bits per block, returns number of 32-byte bursts
}

/****************************************************************************/
static void TraceFBCMap(std::ofstream& MapLogFile, uint8_t* pBufMap, int iWidth, int iHeight, AL_EChromaMode eChromaMode, bool bTenBpc)
{
  if(MapLogFile.is_open() && pBufMap)
  {
    uint8_t uBurstSize = 0;
    uint32_t uTotalBlockSize = 0;
    uint32_t uTotalBlocks = 0;

    for(int h = 0; h < iHeight; h += 4)
    {
      for(int w = 0; w < iWidth; w += 64)
      {
        ReadMap(pBufMap, (h >> 2) * 64 * ((iWidth + 4095) >> 12) + (w >> 6), uBurstSize);

        if(uBurstSize)
        {
          uTotalBlocks++;
          uTotalBlockSize += uBurstSize;
        }
      }
    }

    if(eChromaMode == CHROMA_4_2_0 || eChromaMode == CHROMA_4_2_2)
    {
      int iChromaHeight = iHeight;

      if(eChromaMode == CHROMA_4_2_0)
        iChromaHeight >>= 1;

      for(int h = 0; h < iChromaHeight; h += 4)
      {
        for(int w = 0; w < (iWidth >> 1); w += 32)
        {
          uint8_t uBurstSize = 0;
          ReadMap(pBufMap + (iHeight >> 2) * 32 * ((iWidth + 4095) >> 12), (h >> 2) * 64 * ((iWidth + 4095) >> 12) + (w >> 5), uBurstSize);

          if(uBurstSize)
          {
            uTotalBlocks++;
            uTotalBlockSize += uBurstSize;
          }
        }
      }
    }

    if(!uTotalBlocks)
    {
      MapLogFile << "(inf) % (div by 0)" << std::endl;
    }
    else
    {
      MapLogFile << (uTotalBlockSize * 100 + (bTenBpc ? 10 : 8) * uTotalBlocks - 1) / ((bTenBpc ? 10 : 8) * uTotalBlocks) << "%" << std::endl;
    }
  }
}

#endif

/*****************************************************************************/
bool WriteCompFrame(std::ofstream& File, std::ofstream& MapFile, std::ofstream& MapLogFile, AL_TBuffer* pBuf, AL_EProfile eProfile)
{
  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  if(!File.is_open() || !MapFile.is_open())
    return false;

  uint32_t uWidth = pBufMeta->iWidth;
  uint32_t uHeight = pBufMeta->iHeight;

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
    const char* pTmp = reinterpret_cast<const char*>(pBuf->pData);

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
    const char* pTmp = reinterpret_cast<const char*>(pBuf->pData + iOffset);

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
    const char* pTmp = reinterpret_cast<const char*>(pBuf->pData + iOffset);

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
    const char* pTmp = reinterpret_cast<const char*>(pBuf->pData + iOffset);
    MapFile.write(pTmp, iNumRowMap * iRowSize);

    // Chroma
    if(eChromaMode == CHROMA_4_2_0)
      MapFile.write(pTmp + iNumRowMap * iRowSize, iNumRowMap * iRowSize / 2);
    else if(eChromaMode == CHROMA_4_2_2)
      MapFile.write(pTmp + iNumRowMap * iRowSize, iNumRowMap * iRowSize);

    (void)MapLogFile;
#if AL_ENABLE_FBC_LOG
    TraceFBCMap(MapLogFile, (uint8_t*)pTmp, uWidth, uHeight, eChromaMode, uBitDepth == 10 ? true : false);
#endif // AL_ENABLE_FBC_LOG
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

  if(pStreamMeta->pSections[iSection].uLength)
  {
    uint32_t uRemSize = pStream->zSize - pStreamMeta->pSections[iSection].uOffset;

    if(uRemSize < pStreamMeta->pSections[iSection].uLength)
    {
      File.write((char*)(pStream->pData + pStreamMeta->pSections[iSection].uOffset), uRemSize);
      File.write((char*)pStream->pData, pStreamMeta->pSections[iSection].uLength - uRemSize);
    }
    else
    {
      File.write((char*)(pStream->pData + pStreamMeta->pSections[iSection].uOffset), pStreamMeta->pSections[iSection].uLength);
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

  while((i < pStreamMeta->uNumSection) && (pStreamMeta->pSections[i].uFlags & SECTION_COMPLETE_FLAG))
  {
    if(pStreamMeta->pSections[i].uFlags & SECTION_END_FRAME_FLAG)
      ++iNumFrame;
    WriteOneSection(HEVCFile, pStream, i++);
    ++iNumSectionWritten;
  }

  return iNumFrame;
}

