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

#include "BufferFeeder.h"
#include "lib_rtos/lib_rtos.h"

static void notifyDecoder(AL_TBufferFeeder* this)
{
  AL_DecoderFeeder_Process(this->decoderFeeder);
}

static bool enqueueBuffer(AL_TBufferFeeder* this, AL_TBuffer* pBuf, AL_EBufMode eMode)
{
  AL_Buffer_Ref(pBuf);

  if(!AL_Fifo_Queue(&this->fifo, pBuf, AL_GetWaitMode(eMode)))
  {
    AL_Buffer_Unref(pBuf);
    return false;
  }
  return true;
}

bool AL_BufferFeeder_PushBuffer(AL_TBufferFeeder* this, AL_TBuffer* pBuf, AL_EBufMode eMode, size_t uSize)
{
  AL_TMetaData* pMetaCirc = (AL_TMetaData*)AL_CircMetaData_Create(0, uSize);

  if(!pMetaCirc)
    return false;

  if(!AL_Buffer_AddMetaData(pBuf, pMetaCirc))
  {
    Rtos_Free(pMetaCirc);
    return false;
  }

  if(!enqueueBuffer(this, pBuf, eMode))
    return false;

  notifyDecoder(this);

  return true;
}

/* called when the decoder has finished to decode a frame */
void AL_BufferFeeder_Signal(AL_TBufferFeeder* this)
{
  notifyDecoder(this);
}

void AL_BufferFeeder_Flush(AL_TBufferFeeder* this)
{
  if(this->eosBuffer)
    AL_BufferFeeder_PushBuffer(this, this->eosBuffer, AL_BUF_MODE_BLOCK, AL_Buffer_GetSizeData(this->eosBuffer));

  AL_DecoderFeeder_Flush(this->decoderFeeder);
}

void AL_BufferFeeder_ForceStop(AL_TBufferFeeder* this)
{
  this->eosBuffer = NULL;
  AL_DecoderFeeder_ForceStop(this->decoderFeeder);
}

void AL_BufferFeeder_Destroy(AL_TBufferFeeder* this)
{
  AL_DecoderFeeder_Destroy(this->decoderFeeder);
  AL_Patchworker_Deinit(&this->patchworker);
  AL_Fifo_Deinit(&this->fifo);
  MemDesc_Free(&this->circularBuf.tMD);
  Rtos_Free(this);
}

AL_TBufferFeeder* AL_BufferFeeder_Create(AL_HANDLE hDec, AL_TAllocator* pAllocator, size_t zCircularBufferSize, AL_UINT uMaxBufNum)
{
  AL_TBufferFeeder* this = Rtos_Malloc(sizeof(*this));

  if(!this)
    return NULL;

  this->eosBuffer = NULL;

  if(!MemDesc_Alloc(&this->circularBuf.tMD, pAllocator, zCircularBufferSize))
    goto fail_circular_buffer_allocation;

  if(!AL_Patchworker_Init(&this->patchworker, &this->circularBuf))
    goto fail_patchworker_allocation;

  this->patchworker.inputFifo = &this->fifo;

  if(uMaxBufNum == 0 || !AL_Fifo_Init(&this->fifo, uMaxBufNum))
    goto fail_queue_allocation;

  this->decoderFeeder = AL_DecoderFeeder_Create(&this->circularBuf.tMD, hDec, &this->patchworker);

  if(!this->decoderFeeder)
    goto fail_decoder_feeder_creation;

  return this;

  fail_decoder_feeder_creation:
  AL_Fifo_Deinit(&this->fifo);
  fail_queue_allocation:
  AL_Patchworker_Deinit(&this->patchworker);
  fail_patchworker_allocation:
  MemDesc_Free(&this->circularBuf.tMD);
  fail_circular_buffer_allocation:
  Rtos_Free(this);
  return NULL;
}

