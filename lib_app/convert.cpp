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

#include <cassert>

extern "C" {
#include "lib_rtos/lib_rtos.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Utils.h"
}

#include "convert.h"

static inline uint8_t RND_10B_TO_8B(uint16_t val)
{
  return (uint8_t)(((val) >= 0x3FC) ? 0xFF : (((val) + 2) >> 2));
}

static inline uint8_t RND_12B_TO_8B(uint16_t val)
{
  return (uint8_t)(((val) >= 0xFF0) ? 0xFF : (((val) + 8) >> 4));
}

static inline uint16_t RND_12B_TO_10B(uint16_t val)
{
  return (uint16_t)(((val) >= 0xFFC) ? 0x3FF : (((val) + 2) >> 2));
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
    Rtos_Memcpy(pDstData, pSrcData, iByteWidth);
    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
void CopyPixMapPlane_8To10b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight)
{
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneType);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType) / sizeof(uint16_t);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneType);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  for(int iH = 0; iH < iHeight; iH++)
  {
    for(int iW = 0; iW < iWidth; iW++)
      *pDstData++ = ((uint16_t)*pSrcData++) << 2;

    pSrcData += iPitchSrc - iWidth;
    pDstData += iPitchDst - iWidth;
  }
}

/****************************************************************************/
void CopyPixMapPlane_1XTo8b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight, uint8_t (* RND_FUNC)(uint16_t))
{
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneType) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType);
  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneType);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  for(int iH = 0; iH < iHeight; iH++)
  {
    for(int iW = 0; iW < iWidth; iW++)
      pDstData[iW] = (*RND_FUNC)(pSrcData[iW]);

    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
void CopyPixMapPlane_12To10b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight)
{
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneType) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType) / sizeof(uint16_t);
  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneType);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

  for(int iH = 0; iH < iHeight; iH++)
  {
    for(int iW = 0; iW < iWidth; iW++)
      pDstData[iW] = RND_12B_TO_10B(pSrcData[iW]);

    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
template<typename T>
void SetPixMapPlane(AL_TBuffer* pDst, AL_EPlaneId ePlaneType, int iWidth, int iHeight, T uVal)
{
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneType) / sizeof(T);
  T* pDstData = (T*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneType);

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

  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);
}

/****************************************************************************/
static void I4XX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));

  // Luma
  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);

  // Chroma
  int iWidthChroma = (tDim.iWidth + iHScale - 1) / iHScale;
  int iHeightChroma = (tDim.iHeight + iVScale - 1) / iVScale;
  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_U, iWidthChroma, iHeightChroma);
  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_V, iWidthChroma, iHeightChroma);
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

  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);

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

  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);

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

  CopyPixMapPlane_8To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);
}

/****************************************************************************/
static uint32_t toTen(uint8_t sample)
{
  return (((uint32_t)sample) << 2) & 0x3FF;
}

/****************************************************************************/
void Y800_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));

  int iPitchSrcY = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDstY = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  assert(iPitchDstY % 4 == 0);
  assert(iPitchDstY >= (tDim.iWidth + 2) / 3 * 4);

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
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDstBuf);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, AL_PLANE_UV);

  assert(iPitchDst % 4 == 0);

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

  assert(iPitchDst % 4 == 0);
  assert(iPitchDst >= (iDstWidth + 2) / 3 * 4);

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
static void SemiPlanarToPlanar_1XTo8b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIs10b)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  uint8_t (* RND_FUNC)(uint16_t) = bIs10b ? &RND_10B_TO_8B : &RND_12B_TO_8B;

  // Luma
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, RND_FUNC);

  // Chroma
  {
    uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
    uint32_t uPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
    uint32_t uPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);

    int iWidth = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
    int iHeight = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

    uint16_t* pBufInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    uint8_t* pBufOutU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
    uint8_t* pBufOutV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = RND_FUNC(pBufInC[(iW << 1)]);
        pBufOutV[iW] = RND_FUNC(pBufInC[(iW << 1) + 1]);
      }

      pBufInC += uPitchSrc;
      pBufOutU += uPitchDstU;
      pBufOutV += uPitchDstV;
    }
  }
}

/****************************************************************************/
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 2, true);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void P210_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 1, true);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void P210_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 1, 1, true);
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
// Semi-planar to planar conversion
// No bitDepth conversion
// Support all chroma modes
static void PX1X_To_IXXL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool is12Bits)
{
  // Luma
  P010_To_Y010(pSrc, pDst);

  // Chroma
  uint32_t uSrcPitchUV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uDstPitchU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  uint32_t uDstPitchV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint16_t* pBufOutV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  int iWidth = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeight = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = pBufIn[(iW << 1)];
      pBufOutV[iW] = pBufIn[(iW << 1) + 1];
    }

    pBufIn += uSrcPitchUV;
    pBufOutU += uDstPitchU;
    pBufOutV += uDstPitchV;
  }

  TFourCC fourcc;
  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: fourcc = is12Bits ? FOURCC(I4CL) : FOURCC(I4AL);
    break;
  case 2: fourcc = is12Bits ? FOURCC(I2CL) : FOURCC(I2AL);
    break;
  case 4: fourcc = is12Bits ? FOURCC(I0CL) : FOURCC(I0AL);
    break;
  default: assert(0 && "Unsupported chroma scale");
  }

  return (void)AL_PixMapBuffer_SetFourCC(pDst, fourcc);
}

/****************************************************************************/
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX1X_To_IXXL(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void P210_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX1X_To_IXXL(pSrc, pDst, 2, 1, false);
}

/****************************************************************************/
void P012_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX1X_To_IXXL(pSrc, pDst, 2, 2, true);
}

/****************************************************************************/
void P212_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX1X_To_IXXL(pSrc, pDst, 2, 1, true);
}

/****************************************************************************/
void P012_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 2, false);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void P212_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanarToPlanar_1XTo8b(pSrc, pDst, 2, 1, false);
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
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, &RND_10B_TO_8B);

  int iChromaWidth = ((tDim.iWidth + 1) >> 1) << 1;
  int iChromaHeight = (tDim.iHeight + 1) >> 1;
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_UV, iChromaWidth, iChromaHeight, &RND_10B_TO_8B);

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
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, &RND_10B_TO_8B);
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, &RND_10B_TO_8B);
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
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, &RND_10B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void I0CL_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  CopyPixMapPlane_12To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
static void I0XL_To_Y01X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBdIn, uint8_t uBdOut)
{
  assert(uBdIn == uBdOut); // No bitdepth conversion supported for now
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, uBdOut);

  auto fcc = uBdOut == 10 ? FOURCC(Y010) : FOURCC(Y012);
  AL_PixMapBuffer_SetFourCC(pDst, fcc);
}

/****************************************************************************/
void P010_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, 10, 10);
}

/****************************************************************************/
void P012_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, 12, 12);
}

/****************************************************************************/
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0XL_To_Y01X(pSrc, pDst, 10, 10);
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

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV24));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
  default: assert(0 && "Unsupported chroma scale");
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
}

/****************************************************************************/
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void I444_To_NV24(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I4XX_To_NVXX(pSrc, pDst, 1, 1);
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

  uint8_t* pBufInU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint8_t* pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

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
  default: assert(0 && "Unsupported chroma scale");
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
void Y012_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  CopyPixMapPlane_12To10b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void Y012_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, &RND_12B_TO_8B);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
// Convert 12 bits semi-planar to 10 bits planar
static void PX12_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  Y012_To_Y010(pSrc, pDst);

  uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  uint32_t uPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;

  uint16_t* pBufInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint16_t* pBufOutV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOutU[iW] = RND_12B_TO_10B(pBufInC[(iW << 1)]);
      pBufOutV[iW] = RND_12B_TO_10B(pBufInC[(iW << 1) + 1]);
    }

    pBufInC += uPitchSrc;
    pBufOutU += uPitchDstU;
    pBufOutV += uPitchDstV;
  }

  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
  case 2: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
  case 4: return (void)AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
  default: assert(0 && "Unsupported chroma scale");
  }
}

/****************************************************************************/
void P012_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX12_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void P212_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX12_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void IXCL_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0CL_To_Y012(pSrc, pDst);

  // Chroma
  const int iChromaWidth = (tDim.iWidth + iHScale - 1) / iHScale;
  const int iChromaHeight = (tDim.iHeight + iVScale - 1) / iVScale;
  CopyPixMapPlane_12To10b(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight);
  CopyPixMapPlane_12To10b(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight);
}

/****************************************************************************/
void I4CL_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXCL_To_IXAL(pSrc, pDst, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
static void IXCL_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  Y012_To_Y800(pSrc, pDst);

  // Chroma
  int iChromaWidth = (tDim.iWidth + iHScale - 1) / iHScale;
  int iChromaHeight = (tDim.iHeight + iVScale - 1) / iVScale;
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, &RND_12B_TO_8B);
  CopyPixMapPlane_1XTo8b(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, &RND_12B_TO_8B);
}

/****************************************************************************/
void I4CL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXCL_To_I4XX(pSrc, pDst, 1, 1);
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

  assert(iDstPitch % 4 == 0);
  assert(iDstPitch >= (tDim.iWidth + 2) / 3 * 4);

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

  uint16_t* pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint16_t* pBufInV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

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
  default: assert(0 && "Unsupported chroma scale");
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
static void IXYL_To_PX1Y(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, uint8_t uBd)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0XL_To_Y01X(pSrc, pDst, uBd, uBd);

  // Chroma
  int iHeightC = (tDim.iHeight + uVrtCScale - 1) / uVrtCScale;
  int iWidthC = (tDim.iWidth + uHrzCScale - 1) / uHrzCScale;
  uint32_t uPitchSrcU = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_U) / sizeof(uint16_t);
  uint32_t uPitchSrcV = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_V) / sizeof(uint16_t);
  uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_U);
  uint16_t* pBufInV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_V);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[2 * iW] = pBufInU[iW];
      pBufOut[2 * iW + 1] = pBufInV[iW];
    }

    pBufInU += uPitchSrcU;
    pBufInV += uPitchSrcV;
    pBufOut += uPitchDst;
  }

  uint32_t fcc = 0;
  int iCScale = uHrzCScale * uVrtCScale;
  switch(iCScale)
  {
  case 1: fcc = uBd == 10 ? FOURCC(P410) : FOURCC(P412);
    break;
  case 2: fcc = uBd == 10 ? FOURCC(P210) : FOURCC(P212);
    break;
  case 4: fcc = uBd == 10 ? FOURCC(P010) : FOURCC(P012);
    break;
  default: assert(0 && "Unsupported chroma scale");
  }

  return (void)AL_PixMapBuffer_SetFourCC(pDst, fcc);
}

/****************************************************************************/
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 2, 10);
}

/****************************************************************************/
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 1, 10);
}

/****************************************************************************/
void I4AL_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 1, 1, 10);
}

/****************************************************************************/
void I0CL_To_P012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 2, 12);
}

/****************************************************************************/
void I2CL_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 2, 1, 12);
}

/****************************************************************************/
void I4CL_To_P412(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXYL_To_PX1Y(pSrc, pDst, 1, 1, 12);
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

  assert(uDstPitchChroma % 4 == 0);
  assert(uDstPitchChroma >= (uint32_t)(tDim.iWidth + 2) / 3 * 4);

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

          if(H + h + 4 <= iHeightC)
          {
            pOutU += uPitchDstU;
            pOutV += uPitchDstV;

            for(int p = (2 * (8 / uHrzCScale)); p < (2 * (8 / uHrzCScale)) + (8 / uHrzCScale); p++)
              ((p % 2 == 0) ? pOutU[(p % (8 / uHrzCScale)) / 2] : pOutV[(p % (8 / uHrzCScale)) / 2]) = pInC[p];

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
void T608_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);

          pOutC[0] = pInC[0];
          pOutC[1] = pInC[1];
          pOutC[2] = pInC[2];
          pOutC[3] = pInC[3];
          pOutC += iPitchDst;
          pOutC[0] = pInC[4];
          pOutC[1] = pInC[5];
          pOutC[2] = pInC[6];
          pOutC[3] = pInC[7];
          pOutC += iPitchDst;
          pOutC[0] = pInC[8];
          pOutC[1] = pInC[9];
          pOutC[2] = pInC[10];
          pOutC[3] = pInC[11];
          pOutC += iPitchDst;
          pOutC[0] = pInC[12];
          pOutC[1] = pInC[13];
          pOutC[2] = pInC[14];
          pOutC[3] = pInC[15];
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T64_Untile_Component(AL_TBuffer const* pSrc, AL_TBuffer* pDst, AL_EPlaneId ePlaneId)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneId);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneId);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneId);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneId);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

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
          uint8_t* pOutY = pDstData + (H + h) * iPitchDst + (W + w);

          pOutY[0] = pInY[0];
          pOutY[1] = pInY[1];
          pOutY[2] = pInY[2];
          pOutY[3] = pInY[3];
          pOutY += iPitchDst;
          pOutY[0] = pInY[4];
          pOutY[1] = pInY[5];
          pOutY[2] = pInY[6];
          pOutY[3] = pInY[7];
          pOutY += iPitchDst;
          pOutY[0] = pInY[8];
          pOutY[1] = pInY[9];
          pOutY[2] = pInY[10];
          pOutY[3] = pInY[11];
          pOutY += iPitchDst;
          pOutY[0] = pInY[12];
          pOutY[1] = pInY[13];
          pOutY[2] = pInY[14];
          pOutY[3] = pInY[15];
          pInY += 16;
        }

        pInY += 4 * iCropW;
      }

      pInY += iCropH * iTileW;
    }
  }
}

/****************************************************************************/
void T608_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component(pSrc, pDst, AL_PLANE_Y);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T608_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

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
          uint16_t* pOutY = ((uint16_t*)pDstData) + (H + h) * iPitchDst + (W + w);

          pOutY[0] = ((uint16_t)pInY[0]) << 2;
          pOutY[1] = ((uint16_t)pInY[1]) << 2;
          pOutY[2] = ((uint16_t)pInY[2]) << 2;
          pOutY[3] = ((uint16_t)pInY[3]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[4]) << 2;
          pOutY[1] = ((uint16_t)pInY[5]) << 2;
          pOutY[2] = ((uint16_t)pInY[6]) << 2;
          pOutY[3] = ((uint16_t)pInY[7]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[8]) << 2;
          pOutY[1] = ((uint16_t)pInY[9]) << 2;
          pOutY[2] = ((uint16_t)pInY[10]) << 2;
          pOutY[3] = ((uint16_t)pInY[11]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[12]) << 2;
          pOutY[1] = ((uint16_t)pInY[13]) << 2;
          pOutY[2] = ((uint16_t)pInY[14]) << 2;
          pOutY[3] = ((uint16_t)pInY[15]) << 2;
          pInY += 16;
        }

        pInY += 4 * iCropW;
      }

      pInY += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T608_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = ((uint16_t*)pDstData) + (H + h) * iPitchDst + (W + w);

          pOutC[0] = ((uint16_t)pInC[0]) << 2;
          pOutC[1] = ((uint16_t)pInC[1]) << 2;
          pOutC[2] = ((uint16_t)pInC[2]) << 2;
          pOutC[3] = ((uint16_t)pInC[3]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[4]) << 2;
          pOutC[1] = ((uint16_t)pInC[5]) << 2;
          pOutC[2] = ((uint16_t)pInC[6]) << 2;
          pOutC[3] = ((uint16_t)pInC[7]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[8]) << 2;
          pOutC[1] = ((uint16_t)pInC[9]) << 2;
          pOutC[2] = ((uint16_t)pInC[10]) << 2;
          pOutC[3] = ((uint16_t)pInC[11]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[12]) << 2;
          pOutC[1] = ((uint16_t)pInC[13]) << 2;
          pOutC[2] = ((uint16_t)pInC[14]) << 2;
          pOutC[3] = ((uint16_t)pInC[15]) << 2;
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void T608_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint16_t* pDstDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  const int iTileW = 64;
  const int iTileH = 4;

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = pDstDataU + (H + h) * iPitchDstU + (W + w) / 2;
          uint16_t* pOutV = pDstDataV + (H + h) * iPitchDstV + (W + w) / 2;

          pOutU[0] = ((uint16_t)pInC[0]) << 2;
          pOutV[0] = ((uint16_t)pInC[1]) << 2;
          pOutU[1] = ((uint16_t)pInC[2]) << 2;
          pOutV[1] = ((uint16_t)pInC[3]) << 2;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[4]) << 2;
          pOutV[0] = ((uint16_t)pInC[5]) << 2;
          pOutU[1] = ((uint16_t)pInC[6]) << 2;
          pOutV[1] = ((uint16_t)pInC[7]) << 2;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[8]) << 2;
          pOutV[0] = ((uint16_t)pInC[9]) << 2;
          pOutU[1] = ((uint16_t)pInC[10]) << 2;
          pOutV[1] = ((uint16_t)pInC[11]) << 2;
          pOutU += iPitchDstU;
          pOutV += iPitchDstV;
          pOutU[0] = ((uint16_t)pInC[12]) << 2;
          pOutV[0] = ((uint16_t)pInC[13]) << 2;
          pOutU[1] = ((uint16_t)pInC[14]) << 2;
          pOutV[1] = ((uint16_t)pInC[15]) << 2;
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

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

  assert(iPitchDst % 4 == 0);

  if(bProcessY)
  {
    assert(iPitchDst >= (tDim.iWidth + 2) / 3 * 4);
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

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pOutC = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (iWidthC * 4);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      pOutC[0] = pInC[0];
      pOutC[1] = pInC[1];
      pOutC[2] = pInC[2];
      pOutC[3] = pInC[3];
      pOutC += iPitchDst;
      pOutC[0] = pInC[4];
      pOutC[1] = pInC[5];
      pOutC[2] = pInC[6];
      pOutC[3] = pInC[7];
      pOutC += iPitchDst;
      pOutC[0] = pInC[8];
      pOutC[1] = pInC[9];
      pOutC[2] = pInC[10];
      pOutC[3] = pInC[11];
      pOutC += iPitchDst;
      pOutC[0] = pInC[12];
      pOutC[1] = pInC[13];
      pOutC[2] = pInC[14];
      pOutC[3] = pInC[15];
      pOutC -= 3 * iPitchDst - 4;
      pInC += 16;
    }

    pOutC += iPitchDst * 4 - iWidthC;
    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void T628_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (iWidthC * 4);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(uint16_t);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(uint16_t);

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint16_t* pDstDataV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      uint16_t* pOutU = pDstDataU + h * iPitchDstU + w / 2;
      uint16_t* pOutV = pDstDataV + h * iPitchDstV + w / 2;

      pOutU[0] = ((uint16_t)pInC[0]) << 2;
      pOutV[0] = ((uint16_t)pInC[1]) << 2;
      pOutU[1] = ((uint16_t)pInC[2]) << 2;
      pOutV[1] = ((uint16_t)pInC[3]) << 2;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[4]) << 2;
      pOutV[0] = ((uint16_t)pInC[5]) << 2;
      pOutU[1] = ((uint16_t)pInC[6]) << 2;
      pOutV[1] = ((uint16_t)pInC[7]) << 2;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[8]) << 2;
      pOutV[0] = ((uint16_t)pInC[9]) << 2;
      pOutU[1] = ((uint16_t)pInC[10]) << 2;
      pOutV[1] = ((uint16_t)pInC[11]) << 2;
      pOutU += iPitchDstU;
      pOutV += iPitchDstV;
      pOutU[0] = ((uint16_t)pInC[12]) << 2;
      pOutV[0] = ((uint16_t)pInC[13]) << 2;
      pOutU[1] = ((uint16_t)pInC[14]) << 2;
      pOutV[1] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T628_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (iWidthC * 4);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      uint16_t* pOutC = pDstData + h * iPitchDst + w;

      pOutC[0] = ((uint16_t)pInC[0]) << 2;
      pOutC[1] = ((uint16_t)pInC[1]) << 2;
      pOutC[2] = ((uint16_t)pInC[2]) << 2;
      pOutC[3] = ((uint16_t)pInC[3]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[4]) << 2;
      pOutC[1] = ((uint16_t)pInC[5]) << 2;
      pOutC[2] = ((uint16_t)pInC[6]) << 2;
      pOutC[3] = ((uint16_t)pInC[7]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[8]) << 2;
      pOutC[1] = ((uint16_t)pInC[9]) << 2;
      pOutC[2] = ((uint16_t)pInC[10]) << 2;
      pOutC[3] = ((uint16_t)pInC[11]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[12]) << 2;
      pOutC[1] = ((uint16_t)pInC[13]) << 2;
      pOutC[2] = ((uint16_t)pInC[14]) << 2;
      pOutC[3] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
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
static void Untile4x4Block10bTo8b(uint16_t* pTiled4x4, uint8_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint8_t>(pTiled4x4, pUntiled, iUntiledPitch, RND_10B_TO_8B);
}

/****************************************************************************/
static void Untile4x4Block1xbTo1xb(uint16_t* pTiled4x4, uint16_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint16_t>(pTiled4x4, pUntiled, iUntiledPitch, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void Untile4x4Block12bTo8b(uint16_t* pTiled4x4, uint8_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint8_t>(pTiled4x4, pUntiled, iUntiledPitch, RND_12B_TO_8B);
}

/****************************************************************************/
static void Untile4x4Block12bTo10b(uint16_t* pTiled4x4, uint16_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint16_t>(pTiled4x4, pUntiled, iUntiledPitch, RND_12B_TO_10B);
}

/****************************************************************************/
static void Untile4x4Block12bTo12b(uint16_t* pTiled4x4, uint16_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block12b<uint16_t>(pTiled4x4, pUntiled, iUntiledPitch, [](uint16_t u16) { return u16; });
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
template<typename DstType>
static void T64_Untile_Component_HighBD(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, AL_EPlaneId ePlaneId)
{
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, ePlaneId);
  DstType* pDstData = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, ePlaneId);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, ePlaneId) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, ePlaneId) / sizeof(DstType);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint16_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

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
          DstType* pOutY = pDstData + (H + h) * iPitchDst + (W + w);

          if(bdIn == 12)
          {
            if(bdOut == 12)
              Untile4x4Block12bTo12b(pInY, (uint16_t*)pOutY, iPitchDst);
            else if(bdOut == 10)
              Untile4x4Block12bTo10b(pInY, (uint16_t*)pOutY, iPitchDst);
            else
              Untile4x4Block12bTo8b(pInY, (uint8_t*)pOutY, iPitchDst);
          }
          else // 10 bit
          {
            bdOut == 10 ?
            Untile4x4Block1xbTo1xb(pInY, (uint16_t*)pOutY, iPitchDst) :
            Untile4x4Block10bTo8b(pInY, (uint8_t*)pOutY, iPitchDst);
          }
          pInY += bdIn;
        }

        pInY += bdIn / 2 * iCropW / sizeof(uint16_t);
      }

      pInY += iCropH * iTileW * bdIn / 2 / 4 / sizeof(uint16_t);
    }
  }
}

/****************************************************************************/
void T60A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint8_t>(pSrc, pDst, 10, 8, AL_PLANE_Y);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T60C_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint8_t>(pSrc, pDst, 12, 8, AL_PLANE_Y);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T60A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T60C_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 12, 10, AL_PLANE_Y);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T60C_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y012));
}

/****************************************************************************/
template<typename DstType>
static void T6XX_To_4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t bdIn, uint8_t bdOut, int iHScale, int iVScale)
{
  // Luma
  T64_Untile_Component_HighBD<DstType>(pSrc, pDst, bdIn, bdOut, AL_PLANE_Y);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iWidthC = 2 * ((tDim.iWidth + iHScale - 1) / iHScale);
  int iHeightC = (tDim.iHeight + iVScale - 1) / iVScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDstU = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U) / sizeof(DstType);
  int iPitchDstV = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V) / sizeof(DstType);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  DstType* pDstDataU = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  DstType* pDstDataV = (DstType*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int h = 0; h < iHeightC; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      DstType* pOutU = pDstDataU + h * iPitchDstU + w / 2;
      DstType* pOutV = pDstDataV + h * iPitchDstV + w / 2;

      if(bdIn == 12)
      {
        if(bdOut == 12)
        {
          Untile4x4ChromaBlock12bToPlanar12b(pSrcData, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV);
        }
        else if(bdOut == 10)
        {
          Untile4x4ChromaBlock12bToPlanar10b(pSrcData, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV);
        }
        else
        {
          Untile4x4ChromaBlock12bToPlanar8b(pSrcData, (uint8_t*)pOutU, (uint8_t*)pOutV, iPitchDstU, iPitchDstV);
        }
      }
      else // bdIn 10 bit
      {
        bdOut == 10 ?
        Untile4x4ChromaBlock10bToPlanar10b(pSrcData, (uint16_t*)pOutU, (uint16_t*)pOutV, iPitchDstU, iPitchDstV) :
        Untile4x4ChromaBlock10bToPlanar8b(pSrcData, (uint8_t*)pOutU, (uint8_t*)pOutV, iPitchDstU, iPitchDstV);
      }
      pSrcData += bdIn;
    }

    pSrcData += iPitchSrc - ((iWidthC * bdIn / 2)) / sizeof(uint16_t);
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
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block10bTo8b(pInC, pOutC, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T60A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;
  int iHeightC = (tDim.iHeight + 1) >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < iWidthC; W += iTileW)
    {
      int iCropW = (W + iTileW) - iWidthC;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block1xbTo1xb(pInC, pOutC, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

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

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (iWidthC * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pOutC = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      Untile4x4Block10bTo8b(pInC, pOutC, iPitchDst);
      pOutC -= 3 * iPitchDst - 4;
      pInC += 10;
    }

    pOutC += iPitchDst * 4 - iWidthC;
    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void T62X_To_P21X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uBd)
{
  // Luma
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, uBd, uBd, AL_PLANE_Y);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iWidthC = ((tDim.iWidth + 1) >> 1) << 1;

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (iWidthC * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < iWidthC; w += 4)
    {
      uint16_t* pOutC = pDstData + h * iPitchDst + w;
      Untile4x4Block1xbTo1xb(pInC, pOutC, iPitchDst);
      pInC += 10;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
void T62A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T62X_To_P21X(pSrc, pDst, 10);
}

/****************************************************************************/
void T62C_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T62X_To_P21X(pSrc, pDst, 12);
}

/****************************************************************************/
void T648_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component(pSrc, pDst, AL_PLANE_Y);
  T64_Untile_Component(pSrc, pDst, AL_PLANE_U);
  T64_Untile_Component(pSrc, pDst, AL_PLANE_V);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void T64A_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_Y);
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_U);
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 10, 10, AL_PLANE_V);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
}

/****************************************************************************/
void T64C_To_I4CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_Y);
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_U);
  T64_Untile_Component_HighBD<uint16_t>(pSrc, pDst, 12, 12, AL_PLANE_V);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4CL));
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

  assert(iPitchSrc % 4 == 0);

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

  assert(iPitchSrc % 4 == 0);

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

  assert(iPitchSrc % 4 == 0);

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

  assert(iPitchSrc % 4 == 0);

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

  assert(iPitchSrc % 4 == 0);

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

  assert(iPitchSrc % 4 == 0);

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
      pBufOutU[iW] = pBufInC[(iW << 1)];
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

  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
  uint16_t* pBufOutV = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOutU[iW] = ((uint16_t)pBufIn[(iW << 1)]) << 2;
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
  default: assert(0 && "Unsupported chroma scale");
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
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

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
  default: assert(0 && "Unsupported chroma scale");
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

  assert(iPitchSrc % 4 == 0);

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
void CopyPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pSrc);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_TPicFormat tPicFormat;
  assert(AL_GetPicFormat(tFourCC, &tPicFormat) && !tPicFormat.b10bPacked && !tPicFormat.bCompressed);

  AL_PixMapBuffer_SetFourCC(pDst, tFourCC);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_Y, tDim.iWidth, tDim.iHeight, tPicFormat.uBitDepth);

  // Chroma
  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return;

  int iChromaWidth = tDim.iWidth;
  int iChromaHeight = tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? (tDim.iHeight + 1) / 2 : tDim.iHeight;

  if(tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
  {
    if(tPicFormat.eChromaMode == AL_CHROMA_4_4_4)
      iChromaWidth *= 2;
    else
      iChromaWidth = ((iChromaWidth + 1) / 2) * 2;

    CopyPixMapPlane(pSrc, pDst, AL_PLANE_UV, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
    return;
  }

  if(tPicFormat.eChromaMode != AL_CHROMA_4_4_4)
    iChromaWidth = (iChromaWidth + 1) / 2;

  CopyPixMapPlane(pSrc, pDst, AL_PLANE_U, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
  CopyPixMapPlane(pSrc, pDst, AL_PLANE_V, iChromaWidth, iChromaHeight, tPicFormat.uBitDepth);
}

/*@}*/

