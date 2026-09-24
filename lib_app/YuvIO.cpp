// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <climits>
#include <stdexcept>
#include <string>

#include "lib_app/YuvIO.hpp"

using namespace std;

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufCommon.h"
#include "lib_common/Round.h"
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
static uint32_t GetIOLumaRowSize(TFourCC tFourCC, uint32_t uWidth)
{
  uint32_t uRowSizeLuma;

  if(AL_IsNonCompTiled(tFourCC))
  {
    AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
    uint32_t uTileWidth = eStorageMode == AL_FB_TILE_32x4 ? 32 : 64;

    uint32_t uRndWidth = static_cast<uint32_t>(AL_RoundUp(uWidth, uTileWidth));
    auto iBitDepth = AL_GetBitDepth(tFourCC);
    uRowSizeLuma = uRndWidth * iBitDepth / 8;
  }
  else if(tFourCC == FOURCC(Y410))
    uRowSizeLuma = uWidth * 4;
  else if(AL_GetSamplePackMode(tFourCC) == AL_SAMPLE_PACK_MODE_PACKED_XV)
    uRowSizeLuma = (uWidth + 2) / 3 * 4;
  else if(AL_GetPlaneMode(tFourCC) == AL_PLANE_MODE_INTERLEAVED && AL_GetChromaMode(tFourCC) != AL_CHROMA_4_0_0)
  {
    int32_t const iHorizontalScale = AL_GetChromaMode(tFourCC) == AL_CHROMA_4_4_4 ? 1 : 2;
    int32_t iPixSize = sizeof(uint32_t);

    bool bHasAlpha = AL_GetAlphaMode(tFourCC) == AL_ALPHA_MODE_BEFORE || AL_GetAlphaMode(tFourCC) == AL_ALPHA_MODE_AFTER;

    if((bHasAlpha && AL_GetBitDepth(tFourCC) == 8)
       || (AL_GetSamplePackMode(tFourCC) == AL_SAMPLE_PACK_MODE_PACKED && AL_GetBitDepth(tFourCC) == 10))
      iPixSize = sizeof(uint32_t);

    if(AL_GetSamplePackMode(tFourCC) == AL_SAMPLE_PACK_MODE_BYTE && (AL_GetBitDepth(tFourCC) == 12 || AL_GetBitDepth(tFourCC) == 10))
      iPixSize = sizeof(AL_64U);
    uRowSizeLuma = uWidth * iPixSize / iHorizontalScale;
  }
  else
    uRowSizeLuma = uWidth * AL_GetPixelSize(tFourCC);
  return uRowSizeLuma;
}

/*****************************************************************************/
AL_TBuffer* AllocateDefaultYuvIOBuffer(AL_TDimension const& tDimension, TFourCC tFourCC, uint32_t uRndDim)
{
  if(AL_IsCompressed(tFourCC))
    throw runtime_error("Compressed FourCC cannot be used");

  AL_TDimension tRoundedDim = GetRoundedDim(tDimension, uRndDim);

  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(tFourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  return AL_PixMapBuffer_Create_And_AddPlanes(AL_GetDefaultAllocator(), NULL, tDimension, tRoundedDim, tPicFormat, 1, "IO frame buffer");
}

/*****************************************************************************/
int32_t GetPictureSize(AL_TYUVFileInfo FI)
{
  uint32_t uFileRowSize = GetIOLumaRowSize(FI.FourCC, FI.PictWidth);
  int32_t iPictSize = uFileRowSize * FI.PictHeight;

  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(FI.FourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  if(tPicFormat.eChromaMode != AL_CHROMA_MONO)
  {
    uint32_t uNumRowC = AL_GetChromaHeight(FI.FourCC, FI.PictHeight);
    uint32_t uRowSizeC = AL_GetChromaPitch(FI.FourCC, uFileRowSize);

    int32_t iChromaSize = uNumRowC * uRowSizeC;

    if(tPicFormat.ePlaneMode == AL_PLANE_MODE_PLANAR)
      iChromaSize *= 2;

    iPictSize += iChromaSize;
  }

  return iPictSize;
}

struct TPaddingParams
{
  uint32_t uPadValue;
  uint32_t uNBByteToPad;
  uint32_t uPaddingOffset;
  uint32_t uFirst32PackPadMask;
};

/*****************************************************************************/
static TPaddingParams GetColumnPaddingParameters(TFourCC tFourCC, AL_TDimension tDim, int32_t iPitch, uint32_t uFileRowSize, bool isLuma)
{
  TPaddingParams tPadParams;
  tPadParams.uPadValue = isLuma ? 0 : 0x80;
  tPadParams.uNBByteToPad = iPitch - uFileRowSize;
  tPadParams.uFirst32PackPadMask = 0x0;

  if(AL_GetBitDepth(tFourCC) > 8)
  {
    tPadParams.uPadValue <<= 2;

    if(AL_GetSamplePackMode(tFourCC) == AL_SAMPLE_PACK_MODE_PACKED_XV)
    {
      if(AL_GetBitDepth(tFourCC) != 10)
        throw runtime_error("BitDepth (" + to_string(AL_GetBitDepth(tFourCC)) + ") should be equal to 10");
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
  if(AL_GetSamplePackMode(tFourCC) == AL_SAMPLE_PACK_MODE_PACKED_XV)
  {
    uint32_t* pTmp32 = (uint32_t*)pTmp;

    if(0 != tPadParams.uFirst32PackPadMask)
    {
      *pTmp32 = (*pTmp32 & tPadParams.uFirst32PackPadMask) | (tPadParams.uPadValue & ~tPadParams.uFirst32PackPadMask);
      pTmp32++;
      tPadParams.uNBByteToPad -= sizeof(uint32_t);
    }

    fill_n(pTmp32, tPadParams.uNBByteToPad / sizeof(uint32_t), tPadParams.uPadValue);
  }
  else if(AL_GetBitDepth(tFourCC) > 8)
  {
    // Pitch % 32 == 0 && rowsize % 2 == 0 => iNbByteToPad % 2 == 0
    fill_n((uint16_t*)pTmp, tPadParams.uNBByteToPad / sizeof(uint16_t), tPadParams.uPadValue);
  }
  else
  {
    fill_n(pTmp, tPadParams.uNBByteToPad, tPadParams.uPadValue);
  }
}

/*****************************************************************************/
static void ReadFileLuma(ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  AL_EPlaneId ePlaneId = AL_PLANE_Y;
  int32_t iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneId);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneId));

  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitch, uFileRowSize, true);

  if(static_cast<decltype(uFileRowSize)>(iPitch) < uFileRowSize)
    throw runtime_error("iPitch should be higher or equal than uFileRowSize");

  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  bool bTileFormat = eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4;

  if(0 == tPadParams.uNBByteToPad)
  {
    uint32_t uSize = uFileNumRow * uFileRowSize;
    File.read(pTmp, uSize);
    pTmp += uSize;
  }
  else
  {
    uint32_t uSize = uFileRowSize;

    if(bTileFormat)
    {
      int32_t iLinesInPitch = AL_GetNumLinesInPitch(eStorageMode, AL_GetChromaMode(tFourCC), AL_GetBitDepth(tFourCC));
      int32_t iTileWidth = eStorageMode == AL_FB_TILE_32x4 ? 32 : 64;

      uint32_t uTileSize = iLinesInPitch * iTileWidth;
      uSize = static_cast<uint32_t>(AL_RoundUp(uFileRowSize, uTileSize));
    }

    for(uint32_t h = 0; h < uFileNumRow; h++)
    {
      File.read(pTmp, uSize);
      PadBuffer(pTmp + tPadParams.uPaddingOffset, tPadParams, tFourCC);
      pTmp += iPitch;
    }
  }

  if(uRoundedNumRow != uFileNumRow)
  {
    uint32_t uRowPadding = uRoundedNumRow - uFileNumRow;
    tPadParams.uNBByteToPad = uRowPadding * iPitch;
    tPadParams.uFirst32PackPadMask = 0x0;
    PadBuffer(pTmp, tPadParams, tFourCC);
  }
}

/*****************************************************************************/
static void ReadFileChroma(ifstream& File, AL_TBuffer* pBuf, AL_EPlaneId ePlaneType, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int32_t iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneType);
  char* pTmp = reinterpret_cast<char*>(AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneType));

  uint32_t uNumRowC = AL_GetChromaHeight(tFourCC, uFileNumRow);
  uint32_t uRowSizeC = AL_GetChromaPitch(tFourCC, uFileRowSize);

  TPaddingParams tPadParams = GetColumnPaddingParameters(tFourCC, tDim, iPitch, uRowSizeC, false);

  if(static_cast<decltype(uRowSizeC)>(iPitch) < uRowSizeC)
    throw runtime_error("iPitch should be higher or equal than uRowSizeC");

  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  bool bTileFormat = eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4;

  if(0 == tPadParams.uNBByteToPad)
  {
    uint32_t uSize = uNumRowC * uRowSizeC;
    File.read(pTmp, uSize);
    pTmp += uSize;
  }
  else
  {
    uint32_t uSize = uRowSizeC;

    if(bTileFormat)
    {
      int32_t iLinesInPitch = AL_GetNumLinesInPitch(eStorageMode, AL_GetChromaMode(tFourCC), AL_GetBitDepth(tFourCC));
      uint32_t uTileSize = iLinesInPitch * (eStorageMode == AL_FB_TILE_32x4 ? 32 : 64);
      uSize = static_cast<uint32_t>(AL_RoundUp(uRowSizeC, uTileSize));
    }

    for(uint32_t h = 0; h < uNumRowC; h++)
    {
      File.read(pTmp, uSize);
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
static void ReadFile(ifstream& File, AL_TBuffer* pBuf, uint32_t uFileRowSize, uint32_t uFileNumRow, uint32_t uRoundedNumRow)
{
  ReadFileLuma(File, pBuf, uFileRowSize, uFileNumRow, uRoundedNumRow);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(tFourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  if(tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
    ReadFileChroma(File, pBuf, AL_PLANE_UV, uFileRowSize, uFileNumRow, uRoundedNumRow);
  else if(tPicFormat.eChromaMode != AL_CHROMA_MONO)
  {
    bool bPlaneOrderYuvOrRGB = (tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_YUV || tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_RGB);
    ReadFileChroma(File, pBuf, bPlaneOrderYuvOrRGB ? AL_PLANE_U : AL_PLANE_V, uFileRowSize, uFileNumRow, uRoundedNumRow);
    ReadFileChroma(File, pBuf, bPlaneOrderYuvOrRGB ? AL_PLANE_V : AL_PLANE_U, uFileRowSize, uFileNumRow, uRoundedNumRow);
  }
}

/*****************************************************************************/
bool ReadOneFrameYuv(ifstream& File, AL_TBuffer* pBuf, bool bLoop, uint32_t uRndDim)
{
  if(!pBuf || !File.is_open())
    throw runtime_error("invalid argument");

  if((File.peek() == EOF) && !bLoop)
    return false;
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  AL_TDimension tRoundedDim = GetRoundedDim(tDim, uRndDim);

  uint32_t uRowSizeLuma = GetIOLumaRowSize(tFourCC, tDim.iWidth);
  uint32_t uHeightLuma = tDim.iHeight;
  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  int32_t iLinesInPitch = AL_GetNumLinesInPitch(eStorageMode, AL_GetChromaMode(tFourCC), AL_GetBitDepth(tFourCC));

  if(eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4
     )
  {
    uRowSizeLuma *= iLinesInPitch;
    uHeightLuma /= iLinesInPitch;
    tRoundedDim.iHeight /= iLinesInPitch;
  }

  ReadFile(File, pBuf, uRowSizeLuma, uHeightLuma, tRoundedDim.iHeight);

  if((File.rdstate() & ios::failbit) && bLoop)
  {
    File.clear();
    File.seekg(0, ios::beg);
    ReadFile(File, pBuf, uRowSizeLuma, uHeightLuma, tRoundedDim.iHeight);
  }

  if(File.rdstate() & ios::failbit)
    throw runtime_error("not enough data for a complete frame");

  return true;
}

/*****************************************************************************/
static void WritePlane(ofstream& File, const AL_TBuffer* pBuf, AL_EPlaneId ePlaneType, int32_t iIORowSize, int32_t iNumRowInPitch, int32_t iHeight)
{
  char* pTmp = (char*)AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneType);
  int32_t iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneType);

  if(iPitch == iIORowSize)
  {
    uint32_t uSizeY = iHeight * iIORowSize;
    File.write(pTmp, uSizeY);
  }
  else
  {
    for(int32_t h = 0; h < iHeight; h += iNumRowInPitch)
    {
      File.write(pTmp, iIORowSize * iNumRowInPitch);
      pTmp += iPitch;
    }
  }
}

/*****************************************************************************/
bool WriteOneFrame(ofstream& File, const AL_TBuffer* pBuf)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  if(!File.is_open())
    return false;

  int32_t iIORowSize = GetIOLumaRowSize(tFourCC, tDim.iWidth);
  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  int32_t iRowsInPitch = AL_GetNumLinesInPitch(eStorageMode, AL_GetChromaMode(tFourCC), AL_GetBitDepth(tFourCC));

  int32_t iHeight = tDim.iHeight;
  WritePlane(File, pBuf, AL_PLANE_Y, iIORowSize, iRowsInPitch, iHeight);

  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(tFourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return true;

  iIORowSize = AL_GetChromaPitch(tFourCC, iIORowSize);
  iHeight = AL_GetChromaHeight(tFourCC, iHeight);

  if(tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
    WritePlane(File, pBuf, AL_PLANE_UV, iIORowSize, iRowsInPitch, iHeight);
  else
  {
    WritePlane(File, pBuf, tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_YUV ? AL_PLANE_U : AL_PLANE_V, iIORowSize, iRowsInPitch, iHeight);
    WritePlane(File, pBuf, tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_YUV ? AL_PLANE_V : AL_PLANE_U, iIORowSize, iRowsInPitch, iHeight);
  }

  return true;
}

/*****************************************************************************/
static void ComputeMd5Plane(const AL_TBuffer* pBuf, AL_EPlaneId ePlaneType, int32_t iIORowSize, int32_t iHeight, CMD5& pMD5)
{
  uint8_t* pTmp = AL_PixMapBuffer_GetPlaneAddress(pBuf, ePlaneType);
  int32_t iPitch = AL_PixMapBuffer_GetPlanePitch(pBuf, ePlaneType);

  if(iPitch == iIORowSize)
  {
    uint32_t uSizeY = iHeight * iIORowSize;
    pMD5.Update(pTmp, uSizeY);
  }
  else
  {
    for(int32_t h = 0; h < iHeight; ++h)
    {
      pMD5.Update(pTmp, iIORowSize);
      pTmp += iPitch;
    }
  }
}

/*****************************************************************************/
/* Computes the MD5sum for each plane (Y,U,V) of the frame using the pMD5 calculator.
  The 128 bits code will be written in a .md5 file for the whole video when the
  calculator goes out of scope (its destructor).
  The function should be applied only for non-interleaved YUV frames.
*/
void ComputeMd5SumFrame(AL_TBuffer* pBuf, CMD5& pMD5)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  int32_t iIORowSize = GetIOLumaRowSize(tFourCC, tDim.iWidth);
  int32_t iHeight = tDim.iHeight;

  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  int32_t iLinesInPitch = AL_GetNumLinesInPitch(eStorageMode, AL_GetChromaMode(tFourCC), AL_GetBitDepth(tFourCC));

  bool bIsTile = eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4;

  if(bIsTile)
  {
    iIORowSize *= iLinesInPitch;
    iHeight = AL_UnsignedCeilDiv(iHeight, iLinesInPitch);
  }

  AL_EPlaneId eFistPlaneId = AL_PLANE_Y;

  if(AL_GetPlaneMode(tFourCC) == AL_PLANE_MODE_INTERLEAVED)
    eFistPlaneId = AL_PLANE_YUV;

  ComputeMd5Plane(pBuf, eFistPlaneId, iIORowSize, iHeight, pMD5);

  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(tFourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO || tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED)
    return;

  iIORowSize = AL_GetChromaPitch(tFourCC, iIORowSize);
  iHeight = AL_GetChromaHeight(tFourCC, iHeight);

  if(tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
  {
    ComputeMd5Plane(pBuf, AL_PLANE_UV, iIORowSize, iHeight, pMD5);
  }
  else
  {
    ComputeMd5Plane(pBuf, tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_YUV ? AL_PLANE_U : AL_PLANE_V, iIORowSize, iHeight, pMD5);
    ComputeMd5Plane(pBuf, tPicFormat.eComponentOrder == AL_COMPONENT_ORDER_YUV ? AL_PLANE_V : AL_PLANE_U, iIORowSize, iHeight, pMD5);
  }
}

/*****************************************************************************/
/* Computes the MD5sum for each plane (Y,U,V) of the map buffer using the pMD5
   calculator. Only the map buffer content is taken into account. There is no
   headers
*/
void ComputeMd5SumMap(AL_TBuffer* pBuf, CMD5& pMD5)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(tFourCC, &tPicFormat))
    throw runtime_error("Unknow picture format");

  if(!tPicFormat.bCompressed)
    return;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  int32_t iPitchMap = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_MAP_Y);
  int32_t iLinesInPitch = AL_GetNumLinesInPitch(tPicFormat.eStorageMode, tPicFormat.eChromaMode, tPicFormat.uBitDepth);

  int32_t iHeight = tDim.iHeight / iLinesInPitch;

  ComputeMd5Plane(pBuf, AL_PLANE_MAP_Y, iPitchMap, iHeight, pMD5);

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return;

  iHeight = AL_GetChromaHeight(tFourCC, tDim.iHeight) / iLinesInPitch;

  if(tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
  {
    ComputeMd5Plane(pBuf, AL_PLANE_UV, iPitchMap, iHeight, pMD5);
  }
  else
  {
    ComputeMd5Plane(pBuf, AL_PLANE_MAP_U, iPitchMap, iHeight, pMD5);
    ComputeMd5Plane(pBuf, AL_PLANE_MAP_V, iPitchMap, iHeight, pMD5);
  }
}
