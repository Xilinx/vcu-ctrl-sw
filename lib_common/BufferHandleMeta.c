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

#include "lib_common/BufferHandleMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool destroy(AL_TMetaData* pMeta)
{
  AL_THandleMetaData* pHandleMeta = (AL_THandleMetaData*)pMeta;
  Rtos_Free(pHandleMeta->pHandles);
  Rtos_Free(pHandleMeta);
  return true;
}

AL_THandleMetaData* AL_HandleMetaData_Clone(AL_THandleMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_THandleMetaData* pHandleMeta = AL_HandleMetaData_Create(pMeta->maxHandles, pMeta->handleSizeInBytes);

  if(!pHandleMeta)
    return NULL;

  Rtos_Memcpy(pHandleMeta->pHandles, pMeta->pHandles, (pHandleMeta->maxHandles * pHandleMeta->handleSizeInBytes));

  return pHandleMeta;
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_HandleMetaData_Clone((AL_THandleMetaData*)pMeta);
}

AL_THandleMetaData* AL_HandleMetaData_Create(int iMaxHandles, int iHandleSize)
{
  AL_THandleMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->pHandles = Rtos_Malloc(iMaxHandles * iHandleSize);

  if(!pMeta->pHandles)
  {
    Rtos_Free(pMeta);
    return NULL;
  }

  pMeta->tMeta.eType = AL_META_TYPE_HANDLE;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;
  pMeta->numHandles = 0;
  pMeta->handleSizeInBytes = iHandleSize;
  pMeta->maxHandles = iMaxHandles;

  return pMeta;
}

static AL_HANDLE getHandlePtr(AL_THandleMetaData* pMeta, int iNumHandle)
{
  return (AL_HANDLE)(((uintptr_t)pMeta->pHandles) + (iNumHandle * pMeta->handleSizeInBytes));
}

bool AL_HandleMetaData_AddHandle(AL_THandleMetaData* pMeta, AL_HANDLE pHandle)
{
  if(pMeta->numHandles + 1 > pMeta->maxHandles)
    return false;

  Rtos_Memcpy(getHandlePtr(pMeta, pMeta->numHandles), pHandle, pMeta->handleSizeInBytes);
  ++pMeta->numHandles;
  return true;
}

AL_HANDLE AL_HandleMetaData_GetHandle(AL_THandleMetaData* pMeta, int iNumHandle)
{
  return getHandlePtr(pMeta, iNumHandle);
}
