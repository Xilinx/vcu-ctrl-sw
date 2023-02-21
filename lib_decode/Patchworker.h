// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

