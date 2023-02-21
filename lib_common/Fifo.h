// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"

typedef struct
{
  size_t zMaxElem;
  size_t zTail;
  size_t zHead;
  void** ElemBuffer;
  AL_MUTEX hMutex;
  AL_SEMAPHORE hCountSem;
  AL_SEMAPHORE hSpaceSem;
}AL_TFifo;

bool AL_Fifo_Init(AL_TFifo* pFifo, size_t zMaxElem);
void AL_Fifo_Deinit(AL_TFifo* pFifo);
bool AL_Fifo_Queue(AL_TFifo* pFifo, void* pElem, uint32_t uWait);
void* AL_Fifo_Dequeue(AL_TFifo* pFifo, uint32_t uWait);

