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

