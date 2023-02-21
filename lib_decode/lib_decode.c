// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_decode/lib_decode.h"
#include "lib_decode/LibDecoderHost.h"

/* Initialize with decHost so AL_Lib_Decoder_Init() call is optionnal for the
 * time being.
 */
static AL_IDecArch* pArch = &decHost;

/*****************************************************************************/
AL_ERR AL_Lib_Decoder_Init(AL_ELibDecoderArch eArch)
{
  switch(eArch)
  {
  case AL_LIB_DECODER_ARCH_HOST:
    pArch = LibDecodeHostGet();
    return AL_SUCCESS;
  default:
    return AL_ERROR;
  }
}

/*****************************************************************************/
void AL_Lib_Decoder_DeInit()
{
  if(pArch)
    pArch->vtable->Deinit();
  pArch = NULL;
}

/*****************************************************************************/
AL_ERR AL_Decoder_Create(AL_HDecoder* hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->DecoderCreate(hDec, pScheduler, pAllocator, pSettings, pCB);
}

/*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec)
{
  if(!pArch)
    return;

  pArch->vtable->DecoderDestroy(hDec);
}

/*****************************************************************************/
void AL_Decoder_SetParam(AL_HDecoder hDec, const char* sPrefix, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool bShouldPrintFrameDelimiter)
{
  if(!pArch)
    return;

  pArch->vtable->DecoderSetParam(hDec, sPrefix, iFrmID, iNumFrm, bForceCleanBuffers,
                                 bShouldPrintFrameDelimiter);
}

/*****************************************************************************/
bool AL_Decoder_PushStreamBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags)
{
  if(!pArch)
    return false;

  return pArch->vtable->DecoderPushStreamBuffer(hDec, pBuf, uSize, uFlags);
}

/*****************************************************************************/
bool AL_Decoder_PushBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize)
{
  if(!pArch)
    return false;

  return pArch->vtable->DecoderPushBuffer(hDec, pBuf, uSize);
}

/*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec)
{
  if(!pArch)
    return;

  pArch->vtable->DecoderFlush(hDec);
}

/*****************************************************************************/
bool AL_Decoder_PutDisplayPicture(AL_HDecoder hDec, AL_TBuffer* pDisplay)
{
  if(!pArch)
    return false;

  return pArch->vtable->DecoderPutDisplayPicture(hDec, pDisplay);
}

/*****************************************************************************/
AL_ECodec AL_Decoder_GetCodec(AL_HDecoder hDec)
{
  if(!pArch)
    return AL_CODEC_INVALID;

  return pArch->vtable->DecoderGetCodec(hDec);
}

/*****************************************************************************/
int AL_Decoder_GetMaxBD(AL_HDecoder hDec)
{
  if(!pArch)
    return 0;

  return pArch->vtable->DecoderGetMaxBD(hDec);
}

/*****************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->DecoderGetLastError(hDec);
}

/*****************************************************************************/
AL_ERR AL_Decoder_GetFrameError(AL_HDecoder hDec, AL_TBuffer* pBuf)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->DecoderGetFrameError(hDec, pBuf);
}

/*****************************************************************************/
bool AL_Decoder_PreallocateBuffers(AL_HDecoder hDec)
{
  if(!pArch)
    return false;

  return pArch->vtable->DecoderPreallocateBuffers(hDec);
}

/*****************************************************************************/
uint32_t AL_Decoder_GetMinPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  if(!pArch)
    return 0;

  return pArch->vtable->DecoderGetMinPitch(uWidth, uBitDepth, eFrameBufferStorageMode);
}

/*****************************************************************************/
uint32_t AL_Decoder_GetMinStrideHeight(uint32_t uHeight)
{
  if(!pArch)
    return 0;

  return pArch->vtable->DecoderGetMinStrideHeight(uHeight);
}

