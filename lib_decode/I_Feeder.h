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
#include "lib_common/BufCommonInternal.h"

typedef struct AL_s_TFeeder AL_TFeeder;

typedef struct
{
  void (* pfnFeederDestroy)(AL_TFeeder* pFeeder);
  bool (* pfnFeederPushBuffer)(AL_TFeeder* pFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer);
  void (* pfnFeederSignal)(AL_TFeeder* pFeeder);
  void (* pfnFeederFlush)(AL_TFeeder* pFeeder);
  void (* pfnFeederReset)(AL_TFeeder* pFeeder);
  void (* pfnFeederFreeBuf)(AL_TFeeder* pFeeder, AL_TBuffer* pBuf);
}AL_TFeederVtable;

struct AL_s_TFeeder
{
  AL_TFeederVtable const* vtable;
};

static inline void AL_Feeder_Destroy(AL_TFeeder* pFeeder)
{
  pFeeder->vtable->pfnFeederDestroy(pFeeder);
}

/* push a buffer in the queue. it will be fed to the decoder when possible */
static inline bool AL_Feeder_PushBuffer(AL_TFeeder* pFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer)
{
  return pFeeder->vtable->pfnFeederPushBuffer(pFeeder, pBuf, uSize, bLastBuffer);
}

/* tell the buffer queue that the decoder finished decoding a frame */
static inline void AL_Feeder_Signal(AL_TFeeder* pFeeder)
{
  pFeeder->vtable->pfnFeederSignal(pFeeder);
}

/* flush decoder */
static inline void AL_Feeder_Flush(AL_TFeeder* pFeeder)
{
  pFeeder->vtable->pfnFeederFlush(pFeeder);
}

/* make decoder ready for next sequence */
static inline void AL_Feeder_Reset(AL_TFeeder* pFeeder)
{
  pFeeder->vtable->pfnFeederReset(pFeeder);
}

static inline void AL_Feeder_FreeBuf(AL_TFeeder* pFeeder, AL_TBuffer* pBuf)
{
  pFeeder->vtable->pfnFeederFreeBuf(pFeeder, pBuf);
}

