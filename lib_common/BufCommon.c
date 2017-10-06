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

#include <assert.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferSrcMeta.h"

/****************************************************************************/
uint32_t AL_RndUpPow2(uint32_t uVal)
{
  uint32_t uRnd = 1;

  while(uRnd < uVal)
    uRnd <<= 1;

  return uRnd;
}

/****************************************************************************/
void AL_CopyYuv(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TSrcMetaData* pSrcMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_SOURCE);
  AL_TSrcMetaData* pDstMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pDst, AL_META_TYPE_SOURCE);

  pDstMeta->iWidth = pSrcMeta->iWidth;
  pDstMeta->iHeight = pSrcMeta->iHeight;
  pDstMeta->tFourCC = pSrcMeta->tFourCC;

  assert(pDst->zSize >= pSrc->zSize);

  Rtos_Memcpy(AL_Buffer_GetData(pDst), AL_Buffer_GetData(pSrc), pSrc->zSize);
}

/****************************************************************************/
int AL_CLEAN_BUFFERS;

void AL_CleanupMemory(void* pDst, size_t uSize)
{
  if(AL_CLEAN_BUFFERS)
    Rtos_Memset(pDst, 0, uSize);
}

