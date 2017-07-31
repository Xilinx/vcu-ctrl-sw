/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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
#include <stdlib.h> // for 'exit'

int AL_Decoder_GetStrOffset(AL_HANDLE hDec);
AL_ERR AL_Decoder_TryDecodeOneAU(AL_HDecoder hDec, TCircBuffer* pBufStream);
void AL_Decoder_InternalFlush(AL_HDecoder hDec);

typedef struct AL_TDecoderFeederS
{
  AL_HANDLE hDec;
  AL_TPatchworker* patchworker;
  /* set when the slave thread might have work to do */
  AL_EVENT incomingWorkEvent;

  AL_THREAD slave;
  TCircBuffer decodeBuffer;
  bool keepGoing;
  AL_MUTEX lock;
  bool flushed;
}AL_TDecoderFeeder;

/* Decoder Feeder Slave structure */
typedef AL_TDecoderFeeder DecoderFeederSlave;

static
bool CircBuffer_IsFull(TCircBuffer* pBuf)
{
  return pBuf->uAvailSize == pBuf->tMD.uSize;
}

static bool Slave_Process(DecoderFeederSlave* slave, TCircBuffer* decodeBuffer)
{
  do
  {
    AL_HANDLE hDec = slave->hDec;

    if(AL_Patchworker_ShouldBeStopped(slave->patchworker))
    {
      AL_Patchworker_Drop(slave->patchworker);
      AL_Decoder_InternalFlush(slave->hDec);
      slave->flushed = true;
      break;
    }

    uint32_t uNewOffset = AL_Decoder_GetStrOffset(hDec);

    CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

    decodeBuffer->uAvailSize += AL_Patchworker_Transfer(slave->patchworker);

    // Decode Max AU as possible with this data
    AL_ERR eErr = AL_SUCCESS;

    while(eErr != ERR_NO_FRAME_DECODED)
    {
      eErr = AL_Decoder_TryDecodeOneAU(hDec, decodeBuffer);

      if(eErr == ERR_INIT_FAILED)
        return false;
    }

    if(CircBuffer_IsFull(slave->patchworker->outputCirc))
    {
      // no more AU to get from a full circular buffer:
      // empty it to avoid a stall
      CircBuffer_Init(slave->patchworker->outputCirc);
    }

    // Leave when end of input [all the data were processed in the previous TryDecodeOneAU]
    if(AL_Patchworker_IsAllDataTransfered(slave->patchworker))
    {
      AL_Decoder_InternalFlush(slave->hDec);
      slave->flushed = true;
      break;
    }
  }
  while(AL_Patchworker_IsEndOfInput(slave->patchworker));

  return true;
}

static bool shouldKeepGoing(AL_TDecoderFeeder* slave)
{
  bool keepGoing;
  Rtos_GetMutex(slave->lock);
  keepGoing = slave->keepGoing;
  Rtos_ReleaseMutex(slave->lock);
  return keepGoing;
}

static void Slave_EntryPoint(AL_TDecoderFeeder* slave)
{
  while(1)
  {
    Rtos_WaitEvent(slave->incomingWorkEvent, AL_WAIT_FOREVER);

    if(!shouldKeepGoing(slave))
      break;

    bool bRet = true;

    if(!slave->flushed)
      bRet = Slave_Process(slave, &slave->decodeBuffer);

    if(!bRet)
    {
      exit(99); // workaround to avoid a deadlock. TODO: use an error callback
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
  if(this->slave)
  {
    Rtos_GetMutex(this->lock);
    this->keepGoing = false; /* Will be propagated to the slave */
    Rtos_ReleaseMutex(this->lock);
    Rtos_SetEvent(this->incomingWorkEvent);

    Rtos_JoinThread(this->slave);
    Rtos_DeleteThread(this->slave);
  }
}

void AL_DecoderFeeder_Destroy(AL_TDecoderFeeder* this)
{
  if(this)
  {
    DestroySlave(this);
    Rtos_DeleteEvent(this->incomingWorkEvent);
    Rtos_DeleteMutex(this->lock);
    Rtos_Free(this);
  }
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

void AL_DecoderFeeder_ForceStop(AL_TDecoderFeeder* this)
{
  AL_Patchworker_NotifyForceStop(this->patchworker);
  Rtos_SetEvent(this->incomingWorkEvent);
}

AL_TDecoderFeeder* AL_DecoderFeeder_Create(TMemDesc* decodeMemoryDescriptor, AL_HANDLE hDec, AL_TPatchworker* patchworker)
{
  AL_TDecoderFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  if(!patchworker)
    goto cleanup;

  this->patchworker = patchworker;

  CircBuffer_Init(&this->decodeBuffer);
  this->decodeBuffer.tMD = *decodeMemoryDescriptor;

  this->incomingWorkEvent = Rtos_CreateEvent(false);

  if(!this->incomingWorkEvent)
    goto cleanup;

  this->keepGoing = true;
  this->lock = Rtos_CreateMutex(false);
  this->flushed = false;
  this->hDec = hDec;

  if(!CreateSlave(this))
    goto cleanup;

  return this;

  cleanup:
  Rtos_DeleteEvent(this->incomingWorkEvent);
  Rtos_Free(this);
  return NULL;
}

