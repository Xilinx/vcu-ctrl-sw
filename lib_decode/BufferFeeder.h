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
#include "lib_common/Fifo.h"

#include "lib_decode/lib_decode.h"
#include "Patchworker.h"
#include "DecoderFeeder.h"

typedef struct al_t_BufferFeeder
{
  AL_TFifo fifo;
  AL_TPatchworker patchworker;
  AL_TDecoderFeeder* decoderFeeder;

  AL_TBuffer* eosBuffer;
}AL_TBufferFeeder;

AL_TBufferFeeder* AL_BufferFeeder_Create(AL_HANDLE hDec, TCircBuffer* circularBuf, int uMaxBufNum, AL_CB_Error* errorCallback);
void AL_BufferFeeder_Destroy(AL_TBufferFeeder* pFeeder);
/* push a buffer in the queue. it will be fed to the decoder when possible */
bool AL_BufferFeeder_PushBuffer(AL_TBufferFeeder* pFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer);
/* tell the buffer queue that the decoder finished decoding a frame */
void AL_BufferFeeder_Signal(AL_TBufferFeeder* pFeeder);
/* After telling the feeder that EOS is coming, wait for the decoder to consume all the buffers */
void AL_BufferFeeder_Flush(AL_TBufferFeeder* pFeeder);
/* Make decoder ready for next sequence */
void AL_BufferFeeder_Reset(AL_TBufferFeeder* pFeeder);

