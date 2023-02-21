// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <stdexcept>
extern "C"
{
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Allocator.h"
}

#include "lib_app/BufPool.h"

static bool Fifo_Init(App_Fifo* pFifo, size_t zMaxElem);
static void Fifo_Deinit(App_Fifo* pFifo);
static bool Fifo_Queue(App_Fifo* pFifo, void* pElem, uint32_t uWait);
static void* Fifo_Dequeue(App_Fifo* pFifo, uint32_t uWait);
static void Fifo_Decommit(App_Fifo* pFifo);
static void Fifo_Commit(App_Fifo* pFifo);
static size_t Fifo_GetMaxElements(App_Fifo* pFifo);

/****************************************************************************/
static void AL_sBufPool_FreeBufInPool(AL_TBuffer* pBuf)
{
  auto pBufPool = (AL_TBufPool*)AL_Buffer_GetUserData(pBuf);
  Fifo_Queue(&pBufPool->fifo, pBuf, AL_WAIT_FOREVER);
}

static AL_TBuffer* AL_sBufPool_CreateBuffer(AL_TBufPoolConfig& config, AL_TAllocator* pAllocator)
{
  AL_TMetaData* pMeta = NULL;

  AL_TBuffer* pBuf = AL_Buffer_Create_And_AllocateNamed(pAllocator, config.zBufSize, AL_sBufPool_FreeBufInPool, config.debugName);

  if(!pBuf)
    goto fail_buffer_init;

  if(config.pMetaData)
  {
    pMeta = AL_MetaData_Clone(config.pMetaData);

    if(!pMeta)
      goto fail_meta_clone;

    if(!AL_Buffer_AddMetaData(pBuf, pMeta))
      goto fail_buffer_add_meta;
  }

  return pBuf;

  fail_buffer_add_meta:
  AL_MetaData_Destroy(pMeta);
  fail_meta_clone:
  AL_Buffer_Destroy(pBuf);
  fail_buffer_init:
  return NULL;
}

/****************************************************************************/
static bool AL_sBufPool_AddBuf(AL_TBufPool* pBufPool, AL_TBuffer* pBuf)
{
  if(!pBuf)
    return false;

  if(pBufPool->uNumBuf >= Fifo_GetMaxElements(&pBufPool->fifo))
    return false;

  AL_Buffer_SetUserData(pBuf, pBufPool);
  pBufPool->pPool[pBufPool->uNumBuf++] = pBuf;
  Fifo_Queue(&pBufPool->fifo, pBuf, AL_WAIT_FOREVER);
  return true;
}

/****************************************************************************/
static bool AL_sBufPool_AddAllocBuf(AL_TBufPool* pBufPool, AL_TBufPoolConfig* pConfig)
{
  AL_TBuffer* pBuf = AL_sBufPool_CreateBuffer(*pConfig, pBufPool->pAllocator);
  return AL_sBufPool_AddBuf(pBufPool, pBuf);
}

/****************************************************************************/
static bool AL_sBufPool_InitStructure(AL_TBufPool* pBufPool, AL_TAllocator* pAllocator, uint32_t uNumBuf, size_t zBufSize, AL_TMetaData* pCreationMeta)
{
  size_t zMemPoolSize = 0;

  if(!pBufPool)
    return false;

  if(!pAllocator)
    return false;

  pBufPool->pAllocator = pAllocator;

  if(!Fifo_Init(&pBufPool->fifo, uNumBuf))
    return false;

  pBufPool->zBufSize = zBufSize;
  pBufPool->pCreationMeta = pCreationMeta;
  pBufPool->uNumBuf = 0;

  zMemPoolSize = uNumBuf * sizeof(AL_TBuffer*);

  pBufPool->pPool = (AL_TBuffer**)Rtos_Malloc(zMemPoolSize);

  if(!pBufPool->pPool)
  {
    AL_BufPool_Deinit(pBufPool);
    return false;
  }

  return true;
}

/****************************************************************************/
bool AL_BufPool_Init(AL_TBufPool* pBufPool, AL_TAllocator* pAllocator, AL_TBufPoolConfig* pConfig)
{
  if(!AL_sBufPool_InitStructure(pBufPool, pAllocator, pConfig->uNumBuf, pConfig->zBufSize, pConfig->pMetaData))
    return false;

  // Create uMin free buffers
  while(pBufPool->uNumBuf < pConfig->uNumBuf)
  {
    if(!AL_sBufPool_AddAllocBuf(pBufPool, pConfig))
    {
      AL_BufPool_Deinit(pBufPool);
      return false;
    }
  }

  return true;
}

/****************************************************************************/
void AL_BufPool_Deinit(AL_TBufPool* pBufPool)
{
  for(uint32_t u = 0; u < pBufPool->uNumBuf; ++u)
  {
    AL_TBuffer* pBuf = pBufPool->pPool[u];
    AL_Buffer_Destroy(pBuf);
    pBufPool->pPool[u] = NULL;
  }

  if(pBufPool->pCreationMeta)
    AL_MetaData_Destroy(pBufPool->pCreationMeta);
  Fifo_Deinit(&pBufPool->fifo);
  Rtos_Free(pBufPool->pPool);
  Rtos_Memset(pBufPool, 0, sizeof(*pBufPool));
}

/****************************************************************************/
AL_TBuffer* AL_BufPool_GetBuffer(AL_TBufPool* pBufPool, AL_EBufMode eMode)
{
  uint32_t Wait = AL_GetWaitMode(eMode);

  auto pBuf = (AL_TBuffer*)Fifo_Dequeue(&pBufPool->fifo, Wait);

  if(!pBuf)
    return NULL;

  AL_Buffer_Ref(pBuf);
  return pBuf;
}

/****************************************************************************/
bool AL_BufPool_AddMetaData(AL_TBufPool* pBufPool, AL_TMetaData* pMetaData)
{
  AL_TMetaData* pMeta;
  AL_TBuffer* pBuf;

  for(uint32_t u = 0; u < pBufPool->uNumBuf; ++u)
  {
    pBuf = pBufPool->pPool[u];
    pMeta = AL_MetaData_Clone(pMetaData);

    if(!AL_Buffer_AddMetaData(pBuf, pMeta))
      return false;
  }

  return true;
}

/****************************************************************************/
void AL_BufPool_Decommit(AL_TBufPool* pBufPool)
{
  Fifo_Decommit(&pBufPool->fifo);
}

void AL_BufPool_Commit(AL_TBufPool* pBufPool)
{
  Fifo_Commit(&pBufPool->fifo);
}

static bool Fifo_Init(App_Fifo* pFifo, size_t zMaxElem)
{
  pFifo->m_zMaxElem = zMaxElem + 1;
  pFifo->m_zTail = 0;
  pFifo->m_zHead = 0;
  pFifo->m_iBufNumber = 0;
  pFifo->m_isDecommited = false;

  size_t zElemSize = pFifo->m_zMaxElem * sizeof(void*);
  pFifo->m_ElemBuffer = (void**)Rtos_Malloc(zElemSize);

  if(!pFifo->m_ElemBuffer)
    return false;
  Rtos_Memset(pFifo->m_ElemBuffer, 0xCD, zElemSize);

  pFifo->hEvent = Rtos_CreateEvent(0);

  if(!pFifo->hEvent)
  {
    Rtos_Free(pFifo->m_ElemBuffer);
    return false;
  }

  pFifo->hSpaceSem = Rtos_CreateSemaphore(zMaxElem);
  pFifo->hMutex = Rtos_CreateMutex();

  if(!pFifo->hSpaceSem)
  {
    Rtos_DeleteEvent(pFifo->hEvent);
    Rtos_Free(pFifo->m_ElemBuffer);
    return false;
  }

  return true;
}

static void Fifo_Deinit(App_Fifo* pFifo)
{
  Rtos_Free(pFifo->m_ElemBuffer);
  Rtos_DeleteEvent(pFifo->hEvent);
  Rtos_DeleteSemaphore(pFifo->hSpaceSem);
  Rtos_DeleteMutex(pFifo->hMutex);
}

static bool Fifo_Queue(App_Fifo* pFifo, void* pElem, uint32_t uWait)
{
  if(!Rtos_GetSemaphore(pFifo->hSpaceSem, uWait))
    return false;

  Rtos_GetMutex(pFifo->hMutex);
  pFifo->m_ElemBuffer[pFifo->m_zTail] = pElem;
  pFifo->m_zTail = (pFifo->m_zTail + 1) % pFifo->m_zMaxElem;
  ++pFifo->m_iBufNumber;
  Rtos_SetEvent(pFifo->hEvent);
  Rtos_ReleaseMutex(pFifo->hMutex);

  /* new item was added in the queue */
  return true;
}

static void* Fifo_Dequeue(App_Fifo* pFifo, uint32_t uWait)
{
  /* wait if no items */
  Rtos_GetMutex(pFifo->hMutex);
  bool failed = false;

  while(true)
  {
    if(pFifo->m_iBufNumber > 0)
      break;

    if(failed || pFifo->m_isDecommited)
    {
      Rtos_ReleaseMutex(pFifo->hMutex);
      return NULL;
    }

    Rtos_ReleaseMutex(pFifo->hMutex);

    if(!Rtos_WaitEvent(pFifo->hEvent, uWait))
      failed = true;

    Rtos_GetMutex(pFifo->hMutex);
  }

  void* pElem = pFifo->m_ElemBuffer[pFifo->m_zHead];
  pFifo->m_zHead = (pFifo->m_zHead + 1) % pFifo->m_zMaxElem;
  --pFifo->m_iBufNumber;
  Rtos_ReleaseMutex(pFifo->hMutex);

  /* new empty space available */
  Rtos_ReleaseSemaphore(pFifo->hSpaceSem);
  return pElem;
}

static void Fifo_Decommit(App_Fifo* pFifo)
{
  Rtos_GetMutex(pFifo->hMutex);
  pFifo->m_isDecommited = true;
  Rtos_SetEvent(pFifo->hEvent);
  Rtos_ReleaseMutex(pFifo->hMutex);
}

/* Protected by mutex, but to be really useful, you need to know that you already
 * used the decommit feature successfuly and you want to reuse the bufpool again
 * after that. */
static void Fifo_Commit(App_Fifo* pFifo)
{
  Rtos_GetMutex(pFifo->hMutex);
  pFifo->m_isDecommited = false;
  Rtos_ReleaseMutex(pFifo->hMutex);
}

static size_t Fifo_GetMaxElements(App_Fifo* pFifo)
{
  size_t sMaxElem = 0;
  Rtos_GetMutex(pFifo->hMutex);
  sMaxElem = pFifo->m_zMaxElem - 1;
  Rtos_ReleaseMutex(pFifo->hMutex);
  return sMaxElem;
}

uint32_t AL_GetWaitMode(AL_EBufMode eMode)
{
  uint32_t Wait = 0;
  switch(eMode)
  {
  case AL_BUF_MODE_BLOCK:
    Wait = AL_WAIT_FOREVER;
    break;
  case AL_BUF_MODE_NONBLOCK:
    Wait = AL_NO_WAIT;
    break;
  default:

    if(eMode < AL_BUF_MODE_MAX)
      throw std::runtime_error("eMode should be higher or equal than AL_BUF_MODE_MAX");
    break;
  }

  return Wait;
}

#ifdef __cplusplus

bool BaseBufPool::InitStructure(AL_TAllocator* pAllocator, uint32_t uNumBuf, size_t zBufSize, AL_TMetaData* pCreationMeta)
{
  return AL_sBufPool_InitStructure(&m_pool, pAllocator, uNumBuf, zBufSize, pCreationMeta);
}

bool BaseBufPool::AddBuf(AL_TBuffer* pBuf)
{
  return AL_sBufPool_AddBuf(&m_pool, pBuf);
}

void BaseBufPool::FreeBufInPool(AL_TBuffer* pBuf)
{
  AL_sBufPool_FreeBufInPool(pBuf);
}

#endif
