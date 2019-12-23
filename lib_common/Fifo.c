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

#include "Fifo.h"

bool AL_Fifo_Init(AL_TFifo* pFifo, size_t zMaxElem)
{
  pFifo->zMaxElem = zMaxElem + 1;
  pFifo->zTail = 0;
  pFifo->zHead = 0;

  size_t zElemSize = pFifo->zMaxElem * sizeof(void*);
  pFifo->ElemBuffer = (void**)Rtos_Malloc(zElemSize);

  if(!pFifo->ElemBuffer)
    return false;
  Rtos_Memset(pFifo->ElemBuffer, 0xCD, zElemSize);

  pFifo->hCountSem = Rtos_CreateSemaphore(0);

  if(!pFifo->hCountSem)
  {
    Rtos_Free(pFifo->ElemBuffer);
    return false;
  }

  pFifo->hSpaceSem = Rtos_CreateSemaphore(zMaxElem);
  pFifo->hMutex = Rtos_CreateMutex();

  if(!pFifo->hSpaceSem)
  {
    Rtos_DeleteSemaphore(pFifo->hCountSem);
    Rtos_Free(pFifo->ElemBuffer);
    return false;
  }

  return true;
}

void AL_Fifo_Deinit(AL_TFifo* pFifo)
{
  Rtos_Free(pFifo->ElemBuffer);
  Rtos_DeleteSemaphore(pFifo->hCountSem);
  Rtos_DeleteSemaphore(pFifo->hSpaceSem);
  Rtos_DeleteMutex(pFifo->hMutex);
}

bool AL_Fifo_Queue(AL_TFifo* pFifo, void* pElem, uint32_t uWait)
{
  if(!Rtos_GetSemaphore(pFifo->hSpaceSem, uWait))
    return false;

  Rtos_GetMutex(pFifo->hMutex);
  pFifo->ElemBuffer[pFifo->zTail] = pElem;
  pFifo->zTail = (pFifo->zTail + 1) % pFifo->zMaxElem;
  Rtos_ReleaseMutex(pFifo->hMutex);

  Rtos_ReleaseSemaphore(pFifo->hCountSem);

  /* new item was added in the queue */
  return true;
}

void* AL_Fifo_Dequeue(AL_TFifo* pFifo, uint32_t uWait)
{
  /* wait if no items */
  if(!Rtos_GetSemaphore(pFifo->hCountSem, uWait))
    return NULL;

  Rtos_GetMutex(pFifo->hMutex);
  void* pElem = pFifo->ElemBuffer[pFifo->zHead];
  pFifo->zHead = (pFifo->zHead + 1) % pFifo->zMaxElem;
  Rtos_ReleaseMutex(pFifo->hMutex);

  /* new empty space available */
  Rtos_ReleaseSemaphore(pFifo->hSpaceSem);
  return pElem;
}

