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

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <cassert>
#include <climits>

#include "lib_app/utils.h"
#include "lib_app/YuvIO.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufCommon.h"
}

/*****************************************************************************/
static AL_TDimension GetRoundedDim(AL_TDimension tDimension, int32_t iRndDim)
{
  if(iRndDim == 0)
    return tDimension;

  AL_TDimension tRoundedDim = AL_TDimension {
    ((tDimension.iWidth + iRndDim - 1) / iRndDim) * iRndDim,
    ((tDimension.iHeight + iRndDim - 1) / iRndDim) * iRndDim
  };
  return tRoundedDim;
}

/*****************************************************************************/
static uint32_t GetIOLumaRowSize(TFourCC fourCC, uint32_t uWidth)
{
  uint32_t uRowSizeLuma;

  if(AL_Is10bitPacked(fourCC))
    uRowSizeLuma = (uWidth + 2) / 3 * 4;
  else
    uRowSizeLuma = uWidth * AL_GetPixelSize(fourCC);
  return uRowSizeLuma;
}

/*****************************************************************************/
AL_TBuffer* AllocateDefaultYuvIOBuffer(AL_TDimension const& tDimension, TFourCC tFourCC, uint32_t uRndDim)
{
  assert(AL_IsCompressed(tFourCC) == false);

  AL_TBuffer* pBuf = AL_PixMapBuffer_Create(AL_GetDefaultAllocator(), NULL, tDimension, tFourCC);

  if(pBuf == nullptr)
    return nullptr;

  std::vector<AL_TPlaneDescription> vPlaneDesc;
  int iSrcSize = 0;

  AL_TDimension tRoundedDim = GetRoundedDim(tDimension, uRndDim);
  int iPitch = GetIOLumaRowSize(tFourCC, static_cast<uint32_t>(tRoundedDim.iWidth));

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPlanes(AL_GetChromaOrder(tFourCC), false, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    AL_EPlaneId ePlaneId = usedPlanes[iPlane];
    int iComponentPitch = ePlaneId == AL_PLANE_Y ? iPitch : AL_GetChromaPitch(tFourCC, iPitch);
    int iComponentHeight = ePlaneId == AL_PLANE_Y ? tRoundedDim.iHeight : AL_GetChromaHeight(tFourCC, tRoundedDim.iHeight);
    vPlaneDesc.push_back(AL_TPlaneDescription { ePlaneId, iSrcSize, iComponentPitch });
    iSrcSize += iComponentPitch * iComponentHeight;
  }

  if(!AL_PixMapBuffer_Allocate_And_AddPlanes(pBuf, iSrcSize, &vPlaneDesc[0], vPlaneDesc.size(), "IO frame buffer"))
  {
    AL_Buffer_Destroy(pBuf);
    return nullptr;
  }

  return pBuf;
}

/*****************************************************************************/
static int PictureSize(TYUVFileInfo FI)
{
  uint32_t uFileRowSize = GetIOLumaRowSize(FI.FourCC, FI.PictWidth);
  int iPictSize = uFileRowSize * FI.PictHeight;

  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(FI.FourCC, &tPicFormat);

  if(tPicFormat.eChromaMode != AL_CHROMA_MONO)
  {
    uint32_t uNumRowC = AL_GetChromaHeight(FI.FourCC, FI.PictHeight);
    uint32_t uRowSizeC = AL_GetChromaPitch(FI.FourCC, uFileRowSize);

    int iChromaSize = uNumRowC * uRowSizeC;

    if(tPicFormat.eChromaOrder != AL_C_ORDER_SEMIPLANAR)
      iChromaSize *= 2;

    iPictSize += iChromaSize;
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
      assert(AL_GetBitDepth(tFourCC) == 10);
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
static void ReadFileLumaPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
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

  if(uRoundedNumRow != uFileNumRow)
  {
    uint32_t uRowPadding = uRoundedNumRow - uFileNumRow;
    tPadParams.uNBByteToPad = uRowPadding * iPitchY;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, tFourCC);
  }
}

/*****************************************************************************/
static void ReadFileChromaPlanar(std::ifstream& File, AL_TBuffer* pBuf, AL_EPlaneId ePlaneType, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneType);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneType));

  uint32_t uNumRowC = AL_GetChromaHeight(tFourCC, uFileNumRow);
  uint32_t uRowSizeC = AL_GetChromaPitch(tFourCC, uFileRowSize);

  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitch, uRowSizeC, false);

  assert((uint32_t)iPitch >= uRowSizeC);

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
      pTmp += iPitch;
    }
  }

  if(uRoundedNumRow != uFileNumRow)
  {
    uint32_t uRoundedNumRowC = AL_GetChromaHeight(tFourCC, uRoundedNumRow);
    tPadParams.uNBByteToPad = (uRoundedNumRowC - uNumRowC) * iPitch;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, tFourCC);
  }
}

/*****************************************************************************/
static void ReadFileChromaSemiPlanar(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV));

  uint32_t uNumRowC = AL_GetChromaHeight(tFourCC, uFileNumRow);
  uint32_t uRowSizeC = AL_GetChromaPitch(tFourCC, uFileRowSize);
  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitchUV, uRowSizeC, false);

  assert((uint32_t)iPitchUV >= uRowSizeC);

  if(0 == tPadParams.uNBByteToPad)
  {
    File.read(pTmp, uRowSizeC * uNumRowC);
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

  if(uRoundedNumRow != uFileNumRow)
  {
    uint32_t uRoundedNumRowC = AL_GetChromaHeight(tFourCC, uRoundedNumRow);
    tPadParams.uNBByteToPad = (uRoundedNumRowC - uNumRowC) * iPitchUV;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, tFourCC);
  }
}

/*****************************************************************************/
static void ReadFile(std::ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  ReadFileLumaPlanar(File, pBuf, uFileRowSize, uFileNumRow, uRoundedNumRow);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_EChromaOrder eChromaOrder = AL_GetChromaOrder(tFourCC);

  if(eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    ReadFileChromaSemiPlanar(File, pBuf, uFileRowSize, uFileNumRow, uRoundedNumRow);
  else if(eChromaOrder != AL_C_ORDER_NO_CHROMA)
  {
    ReadFileChromaPlanar(File, pBuf, eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_U : AL_PLANE_V, uFileRowSize, uFileNumRow, uRoundedNumRow);
    ReadFileChromaPlanar(File, pBuf, eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_V : AL_PLANE_U, uFileRowSize, uFileNumRow, uRoundedNumRow);
  }
}

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop, uint32_t uRndDim)
{
  if(!pBuf || !File.is_open())
    throw std::runtime_error("invalid argument");

  if((File.peek() == EOF) && !bLoop)
    return false;

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  AL_TDimension tRoundedDim = GetRoundedDim(tDim, uRndDim);

  uint32_t uRowSizeLuma = GetIOLumaRowSize(tFourCC, tDim.iWidth);

  ReadFile(File, pBuf, uRowSizeLuma, tDim.iHeight, tRoundedDim.iHeight);

  if((File.rdstate() & std::ios::failbit) && bLoop)
  {
    File.clear();
    File.seekg(0, std::ios::beg);
    ReadFile(File, pBuf, uRowSizeLuma, tDim.iHeight, tRoundedDim.iHeight);
  }

  if(File.rdstate() & std::ios::failbit)
    throw std::runtime_error("not enough data for a complete frame");

  return true;
}

/*****************************************************************************/
static void WritePlane(std::ofstream& File, const AL_TBuffer* pBuf, AL_EPlaneId ePlaneType, int iIORowSize, int iHeight)
{
  char* pTmp = (char*)AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneType);
  int iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneType);

  if(iPitch == iIORowSize)
  {
    uint32_t uSizeY = iHeight * iIORowSize;
    File.write(pTmp, uSizeY);
  }
  else
  {
    for(int h = 0; h < iHeight; h++)
    {
      File.write(pTmp, iIORowSize);
      pTmp += iPitch;
    }
  }
}

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, const AL_TBuffer* pBuf)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  if(!File.is_open())
    return false;

  int iIORowSize = GetIOLumaRowSize(tFourCC, tDim.iWidth);
  int iHeight = tDim.iHeight;
  WritePlane(File, pBuf, AL_PLANE_Y, iIORowSize, iHeight);

  AL_EChromaOrder eChromaOrder = AL_GetChromaOrder(tFourCC);

  if(eChromaOrder == AL_C_ORDER_NO_CHROMA)
    return true;

  iIORowSize = AL_GetChromaPitch(tFourCC, iIORowSize);
  iHeight = AL_GetChromaHeight(tFourCC, iHeight);

  if(eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    WritePlane(File, pBuf, AL_PLANE_UV, iIORowSize, iHeight);
  else
  {
    WritePlane(File, pBuf, eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_U : AL_PLANE_V, iIORowSize, iHeight);
    WritePlane(File, pBuf, eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_V : AL_PLANE_U, iIORowSize, iHeight);
  }

  return true;
}