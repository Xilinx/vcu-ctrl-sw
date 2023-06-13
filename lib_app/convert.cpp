// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
// Fourcc.org is not complete for the format definition of yuv
// - For planar format and 10-12 bits precision : the reference used is
// https://github.com/videolan/vlc/blob/master/src/misc/fourcc_list.h
// - For semi-planar format and 10-12 bits precision : the reference used is
// https://docs.microsoft.com/en-us/windows/win32/medfound/10-bit-and-16-bit-yuv-video-formats

#include <stdexcept>
#include <cstring>
#include <string>
#include <algorithm>
#include "lib_app/convert.h"

extern "C" {
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Utils.h"
}

static inline uint16_t CONV_8B_TO_10B(uint8_t val)
{
  return ((uint16_t)val) << 2;
}

static inline uint16_t CONV_8B_TO_12B(uint8_t val)
{
  return ((uint16_t)val) << 4;
}

static inline uint8_t RND_10B_TO_8B(uint16_t val)
{
  return (uint8_t)((val >= 0x3FC) ? 0xFF : ((val + 2) >> 2));
}

static inline uint8_t RND_12B_TO_8B(uint16_t val)
{
  return (uint8_t)((val >= 0xFF0) ? 0xFF : ((val + 8) >> 4));
}

static inline uint16_t RND_12B_TO_10B(uint16_t val)
{
  return (uint16_t)((val >= 0xFFC) ? 0x3FF : ((val + 2) >> 2));
}

static inline uint16_t COPY(uint16_t val)
{
  return val;
}

/****************************************************************************/
void CopyPixMapPlane(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight, uint8_t uBitDepth)
{
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneType);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneType);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  int iByteWidth = iWidth * (uBitDepth >= 10 ? 2 : 1);

  for(int iH = 0; iH < iHeight; iH++)
  {
    std::memcpy(pDstData, pSrcData, iByteWidth);
    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
template<typename TSrc, typename TDst>
void ConvertPixMapPlane(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight, TDst (* CONV_FUNC)(TSrc))
{
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneType) / sizeof(TSrc);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType) / sizeof(TDst);
  auto pSrcData = (TSrc*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneType);
  auto pDstData = (TDst*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  for(int iH = 0; iH < iHeight; iH++)
  {
    for(int iW = 0; iW < iWidth; iW++)
      pDstData[iW] = (*CONV_FUNC)(pSrcData[iW]);

    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
template<typename T>
void SetPixMapPlane(AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight, T uVal)
{
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType) / sizeof(T);
  auto pDstData = (T*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  for(int iH = 0; iH < iHeight; iH++)
  {
    for(int iW = 0; iW < iWidth; iW++)
      *pDstData++ = uVal;

    pDstData += iPitchDst - iWidth;
  }
}

/****************************************************************************/
void I420_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void I420_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void I420_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);
}

/****************************************************************************/
void I420_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));

  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_10B);
}

/****************************************************************************/
void I420_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));

  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_12B);
}

/****************************************************************************/
static void I4XX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));

  // Luma
  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_10B);

  // Chroma
  int iWidthChroma = (tDim.iWidth + iHScale - 1) / iHScale;
  int iHeightChroma = (tDim.iHeight + iVScale - 1) / iVScale;
  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_U, iWidthChroma, iHeightChroma, CONV_8B_TO_10B);
  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_V, iWidthChroma, iHeightChroma, CONV_8B_TO_10B);
}

/****************************************************************************/
void I420_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
  I4XX_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I444_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
  I4XX_To_IXAL(pSrc, pDst, 1, 1);
}

/****************************************************************************/
void IYUV_To_420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_YV12(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_I0AL(pSrc, pDst);
}

/****************************************************************************/
void YV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void YV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void YV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void YV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_I0AL(pSrc, pDst);
}

/****************************************************************************/
void NV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);
}

/****************************************************************************/
void Y800_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);

  int iWidthChroma = (tDim.iWidth + 1) >> 1;
  int iHeightChroma = (tDim.iHeight + 1) >> 1;
  SetPixMapPlane<uint8_t>(pDst, AL_PLANE_U, iWidthChroma, iHeightChroma, 0x80);
  SetPixMapPlane<uint8_t>(pDst, AL_PLANE_V, iWidthChroma, iHeightChroma, 0x80);
}

/****************************************************************************/
void Y800_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void Y800_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void Y800_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);

  int iWidthChroma = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightChroma = (tDim.iHeight + 1) >> 1;
  SetPixMapPlane<uint8_t>(pDst, AL_PLANE_UV, iWidthChroma, iHeightChroma, 0x80);
}

/****************************************************************************/
void Y800_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));

  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_10B);

  int iWidthChroma = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightChroma = (tDim.iHeight + 1) >> 1;
  SetPixMapPlane<uint16_t>(pDst, AL_PLANE_UV, iWidthChroma, iHeightChroma, 0x0200);
}

/****************************************************************************/
void Y800_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));

  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_10B);

  int iWidthChroma = (tDim.iWidth + 1) >> 1;
  int iHeightChroma = (tDim.iHeight + 1) >> 1;
  SetPixMapPlane<uint16_t>(pDst, AL_PLANE_U, iWidthChroma, iHeightChroma >> 1, 0x0200);
  SetPixMapPlane<uint16_t>(pDst, AL_PLANE_V, iWidthChroma, iHeightChroma >> 1, 0x0200);
}

/****************************************************************************/
void Y800_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));

  ConvertPixMapPlane<uint8_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, CONV_8B_TO_10B);
}

/****************************************************************************/
void Y800_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));

  int iPitchSrcY = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDstY = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  if((iPitchDstY % 4) != 0)
    throw std::runtime_error("iPitchDstY (" + std::to_string(iPitchDstY) + ") should be aligned to 4");

  if(iPitchDstY < ((((tDim.iWidth + 2) / 3) * 4)))
    throw std::runtime_error("iPitchDstY (" + std::to_string(iPitchDstY) + ") should be higher than" + std::to_string((((tDim.iWidth + 2) / 3) * 4)));

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * iPitchSrcY);
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iPitchDstY);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
      *pDst32 |= ((uint32_t)*pSrcY++) << 22;
      ++pDst32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
    }
  }
}

/****************************************************************************/
static void Set_XV_ChromaComponent(AL_TBuffer* pDstBuf, int iHrzScale, int iVrtScale)
{
  (void)iHrzScale;
  (void)iVrtScale;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDstBuf);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, AL_PLANE_UV);

  if((iPitchDst % 4) != 0)
    throw std::runtime_error("iPitchDst (" + std::to_string(iPitchDst) + ") should be aligned to 4");

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);

    int w = iWidthC / 3;

    while(w--)
      *pDst++ = 0x20080200;

    if(iWidthC % 3 > 1)
      *pDst = 0x00080200;
    else if(iWidthC % 3 > 0)
      *pDst = 0x00000200;
  }
}

/****************************************************************************/
void Y800_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  // Chroma
  Set_XV_ChromaComponent(pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
void Y800_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  // Chroma
  Set_XV_ChromaComponent(pDst, 2, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
static void SemiPlanar_To_XV_OneComponent(AL_TBuffer const* pSrcBuf, AL_TBuffer* pDstBuf, bool bProcessY, int iHrzScale, int iVrtScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrcBuf);

  int iDstWidth = (tDim.iWidth + iHrzScale - 1) / iHrzScale;
  int iDstHeight = (tDim.iHeight + iVrtScale - 1) / iVrtScale;

  AL_EPlaneId ePlaneID = bProcessY ? AL_PLANE_Y : AL_PLANE_UV;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrcBuf, ePlaneID);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, ePlaneID);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrcBuf, ePlaneID);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, ePlaneID);

  if(!bProcessY)
    iDstWidth *= 2;

  if((iPitchDst % 4) != 0)
    throw std::runtime_error("iPitchDst (" + std::to_string(iPitchDst) + ") should be aligned to 4");

  if(iPitchDst < ((((iDstWidth + 2) / 3) * 4)))
    throw std::runtime_error("iPitchDst (" + std::to_string(iPitchDst) + ") should be higher than" + std::to_string((((iDstWidth + 2) / 3) * 4)));

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + h * iPitchSrc);

    int w = iDstWidth / 3;

    while(w--)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 20;
      ++pDst;
    }

    if(iDstWidth % 3 > 1)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
    }
    else if(iDstWidth % 3 > 0)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
    }
  }
}

/****************************************************************************/
void Y010_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV10));
}

/****************************************************************************/
void Y010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_XV10(pSrc, pDst);

  // Chroma
  Set_XV_ChromaComponent(pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
void Y010_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_XV10(pSrc, pDst);

  // Chroma
  Set_XV_ChromaComponent(pDst, 2, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
static void SemiPlanarToPlanar_1XTo8b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, uint8_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  // Luma
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  // Chroma
  {
    uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
    uint32_t uPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
    uint32_t uPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);

    int iWidth = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
    int iHeight = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

    auto pBufInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    auto pBufOutU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
    auto pBufOutV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = RND_FUNC(pBufInC[iW << 1]);
        pBufOutV[iW] = RND_FUNC(pBufInC[(iW << 1) + 1]);
      }

      pBufInC += uPitchSrc;
      pBufOutU += uPitchDstU;
      pBufOutV += uPitchDstV;
    }
  }
}

/****************************************************************************/
static void SemiPlanarToPlanar_1XTo1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, uint16_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  // Luma
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  // Chroma
  {
    uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
    uint32_t uPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
    uint32_t uPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

    int iWidth = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
    int iHeight = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

    auto pBufInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    auto pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
    auto pBufOutV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = RND_FUNC(pBufInC[iW << 1]);
        pBufOutV[iW] = RND_FUNC(pBufInC[(iW << 1) + 1]);
      }

      pBufInC += uPitchSrc;
      pBufOutU += uPitchDstU;
      pBufOutV += uPitchDstV;
    }
  }
}

/****************************************************************************/
static void PX1X_To_NVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, uint8_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  int iChromaWidth = ((tDim.iWidth + uHrzCScale - 1) / uHrzCScale) * uHrzCScale;
  int iChromaHeight = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_UV, iChromaWidth, iChromaHeight, RND_FUNC);
}

/****************************************************************************/
static void IXYL_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale, uint8_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  // Chroma
  int iChromaWidth = (tDim.iWidth + iHScale - 1) / iHScale;
  int iChromaHeight = (tDim.iHeight + iVScale - 1) / iVScale;
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, RND_FUNC);
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, RND_FUNC);
}

/****************************************************************************/
static void IXYL_To_IXYL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale, uint16_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  // Chroma
  const int iChromaWidth = (tDim.iWidth + iHScale - 1) / iHScale;
  const int iChromaHeight = (tDim.iHeight + iVScale - 1) / iVScale;
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, RND_FUNC);
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, RND_FUNC);
}

/****************************************************************************/
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 2, RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void P210_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 1, RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void P210_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 1, 1, RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

void P210_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 2, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
void P010_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  P010_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 2, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void P210_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void P012_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 2, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0CL));
}

/****************************************************************************/
void P212_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2CL));
}

/****************************************************************************/
void P012_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 2, RND_12B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void P212_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 1, RND_12B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void P010_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  P010_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void P010_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX1X_To_NVXX(pSrc, pDst, 2, 2, RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void P010_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void P010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
static void IXAL_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  int iChromaWidth = (tDim.iWidth + iHScale - 1) / iHScale;
  int iChromaHeight = (tDim.iHeight + iVScale - 1) / iVScale;
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, RND_10B_TO_8B);
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, RND_10B_TO_8B);
}

/****************************************************************************/
void I0AL_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_I4XX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void I4AL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_I4XX(pSrc, pDst, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void I0AL_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void I0AL_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void I0CL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_12B_TO_10B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
void I0CL_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
static void I0XL_To_Y01X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint16_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);
}

/****************************************************************************/
void P010_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void P012_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
static void I4XX_To_NVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);

  // Chroma
  const int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  const int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  const int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  AL_VADDR pBufInU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  AL_VADDR pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  AL_VADDR pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[iW * 2] = pBufInU[iW];
      pBufOut[iW * 2 + 1] = pBufInV[iW];
    }

    pBufOut += iPitchDst;
    pBufInU += iPitchSrcU;
    pBufInV += iPitchSrcV;
  }
}

/****************************************************************************/
void YV12_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_NV12(pSrc, pDst);
}

/****************************************************************************/
void I420_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void I444_To_NV24(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV24));
}

/****************************************************************************/
static void I4XX_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  auto pBufInU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  auto pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
      *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
    }

    pBufInU += iPitchSrcU - iWidthC;
    pBufInV += iPitchSrcV - iWidthC;
    pBufOut += iPitchDst - (2 * iWidthC);
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P410));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
  default: throw std::runtime_error("Unsupported chroma scale");
  }
}

/****************************************************************************/
static void I4XX_To_PX12(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  I420_To_Y012(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  auto pBufInU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  auto pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = ((uint16_t)*pBufInU++) << 4;
      *pBufOut++ = ((uint16_t)*pBufInV++) << 4;
    }

    pBufInU += iPitchSrcU - iWidthC;
    pBufInV += iPitchSrcV - iWidthC;
    pBufOut += iPitchDst - (2 * iWidthC);
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P412));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P212));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P012));
  default: throw std::runtime_error("Unsupported chroma scale");
  }
}

/****************************************************************************/
void YV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_P010(pSrc, pDst);
}

/****************************************************************************/
void I420_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void IYUV_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void I444_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX10(pSrc, pDst, 1, 1);
}

/****************************************************************************/
void I420_To_P012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX12(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX12(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void I444_To_P412(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_PX12(pSrc, pDst, 1, 1);
}

/****************************************************************************/
void Y012_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  ConvertPixMapPlane<uint16_t, uint16_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_12B_TO_10B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void Y012_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  ConvertPixMapPlane<uint16_t, uint8_t>(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_12B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void P012_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 2, RND_12B_TO_10B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void P212_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo1X(pSrc, pDst, 2, 1, RND_12B_TO_10B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void I4CL_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_IXYL(pSrc, pDst, 1, 1, RND_12B_TO_10B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
void I4CL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_I4XX(pSrc, pDst, 1, 1, RND_12B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
static void I42X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iSrcPitchU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  int iSrcPitchV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  int iDstPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if((iDstPitch % 4) != 0)
    throw std::runtime_error("iDstPitch (" + std::to_string(iDstPitch) + ") should be aligned to 4");

  if(iDstPitch < ((((tDim.iWidth + 2) / 3) * 4)))
    throw std::runtime_error("iDstPitch (" + std::to_string(iDstPitch) + ") should be higher than" + std::to_string((((tDim.iWidth + 2) / 3) * 4)));

  uint8_t* pSrcDataU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint8_t* pSrcDataV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iWidthC = ((tDim.iWidth + uHrzCScale - 1) / uHrzCScale);
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iDstPitch);
    uint8_t* pSrcU = (uint8_t*)(pSrcDataU + h * iSrcPitchU);
    uint8_t* pSrcV = (uint8_t*)(pSrcDataV + h * iSrcPitchV);

    int w = iWidthC / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
      *pDst32 |= ((uint32_t)*pSrcU++) << 12;
      *pDst32 |= ((uint32_t)*pSrcV++) << 22;
      ++pDst32;
    }

    if(iWidthC % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(iWidthC % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void YV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_XV15(pSrc, pDst);
}

/****************************************************************************/
void I420_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void IXAL_To_NVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;
  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  uint32_t uPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(uint16_t);
  uint32_t uPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(uint16_t);
  uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  auto pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pBufInV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  auto pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = RND_10B_TO_8B(*pBufInU++);
      *pBufOut++ = RND_10B_TO_8B(*pBufInV++);
    }

    pBufOut += uPitchDst - tDim.iWidth;
    pBufInU += uPitchSrcU - iWidthC;
    pBufInV += uPitchSrcV - iWidthC;
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV24));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
  default: throw std::runtime_error("Unsupported chroma scale");
  }
}

/****************************************************************************/
void I0AL_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void I4AL_To_NV24(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NVXX(pSrc, pDst, 1, 1);
}

/****************************************************************************/
static void IXYL_To_PX1Y(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, uint16_t (* RND_FUNC)(uint16_t))
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0XL_To_Y01X(pSrc, pDst, RND_FUNC);

  // Chroma
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;
  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  uint32_t uPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(uint16_t);
  uint32_t uPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(uint16_t);
  uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  auto pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pBufInV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  auto pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[2 * iW] = RND_FUNC(pBufInU[iW]);
      pBufOut[2 * iW + 1] = RND_FUNC(pBufInV[iW]);
    }

    pBufInU += uPitchSrcU;
    pBufInV += uPitchSrcV;
    pBufOut += uPitchDst;
  }
}

/****************************************************************************/
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 2, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
void I4AL_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 1, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P410));
}

/****************************************************************************/
void I0CL_To_P012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 2, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P012));
}

/****************************************************************************/
void I2CL_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P212));
}

/****************************************************************************/
void I4CL_To_P412(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 1, 1, COPY);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P412));
}

/****************************************************************************/
static void IXAL_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y010_To_XV10(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  uint32_t uSrcPitchChromaU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  uint32_t uSrcPitchChromaV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if((uDstPitchChroma % 4) != 0)
    throw std::runtime_error("uDstPitchChroma (" + std::to_string(uDstPitchChroma) + ") should be aligned to 4");

  if(uDstPitchChroma < ((((static_cast<decltype(uDstPitchChroma)>(tDim.iWidth) + 2) / 3) * 4)))
    throw std::runtime_error("uDstPitchChroma (" + std::to_string(uDstPitchChroma) + ") should be higher than" + std::to_string((((static_cast<decltype(uDstPitchChroma)>(tDim.iWidth) + 2) / 3) * 4)));

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  uint8_t* pSrcDataU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint8_t* pSrcDataV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * uDstPitchChroma);
    uint16_t* pSrcU = (uint16_t*)(pSrcDataU + h * uSrcPitchChromaU);
    uint16_t* pSrcV = (uint16_t*)(pSrcDataV + h * uSrcPitchChromaV);

    int w = iWidthC / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 20;
      ++pDst32;
    }

    if(iWidthC % 3 > 1)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
    }
    else if(iWidthC % 3 > 0)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void I0AL_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_XVXX(pSrc, pDst, 2, 1);
}

static void ALX8_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  const int iTileW = 64;
  const int iTileH = 4;

  uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);

  uint32_t uPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
  uint32_t uPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);
  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint8_t* pDstDataV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale * 2;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * uPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 4)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 8 / uHrzCScale)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstDataU + (H + h) * uPitchDstU + (W + w) / uHrzCScale;
          uint8_t* pOutV = pDstDataV + (H + h) * uPitchDstV + (W + w) / uHrzCScale;

          for(int p = 0; p < (8 / uHrzCScale); p++)
            ((p % 2 == 0) ? pOutU[(p % (8 / uHrzCScale)) / 2] : pOutV[(p % (8 / uHrzCScale)) / 2]) = pInC[p];

          pOutU += uPitchDstU;
          pOutV += uPitchDstV;

          for(int p = (1 * (8 / uHrzCScale)); p < (1 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
            ((p % 2 == 0) ? pOutU[(p % (8 / uHrzCScale)) / 2] : pOutV[(p % (8 / uHrzCScale)) / 2]) = pInC[p];

          if(H + h + 3 <= iHeightC)
          {
            pOutU += uPitchDstU;
            pOutV += uPitchDstV;

            for(int p = (2 * (8 / uHrzCScale)); p < (2 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
              ((p % 2 == 0) ? pOutU[(p % (8 / uHrzCScale)) / 2] : pOutV[(p % (8 / uHrzCScale)) / 2]) = pInC[p];
          }

          if(H + h + 4 <= iHeightC)
          {
            pOutU += uPitchDstU;
            pOutV += uPitchDstV;

            for(int p = (3 * (8 / uHrzCScale)); p < (3 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
              ((p % 2 == 0) ? pOutU[(p % (8 / uHrzCScale)) / 2] : pOutV[(p % (8 / uHrzCScale)) / 2]) = pInC[p];
          }
          pInC += ((4 * 8) / uHrzCScale);
        }

        pInC += (8 / uHrzCScale) * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }
}

/****************************************************************************/
void T608_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  ALX8_To_I4XX(pSrc, pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void T608_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T608_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void T608_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  ALX8_To_I4XX(pSrc, pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4Block8b(uint8_t* pTiled4x4, TUntiled* pUntiled, int iUntiledPitch, FConvert convert)
{
  pUntiled[0] = convert(pTiled4x4[0]);
  pUntiled[1] = convert(pTiled4x4[1]);
  pUntiled[2] = convert(pTiled4x4[2]);
  pUntiled[3] = convert(pTiled4x4[3]);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(pTiled4x4[4]);
  pUntiled[1] = convert(pTiled4x4[5]);
  pUntiled[2] = convert(pTiled4x4[6]);
  pUntiled[3] = convert(pTiled4x4[7]);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(pTiled4x4[8]);
  pUntiled[1] = convert(pTiled4x4[9]);
  pUntiled[2] = convert(pTiled4x4[10]);
  pUntiled[3] = convert(pTiled4x4[11]);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(pTiled4x4[12]);
  pUntiled[1] = convert(pTiled4x4[13]);
  pUntiled[2] = convert(pTiled4x4[14]);
  pUntiled[3] = convert(pTiled4x4[15]);
}

/****************************************************************************/
static void Untile4x4Block8bTo8b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block8b<uint8_t>((uint8_t*)pTiled4x4, (uint8_t*)pUntiled, iUntiledPitch, [](uint8_t u8) { return u8; });
}

/****************************************************************************/
static void Untile4x4Block8bTo10b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block8b<uint16_t>((uint8_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, [](uint8_t u8) { return uint16_t(u8) << 2; });
}

/****************************************************************************/
static void Untile4x4Block8bTo12b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block8b<uint16_t>((uint8_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, [](uint8_t u8) { return uint16_t(u8) << 4; });
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4Block10b(uint16_t* pTiled4x4, TUntiled* pUntiled, int iUntiledPitch, FConvert convert)
{
  pUntiled[0] = convert(pTiled4x4[0] & 0x3FF);
  pUntiled[1] = convert(((pTiled4x4[0] >> 10) | (pTiled4x4[1] << 6)) & 0x3FF);
  pUntiled[2] = convert((pTiled4x4[1] >> 4) & 0x3FF);
  pUntiled[3] = convert(((pTiled4x4[1] >> 14) | (pTiled4x4[2] << 2)) & 0x3FF);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(((pTiled4x4[2] >> 8) | (pTiled4x4[3] << 8)) & 0x3FF);
  pUntiled[1] = convert((pTiled4x4[3] >> 2) & 0x3FF);
  pUntiled[2] = convert(((pTiled4x4[3] >> 12) | (pTiled4x4[4] << 4)) & 0x3FF);
  pUntiled[3] = convert(pTiled4x4[4] >> 6);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(pTiled4x4[5] & 0x3FF);
  pUntiled[1] = convert(((pTiled4x4[5] >> 10) | (pTiled4x4[6] << 6)) & 0x3FF);
  pUntiled[2] = convert((pTiled4x4[6] >> 4) & 0x3FF);
  pUntiled[3] = convert(((pTiled4x4[6] >> 14) | (pTiled4x4[7] << 2)) & 0x3FF);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(((pTiled4x4[7] >> 8) | (pTiled4x4[8] << 8)) & 0x3FF);
  pUntiled[1] = convert((pTiled4x4[8] >> 2) & 0x3FF);
  pUntiled[2] = convert(((pTiled4x4[8] >> 12) | (pTiled4x4[9] << 4)) & 0x3FF);
  pUntiled[3] = convert(pTiled4x4[9] >> 6);
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4Block12b(uint16_t* pTiled4x4, TUntiled* pUntiled, int iUntiledPitch, FConvert convert)
{
  for(int row = 0; row < 4; row++)
  {
    int off = row * 3;
    pUntiled[0] = convert(pTiled4x4[0 + off] & 0xFFF);
    pUntiled[1] = convert(((pTiled4x4[0 + off] >> 12) & 0xF) | ((pTiled4x4[1 + off] & 0xFF) << 4));
    pUntiled[2] = convert(((pTiled4x4[1 + off] >> 8) & 0xFF) | ((pTiled4x4[2 + off] & 0xF) << 8));
    pUntiled[3] = convert((pTiled4x4[2 + off] >> 4) & 0xFFF);
    pUntiled += iUntiledPitch;
  }
}

/****************************************************************************/
static void Untile4x4Block10bTo8b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint8_t>((uint16_t*)pTiled4x4, (uint8_t*)pUntiled, iUntiledPitch, RND_10B_TO_8B);
}

/****************************************************************************/
static void Untile4x4Block10bTo10b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint16_t>((uint16_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void Untile4x4Block10bTo12b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint16_t>((uint16_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, [](uint16_t u16) { return u16 << 2; });
}

/****************************************************************************/
static void Untile4x4Block12bTo8b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint8_t>((uint16_t*)pTiled4x4, (uint8_t*)pUntiled, iUntiledPitch, RND_12B_TO_8B);
}

/****************************************************************************/
static void Untile4x4Block12bTo10b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint16_t>((uint16_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, RND_12B_TO_10B);
}

/****************************************************************************/
static void Untile4x4Block12bTo12b(void* pTiled4x4, void* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint16_t>((uint16_t*)pTiled4x4, (uint16_t*)pUntiled, iUntiledPitch, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4ChromaBlock12bToPlanar(uint16_t* pTiled4x4, TUntiled* pUntiledU, TUntiled* pUntiledV, int iPitchU, int iPitchV, FConvert convert)
{
  for(int row = 0; row < 4; row++)
  {
    int off = row * 3;
    pUntiledU[0] = convert(pTiled4x4[0 + off] & 0xFFF);
    pUntiledV[0] = convert(((pTiled4x4[0 + off] >> 12) & 0xF) | ((pTiled4x4[1 + off] & 0xFF) << 4));
    pUntiledU[1] = convert(((pTiled4x4[1 + off] >> 8) & 0xFF) | ((pTiled4x4[2 + off] & 0xF) << 8));
    pUntiledV[1] = convert((pTiled4x4[2 + off] >> 4) & 0xFFF);
    pUntiledU += iPitchU;
    pUntiledV += iPitchV;
  }
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4ChromaBlock10bToPlanar(uint16_t* pTiled4x4, TUntiled* pUntiledU, TUntiled* pUntiledV, int iPitchU, int iPitchV, FConvert convert)
{
  pUntiledU[0] = convert(pTiled4x4[0] & 0x3FF);
  pUntiledV[0] = convert(((pTiled4x4[0] >> 10) | (pTiled4x4[1] << 6)) & 0x3FF);
  pUntiledU[1] = convert((pTiled4x4[1] >> 4) & 0x3FF);
  pUntiledV[1] = convert(((pTiled4x4[1] >> 14) | (pTiled4x4[2] << 2)) & 0x3FF);
  pUntiledU += iPitchU;
  pUntiledV += iPitchV;
  pUntiledU[0] = convert(((pTiled4x4[2] >> 8) | (pTiled4x4[3] << 8)) & 0x3FF);
  pUntiledV[0] = convert((pTiled4x4[3] >> 2) & 0x3FF);
  pUntiledU[1] = convert(((pTiled4x4[3] >> 12) | (pTiled4x4[4] << 4)) & 0x3FF);
  pUntiledV[1] = convert(pTiled4x4[4] >> 6);
  pUntiledU += iPitchU;
  pUntiledV += iPitchV;
  pUntiledU[0] = convert(pTiled4x4[5] & 0x3FF);
  pUntiledV[0] = convert(((pTiled4x4[5] >> 10) | (pTiled4x4[6] << 6)) & 0x3FF);
  pUntiledU[1] = convert((pTiled4x4[6] >> 4) & 0x3FF);
  pUntiledV[1] = convert(((pTiled4x4[6] >> 14) | (pTiled4x4[7] << 2)) & 0x3FF);
  pUntiledU += iPitchU;
  pUntiledV += iPitchV;
  pUntiledU[0] = convert(((pTiled4x4[7] >> 8) | (pTiled4x4[8] << 8)) & 0x3FF);
  pUntiledV[0] = convert((pTiled4x4[8] >> 2) & 0x3FF);
  pUntiledU[1] = convert(((pTiled4x4[8] >> 12) | (pTiled4x4[9] << 4)) & 0x3FF);
  pUntiledV[1] = convert(pTiled4x4[9] >> 6);
}

/****************************************************************************/
static void Untile4x4ChromaBlock10bToPlanar8b(uint16_t* pTiled4x4, uint8_t* pUntiledU, uint8_t* pUntiledV, int iPitchU, int iPitchV)
{
  Untile4x4ChromaBlock10bToPlanar<uint8_t>(pTiled4x4, pUntiledU, pUntiledV, iPitchU, iPitchV, RND_10B_TO_8B);
}

/****************************************************************************/
static void Untile4x4ChromaBlock10bToPlanar10b(uint16_t* pTiled4x4, uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV)
{
  Untile4x4ChromaBlock10bToPlanar<uint16_t>(pTiled4x4, pUntiledU, pUntiledV, iPitchU, iPitchV, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void Untile4x4ChromaBlock12bToPlanar8b(uint16_t* pTiled4x4, uint8_t* pUntiledU, uint8_t* pUntiledV, int iPitchU, int iPitchV)
{
  Untile4x4ChromaBlock12bToPlanar<uint8_t>(pTiled4x4, pUntiledU, pUntiledV, iPitchU, iPitchV, RND_12B_TO_8B);
}

/****************************************************************************/
static void Untile4x4ChromaBlock12bToPlanar10b(uint16_t* pTiled4x4, uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV)
{
  Untile4x4ChromaBlock12bToPlanar<uint16_t>(pTiled4x4, pUntiledU, pUntiledV, iPitchU, iPitchV, RND_12B_TO_10B);
}

/****************************************************************************/
static void Untile4x4ChromaBlock12bToPlanar12b(uint16_t* pTiled4x4, uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV)
{
  Untile4x4ChromaBlock12bToPlanar<uint16_t>(pTiled4x4, pUntiledU, pUntiledV, iPitchU, iPitchV, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
template<typename SrcType, typename DstType>
static void T64_Untile_Plane(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, AL_EPlaneId ePlaneId, AL_TDimension tPlaneDim)
{
  const int iTileW = 64;
  const int iTileH = 4;

  void (* Untile4x4Block) (void*, void*, int);

  if(bdIn == 12)
  {
    if(bdOut == 12)
      Untile4x4Block = Untile4x4Block12bTo12b;
    else if(bdOut == 10)
      Untile4x4Block = Untile4x4Block12bTo10b;
    else
      Untile4x4Block = Untile4x4Block12bTo8b;
  }
  else if(bdIn == 10)
  {
    if(bdOut == 12)
      Untile4x4Block = Untile4x4Block10bTo12b;
    else if(bdOut == 10)
      Untile4x4Block = Untile4x4Block10bTo10b;
    else
      Untile4x4Block = Untile4x4Block10bTo8b;
  }
  else
  {
    if(bdOut == 12)
      Untile4x4Block = Untile4x4Block8bTo12b;
    else if(bdOut == 10)
      Untile4x4Block = Untile4x4Block8bTo10b;
    else
      Untile4x4Block = Untile4x4Block8bTo8b;
  }

  auto pSrcData = (SrcType*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneId);
  auto pDstData = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneId);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneId) / sizeof(SrcType);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneId) / sizeof(DstType);

  /* Src increment = (16 * bdIn) / (8 * sizeof(SrcType)) */
  const int iSrcIncr4x4 = (bdIn << 1) / sizeof(SrcType);

  for(int H = 0; H < tPlaneDim.iHeight; H += iTileH)
  {
    SrcType* pIn = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tPlaneDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tPlaneDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tPlaneDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          DstType* pOut = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block(pIn, pOut, iPitchDst);
          pIn += iSrcIncr4x4;
        }

        pIn += iSrcIncr4x4 * (iCropW >> 2);
      }

      pIn += iSrcIncr4x4 * (iTileW >> 2) * (iCropH >> 2);
    }
  }
}

/****************************************************************************/
void T608_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T608_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T608_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T608_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 12, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
void T608_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void T608_To_P012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y012(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 12, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P012));
}

/****************************************************************************/
static void Chroma_T608_To_I0XL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBitDepth)
{
  if(uBitDepth != 10 && uBitDepth != 12)
    throw std::runtime_error("uBitDepth(" + std::to_string(uBitDepth) + ") must be equal to 10 or 12");

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

  auto pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  auto pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  auto pDstDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;

  const int iTileW = 64;
  const int iTileH = 4;

  int iShift = uBitDepth == 10 ? 2 : 4;

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = pDstDataU + (H + h) * iPitchDstU + (W + w) / 2;
          uint16_t* pOutV = pDstDataV + (H + h) * iPitchDstV + (W + w) / 2;

          pOutU[0] = ((uint16_t)pInC[0]) << iShift;
          pOutV[0] = ((uint16_t)pInC[1]) << iShift;
          pOutU[1] = ((uint16_t)pInC[2]) << iShift;
          pOutV[1] = ((uint16_t)pInC[3]) << iShift;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[4]) << iShift;
          pOutV[0] = ((uint16_t)pInC[5]) << iShift;
          pOutU[1] = ((uint16_t)pInC[6]) << iShift;
          pOutV[1] = ((uint16_t)pInC[7]) << iShift;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[8]) << iShift;
          pOutV[0] = ((uint16_t)pInC[9]) << iShift;
          pOutU[1] = ((uint16_t)pInC[10]) << iShift;
          pOutV[1] = ((uint16_t)pInC[11]) << iShift;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[12]) << iShift;
          pOutV[0] = ((uint16_t)pInC[13]) << iShift;
          pOutU[1] = ((uint16_t)pInC[14]) << iShift;
          pOutV[1] = ((uint16_t)pInC[15]) << iShift;
          pInC += 16;
        }

        pInC += 16 * (iCropW >> 2);
      }

      pInC += 16 * (iTileW >> 2) * (iCropH >> 2);
    }
  }
}

/****************************************************************************/
void T608_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  Chroma_T608_To_I0XL(pSrc, pDst, 10);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T608_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y012(pSrc, pDst);

  // Chroma
  Chroma_T608_To_I0XL(pSrc, pDst, 12);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T6m8_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T608_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
static uint16_t getTile10BitVal(int w, int hInsideTile, uint16_t* pSrc)
{
  int offset = ((w >> 2) << 4) + ((hInsideTile & 0x3) << 2) + (w & 0x3); // (w / 4) * 16 + (hInsideTile % 4) * 4 + w % 4;
  int u16offset = (offset * 5) >> 3; // offset * 10 / (8 * sizeof(uint16_t));
  int bitOffset = (offset * 10) & 0xF; // (offset * 10) % (8 * sizeof(uint16_t));
  int remainingBit = bitOffset - 6; // 10 + bitOffset - (8 * sizeof(uint16_t));

  pSrc += u16offset;
  uint16_t result = ((*pSrc) >> bitOffset);

  if(remainingBit > 0)
  {
    result |= (*(pSrc + 1)) << (10 - remainingBit);
  }

  result &= 0x3FF;

  return result;
}

/****************************************************************************/
static void Tile_To_XV_OneComponent(AL_TBuffer const* pSrcBuf, AL_TBuffer* pDstBuf, bool bProcessY, int iVrtScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrcBuf);

  int iDstHeight = (tDim.iHeight + iVrtScale - 1) / iVrtScale;

  AL_EPlaneId ePlaneID = bProcessY ? AL_PLANE_Y : AL_PLANE_UV;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrcBuf, ePlaneID);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, ePlaneID);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrcBuf, ePlaneID);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, ePlaneID);

  if((iPitchDst % 4) != 0)
    throw std::runtime_error("iPitchDst (" + std::to_string(iPitchDst) + ") should be aligned to 4");

  if(bProcessY)
  {
    if(iPitchDst < ((((tDim.iWidth + 2) / 3) * 4)))
      throw std::runtime_error("iPitchDst (" + std::to_string(iPitchDst) + ") should be higher than" + std::to_string((((tDim.iWidth + 2) / 3) * 4)));
  }

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + (h >> 2) * iPitchSrc);

    int hInsideTile = h & 0x3;

    int w = 0;
    int wStop = tDim.iWidth - 2;

    while(w < wStop)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 20;
      ++pDst;
    }

    if(w < tDim.iWidth)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);

      if(w < tDim.iWidth)
      {
        *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      }
    }
  }
}

/****************************************************************************/
void T628_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T628_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T628_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  ALX8_To_I4XX(pSrc, pDst, 2, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void T628_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
static void Chroma_T628_To_I2XL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBitDepth)
{
  if(uBitDepth != 10 && uBitDepth != 12)
    throw std::runtime_error("uBitDepth(" + std::to_string(uBitDepth) + ") must be equal to 10 or 12");

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

  auto pSrcDataC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  auto pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  auto pDstDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  int iShift = uBitDepth == 10 ? 2 : 4;

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    uint8_t* pInC = pSrcDataC + (h >> 2) * iPitchSrc;

    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pOutU = pDstDataU + h * iPitchDstU + w / 2;
      uint16_t* pOutV = pDstDataV + h * iPitchDstV + w / 2;

      pOutU[0] = ((uint16_t)pInC[0]) << iShift;
      pOutV[0] = ((uint16_t)pInC[1]) << iShift;
      pOutU[1] = ((uint16_t)pInC[2]) << iShift;
      pOutV[1] = ((uint16_t)pInC[3]) << iShift;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[4]) << iShift;
      pOutV[0] = ((uint16_t)pInC[5]) << iShift;
      pOutU[1] = ((uint16_t)pInC[6]) << iShift;
      pOutV[1] = ((uint16_t)pInC[7]) << iShift;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[8]) << iShift;
      pOutV[0] = ((uint16_t)pInC[9]) << iShift;
      pOutU[1] = ((uint16_t)pInC[10]) << iShift;
      pOutV[1] = ((uint16_t)pInC[11]) << iShift;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[12]) << iShift;
      pOutV[0] = ((uint16_t)pInC[13]) << iShift;
      pOutU[1] = ((uint16_t)pInC[14]) << iShift;
      pOutV[1] = ((uint16_t)pInC[15]) << iShift;
      pInC += 16;
    }
  }
}

/****************************************************************************/
void T628_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  Chroma_T628_To_I2XL(pSrc, pDst, 10);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T628_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y012(pSrc, pDst);

  // Chroma
  Chroma_T628_To_I2XL(pSrc, pDst, 12);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2CL));
}

/****************************************************************************/
void T628_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
void T60A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T60C_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T60A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T60C_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 10, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T60C_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
template<typename DstType>
static void T6XX_To_4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, int iHScale, int iVScale)
{
  // Luma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, DstType>(pSrc, pDst, bdIn, bdOut, AL_PLANE_Y, tDim);

  // Chroma
  int iWidthC = 2 * ((tDim.iWidth + iHScale - 1) / iHScale);
  int iHeightC = (tDim.iHeight + iVScale - 1) / iVScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(DstType);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(DstType);

  auto pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  auto pDstDataU = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  auto pDstDataV = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int h = 0; h < iHeightC; h += 4)
  {
    uint16_t* pInC = pSrcData + (h >> 2) * iPitchSrc;

    for(int w = 0; w < iWidthC; w += 4)
    {
      DstType* pOutU = pDstDataU + h * iPitchDstU + w / 2;
      DstType* pOutV = pDstDataV + h * iPitchDstV + w / 2;

      if(bdIn == 12)
      {
        if(bdOut == 12)
        {
          Untile4x4ChromaBlock12bToPlanar12b(pInC, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV);
        }
        else if(bdOut == 10)
        {
          Untile4x4ChromaBlock12bToPlanar10b(pInC, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV);
        }
        else
        {
          Untile4x4ChromaBlock12bToPlanar8b(pInC, (uint8_t*)pOutU, (uint8_t*)pOutV, iPitchDstU, iPitchDstV);
        }
      }
      else // bdIn 10 bit
      {
        bdOut == 10 ?
        Untile4x4ChromaBlock10bToPlanar10b(pInC, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV) :
        Untile4x4ChromaBlock10bToPlanar8b(pInC, (uint8_t*)pOutU, (uint8_t*)pOutV, iPitchDstU, iPitchDstV);
      }
      pInC += bdIn;
    }
  }
}

/****************************************************************************/
void T60A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 10, 8, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void T60C_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 12, 8, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void T60A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T60A_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void T60C_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T60A_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void T60A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 10, 8, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void T60C_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 12, 8, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void T60A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 10, 10, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T60C_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 12, 12, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0CL));
}

/****************************************************************************/
void T60C_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 12, 10, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T60A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T60A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void T60A_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
void T60A_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV10));
}

/****************************************************************************/
void T62A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T62C_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60C_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T62C_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60C_To_Y012(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
void T62A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T62A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 10, 10, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T62C_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 12, 12, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2CL));
}

/****************************************************************************/
void T62C_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint16_t>(pSrc, pDst, 12, 10, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T62C_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 12, 8, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void T62A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T6XX_To_4XX<uint8_t>(pSrc, pDst, 10, 8, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void T62A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void T62X_To_P21X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBd)
{
  // Luma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, uBd, uBd, AL_PLANE_Y, tDim);

  // Chroma
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, uBd, uBd, AL_PLANE_UV, tDim);
}

/****************************************************************************/
void T62A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T62X_To_P21X(pSrc, pDst, 10);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
void T62C_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T62X_To_P21X(pSrc, pDst, 12);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P212));
}

/****************************************************************************/
void T648_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void T64A_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void T648_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
void T64A_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
void T64C_To_I4CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4CL));
}

/****************************************************************************/
void T64C_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 10, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 10, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint16_t, uint16_t>(pSrc, pDst, 12, 10, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
void T64C_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_Y, tDim);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_U, tDim);
  T64_Untile_Plane<uint16_t, uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Tile4x4Block8b(TUntiled* pUntiled, int iUntiledPitch, uint8_t* pTiled4x4, FConvert convert)
{
  pTiled4x4[0] = convert(pUntiled[0]);
  pTiled4x4[1] = convert(pUntiled[1]);
  pTiled4x4[2] = convert(pUntiled[2]);
  pTiled4x4[3] = convert(pUntiled[3]);
  pUntiled += iUntiledPitch;

  pTiled4x4[4] = convert(pUntiled[0]);
  pTiled4x4[5] = convert(pUntiled[1]);
  pTiled4x4[6] = convert(pUntiled[2]);
  pTiled4x4[7] = convert(pUntiled[3]);
  pUntiled += iUntiledPitch;

  pTiled4x4[8] = convert(pUntiled[0]);
  pTiled4x4[9] = convert(pUntiled[1]);
  pTiled4x4[10] = convert(pUntiled[2]);
  pTiled4x4[11] = convert(pUntiled[3]);
  pUntiled += iUntiledPitch;

  pTiled4x4[12] = convert(pUntiled[0]);
  pTiled4x4[13] = convert(pUntiled[1]);
  pTiled4x4[14] = convert(pUntiled[2]);
  pTiled4x4[15] = convert(pUntiled[3]);
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Tile4x4Block10b(TUntiled* pUntiled, int iUntiledPitch, uint16_t* pTiled4x4, FConvert convert)
{
  pTiled4x4[0] = convert(
    ((pUntiled[1] & 0x3F) << 10) |
    (pUntiled[0] & 0x3FF));
  pTiled4x4[1] = convert(
    ((pUntiled[3] & 0x3) << 14) |
    ((pUntiled[2] & 0x3FF) << 4) |
    ((pUntiled[1] >> 6) & 0xF));
  pTiled4x4[2] = convert(
    ((pUntiled[iUntiledPitch + 0] & 0xFF) << 8) |
    ((pUntiled[3] >> 2) & 0xFF));
  pUntiled += iUntiledPitch;
  pTiled4x4[3] = convert(
    ((pUntiled[2] & 0xF) << 12) |
    ((pUntiled[1] & 0x3FF) << 2) |
    ((pUntiled[0] >> 8) & 0x3));
  pTiled4x4[4] = convert(
    ((pUntiled[3] & 0x3FF) << 6) |
    ((pUntiled[2] >> 4) & 0x3F));
  pUntiled += iUntiledPitch;

  pTiled4x4[5] = convert(
    ((pUntiled[1] & 0x3F) << 10) |
    (pUntiled[0] & 0x3FF));
  pTiled4x4[6] = convert(
    ((pUntiled[3] & 0x3) << 14) |
    ((pUntiled[2] & 0x3FF) << 4) |
    ((pUntiled[1] >> 6) & 0xF));
  pTiled4x4[7] = convert(
    ((pUntiled[iUntiledPitch + 0] & 0xFF) << 8) |
    ((pUntiled[3] >> 2) & 0xFF));
  pUntiled += iUntiledPitch;
  pTiled4x4[8] = convert(
    ((pUntiled[2] & 0xF) << 12) |
    ((pUntiled[1] & 0x3FF) << 2) |
    ((pUntiled[0] >> 8) & 0x3));
  pTiled4x4[9] = convert(
    ((pUntiled[3] & 0x3FF) << 6) |
    ((pUntiled[2] >> 4) & 0x3F));
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Tile4x4Block12b(TUntiled* pUntiled, int iUntiledPitch, uint16_t* pTiled4x4, FConvert convert)
{
  for(int row = 0; row < 4; row++)
  {
    int off = row * 3;

    pTiled4x4[0 + off] = convert(((pUntiled[1] & 0xF) << 12) | (pUntiled[0] & 0xFFF));
    pTiled4x4[1 + off] = convert(((pUntiled[2] & 0xFF) << 8) | ((pUntiled[1] >> 4) & 0xFF));
    pTiled4x4[2 + off] = convert(((pUntiled[3] & 0xFFF) << 4) | ((pUntiled[2] >> 8) & 0xF));
    pUntiled += iUntiledPitch;
  }
}

/****************************************************************************/
static void Tile4x4Block8bTo8b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block8b<uint8_t>((uint8_t*)pUntiled, iUntiledPitch, (uint8_t*)pTiled4x4, [](uint8_t u8) { return u8; });
}

/****************************************************************************/
static void Tile4x4Block8bTo10b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block8b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint8_t*)pTiled4x4, [](uint8_t u8) { return uint16_t(u8) << 2; });
}

/****************************************************************************/
static void Tile4x4Block8bTo12b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block8b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint8_t*)pTiled4x4, [](uint8_t u8) { return uint16_t(u8) << 4; });
}

/****************************************************************************/
static void Tile4x4Block10bTo8b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block10b<uint8_t>((uint8_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, RND_10B_TO_8B);
}

/****************************************************************************/
static void Tile4x4Block10bTo10b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block10b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void Tile4x4Block10bTo12b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block10b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, [](uint16_t u16) { return u16 << 2; });
}

/****************************************************************************/
static void Tile4x4Block12bTo8b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block12b<uint8_t>((uint8_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, RND_12B_TO_8B);
}

/****************************************************************************/
static void Tile4x4Block12bTo10b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block12b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, RND_12B_TO_10B);
}

/****************************************************************************/
static void Tile4x4Block12bTo12b(void* pUntiled, int iUntiledPitch, void* pTiled4x4)
{
  Tile4x4Block12b<uint16_t>((uint16_t*)pUntiled, iUntiledPitch, (uint16_t*)pTiled4x4, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
template<typename SrcType, typename DstType>
static void Plane_Tile_T64(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, AL_EPlaneId ePlaneId, AL_TDimension tPlaneDim)
{
  const int iTileW = 64;
  const int iTileH = 4;

  void (* Tile4x4Block) (void*, int, void*) = nullptr;

  if(bdIn == 12)
  {
    if(bdOut == 12)
      Tile4x4Block = Tile4x4Block12bTo12b;
    else if(bdOut == 10)
      Tile4x4Block = Tile4x4Block12bTo10b;
    else
      Tile4x4Block = Tile4x4Block12bTo8b;
  }
  else if(bdIn == 10)
  {
    if(bdOut == 12)
      Tile4x4Block = Tile4x4Block10bTo12b;
    else if(bdOut == 10)
      Tile4x4Block = Tile4x4Block10bTo10b;
    else
      Tile4x4Block = Tile4x4Block10bTo8b;
  }
  else
  {
    if(bdOut == 12)
      Tile4x4Block = Tile4x4Block8bTo12b;
    else if(bdOut == 10)
      Tile4x4Block = Tile4x4Block8bTo10b;
    else
      Tile4x4Block = Tile4x4Block8bTo8b;
  }

  auto pSrcData = (SrcType*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneId);
  auto pDstData = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneId);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneId) / sizeof(SrcType);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneId) / sizeof(DstType);

  /* Src increment = (16 * bdIn) / (8 * sizeof(SrcType)) */
  const int iSrcIncr4x4 = (bdIn << 1) / sizeof(SrcType);

  for(int H = 0; H < tPlaneDim.iHeight; H += iTileH)
  {
    DstType* pOut = pDstData + (H / iTileH) * iPitchDst;

    int iCropH = (H + iTileH) - tPlaneDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tPlaneDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tPlaneDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          SrcType* pIn = pSrcData + (H + h) * iPitchSrc + (W + w);
          Tile4x4Block(pIn, iPitchSrc, pOut);
          pOut += iSrcIncr4x4;
        }

        pOut += iSrcIncr4x4 * (iCropW >> 2);
      }

      pOut += iSrcIncr4x4 * (iTileW >> 2) * (iCropH >> 2);
    }
  }
}

/****************************************************************************/
static void Chroma_I0XL_To_T608(AL_TBuffer const* pDst, AL_TBuffer* pSrc, uint8_t uBitDepth)
{
  if(uBitDepth != 10 && uBitDepth != 12)
    throw std::runtime_error("uBitDepth(" + std::to_string(uBitDepth) + ") must be equal to 10 or 12");

  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(uint16_t);
  auto pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  auto pSrcDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pSrcDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;

  const int iTileW = 64;
  const int iTileH = 4;

  int iShift = uBitDepth == 10 ? 2 : 4;

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pOutC = pDstData + (H / iTileH) * iPitchDst;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pInU = pSrcDataU + (H + h) * iPitchSrcU + (W + w) / 2;
          uint16_t* pInV = pSrcDataV + (H + h) * iPitchSrcV + (W + w) / 2;

          pOutC[0] = (uint8_t)(pInU[0] >> iShift);
          pOutC[1] = (uint8_t)(pInV[0] >> iShift);
          pOutC[2] = (uint8_t)(pInU[1] >> iShift);
          pOutC[3] = (uint8_t)(pInV[1] >> iShift);
          pInU += iPitchSrcU;
          pInV += iPitchSrcV;
          pOutC[4] = (uint8_t)(pInU[0] >> iShift);
          pOutC[5] = (uint8_t)(pInV[0] >> iShift);
          pOutC[6] = (uint8_t)(pInU[1] >> iShift);
          pOutC[7] = (uint8_t)(pInV[1] >> iShift);
          pInU += iPitchSrcU;
          pInV += iPitchSrcV;
          pOutC[8] = (uint8_t)(pInU[0] >> iShift);
          pOutC[9] = (uint8_t)(pInV[0] >> iShift);
          pOutC[10] = (uint8_t)(pInU[1] >> iShift);
          pOutC[11] = (uint8_t)(pInV[1] >> iShift);
          pInU += iPitchSrcU;
          pInV += iPitchSrcV;
          pOutC[12] = (uint8_t)(pInU[0] >> iShift);
          pOutC[13] = (uint8_t)(pInV[0] >> iShift);
          pOutC[14] = (uint8_t)(pInU[1] >> iShift);
          pOutC[15] = (uint8_t)(pInV[1] >> iShift);
          pOutC += 16;
        }

        pOutC += 16 * (iCropW >> 2);
      }

      pOutC += 16 * (iTileW >> 2) * (iCropH >> 2);
    }
  }
}

/****************************************************************************/
static void I4XX_To_ALX8(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  const int iTileW = 64;
  const int iTileH = 4;

  uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  uint32_t uPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U);
  uint32_t uPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V);
  uint8_t* pSrcDataU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint8_t* pSrcDataV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale * 2;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pOutC = pDstData + (H / iTileH) * uPitchDst;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 4)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 8 / uHrzCScale)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pInU = pSrcDataU + (H + h) * uPitchSrcU + (W + w) / uHrzCScale;
          uint8_t* pInV = pSrcDataV + (H + h) * uPitchSrcV + (W + w) / uHrzCScale;

          for(int p = 0; p < (8 / uHrzCScale); p++)
            pOutC[p] = ((p % 2 == 0) ? pInU[(p % (8 / uHrzCScale)) / 2] : pInV[(p % (8 / uHrzCScale)) / 2]);

          pInU += uPitchSrcU;
          pInV += uPitchSrcV;

          for(int p = (1 * (8 / uHrzCScale)); p < (1 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
            pOutC[p] = ((p % 2 == 0) ? pInU[(p % (8 / uHrzCScale)) / 2] : pInV[(p % (8 / uHrzCScale)) / 2]);

          if(H + h + 3 <= iHeightC)
          {
            pInU += uPitchSrcU;
            pInV += uPitchSrcV;

            for(int p = (2 * (8 / uHrzCScale)); p < (2 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
              pOutC[p] = ((p % 2 == 0) ? pInU[(p % (8 / uHrzCScale)) / 2] : pInV[(p % (8 / uHrzCScale)) / 2]);
          }

          if(H + h + 4 <= iHeightC)
          {
            pInU += uPitchSrcU;
            pInV += uPitchSrcV;

            for(int p = (3 * (8 / uHrzCScale)); p < (3 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
              pOutC[p] = ((p % 2 == 0) ? pInU[(p % (8 / uHrzCScale)) / 2] : pInV[(p % (8 / uHrzCScale)) / 2]);
          }
          pOutC += ((4 * 8) / uHrzCScale);
        }

        pOutC += (8 / uHrzCScale) * iCropW;
      }

      pOutC += iCropH * iTileW;
    }
  }
}

/****************************************************************************/
void Y010_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void Y012_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void P010_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T608(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void Y800_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_Y, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void NV12_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void I0AL_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T608(pSrc, pDst);

  // Chroma
  Chroma_I0XL_To_T608(pSrc, pDst, 10);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void I0CL_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y012_To_T608(pSrc, pDst);

  // Chroma
  Chroma_I0XL_To_T608(pSrc, pDst, 12);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void I420_To_T6m8(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_T608(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T6m8));
}

/****************************************************************************/
void I420_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);

  // Chroma
  I4XX_To_ALX8(pSrc, pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T608));
}

/****************************************************************************/
void IYUV_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_T608(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void YV12_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);

  // Chroma
  I4XX_To_ALX8(pSrc, pDst, 2, 2);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
template<typename TTiled, typename FConvert>
static void PlanarToTile4x4ChromaBlock10b(TTiled* pUntiledU, TTiled* pUntiledV, int iTiledPitchU, int iTiledPitchV, uint16_t* pTiled4x4, FConvert convert)
{
  pTiled4x4[0] = convert(
    ((pUntiledV[0] & 0x3F) << 10) |
    (pUntiledU[0] & 0x3FF));
  pTiled4x4[1] = convert(
    ((pUntiledV[1] & 0x3) << 14) |
    ((pUntiledU[1] & 0x3FF) << 4) |
    ((pUntiledV[0] >> 6) & 0xF));
  pTiled4x4[2] = convert(
    ((pUntiledU[iTiledPitchU + 0] & 0xFF) << 8) |
    ((pUntiledV[1] >> 2) & 0xFF));
  pUntiledU += iTiledPitchU;
  pUntiledV += iTiledPitchV;
  pTiled4x4[3] = convert(
    ((pUntiledU[1] & 0xF) << 12) |
    ((pUntiledV[0] & 0x3FF) << 2) |
    ((pUntiledU[0] >> 8) & 0x3));
  pTiled4x4[4] = convert(
    ((pUntiledV[1] & 0x3FF) << 6) |
    ((pUntiledU[1] >> 4) & 0x3F));
  pUntiledU += iTiledPitchU;
  pUntiledV += iTiledPitchV;

  pTiled4x4[5] = convert(
    ((pUntiledV[0] & 0x3F) << 10) |
    (pUntiledU[0] & 0x3FF));
  pTiled4x4[6] = convert(
    ((pUntiledV[1] & 0x3) << 14) |
    ((pUntiledU[1] & 0x3FF) << 4) |
    ((pUntiledV[0] >> 6) & 0xF));
  pTiled4x4[7] = convert(
    ((pUntiledU[iTiledPitchU + 0] & 0xFF) << 8) |
    ((pUntiledV[1] >> 2) & 0xFF));
  pUntiledU += iTiledPitchU;
  pUntiledV += iTiledPitchV;
  pTiled4x4[8] = convert(
    ((pUntiledU[1] & 0xF) << 12) |
    ((pUntiledV[0] & 0x3FF) << 2) |
    ((pUntiledU[0] >> 8) & 0x3));
  pTiled4x4[9] = convert(
    ((pUntiledV[1] & 0x3FF) << 6) |
    ((pUntiledU[1] >> 4) & 0x3F));
}

/****************************************************************************/
template<typename TTiled, typename FConvert>
static void PlanarToTile4x4ChromaBlock12b(TTiled* pUntiledU, TTiled* pUntiledV, int iTiledPitchU, int iTiledPitchV, uint16_t* pTiled4x4, FConvert convert)
{
  for(int row = 0; row < 4; row++)
  {
    int off = row * 3;

    pTiled4x4[0 + off] = convert(((pUntiledV[0] & 0xF) << 12) | (pUntiledU[0] & 0xFFF));
    pTiled4x4[1 + off] = convert(((pUntiledU[1] & 0xFF) << 8) | ((pUntiledV[0] >> 4) & 0xFF));
    pTiled4x4[2 + off] = convert(((pUntiledV[1] & 0xFFF) << 4) | ((pUntiledU[1] >> 8) & 0xF));
    pUntiledU += iTiledPitchU;
    pUntiledV += iTiledPitchV;
  }
}

/****************************************************************************/
static void Planar8bToTile4x4ChromaBlock10b(uint8_t* pUntiledU, uint8_t* pUntiledV, int iPitchU, int iPitchV, uint16_t* pTiled4x4)
{
  PlanarToTile4x4ChromaBlock10b<uint8_t>(pUntiledU, pUntiledV, iPitchU, iPitchV, pTiled4x4, RND_10B_TO_8B);
}

/****************************************************************************/
static void Planar10bToTile4x4ChromaBlock10b(uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV, uint16_t* pTiled4x4)
{
  PlanarToTile4x4ChromaBlock10b<uint16_t>(pUntiledU, pUntiledV, iPitchU, iPitchV, pTiled4x4, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void Planar8bToTile4x4ChromaBlock12b(uint8_t* pUntiledU, uint8_t* pUntiledV, int iPitchU, int iPitchV, uint16_t* pTiled4x4)
{
  PlanarToTile4x4ChromaBlock12b<uint8_t>(pUntiledU, pUntiledV, iPitchU, iPitchV, pTiled4x4, RND_12B_TO_8B);
}

/****************************************************************************/
static void Planar10bToTile4x4ChromaBlock12b(uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV, uint16_t* pTiled4x4)
{
  PlanarToTile4x4ChromaBlock12b<uint16_t>(pUntiledU, pUntiledV, iPitchU, iPitchV, pTiled4x4, RND_12B_TO_10B);
}

/****************************************************************************/
static void Planar12bToTile4x4ChromaBlock12b(uint16_t* pUntiledU, uint16_t* pUntiledV, int iPitchU, int iPitchV, uint16_t* pTiled4x4)
{
  PlanarToTile4x4ChromaBlock12b<uint16_t>(pUntiledU, pUntiledV, iPitchU, iPitchV, pTiled4x4, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
template<typename SrcType>
static void From_4XX_To_T6XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, int iHScale, int iVScale)
{
  // Luma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  Plane_Tile_T64<SrcType, uint16_t>(pSrc, pDst, bdIn, bdOut, AL_PLANE_Y, tDim);

  // Chroma
  int iWidthC = 2 * ((tDim.iWidth + iHScale - 1) / iHScale);
  int iHeightC = (tDim.iHeight + iVScale - 1) / iVScale;

  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(SrcType);
  int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(SrcType);

  auto pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  auto pSrcDataU = (SrcType*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pSrcDataV = (SrcType*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);

  for(int h = 0; h < iHeightC; h += 4)
  {
    uint16_t* pInC = pDstData + (h >> 2) * iPitchDst;

    for(int w = 0; w < iWidthC; w += 4)
    {
      SrcType* pOutU = pSrcDataU + h * iPitchSrcU + w / 2;
      SrcType* pOutV = pSrcDataV + h * iPitchSrcV + w / 2;

      if(bdIn == 12)
      {
        if(bdOut == 12)
        {
          Planar12bToTile4x4ChromaBlock12b((uint16_t*)pOutU, (uint16_t*)pOutV, iPitchSrcU, iPitchSrcV, pInC);
        }
        else if(bdOut == 10)
        {
          Planar10bToTile4x4ChromaBlock12b((uint16_t*)pOutU, (uint16_t*)pOutV, iPitchSrcU, iPitchSrcV, pInC);
        }
        else
        {
          Planar8bToTile4x4ChromaBlock12b((uint8_t*)pOutU, (uint8_t*)pOutV, iPitchSrcU, iPitchSrcV, pInC);
        }
      }
      else // bdIn 10 bit
      {
        bdOut == 10 ?
        Planar10bToTile4x4ChromaBlock10b((uint16_t*)pOutU, (uint16_t*)pOutV, iPitchSrcU, iPitchSrcV, pInC) :
        Planar8bToTile4x4ChromaBlock10b((uint8_t*)pOutU, (uint8_t*)pOutV, iPitchSrcU, iPitchSrcV, pInC);
      }
      pInC += bdIn;
    }
  }
}

/****************************************************************************/
void I420_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 8, 10, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void I420_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 8, 12, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void IYUV_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_T60A(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void YV12_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint8_t>(pSrc, pDst, 8, 10, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void I0AL_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 10, 10, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void I0AL_To_T6mA(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T6mA));
}

/****************************************************************************/
void I0CL_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 12, 12, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void I0CL_To_T6mC(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T6mC));
}

/****************************************************************************/
void I0AL_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 10, 12, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void Y800_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void Y800_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 8, 12, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void Y010_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void Y010_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 12, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void Y012_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60C));
}

/****************************************************************************/
void NV12_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T60A(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void P010_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T60A(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  tDim.iHeight = (tDim.iHeight + 1) >> 1;
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T60A));
}

/****************************************************************************/
void Y800_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T60A(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void Y800_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T60C(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void Y012_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y012_To_T60C(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void Y010_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T60A(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void I2AL_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 10, 10, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void I2CL_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 12, 12, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void I2AL_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 10, 12, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void I422_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 8, 12, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void I422_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  From_4XX_To_T6XX<uint16_t>(pSrc, pDst, 8, 10, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void NV16_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T60A(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void P21X_To_T62X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBd)
{
  // Luma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, uBd, uBd, AL_PLANE_Y, tDim);

  // Chroma
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, uBd, uBd, AL_PLANE_UV, tDim);
}

/****************************************************************************/
void P210_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  P21X_To_T62X(pSrc, pDst, 10);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62A));
}

/****************************************************************************/
void P212_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  P21X_To_T62X(pSrc, pDst, 12);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T62C));
}

/****************************************************************************/
void I444_To_T648(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T648));
}

/****************************************************************************/
void I444_To_T64A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 10, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T64A));
}

/****************************************************************************/
void I4AL_To_T648(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T648));
}

/****************************************************************************/
void I4AL_To_T64A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T64A));
}

/****************************************************************************/
void I4CL_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T64C));
}

/****************************************************************************/
void I4AL_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 12, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 12, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint16_t, uint16_t>(pSrc, pDst, 10, 12, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T64C));
}

/****************************************************************************/
void I444_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 12, AL_PLANE_Y, tDim);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 12, AL_PLANE_U, tDim);
  Plane_Tile_T64<uint8_t, uint16_t>(pSrc, pDst, 8, 12, AL_PLANE_V, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T64C));
}

/****************************************************************************/
void Y800_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void Y010_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T608(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void I422_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);

  // Chroma
  I4XX_To_ALX8(pSrc, pDst, 2, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void NV16_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y800_To_T608(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  Plane_Tile_T64<uint8_t, uint8_t>(pSrc, pDst, 8, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
static void Chroma_I2XL_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBitDepth)
{
  if(uBitDepth != 10 && uBitDepth != 12)
    throw std::runtime_error("uBitDepth(" + std::to_string(uBitDepth) + ") must be equal to 10 or 12");

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;

  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(uint16_t);

  auto pDstDataC = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  auto pSrcDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  auto pSrcDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);

  int iShift = uBitDepth == 10 ? 2 : 4;

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    uint8_t* pOutC = pDstDataC + (h >> 2) * iPitchDst;

    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pInU = pSrcDataU + h * iPitchSrcU + w / 2;
      uint16_t* pInV = pSrcDataV + h * iPitchSrcV + w / 2;

      pOutC[0] = (uint8_t)(pInU[0] >> iShift);
      pOutC[1] = (uint8_t)(pInV[0] >> iShift);
      pOutC[2] = (uint8_t)(pInU[1] >> iShift);
      pOutC[3] = (uint8_t)(pInV[1] >> iShift);
      pInU += iPitchSrcU;
      pInV += iPitchSrcV;
      pOutC[4] = (uint8_t)(pInU[0] >> iShift);
      pOutC[5] = (uint8_t)(pInV[0] >> iShift);
      pOutC[6] = (uint8_t)(pInU[1] >> iShift);
      pOutC[7] = (uint8_t)(pInV[1] >> iShift);
      pInU += iPitchSrcU;
      pInV += iPitchSrcV;
      pOutC[8] = (uint8_t)(pInU[0] >> iShift);
      pOutC[9] = (uint8_t)(pInV[0] >> iShift);
      pOutC[10] = (uint8_t)(pInU[1] >> iShift);
      pOutC[11] = (uint8_t)(pInV[1] >> iShift);
      pInU += iPitchSrcU;
      pInV += iPitchSrcV;
      pOutC[12] = (uint8_t)(pInU[0] >> iShift);
      pOutC[13] = (uint8_t)(pInV[0] >> iShift);
      pOutC[14] = (uint8_t)(pInU[1] >> iShift);
      pOutC[15] = (uint8_t)(pInV[1] >> iShift);
      pOutC += 16;
    }
  }
}

/****************************************************************************/
void I2AL_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T608(pSrc, pDst);

  // Chroma
  Chroma_I2XL_To_T628(pSrc, pDst, 10);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void I2CL_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y012_To_T608(pSrc, pDst);

  // Chroma
  Chroma_I2XL_To_T628(pSrc, pDst, 12);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void P210_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  Y010_To_T608(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  tDim.iWidth = ((tDim.iWidth + 1) >> 1) << 1;
  Plane_Tile_T64<uint16_t, uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_UV, tDim);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(T628));
}

/****************************************************************************/
void T62A_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
static void Read24BitsOn32Bits(uint32_t* pIn, uint8_t** pOut1, uint8_t** pOut2)
{
  *((*pOut1)++) = (*pIn >> 2) & 0xFF;
  *((*pOut2)++) = (*pIn >> 12) & 0xFF;
  *((*pOut1)++) = (*pIn >> 22) & 0xFF;
}

/****************************************************************************/
static void XVXX_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV10_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint8_t* pDstDataV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstU = pDstDataU + h * iPitchDstU;
    uint8_t* pDstV = pDstDataV + h * iPitchDstV;

    int w = iWidthC / 3;

    while(w--)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read24BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(iWidthC % 3 > 1)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
    }
    else if(iWidthC % 3 > 0)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(I422) : FOURCC(I420));
}

/****************************************************************************/
void XV15_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_I42X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_I42X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void XV10_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstY = (uint8_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
      *pDstY++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void XV10_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstY = (uint16_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void XV15_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  XV10_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void XV15_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  XV10_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void XV15_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void XV15_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void XV20_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV10_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void XV20_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV10_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void XVXX_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV10_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstC = (uint8_t*)(pDstData + h * iPitchDst);

    int w = iWidthC / 3;

    while(w--)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
      *pDstC++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(iWidthC % 3 > 1)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(iWidthC % 3 > 0)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(NV16) : FOURCC(NV12));
}

/****************************************************************************/
void XV15_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void XVXX_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV10_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstC = (uint16_t*)(pDstData + h * iPitchDst);

    int w = iWidthC / 3;

    while(w--)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(iWidthC % 3 > 1)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(iWidthC % 3 > 0)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(P210) : FOURCC(P010));
}

/****************************************************************************/
void XV15_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void Read30BitsOn32Bits(uint32_t* pIn, uint16_t** pOut1, uint16_t** pOut2)
{
  *((*pOut1)++) = (uint16_t)((*pIn) & 0x3FF);
  *((*pOut2)++) = (uint16_t)((*pIn >> 10) & 0x3FF);
  *((*pOut1)++) = (uint16_t)((*pIn >> 20) & 0x3FF);
}

/****************************************************************************/
static void XVXX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV10_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint8_t* pDstDataV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstU = (uint16_t*)(pDstDataU + h * iPitchDstU);
    uint16_t* pDstV = (uint16_t*)(pDstDataV + h * iPitchDstV);

    int w = iWidthC / 3;

    while(w--)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read30BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(iWidthC % 3 > 1)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(iWidthC % 3 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(I2AL) : FOURCC(I0AL));
}

/****************************************************************************/
void XV15_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NVXX_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  // Luma
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, 8);

  // Chroma
  auto const iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  auto const iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
  auto const iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);
  uint8_t* pBufInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pBufOutU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint8_t* pBufOutV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOutU[iW] = pBufInC[iW << 1];
      pBufOutV[iW] = pBufInC[(iW << 1) + 1];
    }

    pBufInC += iPitchSrc;
    pBufOutU += iPitchDstU;
    pBufOutV += iPitchDstV;
  }
}

/****************************************************************************/
void NV12_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV12_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void NV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_I4XX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void NV16_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_I4XX(pSrc, pDst, 2, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void NV24_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_I4XX(pSrc, pDst, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void NV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_I4XX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
static void NVXX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y800_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

  auto pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  auto pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  auto pBufOutV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOutU[iW] = ((uint16_t)pBufIn[iW << 1]) << 2;
      pBufOutV[iW] = ((uint16_t)pBufIn[(iW << 1) + 1]) << 2;
    }

    pBufIn += iPitchSrc;
    pBufOutU += iPitchDstU;
    pBufOutV += iPitchDstV;
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
  default: throw std::runtime_error("Unsupported chroma scale");
  }
}

/****************************************************************************/
void NV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void NV24_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_IXAL(pSrc, pDst, 1, 1);
}

/****************************************************************************/
static void NVXX_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));

  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  auto pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  auto pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidthC = 2 * (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += iPitchSrc;
    pBufOut += iPitchDst;
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P410));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
  default: throw std::runtime_error("Unsupported chroma scale");
  }
}

/****************************************************************************/
void NV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void NV24_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NVXX_To_PX10(pSrc, pDst, 1, 1);
}

/****************************************************************************/
static void NV1X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if((iPitchSrc % 4) != 0)
    throw std::runtime_error("iPitchSrc (" + std::to_string(iPitchSrc) + ") should be aligned to 4");

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iPitchDst);
    uint8_t* pSrcC = (uint8_t*)(pSrcData + h * iPitchSrc);

    int w = iWidthC / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
      *pDst32 |= ((uint32_t)*pSrcC++) << 22;
      ++pDst32;
    }

    if(iWidthC % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
    }
    else if(iWidthC % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void NV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_XVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
// Copy buffer without any planar conversion (semi-planar to semi_planar,
// planar to planar)
// Manage correctly the different chroma mode
bool CopyPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pSrc);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess || tPicFormat.b10bPacked || tPicFormat.bCompressed)
    return false;

  AL_PixMapBuffer_SetFourCC(pDst, tFourCC);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, tPicFormat.uBitDepth);

  // Chroma
  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return true;

  int iChromaWidth = tDim.iWidth;
  int iChromaHeight = tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? (tDim.iHeight + 1) / 2 : tDim.iHeight;

  if(tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
  {
    if(tPicFormat.eChromaMode == AL_CHROMA_4_4_4)
      iChromaWidth *= 2;
    else
      iChromaWidth = ((iChromaWidth + 1) / 2) * 2;

    CopyPixMapPlane(pSrc, pDst, AL_PLANE_UV, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
    return true;
  }

  if(tPicFormat.eChromaMode != AL_CHROMA_4_4_4)
    iChromaWidth = (iChromaWidth + 1) / 2;

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
  return true;
}

struct sFourCCToConvFunc
{
  TFourCC tOutFourCC;
  tConvFourCCFunc tFunc;
};

static const sFourCCToConvFunc ConversionI0ALFuncArray[] =
{
  {
    FOURCC(I420), I0AL_To_I420
  },
  {
    FOURCC(IYUV), I0AL_To_IYUV
  },
  {
    FOURCC(NV12), I0AL_To_NV12
  },
  {
    FOURCC(P010), I0AL_To_P010
  },
  {
    FOURCC(Y010), I0AL_To_Y010
  },
  {
    FOURCC(Y800), I0AL_To_Y800
  },
  {
    FOURCC(T608), I0AL_To_T608
  },
  {
    FOURCC(T60A), I0AL_To_T60A
  },
  {
    FOURCC(T6mA), I0AL_To_T6mA
  },
  {
    FOURCC(T60C), I0AL_To_T60C
  },
  {
    FOURCC(XV15), I0AL_To_XV15
  },
};

static const sFourCCToConvFunc ConversionI0CLFuncArray[] =
{
  {
    FOURCC(P012), I0CL_To_P012
  },
  {
    FOURCC(Y012), I0CL_To_Y012
  },
  {
    FOURCC(T608), I0CL_To_T608
  },
  {
    FOURCC(T60C), I0CL_To_T60C
  },
  {
    FOURCC(T6mC), I0CL_To_T6mC
  },
};

static const sFourCCToConvFunc ConversionI2ALFuncArray[] =
{
  {
    FOURCC(NV16), I2AL_To_NV16
  },
  {
    FOURCC(P210), I2AL_To_P210
  },
  {
    FOURCC(T628), I2AL_To_T628
  },
  {
    FOURCC(T62A), I2AL_To_T62A
  },
  {
    FOURCC(T62C), I2AL_To_T62C
  },
  {
    FOURCC(XV20), I2AL_To_XV20
  },
};

static const sFourCCToConvFunc ConversionI2CLFuncArray[] =
{
  {
    FOURCC(P212), I2CL_To_P212
  },
  {
    FOURCC(T628), I2CL_To_T628
  },
  {
    FOURCC(T62C), I2CL_To_T62C
  },
};

static const sFourCCToConvFunc ConversionI420FuncArray[] =
{
  {
    FOURCC(I0AL), I420_To_I0AL
  },
  {
    FOURCC(IYUV), I420_To_IYUV
  },
  {
    FOURCC(NV12), I420_To_NV12
  },
  {
    FOURCC(P010), I420_To_P010
  },
  {
    FOURCC(P010), I420_To_P012
  },
  {
    FOURCC(Y010), I420_To_Y010
  },
  {
    FOURCC(Y800), I420_To_Y800
  },
  {
    FOURCC(YV12), I420_To_YV12
  },
  {
    FOURCC(T608), I420_To_T608
  },
  {
    FOURCC(T60A), I420_To_T60A
  },
  {
    FOURCC(T60C), I420_To_T60C
  },
  {
    FOURCC(T6m8), I420_To_T6m8
  },
  {
    FOURCC(XV15), I420_To_XV15
  },
};

static const sFourCCToConvFunc ConversionI422FuncArray[] =
{
  {
    FOURCC(NV16), I422_To_NV16
  },
  {
    FOURCC(P210), I422_To_P210
  },
  {
    FOURCC(P212), I422_To_P212
  },
  {
    FOURCC(T628), I422_To_T628
  },
  {
    FOURCC(T62A), I422_To_T62A
  },
  {
    FOURCC(T62C), I422_To_T62C
  },
  {
    FOURCC(XV20), I422_To_XV20
  },
};

static const sFourCCToConvFunc ConversionI444FuncArray[] =
{
  {
    FOURCC(I4AL), I444_To_I4AL
  },
  {
    FOURCC(NV24), I444_To_NV24
  },
  {
    FOURCC(P410), I444_To_P410
  },
  {
    FOURCC(P412), I444_To_P412
  },
  {
    FOURCC(T648), I444_To_T648
  },
  {
    FOURCC(T64A), I444_To_T64A
  },
  {
    FOURCC(T64C), I444_To_T64C
  },
};

static const sFourCCToConvFunc ConversionI4ALFuncArray[] =
{
  {
    FOURCC(I444), I4AL_To_I444
  },
  {
    FOURCC(NV24), I4AL_To_NV24
  },
  {
    FOURCC(P410), I4AL_To_P410
  },
  {
    FOURCC(T648), I4AL_To_T648
  },
  {
    FOURCC(T64A), I4AL_To_T64A
  },
  {
    FOURCC(T64C), I4AL_To_T64C
  },
};

static const sFourCCToConvFunc ConversionI4CLFuncArray[] =
{
  {
    FOURCC(I444), I4CL_To_I444
  },
  {
    FOURCC(I4AL), I4CL_To_I4AL
  },
  {
    FOURCC(P412), I4CL_To_P412
  },
  {
    FOURCC(T64C), I4CL_To_T64C
  },
};

static const sFourCCToConvFunc ConversionIYUVFuncArray[] =
{
  {
    FOURCC(I0AL), IYUV_To_I0AL
  },
  {
    FOURCC(NV12), IYUV_To_NV12
  },
  {
    FOURCC(P010), IYUV_To_P010
  },
  {
    FOURCC(Y800), IYUV_To_Y800
  },
  {
    FOURCC(YV12), IYUV_To_YV12
  },
  {
    FOURCC(T608), IYUV_To_T608
  },
  {
    FOURCC(T60A), IYUV_To_T60A
  },
  {
    FOURCC(XV15), IYUV_To_XV15
  },
};

static const sFourCCToConvFunc ConversionNV12FuncArray[] =
{
  {
    FOURCC(I0AL), NV12_To_I0AL
  },
  {
    FOURCC(I420), NV12_To_I420
  },
  {
    FOURCC(IYUV), NV12_To_IYUV
  },
  {
    FOURCC(P010), NV12_To_P010
  },
  {
    FOURCC(Y800), NV12_To_Y800
  },
  {
    FOURCC(YV12), NV12_To_YV12
  },
  {
    FOURCC(T608), NV12_To_T608
  },
  {
    FOURCC(T60A), NV12_To_T60A
  },
  {
    FOURCC(XV15), NV12_To_XV15
  },
};

static const sFourCCToConvFunc ConversionNV16FuncArray[] =
{
  {
    FOURCC(I2AL), NV16_To_I2AL
  },
  {
    FOURCC(I422), NV16_To_I422
  },
  {
    FOURCC(P210), NV16_To_P210
  },
  {
    FOURCC(T628), NV16_To_T628
  },
  {
    FOURCC(T62A), NV16_To_T62A
  },
  {
    FOURCC(XV20), NV16_To_XV20
  },
};

static const sFourCCToConvFunc ConversionNV24FuncArray[] =
{
  {
    FOURCC(I444), NV24_To_I444
  },
  {
    FOURCC(I4AL), NV24_To_I4AL
  },
  {
    FOURCC(P410), NV24_To_P410
  },
};

static const sFourCCToConvFunc ConversionP010FuncArray[] =
{
  {
    FOURCC(I0AL), P010_To_I0AL
  },
  {
    FOURCC(I420), P010_To_I420
  },
  {
    FOURCC(IYUV), P010_To_IYUV
  },
  {
    FOURCC(NV12), P010_To_NV12
  },
  {
    FOURCC(Y010), P010_To_Y010
  },
  {
    FOURCC(Y800), P010_To_Y800
  },
  {
    FOURCC(YV12), P010_To_YV12
  },
  {
    FOURCC(T608), P010_To_T608
  },
  {
    FOURCC(T60A), P010_To_T60A
  },
  {
    FOURCC(XV15), P010_To_XV15
  },
};

static const sFourCCToConvFunc ConversionP012FuncArray[] =
{
  {
    FOURCC(I0AL), P012_To_I0AL
  },
  {
    FOURCC(I0CL), P012_To_I0CL
  },
  {
    FOURCC(I420), P012_To_I420
  },
  {
    FOURCC(Y012), P012_To_Y012
  },
};

static const sFourCCToConvFunc ConversionP210FuncArray[] =
{
  {
    FOURCC(I2AL), P210_To_I2AL
  },
  {
    FOURCC(I422), P210_To_I422
  },
  {
    FOURCC(T628), P210_To_T628
  },
  {
    FOURCC(T62A), P210_To_T62A
  },
  {
    FOURCC(XV20), P210_To_XV20
  },
};

static const sFourCCToConvFunc ConversionP212FuncArray[] =
{
  {
    FOURCC(I2AL), P212_To_I2AL
  },
  {
    FOURCC(I2CL), P212_To_I2CL
  },
  {
    FOURCC(I422), P212_To_I422
  },
  {
    FOURCC(T62C), P212_To_T62C
  },
};

static const sFourCCToConvFunc ConversionT608FuncArray[] =
{
  {
    FOURCC(I0AL), T608_To_I0AL
  },
  {
    FOURCC(I0CL), T608_To_I0CL
  },
  {
    FOURCC(I420), T608_To_I420
  },
  {
    FOURCC(IYUV), T608_To_IYUV
  },
  {
    FOURCC(NV12), T608_To_NV12
  },
  {
    FOURCC(P010), T608_To_P010
  },
  {
    FOURCC(Y010), T608_To_Y010
  },
  {
    FOURCC(Y800), T608_To_Y800
  },
  {
    FOURCC(YV12), T608_To_YV12
  },
};

static const sFourCCToConvFunc ConversionT60AFuncArray[] =
{
  {
    FOURCC(I0AL), T60A_To_I0AL
  },
  {
    FOURCC(I420), T60A_To_I420
  },
  {
    FOURCC(IYUV), T60A_To_IYUV
  },
  {
    FOURCC(NV12), T60A_To_NV12
  },
  {
    FOURCC(P010), T60A_To_P010
  },
  {
    FOURCC(Y010), T60A_To_Y010
  },
  {
    FOURCC(Y800), T60A_To_Y800
  },
  {
    FOURCC(YV12), T60A_To_YV12
  },
  {
    FOURCC(XV10), T60A_To_XV10
  },
  {
    FOURCC(XV15), T60A_To_XV15
  },
};

static const sFourCCToConvFunc ConversionT60CFuncArray[] =
{
  {
    FOURCC(I0AL), T60C_To_I0AL
  },
  {
    FOURCC(I0CL), T60C_To_I0CL
  },
  {
    FOURCC(I420), T60C_To_I420
  },
  {
    FOURCC(Y010), T60C_To_Y010
  },
  {
    FOURCC(Y012), T60C_To_Y012
  },
  {
    FOURCC(Y800), T60C_To_Y800
  },
};

static const sFourCCToConvFunc ConversionT628FuncArray[] =
{
  {
    FOURCC(I2AL), T628_To_I2AL
  },
  {
    FOURCC(I2CL), T628_To_I2CL
  },
  {
    FOURCC(I422), T628_To_I422
  },
  {
    FOURCC(NV16), T628_To_NV16
  },
  {
    FOURCC(P210), T628_To_P210
  },
  {
    FOURCC(Y010), T628_To_Y010
  },
  {
    FOURCC(Y800), T628_To_Y800
  },
};

static const sFourCCToConvFunc ConversionT62AFuncArray[] =
{
  {
    FOURCC(I2AL), T62A_To_I2AL
  },
  {
    FOURCC(I422), T62A_To_I422
  },
  {
    FOURCC(NV16), T62A_To_NV16
  },
  {
    FOURCC(P210), T62A_To_P210
  },
  {
    FOURCC(Y010), T62A_To_Y010
  },
  {
    FOURCC(Y800), T62A_To_Y800
  },
  {
    FOURCC(XV20), T62A_To_XV20
  },
};

static const sFourCCToConvFunc ConversionT62CFuncArray[] =
{
  {
    FOURCC(I2AL), T62C_To_I2AL
  },
  {
    FOURCC(I2CL), T62C_To_I2CL
  },
  {
    FOURCC(I422), T62C_To_I422
  },
  {
    FOURCC(P212), T62C_To_P212
  },
  {
    FOURCC(Y012), T62C_To_Y012
  },
  {
    FOURCC(Y800), T62C_To_Y800
  },
};

static const sFourCCToConvFunc ConversionT648FuncArray[] =
{
  {
    FOURCC(I444), T648_To_I444
  },
  {
    FOURCC(I4AL), T648_To_I4AL
  },
};

static const sFourCCToConvFunc ConversionT64AFuncArray[] =
{
  {
    FOURCC(I444), T64A_To_I444
  },
  {
    FOURCC(I4AL), T64A_To_I4AL
  },
};

static const sFourCCToConvFunc ConversionT64CFuncArray[] =
{
  {
    FOURCC(I444), T64C_To_I444
  },
  {
    FOURCC(I4AL), T64C_To_I4AL
  },
  {
    FOURCC(I4CL), T64C_To_I4CL
  },
};

static const sFourCCToConvFunc ConversionY010FuncArray[] =
{
  {
    FOURCC(T608), Y010_To_T608
  },
  {
    FOURCC(T60A), Y010_To_T60A
  },
  {
    FOURCC(T60C), Y010_To_T60C
  },
  {
    FOURCC(T628), Y010_To_T628
  },
  {
    FOURCC(T62A), Y010_To_T62A
  },
  {
    FOURCC(XV10), Y010_To_XV10
  },
  {
    FOURCC(XV15), Y010_To_XV15
  },
};

static const sFourCCToConvFunc ConversionY012FuncArray[] =
{
  {
    FOURCC(Y010), Y012_To_Y010
  },
  {
    FOURCC(Y800), Y012_To_Y800
  },
  {
    FOURCC(T608), Y012_To_T608
  },
  {
    FOURCC(T60C), Y012_To_T60C
  },
  {
    FOURCC(T62C), Y012_To_T62C
  },
};

static const sFourCCToConvFunc ConversionY800FuncArray[] =
{
  {
    FOURCC(I0AL), Y800_To_I0AL
  },
  {
    FOURCC(I420), Y800_To_I420
  },
  {
    FOURCC(IYUV), Y800_To_IYUV
  },
  {
    FOURCC(NV12), Y800_To_NV12
  },
  {
    FOURCC(P010), Y800_To_P010
  },
  {
    FOURCC(Y010), Y800_To_Y010
  },
  {
    FOURCC(YV12), Y800_To_YV12
  },
  {
    FOURCC(T608), Y800_To_T608
  },
  {
    FOURCC(T60A), Y800_To_T60A
  },
  {
    FOURCC(T60C), Y800_To_T60C
  },
  {
    FOURCC(T628), Y800_To_T628
  },
  {
    FOURCC(T62A), Y800_To_T62A
  },
  {
    FOURCC(T62C), Y800_To_T62C
  },
  {
    FOURCC(XV10), Y800_To_XV10
  },
  {
    FOURCC(XV15), Y800_To_XV15
  },
};

static const sFourCCToConvFunc ConversionYV12FuncArray[] =
{
  {
    FOURCC(I0AL), YV12_To_I0AL
  },
  {
    FOURCC(I420), YV12_To_I420
  },
  {
    FOURCC(IYUV), YV12_To_IYUV
  },
  {
    FOURCC(NV12), YV12_To_NV12
  },
  {
    FOURCC(P010), YV12_To_P010
  },
  {
    FOURCC(Y800), YV12_To_Y800
  },
  {
    FOURCC(T608), YV12_To_T608
  },
  {
    FOURCC(T60A), YV12_To_T60A
  },
  {
    FOURCC(XV15), YV12_To_XV15
  },
};

static const sFourCCToConvFunc ConversionT6m8FuncArray[] =
{
  {
    FOURCC(Y800), T608_To_Y800
  },
  {
    FOURCC(Y010), T608_To_Y010
  },
  {
    FOURCC(Y012), T608_To_Y012
  },
  {
    FOURCC(I420), T6m8_To_I420
  },
};

static const sFourCCToConvFunc ConversionT6mAFuncArray[] =
{
  {
    FOURCC(Y800), T60A_To_Y800
  },
  {
    FOURCC(Y010), T60A_To_Y010
  },
  {
    FOURCC(XV10), T60A_To_XV10
  },
};

static const sFourCCToConvFunc ConversionT6mCFuncArray[] =
{
  {
    FOURCC(Y012), T60C_To_Y012
  },
};

static const sFourCCToConvFunc ConversionXV10FuncArray[] =
{
  {
    FOURCC(Y010), XV10_To_Y010
  },
  {
    FOURCC(Y800), XV10_To_Y800
  },
};

static const sFourCCToConvFunc ConversionXV15FuncArray[] =
{
  {
    FOURCC(I0AL), XV15_To_I0AL
  },
  {
    FOURCC(I420), XV15_To_I420
  },
  {
    FOURCC(IYUV), XV15_To_IYUV
  },
  {
    FOURCC(NV12), XV15_To_NV12
  },
  {
    FOURCC(P010), XV15_To_P010
  },
  {
    FOURCC(Y010), XV15_To_Y010
  },
  {
    FOURCC(Y800), XV15_To_Y800
  },
  {
    FOURCC(YV12), XV15_To_YV12
  },
};

static const sFourCCToConvFunc ConversionXV20FuncArray[] =
{
  {
    FOURCC(I2AL), XV20_To_I2AL
  },
  {
    FOURCC(I422), XV20_To_I422
  },
  {
    FOURCC(NV16), XV20_To_NV16
  },
  {
    FOURCC(P210), XV20_To_P210
  },
  {
    FOURCC(Y010), XV20_To_Y010
  },
  {
    FOURCC(Y800), XV20_To_Y800
  },
};

#define CONV_FOURCC_ARRAY(array) array, ARRAY_SIZE(array)

struct sConvFourCCArray
{
  TFourCC tInFourCC;
  const sFourCCToConvFunc* array;
  unsigned int array_size;
};

static const sConvFourCCArray ConvMatchArray[] =
{
  { FOURCC(I0AL), CONV_FOURCC_ARRAY(ConversionI0ALFuncArray) },
  { FOURCC(I0CL), CONV_FOURCC_ARRAY(ConversionI0CLFuncArray) },
  { FOURCC(I2AL), CONV_FOURCC_ARRAY(ConversionI2ALFuncArray) },
  { FOURCC(I2CL), CONV_FOURCC_ARRAY(ConversionI2CLFuncArray) },
  { FOURCC(I420), CONV_FOURCC_ARRAY(ConversionI420FuncArray) },
  { FOURCC(I422), CONV_FOURCC_ARRAY(ConversionI422FuncArray) },
  { FOURCC(I444), CONV_FOURCC_ARRAY(ConversionI444FuncArray) },
  { FOURCC(I4AL), CONV_FOURCC_ARRAY(ConversionI4ALFuncArray) },
  { FOURCC(I4CL), CONV_FOURCC_ARRAY(ConversionI4CLFuncArray) },
  { FOURCC(IYUV), CONV_FOURCC_ARRAY(ConversionIYUVFuncArray) },
  { FOURCC(NV12), CONV_FOURCC_ARRAY(ConversionNV12FuncArray) },
  { FOURCC(NV16), CONV_FOURCC_ARRAY(ConversionNV16FuncArray) },
  { FOURCC(NV24), CONV_FOURCC_ARRAY(ConversionNV24FuncArray) },
  { FOURCC(P010), CONV_FOURCC_ARRAY(ConversionP010FuncArray) },
  { FOURCC(P012), CONV_FOURCC_ARRAY(ConversionP012FuncArray) },
  { FOURCC(P210), CONV_FOURCC_ARRAY(ConversionP210FuncArray) },
  { FOURCC(P212), CONV_FOURCC_ARRAY(ConversionP212FuncArray) },
  { FOURCC(T608), CONV_FOURCC_ARRAY(ConversionT608FuncArray) },
  { FOURCC(T60A), CONV_FOURCC_ARRAY(ConversionT60AFuncArray) },
  { FOURCC(T60C), CONV_FOURCC_ARRAY(ConversionT60CFuncArray) },
  { FOURCC(T628), CONV_FOURCC_ARRAY(ConversionT628FuncArray) },
  { FOURCC(T62A), CONV_FOURCC_ARRAY(ConversionT62AFuncArray) },
  { FOURCC(T62C), CONV_FOURCC_ARRAY(ConversionT62CFuncArray) },
  { FOURCC(T648), CONV_FOURCC_ARRAY(ConversionT648FuncArray) },
  { FOURCC(T64A), CONV_FOURCC_ARRAY(ConversionT64AFuncArray) },
  { FOURCC(T64C), CONV_FOURCC_ARRAY(ConversionT64CFuncArray) },
  { FOURCC(Y012), CONV_FOURCC_ARRAY(ConversionY012FuncArray) },
  { FOURCC(Y800), CONV_FOURCC_ARRAY(ConversionY800FuncArray) },
  { FOURCC(YV12), CONV_FOURCC_ARRAY(ConversionYV12FuncArray) },
  { FOURCC(T6m8), CONV_FOURCC_ARRAY(ConversionT6m8FuncArray) },
  { FOURCC(T6mA), CONV_FOURCC_ARRAY(ConversionT6mAFuncArray) },
  { FOURCC(T6mC), CONV_FOURCC_ARRAY(ConversionT6mCFuncArray) },
  { FOURCC(Y010), CONV_FOURCC_ARRAY(ConversionY010FuncArray) },
  { FOURCC(XV10), CONV_FOURCC_ARRAY(ConversionXV10FuncArray) },
  { FOURCC(XV15), CONV_FOURCC_ARRAY(ConversionXV15FuncArray) },
  { FOURCC(XV20), CONV_FOURCC_ARRAY(ConversionXV20FuncArray) },
};

typedef union
{
  uint32_t uFourCC;
  struct
  {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
  }Value;
}tUnionFourCC;

static tConvFourCCFunc GetConversionOutFunction(sConvFourCCArray const* pMatchArray, TFourCC tOutFourCC)
{
  sFourCCToConvFunc const* pFourCCConv;

  for(unsigned int i = 0; i < pMatchArray->array_size; i++)
  {
    pFourCCConv = &pMatchArray->array[i];

    if(pFourCCConv->tOutFourCC == tOutFourCC)
      return pFourCCConv->tFunc;
  }

  return nullptr;
}

tConvFourCCFunc GetConvFourCCFunc(TFourCC tInFourCC, TFourCC tOutFourCC)
{
  sConvFourCCArray const* pMatchArray;

  if(tInFourCC == tOutFourCC)
  {
    AL_TPicFormat tPicFormat;
    bool bSuccess = AL_GetPicFormat(tInFourCC, &tPicFormat);

    if(!bSuccess || tPicFormat.b10bPacked || tPicFormat.bCompressed)
      return nullptr;

    if(AL_IsTiled(tInFourCC))
      return nullptr;

    return CopyPixMapBuffer;
  }

  auto const AdjustTileFourCC = [](TFourCC& tFourCC)
                                {
                                  tUnionFourCC sF;
                                  sF.uFourCC = tFourCC;

                                  /* T5XX FourCC are handled by T6XX function */
                                  if(sF.Value.a == 'T' && sF.Value.b == '5')
                                  {
                                    sF.Value.b = '6';
                                    tFourCC = sF.uFourCC;
                                  }
                                };

  AdjustTileFourCC(tInFourCC);
  AdjustTileFourCC(tOutFourCC);

  for(unsigned int i = 0; i < ARRAY_SIZE(ConvMatchArray); i++)
  {
    pMatchArray = &ConvMatchArray[i];

    if(pMatchArray->tInFourCC == tInFourCC)
      return GetConversionOutFunction(pMatchArray, tOutFourCC);
  }

  return nullptr;
}

int ConvertPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  TFourCC tInFourCC = AL_PixMapBuffer_GetFourCC(pSrc);
  TFourCC tOutFourCC = AL_PixMapBuffer_GetFourCC(pDst);
  tConvFourCCFunc tFunc;

  tFunc = GetConvFourCCFunc(tInFourCC, tOutFourCC);

  if(!tFunc)
    return 1;

  tFunc(pSrc, pDst);

  return 0;
}

/*@}*/

