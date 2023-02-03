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

