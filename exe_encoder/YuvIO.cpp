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
#include "lib_common/PixMapBuffer.h"
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
static TPaddingParams GetColumnPaddingParameters(TFourCC tFourCC, AL_TDimension tDim, int iPitch, uint32_t uFileRowSize, bool isLuma)
{
  TPaddingParams tPadParams;

  tPadParams.uPadValue = isLuma ? 0 : 0x80;
  tPadParams.uNBByteToPad = iPitch - uFileRowSize;
  tPadParams.uFirst32PackPadMask = 0x0;

  if(AL_GetBitDepth(tFourCC) > 8)
  {
    tPadParams.uPadValue <<= 2;

    if(AL_Is10bitPacked(tFourCC))
    {
      tPadParams.uPadValue = tPadParams.uPadValue | (tPadParams.uPadValue << 10) | (tPadParams.uPadValue << 20);
      switch(tDim.iWidth % 3)
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

  tPadParams.uPaddingOffset = iPitch - tPadParams.uNBByteToPad;

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
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_Y));

  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitchY, uFileRowSize, true);

  assert((uint32_t)iPitchY >= uFileRowSize);

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
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, tFourCC);
      pTmp += iPitchY;
    }
  }

  if(bPadding && (tDim.iHeight & 15))
  {
    uint32_t uRowPadding = ((tDim.iHeight + 15) & ~15) - tDim.iHeight;
    tPadParams.uNBByteToPad = uRowPadding * iPitchY;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, tFourCC);
    uFileNumRow += uRowPadding;
  }
  return iPitchY * uFileNumRow;
}

/*****************************************************************************/
static uint32_t ReadFileChromaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uSubPlanOffset, uint32_t uFileRowSize, uint32_t uFileNumRow, bool bPadding = false)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV)) + uSubPlanOffset;

  uint32_t uNumRowC = (AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;
  uint32_t uRowSizeC = uFileRowSize >> 1;

  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitchUV, uRowSizeC, false);

  assert((uint32_t)iPitchUV >= uRowSizeC);

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
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, tFourCC);
      pTmp += iPitchUV;
    }
  }

  if(bPadding && (tDim.iHeight & 15))
  {
    uint32_t uRowPadding;

    if(AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0)
      uRowPadding = (((tDim.iHeight >> 1) + 7) & ~7) - (tDim.iHeight >> 1);
    else
      uRowPadding = ((tDim.iHeight + 15) & ~15) - tDim.iHeight;

    tPadParams.uNBByteToPad = uRowPadding * iPitchUV;
    PadBuffer(pTmp, tPadParams, tFourCC);

    uNumRowC += uRowPadding;
  }

  return iPitchUV * uNumRowC;
}

/*****************************************************************************/
static void ReadFileChromaSemiPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV));

  uint32_t uNumRowC = (AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0) ? uFileNumRow >> 1 : uFileNumRow;
  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitchUV, uFileRowSize, false);

  assert((uint32_t)iPitchUV >= uFileRowSize);

  if(0 == tPadParams.uNBByteToPad)
  {
    File.read(pTmp, uFileRowSize * uNumRowC);
  }
  else
  {
    for(uint32_t h = 0; h < uNumRowC; h++)
    {
      File.read(pTmp, uFileRowSize);
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, tFourCC);
      pTmp += iPitchUV;
    }
  }
}

/*****************************************************************************/
static void ReadFile(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow)
{
  ReadFileLumaPlanar(File, pBuf, uFileRowSize, uFileNumRow);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);

  if(AL_IsSemiPlanar(tFourCC))
  {
    ReadFileChromaSemiPlanar(File, pBuf, uFileRowSize, uFileNumRow);
  }
  else if(AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0 || AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_2)
  {
    uint32_t uSubPlanOffset = ReadFileChromaPlanar(File, pBuf, 0, uFileRowSize, uFileNumRow); // Cb
    ReadFileChromaPlanar(File, pBuf, uSubPlanOffset, uFileRowSize, uFileNumRow); // Cr
  }
}

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop)
{
  if(!pBuf || !File.is_open())
    throw std::runtime_error("invalid argument");

  if((File.peek() == EOF) && !bLoop)
    return false;

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  uint32_t uRowSizeLuma = GetIOLumaRowSize(tFourCC, tDim.iWidth);

  ReadFile(File, pBuf, uRowSizeLuma, tDim.iHeight);

  if((File.rdstate() & std::ios::failbit) && bLoop)
  {
    File.clear();
    File.seekg(0, std::ios::beg);
    ReadFile(File, pBuf, uRowSizeLuma, tDim.iHeight);
  }

  if(File.rdstate() & std::ios::failbit)
    throw std::runtime_error("not enough data for a complete frame");

  return true;
}

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, const AL_TBuffer* pBuf)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);
  int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);

  if(!File.is_open())
    return false;

  char* pTmp = (char*)AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_Y);
  int uRowSizeLuma = GetIOLumaRowSize(tFourCC, tDim.iWidth);

  if(iPitchY == uRowSizeLuma)
  {
    uint32_t uSizeY = tDim.iHeight * uRowSizeLuma;
    File.write(pTmp, uSizeY);
  }
  else
  {
    for(int h = 0; h < tDim.iHeight; h++)
    {
      File.write(pTmp, uRowSizeLuma);
      pTmp += iPitchY;
    }
  }

  pTmp = (char*)AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV);

  // 1 Interleaved Chroma plane
  if(AL_IsSemiPlanar(tFourCC))
  {
    int iHeightC = (AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0) ? tDim.iHeight >> 1 : tDim.iHeight;

    if(iPitchUV == uRowSizeLuma)
      File.write(pTmp, iHeightC * uRowSizeLuma);
    else
    {
      for(int h = 0; h < iHeightC; ++h)
      {
        File.write(pTmp, uRowSizeLuma);
        pTmp += iPitchUV;
      }
    }
  }
  // 2 Separated chroma plane
  else if(AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0 || AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_2)
  {
    int iWidthC = tDim.iWidth >> 1;
    int iHeightC = (AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_0) ? tDim.iHeight >> 1 : tDim.iHeight;

    int iSizePix = AL_GetPixelSize(tFourCC);

    if(iPitchUV == iWidthC * iSizePix)
    {
      uint32_t uSizeC = iWidthC * iHeightC * iSizePix;
      File.write(pTmp, uSizeC);
      pTmp += uSizeC;
      File.write(pTmp, uSizeC);
    }
    else
    {
      for(int iComponent = 0; iComponent < 2; iComponent++)
      {
        for(int h = 0; h < iHeightC; ++h)
        {
          File.write(pTmp, iWidthC * iSizePix);
          pTmp += iPitchUV;
        }
      }
    }
  }
  return true;
}

