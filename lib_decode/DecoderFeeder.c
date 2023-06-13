// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

static bool CreateSlave(AL_TDecoderFeeder* pDecFeeder)
{
  pDecFeeder->slave = Rtos_CreateThread(Slave_EntryPoint, pDecFeeder);

  if(!pDecFeeder->slave)
    return false;
  return true;
}

static void DestroySlave(AL_TDecoderFeeder* pDecFeeder)
{
  if(!pDecFeeder->slave)
    return;

  Rtos_AtomicDecrement(&pDecFeeder->keepGoing); /* Will be propagated to the slave */
  Rtos_ReleaseSemaphore(pDecFeeder->incomingWorkSem);

  Rtos_JoinThread(pDecFeeder->slave);
  Rtos_DeleteThread(pDecFeeder->slave);
}

void AL_DecoderFeeder_Destroy(AL_TDecoderFeeder* pDecFeeder)
{
  if(!pDecFeeder)
    return;
  DestroySlave(pDecFeeder);
  Rtos_DeleteSemaphore(pDecFeeder->incomingWorkSem);
  AL_Buffer_Destroy(pDecFeeder->startCodeStreamView);
  Rtos_Free(pDecFeeder);
}

void AL_DecoderFeeder_Process(AL_TDecoderFeeder* pDecFeeder)
{
  Rtos_ReleaseSemaphore(pDecFeeder->incomingWorkSem);
}

void AL_DecoderFeeder_Reset(AL_TDecoderFeeder* pDecFeeder)
{
  AL_Patchworker_Reset(pDecFeeder->patchworker);
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pDecFeeder->startCodeStreamView, AL_META_TYPE_CIRCULAR);
  pMeta->iOffset = 0;
  pMeta->iAvailSize = 0;
  pMeta->bLastBuffer = false;
}

AL_TDecoderFeeder* AL_DecoderFeeder_Create(AL_TBuffer* stream, AL_HANDLE hDec, AL_TPatchworker* patchworker)
{
  AL_TDecoderFeeder* pDecFeeder = Rtos_Malloc(sizeof(*pDecFeeder));

  if(!pDecFeeder)
    return NULL;

  pDecFeeder->patchworker = patchworker;

  pDecFeeder->startCodeStreamView = stream;
  AL_TCircMetaData* pMeta = AL_CircMetaData_Create(0, 0, false);

  if(!pMeta)
    goto fail_;

  if(!AL_Buffer_AddMetaData(pDecFeeder->startCodeStreamView, (AL_TMetaData*)pMeta))
  {
    Rtos_Free(pMeta);
    goto fail_;
  }

  pDecFeeder->incomingWorkSem = Rtos_CreateSemaphore(0);

  if(!pDecFeeder->incomingWorkSem)
    goto fail_;

  pDecFeeder->keepGoing = 1;
  pDecFeeder->decoderHasBeenFlushed = false;
  pDecFeeder->endWithAccessUnit = true;
  pDecFeeder->hDec = hDec;

  if(!CreateSlave(pDecFeeder))
    goto cleanup;

  return pDecFeeder;

  cleanup:
  Rtos_DeleteSemaphore(pDecFeeder->incomingWorkSem);
  fail_:
  Rtos_Free(pDecFeeder);
  return NULL;
}

