// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
#include "BufPool.h"

/*************************************************************************/
void AL_TBufPool_Deinit(AL_TBufPool* pBufPool)
{
  for(uint8_t i = 0; i < pBufPool->uBufCnt; i++)
    AL_MemDesc_Free(&pBufPool->pBufs[i].tMD);

  if(pBufPool->Semaphore)
    Rtos_DeleteSemaphore(pBufPool->Semaphore);

  if(pBufPool->Mutex)
    Rtos_DeleteMutex(pBufPool->Mutex);
}

/*************************************************************************/
bool AL_TBufPool_Init(AL_TBufPool* pBufPool, uint8_t uMaxBuf, size_t zSize, AL_TAllocator* pAllocator, char const* name, bool bShouldCleanMemory)
{
  Rtos_Assert(uMaxBuf <= BUFPOOL_MAX_SIZE);

  pBufPool->uBufCnt = uMaxBuf;
  IntFifo_Init(&pBufPool->tFreeIdFifo, pBufPool->vFreeIds, ARRAY_SIZE(pBufPool->vFreeIds));

  for(uint8_t i = 0; i < uMaxBuf; ++i)
  {
    Rtos_Memset(&pBufPool->pBufs[i].tMD, 0, sizeof(pBufPool->pBufs[i].tMD));
    pBufPool->iAccessCnt[i] = 0;
    IntFifo_Queue(&pBufPool->tFreeIdFifo, i);

    if(zSize != 0 && !AL_MemDesc_AllocNamed(&pBufPool->pBufs[i].tMD, pAllocator, zSize, name))
      goto fail_alloc;

    AL_CleanupMemory(pBufPool->pBufs[i].tMD.pVirtualAddr, pBufPool->pBufs[i].tMD.uSize);
  }

  pBufPool->Mutex = Rtos_CreateMutex();

  if(!pBufPool->Mutex)
    goto fail_alloc;

  pBufPool->Semaphore = Rtos_CreateSemaphore(uMaxBuf);

  if(!pBufPool->Semaphore)
    goto fail_alloc;

  pBufPool->bShouldCleanMemory = bShouldCleanMemory;

  return true;

  fail_alloc:
  AL_TBufPool_Deinit(pBufPool);

  return false;
}

/*************************************************************************/
AL_TIndex AL_TBufPool_GetFreeBufID(AL_TBufPool* pBufPool)
{
  Rtos_GetSemaphore(pBufPool->Semaphore, AL_WAIT_FOREVER);
  Rtos_GetMutex(pBufPool->Mutex);

  Rtos_Assert(!IntFifo_Empty(&pBufPool->tFreeIdFifo));
  AL_TIndex tID = IntFifo_Dequeue(&pBufPool->tFreeIdFifo);
  Rtos_Assert(pBufPool->iAccessCnt[tID] == 0);
  pBufPool->iAccessCnt[tID] = 1;

  if(pBufPool->bShouldCleanMemory)
    AL_CleanupMemory(pBufPool->pBufs[tID].tMD.pVirtualAddr, pBufPool->pBufs[tID].tMD.uSize);
  Rtos_ReleaseMutex(pBufPool->Mutex);
  return tID;
}

/*************************************************************************/
bool AL_TBufPool_GetBufFreeStatus(AL_TBufPool* pBufPool, AL_TIndex tID)
{
  Rtos_GetMutex(pBufPool->Mutex);
  bool bFree = (pBufPool->iAccessCnt[tID] == 0);
  Rtos_ReleaseMutex(pBufPool->Mutex);

  return bFree;
}

/*************************************************************************/
bool AL_TBufPool_DecrementBufID(AL_TBufPool* pBufPool, AL_TIndex tID)
{
  Rtos_Assert(tID < BUFPOOL_MAX_SIZE);
  Rtos_GetMutex(pBufPool->Mutex);

  bool bFree = false;

  if(pBufPool->iAccessCnt[tID])
  {
    Rtos_AtomicDecrement(&(pBufPool->iAccessCnt[tID]));
    bFree = (pBufPool->iAccessCnt[tID] == 0);

    if(bFree)
      IntFifo_Queue(&pBufPool->tFreeIdFifo, tID);
  }

  Rtos_ReleaseMutex(pBufPool->Mutex);

  if(bFree)
    Rtos_ReleaseSemaphore(pBufPool->Semaphore);

  return bFree;
}

/*************************************************************************/
void AL_TBufPool_IncrementBufID(AL_TBufPool* pBufPool, AL_TIndex tID)
{
  Rtos_Assert(tID < BUFPOOL_MAX_SIZE);
  Rtos_GetMutex(pBufPool->Mutex);
  Rtos_AtomicIncrement(&(pBufPool->iAccessCnt[tID]));
  Rtos_ReleaseMutex(pBufPool->Mutex);
}

/*****************************************************************************/
void AL_TBufPool_Terminate(AL_TBufPool* pBufPool)
{
  for(uint8_t i = 0; i < pBufPool->uBufCnt; ++i)
    Rtos_GetSemaphore(pBufPool->Semaphore, AL_WAIT_FOREVER);

  for(uint8_t i = 0; i < pBufPool->uBufCnt; ++i)
    Rtos_ReleaseSemaphore(pBufPool->Semaphore);
}

/*****************************************************************************/
TBuffer AL_TBufPool_GetBufFromId(AL_TBufPool* pBufPool, AL_TIndex tID)
{
  return pBufPool->pBufs[tID];
}
