// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_decode/lib_decode.h"
#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_decode/LibDecoderHost.h"
#include "lib_common_dec/DecOutputSettingsInternal.h"

/* Initialize with decHost so AL_Lib_Decoder_Init() call is optional for the
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
void AL_Lib_Decoder_DeInit(void)
{
  if(pArch)
    pArch->vtable->Deinit();
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
void AL_Decoder_SetParam(AL_HDecoder hDec, const char* sPrefix, int32_t iFrmID, int32_t iNumFrm, bool bShouldPrintFrameDelimiter, bool bOutputTraceOnly)
{
  if(!pArch)
    return;

  pArch->vtable->DecoderSetParam(hDec, sPrefix, iFrmID, iNumFrm,
                                 bShouldPrintFrameDelimiter,
                                 bOutputTraceOnly);
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

bool AL_Decoder_ConfigureOutputSettings(AL_HDecoder hDec, AL_TDecOutputSettings const* pDecOutputSettings)
{
  if(!pArch)
    return false;
  return pArch->vtable->DecoderSetDecOutputSettings(hDec, pDecOutputSettings);
}

/*****************************************************************************/
AL_ECodec AL_Decoder_GetCodec(AL_HDecoder hDec)
{
  if(!pArch)
    return AL_CODEC_INVALID;

  return pArch->vtable->DecoderGetCodec(hDec);
}

/*****************************************************************************/
int32_t AL_Decoder_GetMaxBD(AL_HDecoder hDec)
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
AL_ERR AL_Decoder_GetFrameError(AL_HDecoder hDec, AL_TBuffer const* pBuf)
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
int32_t AL_Decoder_GetMinPitch(int32_t iWidth, AL_TPicFormat const* pPicFormat)
{
  return AL_DecGetLumaPixPlanePitch(iWidth, pPicFormat);
}

/*****************************************************************************/
int32_t AL_Decoder_GetMinStrideHeight(int32_t iHeight, AL_TPicFormat const* pPicFormat)
{
  return AL_DecGetPixPlaneHeight(iHeight, pPicFormat);
}

