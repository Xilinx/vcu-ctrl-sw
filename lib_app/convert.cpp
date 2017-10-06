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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include <cstring>
#include <cassert>

extern "C" {
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferAPI.h"
}

#include "convert.h"

#define RND_10B_TO_8B(val) (((val) >= 0x3FC) ? 0xFF : (((val) + 2) >> 2))

/****************************************************************************/
void I420_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  AL_CopyYuv(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void I420_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(YV12);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);

  // luma
  memcpy(pDstData, pSrcData, iSize);
  // Chroma
  memcpy(pDstData + iSize, pSrcData + iSize + (iSize >> 2), iSize >> 2);
  memcpy(pDstData + iSize + (iSize >> 2), pSrcData + iSize, iSize >> 2);
}

/****************************************************************************/
void I420_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  assert(pSrcMeta);
  assert(pDstMeta);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // Luma
  AL_VADDR pSrcY = AL_Buffer_GetData(pSrc);
  AL_VADDR pDstY = AL_Buffer_GetData(pDst);

  for(int iH = 0; iH < pSrcMeta->iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, pSrcMeta->iWidth);
    pDstY += pDstMeta->tPitches.iLuma;
    pSrcY += pSrcMeta->iWidth;
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void I420_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  I420_To_Y800(pSrc, pDst);

  // Chroma
  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  AL_VADDR pBufInU = pSrcData + iSize;
  AL_VADDR pBufInV = pSrcData + iSize + (iSize >> 2);
  AL_VADDR pBufOut = pDstData + (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);

  int iHeightC = pSrcMeta->iHeight >> 1;
  int iWidthC = pSrcMeta->iWidth >> 1;

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

  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void I420_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(Y010);

  // Luma
  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst));
  int iSize = iLumaSize;

  while(iSize--)
    *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
}

/****************************************************************************/
void I420_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 2;

  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pBufInU = pSrcData + iLumaSize;
  uint8_t* pBufInV = pSrcData + iLumaSize + iChromaSize;
  uint16_t* pBufOut = ((uint16_t*)(AL_Buffer_GetData(pDst))) + iLumaSize;
  int iSize = iChromaSize;

  while(iSize--)
  {
    *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
    *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
  }

  pDstMeta->tFourCC = FOURCC(P010);
}

/****************************************************************************/
void I420_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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
void I420_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y800_To_RX0A(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma > (pDstMeta->iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iDstSizeY = pSrcMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iSrcSizeY = pDstMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcU = (uint8_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pSrcV = pSrcU + iSrcSizeC;

    int w = pSrcMeta->iWidth / 6;

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

    if(pSrcMeta->iWidth / 6 > 2)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(pSrcMeta->iWidth / 6 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
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
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_NV12(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_P010(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_I0AL(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_RX0A(pSrc, pDst);
}

/****************************************************************************/
void YV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iLumaSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);
  int iChromaSize = iLumaSize >> 2;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(I420);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // luma
  memcpy(pDstData, pSrcData, iLumaSize);
  // Chroma
  memcpy(pDstData + iLumaSize, pSrcData + iLumaSize + iChromaSize, iChromaSize);
  memcpy(pDstData + iLumaSize + iChromaSize, pSrcData + iLumaSize, iChromaSize);
}

/****************************************************************************/
void YV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  YV12_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void YV12_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(NV12);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  memcpy(pDstData, pSrcData, iLumaSize);

  // Chroma
  {
    uint8_t* pBufInV = pSrcData + iLumaSize;
    uint8_t* pBufInU = pSrcData + iLumaSize + iChromaSize;
    uint8_t* pBufOut = pDstData + iLumaSize;

    int iSize = iChromaSize;

    while(iSize--)
    {
      *pBufOut++ = *pBufInU++;
      *pBufOut++ = *pBufInV++;
    }
  }
}

/****************************************************************************/
void YV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), iSize);
}

/****************************************************************************/
void YV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(P010);

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
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iLumaSize;
    int iSize = iChromaSize;

    while(iSize--)
    {
      *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
      *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
    }
  }
}

/****************************************************************************/
void YV12_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y800_To_RX0A(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma > (pDstMeta->iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iDstSizeY = pSrcMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iSrcSizeY = pDstMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcV = (uint8_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pSrcU = pSrcV + iSrcSizeC;

    int w = pSrcMeta->iWidth / 6;

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

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
void YV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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
void NV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->iWidth = pSrcMeta->iWidth;

  int iSizeSrcY = pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight;
  int iSizeDstY = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  uint8_t* pBufIn = pSrcData;
  uint8_t* pBufOut = pDstData;

  // Luma
  for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, pDstMeta->iWidth);

    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += pDstMeta->tPitches.iLuma;
  }

  // Chroma
  uint8_t* pBufInC = pSrcData + iSizeSrcY;
  uint8_t* pBufOutU = pDstData + iSizeDstY;
  uint8_t* pBufOutV = pDstData + iSizeDstY + (iSizeDstY >> 2);

  int iWidth = pDstMeta->iWidth >> 1;
  int iHeight = pDstMeta->iHeight >> 1;

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
}

/****************************************************************************/
void NV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  NV12_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void NV12_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(YV12);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  memcpy(pDstData, pSrcData, iSize);

  // Chroma
  {
    uint8_t* pBufIn = pSrcData + iSize;
    uint8_t* pBufOutV = pDstData + iSize;
    uint8_t* pBufOutU = pDstData + iSize + (iSize >> 2);

    iSize >>= 2;

    while(iSize--)
    {
      *pBufOutU++ = *pBufIn++;
      *pBufOutV++ = *pBufIn++;
    }
  }
}

/****************************************************************************/
void NV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), iSize);
}

/****************************************************************************/
void NV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(P010);

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
void NV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // pDstMeta->iWidth  = pSrcMeta->iWidth;
  // pDstMeta->iHeight = pSrcMeta->iHeight;
  // pDstMeta->tFourCC = FOURCC(I0AL);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  {
    uint8_t* pBufIn = pSrcData;
    uint16_t* pBufOut = (uint16_t*)(pDstData);

    for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
    {
      for(int iW = 0; iW < pDstMeta->iWidth; ++iW)
        pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

      pBufIn += pSrcMeta->tPitches.iLuma;
      pBufOut += pDstMeta->tPitches.iLuma;
    }
  }

  // Chroma
  {
    int iSizeSrc = pSrcMeta->iWidth * pSrcMeta->iHeight;
    int iSizeDst = pDstMeta->iWidth * pDstMeta->iHeight;

    uint8_t* pBufIn = pSrcData + iSizeSrc;
    uint16_t* pBufOutU = ((uint16_t*)(pDstData)) + iSizeDst;
    uint16_t* pBufOutV = ((uint16_t*)(pDstData)) + iSizeDst + (iSizeDst >> 2);

    int iWidth = pDstMeta->iWidth >> 1;
    int iHeight = pDstMeta->iHeight >> 1;

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = ((uint16_t)pBufIn[(iW << 1)]) << 2;
        pBufOutV[iW] = ((uint16_t)pBufIn[(iW << 1) + 1]) << 2;
      }

      pBufIn += pSrcMeta->tPitches.iChroma;
      pBufOutU += pDstMeta->tPitches.iChroma;
      pBufOutV += pDstMeta->tPitches.iChroma;
    }
  }
}

/****************************************************************************/
void NV12_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcC = (uint8_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
      *pDst32 |= ((uint32_t)*pSrcC++) << 22;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
void Y800_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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
void Y800_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma > (pDstMeta->iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
      *pDst32 |= ((uint32_t)*pSrcY++) << 22;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
void Y800_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(Y010);

  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst));
  int iDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
  {
    for(int iW = 0; iW < pDstMeta->iWidth; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += iDstPitchLuma;
  }
}

/****************************************************************************/
void Y010_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma > (pDstMeta->iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint16_t* pSrcY = (uint16_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcY++);
      *pDst32 |= ((uint32_t)*pSrcY++) << 10;
      *pDst32 |= ((uint32_t)*pSrcY++) << 20;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcY++);
      *pDst32 |= ((uint32_t)*pSrcY++) << 10;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
      *pDst32 = ((uint32_t)*pSrcY++);
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
static
uint32_t toTen(uint8_t sample)
{
  return (((uint32_t)sample) << 2) & 0x3FF;
}

/****************************************************************************/
void Y800_To_RX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
      *pDst32 |= toTen(*pSrcY++) << 20;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = toTen(*pSrcY++);
    }
  }

  pDstMeta->tFourCC = FOURCC(RX10);
}

/****************************************************************************/
void Y010_To_RX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pDstMeta->tPitches.iLuma % 4 == 0);
  assert(pDstMeta->tPitches.iLuma >= (pDstMeta->iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * pDstMeta->tPitches.iLuma);
    uint16_t* pSrcY = (uint16_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)(*pSrcY++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcY++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcY++) & 0x3FF) << 20;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)(*pSrcY++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcY++) & 0x3FF) << 10;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)(*pSrcY++) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(RX10);
}

/****************************************************************************/
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeSrcY = pSrcMeta->iWidth * pSrcMeta->iHeight;
  int iSizeDstY = pDstMeta->iWidth * pDstMeta->iHeight;

  pDstMeta->tFourCC = FOURCC(I420);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)pSrcData;
    uint8_t* pBufOut = pDstData;
    uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);

    for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
    {
      for(int iW = 0; iW < pDstMeta->iWidth; ++iW)
        pBufOut[iW] = (uint8_t)((2 + pBufIn[iW]) >> 2);

      pBufIn += uSrcPitchLuma;
      pBufOut += pDstMeta->tPitches.iLuma;
    }
  }
  // Chroma
  {
    uint16_t* pBufInC = ((uint16_t*)pSrcData) + iSizeSrcY;
    uint8_t* pBufOutU = pDstData + iSizeDstY;
    uint8_t* pBufOutV = pDstData + iSizeDstY + (iSizeDstY >> 2);
    uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

    int iWidth = pDstMeta->iWidth >> 1;
    int iHeight = pDstMeta->iHeight >> 1;

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
}

/****************************************************************************/
void P010_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  P010_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void P010_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 2;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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

  const int iLumaSize = pSrcMeta->iWidth * pSrcMeta->iHeight;
  const int iChromaSize = iLumaSize >> 1;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
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
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  P010_To_Y010(pSrc, pDst);

  // Chroma
  int iSizeSrc = pSrcMeta->iWidth * pSrcMeta->iHeight;
  int iSizeDst = pDstMeta->iWidth * pDstMeta->iHeight;

  uint16_t* pBufIn = ((uint16_t*)AL_Buffer_GetData(pSrc)) + iSizeSrc;
  uint16_t* pBufOutU = ((uint16_t*)AL_Buffer_GetData(pDst)) + iSizeDst;
  uint16_t* pBufOutV = ((uint16_t*)AL_Buffer_GetData(pDst)) + iSizeDst + (iSizeDst >> 2);

  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iWidth = pDstMeta->iWidth >> 1;
  int iHeight = pDstMeta->iHeight >> 1;

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

  pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void P010_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  P010_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pSrcC = (uint16_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++);
      *pDst32 |= ((uint32_t)*pSrcC++) << 10;
      *pDst32 |= ((uint32_t)*pSrcC++) << 20;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++);
      *pDst32 |= ((uint32_t)*pSrcC++) << 10;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++);
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
void I0AL_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // Luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  uint16_t* pBufIn = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst) + pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  int iH = pSrcMeta->iHeight;

  while(iH--)
  {
    int iW = pSrcMeta->iWidth >> 1;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += pDstMeta->tPitches.iChroma - (pSrcMeta->iWidth >> 1);
    pBufIn += uSrcPitchChroma - (pSrcMeta->iWidth >> 1);
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
void I0AL_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // Luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  uint16_t* pBufIn = (uint16_t*)(pSrcData + pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight + pSrcMeta->tPitches.iChroma * (pSrcMeta->iHeight >> 1));
  uint8_t* pBufOut = pDstData + pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight;
  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  int iH = pSrcMeta->iHeight >> 1;

  while(iH--)
  {
    int iW = pSrcMeta->iWidth >> 1;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += pDstMeta->tPitches.iChroma - (pSrcMeta->iWidth >> 1);
    pBufIn += uSrcPitchChroma - (pSrcMeta->iWidth >> 1);
  }

  pBufIn = (uint16_t*)(pSrcData + pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight);

  while(iH--)
  {
    int iW = pSrcMeta->iWidth >> 1;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += pDstMeta->tPitches.iChroma - (pSrcMeta->iWidth >> 1);
    pBufIn += uSrcPitchChroma - (pSrcMeta->iWidth >> 1);
  }

  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void I0AL_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_Buffer_GetData(pSrc);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst);
  uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);

  int iH = pSrcMeta->iHeight;

  while(iH--)
  {
    int iW = pSrcMeta->iWidth;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += uSrcPitchLuma - pSrcMeta->iWidth;
    pBufIn += uSrcPitchLuma - pSrcMeta->iWidth;
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void I0AL_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  uint16_t* pBufInU = (uint16_t*)(AL_Buffer_GetData(pSrc) + pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight);
  uint16_t* pBufInV = pBufInU + pSrcMeta->tPitches.iChroma / sizeof(uint16_t) * (pSrcMeta->iHeight >> 1);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst) + pDstMeta->tPitches.iLuma * pDstMeta->iHeight;

  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);

  int iH = pSrcMeta->iHeight >> 1;

  while(iH--)
  {
    int iW = pSrcMeta->iWidth >> 1;

    while(iW--)
    {
      *pBufOut++ = (uint8_t)((2 + *pBufInU++) >> 2);
      *pBufOut++ = (uint8_t)((2 + *pBufInV++) >> 2);
    }

    pBufOut += pDstMeta->tPitches.iChroma - pDstMeta->iWidth;
    pBufInU += uSrcPitchChroma - (pSrcMeta->iWidth >> 1);
    pBufInV += uSrcPitchChroma - (pSrcMeta->iWidth >> 1);
  }

  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_Buffer_GetData(pSrc);
  uint16_t* pBufOut = (uint16_t*)AL_Buffer_GetData(pDst);
  uint32_t uSrcPitchLuma = pSrcMeta->tPitches.iLuma / sizeof(uint16_t);
  uint32_t uDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  int iH = pSrcMeta->iHeight;

  while(iH--)
  {
    memcpy(pBufOut, pBufIn, pSrcMeta->iWidth * sizeof(uint16_t));
    pBufOut += uDstPitchLuma;
    pBufIn += uSrcPitchLuma;
  }

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeDstY = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iSizeSrcY = pSrcMeta->tPitches.iLuma * pSrcMeta->iHeight;

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;

  // Luma
  I0AL_To_Y010(pSrc, pDst);

  // Chroma
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint16_t* pBufInU = (uint16_t*)(pSrcData + iSizeSrcY);
  uint16_t* pBufInV = (uint16_t*)(pSrcData + iSizeSrcY + (iSizeSrcY >> 2));
  uint16_t* pBufOut = (uint16_t*)(AL_Buffer_GetData(pDst) + iSizeDstY);

  uint32_t uSrcPitchChroma = pSrcMeta->tPitches.iChroma / sizeof(uint16_t);
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  for(int iH = 0; iH < pSrcMeta->iHeight; iH += 2)
  {
    for(int iW = 0; iW < pSrcMeta->iWidth; iW += 2)
    {
      pBufOut[iW] = pBufInU[iW >> 1];
      pBufOut[iW + 1] = pBufInV[iW >> 1];
    }

    pBufInU += uSrcPitchChroma;
    pBufInV += uSrcPitchChroma;
    pBufOut += uDstPitchChroma;
  }

  pDstMeta->tFourCC = FOURCC(P010);
}

/****************************************************************************/
void I0AL_To_RX0A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y010_To_RX10(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma >= (pDstMeta->iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iDstSizeY = pSrcMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iSrcSizeY = pDstMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pSrcU = (uint16_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pSrcV = (uint16_t*)(pSrcData + iSrcSizeY + iSrcSizeC + h * pSrcMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 6;

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

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX0A);
}

/****************************************************************************/
void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSize = (pSrcMeta->iWidth * pSrcMeta->iHeight);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(NV16);

  // Luma
  AL_VADDR pSrcY = pSrcData;
  AL_VADDR pDstY = pDstData;

  for(int iH = 0; iH < pSrcMeta->iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, pSrcMeta->iWidth);
    pDstY += pDstMeta->tPitches.iLuma;
    pSrcY += pSrcMeta->iWidth;
  }

  // Chroma
  AL_VADDR pBufInU = pSrcData + iSize;
  AL_VADDR pBufInV = pSrcData + iSize + (iSize >> 1);
  AL_VADDR pBufOut = pDstData + (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);

  int iHeightC = pSrcMeta->iHeight;
  int iWidthC = pSrcMeta->iWidth >> 1;

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
}

/****************************************************************************/
void I422_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(P210);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint8_t* pBufIn = pSrcData;
    uint16_t* pBufOut = (uint16_t*)(pDstData);

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }
  // Chroma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint8_t* pBufInU = pSrcData + iSize;
    uint8_t* pBufInV = pSrcData + iSize + (iSize >> 1);
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iSize;

    iSize >>= 1;

    while(iSize--)
    {
      *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
      *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
    }
  }
}

/****************************************************************************/
void I422_To_RX2A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y800_To_RX0A(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma >= (pDstMeta->iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iDstSizeY = pSrcMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iSrcSizeY = pDstMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcU = (uint8_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pSrcV = pSrcU + iSrcSizeC;

    int w = pSrcMeta->iWidth / 6;

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

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX2A);
}

/****************************************************************************/
void NV16_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(P010);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint8_t* pBufIn = pSrcData;
    uint16_t* pBufOut = (uint16_t*)(pDstData);

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }
  // Chroma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint8_t* pBufIn = pSrcData + iSize;
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iSize;

    while(iSize--)
      *pBufOut++ = ((uint16_t)*pBufIn++) << 2;
  }
}

/****************************************************************************/
void NV16_To_RX2A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pSrcC = (uint8_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
      *pDst32 |= ((uint32_t)*pSrcC++) << 22;
      ++pDst32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
    }
  }

  pDstMeta->tFourCC = FOURCC(RX2A);
}

/****************************************************************************/
void I2AL_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(NV16);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint16_t* pBufIn = (uint16_t*)(pSrcData);
    uint8_t* pBufOut = pDstData;

    while(iSize--)
      *pBufOut++ = (uint8_t)(*pBufIn++ >> 2);
  }
  // Chroma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint16_t* pBufInU = ((uint16_t*)(pSrcData)) + iSize;
    uint16_t* pBufInV = ((uint16_t*)(pSrcData)) + iSize + (iSize >> 1);
    uint8_t* pBufOut = pDstData + iSize;

    iSize >>= 1;

    while(iSize--)
    {
      *pBufOut++ = (uint8_t)(*pBufInU++ >> 2);
      *pBufOut++ = (uint8_t)(*pBufInV++ >> 2);
    }
  }
}

/****************************************************************************/
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(P210);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  // Luma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint16_t* pBufIn = (uint16_t*)(pSrcData);
    uint16_t* pBufOut = (uint16_t*)(pDstData);

    while(iSize--)
      *pBufOut++ = *pBufIn++;
  }
  // Chroma
  {
    int iSize = pSrcMeta->iWidth * pSrcMeta->iHeight;

    uint16_t* pBufInU = ((uint16_t*)(pSrcData)) + iSize;
    uint16_t* pBufInV = ((uint16_t*)(pSrcData)) + iSize + (iSize >> 1);
    uint16_t* pBufOut = ((uint16_t*)(pDstData)) + iSize;

    iSize >>= 1;

    while(iSize--)
    {
      *pBufOut++ = *pBufInU++;
      *pBufOut++ = *pBufInV++;
    }
  }
}

/****************************************************************************/
void I2AL_To_RX2A(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  Y010_To_RX10(pSrc, pDst);

  assert(pDstMeta->tPitches.iChroma % 4 == 0);
  assert(pDstMeta->tPitches.iChroma >= (pDstMeta->iWidth + 2) / 3 * 4);

  // Chroma
  {
    int iHeightC = pSrcMeta->iHeight;
    int iDstSizeY = pSrcMeta->iHeight * pDstMeta->tPitches.iLuma;
    int iSrcSizeY = pDstMeta->iHeight * pSrcMeta->tPitches.iLuma;
    int iSrcSizeC = iHeightC * pSrcMeta->tPitches.iChroma;

    uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
    uint8_t* pDstData = AL_Buffer_GetData(pDst);

    for(int h = 0; h < iHeightC; h++)
    {
      uint32_t* pDst32 = (uint32_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
      uint16_t* pSrcU = (uint16_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
      uint16_t* pSrcV = (uint16_t*)(pSrcData + iSrcSizeY + iSrcSizeC + h * pSrcMeta->tPitches.iChroma);

      int w = pSrcMeta->iWidth / 6;

      while(w--)
      {
        *pDst32 = ((uint32_t)*pSrcU++);
        *pDst32 |= ((uint32_t)*pSrcV++) << 10;
        *pDst32 |= ((uint32_t)*pSrcU++) << 20;
        ++pDst32;
        *pDst32 = ((uint32_t)*pSrcV++);
        *pDst32 |= ((uint32_t)*pSrcU++) << 10;
        *pDst32 |= ((uint32_t)*pSrcV++) << 20;
        ++pDst32;
      }

      if(pSrcMeta->iWidth % 6 > 2)
      {
        *pDst32 = ((uint32_t)*pSrcU++);
        *pDst32 |= ((uint32_t)*pSrcV++) << 10;
        *pDst32 |= ((uint32_t)*pSrcU++) << 20;
        ++pDst32;
        *pDst32 = ((uint32_t)*pSrcV++);
      }
      else if(pSrcMeta->iWidth % 6 > 0)
      {
        *pDst32 = ((uint32_t)*pSrcU++);
        *pDst32 |= ((uint32_t)*pSrcV++) << 10;
      }
    }
  }
  pDstMeta->tFourCC = FOURCC(RX2A);
}


void ALX8_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHeightC)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL08_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pDstMeta->iHeight >> 1;
  ALX8_To_I42X(pSrc, pDst, iHeightC);
  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void AL08_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  AL08_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void AL08_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;

  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetV = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iOffsetU = iOffsetV + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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

  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void AL08_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL08_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma;

    int iCropH = (H + iTileH) - pDstMeta->iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL08_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  int iDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma;

    int iCropH = (H + iTileH) - pDstMeta->iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
}

/****************************************************************************/
void AL08_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL08_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
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

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void ALm8_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeY = (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);
  int iSizeC = (pDstMeta->tPitches.iChroma * pDstMeta->iHeight / 2);

  // Luma
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  Rtos_Memset(AL_Buffer_GetData(pDst) + iSizeY, 0x80, 2 * iSizeC);

  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void AL28_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y800(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void AL28_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y010(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void AL28_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  int iHeightC = pDstMeta->iHeight;
  ALX8_To_I42X(pSrc, pDst, iHeightC);

  pDstMeta->tFourCC = FOURCC(I422);
}

/****************************************************************************/
void AL28_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y800(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + iSrcLumaSize;
  uint8_t* pOutC = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 4);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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

    pOutC += pDstMeta->tPitches.iChroma * 4 - pDstMeta->iWidth / 2;
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(NV16);
}

/****************************************************************************/
void AL28_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y010(pSrc, pDst);

  // Chroma
  int iOffsetU = pDstMeta->tPitches.iLuma * pSrcMeta->iHeight;
  int iOffsetV = iOffsetU + pDstMeta->tPitches.iChroma * pSrcMeta->iHeight;
  int uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + iSrcLumaSize;

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 4);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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
void AL28_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL08_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint8_t* pInC = AL_Buffer_GetData(pSrc) + iSrcLumaSize;
  uint32_t uDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 4);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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
void AL0A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  AL0A_To_I420(pSrc, pDst);
  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void AL0A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetV = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iOffsetU = iOffsetV + (pDstMeta->tPitches.iChroma * iHeightC);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->iHeight; H += iTileH)
  {
    uint16_t* pInY = (uint16_t*)(pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma);

    int iCropH = (H + iTileH) - pDstMeta->iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;
  uint32_t uDstPitchLuma = pDstMeta->tPitches.iLuma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < pDstMeta->iHeight; H += iTileH)
  {
    uint16_t* pInY = (uint16_t*)(pSrcData + (H / iTileH) * pSrcMeta->tPitches.iLuma);

    int iCropH = (H + iTileH) - pDstMeta->iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetC = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + iSrcLumaSize + (H / iTileH) * pSrcMeta->tPitches.iChroma);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL0A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  int iHeightC = pDstMeta->iHeight >> 1;

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
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

    for(int W = 0; W < pDstMeta->iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - pDstMeta->iWidth;

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
void AL2A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void AL2A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y010(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void AL2A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);
  uint8_t* pOutU = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);
  uint8_t* pOutV = pOutU + (pDstMeta->tPitches.iChroma * pDstMeta->iHeight);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 5)) / sizeof(uint16_t);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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

    pOutU += pDstMeta->tPitches.iChroma * 4 - (pDstMeta->iWidth / 2);
    pOutV += pDstMeta->tPitches.iChroma * 4 - (pDstMeta->iWidth / 2);
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(I422);
}

/****************************************************************************/
void AL2A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y800(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);
  uint8_t* pOutC = AL_Buffer_GetData(pDst) + (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 5)) / sizeof(uint16_t);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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

    pOutC += pDstMeta->tPitches.iChroma * 4 - pDstMeta->iWidth;
    pInC += iJump;
  }

  pDstMeta->tFourCC = FOURCC(NV16);
}

/****************************************************************************/
void AL2A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);

  int iOffsetU = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iOffsetV = iOffsetU + (pDstMeta->tPitches.iChroma * pDstMeta->iHeight);
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 5)) / sizeof(uint16_t);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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
void AL2A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  AL0A_To_Y010(pSrc, pDst);

  // Chroma
  const int iSrcLumaSize = (((pSrcMeta->iHeight + 63) & ~63) >> 2) * pSrcMeta->tPitches.iLuma;
  uint16_t* pInC = (uint16_t*)(AL_Buffer_GetData(pSrc) + iSrcLumaSize);

  int iOffsetC = (pDstMeta->tPitches.iLuma * pDstMeta->iHeight);
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  int iJump = (pSrcMeta->tPitches.iChroma - (pDstMeta->iWidth * 5)) / sizeof(uint16_t);

  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pDstMeta->iHeight; h += 4)
  {
    for(int w = 0; w < pDstMeta->iWidth; w += 4)
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
void RX0A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pSrcMeta->tPitches.iLuma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);
    uint8_t* pDstY = (uint8_t*)(pDstData + h * pDstMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
      *pDstY++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(Y800);
}

/****************************************************************************/
void RX0A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  assert(pSrcMeta->tPitches.iLuma % 4 == 0);

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < pSrcMeta->iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * pSrcMeta->tPitches.iLuma);
    uint16_t* pDstY = (uint16_t*)(pDstData + h * pDstMeta->tPitches.iLuma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(Y010);
}

/****************************************************************************/
void RX0A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstV = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pDstU = pDstV + iDstSizeC;

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
      *pDstU++ = (*pSrc32 >> 12) & 0xFF;
      *pDstV++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(YV12);
}

/****************************************************************************/
void RX0A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstU = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pDstV = pDstU + iDstSizeC;

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
      *pDstU++ = (*pSrc32 >> 12) & 0xFF;
      *pDstV++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(I420);
}

/****************************************************************************/
void RX0A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  RX0A_To_I420(pSrc, pDst);

  pDstMeta->tFourCC = FOURCC(IYUV);
}

/****************************************************************************/
void RX0A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstC = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
      *pDstC++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(NV12);
}

/****************************************************************************/
void RX0A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstC = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(P010);
}

/****************************************************************************/
void RX0A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / 2;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstU = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pDstV = pDstU + iDstSizeC;

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void RX2A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  RX0A_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void RX2A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  RX0A_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void RX2A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX2A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstU = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint8_t* pDstV = (uint8_t*)(pDstU + iDstSizeC);

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
      *pDstU++ = (*pSrc32 >> 12) & 0xFF;
      *pDstV++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
      *pDstU++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(I422);
}

/****************************************************************************/
void RX2A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX2A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstC = (uint8_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
      *pDstC++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  pDstMeta->tFourCC = FOURCC(NV16);
}

/****************************************************************************/
void RX2A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX2A_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstU = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pDstV = (uint16_t*)(pDstData + iDstSizeY + iDstSizeC + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(I2AL);
}

/****************************************************************************/
void RX2A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight;
  int iSrcSizeY = pSrcMeta->iHeight * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstC = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 3;

    while(w--)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 3 > 1)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 3 > 0)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  pDstMeta->tFourCC = FOURCC(P210);
}

/****************************************************************************/
void NV1X_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  int iSizeSrcY = pSrcMeta->tPitches.iLuma * ((pSrcMeta->iHeight + 63) & ~63);
  int iSizeDstY = pDstMeta->iWidth * pDstMeta->iHeight;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  // Luma
  uint8_t* pBufIn = pSrcData;
  uint8_t* pBufOut = pDstData;

  for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, pDstMeta->iWidth);

    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += pDstMeta->tPitches.iLuma;
  }

  // Chroma
  uint8_t* pBufInC = pSrcData + iSizeSrcY;
  uint8_t* pBufOutU = pDstData + iSizeDstY;
  uint8_t* pBufOutV = pDstData + iSizeDstY + (iSizeDstY / (uHrzCScale * uVrtCScale));

  int iWidth = pDstMeta->iWidth / uHrzCScale;
  int iHeight = pDstMeta->iHeight / uVrtCScale;

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
}

/****************************************************************************/
void RX0A_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  (void)uHrzCScale;
  (void)uVrtCScale;
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // Luma
  RX0A_To_Y010(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / uVrtCScale;
  int iSrcSizeY = ((pSrcMeta->iHeight + 63) & ~63) * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pDstData = AL_Buffer_GetData(pDst);
  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint16_t* pDstU = (uint16_t*)(pDstData + iDstSizeY + h * pDstMeta->tPitches.iChroma);
    uint16_t* pDstV = (uint16_t*)(pDstData + iDstSizeY + iDstSizeC + h * pDstMeta->tPitches.iChroma);

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstU++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  // pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void NV1X_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  // pDstMeta->iWidth  = pSrcMeta->iWidth;
  // pDstMeta->iHeight = pSrcMeta->iHeight;
  // pDstMeta->tFourCC = FOURCC(I0AL);

  // Luma
  Y800_To_Y010(pSrc, pDst);

  // Chroma
  int iSizeSrc = pSrcMeta->tPitches.iLuma * ((pSrcMeta->iHeight + 63) & ~63);
  int iSizeDst = pDstMeta->tPitches.iLuma * pDstMeta->iHeight;
  int iDstPitchChroma = pDstMeta->tPitches.iChroma / sizeof(uint16_t);

  uint8_t* pBufIn = AL_Buffer_GetData(pSrc) + iSizeSrc;
  uint16_t* pBufOutU = (uint16_t*)(AL_Buffer_GetData(pDst) + iSizeDst);
  uint16_t* pBufOutV = (uint16_t*)(AL_Buffer_GetData(pDst) + iSizeDst + (iSizeDst / (uHrzCScale * uVrtCScale)));

  int iWidth = pDstMeta->iWidth / uHrzCScale;
  int iHeight = pDstMeta->iHeight / uVrtCScale;

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
}

/****************************************************************************/
void RX0A_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  (void)uHrzCScale;
  (void)uVrtCScale;
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);
  // Luma
  RX0A_To_Y800(pSrc, pDst);

  assert(pSrcMeta->tPitches.iChroma % 4 == 0);

  // Chroma
  int iHeightC = pSrcMeta->iHeight / uVrtCScale;
  int iSrcSizeY = ((pSrcMeta->iHeight + 63) & ~63) * pSrcMeta->tPitches.iLuma;
  int iDstSizeY = pDstMeta->iHeight * pDstMeta->tPitches.iLuma;
  int iDstSizeC = iHeightC * pDstMeta->tPitches.iChroma;

  uint8_t* pSrcData = AL_Buffer_GetData(pSrc);
  uint8_t* pDstData = AL_Buffer_GetData(pDst);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + iSrcSizeY + h * pSrcMeta->tPitches.iChroma);
    uint8_t* pDstU = ((uint8_t*)pDstData) + iDstSizeY + h * pDstMeta->tPitches.iChroma;
    uint8_t* pDstV = pDstU + iDstSizeC;

    int w = pSrcMeta->iWidth / 6;

    while(w--)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
      *pDstU++ = (uint8_t)((*pSrc32 >> 22) & 0xFF);
      ++pSrc32;
      *pDstV++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstU++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 22) & 0xFF);
      ++pSrc32;
    }

    if(pSrcMeta->iWidth % 6 > 2)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
      *pDstU++ = (uint8_t)((*pSrc32 >> 22) & 0xFF);
      ++pSrc32;
      *pDstV++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
    }
    else if(pSrcMeta->iWidth % 6 > 0)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
    }
  }

  // pDstMeta->tFourCC = FOURCC(I0AL);
}

/****************************************************************************/
void Y800_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = FOURCC(Y800);

  // Luma
  uint8_t* pBufIn = AL_Buffer_GetData(pSrc);
  uint8_t* pBufOut = AL_Buffer_GetData(pDst);

  for(int iH = 0; iH < pDstMeta->iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, pDstMeta->iWidth);
    pBufIn += pSrcMeta->tPitches.iLuma;
    pBufOut += pDstMeta->tPitches.iLuma;
  }
}

/*@}*/

