/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "DecoderFeeder.h"
#include "InternalError.h"

#include "lib_rtos/lib_rtos.h"

void AL_Default_Decoder_WaitFrameSent(AL_HDecoder hDec, uint32_t uStreamOffset);

extern UNIT_ERROR AL_Decoder_TryDecodeOneUnit(AL_HDecoder hDec, AL_TBuffer* pBufStream);
extern int AL_Decoder_GetDecodedStrOffset(AL_HANDLE hDec);
extern int AL_Decoder_SkipParsedUnits(AL_HANDLE hDec);
extern void AL_Decoder_FlushInput(AL_HDecoder hDec);
extern void AL_Decoder_InternalFlush(AL_HDecoder hDec);

typedef struct AL_TDecoderFeederS
{
  AL_HANDLE hDec;
  AL_TPatchworker* patchworker;
  /* set when the slave thread might have work to do */
  AL_SEMAPHORE incomingWorkSem;

  AL_THREAD slave;
  AL_TBuffer* startCodeStreamView;
  Rtos_AtomicInt keepGoing;
  bool decoderHasBeenFlushed;
  bool endWithAccessUnit;
}AL_TDecoderFeeder;

/* Decoder Feeder Slave structure */
typedef AL_TDecoderFeeder DecoderFeederSlave;

static bool CircBuffer_IsFull(AL_TBuffer* pBuf)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_CIRCULAR);
  return pMeta->iAvailSize == ((int32_t)AL_Buffer_GetSize(pBuf) - AL_CIRCULAR_BUFFER_SIZE_MARGIN);
}

static bool shouldKeepGoing(AL_TDecoderFeeder* slave)
{
  Rtos_AtomicInt keepGoing = Rtos_AtomicDecrement(&slave->keepGoing);
  Rtos_AtomicIncrement(&slave->keepGoing);
  return keepGoing >= 0 || !slave->endWithAccessUnit;
}

static bool Slave_Process(DecoderFeederSlave* slave, AL_TBuffer* startCodeStreamView)
{
  AL_HANDLE hDec = slave->hDec;
  bool bIsIncomingWorkSignaled = false;

  uint32_t uNewOffset = AL_Decoder_GetDecodedStrOffset(hDec);

  CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

  uint32_t uTransferedBytes = AL_Patchworker_Transfer(slave->patchworker);

  if(uTransferedBytes)
  {
    bIsIncomingWorkSignaled = true;
    Rtos_ReleaseSemaphore(slave->incomingWorkSem);
  }

  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(startCodeStreamView, AL_META_TYPE_CIRCULAR);
  pMeta->iAvailSize += uTransferedBytes;
  pMeta->bLastBuffer = AL_Patchworker_IsAllDataTransfered(slave->patchworker);

  // Decode
  UNIT_ERROR const eErr = AL_Decoder_TryDecodeOneUnit(hDec, startCodeStreamView);

  if(eErr == ERR_UNIT_INVALID_CHANNEL || eErr == ERR_UNIT_DYNAMIC_ALLOC)
    return false;

  if(eErr == SUCCESS_ACCESS_UNIT || eErr == SUCCESS_NAL_UNIT)
  {
    slave->decoderHasBeenFlushed = false;
    slave->endWithAccessUnit = (eErr == SUCCESS_ACCESS_UNIT);

    if(!bIsIncomingWorkSignaled)
    {
      bIsIncomingWorkSignaled = true;
      Rtos_ReleaseSemaphore(slave->incomingWorkSem);
    }
  }

  if(eErr == ERR_UNIT_NOT_FOUND)
  {
    if(CircBuffer_IsFull(slave->patchworker->outputCirc))
    {
      AL_Default_Decoder_WaitFrameSent(hDec, uNewOffset);

      uNewOffset = AL_Decoder_GetDecodedStrOffset(hDec);
      CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

      if(CircBuffer_IsFull(slave->patchworker->outputCirc))
      {
        // No more on-going unit -> consume up to first unprocessed NAL
        uNewOffset = AL_Decoder_SkipParsedUnits(hDec);
        CircBuffer_ConsumeUpToOffset(slave->patchworker->outputCirc, uNewOffset);

        if(CircBuffer_IsFull(slave->patchworker->outputCirc))
        {
          // No more AU to get from a full circular buffer -> empty it to avoid a stall
          AL_Decoder_FlushInput(hDec);
        }
      }

      if(!bIsIncomingWorkSignaled)
      {
        bIsIncomingWorkSignaled = true;
        Rtos_ReleaseSemaphore(slave->incomingWorkSem);
      }
    }

    // Leave when end of input [all the data were processed in the previous TryDecodeOneUnit]
    if(AL_Patchworker_IsAllDataTransfered(slave->patchworker))
    {
      AL_Decoder_InternalFlush(slave->hDec);
      slave->decoderHasBeenFlushed = true;
    }
  }

  return true;
}

static void* Slave_EntryPoint(void* userParam)
{
  Rtos_SetCurrentThreadName("DecFeeder");
  AL_TDecoderFeeder* slave = (AL_TDecoderFeeder*)userParam;

  while(1)
  {
    Rtos_GetSemaphore(slave->incomingWorkSem, AL_WAIT_FOREVER);

    if(!shouldKeepGoing(slave))
    {
      while(Rtos_GetSemaphore(slave->incomingWorkSem, AL_NO_WAIT))
      {
        bool bRet = Slave_Process(slave, slave->startCodeStreamView);

        if(!bRet)
        {
          Rtos_Log(AL_LOG_CRITICAL, "Slave_Process returned error.\n");
          break;
        }
      }

      if(!slave->decoderHasBeenFlushed)
        AL_Decoder_InternalFlush(slave->hDec);
      break;
    }

    bool bRet = Slave_Process(slave, slave->startCodeStreamView);

    if(!bRet)
    {
      Rtos_Log(AL_LOG_CRITICAL, "Slave_Process returned error. Exiting thread.\n");
      break; // exit thread
    }
  }

  return NULL;
}

static bool CreateSlave(AL_TDecoderFeeder* this)
{
  this->slave = Rtos_CreateThread(Slave_EntryPoint, this);

  if(!this->slave)
    return false;
  return true;
}

static void DestroySlave(AL_TDecoderFeeder* this)
{
  if(!this->slave)
    return;

  Rtos_AtomicDecrement(&this->keepGoing); /* Will be propagated to the slave */
  Rtos_ReleaseSemaphore(this->incomingWorkSem);

  Rtos_JoinThread(this->slave);
  Rtos_DeleteThread(this->slave);
}

void AL_DecoderFeeder_Destroy(AL_TDecoderFeeder* this)
{
  if(!this)
    return;
  DestroySlave(this);
  Rtos_DeleteSemaphore(this->incomingWorkSem);
  AL_Buffer_Destroy(this->startCodeStreamView);
  Rtos_Free(this);
}

void AL_DecoderFeeder_Process(AL_TDecoderFeeder* this)
{
  Rtos_ReleaseSemaphore(this->incomingWorkSem);
}

void AL_DecoderFeeder_Reset(AL_TDecoderFeeder* this)
{
  AL_Patchworker_Reset(this->patchworker);
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(this->startCodeStreamView, AL_META_TYPE_CIRCULAR);
  pMeta->iOffset = 0;
  pMeta->iAvailSize = 0;
  pMeta->bLastBuffer = false;
}

AL_TDecoderFeeder* AL_DecoderFeeder_Create(AL_TBuffer* stream, AL_HANDLE hDec, AL_TPatchworker* patchworker)
{
  AL_TDecoderFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  this->patchworker = patchworker;

  this->startCodeStreamView = stream;
  AL_TCircMetaData* pMeta = AL_CircMetaData_Create(0, 0, false);

  if(!pMeta)
    goto fail_;

  if(!AL_Buffer_AddMetaData(this->startCodeStreamView, (AL_TMetaData*)pMeta))
  {
    Rtos_Free(pMeta);
    goto fail_;
  }

  this->incomingWorkSem = Rtos_CreateSemaphore(0);

  if(!this->incomingWorkSem)
    goto fail_;

  this->keepGoing = 1;
  this->decoderHasBeenFlushed = false;
  this->endWithAccessUnit = true;
  this->hDec = hDec;

  if(!CreateSlave(this))
    goto cleanup;

  return this;

  cleanup:
  Rtos_DeleteSemaphore(this->incomingWorkSem);
  fail_:
  Rtos_Free(this);
  return NULL;
}

