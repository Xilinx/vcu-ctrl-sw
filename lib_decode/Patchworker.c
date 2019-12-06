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

#include "Patchworker.h"
#include "lib_common/Utils.h"
#include <assert.h>

static int32_t GetBufferOffset(AL_TCircMetaData* pMeta)
{
  if(!pMeta)
    return 0;
  return pMeta->iOffset;
}

static size_t GetCopiedAreaSize(AL_TBuffer* pBuf, AL_TCircMetaData* pMeta, AL_TBuffer* stream, AL_TCircMetaData* pStreamMeta)
{
  /* This is required to be able to write the last NAL EOS, or at least to prevent
   * the offsets / available size to overlap
   * we keep a chunk of memory always free after the used area to be able to write
   * the EOS at any moment.
   */
  size_t unusedAreaSize = AL_Buffer_GetSize(stream) - pStreamMeta->iAvailSize;

  /* if no metadata, assume offset = 0 and available size = buffer size */
  if(!pMeta)
    return UnsignedMin(AL_Buffer_GetSize(pBuf), unusedAreaSize);
  return UnsignedMin(pMeta->iAvailSize, unusedAreaSize);
}

static size_t GetNotCopiedAreaSize(AL_TBuffer* pBuf, AL_TCircMetaData* pMeta, size_t zCopiedSize)
{
  if(!pMeta)
    return AL_Buffer_GetSize(pBuf) - zCopiedSize;
  return pMeta->iAvailSize - zCopiedSize;
}

static void CopyAreaToStream(uint8_t* pData, uint32_t uOffset, size_t zCopySize, AL_TBuffer* stream, AL_TCircMetaData* pStreamMeta)
{
  size_t sStreamSize = AL_Buffer_GetSize(stream);
  uint32_t uEndStream = (pStreamMeta->iOffset + pStreamMeta->iAvailSize) % sStreamSize;
  uint8_t* pStreamData = AL_Buffer_GetData(stream);

  if(uEndStream + zCopySize > sStreamSize)
  {
    uint32_t SpaceLeftBeforeWrapping = sStreamSize - uEndStream;
    Rtos_Memcpy(pStreamData + uEndStream, pData + uOffset, SpaceLeftBeforeWrapping);
    Rtos_Memcpy(pStreamData, pData + uOffset + SpaceLeftBeforeWrapping, zCopySize - SpaceLeftBeforeWrapping);
    return;
  }

  Rtos_Memcpy(pStreamData + uEndStream, pData + uOffset, zCopySize);
}

static size_t TryCopyBufferToStream(AL_TBuffer* pBuf, AL_TCircMetaData* pMeta, AL_TBuffer* stream)
{
  uint32_t uBufOffset = GetBufferOffset(pMeta);
  AL_TCircMetaData* pStreamMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(stream, AL_META_TYPE_CIRCULAR);
  size_t zCopySize = GetCopiedAreaSize(pBuf, pMeta, stream, pStreamMeta);

  /* nothing to do, and we don't want to call memcpy on a potentially
   * invalid pointer with size 0 (undefined behavior) */
  if(zCopySize == 0)
    return 0;

  CopyAreaToStream(AL_Buffer_GetData(pBuf), uBufOffset, zCopySize, stream, pStreamMeta);

  pStreamMeta->iAvailSize += zCopySize;

  return zCopySize;
}

static void updateConsumedSpaceInBuffer(AL_TBuffer* pBuf, size_t zCopiedSize, size_t zNotCopiedSize)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_CIRCULAR);

  /* support for samples without meta-data */
  if(!pMeta)
  {
    pMeta = AL_CircMetaData_Create(0, 0, false);
    AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pMeta);
  }

  pMeta->iOffset += zCopiedSize;
  pMeta->iAvailSize = zNotCopiedSize;
}

size_t AL_Patchworker_CopyBuffer(AL_TPatchworker* this, AL_TBuffer* pBuf, size_t* pCopiedSize)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_CIRCULAR);
  size_t zCopiedSize = TryCopyBufferToStream(pBuf, pMeta, this->outputCirc);

  size_t zNotCopiedSize = GetNotCopiedAreaSize(pBuf, pMeta, zCopiedSize);

  if(zNotCopiedSize != 0)
  {
    updateConsumedSpaceInBuffer(pBuf, zCopiedSize, zNotCopiedSize);
  }
  else if(zNotCopiedSize == 0 && pMeta)
  {
    if(pMeta->bLastBuffer)
      this->endOfOutput = true;
    AL_TMetaData* m = (AL_TMetaData*)pMeta;
    AL_Buffer_RemoveMetaData(pBuf, m);
    AL_MetaData_Destroy(m);
  }

  *pCopiedSize = zCopiedSize;

  return zNotCopiedSize;
}

bool AL_Patchworker_Init(AL_TPatchworker* this, AL_TBuffer* stream, AL_TFifo* pInputFifo)
{
  this->endOfInput = false;
  this->endOfOutput = false;
  this->lock = Rtos_CreateMutex();
  this->workBuf = NULL;
  this->inputFifo = pInputFifo;

  this->outputCirc = AL_Buffer_WrapData(AL_Buffer_GetData(stream), AL_Buffer_GetSize(stream), NULL);

  if(!this->outputCirc)
    return false;

  AL_TMetaData* pMeta = (AL_TMetaData*)AL_CircMetaData_Create(0, 0, false);

  if(!pMeta)
    goto cleanup;

  if(!AL_Buffer_AddMetaData(this->outputCirc, pMeta))
  {
    Rtos_Free(pMeta);
    goto cleanup;
  }
  return true;

  cleanup:
  AL_Buffer_Destroy(this->outputCirc);
  return false;
}

void AL_Patchworker_Deinit(AL_TPatchworker* this)
{
  if(this->workBuf)
    AL_Buffer_Unref(this->workBuf);

  this->workBuf = AL_Fifo_Dequeue(this->inputFifo, AL_NO_WAIT);

  while(this->workBuf)
  {
    AL_Buffer_Unref(this->workBuf);
    this->workBuf = AL_Fifo_Dequeue(this->inputFifo, AL_NO_WAIT);
  }

  AL_Buffer_Destroy(this->outputCirc);
  Rtos_DeleteMutex(this->lock);
}

size_t AL_Patchworker_Transfer(AL_TPatchworker* this)
{
  assert(this->inputFifo);

  if(!this->workBuf)
    this->workBuf = AL_Fifo_Dequeue(this->inputFifo, AL_NO_WAIT);

  if(!this->workBuf)
    return 0; /* no more input buffers in fifo */

  size_t zCopiedSize = 0;
  size_t zNotCopiedSize = AL_Patchworker_CopyBuffer(this, this->workBuf, &zCopiedSize);
  size_t zTotalSize = zCopiedSize;

  if(zNotCopiedSize != 0)
    return zTotalSize; /* no more free space in circular buffer */

  /* buffer is totally copied to the circular buffer and isn't needed anymore */
  AL_Buffer_Unref(this->workBuf);
  this->workBuf = NULL;

  return zTotalSize;
}

void AL_Patchworker_NotifyEndOfInput(AL_TPatchworker* this)
{
  Rtos_GetMutex(this->lock);
  this->endOfInput = true;
  Rtos_ReleaseMutex(this->lock);
}

bool AL_Patchworker_IsEndOfInput(AL_TPatchworker* this)
{
  Rtos_GetMutex(this->lock);
  bool bRet = this->endOfInput;
  Rtos_ReleaseMutex(this->lock);
  return bRet;
}

bool AL_Patchworker_IsAllDataTransfered(AL_TPatchworker* this)
{
  return this->endOfOutput;
}

void AL_Patchworker_Drop(AL_TPatchworker* this)
{
  if(!this->workBuf)
    this->workBuf = AL_Fifo_Dequeue(this->inputFifo, AL_NO_WAIT);

  if(!this->workBuf)
    return;

  while(this->workBuf)
  {
    AL_Buffer_Unref(this->workBuf);
    this->workBuf = AL_Fifo_Dequeue(this->inputFifo, AL_NO_WAIT);
  }
}

void AL_Patchworker_Reset(AL_TPatchworker* this)
{
  Rtos_GetMutex(this->lock);
  this->endOfOutput = false;
  this->endOfInput = false;
  AL_Patchworker_Drop(this);
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(this->outputCirc, AL_META_TYPE_CIRCULAR);
  pMeta->iOffset = 0;
  pMeta->iAvailSize = 0;
  Rtos_ReleaseMutex(this->lock);
}

