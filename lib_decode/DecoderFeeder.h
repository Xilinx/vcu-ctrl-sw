// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufferAPI.h"

#include "lib_decode/lib_decode.h"
#include "Patchworker.h"
#include "lib_rtos/types.h"

typedef struct AL_TDecoderFeederS AL_TDecoderFeeder;

AL_TDecoderFeeder* AL_DecoderFeeder_Create(AL_TBuffer* stream, AL_HANDLE hDec, AL_TPatchworker* patchworker);
void AL_DecoderFeeder_Destroy(AL_TDecoderFeeder* pDecFeeder);
/* push a buffer in the queue. it will be fed to the decoder when possible */
void AL_DecoderFeeder_Process(AL_TDecoderFeeder* pDecFeeder);
void AL_DecoderFeeder_Reset(AL_TDecoderFeeder* pDecFeeder);

