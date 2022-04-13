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

#include "lib_decode/WorkPool.h"
#include "lib_assert/al_assert.h"

#define INVALID_BUF_POS -1

bool AL_WorkPool_Init(WorkPool* pool, int iMaxBufNum)
{
  pool->elems = Rtos_Malloc(sizeof(WorkPoolElem) * iMaxBufNum);

  if(!pool->elems)
    return false;

  pool->lock = Rtos_CreateMutex();

  if(!pool->lock)
  {
    Rtos_Free(pool->elems);
    return false;
  }

  pool->spaceAvailable = Rtos_CreateEvent(false);

  if(!pool->spaceAvailable)
  {
    Rtos_Free(pool->elems);
    Rtos_DeleteMutex(pool->lock);
    return false;
  }

  pool->freeHead = 0;
  pool->freeQueue = iMaxBufNum - 1;
  pool->filledHead = INVALID_BUF_POS;
  pool->filledQueue = INVALID_BUF_POS;
  pool->capacity = iMaxBufNum;

  for(int i = 0; i < pool->capacity; ++i)
  {
    pool->elems[i].buf = NULL;
    pool->elems[i].prev = i - 1;
    pool->elems[i].next = i + 1;
  }

  pool->elems[0].prev = INVALID_BUF_POS;
  pool->elems[pool->capacity - 1].next = INVALID_BUF_POS;

  return true;
}

void AL_WorkPool_Deinit(WorkPool* pool)
{
  Rtos_Free(pool->elems);
  Rtos_DeleteMutex(pool->lock);
  Rtos_DeleteEvent(pool->spaceAvailable);
}

void AL_WorkPool_Remove(WorkPool* pool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pool->lock);

  int iPos = pool->filledHead;

  while(iPos != INVALID_BUF_POS && pool->elems[iPos].buf != pBuf)
    iPos = pool->elems[iPos].next;

  AL_Assert(iPos != INVALID_BUF_POS);

  WorkPoolElem* pElem = &pool->elems[iPos];

  /* Remove from filled list */
  if(pElem->prev != INVALID_BUF_POS)
    pool->elems[pElem->prev].next = pElem->next;
  else
    pool->filledHead = pElem->next;

  if(pElem->next != INVALID_BUF_POS)
    pool->elems[pElem->next].prev = pElem->prev;
  else
    pool->filledQueue = pElem->prev;

  /* Add in free list */
  pElem->buf = NULL;
  pElem->prev = pool->freeQueue;
  pElem->next = INVALID_BUF_POS;

  if(pool->freeQueue != INVALID_BUF_POS)
    pool->elems[pool->freeQueue].next = iPos;
  pool->freeQueue = iPos;

  if(pool->freeHead == INVALID_BUF_POS)
    pool->freeHead = iPos;

  Rtos_SetEvent(pool->spaceAvailable);

  Rtos_ReleaseMutex(pool->lock);
}

void AL_WorkPool_PushBack(WorkPool* pool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pool->lock);

  while(AL_WorkPool_IsFull(pool))
  {
    Rtos_ReleaseMutex(pool->lock);
    Rtos_WaitEvent(pool->spaceAvailable, AL_WAIT_FOREVER);
    Rtos_GetMutex(pool->lock);
  }

  int iPos = pool->freeHead;
  WorkPoolElem* pElem = &pool->elems[iPos];

  /* Remove from free list */
  if(pElem->next != INVALID_BUF_POS)
    pool->elems[pElem->next].prev = INVALID_BUF_POS;
  else
    pool->freeQueue = INVALID_BUF_POS;
  pool->freeHead = pElem->next;

  /* Add in filled list */
  pElem->buf = pBuf;
  pElem->prev = pool->filledQueue;
  pElem->next = INVALID_BUF_POS;

  if(pool->filledQueue != INVALID_BUF_POS)
    pool->elems[pool->filledQueue].next = iPos;
  pool->filledQueue = iPos;

  if(pool->filledHead == INVALID_BUF_POS)
    pool->filledHead = iPos;

  Rtos_ReleaseMutex(pool->lock);
}

bool AL_WorkPool_IsEmpty(WorkPool* pool)
{
  return pool->filledHead == INVALID_BUF_POS;
}

bool AL_WorkPool_IsFull(WorkPool* pool)
{
  return pool->freeHead == INVALID_BUF_POS;
}

int AL_WorkPool_GetSize(WorkPool* pool)
{
  int iCnt = 0;
  int iPos = pool->filledHead;

  while(iPos != INVALID_BUF_POS)
  {
    iPos = pool->elems[iPos].next;
    iCnt++;
  }

  return iCnt;
}
