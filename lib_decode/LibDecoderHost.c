// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_decode/lib_decode.h"
#include "lib_decode/DefaultDecoder.h"
#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_decode/I_DecArch.h"

AL_ERR CreateAvcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);
AL_ERR CreateHevcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*****************************************************************************/
static AL_ERR AL_Decoder_Create_Host(AL_HDecoder* hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  if(!pSettings || !pCB || !pAllocator || !pScheduler || !hDec)
    return AL_ERROR;

  if(pSettings->eCodec == AL_CODEC_AVC)
    return CreateAvcDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  if(pSettings->eCodec == AL_CODEC_HEVC)
    return CreateHevcDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  return AL_ERROR;
}

/*****************************************************************************/
static AL_ECodec getCodec(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->ctx.pChanParam ? pDec->ctx.pChanParam->eCodec : AL_CODEC_INVALID;
}

/*****************************************************************************/
static void AL_Decoder_Destroy_Host(AL_HDecoder hDec)
{
  AL_Default_Decoder_Destroy((AL_TDecoder*)hDec);
}

/*****************************************************************************/
static void AL_Decoder_SetParam_Host(AL_HDecoder hDec, const char* sPrefix, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool bShouldPrintFrameDelimiter)
{
  AL_Default_Decoder_SetParam((AL_TDecoder*)hDec, sPrefix, iFrmID, iNumFrm, bForceCleanBuffers, bShouldPrintFrameDelimiter);
}

/*****************************************************************************/
static bool AL_Decoder_PushStreamBuffer_Host(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags)
{
  return AL_Default_Decoder_PushStreamBuffer((AL_TDecoder*)hDec, pBuf, uSize, uFlags);
}

/*****************************************************************************/
static bool AL_Decoder_PushBuffer_Host(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize)
{
  return AL_Default_Decoder_PushBuffer((AL_TDecoder*)hDec, pBuf, uSize);
}

/*****************************************************************************/
static void AL_Decoder_Flush_Host(AL_HDecoder hDec)
{
  AL_Default_Decoder_Flush((AL_TDecoder*)hDec);
}

/*****************************************************************************/
static bool AL_Decoder_PutDisplayPicture_Host(AL_HDecoder hDec, AL_TBuffer* pDisplay)
{
  return AL_Default_Decoder_PutDecPict((AL_TDecoder*)hDec, pDisplay);
}

/*****************************************************************************/
static int AL_Decoder_GetMaxBD_Host(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetMaxBD((AL_TDecoder*)hDec);
}

/*****************************************************************************/
static AL_ECodec AL_Decoder_GetCodec_Host(AL_HDecoder hDec)
{
  return getCodec(hDec);
}

/*****************************************************************************/
static AL_ERR AL_Decoder_GetLastError_Host(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetLastError((AL_TDecoder*)hDec);
}

/*****************************************************************************/
static AL_ERR AL_Decoder_GetFrameError_Host(AL_HDecoder hDec, AL_TBuffer* pBuf)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return AL_Default_Decoder_GetFrameError(pDec, pBuf);
}

/*****************************************************************************/
static bool AL_Decoder_PreallocateBuffers_Host(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return AL_Default_Decoder_PreallocateBuffers(pDec);
}

static uint32_t AL_Decoder_GetMinPitch_Host(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  return RndPitch(uWidth, uBitDepth, eFrameBufferStorageMode);
}

static uint32_t AL_Decoder_GetMinStrideHeight_Host(uint32_t uHeight)
{
  return RndHeight(uHeight);
}

uint32_t AL_Decoder_RoundPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  return AL_Decoder_GetMinPitch(uWidth, uBitDepth, eFrameBufferStorageMode);
}

uint32_t AL_Decoder_RoundHeight(uint32_t uHeight)
{
  return AL_Decoder_GetMinStrideHeight(uHeight);
}

/*****************************************************************************/
UNIT_ERROR AL_Decoder_TryDecodeOneUnit(AL_HDecoder hDec, AL_TBuffer* pBufStream)
{
  return AL_Default_Decoder_TryDecodeOneUnit((AL_TDecoder*)hDec, pBufStream);
}

/*****************************************************************************/
int AL_Decoder_GetDecodedStrOffset(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetStrOffset((AL_TDecoder*)hDec);
}

/*****************************************************************************/
int AL_Decoder_SkipParsedUnits(AL_HDecoder hDec)
{
  return AL_Default_Decoder_SkipParsedNals((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_InternalFlush(AL_HDecoder hDec)
{
  AL_Default_Decoder_InternalFlush((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_FlushInput(AL_HDecoder hDec)
{
  AL_Default_Decoder_FlushInput((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Deinit_Host(void)
{
  ;
}

static AL_IDecArchVtable vtable =
{
  .Deinit = AL_Deinit_Host,
  .DecoderCreate = AL_Decoder_Create_Host,
  .DecoderDestroy = AL_Decoder_Destroy_Host,
  .DecoderSetParam = AL_Decoder_SetParam_Host,
  .DecoderPushStreamBuffer = AL_Decoder_PushStreamBuffer_Host,
  .DecoderPushBuffer = AL_Decoder_PushBuffer_Host,
  .DecoderFlush = AL_Decoder_Flush_Host,
  .DecoderPutDisplayPicture = AL_Decoder_PutDisplayPicture_Host,
  .DecoderGetCodec = AL_Decoder_GetCodec_Host,
  .DecoderGetMaxBD = AL_Decoder_GetMaxBD_Host,
  .DecoderGetLastError = AL_Decoder_GetLastError_Host,
  .DecoderGetFrameError = AL_Decoder_GetFrameError_Host,
  .DecoderPreallocateBuffers = AL_Decoder_PreallocateBuffers_Host,
  .DecoderGetMinPitch = AL_Decoder_GetMinPitch_Host,
  .DecoderGetMinStrideHeight = AL_Decoder_GetMinStrideHeight_Host,
};

AL_IDecArch decHost =
{
  .vtable = &vtable
};

/*****************************************************************************/
AL_IDecArch* LibDecodeHostGet(void)
{
  return &decHost;
}
