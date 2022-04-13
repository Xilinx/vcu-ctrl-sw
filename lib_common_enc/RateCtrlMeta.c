/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/EncBuffersInternal.h"

static bool destroy(AL_TMetaData* pBaseMeta)
{
  AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)pBaseMeta;
  AL_Buffer_Unref(pMeta->pMVBuf);
  Rtos_Free(pMeta);
  return true;
}

static AL_TRateCtrlMetaData* create(AL_TAllocator* pAllocator, uint32_t uBufMVSize);

static AL_TMetaData* clone(AL_TMetaData* pBaseMeta)
{
  AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)pBaseMeta;
  AL_TRateCtrlMetaData* pNewMeta = create(pMeta->pMVBuf->pAllocator, AL_Buffer_GetSize(pMeta->pMVBuf));

  if(pNewMeta == NULL)
    return false;

  pNewMeta->tRateCtrlStats = pMeta->tRateCtrlStats;

  Rtos_Memcpy(AL_Buffer_GetData(pNewMeta->pMVBuf), AL_Buffer_GetData(pMeta->pMVBuf), AL_Buffer_GetSize(pMeta->pMVBuf));

  return (AL_TMetaData*)pNewMeta;
}

static AL_TRateCtrlMetaData* create(AL_TAllocator* pAllocator, uint32_t uBufMVSize)
{
  AL_TRateCtrlMetaData* pMeta = Rtos_Malloc(sizeof(AL_TRateCtrlMetaData));

  if(pMeta == NULL)
    return NULL;

  pMeta->pMVBuf = AL_Buffer_Create_And_Allocate(pAllocator, uBufMVSize, &AL_Buffer_Destroy);

  if(pMeta->pMVBuf == NULL)
  {
    Rtos_Free(pMeta);
    return NULL;
  }

  AL_Buffer_Ref(pMeta->pMVBuf);

  pMeta->tMeta.eType = AL_META_TYPE_RATECTRL;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;

  pMeta->bFilled = false;

  return pMeta;
}

AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create(AL_TAllocator* pAllocator, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec)
{
  uint32_t uSizeMV = AL_GetAllocSize_MV(tDim, uLog2MaxCuSize, eCodec) - MVBUFF_MV_OFFSET;
  return create(pAllocator, uSizeMV);
}
