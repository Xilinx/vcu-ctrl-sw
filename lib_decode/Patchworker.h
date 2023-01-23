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

#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/Fifo.h"

#include "lib_common/BufCommonInternal.h"

#define AL_CIRCULAR_BUFFER_SIZE_MARGIN 1

typedef struct al_t_Patchworker
{
  bool endOfOutput;
  AL_MUTEX lock;
  AL_TFifo* inputFifo;
  AL_TBuffer* outputCirc;
  AL_TBuffer* workBuf;
  bool bCompleteFill;
}AL_TPatchworker;

/*
 * Copy the data from a buffer in the circular buffer. pCopiedSize is updated with the size of the chunk that could be copied
 * return the size of what couldn't be copied.
 */
size_t AL_Patchworker_CopyBuffer(AL_TPatchworker* pPatchworker, AL_TBuffer* pBuf, size_t* pCopiedSize);

/* Transfer as much data as possible from one buffer of the fifo to the circular buffer */
size_t AL_Patchworker_Transfer(AL_TPatchworker* pPatchworker);

bool AL_Patchworker_IsAllDataTransfered(AL_TPatchworker* pPatchworker);

void AL_Patchworker_Reset(AL_TPatchworker* pPatchworker);

void AL_Patchworker_Deinit(AL_TPatchworker* pPatchworker);
bool AL_Patchworker_Init(AL_TPatchworker* pPatchworker, AL_TBuffer* stream, AL_TFifo* pInputFifo);

