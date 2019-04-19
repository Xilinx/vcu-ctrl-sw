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

#include "lib_common/BufferBufHandleMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool destroy(AL_TMetaData* pMeta)
{
  AL_TBufHandleMetaData* pBufHandleMeta = (AL_TBufHandleMetaData*)pMeta;
  Rtos_Free(pBufHandleMeta->pHandles);
  Rtos_Free(pBufHandleMeta);
  return true;
}

AL_TBufHandleMetaData* AL_BufHandleMetaData_Clone(AL_TBufHandleMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_TBufHandleMetaData* pBufHandleMeta = AL_BufHandleMetaData_Create(pMeta->maxHandles);

  if(!pBufHandleMeta)
    return NULL;

  for(int i = 0; i < pMeta->numHandles; ++i, ++pBufHandleMeta->numHandles)
    pBufHandleMeta->pHandles[i] = pMeta->pHandles[i];

  return pBufHandleMeta;
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_BufHandleMetaData_Clone((AL_TBufHandleMetaData*)pMeta);
}

AL_TBufHandleMetaData* AL_BufHandleMetaData_Create(int iMaxHandles)
{
  AL_TBufHandleMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->pHandles = Rtos_Malloc(iMaxHandles * sizeof(*(pMeta->pHandles)));

  if(!pMeta->pHandles)
  {
    Rtos_Free(pMeta);
    return NULL;
  }

  pMeta->tMeta.eType = AL_META_TYPE_BUFHANDLE;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;
  pMeta->numHandles = 0;
  pMeta->maxHandles = iMaxHandles;

  return pMeta;
}

bool AL_BufHandleMetaData_AddHandle(AL_TBufHandleMetaData* pMeta, AL_TBuffer* pBuf)
{
  if(pMeta->numHandles + 1 > pMeta->maxHandles)
    return false;

  pMeta->pHandles[pMeta->numHandles] = pBuf;
  ++pMeta->numHandles;
  return true;
}

