/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

bool AL_WorkPool_Init(WorkPool* pool, int iMaxBufNum)
{
  pool->bufs = Rtos_Malloc(sizeof(AL_TBuffer*) * iMaxBufNum);
  Rtos_Memset(pool->bufs, 0, sizeof(AL_TBuffer*) * iMaxBufNum);

  if(!pool->bufs)
    return false;

  pool->lock = Rtos_CreateMutex();

  if(!pool->lock)
  {
    Rtos_Free(pool->bufs);
    return false;
  }

  pool->spaceAvailable = Rtos_CreateEvent(false);

  if(!pool->spaceAvailable)
  {
    Rtos_Free(pool->bufs);
    Rtos_DeleteMutex(pool->bufs);
    return false;
  }
  pool->capacity = iMaxBufNum;
  pool->size = 0;
  pool->head = 0;
  return true;
}

void AL_WorkPool_Deinit(WorkPool* pool)
{
  Rtos_Free(pool->bufs);
  Rtos_DeleteMutex(pool->lock);
  Rtos_DeleteEvent(pool->spaceAvailable);
}

void AL_WorkPool_Remove(WorkPool* pool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pool->lock);
  int i;

  for(i = pool->head; i < pool->head + pool->size; ++i)
  {
    if(pool->bufs[i % pool->capacity] == pBuf)
    {
      pool->bufs[i % pool->capacity] = NULL;
      break;
    }
  }

  AL_Assert(i < pool->head + pool->size);

  AL_TBuffer* curBuf = pool->bufs[pool->head];

  while(curBuf == NULL && pool->size != 0)
  {
    pool->head = (pool->head + 1) % pool->capacity;
    --pool->size;
    curBuf = pool->bufs[pool->head];
  }

  if(pool->size != pool->capacity)
    Rtos_SetEvent(pool->spaceAvailable);

  Rtos_ReleaseMutex(pool->lock);
}

void AL_WorkPool_PushBack(WorkPool* pool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pool->lock);

  while(pool->size == pool->capacity)
  {
    Rtos_ReleaseMutex(pool->lock);
    Rtos_WaitEvent(pool->spaceAvailable, AL_WAIT_FOREVER);
    Rtos_GetMutex(pool->lock);
  }

  int index = (pool->head + pool->size) % pool->capacity;
  pool->bufs[index] = pBuf;
  ++pool->size;
  Rtos_ReleaseMutex(pool->lock);
}
