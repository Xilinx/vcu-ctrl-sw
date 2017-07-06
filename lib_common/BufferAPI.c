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

#include "lib_common/BufferAPI.h"
#include "List.h"
#include "assert.h"

typedef struct al_t_MetaItem
{
  AL_ListHead list;
  AL_TMetaData* pMeta;
}AL_TMetaItem;

typedef struct al_t_BufferImpl
{
  AL_TBuffer buf;
  AL_MUTEX pLock;
  int iRefCount;
  AL_ListHead metaList;
  void* pUserData; /*!< user private data */
  PFN_RefCount_CallBack pCallBack; /*!< user callback. called when the buffer refcount reaches 0 */
}AL_TBufferImpl;

static bool AL_Buffer_InitData(AL_TBufferImpl* pBuf, AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  pBuf->buf.zSize = zSize;
  pBuf->buf.pAllocator = pAllocator;
  pBuf->pCallBack = pCallBack;
  pBuf->buf.hBuf = hBuf;
  pBuf->buf.pData = AL_Allocator_GetVirtualAddr(pAllocator, pBuf->buf.hBuf);
  AL_ListHeadInit(&pBuf->metaList);

  pBuf->iRefCount = 0;
  pBuf->pLock = Rtos_CreateMutex(false);

  if(!pBuf->pLock)
    return false;

  return true;
}

static AL_TBuffer* createBuffer(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  AL_TBufferImpl* pBuf;

  if(!hBuf)
    return NULL;

  pBuf = Rtos_Malloc(sizeof(*pBuf));

  if(!pBuf)
    return NULL;

  if(!AL_Buffer_InitData(pBuf, pAllocator, hBuf, zSize, pCallBack))
    goto fail_init_data;

  return (AL_TBuffer*)pBuf;

  fail_init_data:
  Rtos_Free(pBuf);
  return NULL;
}

AL_TBuffer* AL_Buffer_WrapData(uint8_t* pData, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  AL_TAllocator* pAllocator = AL_GetWrapperAllocator();
  AL_HANDLE hBuf = AL_WrapperAllocator_WrapData(pData);

  return createBuffer(pAllocator, hBuf, zSize, pCallBack);
}

AL_TBuffer* AL_Buffer_Create(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  return createBuffer(pAllocator, hBuf, zSize, pCallBack);
}

AL_TBuffer* AL_Buffer_Create_And_Allocate(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  AL_HANDLE hBuf = AL_Allocator_Alloc(pAllocator, zSize);

  AL_TBuffer* pBuf = createBuffer(pAllocator, hBuf, zSize, pCallBack);

  if(!pBuf)
    AL_Allocator_Free(pAllocator, pBuf->hBuf);

  return pBuf;
}

void AL_Buffer_Destroy(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  AL_TMetaItem* item, * tmp;

  AL_ListForEachEntrySafe(item, tmp, &pBuf->metaList, list, AL_TMetaItem)
  {
    AL_ListDel(&item->list);
    item->pMeta->MetaDestroy(item->pMeta);
    Rtos_Free(item);
  }

  Rtos_DeleteMutex(pBuf->pLock);
  Rtos_Free(pBuf);
}

void AL_Buffer_SetUserData(AL_TBuffer* hBuf, void* pUserData)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  Rtos_GetMutex(pBuf->pLock);
  pBuf->pUserData = pUserData;
  Rtos_ReleaseMutex(pBuf->pLock);
}

void* AL_Buffer_GetUserData(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  void* pUserData;

  Rtos_GetMutex(pBuf->pLock);
  pUserData = pBuf->pUserData;
  Rtos_ReleaseMutex(pBuf->pLock);

  return pUserData;
}

/*****************************************************************************/
uint8_t* AL_Buffer_GetBufferData(AL_TBuffer* pBuf)
{
  return pBuf->pData;
}

/*****************************************************************************/
size_t AL_Buffer_GetSizeData(AL_TBuffer* pBuf)
{
  return pBuf->zSize;
}

/****************************************************************************/
void AL_Buffer_Ref(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  Rtos_GetMutex(pBuf->pLock);
  ++pBuf->iRefCount;
  Rtos_ReleaseMutex(pBuf->pLock);
}

/****************************************************************************/
void AL_Buffer_Unref(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  Rtos_GetMutex(pBuf->pLock);
  --pBuf->iRefCount;
  assert(pBuf->iRefCount >= 0);

  if(pBuf->iRefCount <= 0)
  {
    Rtos_ReleaseMutex(pBuf->pLock);

    if(pBuf->pCallBack)
      pBuf->pCallBack(hBuf);
  }
  else
  {
    Rtos_ReleaseMutex(pBuf->pLock);
  }
}

/****************************************************************************/
AL_TMetaData* AL_Buffer_GetMetaData(AL_TBuffer const* hBuf, AL_EMetaType eType)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  AL_TMetaItem* item;
  Rtos_GetMutex(pBuf->pLock);
  AL_ListForEachEntry(item, &pBuf->metaList, list, AL_TMetaItem)
  {
    if(item->pMeta->eType == eType)
    {
      Rtos_ReleaseMutex(pBuf->pLock);
      return item->pMeta;
    }
  }

  Rtos_ReleaseMutex(pBuf->pLock);
  return NULL;
}

/****************************************************************************/
bool AL_Buffer_AddMetaData(AL_TBuffer* hBuf, AL_TMetaData* pMeta)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  AL_TMetaItem* pItem = Rtos_Malloc(sizeof(*pItem));

  if(!pItem)
    return false;

  pItem->pMeta = pMeta;

  Rtos_GetMutex(pBuf->pLock);
  AL_ListAddTail(&pItem->list, &pBuf->metaList);
  Rtos_ReleaseMutex(pBuf->pLock);

  return true;
}

/****************************************************************************/
bool AL_Buffer_RemoveMetaData(AL_TBuffer* hBuf, AL_TMetaData* pMeta)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  AL_TMetaItem* item;
  AL_TMetaItem* tmp;

  Rtos_GetMutex(pBuf->pLock);
  AL_ListForEachEntrySafe(item, tmp, &pBuf->metaList, list, AL_TMetaItem)
  {
    if(item->pMeta == pMeta)
    {
      AL_ListDel(&item->list);
      Rtos_Free(item);
      Rtos_ReleaseMutex(pBuf->pLock);
      return true;
    }
  }

  Rtos_ReleaseMutex(pBuf->pLock);
  return false;
}

/****************************************************************************/
AL_TMetaData* AL_Buffer_IterateMetaData(AL_TBuffer* hBuf, AL_HANDLE* hState)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  AL_TMetaItem* item;

  AL_ListHead** pState = (AL_ListHead**)hState;
  AL_ListHead* pHead = *pState;

  /* if the iteration has ended */
  if(pHead != NULL && pHead->pNext == &pBuf->metaList)
  {
    *pState = NULL;
    return NULL;
  }

  /* if state null, take the first metadata */
  if(pHead == NULL)
    pHead = &pBuf->metaList;

  Rtos_GetMutex(pBuf->pLock);

  if(AL_ListEmpty(pHead))
  {
    Rtos_ReleaseMutex(pBuf->pLock);
    return NULL;
  }

  item = AL_ListFirstEntry(pHead, AL_TMetaItem, list);
  *pState = (pHead)->pNext;

  Rtos_ReleaseMutex(pBuf->pLock);
  return item->pMeta;
}

