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

#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/BufferCircMeta.h"
#include "lib_common/Fifo.h"

#include "lib_common_dec/DecBuffers.h"

typedef struct al_t_Patchworker
{
  bool endOfInput;
  bool endOfOutput;
  AL_MUTEX lock;
  AL_TFifo* inputFifo;
  TCircBuffer* outputCirc;
  AL_TBuffer* workBuf;
}AL_TPatchworker;

/*
 * Copy the data from a buffer in the circular buffer. pCopiedSize is updated with the size of the chunk that could be copied
 * return the size of what couldn't be copied.
 */
size_t AL_Patchworker_CopyBuffer(AL_TPatchworker* pPatchworker, AL_TBuffer* pBuf, size_t* pCopiedSize);

/* Transfer as much data as possible from one buffer of the fifo to the circular buffer */
size_t AL_Patchworker_Transfer(AL_TPatchworker* pPatchworker);

void AL_Patchworker_NotifyEndOfInput(AL_TPatchworker* pPatchworker);
bool AL_Patchworker_IsEndOfInput(AL_TPatchworker* pPatchworker);
bool AL_Patchworker_IsAllDataTransfered(AL_TPatchworker* pPatchworker);

void AL_Patchworker_Drop(AL_TPatchworker* pPatchworker);
void AL_Patchworker_Reset(AL_TPatchworker* pPatchworker);

void AL_Patchworker_Deinit(AL_TPatchworker* pPatchworker);
bool AL_Patchworker_Init(AL_TPatchworker* pPatchworker, TCircBuffer* pCircularBuf, AL_TFifo* pInputFifo);

