// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
