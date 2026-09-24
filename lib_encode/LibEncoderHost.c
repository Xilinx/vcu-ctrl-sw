// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Encoder.h"
#include "IP_EncoderCtx.h"
#include "lib_encode/I_EncScheduler.h"
#include "lib_encode/Common_Encoder.h"
#include "lib_common/Arithmetics.h"
#include "lib_encode/lib_encode.h"
#include "lib_encode/I_EncArch.h"
#include "lib_encode/lib_encode.h"

#include "lib_rtos/lib_rtos.h"

void AL_CreateHevcEncoder(HighLevelEncoder* pCtx);
void AL_CreateAvcEncoder(HighLevelEncoder* pCtx);

static AL_ERR CreateEncCtx(AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback, AL_TEncCtx** ppCtx)
{
  *ppCtx = AL_Common_Encoder_Create(pAlloc, pSettings);

  if(*ppCtx == NULL)
    return AL_ERR_NO_MEMORY;

  AL_TEncCtx* pCtx = *ppCtx;

  if(AL_IS_HEVC(pSettings->tChParam[0].eProfile))
    AL_CreateHevcEncoder(&pCtx->encoder);

  if(AL_IS_AVC(pSettings->tChParam[0].eProfile))
    AL_CreateAvcEncoder(&pCtx->encoder);

  return AL_Common_Encoder_CreateChannel(pCtx, pScheduler, pAlloc, pSettings, callback);
}

static bool CheckVersion(AL_TIEncSchedulerVersion const* pVersion)
{
  if(pVersion->version.uMajor != AL_VERSION_MAJOR)
    return false;

  if(pVersion->version.uMinor != AL_VERSION_MINOR)
    return false;

  if(pVersion->version.uPatch != AL_VERSION_PATCH)
    return false;
  return true;
}

/****************************************************************************/
static AL_ERR AL_Encoder_Create_Host(AL_HEncoder* hEnc, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback)
{
  if(!pSettings)
    return AL_ERROR;

  AL_TIEncSchedulerVersion tVersion;
  AL_IEncScheduler_Get(pScheduler, AL_IENCSCHEDULER_VERSION, &tVersion);

  if(!CheckVersion(&tVersion))
    return AL_ERROR;

  AL_ERR errorCode = AL_ERR_NO_MEMORY;

  AL_TEncoder* pEncoder = Rtos_Calloc(1, sizeof(*pEncoder));

  if(pEncoder == NULL)
    goto fail;

  *hEnc = (AL_HEncoder)pEncoder;

  errorCode = CreateEncCtx(pScheduler, pAlloc, pSettings, callback, &pEncoder->pCtx);

  if(AL_IS_ERROR_CODE(errorCode))
    goto fail;

  return errorCode;

  fail:
  AL_Encoder_Destroy(*hEnc);
  *hEnc = NULL;
  return errorCode;
}

/****************************************************************************/
static void AL_Encoder_Destroy_Host(AL_HEncoder hEnc)
{
  if(hEnc == NULL)
    return;

  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  AL_Common_Encoder_Destroy(pEnc->pCtx);

  Rtos_Free(pEnc);
}

/****************************************************************************/
static bool AL_Encoder_GetInfo_Host(AL_HEncoder hEnc, AL_TEncoderInfo* pEncInfo)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_GetInfo(pEnc->pCtx, pEncInfo);
}

/****************************************************************************/
static void AL_Encoder_NotifySceneChange_Host(AL_HEncoder hEnc, int32_t iAhead)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifySceneChange(pEnc->pCtx, iAhead);

}

/****************************************************************************/
static void AL_Encoder_NotifyIsLongTerm_Host(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifyIsLongTerm(pEnc->pCtx);
}

/****************************************************************************/
static void AL_Encoder_NotifyUseLongTerm_Host(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifyUseLongTerm(pEnc->pCtx);
}

/****************************************************************************/
static bool AL_Encoder_GetRecPicture_Host(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  return AL_Common_Encoder_GetRecPicture(pEnc->pCtx, pRecPic, 0);
}

/****************************************************************************/
static void AL_Encoder_ReleaseRecPicture_Host(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  AL_TEncCtx* pEncCtx = pEnc->pCtx;

  AL_Common_Encoder_ReleaseRecPicture(pEncCtx, pRecPic, 0);
}

/****************************************************************************/
static bool AL_Encoder_PutStreamBuffer_Host(AL_HEncoder hEnc, AL_TBuffer* pStream)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_PutStreamBuffer(pEnc->pCtx, pStream, 0);
}

/****************************************************************************/
static bool AL_Encoder_Process_Host(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  bool bMainPassOK = AL_Common_Encoder_Process(pEnc->pCtx, pFrame, pQpTable, 0);

  return bMainPassOK;
}

/****************************************************************************/
static int32_t AL_Encoder_AddSei_Host(AL_HEncoder hEnc, AL_TBuffer* pStream, bool isPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, int32_t iTempId)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_AddSei(pEnc->pCtx, pStream, isPrefix, iPayloadType, pPayload, iPayloadSize, iTempId);
}

/****************************************************************************/
static AL_ERR AL_Encoder_GetLastError_Host(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_GetLastError(pEnc->pCtx);
}

/****************************************************************************/
static bool AL_Encoder_SetCostMode_Host(AL_HEncoder hEnc, bool costMode)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetCostMode(pEnc->pCtx, costMode);
}

/****************************************************************************/
static bool AL_Encoder_SetMaxPictureSize_Host(AL_HEncoder hEnc, uint32_t uMaxPictureSize)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetMaxPictureSize(pEnc->pCtx, uMaxPictureSize, AL_SLICE_I) &&
         AL_Common_Encoder_SetMaxPictureSize(pEnc->pCtx, uMaxPictureSize, AL_SLICE_P) &&
         AL_Common_Encoder_SetMaxPictureSize(pEnc->pCtx, uMaxPictureSize, AL_SLICE_B);
}

/****************************************************************************/
static bool AL_Encoder_SetMaxPictureSizePerFrameType_Host(AL_HEncoder hEnc, uint32_t uMaxPictureSize, AL_ESliceType sliceType)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetMaxPictureSize(pEnc->pCtx, uMaxPictureSize, sliceType);
}

/****************************************************************************/

/****************************************************************************/

/****************************************************************************/
static bool AL_Encoder_RestartGop_Host(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  bool bStatus = AL_Common_Encoder_RestartGop(pEnc->pCtx);

  return bStatus;
}

/****************************************************************************/
static bool AL_Encoder_RestartGopRecoveryPoint_Host(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_RestartGopRecoveryPoint(pEnc->pCtx);
}

/****************************************************************************/
static bool AL_Encoder_SetGopLength_Host(AL_HEncoder hEnc, int32_t iGopLength)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  bool bStatus = AL_Common_Encoder_SetGopLength(pEnc->pCtx, iGopLength);

  return bStatus;
}

/****************************************************************************/
static bool AL_Encoder_SetGopNumB_Host(AL_HEncoder hEnc, int32_t iNumB)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetGopNumB(pEnc->pCtx, iNumB);
}

/****************************************************************************/
static bool AL_Encoder_SetFreqIDR_Host(AL_HEncoder hEnc, int32_t iFreqIDR)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetFreqIDR(pEnc->pCtx, iFreqIDR);
}

/****************************************************************************/
static bool AL_Encoder_SetBitRate_Host(AL_HEncoder hEnc, int32_t iBitRate)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetBitRate(pEnc->pCtx, iBitRate, 0);
}

/****************************************************************************/
static bool AL_Encoder_SetMaxBitRate_Host(AL_HEncoder hEnc, int32_t iTargetBitRate, int32_t iMaxBitRate)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetMaxBitRate(pEnc->pCtx, iTargetBitRate, iMaxBitRate, 0);
}

/****************************************************************************/
static bool AL_Encoder_SetFrameRate_Host(AL_HEncoder hEnc, uint16_t uFrameRate, uint16_t uClkRatio)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetFrameRate(pEnc->pCtx, uFrameRate, uClkRatio);
}

/****************************************************************************/
static bool AL_Encoder_SetQP_Host(AL_HEncoder hEnc, int16_t iQP)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQP(pEnc->pCtx, iQP);
}

/****************************************************************************/
static bool AL_Encoder_SetQPOffset_Host(AL_HEncoder hEnc, int16_t iQPOffset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPOffset(pEnc->pCtx, iQPOffset);
}

/****************************************************************************/
static bool AL_Encoder_SetQPBounds_Host(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPBounds(pEnc->pCtx, iMinQP, iMaxQP, AL_SLICE_I) &&
         AL_Common_Encoder_SetQPBounds(pEnc->pCtx, iMinQP, iMaxQP, AL_SLICE_P) &&
         AL_Common_Encoder_SetQPBounds(pEnc->pCtx, iMinQP, iMaxQP, AL_SLICE_B);
}

/****************************************************************************/
static bool AL_Encoder_SetQPBoundsPerFrameType_Host(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPBounds(pEnc->pCtx, iMinQP, iMaxQP, sliceType);
}

/****************************************************************************/
static bool AL_Encoder_SetQPIPDelta_Host(AL_HEncoder hEnc, int16_t iIPDelta)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPIPDelta(pEnc->pCtx, iIPDelta);
}

/****************************************************************************/
static bool AL_Encoder_SetQPPBDelta_Host(AL_HEncoder hEnc, int16_t iPBDelta)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPPBDelta(pEnc->pCtx, iPBDelta);
}

/****************************************************************************/
static bool AL_Encoder_SetInputResolution_Host(AL_HEncoder hEnc, AL_TDimension tDim)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetInputResolution(pEnc->pCtx, tDim);
}

/****************************************************************************/
static bool AL_Encoder_SetLoopFilterMode_Host(AL_HEncoder hEnc, uint8_t uMode)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetLoopFilterMode(pEnc->pCtx, uMode);
}

/****************************************************************************/
static bool AL_Encoder_SetLoopFilterBetaOffset_Host(AL_HEncoder hEnc, int8_t iBetaOffset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetLoopFilterBetaOffset(pEnc->pCtx, iBetaOffset);
}

/****************************************************************************/
static bool AL_Encoder_SetLoopFilterTcOffset_Host(AL_HEncoder hEnc, int8_t iTcOffset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetLoopFilterTcOffset(pEnc->pCtx, iTcOffset);
}

/****************************************************************************/
static bool AL_Encoder_SetQPChromaOffsets_Host(AL_HEncoder hEnc, int8_t iQp1Offset, int8_t iQp2Offset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPChromaOffsets(pEnc->pCtx, iQp1Offset, iQp2Offset);
}

/****************************************************************************/
static bool AL_Encoder_SetAutoQP_Host(AL_HEncoder hEnc, bool useAutoQP)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetAutoQP(pEnc->pCtx, useAutoQP);
}

/****************************************************************************/
static bool AL_Encoder_SetHDRSEIs_Host(AL_HEncoder hEnc, AL_THDRSEIs* pHDRSEIs)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetHDRSEIs(pEnc->pCtx, pHDRSEIs);
}

/*****************************************************************************/
static void AL_Deinit_Host(void)
{
  ;
}

static AL_IEncArchVtable vtable =
{
  .Deinit = AL_Deinit_Host,
  .EncoderCreate = AL_Encoder_Create_Host,
  .EncoderDestroy = AL_Encoder_Destroy_Host,
  .EncoderGetInfo = AL_Encoder_GetInfo_Host,
  .EncoderNotifySceneChange = AL_Encoder_NotifySceneChange_Host,
  .EncoderNotifyIsLongTerm = AL_Encoder_NotifyIsLongTerm_Host,
  .EncoderNotifyUseLongTerm = AL_Encoder_NotifyUseLongTerm_Host,
  .EncoderGetRecPicture = AL_Encoder_GetRecPicture_Host,
  .EncoderReleaseRecPicture = AL_Encoder_ReleaseRecPicture_Host,
  .EncoderPutStreamBuffer = AL_Encoder_PutStreamBuffer_Host,
  .EncoderProcess = AL_Encoder_Process_Host,
  .EncoderAddSei = AL_Encoder_AddSei_Host,
  .EncoderGetLastError = AL_Encoder_GetLastError_Host,
  .EncoderSetCostMode = AL_Encoder_SetCostMode_Host,
  .EncoderSetMaxPictureSize = AL_Encoder_SetMaxPictureSize_Host,
  .EncoderSetMaxPictureSizePerFrameType = AL_Encoder_SetMaxPictureSizePerFrameType_Host,
  .EncoderRestartGop = AL_Encoder_RestartGop_Host,
  .EncoderRestartGopRecoveryPoint = AL_Encoder_RestartGopRecoveryPoint_Host,
  .EncoderSetGopLength = AL_Encoder_SetGopLength_Host,
  .EncoderSetGopNumB = AL_Encoder_SetGopNumB_Host,
  .EncoderSetFreqIDR = AL_Encoder_SetFreqIDR_Host,
  .EncoderSetBitRate = AL_Encoder_SetBitRate_Host,
  .EncoderSetMaxBitRate = AL_Encoder_SetMaxBitRate_Host,
  .EncoderSetFrameRate = AL_Encoder_SetFrameRate_Host,
  .EncoderSetQP = AL_Encoder_SetQP_Host,
  .EncoderSetQPOffset = AL_Encoder_SetQPOffset_Host,
  .EncoderSetQPBounds = AL_Encoder_SetQPBounds_Host,
  .EncoderSetQPBoundsPerFrameType = AL_Encoder_SetQPBoundsPerFrameType_Host,
  .EncoderSetQPIPDelta = AL_Encoder_SetQPIPDelta_Host,
  .EncoderSetQPPBDelta = AL_Encoder_SetQPPBDelta_Host,
  .EncoderSetInputResolution = AL_Encoder_SetInputResolution_Host,
  .EncoderSetLoopFilterMode = AL_Encoder_SetLoopFilterMode_Host,
  .EncoderSetLoopFilterBetaOffset = AL_Encoder_SetLoopFilterBetaOffset_Host,
  .EncoderSetLoopFilterTcOffset = AL_Encoder_SetLoopFilterTcOffset_Host,
  .EncoderSetQPChromaOffsets = AL_Encoder_SetQPChromaOffsets_Host,
  .EncoderSetAutoQP = AL_Encoder_SetAutoQP_Host,
  .EncoderSetHDRSEIs = AL_Encoder_SetHDRSEIs_Host,
};

AL_IEncArch encHost =
{
  .vtable = &vtable
};

/*****************************************************************************/
AL_IEncArch* LibEncodeHostGet(void)
{
  return &encHost;
}
