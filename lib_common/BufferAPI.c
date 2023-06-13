// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/BufferAPIInternal.h"
#include "lib_assert/al_assert.h"
#include "lib_common/BufCommon.h"

#define AL_BUFFER_META_ALLOC_COUNT 4
// Maximum "estimated" number of metadata for user
#define AL_BUFFER_USER_META_MAX_COUNT 20
#define AL_BUFFER_META_MAX_COUNT (AL_META_TYPE_MAX + AL_BUFFER_USER_META_MAX_COUNT)

typedef struct al_t_BufferImpl
{
  AL_TBuffer buf;
  AL_MUTEX pLock;
  int32_t iRefCount;

  AL_TMetaData** pMeta;
  int iMetaCount;
  int iAllocatedMetaCount;

  void* pUserData; /*!< user private data */
  PFN_RefCount_CallBack pCallBack; /*!< user callback. called when the buffer refcount reaches 0 */
}AL_TBufferImpl;

static void* Realloc(void* pPtr, size_t zOldSize, size_t zNewSize)
{
  void* pNewPtr = Rtos_Malloc(zNewSize);

  if(!pNewPtr)
    return NULL;

  if(pPtr)
  {
    size_t zSizeToCopy = zOldSize < zNewSize ? zOldSize : zNewSize;
    Rtos_Memcpy(pNewPtr, pPtr, zSizeToCopy);
    Rtos_Free(pPtr);
  }

  return pNewPtr;
}

static bool isMaxChunkReached(AL_TBufferImpl* pBuf)
{
  return pBuf->buf.iChunkCnt >= AL_BUFFER_MAX_CHUNK;
}

static inline int8_t addBufferChunk(AL_TBufferImpl* pBuf, AL_HANDLE hChunk, size_t zSize)
{
  if(zSize && !hChunk)
    return AL_BUFFER_BAD_CHUNK;

  int iChunkIdx = pBuf->buf.iChunkCnt;
  pBuf->buf.hBufs[iChunkIdx] = hChunk;
  pBuf->buf.zSizes[iChunkIdx] = zSize;
  pBuf->buf.iChunkCnt++;

  return iChunkIdx;
}

static bool initData(AL_TBufferImpl* pBuf, AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack)
{
  pBuf->buf.pAllocator = pAllocator;
  pBuf->pCallBack = pCallBack;
  pBuf->pUserData = NULL;
  pBuf->pMeta = NULL;
  pBuf->iMetaCount = 0;
  pBuf->iAllocatedMetaCount = 0;
  pBuf->iRefCount = 0;
  pBuf->buf.iChunkCnt = 0;
  Rtos_Memset(pBuf->buf.zSizes, 0, sizeof(pBuf->buf.zSizes));
  Rtos_Memset(pBuf->buf.hBufs, 0, sizeof(pBuf->buf.hBufs));

  pBuf->pLock = Rtos_CreateMutex();

  if(!pBuf->pLock)
    return false;

  return true;
}

static AL_TBuffer* createEmptyBuffer(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)Rtos_Malloc(sizeof(*pBuf));

  if(!pBuf)
    return NULL;

  if(!initData(pBuf, pAllocator, pCallBack))
    goto fail_init_data;

  return (AL_TBuffer*)pBuf;

  fail_init_data:
  Rtos_Free(pBuf);
  return NULL;
}

static AL_TBuffer* createBufferWithOneChunk(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  AL_TBuffer* pBuf = createEmptyBuffer(pAllocator, pCallBack);

  if(pBuf == NULL)
    return NULL;

  if(addBufferChunk((AL_TBufferImpl*)pBuf, hBuf, zSize) == AL_BUFFER_BAD_CHUNK)
  {
    AL_Buffer_Destroy(pBuf);
    return NULL;
  }

  return pBuf;
}

AL_TBuffer* AL_Buffer_CreateEmpty(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack)
{
  return createEmptyBuffer(pAllocator, pCallBack);
}

int AL_Buffer_AllocateChunkNamed(AL_TBuffer* hBuf, size_t zSize, char const* name)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  if(isMaxChunkReached(pBuf))
    return AL_BUFFER_BAD_CHUNK;

  AL_HANDLE hChunk = AL_Allocator_AllocNamed(pBuf->buf.pAllocator, zSize, name);

  return addBufferChunk(pBuf, hChunk, zSize);
}

int AL_Buffer_AllocateChunk(AL_TBuffer* hBuf, size_t zSize)
{
  return AL_Buffer_AllocateChunkNamed(hBuf, zSize, "unknown");
}

int AL_Buffer_AddChunk(AL_TBuffer* hBuf, AL_HANDLE hChunk, size_t zSize)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  if(isMaxChunkReached(pBuf))
    return AL_BUFFER_BAD_CHUNK;

  return addBufferChunk(pBuf, hChunk, zSize);
}

AL_TBuffer* AL_Buffer_WrapData(uint8_t* pData, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  AL_TAllocator* pAllocator = AL_GetWrapperAllocator();
  AL_HANDLE hBuf = AL_WrapperAllocator_WrapData(pData, NULL, NULL);

  return createBufferWithOneChunk(pAllocator, hBuf, zSize, pCallBack);
}

AL_TBuffer* AL_Buffer_Create(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  return createBufferWithOneChunk(pAllocator, hBuf, zSize, pCallBack);
}

AL_TBuffer* AL_Buffer_ShallowCopy(AL_TBuffer const* pCopy, PFN_RefCount_CallBack pCallBack)
{
  if(!pCopy)
    return NULL;

  AL_TBuffer* pBuf = createEmptyBuffer(pCopy->pAllocator, pCallBack);

  if(pBuf == NULL)
    return NULL;

  for(int i = 0; i < AL_BUFFER_MAX_CHUNK; i++)
  {
    if(addBufferChunk((AL_TBufferImpl*)pBuf, pCopy->hBufs[i], pCopy->zSizes[i]) == AL_BUFFER_BAD_CHUNK)
    {
      AL_Buffer_Destroy(pBuf);
      return NULL;
    }
  }

  return pBuf;
}

AL_TBuffer* AL_Buffer_Create_And_AllocateNamed(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack, char const* name)
{
  AL_HANDLE hBuf = AL_Allocator_AllocNamed(pAllocator, zSize, name);

  if(!hBuf)
    return NULL;

  AL_TBuffer* pBuf = createBufferWithOneChunk(pAllocator, hBuf, zSize, pCallBack);

  if(!pBuf)
    AL_Allocator_Free(pAllocator, hBuf);

  return pBuf;
}

AL_TBuffer* AL_Buffer_Create_And_Allocate(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack)
{
  return AL_Buffer_Create_And_AllocateNamed(pAllocator, zSize, pCallBack, "unknown");
}

void AL_Buffer_Destroy(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  Rtos_GetMutex(pBuf->pLock);

  AL_Assert(pBuf->iRefCount == 0);

  if(pBuf->pMeta)
  {
    for(int i = 0; i < pBuf->iMetaCount; ++i)
      AL_MetaData_Destroy(pBuf->pMeta[i]);

    Rtos_Free(pBuf->pMeta);
  }

  for(int i = 0; i < hBuf->iChunkCnt; ++i)
  {
    AL_HANDLE hChunk = hBuf->hBufs[i];
    AL_Allocator_Free(hBuf->pAllocator, hChunk);
  }

  Rtos_ReleaseMutex(pBuf->pLock);
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

  Rtos_GetMutex(pBuf->pLock);
  void* pUserData = pBuf->pUserData;
  Rtos_ReleaseMutex(pBuf->pLock);

  return pUserData;
}

/****************************************************************************/
void AL_Buffer_Ref(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  Rtos_AtomicIncrement(&pBuf->iRefCount);
}

/****************************************************************************/
void AL_Buffer_Unref(AL_TBuffer* hBuf)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  int32_t iRefCount = Rtos_AtomicDecrement(&pBuf->iRefCount);
  AL_Assert(iRefCount >= 0);

  if(iRefCount > 0)
    return;

  if(pBuf->pCallBack)
    pBuf->pCallBack(hBuf);
}

/****************************************************************************/
AL_TMetaData* AL_Buffer_GetMetaData(AL_TBuffer const* hBuf, AL_EMetaType eType)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;
  Rtos_GetMutex(pBuf->pLock);

  for(int i = 0; i < pBuf->iMetaCount; ++i)
  {
    if(pBuf->pMeta[i]->eType == eType)
    {
      AL_TMetaData* pMeta = pBuf->pMeta[i];
      Rtos_ReleaseMutex(pBuf->pLock);
      return pMeta;
    }
  }

  Rtos_ReleaseMutex(pBuf->pLock);
  return NULL;
}

/****************************************************************************/
bool AL_Buffer_AddMetaData(AL_TBuffer* hBuf, AL_TMetaData* pMeta)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  Rtos_GetMutex(pBuf->pLock);

  if(pBuf->iMetaCount == pBuf->iAllocatedMetaCount)
  {
    size_t const zOldSize = sizeof(AL_TMetaData*) * pBuf->iAllocatedMetaCount;
    size_t const zNewSize = sizeof(AL_TMetaData*) * (pBuf->iAllocatedMetaCount + AL_BUFFER_META_ALLOC_COUNT);

    AL_TMetaData** pNewBuffer = (AL_TMetaData**)Realloc(pBuf->pMeta, zOldSize, zNewSize);

    if(!pNewBuffer)
    {
      Rtos_ReleaseMutex(pBuf->pLock);
      return false;
    }

    pBuf->iAllocatedMetaCount += AL_BUFFER_META_ALLOC_COUNT;
    pBuf->pMeta = pNewBuffer;

    AL_Assert(pBuf->iAllocatedMetaCount < AL_BUFFER_META_MAX_COUNT);
  }

  pBuf->pMeta[pBuf->iMetaCount] = pMeta;
  pBuf->iMetaCount++;

  Rtos_ReleaseMutex(pBuf->pLock);

  return true;
}

/****************************************************************************/
bool AL_Buffer_RemoveMetaData(AL_TBuffer* hBuf, AL_TMetaData* pMeta)
{
  AL_TBufferImpl* pBuf = (AL_TBufferImpl*)hBuf;

  Rtos_GetMutex(pBuf->pLock);

  for(int i = 0; i < pBuf->iMetaCount; ++i)
  {
    if(pBuf->pMeta[i] == pMeta)
    {
      pBuf->pMeta[i] = pBuf->pMeta[pBuf->iMetaCount - 1];
      pBuf->iMetaCount--;

      Rtos_ReleaseMutex(pBuf->pLock);
      return true;
    }
  }

  Rtos_ReleaseMutex(pBuf->pLock);
  return false;
}

/****************************************************************************/
uint8_t* AL_Buffer_GetData(const AL_TBuffer* hBuf)
{
  return AL_Buffer_GetDataChunk(hBuf, 0);
}

/****************************************************************************/
uint8_t* AL_Buffer_GetDataChunk(const AL_TBuffer* hBuf, int iChunkIdx)
{
  if(!AL_Buffer_HasChunk(hBuf, iChunkIdx))
    return NULL;

  return AL_Allocator_GetVirtualAddr(hBuf->pAllocator, hBuf->hBufs[iChunkIdx]);
}

/****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddress(const AL_TBuffer* hBuf)
{
  return AL_Buffer_GetPhysicalAddressChunk(hBuf, 0);
}

/****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddressChunk(const AL_TBuffer* hBuf, int iChunkIdx)
{
  if(!AL_Buffer_HasChunk(hBuf, iChunkIdx))
    return 0;

  return AL_Allocator_GetPhysicalAddr(hBuf->pAllocator, hBuf->hBufs[iChunkIdx]);
}

/****************************************************************************/
AL_VADDR AL_Buffer_GetVirtualAddress(const AL_TBuffer* hBuf)
{
  return AL_Buffer_GetVirtualAddressChunk(hBuf, 0);
}

/****************************************************************************/
AL_VADDR AL_Buffer_GetVirtualAddressChunk(const AL_TBuffer* hBuf, int iChunkIdx)
{
  if(!AL_Buffer_HasChunk(hBuf, iChunkIdx))
    return 0;

  return AL_Allocator_GetVirtualAddr(hBuf->pAllocator, hBuf->hBufs[iChunkIdx]);
}

/****************************************************************************/
size_t AL_Buffer_GetSize(const AL_TBuffer* hBuf)
{
  return AL_Buffer_GetSizeChunk(hBuf, 0);
}

/****************************************************************************/
size_t AL_Buffer_GetSizeChunk(const AL_TBuffer* hBuf, int iChunkIdx)
{
  if(!AL_Buffer_HasChunk(hBuf, iChunkIdx))
    return 0;

  return hBuf->zSizes[iChunkIdx];
}

void AL_Buffer_MemSet(const AL_TBuffer* pBuf, int iVal)
{
  int iChunkCnt = AL_Buffer_GetChunkCount(pBuf);

  for(int i = 0; i < iChunkCnt; i++)
    Rtos_Memset(AL_Buffer_GetDataChunk(pBuf, i), iVal, AL_Buffer_GetSizeChunk(pBuf, i));
}

typedef void (* BufferChunkCB)(uint8_t* pChunk, size_t zChunkSize);

static void BufferForEachChunk(const AL_TBuffer* pBuf, BufferChunkCB pfnChunkCB)
{
  int8_t iChunkCount = AL_Buffer_GetChunkCount(pBuf);
  int8_t iChunk;

  for(iChunk = 0; iChunk < iChunkCount; iChunk++)
  {
    uint8_t* pChunk = AL_Buffer_GetDataChunk(pBuf, iChunk);
    size_t zChunkSize = AL_Buffer_GetSizeChunk(pBuf, iChunk);
    pfnChunkCB(pChunk, zChunkSize);
  }
}

void AL_Buffer_InvalidateMemory(const AL_TBuffer* pBuf)
{
  BufferForEachChunk(pBuf, (BufferChunkCB)Rtos_InvalidateCacheMemory);
}

void AL_Buffer_FlushMemory(const AL_TBuffer* pBuf)
{
  BufferForEachChunk(pBuf, (BufferChunkCB)Rtos_FlushCacheMemory);
}

void AL_Buffer_Cleanup(AL_TBuffer* pBuf)
{
  for(int i = 0; i < pBuf->iChunkCnt; ++i)
    AL_CleanupMemory(AL_Buffer_GetDataChunk(pBuf, i), AL_Buffer_GetSizeChunk(pBuf, i));
}

