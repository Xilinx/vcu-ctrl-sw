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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

/****************************************************************************/
typedef struct AL_i_DecArchVtable AL_IDecArchVtable;

typedef struct AL_i_DecArch
{
  const AL_IDecArchVtable* vtable;
}AL_IDecArch;

typedef struct AL_i_DecArchVtable
{
  void (* Deinit)(void);
  AL_ERR (* DecoderCreate)(AL_HDecoder* hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);
  void (* DecoderDestroy)(AL_HDecoder hDec);
  void (* DecoderSetParam)(AL_HDecoder hDec, const char* sPrefix, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool bShouldPrintFrameDelimiter);
  bool (* DecoderPushStreamBuffer)(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags);
  bool (* DecoderPushBuffer)(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize);
  void (* DecoderFlush)(AL_HDecoder hDec);
  bool (* DecoderPutDisplayPicture)(AL_HDecoder hDec, AL_TBuffer* pDisplay);
  AL_ECodec (* DecoderGetCodec)(AL_HDecoder hDec);
  int (* DecoderGetMaxBD)(AL_HDecoder hDec);
  AL_ERR (* DecoderGetLastError)(AL_HDecoder hDec);
  AL_ERR (* DecoderGetFrameError)(AL_HDecoder hDec, AL_TBuffer* pBuf);
  bool (* DecoderPreallocateBuffers)(AL_HDecoder hDec);
  uint32_t (* DecoderGetMinPitch)(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);
  uint32_t (* DecoderGetMinStrideHeight)(uint32_t uHeight);
}AL_IDecArchVtable;

/*@}*/
