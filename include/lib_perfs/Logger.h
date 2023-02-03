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

#pragma once

#include "lib_rtos/lib_rtos.h"

typedef struct
{
  uint64_t timestamp;
  char label[32];
}LogEvent;

typedef struct AL_t_Timer AL_Timer;
typedef struct
{
  uint32_t (* pfnGetTime)(AL_Timer* timer);
}AL_TimerVtable;

struct AL_t_Timer
{
  const AL_TimerVtable* vtable;
};

typedef struct
{
  const AL_TimerVtable* vtable;
}AL_CpuTimer;
AL_Timer* AL_CpuTimerInit(AL_CpuTimer* timer);
extern AL_CpuTimer g_CpuTimer;

typedef struct
{
  AL_Timer* timer;
  LogEvent* events;
  int count;
  int maxCount;
  AL_MUTEX mutex;
}AL_Logger;

static inline uint32_t AL_Timer_GetTime(AL_Timer* timer)
{
  return timer->vtable->pfnGetTime(timer);
}

extern AL_Logger g_Logger;

void AL_LoggerInit(AL_Logger* logger, AL_Timer* timer, LogEvent* buffer, int maxCount);
void AL_LoggerDeinit(AL_Logger* logger);
void AL_Log(AL_Logger* logger, const char* label);

