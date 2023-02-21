// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

