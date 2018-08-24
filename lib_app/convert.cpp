/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <cstring>
#include <cassert>
#include <iostream>

extern "C" {
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferAPI.h"
}

#include "convert.h"

#define RND_10B_TO_8B(val) (((val) >= 0x3FC) ? 0xFF : (((val) + 2) >> 2))

static void SetFourCC(AL_TSrcMetaData* pMetaData, TFourCC tFourCC420, TFourCC tFourCC422, int iScale)
{
  switch(iScale)
  {
  case 2:
    pMetaData->tFourCC = tFourCC422;
    break;

  case 4:
    pMetaData->tFourCC = tFourCC420;
    break;

  default:
    break;
  }
}

/****************************************************************************/
void I420_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  AL_CopyYuv(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
static void SwapChromaComponents(AL_TBuffer const* pSrc, AL_TBuffer* pDst, TFourCC tDestFourCC)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iLumaSize = (pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight);
  int iChromaSize = iLumaSize >> 2;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = tDestFourCC;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // luma
  memcpy(pDstData, pSrcData, iLumaSize);
  // Chroma
  memcpy(pDstData + iLumaSize, pSrcData + iLumaSize + iChromaSize, iChromaSize);
  memcpy(pDstData + iLumaSize + iChromaSize, pSrcData + iLumaSize, iChromaSize);
}

/****************************************************************************/
void I420_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SwapChromaComponents(pSrc, pDst, FOURCC(YV12));
}

/****************************************************************************/
void I420_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  assert(pSrcMeta);
  assert(pDstMeta);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // Luma
  AL_VADDR pSrcY = AL_Buffer_GetData(pSrc);
  AL_VADDR pDstY = AL_Buffer_GetData(pDst);

  for(int iH = 0; iH < pSrcMeta->tDim.iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, pSrcMeta->tDim.iWidth);
    pDstY += pDstMeta->tPitches.iLuma;
    pSrcY += pSrcMeta->tDim.iWidth;
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void I420_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y010);

  // Luma
  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst));
  int iSize = iLumaSize;

  while(iSize--)
    *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
}

/****************************************************************************/
void I420_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(I0AL);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    uint8_t* pBufIn = pSrcData;
    uint16_t* pBufOut = (uint16_t*)(pDstData);

    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }

  // Chroma
  {
    uint8_t* pBufIn = pSrcData + iLumaSize;
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iLumaSize;

    int iSize = iChromaSize;

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }
}

/****************************************************************************/
void IYUV_To_420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_CopyYuv(pSrc, pDst);
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
  SwapChromaComponents(pSrc, pDst, FOURCC(I420));
}

/****************************************************************************/
void YV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  YV12_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void YV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), iSize);
}

/****************************************************************************/
void YV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(I0AL);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  {
    uint8_t* pBufIn = pSrcData;
    uint16_t* pBufOut = (uint16_t*)(pDstData);
    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }

  // Chroma
  {
    uint8_t* pBufInV = pSrcData + iLumaSize;
    uint8_t* pBufInU = pSrcData + iLumaSize + iChromaSize;
    uint16_t* pBufOutU = ((uint16_t*)(pDstData)) + iLumaSize;
    uint16_t* pBufOutV = ((uint16_t*)(pDstData)) + iLumaSize + iChromaSize;
    int iSize = iChromaSize;

    while(iSize--)
    {
      *pBufOutU++ = ((uint16_t)*pBufInU++) << 2;
      *pBufOutV++ = ((uint16_t)*pBufInV++) << 2;
    }
  }
}

/****************************************************************************/
void NV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), iSize);
}

/****************************************************************************/
void Y800_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  memcpy(pDstData, AL_Buffer_GetData(pSrc), iSize);

  // Chroma
  Rtos_Memset(pDstData + iSize, 0x80, iSize >> 1);
}

/****************************************************************************/
void Y800_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  Y800_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void Y800_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  Y800_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void Y800_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  Y800_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void Y800_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(P010);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  {
    uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
    uint16_t* pBufOut = (uint16_t*)(pDstData);
    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }
  // Chroma
  {
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iLumaSize;
    int iSize = iChromaSize;

    while(iSize--)
      *pBufOut++ = 0x0200;
  }
}

/****************************************************************************/
void Y800_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  Y800_To_P010(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void Y800_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->tDim.iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
      *pDst32 |= ((uint32_t)*pSrcY++) << 22;
      ++pDst32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
    }
  }

  pDstMeta->tFourCC = FOURCC(XV15);
}


/****************************************************************************/
void Y800_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y010);

  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst));
  int iDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  for(int iH = 0; iH < pDstMeta->tDim.iHeight; ++iH)
  {
    for(int iW = 0; iW < pDstMeta->tDim.iWidth; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += iDstPitchLuma;
  }
}

/****************************************************************************/
static uint32_t toTen(uint8_t sample)
{
  return (((uint32_t)sample) << 2) & 0x3FF;
}

/****************************************************************************/
void Y800_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->tDim.iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
      *pDst32 |= toTen(*pSrcY++) << 20;
      ++pDst32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDst32 = toTen(*pSrcY++);
    }
  }

  pDstMeta->tFourCC = FOURCC(XV10);
}

/****************************************************************************/
static void SemiPlanar_To_XV_OneComponent(AL_TBuffer const* pSrcBuf, AL_TBuffer* pDstBuf, bool bProcessY, int iVrtScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrcBuf, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDstBuf, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  int iDstHeight = pDstMeta->tDim.iHeight / iVrtScale;
  int iPitchSrc, iPitchDst;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrcBuf);
  uint8_t* pDstData = AL_Buffer_GetData(pDstBuf);

  if(bProcessY)
  {
    pSrcData += pSrcMeta->tOffsetYC.iLuma;
    pDstData += pDstMeta->tOffsetYC.iLuma;
    iPitchSrc = pSrcMeta->tPitches.iLuma;
    iPitchDst = pDstMeta->tPitches.iLuma;
  }
  else
  {
    assert(pSrcMeta->tPitches.iChroma % 4 == 0);
    pSrcData += pSrcMeta->tOffsetYC.iChroma;
    pDstData += pDstMeta->tOffsetYC.iChroma;
    iPitchSrc = pSrcMeta->tPitches.iChroma;
    iPitchDst = pDstMeta->tPitches.iChroma;
  }

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + h * iPitchSrc);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 20;
      ++pDst;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
    }
  }
}

/****************************************************************************/
void Y010_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV10);
}

/****************************************************************************/
void Y010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV15);
}


/****************************************************************************/
static void PX10_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeDstY = pDstMeta->tDim.iWidth * pDstMeta->tDim.iHeight;
  int iCScale = uHrzCScale * uVrtCScale;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)pSrcData;
    uint8_t* pBufOut = pDstData;
    uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);

    for(int iH = 0; iH < pDstMeta->tDim.iHeight; ++iH)
    {
      for(int iW = 0; iW < pDstMeta->tDim.iWidth; ++iW)
        pBufOut[iW] = (uint8_t)((2 + pBufIn[iW]) >> 2);

      pBufIn += uSrcPitchLuma;
      pBufOut += pDstMeta->tPitches.iLuma;
    }
  }
  // Chroma
  {
    uint16_t* pBufInC = (uint16_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma);
    uint8_t* pBufOutU = pDstData + iSizeDstY;
    uint8_t* pBufOutV = pDstData + iSizeDstY + (iSizeDstY / iCScale);
    uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

    int iWidth = pDstMeta->tDim.iWidth / uHrzCScale;
    int iHeight = pDstMeta->tDim.iHeight / uVrtCScale;

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = (uint8_t)((2 + pBufInC[(iW << 1)]) >> 2);
        pBufOutV[iW] = (uint8_t)((2 + pBufInC[(iW << 1) + 1]) >> 2);
      }

      pBufInC += uSrcPitchChroma;
      pBufOutU += pDstMeta->tPitches.iChroma;
      pBufOutV += pDstMeta->tPitches.iChroma;
    }
  }
  SetFourCC(pDstMeta, FOURCC(I420), FOURCC(I422), iCScale);
}

/****************************************************************************/
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_I42X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void P210_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_I42X(pSrc, pDst, 2, 1);
}

void P210_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 1);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV20);
}


/****************************************************************************/
void P010_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  PX10_To_I42X(pSrc, pDst, 2, 2);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
static void PX10_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  P010_To_Y010(pSrc, pDst);

  // Chroma
  int iSizeDst = pDstMeta->tDim.iWidth * pDstMeta->tDim.iHeight;
  int iCScale = uHrzCScale * uVrtCScale;

  uint16_t* pBufIn = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tOffsetYC.iChroma);
  uint16_t* pBufOutU = ((uint16_t*)AL_Buffer_GetData(pDst)) + iSizeDst;
  uint16_t* pBufOutV = ((uint16_t*)AL_Buffer_GetData(pDst)) + iSizeDst + (iSizeDst / iCScale);

  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iWidth = pDstMeta->tDim.iWidth / uHrzCScale;
  int iHeight = pDstMeta->tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = pBufIn[(iW << 1)];
      pBufOutV[iW] = pBufIn[(iW << 1) + 1];
    }

    pBufIn += uSrcPitchChroma;
    pBufOutU += uDstPitchChroma;
    pBufOutV += uDstPitchChroma;
  }

  SetFourCC(pDstMeta, FOURCC(I0AL), FOURCC(I2AL), iCScale);
}

/****************************************************************************/
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void P210_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void P010_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(YV12);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)pSrcData;
    uint8_t* pBufOut = pDstData;
    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);
  }

  // Chroma
  {
    uint16_t* pBufIn = ((uint16_t*)pSrcData) + iLumaSize;
    uint8_t* pBufOutV = pDstData + iLumaSize;
    uint8_t* pBufOutU = pDstData + iLumaSize + iChromaSize;
    int iSize = iChromaSize;

    while(iSize--)
    {
      *pBufOutU++ = (uint8_t)((2 + *pBufIn++) >> 2);
      *pBufOutV++ = (uint8_t)((2 + *pBufIn++) >> 2);
    }
  }
}

/****************************************************************************/
void P010_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(NV12);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)pSrcData;
    uint8_t* pBufOut = pDstData;
    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);
  }
  // Chroma
  {
    uint16_t* pBufIn = ((uint16_t*)pSrcData) + iLumaSize;
    uint8_t* pBufOut = pDstData + iLumaSize;
    int iSize = iChromaSize >> 1;

    while(iSize--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);
  }
}

/****************************************************************************/
void P010_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void P010_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void P010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 2);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV15);
}


/****************************************************************************/
void I0AL_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // Luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  uint16_t* pBufIn = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tPitches.iLuma * pSrcMeta->tDim.iHeight);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst) + pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  int iH = pSrcMeta->tDim.iHeight;

  while(iH--)
  {
    int iW = pSrcMeta->tDim.iWidth >> 1;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += pDstMeta->tPitches.iChroma - (pSrcMeta->tDim.iWidth >> 1);
    pBufIn += uSrcPitchChroma - (pSrcMeta->tDim.iWidth >> 1);
  }

  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void I0AL_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  I0AL_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void I0AL_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_Buffer_GetData(pSrc);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst);
  uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);

  int iH = pSrcMeta->tDim.iHeight;

  while(iH--)
  {
    int iW = pSrcMeta->tDim.iWidth;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += uSrcPitchLuma - pSrcMeta->tDim.iWidth;
    pBufIn += uSrcPitchLuma - pSrcMeta->tDim.iWidth;
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)AL_Buffer_GetData(pDst);
  uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);
  uint32_t uDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  int iH = pSrcMeta->tDim.iHeight;

  while(iH--)
  {
    memcpy(pBufOut, pBufIn, pSrcMeta->tDim.iWidth * sizeof(uint16_t));
    pBufOut += uDstPitchLuma;
    pBufIn += uSrcPitchLuma;
  }

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
static void I42X_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight);
  int iCScale = uHrzCScale * uVrtCScale;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // Luma
  AL_VADDR pSrcY = pSrcData;
  AL_VADDR pDstY = pDstData;

  for(int iH = 0; iH < pSrcMeta->tDim.iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, pSrcMeta->tDim.iWidth);
    pDstY += pDstMeta->tPitches.iLuma;
    pSrcY += pSrcMeta->tDim.iWidth;
  }

  // Chroma
  int iChromaSecondCompOffset = iSize / iCScale;
  AL_VADDR pBufInU = pSrcData + iSize + (bIsUFirst ? 0 : iChromaSecondCompOffset);
  AL_VADDR pBufInV = pSrcData + iSize + (bIsUFirst ? iChromaSecondCompOffset : 0);
  AL_VADDR pBufOut = pDstData + (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);

  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iWidthC = pSrcMeta->tDim.iWidth / uHrzCScale;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[iW * 2] = pBufInU[iW];
      pBufOut[iW * 2 + 1] = pBufInV[iW];
    }

    pBufOut += pDstMeta->tPitches.iChroma;
    pBufInU += iWidthC;
    pBufInV += iWidthC;
  }

  SetFourCC(pDstMeta, FOURCC(NV12), FOURCC(NV16), iCScale);
}

/****************************************************************************/
void YV12_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void I420_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void I42X_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tDim.iWidth * pSrcMeta->tDim.iHeight;
  int iCScale = uHrzCScale * uVrtCScale;
  const int iChromaSize = iLumaSize / iCScale;

  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pBufInU = pSrcData + iLumaSize + (bIsUFirst ? 0 : iChromaSize);
  uint8_t* pBufInV = pSrcData + iLumaSize + (bIsUFirst ? iChromaSize : 0);
  uint16_t* pBufOut = ((uint16_t*)(AL_Buffer_GetData(pDst))) + iLumaSize;
  int iSize = iChromaSize;

  while(iSize--)
  {
    *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
    *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
  }

  SetFourCC(pDstMeta, FOURCC(P010), FOURCC(P210), iCScale);
}

/****************************************************************************/
void YV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void I420_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void I42X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y800_To_XV15(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pSrcMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  int iSrcSizeY = pDstMeta->tDim.iHeight * pSrcMeta->tPitches.iLuma;
  int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcFirstCComp = pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma;
    uint8_t* pSrcU = (uint8_t*)(pSrcFirstCComp + (bIsUFirst ? 0 : iSrcSizeC));
    uint8_t* pSrcV = (uint8_t*)(pSrcFirstCComp + (bIsUFirst ? iSrcSizeC : 0));

    int w = pSrcMeta->tDim.iWidth / 6;

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

    if(pSrcMeta->tDim.iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(pSrcMeta->tDim.iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  SetFourCC(pDstMeta, FOURCC(XV15), FOURCC(XV20), uHrzCScale * uVrtCScale);
}

/****************************************************************************/
void YV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2, false);
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
static void IXAL_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iWidthC = pSrcMeta->tDim.iWidth / uHrzCScale;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  uint16_t* pBufInU = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tOffsetYC.iChroma);
  uint16_t* pBufInV = pBufInU + uSrcPitchChroma * iHeightC;
  uint8_t* pBufOut = AL_Buffer_GetData(pDst) + pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = (uint8_t)((2 + *pBufInU++) >> 2);
      *pBufOut++ = (uint8_t)((2 + *pBufInV++) >> 2);
    }

    pBufOut += pDstMeta->tPitches.iChroma - pDstMeta->tDim.iWidth;
    pBufInU += uSrcPitchChroma - iWidthC;
    pBufInV += uSrcPitchChroma - iWidthC;
  }

  SetFourCC(pDstMeta, FOURCC(NV12), FOURCC(NV16), uHrzCScale * uVrtCScale);
}

/****************************************************************************/
void I0AL_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void IXAL_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;

  // Luma
  I0AL_To_Y010(pSrc, pDst);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iWidthC = pSrcMeta->tDim.iWidth / uHrzCScale;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst) + pDstMeta->tOffsetYC.iChroma);
  uint16_t* pBufInU = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tOffsetYC.iChroma);
  uint16_t* pBufInV = pBufInU + uSrcPitchChroma * iHeightC;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[2 * iW] = pBufInU[iW];
      pBufOut[2 * iW + 1] = pBufInV[iW];
    }

    pBufInU += uSrcPitchChroma;
    pBufInV += uSrcPitchChroma;
    pBufOut += uDstPitchChroma;
  }

  SetFourCC(pDstMeta, FOURCC(P010), FOURCC(P210), uHrzCScale * uVrtCScale);
}

/****************************************************************************/
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void IXAL_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y010_To_XV10(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pSrcMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pSrcU = (uint16_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pSrcV = pSrcU + uSrcPitchChroma * iHeightC;

    int w = pSrcMeta->tDim.iWidth / 6;

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

    if(pSrcMeta->tDim.iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
    }
    else if(pSrcMeta->tDim.iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
    }
  }

  SetFourCC(pDstMeta, FOURCC(XV15), FOURCC(XV20), uHrzCScale * uVrtCScale);
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



void ALX8_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHeightC, bool bIsUFirst = true)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  const int iTileW = 64;
  const int iTileH = 4;

  int iOffsetFirstChromaComp = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetSecondChromaComp = iOffsetFirstChromaComp + (pDstMeta->tPitches.iChroma * iHeightC);
  int iOffsetU = bIsUFirst ? iOffsetFirstChromaComp : iOffsetSecondChromaComp;
  int iOffsetV = bIsUFirst ? iOffsetSecondChromaComp : iOffsetFirstChromaComp;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + pSrcMeta->tOffsetYC.iChroma + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstData + iOffsetU + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;
          uint8_t* pOutV = pDstData + iOffsetV + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;

          pOutU[0] = pInC[0];
          pOutV[0] = pInC[1];
          pOutU[1] = pInC[2];
          pOutV[1] = pInC[3];
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = pInC[4];
          pOutV[0] = pInC[5];
          pOutU[1] = pInC[6];
          pOutV[1] = pInC[7];
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = pInC[8];
          pOutV[0] = pInC[9];
          pOutU[1] = pInC[10];
          pOutV[1] = pInC[11];
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = pInC[12];
          pOutV[0] = pInC[13];
          pOutU[1] = pInC[14];
          pOutV[1] = pInC[15];
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }
}

/****************************************************************************/
void T608_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  T608_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pDstMeta->tDim.iHeight >> 1;
  ALX8_To_I42X(pSrc, pDst, iHeightC);
  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void T608_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  T608_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void T608_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  T608_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pDstMeta->tDim.iHeight >> 1;
  ALX8_To_I42X(pSrc, pDst, iHeightC, false);
  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void T608_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + iOffsetC + (H + h) * pDstMeta->tPitches.iChroma + (W + w);

          pOutC[0] = pInC[0];
          pOutC[1] = pInC[1];
          pOutC[2] = pInC[2];
          pOutC[3] = pInC[3];
          pOutC += pDstMeta->tPitches.iChroma;
          pOutC[0] = pInC[4];
          pOutC[1] = pInC[5];
          pOutC[2] = pInC[6];
          pOutC[3] = pInC[7];
          pOutC += pDstMeta->tPitches.iChroma;
          pOutC[0] = pInC[8];
          pOutC[1] = pInC[9];
          pOutC[2] = pInC[10];
          pOutC[3] = pInC[11];
          pOutC += pDstMeta->tPitches.iChroma;
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

  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void T608_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma;

    int iCropH = (H + iTileH) - pDstMeta->tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutY = pDstData + (H + h) * pDstMeta->tPitches.iLuma + (W + w);

          pOutY[0] = pInY[0];
          pOutY[1] = pInY[1];
          pOutY[2] = pInY[2];
          pOutY[3] = pInY[3];
          pOutY += pDstMeta->tPitches.iLuma;
          pOutY[0] = pInY[4];
          pOutY[1] = pInY[5];
          pOutY[2] = pInY[6];
          pOutY[3] = pInY[7];
          pOutY += pDstMeta->tPitches.iLuma;
          pOutY[0] = pInY[8];
          pOutY[1] = pInY[9];
          pOutY[2] = pInY[10];
          pOutY[3] = pInY[11];
          pOutY += pDstMeta->tPitches.iLuma;
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

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void T608_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  int iDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma;

    int iCropH = (H + iTileH) - pDstMeta->tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutY = ((uint16_t*)pDstData) + (H + h) * iDstPitchLuma + (W + w);

          pOutY[0] = ((uint16_t)pInY[0]) << 2;
          pOutY[1] = ((uint16_t)pInY[1]) << 2;
          pOutY[2] = ((uint16_t)pInY[2]) << 2;
          pOutY[3] = ((uint16_t)pInY[3]) << 2;
          pOutY += iDstPitchLuma;
          pOutY[0] = ((uint16_t)pInY[4]) << 2;
          pOutY[1] = ((uint16_t)pInY[5]) << 2;
          pOutY[2] = ((uint16_t)pInY[6]) << 2;
          pOutY[3] = ((uint16_t)pInY[7]) << 2;
          pOutY += iDstPitchLuma;
          pOutY[0] = ((uint16_t)pInY[8]) << 2;
          pOutY[1] = ((uint16_t)pInY[9]) << 2;
          pOutY[2] = ((uint16_t)pInY[10]) << 2;
          pOutY[3] = ((uint16_t)pInY[11]) << 2;
          pOutY += iDstPitchLuma;
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

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void T608_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = ((uint16_t*)(pDstData + iOffsetC)) + (H + h) * iDstPitchChroma + (W + w);

          pOutC[0] = ((uint16_t)pInC[0]) << 2;
          pOutC[1] = ((uint16_t)pInC[1]) << 2;
          pOutC[2] = ((uint16_t)pInC[2]) << 2;
          pOutC[3] = ((uint16_t)pInC[3]) << 2;
          pOutC += iDstPitchChroma;
          pOutC[0] = ((uint16_t)pInC[4]) << 2;
          pOutC[1] = ((uint16_t)pInC[5]) << 2;
          pOutC[2] = ((uint16_t)pInC[6]) << 2;
          pOutC[3] = ((uint16_t)pInC[7]) << 2;
          pOutC += iDstPitchChroma;
          pOutC[0] = ((uint16_t)pInC[8]) << 2;
          pOutC[1] = ((uint16_t)pInC[9]) << 2;
          pOutC[2] = ((uint16_t)pInC[10]) << 2;
          pOutC[3] = ((uint16_t)pInC[11]) << 2;
          pOutC += iDstPitchChroma;
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

  pDstMeta->tFourCC = FOURCC(P010);
}

/****************************************************************************/
void T608_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * iHeightC);
  int iDstPichChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = (uint16_t*)(pDstData + iOffsetU) + (H + h) * iDstPichChroma + (W + w) / 2;
          uint16_t* pOutV = (uint16_t*)(pDstData + iOffsetV) + (H + h) * iDstPichChroma + (W + w) / 2;

          pOutU[0] = ((uint16_t)pInC[0]) << 2;
          pOutV[0] = ((uint16_t)pInC[1]) << 2;
          pOutU[1] = ((uint16_t)pInC[2]) << 2;
          pOutV[1] = ((uint16_t)pInC[3]) << 2;
          pOutU += iDstPichChroma;
          pOutV += iDstPichChroma;
          pOutU[0] = ((uint16_t)pInC[4]) << 2;
          pOutV[0] = ((uint16_t)pInC[5]) << 2;
          pOutU[1] = ((uint16_t)pInC[6]) << 2;
          pOutV[1] = ((uint16_t)pInC[7]) << 2;
          pOutU += iDstPichChroma;
          pOutV += iDstPichChroma;
          pOutU[0] = ((uint16_t)pInC[8]) << 2;
          pOutV[0] = ((uint16_t)pInC[9]) << 2;
          pOutU[1] = ((uint16_t)pInC[10]) << 2;
          pOutV[1] = ((uint16_t)pInC[11]) << 2;
          pOutU += iDstPichChroma;
          pOutV += iDstPichChroma;
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

  pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void T6m8_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeY = (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);
  int iSizeC = (pDstMeta->tPitches.iChroma * pDstMeta->tDim.iHeight / 2);

  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  Rtos_Memset(AL_Buffer_GetData(pDst) + iSizeY, 0x80, 2 * iSizeC);

  pDstMeta->tFourCC = FOURCC(I420);
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
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrcBuf, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDstBuf, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->tDim.iWidth + 2) / 3 * 4);

  int iDstHeight = pDstMeta->tDim.iHeight / iVrtScale;
  int iPitchSrc, iPitchDst;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrcBuf);
  uint8_t* pDstData = AL_Buffer_GetData(pDstBuf);

  if(bProcessY)
  {
    pSrcData += pSrcMeta->tOffsetYC.iLuma;
    pDstData += pDstMeta->tOffsetYC.iLuma;
    iPitchSrc = pSrcMeta->tPitches.iLuma;
    iPitchDst = pDstMeta->tPitches.iLuma;
  }
  else
  {
    pSrcData += pSrcMeta->tOffsetYC.iChroma;
    pDstData += pDstMeta->tOffsetYC.iChroma;
    iPitchSrc = pSrcMeta->tPitches.iChroma;
    iPitchDst = pDstMeta->tPitches.iChroma;
  }

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + (h >> 2) * iPitchSrc);

    int hInsideTile = h & 0x3;

    int w = 0;
    int wStop = pSrcMeta->tDim.iWidth - 2;

    while(w < wStop)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 20;
      ++pDst;
    }

    if(w < pDstMeta->tDim.iWidth)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);

      if(w < pDstMeta->tDim.iWidth)
      {
        *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      }
    }
  }
}


/****************************************************************************/
void T628_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y800(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void T628_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y010(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void T628_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pDstMeta->tDim.iHeight;
  ALX8_To_I42X(pSrc, pDst, iHeightC);

  pDstMeta->tFourCC = FOURCC(I422);
}

/****************************************************************************/
void T628_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + pSrcMeta->tOffsetYC.iChroma;
  uint8_t* pOutC = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 4);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      pOutC[0] = pInC[0];
      pOutC[1] = pInC[1];
      pOutC[2] = pInC[2];
      pOutC[3] = pInC[3];
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = pInC[4];
      pOutC[1] = pInC[5];
      pOutC[2] = pInC[6];
      pOutC[3] = pInC[7];
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = pInC[8];
      pOutC[1] = pInC[9];
      pOutC[2] = pInC[10];
      pOutC[3] = pInC[11];
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = pInC[12];
      pOutC[1] = pInC[13];
      pOutC[2] = pInC[14];
      pOutC[3] = pInC[15];
      pOutC -= 3 * pDstMeta->tPitches.iChroma - 4;
      pInC += 16;
    }

    pOutC += pDstMeta->tPitches.iChroma * 4 - pDstMeta->tDim.iWidth;
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(NV16);
}

/****************************************************************************/
void T628_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  int iOffsetU = pDstMeta->tPitches.iLuma * pSrcMeta->tDim.iHeight;
  int iOffsetV = iOffsetU + pDstMeta->tPitches.iChroma * pSrcMeta->tDim.iHeight;
  int uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + iSrcLumaSize;

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 4);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      uint16_t* pOutU = (uint16_t*)(pDstData + iOffsetU) + h * uDstPitchChroma + w / 2;
      uint16_t* pOutV = (uint16_t*)(pDstData + iOffsetV) + h * uDstPitchChroma + w / 2;

      pOutU[0] = ((uint16_t)pInC[0]) << 2;
      pOutV[0] = ((uint16_t)pInC[1]) << 2;
      pOutU[1] = ((uint16_t)pInC[2]) << 2;
      pOutV[1] = ((uint16_t)pInC[3]) << 2;
      pOutU += uDstPitchChroma;
      pOutV += uDstPitchChroma;
      pOutU[0] = ((uint16_t)pInC[4]) << 2;
      pOutV[0] = ((uint16_t)pInC[5]) << 2;
      pOutU[1] = ((uint16_t)pInC[6]) << 2;
      pOutV[1] = ((uint16_t)pInC[7]) << 2;
      pOutU += uDstPitchChroma;
      pOutV += uDstPitchChroma;
      pOutU[0] = ((uint16_t)pInC[8]) << 2;
      pOutV[0] = ((uint16_t)pInC[9]) << 2;
      pOutU[1] = ((uint16_t)pInC[10]) << 2;
      pOutV[1] = ((uint16_t)pInC[11]) << 2;
      pOutU += uDstPitchChroma;
      pOutV += uDstPitchChroma;
      pOutU[0] = ((uint16_t)pInC[12]) << 2;
      pOutV[0] = ((uint16_t)pInC[13]) << 2;
      pOutU[1] = ((uint16_t)pInC[14]) << 2;
      pOutV[1] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(I2AL);
}

/****************************************************************************/
void T628_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + iSrcLumaSize;
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 4);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      uint16_t* pOutC = ((uint16_t*)pDstData) + h * uDstPitchChroma + w;

      pOutC[0] = ((uint16_t)pInC[0]) << 2;
      pOutC[1] = ((uint16_t)pInC[1]) << 2;
      pOutC[2] = ((uint16_t)pInC[2]) << 2;
      pOutC[3] = ((uint16_t)pInC[3]) << 2;
      pOutC += uDstPitchChroma;
      pOutC[0] = ((uint16_t)pInC[4]) << 2;
      pOutC[1] = ((uint16_t)pInC[5]) << 2;
      pOutC[2] = ((uint16_t)pInC[6]) << 2;
      pOutC[3] = ((uint16_t)pInC[7]) << 2;
      pOutC += uDstPitchChroma;
      pOutC[0] = ((uint16_t)pInC[8]) << 2;
      pOutC[1] = ((uint16_t)pInC[9]) << 2;
      pOutC[2] = ((uint16_t)pInC[10]) << 2;
      pOutC[3] = ((uint16_t)pInC[11]) << 2;
      pOutC += uDstPitchChroma;
      pOutC[0] = ((uint16_t)pInC[12]) << 2;
      pOutC[1] = ((uint16_t)pInC[13]) << 2;
      pOutC[2] = ((uint16_t)pInC[14]) << 2;
      pOutC[3] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(P210);
}

/****************************************************************************/
void T60A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstData + iOffsetU + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;
          uint8_t* pOutV = pDstData + iOffsetV + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;

          pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[0] & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[1] >> 4) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[3] >> 2) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[4] >> 6);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[5] & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[6] >> 4) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[8] >> 2) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[9] >> 6);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void T60A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  T60A_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void T60A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetV = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetU = iOffsetV + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstData + iOffsetU + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;
          uint8_t* pOutV = pDstData + iOffsetV + (H + h) * pDstMeta->tPitches.iChroma + (W + w) / 2;

          pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[0] & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[1] >> 4) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[3] >> 2) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[4] >> 6);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[5] & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[6] >> 4) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF);
          pOutU += pDstMeta->tPitches.iChroma;
          pOutV += pDstMeta->tPitches.iChroma;
          pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF);
          pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[8] >> 2) & 0x3FF);
          pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF);
          pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[9] >> 6);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void T60A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + iOffsetC + (H + h) * pDstMeta->tPitches.iChroma + (W + w);

          pOutC[0] = (uint8_t)RND_10B_TO_8B(pInC[0] & 0x3FF);
          pOutC[1] = (uint8_t)RND_10B_TO_8B(((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF);
          pOutC[2] = (uint8_t)RND_10B_TO_8B((pInC[1] >> 4) & 0x3FF);
          pOutC[3] = (uint8_t)RND_10B_TO_8B(((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF);
          pOutC += pDstMeta->tPitches.iChroma;
          pOutC[0] = (uint8_t)RND_10B_TO_8B(((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF);
          pOutC[1] = (uint8_t)RND_10B_TO_8B((pInC[3] >> 2) & 0x3FF);
          pOutC[2] = (uint8_t)RND_10B_TO_8B(((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF);
          pOutC[3] = (uint8_t)RND_10B_TO_8B(pInC[4] >> 6);
          pOutC += pDstMeta->tPitches.iChroma;
          pOutC[0] = (uint8_t)RND_10B_TO_8B(pInC[5] & 0x3FF);
          pOutC[1] = (uint8_t)RND_10B_TO_8B(((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF);
          pOutC[2] = (uint8_t)RND_10B_TO_8B((pInC[6] >> 4) & 0x3FF);
          pOutC[3] = (uint8_t)RND_10B_TO_8B(((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF);
          pOutC += pDstMeta->tPitches.iChroma;
          pOutC[0] = (uint8_t)RND_10B_TO_8B(((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF);
          pOutC[1] = (uint8_t)RND_10B_TO_8B((pInC[8] >> 2) & 0x3FF);
          pOutC[2] = (uint8_t)RND_10B_TO_8B(((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF);
          pOutC[3] = (uint8_t)RND_10B_TO_8B(pInC[9] >> 6);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void T60A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->tDim.iHeight; H += iTileH)
  {
    uint16_t* pInY = (uint16_t*)(pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma);

    int iCropH = (H + iTileH) - pDstMeta->tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutY = pDstData + (H + h) * pDstMeta->tPitches.iLuma + (W + w);

          pOutY[0] = (uint8_t)RND_10B_TO_8B(pInY[0] & 0x3FF);
          pOutY[1] = (uint8_t)RND_10B_TO_8B(((pInY[0] >> 10) | (pInY[1] << 6)) & 0x3FF);
          pOutY[2] = (uint8_t)RND_10B_TO_8B((pInY[1] >> 4) & 0x3FF);
          pOutY[3] = (uint8_t)RND_10B_TO_8B(((pInY[1] >> 14) | (pInY[2] << 2)) & 0x3FF);
          pOutY += pDstMeta->tPitches.iLuma;
          pOutY[0] = (uint8_t)RND_10B_TO_8B(((pInY[2] >> 8) | (pInY[3] << 8)) & 0x3FF);
          pOutY[1] = (uint8_t)RND_10B_TO_8B((pInY[3] >> 2) & 0x3FF);
          pOutY[2] = (uint8_t)RND_10B_TO_8B(((pInY[3] >> 12) | (pInY[4] << 4)) & 0x3FF);
          pOutY[3] = (uint8_t)RND_10B_TO_8B(pInY[4] >> 6);
          pOutY += pDstMeta->tPitches.iLuma;
          pOutY[0] = (uint8_t)RND_10B_TO_8B(pInY[5] & 0x3FF);
          pOutY[1] = (uint8_t)RND_10B_TO_8B(((pInY[5] >> 10) | (pInY[6] << 6)) & 0x3FF);
          pOutY[2] = (uint8_t)RND_10B_TO_8B((pInY[6] >> 4) & 0x3FF);
          pOutY[3] = (uint8_t)RND_10B_TO_8B(((pInY[6] >> 14) | (pInY[7] << 2)) & 0x3FF);
          pOutY += pDstMeta->tPitches.iLuma;
          pOutY[0] = (uint8_t)RND_10B_TO_8B(((pInY[7] >> 8) | (pInY[8] << 8)) & 0x3FF);
          pOutY[1] = (uint8_t)RND_10B_TO_8B((pInY[8] >> 2) & 0x3FF);
          pOutY[2] = (uint8_t)RND_10B_TO_8B(((pInY[8] >> 12) | (pInY[9] << 4)) & 0x3FF);
          pOutY[3] = (uint8_t)RND_10B_TO_8B(pInY[9] >> 6);
          pInY += 10;
        }

        pInY += 5 * iCropW / sizeof(uint16_t);
      }

      pInY += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void T60A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;
  uint32_t uDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->tDim.iHeight; H += iTileH)
  {
    uint16_t* pInY = (uint16_t*)(pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma);

    int iCropH = (H + iTileH) - pDstMeta->tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutY = ((uint16_t*)pDstData) + (H + h) * uDstPitchLuma + (W + w);

          pOutY[0] = pInY[0] & 0x3FF;
          pOutY[1] = ((pInY[0] >> 10) | (pInY[1] << 6)) & 0x3FF;
          pOutY[2] = (pInY[1] >> 4) & 0x3FF;
          pOutY[3] = ((pInY[1] >> 14) | (pInY[2] << 2)) & 0x3FF;
          pOutY += uDstPitchLuma;
          pOutY[0] = ((pInY[2] >> 8) | (pInY[3] << 8)) & 0x3FF;
          pOutY[1] = (pInY[3] >> 2) & 0x3FF;
          pOutY[2] = ((pInY[3] >> 12) | (pInY[4] << 4)) & 0x3FF;
          pOutY[3] = pInY[4] >> 6;
          pOutY += uDstPitchLuma;
          pOutY[0] = pInY[5] & 0x3FF;
          pOutY[1] = ((pInY[5] >> 10) | (pInY[6] << 6)) & 0x3FF;
          pOutY[2] = (pInY[6] >> 4) & 0x3FF;
          pOutY[3] = ((pInY[6] >> 14) | (pInY[7] << 2)) & 0x3FF;
          pOutY += uDstPitchLuma;
          pOutY[0] = ((pInY[7] >> 8) | (pInY[8] << 8)) & 0x3FF;
          pOutY[1] = (pInY[8] >> 2) & 0x3FF;
          pOutY[2] = ((pInY[8] >> 12) | (pInY[9] << 4)) & 0x3FF;
          pOutY[3] = pInY[9] >> 6;
          pInY += 10;
        }

        pInY += 5 * iCropW / sizeof(uint16_t);
      }

      pInY += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void T60A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = ((uint16_t*)(pDstData + iOffsetC)) + (H + h) * iDstPitchChroma + (W + w);

          pOutC[0] = pInC[0] & 0x3FF;
          pOutC[1] = ((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF;
          pOutC[2] = (pInC[1] >> 4) & 0x3FF;
          pOutC[3] = ((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF;
          pOutC += iDstPitchChroma;
          pOutC[0] = ((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF;
          pOutC[1] = (pInC[3] >> 2) & 0x3FF;
          pOutC[2] = ((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF;
          pOutC[3] = pInC[4] >> 6;
          pOutC += iDstPitchChroma;
          pOutC[0] = pInC[5] & 0x3FF;
          pOutC[1] = ((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF;
          pOutC[2] = (pInC[6] >> 4) & 0x3FF;
          pOutC[3] = ((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF;
          pOutC += iDstPitchChroma;
          pOutC[0] = ((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF;
          pOutC[1] = (pInC[8] >> 2) & 0x3FF;
          pOutC[2] = ((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF;
          pOutC[3] = pInC[9] >> 6;
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(P010);
}

/****************************************************************************/
void T60A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->tDim.iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * iHeightC);
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = ((uint16_t*)(pDstData + iOffsetU)) + (H + h) * iDstPitchChroma + (W + w) / 2;
          uint16_t* pOutV = ((uint16_t*)(pDstData + iOffsetV)) + (H + h) * iDstPitchChroma + (W + w) / 2;

          pOutU[0] = pInC[0] & 0x3FF;
          pOutV[0] = ((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF;
          pOutU[1] = (pInC[1] >> 4) & 0x3FF;
          pOutV[1] = ((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF;
          pOutU += iDstPitchChroma;
          pOutV += iDstPitchChroma;
          pOutU[0] = ((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF;
          pOutV[0] = (pInC[3] >> 2) & 0x3FF;
          pOutU[1] = ((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF;
          pOutV[1] = pInC[4] >> 6;
          pOutU += iDstPitchChroma;
          pOutV += iDstPitchChroma;
          pOutU[0] = pInC[5] & 0x3FF;
          pOutV[0] = ((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF;
          pOutU[1] = (pInC[6] >> 4) & 0x3FF;
          pOutV[1] = ((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF;
          pOutU += iDstPitchChroma;
          pOutV += iDstPitchChroma;
          pOutU[0] = ((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF;
          pOutV[0] = (pInC[8] >> 2) & 0x3FF;
          pOutU[1] = ((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF;
          pOutV[1] = pInC[9] >> 6;
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void T60A_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 2);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV15);
}

/****************************************************************************/
void T60A_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV10);
}


/****************************************************************************/
void T62A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void T62A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y010(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void T62A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);
  uint8_t* pOutU = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);
  uint8_t* pOutV = pOutU + (pDstMeta->tPitches.iChroma * pDstMeta->tDim.iHeight);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 5)) / sizeof(uint16_t);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[0] & 0x3FF);
      pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF);
      pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[1] >> 4) & 0x3FF);
      pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF);
      pOutU += pDstMeta->tPitches.iChroma;
      pOutV += pDstMeta->tPitches.iChroma;
      pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF);
      pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[3] >> 2) & 0x3FF);
      pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF);
      pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[4] >> 6);
      pOutU += pDstMeta->tPitches.iChroma;
      pOutV += pDstMeta->tPitches.iChroma;
      pOutU[0] = (uint8_t)RND_10B_TO_8B(pInC[5] & 0x3FF);
      pOutV[0] = (uint8_t)RND_10B_TO_8B(((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF);
      pOutU[1] = (uint8_t)RND_10B_TO_8B((pInC[6] >> 4) & 0x3FF);
      pOutV[1] = (uint8_t)RND_10B_TO_8B(((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF);
      pOutU += pDstMeta->tPitches.iChroma;
      pOutV += pDstMeta->tPitches.iChroma;
      pOutU[0] = (uint8_t)RND_10B_TO_8B(((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF);
      pOutV[0] = (uint8_t)RND_10B_TO_8B((pInC[8] >> 2) & 0x3FF);
      pOutU[1] = (uint8_t)RND_10B_TO_8B(((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF);
      pOutV[1] = (uint8_t)RND_10B_TO_8B(pInC[9] >> 6);
      pOutU -= 3 * pDstMeta->tPitches.iChroma - 2;
      pOutV -= 3 * pDstMeta->tPitches.iChroma - 2;
      pInC += 10;
    }

    pOutU += pDstMeta->tPitches.iChroma * 4 - (pDstMeta->tDim.iWidth / 2);
    pOutV += pDstMeta->tPitches.iChroma * 4 - (pDstMeta->tDim.iWidth / 2);
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(I422);
}

/****************************************************************************/
void T62A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);
  uint8_t* pOutC = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 5)) / sizeof(uint16_t);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      pOutC[0] = (uint8_t)RND_10B_TO_8B(pInC[0] & 0x3FF);
      pOutC[1] = (uint8_t)RND_10B_TO_8B(((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF);
      pOutC[2] = (uint8_t)RND_10B_TO_8B((pInC[1] >> 4) & 0x3FF);
      pOutC[3] = (uint8_t)RND_10B_TO_8B(((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF);
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = (uint8_t)RND_10B_TO_8B(((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF);
      pOutC[1] = (uint8_t)RND_10B_TO_8B((pInC[3] >> 2) & 0x3FF);
      pOutC[2] = (uint8_t)RND_10B_TO_8B(((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF);
      pOutC[3] = (uint8_t)RND_10B_TO_8B(pInC[4] >> 6);
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = (uint8_t)RND_10B_TO_8B(pInC[5] & 0x3FF);
      pOutC[1] = (uint8_t)RND_10B_TO_8B(((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF);
      pOutC[2] = (uint8_t)RND_10B_TO_8B((pInC[6] >> 4) & 0x3FF);
      pOutC[3] = (uint8_t)RND_10B_TO_8B(((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF);
      pOutC += pDstMeta->tPitches.iChroma;
      pOutC[0] = (uint8_t)RND_10B_TO_8B(((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF);
      pOutC[1] = (uint8_t)RND_10B_TO_8B((pInC[8] >> 2) & 0x3FF);
      pOutC[2] = (uint8_t)RND_10B_TO_8B(((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF);
      pOutC[3] = (uint8_t)RND_10B_TO_8B(pInC[9] >> 6);
      pOutC -= 3 * pDstMeta->tPitches.iChroma - 4;
      pInC += 10;
    }

    pOutC += pDstMeta->tPitches.iChroma * 4 - pDstMeta->tDim.iWidth;
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(NV16);
}

/****************************************************************************/
void T62A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * pDstMeta->tDim.iHeight);
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 5)) / sizeof(uint16_t);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      uint16_t* pOutU = ((uint16_t*)(pDstData + iOffsetU)) + h * iDstPitchChroma + w / 2;
      uint16_t* pOutV = ((uint16_t*)(pDstData + iOffsetV)) + h * iDstPitchChroma + w / 2;

      pOutU[0] = pInC[0] & 0x3FF;
      pOutV[0] = ((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF;
      pOutU[1] = (pInC[1] >> 4) & 0x3FF;
      pOutV[1] = ((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF;
      pOutU += iDstPitchChroma;
      pOutV += iDstPitchChroma;
      pOutU[0] = ((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF;
      pOutV[0] = (pInC[3] >> 2) & 0x3FF;
      pOutU[1] = ((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF;
      pOutV[1] = pInC[4] >> 6;
      pOutU += iDstPitchChroma;
      pOutV += iDstPitchChroma;
      pOutU[0] = pInC[5] & 0x3FF;
      pOutV[0] = ((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF;
      pOutU[1] = (pInC[6] >> 4) & 0x3FF;
      pOutV[1] = ((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF;
      pOutU += iDstPitchChroma;
      pOutV += iDstPitchChroma;
      pOutU[0] = ((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF;
      pOutV[0] = (pInC[8] >> 2) & 0x3FF;
      pOutU[1] = ((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF;
      pOutV[1] = pInC[9] >> 6;
      pInC += 10;
    }

    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(I2AL);
}

/****************************************************************************/
void T62A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->tDim.iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);

  int iOffsetC = (pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight);
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->tDim.iWidth * 5)) / sizeof(uint16_t);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->tDim.iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->tDim.iWidth; w += 4)
    {
      uint16_t* pOutC = ((uint16_t*)(pDstData + iOffsetC)) + h * iDstPitchChroma + w;

      pOutC[0] = pInC[0] & 0x3FF;
      pOutC[1] = ((pInC[0] >> 10) | (pInC[1] << 6)) & 0x3FF;
      pOutC[2] = (pInC[1] >> 4) & 0x3FF;
      pOutC[3] = ((pInC[1] >> 14) | (pInC[2] << 2)) & 0x3FF;
      pOutC += iDstPitchChroma;
      pOutC[0] = ((pInC[2] >> 8) | (pInC[3] << 8)) & 0x3FF;
      pOutC[1] = (pInC[3] >> 2) & 0x3FF;
      pOutC[2] = ((pInC[3] >> 12) | (pInC[4] << 4)) & 0x3FF;
      pOutC[3] = pInC[4] >> 6;
      pOutC += iDstPitchChroma;
      pOutC[0] = pInC[5] & 0x3FF;
      pOutC[1] = ((pInC[5] >> 10) | (pInC[6] << 6)) & 0x3FF;
      pOutC[2] = (pInC[6] >> 4) & 0x3FF;
      pOutC[3] = ((pInC[6] >> 14) | (pInC[7] << 2)) & 0x3FF;
      pOutC += iDstPitchChroma;
      pOutC[0] = ((pInC[7] >> 8) | (pInC[8] << 8)) & 0x3FF;
      pOutC[1] = (pInC[8] >> 2) & 0x3FF;
      pOutC[2] = ((pInC[8] >> 12) | (pInC[9] << 4)) & 0x3FF;
      pOutC[3] = pInC[9] >> 6;
      pInC += 10;
    }

    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(P210);
}

/****************************************************************************/
void T62A_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 1);

  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  pDstMeta->tFourCC = FOURCC(XV20);
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
  (void)uHrzCScale;
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  // Luma
  XV15_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstU = ((uint8_t*)pDstData) + iDstSizeY + h * pDstMeta->tPitches.iChroma;
    uint8_t* pDstV = pDstU + iDstSizeC;

    int w = pSrcMeta->tDim.iWidth / 6;

    while(w--)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read24BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 6 > 2)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
    }
    else if(pSrcMeta->tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
    }
  }

  SetFourCC(pDstMeta, FOURCC(I420), FOURCC(I422), uHrzCScale * uVrtCScale);
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
void XV15_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pSrcMeta->tPitches.iLuma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);
    uint8_t* pDstY = (uint8_t*)(pDstData + h * pDstMeta->tPitches.iLuma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
      *pDstY++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void XV15_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pSrcMeta->tPitches.iLuma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);
    uint16_t* pDstY = (uint16_t*)(pDstData + h * pDstMeta->tPitches.iLuma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void XV15_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  XV15_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / 2;
  int iSrcSizeY = pSrcMeta->tDim.iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstV = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pDstU = pDstV + iDstSizeC;

    int w = pSrcMeta->tDim.iWidth / 6;

    while(w--)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read24BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 6 > 2)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
    }
    else if(pSrcMeta->tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void XV15_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  XV15_To_I420(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void XV20_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void XV20_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void XVXX_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  XV15_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstC = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
      *pDstC++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  SetFourCC(pDstMeta, FOURCC(NV12), FOURCC(NV16), uHrzCScale * uVrtCScale);
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
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  XV15_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstC = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  SetFourCC(pDstMeta, FOURCC(P010), FOURCC(P210), uHrzCScale * uVrtCScale);
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
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  XV15_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstU = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pDstV = pDstU + iDstSizeC;

    int w = pSrcMeta->tDim.iWidth / 6;

    while(w--)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read30BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(pSrcMeta->tDim.iWidth % 6 > 2)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(pSrcMeta->tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  SetFourCC(pDstMeta, FOURCC(I0AL), FOURCC(I2AL), uHrzCScale * uVrtCScale);
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
static void NV1X_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, TFourCC tDestFourCC, bool bIsUFirst = true)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeDstY = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iCScale = uHrzCScale * uVrtCScale;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  uint8_t* pBufIn = pSrcData;
  uint8_t* pBufOut = pDstData;

  for(int iH = 0; iH < pDstMeta->tDim.iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, pDstMeta->tDim.iWidth);

    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += pDstMeta->tPitches.iLuma;
  }

  // Chroma
  uint8_t* pBufInC = pSrcData + pSrcMeta->tOffsetYC.iChroma;
  int iChromaCompSize = iSizeDstY / iCScale;
  uint8_t* pBufOutU = pDstData + iSizeDstY + (bIsUFirst ? 0 : iChromaCompSize);
  uint8_t* pBufOutV = pDstData + iSizeDstY + (bIsUFirst ? iChromaCompSize : 0);

  int iWidth = pDstMeta->tDim.iWidth / uHrzCScale;
  int iHeight = pDstMeta->tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = pBufInC[(iW << 1)];
      pBufOutV[iW] = pBufInC[(iW << 1) + 1];
    }

    pBufInC += pSrcMeta->tPitches.iChroma;
    pBufOutU += pDstMeta->tPitches.iChroma;
    pBufOutV += pDstMeta->tPitches.iChroma;
  }

  pDstMeta->tFourCC = tDestFourCC;
}

/****************************************************************************/
void NV12_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(YV12), false);
}

/****************************************************************************/
void NV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(I420));
}

/****************************************************************************/
void NV16_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 1, FOURCC(I422));
}

/****************************************************************************/
void NV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(IYUV));
}

/****************************************************************************/
static void NV1X_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y800_To_Y010(pSrc, pDst);

  // Chroma
  int iCScale = uHrzCScale * uVrtCScale;
  int iSizeDst = pDstMeta->tPitches.iLuma * pDstMeta->tDim.iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pBufIn = AL_Buffer_GetData(pSrc) + pSrcMeta->tOffsetYC.iChroma;
  uint16_t* pBufOutU = (uint16_t*)(AL_Buffer_GetData(pDst) + iSizeDst);
  uint16_t* pBufOutV = (uint16_t*)(AL_Buffer_GetData(pDst) + iSizeDst + (iSizeDst / iCScale));

  int iWidth = pDstMeta->tDim.iWidth / uHrzCScale;
  int iHeight = pDstMeta->tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = ((uint16_t)pBufIn[(iW << 1)]) << 2;
      pBufOutV[iW] = ((uint16_t)pBufIn[(iW << 1) + 1]) << 2;
    }

    pBufIn += pSrcMeta->tPitches.iChroma;
    pBufOutU += iDstPitchChroma;
    pBufOutV += iDstPitchChroma;
  }

  SetFourCC(pDstMeta, FOURCC(I0AL), FOURCC(I2AL), iCScale);
}

/****************************************************************************/
void NV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NV1X_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->tPitches.iLuma * pSrcMeta->tDim.iHeight;

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(P010);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  uint8_t* pBufIn = pSrcData + pDstMeta->tOffsetYC.iChroma;
  uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iLumaSize;

  int iWidth = 2 * pDstMeta->tDim.iWidth / uHrzCScale;
  int iHeight = pDstMeta->tDim.iHeight / uVrtCScale;

  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += pSrcMeta->tPitches.iChroma;
    pBufOut += iDstPitchChroma;
  }

  SetFourCC(pDstMeta, FOURCC(P010), FOURCC(P210), uHrzCScale * uVrtCScale);
}

/****************************************************************************/
void NV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NV1X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  // Luma
  XV15_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Chroma
  int iHeightC = pSrcMeta->tDim.iHeight / uVrtCScale;
  int iDstSizeY = pDstMeta->tDim.iHeight * pDstMeta->tPitches.iLuma;

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcC = (uint8_t*)(pSrcData + pSrcMeta->tOffsetYC.iChroma + h * pSrcMeta->tPitches.iChroma);

    int w = pSrcMeta->tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
      *pDst32 |= ((uint32_t)*pSrcC++) << 22;
      ++pDst32;
    }

    if(pSrcMeta->tDim.iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
    }
    else if(pSrcMeta->tDim.iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
    }
  }

  SetFourCC(pDstMeta, FOURCC(XV15), FOURCC(XV20), uHrzCScale * uVrtCScale);
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
void Y800_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim.iWidth = pSrcMeta->tDim.iWidth;
  pDstMeta->tDim.iHeight = pSrcMeta->tDim.iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst);

  for(int iH = 0; iH < pDstMeta->tDim.iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, pDstMeta->tDim.iWidth);
    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += pDstMeta->tPitches.iLuma;
  }
}

/*@}*/

