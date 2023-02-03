/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include "UnsplitBufferFeeder.h"
#include "lib_common/Fifo.h"
#include "lib_rtos/lib_rtos.h"

#include "Patchworker.h"

typedef struct al_t_UnsplitBufferFeeder
{
  AL_TFeederVtable const* vtable;
  AL_TFifo fifo;
  AL_TPatchworker patchworker;
  AL_TDecoderFeeder* decoderFeeder;

  AL_TBuffer* eosBuffer;
  bool bForceAccessUnitDestroy;
  bool eos;
}AL_TUnsplitBufferFeeder;

static void notifyDecoder(AL_TUnsplitBufferFeeder* this)
{
  AL_DecoderFeeder_Process(this->decoderFeeder);
}

static bool enqueueBuffer(AL_TUnsplitBufferFeeder* this, AL_TBuffer* pBuf)
{
  AL_Buffer_Ref(pBuf);

  if(!AL_Fifo_Queue(&this->fifo, pBuf, AL_WAIT_FOREVER))
  {
    AL_Buffer_Unref(pBuf);
    return false;
  }
  return true;
}

static bool pushBuffer(AL_TFeeder* hFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer)
{
  AL_TUnsplitBufferFeeder* this = (AL_TUnsplitBufferFeeder*)hFeeder;

  if(bLastBuffer && this->eos && pBuf == this->eosBuffer)
    return false;

  this->eos = bLastBuffer;

  AL_TMetaData* pMetaCirc = (AL_TMetaData*)AL_CircMetaData_Create(0, uSize, bLastBuffer);

  if(!pMetaCirc)
    return false;

  if(!AL_Buffer_AddMetaData(pBuf, pMetaCirc))
  {
    Rtos_Free(pMetaCirc);
    return false;
  }

  if(!enqueueBuffer(this, pBuf))
    return false;

  notifyDecoder(this);

  return true;
}

/* called when the decoder has finished to decode a frame */
static void signal(AL_TFeeder* hFeeder)
{
  AL_TUnsplitBufferFeeder* this = (AL_TUnsplitBufferFeeder*)hFeeder;
  notifyDecoder(this);
}

static void flush(AL_TFeeder* hFeeder)
{
  AL_TUnsplitBufferFeeder* this = (AL_TUnsplitBufferFeeder*)hFeeder;

  pushBuffer(hFeeder, this->eosBuffer, AL_Buffer_GetSize(this->eosBuffer), true);
}

static void reset(AL_TFeeder* hFeeder)
{
  AL_TUnsplitBufferFeeder* this = (AL_TUnsplitBufferFeeder*)hFeeder;
  AL_DecoderFeeder_Reset(this->decoderFeeder);
}

static void destroy(AL_TFeeder* hFeeder)
{
  AL_TUnsplitBufferFeeder* this = (AL_TUnsplitBufferFeeder*)hFeeder;

  if(this->bForceAccessUnitDestroy)
    pushBuffer(hFeeder, this->eosBuffer, AL_Buffer_GetSize(this->eosBuffer), false);
  AL_DecoderFeeder_Destroy(this->decoderFeeder);
  AL_Patchworker_Deinit(&this->patchworker);
  AL_Fifo_Deinit(&this->fifo);
  Rtos_Free(this);
}

static void freeBuf(AL_TFeeder* hFeeder, AL_TBuffer* pBuf)
{
  (void)hFeeder;
  (void)pBuf;
}

static const AL_TFeederVtable UnsplitBufferFeederVtable =
{
  &destroy,
  &pushBuffer,
  &signal,
  &flush,
  &reset,
  &freeBuf,
};

AL_TFeeder* AL_UnsplitBufferFeeder_Create(AL_HANDLE hDec, int iMaxBufNum, AL_TAllocator* pAllocator, int iBufferStreamSize, AL_TBuffer* eosBuffer, bool bForceAccessUnitDestroy)
{
  AL_TUnsplitBufferFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  AL_Assert(eosBuffer);

  this->vtable = &UnsplitBufferFeederVtable;
  this->eosBuffer = eosBuffer;
  this->bForceAccessUnitDestroy = bForceAccessUnitDestroy;
  this->eos = false;

  if(iMaxBufNum <= 0 || !AL_Fifo_Init(&this->fifo, iMaxBufNum))
    goto fail_queue_allocation;

  AL_TBuffer* stream = AL_Buffer_Create_And_AllocateNamed(pAllocator, iBufferStreamSize, NULL, "circular stream");

  if(!stream)
    goto fail_stream_allocation;

  if(!AL_Patchworker_Init(&this->patchworker, stream, &this->fifo))
    goto fail_patchworker_allocation;

  this->decoderFeeder = AL_DecoderFeeder_Create(stream, hDec, &this->patchworker);

  if(!this->decoderFeeder)
    goto fail_decoder_feeder_creation;

  return (AL_TFeeder*)this;

  fail_decoder_feeder_creation:
  AL_Patchworker_Deinit(&this->patchworker);
  fail_patchworker_allocation:
  AL_Buffer_Destroy(stream);
  fail_stream_allocation:
  AL_Fifo_Deinit(&this->fifo);
  fail_queue_allocation:
  Rtos_Free(this);
  return NULL;
}

