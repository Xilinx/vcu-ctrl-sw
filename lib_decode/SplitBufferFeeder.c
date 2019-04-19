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

#include <assert.h>
#include "SplitBufferFeeder.h"
#include "InternalError.h"
#include "lib_decode/lib_decode.h"
#include "lib_common/Fifo.h"
#include "lib_common/BufferSeiMeta.h"

UNIT_ERROR AL_Decoder_TryDecodeOneUnit(AL_HDecoder hDec, AL_TBuffer* pBuf);
void AL_Decoder_InternalFlush(AL_HDecoder hDec);

typedef struct al_t_SplitBufferFeeder
{
  const AL_TFeederVtable* vtable;
  AL_HANDLE hDec;
  AL_TFifo inputFifo;
  AL_TFifo workFifo;

  AL_EVENT incomingWorkEvent;
  AL_THREAD process;
  int32_t keepGoing;
  AL_MUTEX lock;
  bool eos;
}AL_TSplitBufferFeeder;

static bool enqueueBuffer(AL_TSplitBufferFeeder* this, AL_TBuffer* pBuf)
{
  AL_Buffer_Ref(pBuf);

  if(!AL_Fifo_Queue(&this->inputFifo, pBuf, AL_WAIT_FOREVER))
  {
    AL_Buffer_Unref(pBuf);
    return false;
  }
  Rtos_AtomicIncrement(&this->keepGoing);
  return true;
}

static bool shouldKeepGoing(AL_TSplitBufferFeeder* this)
{
  int32_t keepGoing = Rtos_AtomicDecrement(&this->keepGoing);
  Rtos_AtomicIncrement(&this->keepGoing);
  return keepGoing >= 0;
}

static void signal(AL_TFeeder* hFeeder)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;
  Rtos_SetEvent(this->incomingWorkEvent);
}

static bool IsSuccess(UNIT_ERROR err)
{
  return (err == SUCCESS_ACCESS_UNIT) || (err == SUCCESS_NAL_UNIT);
}

static void Process(AL_TSplitBufferFeeder* this)
{
  AL_TBuffer* workBuf = AL_Fifo_Dequeue(&this->inputFifo, AL_NO_WAIT);

  if(!workBuf)
    return;

  Rtos_AtomicDecrement(&this->keepGoing);

  AL_HANDLE hDec = this->hDec;

  UNIT_ERROR err = AL_Decoder_TryDecodeOneUnit(hDec, workBuf);
  signal((AL_TFeeder*)this);

  if(!IsSuccess(err))
  {
    AL_Buffer_Unref(workBuf);
    return;
  }
  AL_Fifo_Queue(&this->workFifo, workBuf, AL_WAIT_FOREVER);
}

static bool IsEndOfStream(AL_TSplitBufferFeeder* this)
{
  Rtos_GetMutex(this->lock);
  bool bRet = this->eos;
  Rtos_ReleaseMutex(this->lock);
  return bRet;
}

static void Process_EntryPoint(AL_TSplitBufferFeeder* this)
{
  Rtos_SetCurrentThreadName("DecFeeder");

  while(1)
  {
    Rtos_WaitEvent(this->incomingWorkEvent, AL_WAIT_FOREVER);

    if(!shouldKeepGoing(this) && IsEndOfStream(this))
    {
      AL_Decoder_InternalFlush(this->hDec);
      break;
    }

    Process(this);
  }
}

static void reset(AL_TFeeder* pFeeder)
{
  (void)pFeeder;
}

static bool pushBuffer(AL_TFeeder* hFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;
  AL_TCircMetaData* pMetaCirc = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_CIRCULAR);

  if(!pMetaCirc)
  {
    AL_TMetaData* pMeta = (AL_TMetaData*)AL_CircMetaData_Create(0, uSize, bLastBuffer);

    if(!pMeta)
      return false;

    if(!AL_Buffer_AddMetaData(pBuf, pMeta))
    {
      Rtos_Free(pMeta);
      return false;
    }
  }
  else
  {
    pMetaCirc->iOffset = 0;
    pMetaCirc->iAvailSize = uSize;
  }

  AL_TSeiMetaData* pSei = (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SEI);

  if(pSei)
    AL_SeiMetaData_Reset(pSei);

  if(!enqueueBuffer(this, pBuf))
    return false;

  signal(hFeeder);
  return true;
}

static void freeBuf(AL_TFeeder* hFeeder, AL_TBuffer* pBuf)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;
  AL_TBuffer* workBuf = AL_Fifo_Dequeue(&this->workFifo, AL_NO_WAIT);

  while(workBuf)
  {
    AL_Buffer_Unref(workBuf);

    if(workBuf == pBuf)
      break;
    workBuf = AL_Fifo_Dequeue(&this->workFifo, AL_NO_WAIT);
  }
}

static void flush(AL_TFeeder* hFeeder)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;
  Rtos_GetMutex(this->lock);
  this->eos = true;
  Rtos_ReleaseMutex(this->lock);
  signal(hFeeder);
}

static void destroy_process(AL_TFeeder* hFeeder)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;

  if(!this->eos)
  {
    while(Rtos_AtomicDecrement(&this->keepGoing) > 0)
    {
    }

    AL_TBuffer* workBuf = AL_Fifo_Dequeue(&this->inputFifo, AL_NO_WAIT);

    while(workBuf)
    {
      AL_Buffer_Unref(workBuf);
      workBuf = AL_Fifo_Dequeue(&this->inputFifo, AL_NO_WAIT);
    }

    flush(hFeeder);
  }
  Rtos_JoinThread(this->process);
  Rtos_DeleteThread(this->process);
}

static void destroy(AL_TFeeder* hFeeder)
{
  AL_TSplitBufferFeeder* this = (AL_TSplitBufferFeeder*)hFeeder;

  if(this->process)
    destroy_process(hFeeder);

  Rtos_DeleteMutex(this->lock);
  Rtos_DeleteEvent(this->incomingWorkEvent);
  assert(!AL_Fifo_Dequeue(&this->workFifo, AL_NO_WAIT));
  AL_Fifo_Deinit(&this->workFifo);
  AL_Fifo_Deinit(&this->inputFifo);
  Rtos_Free(this);
}

static bool CreateProcess(AL_TSplitBufferFeeder* this)
{
  this->process = Rtos_CreateThread((void*)&Process_EntryPoint, this);

  if(!this->process)
    return false;
  return true;
}

static const AL_TFeederVtable SplitBufferFeederVtable =
{
  &destroy,
  &pushBuffer,
  &signal,
  &flush,
  &reset,
  &freeBuf,
};

AL_TFeeder* AL_SplitBufferFeeder_Create(AL_HANDLE hDec, int iMaxBufNum)
{
  AL_TSplitBufferFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  this->vtable = &SplitBufferFeederVtable;
  this->hDec = hDec;
  this->eos = false;
  this->keepGoing = 0;

  if(iMaxBufNum <= 0 || !AL_Fifo_Init(&this->inputFifo, iMaxBufNum))
    goto fail_queue_allocation;

  if(iMaxBufNum <= 0 || !AL_Fifo_Init(&this->workFifo, iMaxBufNum))
    goto fail_work_queue_allocation;

  this->incomingWorkEvent = Rtos_CreateEvent(false);

  if(!this->incomingWorkEvent)
    goto fail_event;

  this->lock = Rtos_CreateMutex();

  if(!this->lock)
    goto fail_mutex;

  if(!CreateProcess(this))
    goto cleanup;

  return (AL_TFeeder*)this;

  cleanup:
  Rtos_DeleteMutex(this->lock);
  fail_mutex:
  Rtos_DeleteEvent(this->incomingWorkEvent);
  fail_event:
  AL_Fifo_Deinit(&this->workFifo);
  fail_work_queue_allocation:
  AL_Fifo_Deinit(&this->inputFifo);
  fail_queue_allocation:
  Rtos_Free(this);
  return NULL;
}

