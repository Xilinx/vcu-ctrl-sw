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

#include "lib_rtos/lib_rtos.h"
#include "DecoderFeeder.h"
#include "lib_common/Error.h"
#include "lib_common/Utils.h"
#include "InternalError.h"

int AL_Decoder_GetStrOffset(AL_HANDLE hDec);
UNIT_ERROR AL_Decoder_TryDecodeOneUnit(AL_HDecoder hDec, TCircBuffer* pBufStream);
void AL_Decoder_InternalFlush(AL_HDecoder hDec);
void AL_Default_Decoder_WaitFrameSent(AL_HDecoder hDec);
void AL_Default_Decoder_ReleaseFrames(AL_HDecoder hDec);
void AL_Decoder_FlushInput(AL_HDecoder hDec);

typedef struct AL_TDecoderFeederS
{
  AL_HANDLE hDec;
  AL_TPatchworker* patchworker;
  /* set when the slave thread might have work to do */
  AL_EVENT incomingWorkEvent;

  AL_THREAD slave;
  TCircBuffer startCodeStreamView;
  int32_t keepGoing;
  bool stopped;
  bool endWithAccessUnit;
  AL_CB_Error errorCallback;
}AL_TDecoderFeeder;

/* Decoder Feeder Slave structure */
typedef AL_TDecoderFeeder DecoderFeederSlave;

static bool CircBuffer_IsFull(TCircBuffer* pBuf)
{
  return pBuf->iAvailSize == (int32_t)pBuf->tMD.uSize;
}

static bool shouldKeepGoing(AL_TDecoderFeeder* slave)
{
  int32_t keepGoing = Rtos_AtomicDecrement(&slave->keepGoing);
  Rtos_AtomicIncrement(&slave->keepGoing);
  return keepGoing >= 0 || !slave->endWithAccessUnit;
}

static bool Slave_Process(DecoderFeederSlave* slave, TCircBuffer* startCodeStreamView)
{
  AL_HANDLE hDec = slave->hDec;

  uint32_t uNewOffset = AL_Decoder_GetStrOffset(hDec);

  CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

  size_t transferedBytes = AL_Patchworker_Transfer(slave->patchworker);

  if(transferedBytes)
    Rtos_SetEvent(slave->incomingWorkEvent);

  startCodeStreamView->iAvailSize += transferedBytes;

  // Decode Max AU as possible with this data
  UNIT_ERROR eErr = SUCCESS_ACCESS_UNIT;

  while(eErr != ERR_UNIT_NOT_FOUND && shouldKeepGoing(slave))
  {
    eErr = AL_Decoder_TryDecodeOneUnit(hDec, startCodeStreamView);

    if(eErr == SUCCESS_ACCESS_UNIT)
      slave->endWithAccessUnit = true;

    if(eErr == SUCCESS_NAL_UNIT)
      slave->endWithAccessUnit = false;

    if(eErr != ERR_UNIT_NOT_FOUND)
      slave->stopped = false;

    if(eErr == ERR_UNIT_INVALID_CHANNEL || eErr == ERR_UNIT_DYNAMIC_ALLOC)
      return false;
  }

  if(CircBuffer_IsFull(slave->patchworker->outputCirc))
  {
    AL_Default_Decoder_WaitFrameSent(hDec);

    uint32_t uNewOffset = AL_Decoder_GetStrOffset(hDec);
    CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

    if(CircBuffer_IsFull(slave->patchworker->outputCirc))
    {
      // no more AU to get from a full circular buffer:
      // empty it to avoid a stall
      AL_Decoder_FlushInput(hDec);
    }

    AL_Default_Decoder_ReleaseFrames(hDec);
    Rtos_SetEvent(slave->incomingWorkEvent);
  }

  // Leave when end of input [all the data were processed in the previous TryDecodeOneUnit]
  if(AL_Patchworker_IsAllDataTransfered(slave->patchworker))
  {
    AL_Decoder_InternalFlush(slave->hDec);
    slave->stopped = true;
  }

  return true;
}

static void Slave_EntryPoint(AL_TDecoderFeeder* slave)
{
  while(1)
  {
    Rtos_WaitEvent(slave->incomingWorkEvent, AL_WAIT_FOREVER);

    if(!shouldKeepGoing(slave))
    {
      if(!slave->stopped)
        AL_Decoder_InternalFlush(slave->hDec);
      break;
    }

    bool bRet = Slave_Process(slave, &slave->startCodeStreamView);

    if(!bRet)
    {
      slave->errorCallback.func(slave->errorCallback.userParam);
      break; // exit thread
    }
  }
}

static bool CreateSlave(AL_TDecoderFeeder* this)
{
  this->slave = Rtos_CreateThread((void*)&Slave_EntryPoint, this);

  if(!this->slave)
    return false;
  return true;
}

static void DestroySlave(AL_TDecoderFeeder* this)
{
  if(!this->slave)
    return;

  Rtos_AtomicDecrement(&this->keepGoing); /* Will be propagated to the slave */
  Rtos_SetEvent(this->incomingWorkEvent);

  Rtos_JoinThread(this->slave);
  Rtos_DeleteThread(this->slave);
}

void AL_DecoderFeeder_Destroy(AL_TDecoderFeeder* this)
{
  if(!this)
    return;
  DestroySlave(this);
  Rtos_DeleteEvent(this->incomingWorkEvent);
  Rtos_Free(this);
}

void AL_DecoderFeeder_Process(AL_TDecoderFeeder* this)
{
  Rtos_SetEvent(this->incomingWorkEvent);
}

void AL_DecoderFeeder_Flush(AL_TDecoderFeeder* this)
{
  AL_Patchworker_NotifyEndOfInput(this->patchworker);
  Rtos_SetEvent(this->incomingWorkEvent);
}

void AL_DecoderFeeder_Reset(AL_TDecoderFeeder* this)
{
  AL_Patchworker_Reset(this->patchworker);
  CircBuffer_Init(&this->startCodeStreamView);
}

AL_TDecoderFeeder* AL_DecoderFeeder_Create(TMemDesc* streamMemory, AL_HANDLE hDec, AL_TPatchworker* patchworker, AL_CB_Error* errorCallback)
{
  AL_TDecoderFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  if(!patchworker)
    goto cleanup;

  this->patchworker = patchworker;
  this->errorCallback = *errorCallback;

  CircBuffer_Init(&this->startCodeStreamView);
  this->startCodeStreamView.tMD = *streamMemory;

  this->incomingWorkEvent = Rtos_CreateEvent(false);

  if(!this->incomingWorkEvent)
    goto cleanup;

  this->keepGoing = 1;
  this->stopped = true;
  this->endWithAccessUnit = true;
  this->hDec = hDec;

  if(!CreateSlave(this))
    goto cleanup;

  return this;

  cleanup:
  Rtos_DeleteEvent(this->incomingWorkEvent);
  Rtos_Free(this);
  return NULL;
}

