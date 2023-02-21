// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferAPI.h"

typedef struct
{
  AL_TBuffer* buf;
  int prev;
  int next;
}WorkPoolElem;

typedef struct
{
  WorkPoolElem* elems;
  AL_MUTEX lock;
  AL_EVENT spaceAvailable;
  int freeHead;
  int freeQueue;
  int filledHead;
  int filledQueue;
  int capacity;
}WorkPool;

bool AL_WorkPool_Init(WorkPool* pool, int iMaxBufNum);
void AL_WorkPool_Deinit(WorkPool* pool);
void AL_WorkPool_Remove(WorkPool* pool, AL_TBuffer* pBuf);
void AL_WorkPool_PushBack(WorkPool* pool, AL_TBuffer* pBuf);
bool AL_WorkPool_IsEmpty(WorkPool* pool); /* Not thread safe */
bool AL_WorkPool_IsFull(WorkPool* pool);  /* Not thread safe */
int AL_WorkPool_GetSize(WorkPool* pool);  /* Not thread safe */
