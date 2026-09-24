// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common/IntFifo.h"
#include "lib_common/Index.h"

#define BUFPOOL_MAX_SIZE 48
#define BUFPOOL_UNDEF_ID UINT8_MAX

/*****************************************************************************/
typedef struct AL_TBufPool
{
  TBuffer pBufs[BUFPOOL_MAX_SIZE]; /*!< The buffer pool */
  Rtos_AtomicVolatileType iAccessCnt[BUFPOOL_MAX_SIZE]; /*!< Number of handles holding the buffer */
  uint8_t uBufCnt;
  bool bShouldCleanMemory;

  int32_t vFreeIds[BUFPOOL_MAX_SIZE]; // Used by IntFifo
  IntFifo tFreeIdFifo;

  AL_MUTEX Mutex;
  AL_SEMAPHORE Semaphore;
}AL_TBufPool;

/*****************************************************************************/
bool AL_TBufPool_Init(AL_TBufPool* pBufPool, uint8_t uMaxBuf, size_t zSize, AL_TAllocator* pAllocator, char const* name, bool bShouldCleanMemory);
void AL_TBufPool_Deinit(AL_TBufPool* pBufPool);
AL_TIndex AL_TBufPool_GetFreeBufID(AL_TBufPool* pBufPool);
TBuffer AL_TBufPool_GetBufFromId(AL_TBufPool* pBufPool, AL_TIndex tID);
void AL_TBufPool_DecrementBufIDCdf(AL_TBufPool* pCdfBufPool, AL_TIndex tCdfID);
bool AL_TBufPool_GetBufFreeStatus(AL_TBufPool* pBufPool, AL_TIndex tID);
bool AL_TBufPool_DecrementBufID(AL_TBufPool* pBufPool, AL_TIndex tID);
void AL_TBufPool_IncrementBufID(AL_TBufPool* pBufPool, AL_TIndex tID);
void AL_TBufPool_Terminate(AL_TBufPool* pBufPool);
