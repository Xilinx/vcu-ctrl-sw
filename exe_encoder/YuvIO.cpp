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

#include "YuvIO.h"

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <cassert>
#include <climits>

#include "lib_app/utils.h"

extern "C"
{
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferSrcMeta.h"
}

/*****************************************************************************/
static int PictureSize(TYUVFileInfo FI)
{
  int iPictSize = GetIOLumaRowSize(FI.FourCC, FI.PictWidth) * FI.PictHeight;

  if(AL_GetChromaMode(FI.FourCC) != AL_CHROMA_MONO)
  {
    int iRx, iRy;
    AL_GetSubsampling(FI.FourCC, &iRx, &iRy);
    iPictSize += 2 * (iPictSize / (iRx * iRy));
  }

  return iPictSize;
}

/*****************************************************************************/
void GotoFirstPicture(TYUVFileInfo const& FI, std::ifstream& File, unsigned int iFirstPict)
{
  int64_t const iPictLen = PictureSize(FI);
  File.seekg(iPictLen * iFirstPict);
}

/*****************************************************************************/
uint32_t GetIOLumaRowSize(TFourCC fourCC, uint32_t uWidth)
{
  uint32_t uRowSizeLuma;

  if(AL_Is10bitPacked(fourCC))
    uRowSizeLuma = (uWidth + 2) / 3 * 4;
  else
    uRowSizeLuma = uWidth * AL_GetPixelSize(fourCC);
  return uRowSizeLuma;
}

/*****************************************************************************/
int GotoNextPicture(TYUVFileInfo const& FI, std::ifstream& File, int iEncFrameRate, int iEncPictCount, int iFilePictCount)
{
  int iMove = ((iEncPictCount * FI.FrameRate) / iEncFrameRate) - iFilePictCount;

  if(iMove != 0)
  {
    int iPictSize = PictureSize(FI);
    File.seekg(iPictSize * iMove, std::ios_base::cur);
  }
  return iMove;
}

typedef struct tPaddingParams
{
  uint32_t uPadValue;
  uint32_t uNBByteToPad;
  uint32_t uPaddingOffset;
  uint32_t uFirst32PackPadMask;
}TPaddingParams;

/*****************************************************************************/
static TPaddingParams GetColumnPaddingParameters(AL_TSrcMetaData* pSrcMeta, uint32_t uFileRowSize, bool isLuma)
{
  TPaddingParams tPadParams;

  auto const pitch = isLuma ? pSrcMeta->tPlanes[AL_PLANE_Y].iPitch : pSrcMeta->tPlanes[AL_PLANE_UV].iPitch;

  tPadParams.uPadValue = isLuma ? 0 : 0x80;
  tPadParams.uNBByteToPad = pitch - uFileRowSize;
  tPadParams.uFirst32PackPadMask = 0x0;

  if(AL_GetBitDepth(pSrcMeta->tFourCC) > 8)
  {
    tPadParams.uPadValue <<= 2;

    if(AL_Is10bitPacked(pSrcMeta->tFourCC))
    {
      tPadParams.uPadValue = tPadParams.uPadValue | (tPadParams.uPadValue << 10) | (tPadParams.uPadValue << 20);
      switch(pSrcMeta->tDim.iWidth % 3)
      {
      case 1:
        tPadParams.uFirst32PackPadMask = 0x3FF;
        tPadParams.uNBByteToPad += sizeof(uint32_t);
        break;
      case 2:
        tPadParams.uFirst32PackPadMask = 0xFFFFF;
        tPadParams.uNBByteToPad += sizeof(uint32_t);
        break;
      default:
        break;
      }
    }
  }

  tPadParams.uPaddingOffset = pitch - tPadParams.uNBByteToPad;

  return tPadParams;
}

/*****************************************************************************/
static void PadBuffer(char* pTmp, TPaddingParams tPadParams, TFourCC tFourCC)
{
  if(AL_Is10bitPacked(tFourCC))
  {
    uint32_t* pTmp32 = (uint32_t*)pTmp;

    if(0 != tPadParams.uFirst32PackPadMask)
    {
      *pTmp32 = (*pTmp32 & tPadParams.uFirst32PackPadMask) | (tPadParams.uPadValue & ~tPadParams.uFirst32PackPadMask);
      pTmp32++;
      tPadParams.uNBByteToPad -= sizeof(uint32_t);
    }

    std::fill_n(pTmp32, tPadParams.uNBByteToPad / sizeof(uint32_t), tPadParams.uPadValue);
  }
  else if(AL_GetBitDepth(tFourCC) > 8)
  {
    // Pitch % 32 == 0 && rowsize % 2 == 0 => iNbByteToPad % 2 == 0
    std::fill_n((uint16_t*)pTmp, tPadParams.uNBByteToPad / sizeof(uint16_t), tPadParams.uPadValue);
  }
  else
  {
    std::fill_n(pTmp, tPadParams.uNBByteToPad, tPadParams.uPadValue);
  }
}

/*****************************************************************************/
static uint32_t ReadFileLumaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf));

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  TPaddingParams tPadParams = GetColumnPaddingParameters(pSrcMeta, uFileRowSize, true);

  assert((uint32_t)pSrcMeta->tPlanes[AL_PLANE_Y].iPitch >= uFileRowSize);

  if(0 == tPadParams.uNBByteToPad)
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
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, pSrcMeta->tFourCC);
      pTmp += pSrcMeta->tPlanes[AL_PLANE_Y].iPitch;
    }
  }

  if(bPadding && (pSrcMeta->tDim.iHeight & 15))
  {
    uint32_t uRowPadding = ((pSrcMeta->tDim.iHeight + 15) & ~15) - pSrcMeta->tDim.iHeight;
    tPadParams.uNBByteToPad = uRowPadding * pSrcMeta->tPlanes[AL_PLANE_Y].iPitch;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, pSrcMeta->tFourCC);
    uFileNumRow += uRowPadding;
  }
  return pSrcMeta->tPlanes[AL_PLANE_Y].iPitch * uFileNumRow;
}

/*****************************************************************************/
static uint32_t ReadFileChromaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf) + uOffset);
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uNumRowC = (AL_GetChromaMode(pSrcMeta->tFourCC) == AL_CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;
  uint32_t uRowSizeC = uFileRowSize >> 1;

  TPaddingParams tPadParams = GetColumnPaddingParameters(pSrcMeta, uRowSizeC, false);

  assert((uint32_t)pSrcMeta->tPlanes[AL_PLANE_UV].iPitch >= uRowSizeC);

  if(0 == tPadParams.uNBByteToPad)
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
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, pSrcMeta->tFourCC);
      pTmp += pSrcMeta->tPlanes[AL_PLANE_UV].iPitch;
    }
  }

  if(bPadding && (pSrcMeta->tDim.iHeight & 15))
  {
    uint32_t uRowPadding;

    if(AL_GetChromaMode(pSrcMeta->tFourCC) == AL_CHROMA_4_2_0)
      uRowPadding = (((pSrcMeta->tDim.iHeight >> 1) + 7) & ~7) - (pSrcMeta->tDim.iHeight >> 1);
    else
      uRowPadding = ((pSrcMeta->tDim.iHeight + 15) & ~15) - pSrcMeta->tDim.iHeight;

    tPadParams.uNBByteToPad = uRowPadding * pSrcMeta->tPlanes[AL_PLANE_UV].iPitch;
    PadBuffer(pTmp, tPadParams, pSrcMeta->tFourCC);

    uNumRowC += uRowPadding;
  }

  return pSrcMeta->tPlanes[AL_PLANE_UV].iPitch * uNumRowC;
}

/*****************************************************************************/
static void ReadFileChromaSemiPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uOffset, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  char* pTmp = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf) + uOffset);
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
  uint32_t uNumRowC = (AL_GetChromaMode(pSrcMeta->tFourCC) == AL_CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;

  TPaddingParams tPadParams = GetColumnPaddingParameters(pSrcMeta, uFileRowSize, false);

  assert((uint32_t)pSrcMeta->tPlanes[AL_PLANE_UV].iPitch >= uFileRowSize);

  if(0 == tPadParams.uNBByteToPad)
  {
    File.read(pTmp, uFileRowSize * uNumRowC);
  }
  else
  {
    for(uint32_t h = 0; h < uNumRowC; h++)
    {
      File.read(pTmp, uFileRowSize);
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, pSrcMeta->tFourCC);
      pTmp += pSrcMeta->tPlanes[AL_PLANE_UV].iPitch;
    }
  }
}

/*****************************************************************************/
static void ReadFile(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  ReadFileLumaPlanar(File, pBuf, uFileRowSize, uFileNumRow);

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
  uint32_t uOffset = pSrcMeta->tPlanes[AL_PLANE_UV].iOffset;

  if(AL_IsSemiPlanar(pSrcMeta->tFourCC))
  {
    ReadFileChromaSemiPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow);
  }
  else if(AL_GetChromaMode(pSrcMeta->tFourCC) == AL_CHROMA_4_2_0 || AL_GetChromaMode(pSrcMeta->tFourCC) == AL_CHROMA_4_2_2)
  {
    uOffset += ReadFileChromaPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow); // Cb
    ReadFileChromaPlanar(File, pBuf, uOffset, uFileRowSize, uFileNumRow); // Cr
  }
}

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop)
{
  if(!pBuf || !File.is_open())
    throw std::runtime_error("invalid argument");

  if((File.peek() == EOF) && !bLoop)
    return false;

  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uRowSizeLuma = GetIOLumaRowSize(pSrcMeta->tFourCC, pSrcMeta->tDim.iWidth);

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
bool WriteOneFrame(std::ofstream& File, const AL_TBuffer* pBuf)
{
  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  int iWidth = pBufMeta->tDim.iWidth;
  int iHeight = pBufMeta->tDim.iHeight;

  if(!File.is_open())
    return false;

  char* pBufData = reinterpret_cast<char*>(AL_Buffer_GetData(pBuf));
  char* pTmp = pBufData;
  int uRowSizeLuma = GetIOLumaRowSize(pBufMeta->tFourCC, iWidth);

  if(pBufMeta->tPlanes[AL_PLANE_Y].iPitch == uRowSizeLuma)
  {
    uint32_t uSizeY = iHeight * uRowSizeLuma;
    File.write(pTmp, uSizeY);
  }
  else
  {
    for(int h = 0; h < iHeight; h++)
    {
      File.write(pTmp, uRowSizeLuma);
      pTmp += pBufMeta->tPlanes[AL_PLANE_Y].iPitch;
    }
  }

  pTmp = pBufData + pBufMeta->tPlanes[AL_PLANE_UV].iOffset;

  // 1 Interleaved Chroma plane
  if(AL_IsSemiPlanar(pBufMeta->tFourCC))
  {
    int iHeightC = (AL_GetChromaMode(pBufMeta->tFourCC) == AL_CHROMA_4_2_0) ? iHeight >> 1 : iHeight;

    if(pBufMeta->tPlanes[AL_PLANE_UV].iPitch == uRowSizeLuma)
      File.write(pTmp, iHeightC * uRowSizeLuma);
    else
    {
      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, uRowSizeLuma);
        pTmp += pBufMeta->tPlanes[AL_PLANE_UV].iPitch;
      }
    }
  }
  // 2 Separated chroma plane
  else if(AL_GetChromaMode(pBufMeta->tFourCC) == AL_CHROMA_4_2_0 || AL_GetChromaMode(pBufMeta->tFourCC) == AL_CHROMA_4_2_2)
  {
    int iWidthC = iWidth >> 1;
    int iHeightC = (AL_GetChromaMode(pBufMeta->tFourCC) == AL_CHROMA_4_2_0) ? iHeight >> 1 : iHeight;

    int iSizePix = AL_GetPixelSize(pBufMeta->tFourCC);

    if(pBufMeta->tPlanes[AL_PLANE_UV].iPitch == iWidthC * iSizePix)
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
        pTmp += pBufMeta->tPlanes[AL_PLANE_UV].iPitch;
      }

      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, iWidthC * iSizePix);
        pTmp += pBufMeta->tPlanes[AL_PLANE_UV].iPitch;
      }
    }
  }
  return true;
}

