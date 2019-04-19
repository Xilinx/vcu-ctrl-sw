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

#include <assert.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferSrcMeta.h"
#include "Utils.h"

/*****************************************************************************/
int AL_GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode)
{
  switch(eFrameBufferStorageMode)
  {
  case AL_FB_RASTER:
    return 1;
  case AL_FB_TILE_32x4:
  case AL_FB_TILE_64x4:
    return 4;
  default:
    assert(false);
    return 0;
  }
}

/******************************************************************************/
static inline int GetWidthRound(AL_EFbStorageMode eStorageMode)
{
  switch(eStorageMode)
  {
  case AL_FB_RASTER:
    return 1;
  case AL_FB_TILE_64x4:
    return 64;
  case AL_FB_TILE_32x4:
    return 32;
  default:
    assert(false);
    return 0;
  }
}

/******************************************************************************/
int32_t ComputeRndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode, int iBurstAlignment)
{
  int32_t iVal = 0;
  int iRndWidth = RoundUp(iWidth, GetWidthRound(eFrameBufferStorageMode));
  switch(eFrameBufferStorageMode)
  {
  case AL_FB_RASTER:
  {
    if(uBitDepth == 8)
      iVal = iRndWidth;
    else
    {
      iVal = (iRndWidth + 2) / 3 * 4;
    }
    break;
  }
  case AL_FB_TILE_32x4:
  case AL_FB_TILE_64x4:
  {
    int const uDepth = uBitDepth > 8 ? 10 : 8;
    iVal = iRndWidth * AL_GetNumLinesInPitch(eFrameBufferStorageMode) * uDepth / 8;
    break;
  }
  default:
    assert(false);
  }

  assert(iBurstAlignment > 0 && (iBurstAlignment % HW_IP_BURST_ALIGNMENT) == 0);
  return RoundUp(iVal, iBurstAlignment);
}

/****************************************************************************/
void AL_CopyYuv(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->tDim = pSrcMeta->tDim;
  pDstMeta->tFourCC = pSrcMeta->tFourCC;

  assert(pDst->zSize >= pSrc->zSize);

  Rtos_Memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), pSrc->zSize);
}

/****************************************************************************/
int AL_CLEAN_BUFFERS = 0;

void AL_CleanupMemory(void* pDst, size_t uSize)
{
  if(AL_CLEAN_BUFFERS)
    Rtos_Memset(pDst, 0, uSize);
}

