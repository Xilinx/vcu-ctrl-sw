// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/Scheduler_Defs.h"
#include <limits.h>

#include "DefaultDecoder.h"
#include "NalUnitParser.h"

#include "lib_common/Arithmetics.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/Round.h"
#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common_dec/InternalError.h"
#include "NalUnitParserPrivate.h"
#include "UnsplitBufferFeeder.h"
#include "SplitBufferFeeder.h"
#include "DecSettingsInternal.h"
#include "SearchDecUnit.h"
#include "NullBufferFeeder.h"

#include "lib_common/Error.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/SEI.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common_dec/DecHardwareConfig.h"

#include "lib_common/AvcLevelsLimit.h"
#include "lib_parsing/AvcParser.h"
#include "lib_common/HevcLevelsLimit.h"
#include "lib_parsing/HevcParser.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_decode/I_DecScheduler.h"
#include "lib_common/CodecHook.h"
#include "lib_common_dec/HDRMeta.h"
#include "lib_common_dec/StreamSettingsInternal.h"
#include "lib_common_dec/DecInfoInternal.h"
#include "lib_common_dec/DecOutputSettingsInternal.h"

#include "lib_parsing/DPB.h"

#define AL_BITSTREAM_MAX_SIZE_ALIGNMENT 4096

#define TraceRecordEnd(pTraceHooks, unit, frameId, coreId, modId, chanId) \
  do { \
    if((pTraceHooks) && (pTraceHooks)->RecordEnd) \
      (pTraceHooks)->RecordEnd((pTraceHooks)->pUserParam, unit, frameId, coreId, modId, chanId, false); \
  } while(0) \

#define TraceRecordStart(pTraceHooks, unit, frameId, coreId, modId, chanId) \
  do { \
    if((pTraceHooks) && (pTraceHooks)->RecordStart) \
      (pTraceHooks)->RecordStart((pTraceHooks)->pUserParam, unit, frameId, coreId, modId, chanId, false); \
  } while(0) \

#define SetActiveWorker(pTraceHooks, unit, coreId, modId) \
  do { \
    if((pTraceHooks) && (pTraceHooks)->SetActiveWorker) \
      (pTraceHooks)->SetActiveWorker((pTraceHooks)->pUserParam, unit, coreId, modId); \
  } while(0) \

#define BeginTraces(pTraceHooks) \
  do { \
    if((pTraceHooks) && (pTraceHooks)->BeginTraces) \
      (pTraceHooks)->BeginTraces((pTraceHooks)->pUserParam); \
  } while(0) \

#define EndTraces(pTraceHooks) \
  do { \
    if((pTraceHooks) && (pTraceHooks)->EndTraces) \
      (pTraceHooks)->EndTraces((pTraceHooks)->pUserParam); \
  } while(0) \

static int32_t const LAST_VCL_NAL_IN_AU_NOT_PRESENT = -1;

/*****************************************************************************/
static bool isAVC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_AVC;
}

/*****************************************************************************/
static bool isHEVC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_HEVC;
}

static bool IsEarlyCallback(AL_TDecChanParam* pChanParam)
{
  (void)pChanParam;
  bool bIsEarlyCallback = false;
  bIsEarlyCallback = pChanParam->bCallDecodeCallbackAtStart;

  return bIsEarlyCallback;
}

/*****************************************************************************/
static int32_t GetCircularBufferSize(AL_ECodec eCodec, int32_t iStack, AL_TStreamSettings const* pStreamSettings)
{
  (void)eCodec;

  int32_t zMaxCPBSize = 50 * 1024 * 1024; /* CPB default worst case */
  int32_t zWorstCaseNalSize = 2 << 23; /* Single frame worst case */

  if(isAVC(eCodec))
  {
    zMaxCPBSize = 120 * 1024 * 1024;
    zWorstCaseNalSize = 2 << 24;
  }

  int32_t circularBufferSize = 0;

  if(IsAllStreamSettingsSet(pStreamSettings))
  {
    /* Circular buffer always should be able to hold one frame, therefore compute the worst case and use it as a lower bound.  */
    int32_t const zMaxNalSize = AL_GetMaxNalSize(pStreamSettings->tDim, pStreamSettings->eChroma, pStreamSettings->iBitDepth, pStreamSettings->eProfile, pStreamSettings->iLevel); /* Worst case: (5/3)*PCM + Worst case slice Headers */
    int32_t const zRealworstcaseNalSize = AL_GetMitigatedMaxNalSize(pStreamSettings->tDim, pStreamSettings->eChroma, pStreamSettings->iBitDepth); /* Reasonable: PCM + Slice Headers */
    circularBufferSize = UnsignedMax(zMaxNalSize, iStack * zRealworstcaseNalSize);
  }
  else
    circularBufferSize = zWorstCaseNalSize * iStack;

  /* Get minimum between absolute CPB worst case and computed value. */
  int32_t const bufferSize = UnsignedMin(circularBufferSize, zMaxCPBSize);

  /* Round up for hardware. */
  return GetAlignedStreamBufferSize(bufferSize);
}

/*****************************************************************************/
static AL_TDecCtx* AL_sGetContext(AL_TDecoder* pDec)
{
  return &(pDec->ctx);
}

/*****************************************************************************/
static bool isSubframeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit == AL_VCL_NAL_UNIT;
}

/*****************************************************************************/
static void AL_sDecoder_CallEndParsing(AL_TDecCtx* pCtx, AL_TBuffer* pParsedFrame, int32_t iParsingID)
{
  AL_THandleMetaData const* pHandlesMeta = (AL_THandleMetaData const*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
  {
    if(pCtx->tDecCB.endParsingCB.func)
      pCtx->tDecCB.endParsingCB.func(pParsedFrame, pCtx->tDecCB.endParsingCB.userParam, iParsingID);
    return;
  }
  int32_t const numHandles = AL_HandleMetaData_GetNumHandles(pHandlesMeta);

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
  {
    (void)numHandles;
    Rtos_Assert(numHandles == 1);
  }
  Rtos_Assert(iParsingID < numHandles);

  // The handles should be stored in the slice order.
  AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, iParsingID);

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSING)
  {
    AL_TBuffer* pStream = pDecMetaHandle->pHandle;
    AL_Buffer_Ref(pStream);
    pDecMetaHandle->eState = AL_DEC_HANDLE_STATE_PROCESSED;

    if(pCtx->tDecCB.endParsingCB.func)
      pCtx->tDecCB.endParsingCB.func(pParsedFrame, pCtx->tDecCB.endParsingCB.userParam, iParsingID);
    AL_Feeder_FreeBuf(pCtx->Feeder, pStream);
    AL_Buffer_Unref(pStream);
  }
  else if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_CONCEALING)
  {
    pDecMetaHandle->eState = AL_DEC_HANDLE_STATE_CONCEALED;

    if(pCtx->tDecCB.endParsingCB.func)
      pCtx->tDecCB.endParsingCB.func(pParsedFrame, pCtx->tDecCB.endParsingCB.userParam, iParsingID);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallDecode(AL_TDecCtx* pCtx, AL_TIndex tFrameID)
{
  AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, tFrameID);
  Rtos_Assert(pDecodedFrame);

  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pDecodedFrame, AL_META_TYPE_HANDLE);

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    AL_sDecoder_CallEndParsing(pCtx, pDecodedFrame, 0);
  else if(pHandlesMeta)
  {
    int32_t numHandles = AL_HandleMetaData_GetNumHandles(pHandlesMeta);

    for(int32_t handle = 0; handle < numHandles; ++handle)
    {
      AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, handle);
      AL_TBuffer* pStream = pDecMetaHandle->pHandle;

      if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSING)
      {
        AL_Buffer_Ref(pStream);
        AL_Feeder_FreeBuf(pCtx->Feeder, pStream);
        AL_Buffer_Unref(pStream);
      }
    }
  }

  if(!IsEarlyCallback(pCtx->pChanParam))
    pCtx->tDecCB.endDecodingCB.func(pDecodedFrame, pCtx->tDecCB.endDecodingCB.userParam);

  if(pHandlesMeta)
    AL_HandleMetaData_ResetHandles(pHandlesMeta);
}

/*****************************************************************************/
static void BuildCurrentHRD(AL_TDecCtx* pCtx, AL_TBuffer* pFrameToDisplay, bool bStartsNewCVS)
{
  AL_THDRMetaData* pMeta = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pFrameToDisplay, AL_META_TYPE_HDR);

  if(pMeta != NULL)
  {
    if(bStartsNewCVS)
    {
      pCtx->aup.tActiveHDRSEIs.bHasMDCV = false;
      pCtx->aup.tActiveHDRSEIs.bHasCLL = false;
      pCtx->aup.tActiveHDRSEIs.bHasATC = false;
      pCtx->aup.tActiveHDRSEIs.bHasST2094_10 = false;
    }

    if(pMeta->tHDRSEIs.bHasMDCV)
    {
      pCtx->aup.tActiveHDRSEIs.bHasMDCV = true;
      pCtx->aup.tActiveHDRSEIs.tMDCV = pMeta->tHDRSEIs.tMDCV;
    }
    else if(pCtx->aup.tActiveHDRSEIs.bHasMDCV)
    {
      pMeta->tHDRSEIs.bHasMDCV = true;
      pMeta->tHDRSEIs.tMDCV = pCtx->aup.tActiveHDRSEIs.tMDCV;
    }

    if(pMeta->tHDRSEIs.bHasCLL)
    {
      pCtx->aup.tActiveHDRSEIs.bHasCLL = true;
      pCtx->aup.tActiveHDRSEIs.tCLL = pMeta->tHDRSEIs.tCLL;
    }
    else if(pCtx->aup.tActiveHDRSEIs.bHasCLL)
    {
      pMeta->tHDRSEIs.bHasCLL = true;
      pMeta->tHDRSEIs.tCLL = pCtx->aup.tActiveHDRSEIs.tCLL;
    }

    if(pMeta->tHDRSEIs.bHasATC)
    {
      pCtx->aup.tActiveHDRSEIs.bHasATC = true;
      pCtx->aup.tActiveHDRSEIs.tATC = pMeta->tHDRSEIs.tATC;
    }
    else if(pCtx->aup.tActiveHDRSEIs.bHasATC)
    {
      pMeta->tHDRSEIs.bHasATC = true;
      pMeta->tHDRSEIs.tATC = pCtx->aup.tActiveHDRSEIs.tATC;
    }

    if(pMeta->tHDRSEIs.bHasST2094_10)
    {
      pCtx->aup.tActiveHDRSEIs.bHasST2094_10 = true;
      pCtx->aup.tActiveHDRSEIs.tST2094_10 = pMeta->tHDRSEIs.tST2094_10;
    }
    else if(pCtx->aup.tActiveHDRSEIs.bHasST2094_10)
    {
      pMeta->tHDRSEIs.bHasST2094_10 = true;
      pMeta->tHDRSEIs.tST2094_10 = pCtx->aup.tActiveHDRSEIs.tST2094_10;
    }

    if(pMeta->tHDRSEIs.bHasST2094_40)
    {
      pCtx->aup.tActiveHDRSEIs.bHasST2094_40 = true;
      pCtx->aup.tActiveHDRSEIs.tST2094_40 = pMeta->tHDRSEIs.tST2094_40;
    }
    else if(pCtx->aup.tActiveHDRSEIs.bHasST2094_40)
    {
      pMeta->tHDRSEIs.bHasST2094_40 = true;
      pMeta->tHDRSEIs.tST2094_40 = pCtx->aup.tActiveHDRSEIs.tST2094_40;
    }
  }
}

/*****************************************************************************/
static bool AL_sDecoder_TryDisplayOneFrame(AL_TDecCtx* pCtx, int32_t iFrameID)
{
  bool bEarlyDisplay = iFrameID != -1;
  AL_TInfoDecode tInfo = { 0 };
  bool bStartsNewCVS = false;
  AL_TBuffer* pFrameToDisplay = NULL;

  if(bEarlyDisplay)
    pFrameToDisplay = AL_PictMngr_ForceDisplayBuffer(&pCtx->PictMngr, &tInfo, &bStartsNewCVS, (uint8_t)iFrameID);
  else
    pFrameToDisplay = AL_PictMngr_GetDisplayBuffer(&pCtx->PictMngr, &tInfo, &bStartsNewCVS);

  if(pFrameToDisplay == NULL)
    return false;

  uint8_t* pPtrIsNotNull = AL_Buffer_GetData(pFrameToDisplay);
  (void)pPtrIsNotNull;
  Rtos_Assert(pPtrIsNotNull != NULL);

  BuildCurrentHRD(pCtx, pFrameToDisplay, bStartsNewCVS);

  if(bEarlyDisplay || !IsEarlyCallback(pCtx->pChanParam))
    pCtx->tDecCB.displayCB.func(pFrameToDisplay, &tInfo, pCtx->tDecCB.displayCB.userParam);
  AL_PictMngr_SignalCallbackDisplayIsDone(&pCtx->PictMngr);

  return true;
}

/*****************************************************************************/
static void AL_sDecoder_CallDisplay(AL_TDecCtx* pCtx)
{
  while(AL_sDecoder_TryDisplayOneFrame(pCtx, -1))
  {
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, AL_TIndex tFrameID)
{
  AL_sDecoder_CallDecode(pCtx, tFrameID);
  AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Default_Decoder_EndParsing(void* pUserParam, AL_TIndex tFrameID, int32_t iParsingID)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    return;

  AL_TBuffer* pParsedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, tFrameID);
  Rtos_Assert(pParsedFrame);

  AL_sDecoder_CallEndParsing(pCtx, pParsedFrame, iParsingID);
}

/*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus const* pStatus)
{
  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_CMD_INVALID))
  {
    Rtos_Log(AL_LOG_CRITICAL, "\n***** /!\\ Error trying to conceal bitstream - ending decoding /!\\ *****\n");
    Rtos_Assert(false);
  }

  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  AL_TIndex tFrameID = pStatus->tBufIDs.tFrmID;

  AL_PictMngr_UpdateDisplayBufferCRC(&pCtx->PictMngr, tFrameID, pStatus->uCRC);
  AL_PictMngr_UpdateDisplayBufferPixCount(&pCtx->PictMngr, tFrameID, pStatus->uPixCount);

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_NOT_FINISHED))
  {
    /* we want to notify the user, but we don't want to update the decoder state */
    AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, tFrameID);
    Rtos_Assert(pDecodedFrame);
    pCtx->tDecCB.endDecodingCB.func(pDecodedFrame, pCtx->tDecCB.endDecodingCB.userParam);
    bool const bSuccess = AL_sDecoder_TryDisplayOneFrame(pCtx, tFrameID);
    (void)bSuccess;
    Rtos_Assert(bSuccess);
    return;
  }

  AL_PictMngr_EndDecoding(&pCtx->PictMngr, tFrameID);

  int32_t iOffset = pCtx->iNumFrmBlk2 % AL_DEC_SW_MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->tFrameIDRefList[iOffset], pCtx->tMvIDRefList[iOffset]);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = pCtx->iStreamOffset[pCtx->iNumFrmBlk2 % pCtx->iStackSize];
  ++pCtx->iNumFrmBlk2;

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_HANGED))
    Rtos_Log(AL_LOG_CRITICAL, "\n***** /!\\ Timeout - resetting the decoder /!\\ *****\n");

  Rtos_ReleaseMutex(pCtx->DecMutex);

  if(pStatus->bConcealed || pStatus->uNumConcealedLCU > 0)
    AL_Default_Decoder_SetError(pCtx, AL_WARN_HW_CONCEAL_DETECT, tFrameID, true);

  AL_Feeder_Signal(pCtx->Feeder);
  AL_sDecoder_CallBacks(pCtx, tFrameID);

  Rtos_GetMutex(pCtx->DecMutex);
  AL_TIndex tMvID = pStatus->tBufIDs.tMvID;
  AL_PictMngr_UnlockID(&pCtx->PictMngr, tFrameID, tMvID);
  Rtos_ReleaseMutex(pCtx->DecMutex);

  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
bool AL_Default_Decoder_CreateChannel(AL_TDecCtx* pCtx, void (* pfnEndParsing)(void* pUserParam, AL_TIndex tFrameID, AL_TIndex tSliceID), void (* pfnEndDecoding)(void* pUserParam, AL_TDecPicStatus const* pPicStatus))
{

  // ToDo : #if AL_ENABLE_ARCH_WITHOUT_SCHEDULER
  if(pCtx->pScheduler)
  {
    AL_TDecScheduler_CB_EndParsing endParsingCallback = { pfnEndParsing, pCtx };
    AL_TDecScheduler_CB_EndDecoding endDecodingCallback = { pfnEndDecoding, pCtx };
    AL_TDecScheduler_ChannelParams tSchedulerChanParams =
    {
      &pCtx->tMDChanParam,
      endParsingCallback,
      endDecodingCallback,
    };
    AL_ERR eError = AL_IDecScheduler_CreateChannel(&pCtx->hChannel, pCtx->pScheduler, &tSchedulerChanParams);

    if(AL_IS_ERROR_CODE(eError))
    {
      AL_Default_Decoder_SetError(pCtx, eError, -1, true);
      pCtx->eChanState = CHAN_INVALID;
      return false;
    }

  }

  pCtx->eChanState = CHAN_CONFIGURED;

  return true;
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseStreamBuffer(void* pUserParam, AL_TBuffer* pBufStream)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  AL_Feeder_FreeBuf(pCtx->Feeder, pBufStream);
}

/*****************************************************************************/
static void ReleaseFramePictureUnused(AL_TDecCtx* pCtx)
{
  while(true)
  {
    AL_TBuffer* pFrameToRelease = AL_PictMngr_GetUnusedDisplayBuffer(&pCtx->PictMngr);

    if(!pFrameToRelease)
      break;

    uint8_t* pPtrIsNotNull = AL_Buffer_GetData(pFrameToRelease);
    (void)pPtrIsNotNull;
    Rtos_Assert(pPtrIsNotNull != NULL);

    pCtx->tDecCB.displayCB.func(pFrameToRelease, NULL, pCtx->tDecCB.displayCB.userParam);
    AL_PictMngr_SignalCallbackReleaseIsDone(&pCtx->PictMngr, pFrameToRelease);
  }
}

/*****************************************************************************/
static void DeinitPictureManager(AL_TDecCtx* pCtx)
{
  ReleaseFramePictureUnused(pCtx);
  AL_PictMngr_Deinit(&pCtx->PictMngr);
}

/*****************************************************************************/
void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_Assert(pCtx);

  AL_PictMngr_DecommitPool(&pCtx->PictMngr);

  {
    if(pCtx->Feeder)
      AL_Feeder_Destroy(pCtx->Feeder);
  }

  if(pCtx->eosBuffer)
    AL_Buffer_Unref(pCtx->eosBuffer);

  if(pCtx->pScheduler != NULL)
  {
    if(pCtx->hStartCodeChannel)
      AL_IDecScheduler_DestroyStartCodeChannel(pCtx->pScheduler, pCtx->hStartCodeChannel);

    if(pCtx->hChannel)
      AL_IDecScheduler_DestroyChannel(pCtx->pScheduler, pCtx->hChannel);
  }
  DeinitPictureManager(pCtx);
  Rtos_Free(pCtx->BufNoAE.tMD.pVirtualAddr);
  AL_Decoder_DeinitBuffers(pCtx);

  Rtos_DeleteSemaphore(pCtx->Sem);
  Rtos_DeleteEvent(pCtx->ScDetectionComplete);
  Rtos_DeleteEvent(pCtx->hDecOutSettingsConfiguredEvt);

  {
    Rtos_DeleteMutex(pCtx->DecMutex);
  }

  Rtos_Free(pDec);
}

/*****************************************************************************/
void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, const char* sPrefix, int32_t iFrmID, int32_t iNumFrm, bool bShouldPrintFrameDelimiter, bool bOutputTraceOnly)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  for(size_t i = 0; i < sizeof(pCtx->sTracePrefix) && sPrefix[i]; ++i)
    pCtx->sTracePrefix[i] = sPrefix[i];

  pCtx->sTracePrefix[sizeof(pCtx->sTracePrefix) - 1] = '\000';

  pCtx->iTraceFirstFrame = iFrmID;
  pCtx->iTraceLastFrame = iFrmID + iNumFrm;
  pCtx->bShouldPrintFrameDelimiter = bShouldPrintFrameDelimiter;
  pCtx->bOutputTraceOnly = bOutputTraceOnly;
}

/*****************************************************************************/
static bool addEOSMeta(AL_TBuffer* pEOSBuffer, int32_t iAvailSize)
{
  AL_TStreamMetaData* pStreamMeta = NULL;

  if(NULL == AL_Buffer_GetMetaData(pEOSBuffer, AL_META_TYPE_STREAM))
  {
    pStreamMeta = AL_StreamMetaData_Create(1);

    if(!AL_Buffer_AddMetaData(pEOSBuffer, (AL_TMetaData*)pStreamMeta))
    {
      AL_MetaData_Destroy((AL_TMetaData*)pStreamMeta);
      return false;
    }
    AL_StreamMetaData_ClearAllSections(pStreamMeta);
    AL_StreamMetaData_AddSection(pStreamMeta, 0, 0, AL_SECTION_END_FRAME_FLAG);
  }

  if(NULL == AL_Buffer_GetMetaData(pEOSBuffer, AL_META_TYPE_CIRCULAR))
  {
    AL_TCircMetaData* pCircMeta = AL_CircMetaData_Create(0, iAvailSize, true);

    if(!pCircMeta)
    {
      AL_Buffer_RemoveMetaData(pEOSBuffer, (AL_TMetaData*)pStreamMeta);
      AL_MetaData_Destroy((AL_TMetaData*)pStreamMeta);
      return false;
    }

    if(!AL_Buffer_AddMetaData(pEOSBuffer, (AL_TMetaData*)pCircMeta))
    {
      AL_Buffer_RemoveMetaData(pEOSBuffer, (AL_TMetaData*)pStreamMeta);
      AL_MetaData_Destroy((AL_TMetaData*)pStreamMeta);
      AL_MetaData_Destroy((AL_TMetaData*)pCircMeta);
      return false;
    }
  }
  return true;
}

/*****************************************************************************/
static bool SearchNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int32_t* pLastStartCodeInDecodingUnit, int32_t* iLastVclNalInDecodingUnit)
{
  (void)pStream;
  return AL_SearchDecUnit_GetNextUnit(&pCtx->SearchCtx, pLastStartCodeInDecodingUnit, iLastVclNalInDecodingUnit);
}

/*****************************************************************************/
static void CheckNALParserResult(AL_TDecCtx* pCtx, AL_EParseResult eParseResult)
{
  if((eParseResult == AL_PARSE_OK) || (eParseResult == AL_PARSER_OBU_OP_POINT_DISCARD))
    return;

  AL_Default_Decoder_SetError(pCtx, eParseResult == AL_PARSE_UNSUPPORTED ? AL_WARN_UNSUPPORTED_NAL : AL_WARN_CONCEAL_DETECT, -1, true);
}

/*****************************************************************************/
static bool CheckAvailSpace(AL_TDecCtx* pCtx, AL_TSeiMetaData* pMeta)
{
  uint32_t uLengthNAL = GetNonVclSize(pCtx->pStream);
  int32_t Offset = (uintptr_t)AL_SeiMetaData_GetBuffer(pMeta) - (uintptr_t)pMeta->pBuf;
  return Offset + uLengthNAL <= pMeta->maxBufSize;
}

/*****************************************************************************/
static AL_TSeiMetaData* GetSeiMetaData(AL_TDecCtx* pCtx)
{
  if(!pCtx->pInputBuffer)
    return NULL;

  return (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pCtx->pInputBuffer, AL_META_TYPE_SEI);
}

/*****************************************************************************/
static bool AL_DecodeOneNal(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut nut, uint32_t uNutSize, bool bIsLastAUNal, int32_t* iNumSlice)
{
  (void)uNutSize;

  if(pCtx->nalParser.isNutError(nut))
    return false;

  AL_NalParser parser = pCtx->nalParser;
  AL_NonVclNuts nuts = parser.getNonVclNuts();
  bool bConcealRequestLaunched = false;

  if(parser.isSliceData(nut))
  {
    return parser.decodeSliceData(pAUP, pCtx, nut, bIsLastAUNal, iNumSlice);
  }
  else
  {
    // There is no NAL reordering in split-input + subframeUnit
    // This is to ensure previous requests are finished for any non-slice nals
    if((pCtx->eInputMode == AL_DEC_SPLIT_INPUT) && (isSubframeUnit(pCtx->pChanParam->eDecUnit)))
    {
      if(AL_Default_Decoder_HasOngoingFrame(pCtx))
      {
        /* potentially we have already issue a decode slice request so will need to keep track of work request in this mode*/
        parser.finishPendingRequest(pCtx);
        pCtx->tConceal.bSkipRemainingNals = true;
        bConcealRequestLaunched = true;
      }
    }
  }

  if((nut == nuts.seiPrefix || (nut == nuts.seiSuffix && pCtx->bAreBuffersAllocated)) && parser.parseSei)
  {
    bool bIsPrefix = (nut == nuts.seiPrefix);
    AL_TSeiMetaData* pMeta = GetSeiMetaData(pCtx);

    if(pMeta && !CheckAvailSpace(pCtx, pMeta))
    {
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, -1, true);
      return false;
    }

    AL_TRbspParser rp = pMeta ? getParserOnNonVclNal(pCtx, AL_SeiMetaData_GetBuffer(pMeta), pCtx->BufNoAE.tMD.uSize) : getParserOnNonVclNalInternalBuf(pCtx);

    if(!parser.parseSei(pAUP, &rp, bIsPrefix, &pCtx->tDecCB.parsedSeiCB, pMeta))
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, -1, true);
    else
    {

    }
  }
  else if(nut == nuts.sps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_EParseResult eParserResult = parser.parseSps(pAUP, &rp, pCtx);

    if(eParserResult == AL_PARSER_LAUNCHED_OK)
    {
      if(pCtx->eInputMode != AL_DEC_UNSPLIT_INPUT)
        pCtx->tConceal.bSkipRemainingNals = true;

      return true;
    }
    CheckNALParserResult(pCtx, eParserResult);
  }
  else if(nut == nuts.pps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_EParseResult eParserResult = parser.parsePps(pAUP, &rp, pCtx);

    if(eParserResult == AL_PARSER_LAUNCHED_OK)
    {
      if(pCtx->eInputMode != AL_DEC_UNSPLIT_INPUT)
        pCtx->tConceal.bSkipRemainingNals = true;

      return true;
    }

    CheckNALParserResult(pCtx, eParserResult);
  }
  else if(nut == nuts.vps && parser.parseVps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_EParseResult eParserResult = parser.parseVps(pAUP, &rp);
    CheckNALParserResult(pCtx, eParserResult);
  }
  else if((nut == nuts.apsPrefix) || (nut == nuts.apsSuffix))
  {
    if(parser.parseAps)
    {
      AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
      AL_EParseResult eParserResult = parser.parseAps(pAUP, &rp, pCtx);
      CheckNALParserResult(pCtx, eParserResult);
    }

    // In split-input, apsSuffix is a non-reorderable delimiter,
    // we need to ensure that previous NAL is launched
    if((pCtx->eInputMode == AL_DEC_SPLIT_INPUT) && (nut == nuts.apsSuffix))
    {
      if(bConcealRequestLaunched == false)
      {
        if(AL_Default_Decoder_HasOngoingFrame(pCtx))
        {
          parser.finishPendingRequest(pCtx);
          pCtx->bIsFirstPicture = true;
          pCtx->tConceal.bSkipRemainingNals = true;
          return true;
        }
      }
      pCtx->bIsFirstPicture = true;
    }
  }
  else if(nut == nuts.ph && parser.parsePh)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_EParseResult eParserResult = parser.parsePh(pAUP, &rp, pCtx);
    CheckNALParserResult(pCtx, eParserResult);
  }
  else if((nut == nuts.eos) || (nut == nuts.eob))
  {
    pCtx->bIsFirstPicture = true;
  }
  else if(nut == nuts.fd)
  {
    (void)nut;
  }
  else if(parser.parseOtherNal)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_EParseResult eParserResult = parser.parseOtherNal(pAUP, &rp, nut);
    CheckNALParserResult(pCtx, eParserResult);
  }

  return bConcealRequestLaunched;
}

/*****************************************************************************/
static bool DecodeOneNAL(AL_TDecCtx* pCtx, AL_TNal const* pNal, int32_t* pNumSlice, bool bIsLastVclNal)
{
  if(pCtx->tCurrentFrameCtx.uNumSlice > 0 && *pNumSlice > pCtx->pChanParam->iMaxSlices)
    return false;

  return AL_DecodeOneNal(&pCtx->aup, pCtx, pNal->tStartCode.uNUT, pNal->uSize, bIsLastVclNal, pNumSlice);

  (void)pNal;
  (void)bIsLastVclNal;
  return false;
}

/*****************************************************************************/
static void UpdateStartCodeNumber(AL_TDecCtx* pCtx, AL_TDecScdBuffers* pScdBuffers, AL_TCircMetaData* pMeta, uint16_t uNumSC)
{
  uint32_t uMaxSize = pScdBuffers->StreamBuf.tMD.uSize;

  pMeta->iOffset = (pMeta->iOffset + pCtx->ScdStatus.uNumBytes) % uMaxSize;
  pMeta->iAvailSize -= pCtx->ScdStatus.uNumBytes;

  AL_TStartCode* pSC = (AL_TStartCode*)pScdBuffers->ScdBufOut.tMD.pVirtualAddr;

  AL_SearchDecUnit_AddNals(&pCtx->SearchCtx, pSC, uNumSC, pMeta->iOffset, false);
}

/*****************************************************************************/
static void initScdBuffer(AL_TDecScdBuffers* pScdBufs, AL_TBuffer* pStream, AL_TCircMetaData* pMeta, TBuffer* pScdOut)
{
  Rtos_Memset(&pScdBufs->StreamBuf.tMD, 0, sizeof(pScdBufs->StreamBuf.tMD));
  pScdBufs->StreamBuf.tMD.pVirtualAddr = AL_Buffer_GetVirtualAddress(pStream);
  pScdBufs->StreamBuf.tMD.uPhysicalAddr = AL_Buffer_GetPhysicalAddress(pStream);
  pScdBufs->StreamBuf.tMD.uSize = AL_Buffer_GetSize(pStream);

  pScdBufs->StreamMeta = *pMeta;

  Rtos_Memset(&pScdBufs->ScdBufOut.tMD, 0, sizeof(pScdBufs->ScdBufOut.tMD));
  pScdBufs->ScdBufOut.tMD.pVirtualAddr = pScdOut->tMD.pVirtualAddr;
  pScdBufs->ScdBufOut.tMD.uPhysicalAddr = pScdOut->tMD.uPhysicalAddr;
  pScdBufs->ScdBufOut.tMD.uSize = pScdOut->tMD.uSize;

  AL_CleanupMemory(pScdBufs->ScdBufOut.tMD.pVirtualAddr, pScdBufs->ScdBufOut.tMD.uSize);
}

/*****************************************************************************/
static void GetScdAddrs(AL_TStartCodeBufferAddrs* pScdBufAddrs, AL_TDecScdBuffers const* pScdBuffers)
{
  pScdBufAddrs->pStream = pScdBuffers->StreamBuf.tMD.uPhysicalAddr;
  pScdBufAddrs->uMaxSize = pScdBuffers->StreamBuf.tMD.uSize;
  pScdBufAddrs->uOffset = pScdBuffers->StreamMeta.iOffset;
  pScdBufAddrs->uAvailSize = pScdBuffers->StreamMeta.iAvailSize;
  pScdBufAddrs->pBufOut = pScdBuffers->ScdBufOut.tMD.uPhysicalAddr;
}

/*****************************************************************************
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current start code searching status
*****************************************************************************/
static void EndScd(void* pUserParam, AL_TStartCodeStatus const* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  pCtx->ScdStatus.uNumBytes = pStatus->uNumBytes;
  pCtx->ScdStatus.uNumSC = pStatus->uNumSC;

  Rtos_SetEvent(pCtx->ScDetectionComplete);
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferHEVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x4A, 0
  }; // simulate a end_of_bitstream
  int32_t iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);

  int32_t iMaxSize = AL_RoundUp(iSize, AL_BITSTREAM_MAX_SIZE_ALIGNMENT);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iMaxSize, &AL_Buffer_Destroy, "eos-buffer");

  if(pEOS == NULL)
    return NULL;

  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);

  if(!addEOSMeta(pEOS, iSize))
    AL_Buffer_Destroy(pEOS);

  return pEOS;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferAVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x0B, 0
  }; // simulate end_of_stream
  int32_t iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);

  int32_t iMaxSize = AL_RoundUp(iSize, AL_BITSTREAM_MAX_SIZE_ALIGNMENT);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iMaxSize, &AL_Buffer_Destroy, "eos-buffer");

  if(pEOS == NULL)
    return NULL;

  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);

  if(!addEOSMeta(pEOS, iSize))
    AL_Buffer_Destroy(pEOS);

  return pEOS;
}

/*****************************************************************************/
static bool RefillStartCodes(AL_TDecCtx* pCtx, AL_TBuffer* pStream)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);

  int32_t const SCDHardwareConstraintMinSize = 5;

  if(pMeta->iAvailSize < SCDHardwareConstraintMinSize)
    return false;

  AL_TDecScdBuffers ScdBuffers;
  initScdBuffer(&ScdBuffers, pStream, pMeta, &pCtx->BufSCD);

  AL_TDecScheduler_CB_EndStartCode callback = { EndScd, pCtx };

  AL_TStartCodeParam tStartCodeParam = { 0 };
  tStartCodeParam.MaxSize = ScdBuffers.ScdBufOut.tMD.uSize / sizeof(AL_TStartCode);
  tStartCodeParam.eCodec = pCtx->pChanParam->eCodec;

  /* if the start code couldn't be launched because the start code queue is full,
   * we have to retry as failing to do so will make us return ERR_UNIT_NOT_FOUND which
   * means that the whole chunk of data doesn't have a startcode in it and
   * for example that we can stop sending new decoding request if there isn't
   * more input data. */

  do
  {

    AL_TStartCodeBufferAddrs ScdBufAddrs;
    GetScdAddrs(&ScdBufAddrs, &ScdBuffers);

    AL_IDecScheduler_SearchSC(pCtx->pScheduler, pCtx->hStartCodeChannel, &tStartCodeParam, &ScdBufAddrs, callback);
    Rtos_WaitEvent(pCtx->ScDetectionComplete, AL_WAIT_FOREVER);

    if(pCtx->ScdStatus.uNumBytes == 0)
    {
      Rtos_Log(AL_LOG_CRITICAL, "***** /!\\ Warning: Start code queue was full, degraded mode, retrying. /!\\ *****\n");
      Rtos_Sleep(1);
    }
  }
  while(pCtx->ScdStatus.uNumBytes == 0);

  Rtos_InvalidateCacheMemory(ScdBuffers.ScdBufOut.tMD.pVirtualAddr, ScdBuffers.ScdBufOut.tMD.uSize);
  UpdateStartCodeNumber(pCtx, &ScdBuffers, pMeta, pCtx->ScdStatus.uNumSC);

  return pCtx->ScdStatus.uNumSC > 0;
  return false;
}

/*****************************************************************************/
static bool canStoreMoreStartCodes(AL_TDecCtx* pCtx)
{
  return AL_SearchDecUnit_GetFreeNalCount(&pCtx->SearchCtx) >= (pCtx->BufSCD.tMD.uSize / sizeof(AL_TStartCode));
}

/*****************************************************************************/
static int32_t FindNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int32_t* iLastVclNalInAU)
{
  int32_t iNalCount = 0;

  while(!SearchNextDecodingUnit(pCtx, pStream, &iNalCount, iLastVclNalInAU))
  {

    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      AL_SearchDecUnit_ResetNals(&pCtx->SearchCtx);
    }

    if(!RefillStartCodes(pCtx, pStream))
      return 0;
  }

  return iNalCount;
}

/*****************************************************************************/
static int32_t FillNalInfo(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int32_t* pLastVclNalInAU)
{
  (void)pLastVclNalInAU;
  pCtx->pInputBuffer = pStream;

  while(RefillStartCodes(pCtx, pStream) != false)
    ;

  int32_t iNalCount = AL_SearchDecUnit_GetCurrentNalCount(&pCtx->SearchCtx);

  if(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec))
  {
    AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
    bool bSearchLastVCLNal = false;

    for(int32_t curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection)
    {
      if(pStreamMeta->pSections[curSection].eFlags & AL_SECTION_END_FRAME_FLAG)
      {
        bSearchLastVCLNal = true;
        break;
      }
    }

    if(bSearchLastVCLNal)
      *pLastVclNalInAU = AL_SearchDecUnit_GetLastVCL(&pCtx->SearchCtx);
  }

  return iNalCount;
}

/*****************************************************************************/
static void ResetCurrentFrameFlags(AL_TDecCtx* pCtx)
{
  AL_DecFrameCtx_ResetFlags(&pCtx->tCurrentFrameCtx);
}

/*****************************************************************************/
static bool GetNextNal(AL_TDecCtx* pCtx, AL_TNal* nals, int32_t iNalCount, int32_t iLastVclNalInAU, int32_t* iNal, AL_DecodeNalStep* step)
{
  (void)nals;
  (void)iNalCount;
  (void)step;

  if(pCtx->tConceal.bSkipRemainingNals)
  {
    *iNal = iNalCount;
    return false;
  }

  if((pCtx->eInputMode != AL_DEC_SPLIT_INPUT) || (iLastVclNalInAU == LAST_VCL_NAL_IN_AU_NOT_PRESENT))
  {
    (*iNal)++;
    return true;
  }

  Rtos_Assert(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec) && "Unsupported codec");

  while(true)
  {
    switch(*step)
    {
    case SEND_NAL_UNTIL_LAST_VCL:
    {
      (*iNal)++;

      if(*iNal < iLastVclNalInAU)
        return true;

      (*step) = SEND_REORDERED_SUFFIX;
      continue;
    }
    case SEND_REORDERED_SUFFIX:
    {
      do
        (*iNal)++;
      while(*iNal < iNalCount && !pCtx->nalParser.canNalBeReordered(nals[*iNal].tStartCode.uNUT));

      if(*iNal >= iNalCount)
      {
        (*iNal) = iLastVclNalInAU;
        (*step) = SEND_LAST_VCL;
        continue;
      }
      return true;
    }
    case SEND_LAST_VCL:
    {
      Rtos_Assert(*iNal == iLastVclNalInAU);
      (*step) = SEND_REMAINING_NAL;
      return true;
    }
    case SEND_REMAINING_NAL:
    {
      do
        (*iNal)++;
      while(*iNal < iNalCount && pCtx->nalParser.canNalBeReordered(nals[*iNal].tStartCode.uNUT));

      if(*iNal < iNalCount)
      {
        AL_ENut const nal = nals[*iNal].tStartCode.uNUT;
        AL_NonVclNuts nuts = pCtx->nalParser.getNonVclNuts();
        (void)nal;
        (void)nuts;

        if(nal != nuts.fd && nal != nuts.eos && nal != nuts.eob && nal != nuts.apsSuffix)
          return false;
      }

      return true;
    }
    default:
    {
      Rtos_Assert(false);
      return false;
    }
    }
  }

  return false;
}

/*****************************************************************************/
static uint32_t GetLayerID(AL_TDecCtx* pCtx, AL_TNal const* pNal)
{
  (void)pCtx;
  (void)pNal;

  if(isHEVC(pCtx->pChanParam->eCodec))
    return pNal->tStartCode.uReserved.multilayer.uLayerID;
  return 0;
}

/*****************************************************************************/
static UNIT_ERROR DecodeOneUnit(AL_TDecCtx* pCtx, AL_TCircBuffer* pStream, AL_TNal* pNals, int32_t iNalCount, int32_t iLastVclNalInAU)
{
  /* copy start code buffer stream information into decoder stream buffer */
  Rtos_Memcpy(pCtx->pStream, pStream, sizeof(*pStream));

  int32_t iNumSlice = 0;
  bool bIsEndOfFrame = false;

  // In split input, after we sent the last vcl nal, we can get the end parsing
  // for the current buffer, which will release it. This means accessing that
  // buffer + meta after that is an use after free error.

  // The only non vcl suffix we parse are the sei suffixes, so we need to
  // parse them before the end parsing.

  // We reorder the sei nal to parse them before the non vcl suffix
  // (This assume they do not change the vcl parsing / decoding process)

  // We do not reorder the filler data suffix or aps suffix because in split input,
  // it is used like an eos / eob and it would affect the parsing / decoding process
  // and we do not use the stream in that case.
  uint32_t const uStartCodeDataEnd = pStream->iOffset;
  uint32_t const uStreamSize = pStream->tMD.uSize;

  int32_t iNal = -1;
  AL_DecodeNalStep iStep = SEND_NAL_UNTIL_LAST_VCL;
  AL_TDecUnitSearchCtx* pSearchCtx = &pCtx->SearchCtx;

  bool bIsNalProcessed = false;
  bool bIsUnitFilteredOut = false;

  for(int32_t iNalIdx = 0; iNalIdx < iNalCount; ++iNalIdx)
  {
    if(!GetNextNal(pCtx, pNals, iNalCount, iLastVclNalInAU, &iNal, &iStep) || iNal >= iNalCount)
    {
      AL_Default_Decoder_SetError(pCtx, AL_WARN_INVALID_ACCESS_UNIT_STRUCTURE, -1, true);
      return bIsNalProcessed ? SUCCESS_ACCESS_UNIT : ERR_INVALID_ACCESS_UNIT;
    }
    AL_TNal CurrentNal = pNals[iNal];
    bool bIsLastVclNal = (iNal == iLastVclNalInAU);
    bool bIsANeededLcevcUnit = false;
    uint8_t uLayerID = GetLayerID(pCtx, &CurrentNal);

    bIsUnitFilteredOut = false;
    bIsUnitFilteredOut |= (uLayerID > pCtx->uMaxLayerId);
    bIsUnitFilteredOut &= !bIsANeededLcevcUnit;

    if(bIsUnitFilteredOut)
      continue;

    AL_TDecCtx* pLayerCtx = pCtx;
    {

      AL_TStartCode tCurrentStartCode = CurrentNal.tStartCode;
      AL_TStartCode tNextStartCode = { 0 };

      bool bNextStartCodeIsAvailable = (iNal + 1) < AL_SearchDecUnit_GetCurrentNalCount(pSearchCtx);

      if(bNextStartCodeIsAvailable)
      {
        tNextStartCode = pNals[iNal + 1].tStartCode;
      }
      else
      {
        /* If there isn't a next start code, we take the end of the data processed
         * by the start code detector */
        tNextStartCode.uPosition = uStartCodeDataEnd;

      }

      Rtos_Assert(pLayerCtx);

      pLayerCtx->pStream->iOffset = tCurrentStartCode.uPosition;
      pLayerCtx->pStream->iAvailSize = DeltaPosition(tCurrentStartCode.uPosition, tNextStartCode.uPosition, uStreamSize);

      if(bIsLastVclNal)
        bIsEndOfFrame = true;
      bIsNalProcessed |= DecodeOneNAL(pLayerCtx, &CurrentNal, &iNumSlice, bIsLastVclNal);

      if(pLayerCtx->eChanState == CHAN_INVALID)
        return ERR_UNIT_INVALID_CHANNEL;

      if(pLayerCtx->error == AL_ERR_NO_MEMORY)
        return ERR_UNIT_DYNAMIC_ALLOC;

      if(bIsLastVclNal)
        ResetCurrentFrameFlags(pLayerCtx);

      if(pLayerCtx->eChanState == CHAN_DESTROYING)
      {
        AL_SearchDecUnit_ConsumeNals(pSearchCtx, iNalCount);
        ResetCurrentFrameFlags(pLayerCtx);
        return ERR_CHAN_DESTROYING;
      }
    }
  }

  if(pCtx->pScheduler)
    AL_SearchDecUnit_ConsumeNals(pSearchCtx, iNalCount);

  if(!bIsNalProcessed && (bIsUnitFilteredOut || (pCtx->eInputMode == AL_DEC_SPLIT_INPUT))) // re-add condition plus boolean about temporal filtering XXX
    return bIsEndOfFrame ? ERR_INVALID_ACCESS_UNIT : ERR_INVALID_NAL_UNIT;

  return bIsEndOfFrame ? SUCCESS_ACCESS_UNIT : SUCCESS_NAL_UNIT;
}

/*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_TryDecodeOneUnit(AL_TDecoder* pAbsDec, AL_TBuffer* pStream)
{
  (void)pStream;
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);
  (void)pCtx;

  AL_ECodec eCodec = pCtx->pChanParam->eCodec;

  if(AL_IS_ITU_CODEC(eCodec) || AL_IS_JPEG_OR_JPEG_XS_CODEC(eCodec) || AL_IS_AOM_CODEC(eCodec))
  {
    AL_TCircBuffer StreamBuf;
    StreamBuf.tMD.pVirtualAddr = AL_Buffer_GetData(pStream);
    StreamBuf.tMD.uPhysicalAddr = AL_Buffer_GetPhysicalAddress(pStream);
    StreamBuf.tMD.uSize = AL_Buffer_GetSize(pStream);
    StreamBuf.tMD.pAllocator = pStream->pAllocator;
    StreamBuf.tMD.hAllocBuf = pStream->hBufs[0];

    AL_TNal* pNals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

    AL_SearchDecUnit_SetStream(&pCtx->SearchCtx, AL_Buffer_GetData(pStream), AL_Buffer_GetSize(pStream));

    int32_t iLastVclNalInAU = LAST_VCL_NAL_IN_AU_NOT_PRESENT;
    int32_t iNalCount = pCtx->eInputMode == AL_DEC_SPLIT_INPUT ? FillNalInfo(pCtx, pStream, &iLastVclNalInAU)
                        : FindNextDecodingUnit(pCtx, pStream, &iLastVclNalInAU);

    if(iNalCount == 0)
      return ERR_UNIT_NOT_FOUND;

    AL_TCircMetaData const* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);
    StreamBuf.iOffset = pMeta->iOffset;
    StreamBuf.iAvailSize = pMeta->iAvailSize;

    UNIT_ERROR ret = DecodeOneUnit(pCtx, &StreamBuf, pNals, iNalCount, iLastVclNalInAU);

    if(pCtx->eInputMode == AL_DEC_SPLIT_INPUT)
      AL_SearchDecUnit_ResetNals(&pCtx->SearchCtx);

    pCtx->tConceal.bSkipRemainingNals = false;
    pCtx->tConceal.iSliceDecodeIssued = 0;

    return ret;
  }
  return ERR_UNIT_FAILED;
}

/*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_DecodeOneUnit(AL_TDecoder* pAbsDec, AL_TCircBuffer* pStream, AL_TNal* pNals, int32_t iNalCount, int32_t iLastVclNalInAU)
{
  (void)pAbsDec;
  (void)pStream;
  (void)pNals;
  (void)iNalCount;
  (void)iLastVclNalInAU;

  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);

  if(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec)
     )
  {
    return DecodeOneUnit(pCtx, pStream, pNals, iNalCount, iLastVclNalInAU);
  }

  return ERR_UNIT_INVALID_CHANNEL;
}

/*****************************************************************************/
bool AL_Default_Decoder_PushStreamBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  if(pCtx->eInputMode == AL_DEC_SPLIT_INPUT)
  {
    if(uFlags == AL_STREAM_BUF_FLAG_UNKNOWN)
      return false;

    AL_TStreamMetaData* pMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_STREAM);

    if(pMeta == NULL)
    {
      pMeta = AL_StreamMetaData_Create(1);

      if(pMeta == NULL)
        return false;

      if(!AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pMeta))
      {
        AL_MetaData_Destroy((AL_TMetaData*)pMeta);
        return false;
      }
    }

    AL_StreamMetaData_ClearAllSections(pMeta);

    if(uFlags & AL_STREAM_BUF_FLAG_ENDOFFRAME)
      AL_StreamMetaData_AddSection(pMeta, 0, uSize, AL_SECTION_END_FRAME_FLAG);
  }

  return AL_Feeder_PushBuffer(pCtx->Feeder, pBuf, uSize, false);
}

/*****************************************************************************/
bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize)
{
  uint8_t uFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

  AL_TStreamMetaData const* pStreamMeta = (AL_TStreamMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_STREAM);

  if(pStreamMeta != NULL)
  {
    uFlags = AL_STREAM_BUF_FLAG_ENDOFSLICE;

    if((pStreamMeta->uNumSection != 0) && (pStreamMeta->pSections[0].eFlags & AL_SECTION_END_FRAME_FLAG))
      uFlags |= AL_STREAM_BUF_FLAG_ENDOFFRAME;
  }

  return AL_Default_Decoder_PushStreamBuffer(pAbsDec, pBuf, uSize, uFlags);
}

/*****************************************************************************/
void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_Feeder_Flush(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_WaitFrameSent(AL_TDecoder* pAbsDec, uint32_t uStreamOffset)
{
  (void)uStreamOffset;

  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);
  {
    for(int32_t iSem = 0; iSem < pCtx->iStackSize; ++iSem)
      Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);

    for(int32_t iSem = 0; iSem < pCtx->iStackSize; ++iSem)
      Rtos_ReleaseSemaphore(pCtx->Sem);
  }
}

/*****************************************************************************/
bool AL_Default_Decoder_HasOngoingFrame(AL_TDecCtx* pCtx)
{
  return pCtx->bFirstIsValid && pCtx->tCurrentFrameCtx.bFirstSliceValid;
}

/*****************************************************************************/
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_SearchDecUnit_ResetNals(&pCtx->SearchCtx);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = 0;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  AL_Feeder_Reset(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);

  AL_Default_Decoder_WaitFrameSent(pAbsDec, 0xFFFFFFFF);

  AL_PictMngr_Flush(&pCtx->PictMngr);

  AL_Default_Decoder_FlushInput(pAbsDec);

  // Send eos & get last frames in dpb if any
  AL_sDecoder_CallDisplay(pCtx);

  AL_PictMngr_Terminate(&pCtx->PictMngr);

  {
    pCtx->tDecCB.displayCB.func(NULL, NULL, pCtx->tDecCB.displayCB.userParam);
  }
}

/*****************************************************************************/
static bool CheckDisplayBufferCanBeUsed(AL_TDecCtx* pCtx, AL_TBuffer* pBuf)
{
  if(!pBuf)
    return false;

  int32_t const iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);

  if(iPitchY <= 0)
    return false;

  if(iPitchY % 64 != 0)
    return false;

  AL_TStreamSettings const* pSettings = &pCtx->tInitialStreamSettings;
  bool bEnableDisplayCompression;
  AL_EFbStorageMode const eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, pSettings->iBitDepth, &bEnableDisplayCompression);
  AL_ESamplePackMode eSamplePackMode = AL_SAMPLE_PACK_MODE_BYTE;

  if(eDisplayStorageMode == AL_FB_TILE_32x4 || eDisplayStorageMode == AL_FB_TILE_64x4)
    eSamplePackMode = AL_SAMPLE_PACK_MODE_PACKED;

  AL_TPicFormat tPicFormat;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(pSettings->eChroma);
  tPicFormat.eComponentOrder = AL_COMPONENT_ORDER_YUV;
  tPicFormat.eChromaMode = pSettings->eChroma;
  tPicFormat.eStorageMode = eDisplayStorageMode;
  tPicFormat.uBitDepth = pSettings->iBitDepth;
  tPicFormat.eAlphaMode = AL_ALPHA_MODE_DISABLED;
  tPicFormat.eSamplePackMode = eSamplePackMode;
  tPicFormat.bCompressed = false;
  tPicFormat.bMSB = false;

  if(iPitchY < (int)AL_Decoder_GetMinPitch(pSettings->tDim.iWidth, &tPicFormat))
    return false;

  AL_TDimension tOutputDim = pSettings->tDim;

  if(iPitchY < (int)AL_Decoder_GetMinPitch(tOutputDim.iWidth, &tPicFormat))
    return false;

  if(pSettings->eChroma == AL_CHROMA_4_4_4)
  {
    int32_t const iPitchU = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_U);
    int32_t const iPitchV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_V);

    if((iPitchY != iPitchU) || (iPitchU != iPitchV))
      return false;
  }
  else if(pSettings->eChroma != AL_CHROMA_MONO)
  {
    int32_t const iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);

    if(iPitchY != iPitchUV)
      return false;
  }
  return true;
}

/*****************************************************************************/
/* This function should be a basis for checking if output settings set by the user are supported */
static AL_ERR CheckOutputSettingsValidity(AL_TStreamSettings const* pStreamSettings, AL_TDecOutputSettings const* pDecOutputSettings, AL_ECodec eCodec)
{
  (void)pStreamSettings;
  (void)pDecOutputSettings;
  (void)eCodec;
  AL_ERR err = AL_SUCCESS;

  return err;
}

/*****************************************************************************/
static bool CheckErrorCode(AL_TDecCtx* pCtx, AL_ERR err)
{
  if(!AL_IS_SUCCESS_CODE(err))
  {
    AL_Default_Decoder_SetError(pCtx, err, -1, true);
    return false;
  }

  return true;
}

/*****************************************************************************/
static bool ApplyOutputSettings(AL_TDecCtx* pCtx, AL_TDecOutputSettings const* pDecOutputSettings)
{
  AL_TAllocator* pAllocator = NULL;

  if(!AL_PictMngr_CompleteInit(&pCtx->PictMngr, pAllocator, pDecOutputSettings))
    return false;

  return true;
}

/*****************************************************************************/
bool AL_Default_Decoder_ConfigureOutputSettings(AL_TDecoder* pAbsDec, AL_TDecOutputSettings const* pDecOutputSettings)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  if(NULL == pCtx)
    return false;

  if(NULL == pDecOutputSettings)
    return false;

  /* For the moment, output parameters can only be set once, but the function returns true
     when its called a second time to avoid causing an error during pre-allocation. */
  if(AL_PictMngr_IsInitComplete(&pCtx->PictMngr))
  {
    return true;
  }

  AL_TDecOutputSettings tDecOutputSettings = *pDecOutputSettings;

  AL_ERR err = CheckOutputSettingsValidity(&pCtx->tCurrentStreamSettings, pDecOutputSettings, pCtx->pChanParam->eCodec);

  if(!CheckErrorCode(pCtx, err))
    return false;

  pCtx->pChanParam->tOutputSettings = tDecOutputSettings;

  bool (* pfnConfiguration)(AL_TDecCtx*, AL_TDecOutputSettings const*) = ApplyOutputSettings;

  if(!pfnConfiguration(pCtx, &tDecOutputSettings))
    return false;

  Rtos_SetEvent(pCtx->hDecOutSettingsConfiguredEvt);

  return true;
}

/*****************************************************************************/
bool AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, uint8_t uLayerID, AL_TBuffer* pDecPict)
{
  (void)uLayerID;

  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);

  bool bSucceed = CheckDisplayBufferCanBeUsed(pCtx, pDecPict);

  bSucceed = AL_PictMngr_PutDisplayBuffer(&pCtx->PictMngr, pDecPict);

  if(bSucceed)
    AL_Feeder_Signal(pCtx->Feeder);

  return bSucceed;
}

/*****************************************************************************/
int32_t AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);

  return pCtx->tInitialStreamSettings.iBitDepth;
}

/*****************************************************************************/
static int32_t Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int32_t iCurOffset;
  Rtos_GetMutex(pCtx->DecMutex);
  iCurOffset = pCtx->iCurOffset;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return iCurOffset;
}

/*****************************************************************************/
int32_t AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec)
{
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);
  return Secure_GetStrOffset(pCtx);
}

/*****************************************************************************/
int32_t AL_Default_Decoder_SkipParsedNals(AL_TDecoder* pAbsDec)
{
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = AL_SearchDecUnit_GetCurrentStreamOffset(&pCtx->SearchCtx);
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return pCtx->iCurOffset;
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_GetLastError(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_GetMutex(pCtx->DecMutex);
  AL_ERR ret = pCtx->error;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return ret;
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_GetFrameError(AL_TDecoder* pAbsDec, AL_TBuffer const* pBuf)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  AL_ERR error = AL_ERROR;
  bool bUseLastError = (pBuf == NULL) || (!AL_PictMngr_GetFrameDecodingError(&pCtx->PictMngr, pBuf, &error));

  if(bUseLastError)
    error = AL_Default_Decoder_GetLastError(pAbsDec);

  return error;
}

/*****************************************************************************/
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_Assert(!pCtx->bAreBuffersAllocated);

  AL_ERR error = AL_ERR_NO_MEMORY;
  pCtx->tInitialStreamSettings = pCtx->tCurrentStreamSettings;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tInitialStreamSettings;

  if(!CheckStreamSettings(pStreamSettings))
    return false;

  switch(pCtx->pChanParam->eCodec)
  {
  default: break;
  }

  int32_t iSPSMaxSlices = AL_RoundUp(pCtx->tCurrentStreamSettings.tDim.iHeight, 16) / 16;

  AL_TDecoderPoolSizes tSizes = { 0 };

  tSizes.iCompMapSize = AL_GetAllocSize_DecCompMap(pStreamSettings->tDim);

  int32_t iDpbMaxBuf = 0;
  int32_t iSizeMV = 0;
  int32_t iMaxBuf = 0;
  switch(pCtx->pChanParam->eCodec)
  {
  case AL_CODEC_AVC:
    iSPSMaxSlices = AL_AVC_GetMaxNumberOfSlices(pStreamSettings->eProfile, pStreamSettings->iLevel,
                                                pCtx->pChanParam->uClkRatio, pCtx->pChanParam->uFrameRate,
                                                GetSquareBlkNumber(pStreamSettings->tDim, 16));
    tSizes.iCompDataSize = AL_GetAllocSize_AvcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);
    iDpbMaxBuf = AL_AVC_GetMaxDpbBuffers(pStreamSettings, pStreamSettings->iMaxRef);
    iMaxBuf = AL_AVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
    iSizeMV = AL_GetAllocSize_AvcMV(pStreamSettings->tDim);
    break;
  case AL_CODEC_HEVC:
    iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(pStreamSettings->iLevel);
    tSizes.iCompDataSize = AL_GetAllocSize_HevcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);
    iDpbMaxBuf = AL_HEVC_GetMaxDpbBuffers(pStreamSettings);
    iMaxBuf = AL_HEVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
    iSizeMV = AL_GetAllocSize_HevcMV(pStreamSettings->tDim);
    break;
  default:
    break;
  }

  tSizes.iWPSize = iSPSMaxSlices * WP_SLICE_SIZE;
  tSizes.iSPSize = iSPSMaxSlices * sizeof(AL_TDecSliceParam);

  if(!AL_Decoder_AllocPool(pCtx, &tSizes))
    goto fail_alloc;

  AL_TPictMngrParam tPictMngrParam;
  tPictMngrParam.eCodec = pCtx->pChanParam->eCodec;
  tPictMngrParam.uNumRef = iDpbMaxBuf;
  tPictMngrParam.eFbStorageMode = pCtx->pChanParam->eFBStorageMode;
  tPictMngrParam.uNumAnnexBuf = iMaxBuf;
  tPictMngrParam.uNumSubBuf = 2;
  size_t zSizes[] = { iSizeMV, POCBUFF_PL_SIZE };
  tPictMngrParam.zSubBufSizes = zSizes;
  tPictMngrParam.bForceDisplay = pCtx->pChanParam->bCallDecodeCallbackAtStart;
  tPictMngrParam.tOutputPosition = pCtx->tOutputPosition;

  AL_IReferenceManager* pRefMngr = NULL;

  if(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec) || AL_IS_JPEG_CODEC(pCtx->pChanParam->eCodec))
  {
    AL_TDpbInitParam tDpbParams;
    tDpbParams.uNumRef = iDpbMaxBuf;
    tDpbParams.eMode = pCtx->eDpbMode;
    pRefMngr = AL_Dpb_Create(&tDpbParams);
  }

  if(!AL_PictMngr_BasicInit(&pCtx->PictMngr, pRefMngr, &tPictMngrParam, pCtx->pAllocator))
    goto fail_alloc;

  AL_TCropInfo tCropInfo = { false, 0, 0, 0, 0 };

  error = pCtx->tDecCB.resolutionFoundCB.func(iMaxBuf, pStreamSettings, &tCropInfo, pCtx->tDecCB.resolutionFoundCB.userParam);

  if(!AL_IS_SUCCESS_CODE(error))
    goto fail_alloc;

  pCtx->bAreBuffersAllocated = true;

  return true;
  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1, false);
  return false;
}

/*****************************************************************************/
static bool CheckDecodeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit != AL_DEC_UNIT_MAX_ENUM;
}

/*****************************************************************************/
static bool CheckRecStorageMode(AL_TDecSettings const* pSettings)
{
  if(pSettings->eFBStorageMode == AL_FB_RASTER)
    return true;

  return false;
}

/*****************************************************************************/
static bool CheckSettings(AL_TDecSettings const* pSettings)
{
  if(!CheckStreamSettings(&pSettings->tStream))
    return false;

  int32_t const iStack = pSettings->iStackSize;

  if((iStack < 1) || (iStack > AL_DEC_SW_MAX_STACK_SIZE))
    return false;

  if((pSettings->uDDRWidth != 16) && (pSettings->uDDRWidth != 32) && (pSettings->uDDRWidth != 64))
    return false;

  if((pSettings->uFrameRate == 0) && pSettings->bForceFrameRate)
    return false;

  if(!CheckDecodeUnit(pSettings->eDecUnit))
    return false;

  if(isSubframeUnit(pSettings->eDecUnit) && (pSettings->bParallelWPP || (pSettings->eDpbMode != AL_DPB_NO_REORDERING)))
    return false;

  if(!CheckRecStorageMode(pSettings))
    return false;

  return true;
}

/*****************************************************************************/
static void AssignSettings(AL_TDecCtx* pCtx, AL_TDecSettings const* pSettings)
{
  pCtx->uMaxLayerId = 0;
  pCtx->eInputMode = pSettings->eInputMode;
  pCtx->iStackSize = pSettings->iStackSize;
  pCtx->bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->uConcealMaxFps = pSettings->uConcealMaxFps;
  pCtx->eDpbMode = pSettings->eDpbMode;
  pCtx->tCurrentStreamSettings = pSettings->tStream;
  pCtx->tInitialStreamSettings = pSettings->tStream;
  pCtx->bUseIFramesAsSyncPoint = pSettings->bUseIFramesAsSyncPoint;

  AL_TDecChanParam* pChan = pCtx->pChanParam;
  pChan->uMaxLatency = pSettings->iStackSize;
  pChan->uNumCore = pSettings->uNumCore;
  pChan->bForceMultipleDec1PerFrame = pSettings->bForceMultipleDec1PerFrame;
  pChan->bNonRealtime = pSettings->bNonRealtime;
  pChan->uClkRatio = pSettings->uClkRatio;
  pChan->uFrameRate = pSettings->uFrameRate == 0 ? pSettings->uClkRatio : pSettings->uFrameRate;
  pChan->bDisableCache = pSettings->bDisableCache;
  pChan->bFrameBufferCompression = pSettings->bFrameBufferCompression;
  pChan->eFBStorageMode = pSettings->eFBStorageMode;
  pChan->uDDRWidth = pSettings->uDDRWidth == 16 ? 0 : pSettings->uDDRWidth == 32 ? 1 : 2;
  pChan->bParallelWPP = pSettings->bParallelWPP;
  pChan->bLowLat = pSettings->bLowLat;
  pChan->bCallDecodeCallbackAtStart = pSettings->bUseEarlyCallback;
  pChan->eCodec = pSettings->eCodec;
  pChan->eMaxChromaMode = pSettings->tStream.eChroma;
  pChan->eDecUnit = pSettings->eDecUnit;
}

/*****************************************************************************/
static bool CheckCallBacks(AL_TDecCallBacks* pCallbacks)
{
  if(pCallbacks->endDecodingCB.func == NULL)
    return false;

  if(pCallbacks->displayCB.func == NULL)
    return false;

  if(pCallbacks->resolutionFoundCB.func == NULL)
    return false;

  if(pCallbacks->errorCB.func == NULL)
    return false;

  return true;
}

/*****************************************************************************/
static void AssignCallBacks(AL_TDecCtx* pCtx, AL_TDecCallBacks const* pCB)
{
  pCtx->tDecCB = *pCB;
}

/*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError, int32_t iFrameID, bool bTriggerCB)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->error = eError;
  Rtos_ReleaseMutex(pCtx->DecMutex);

  if(iFrameID != -1)
    AL_PictMngr_UpdateDisplayBufferError(&pCtx->PictMngr, (uint8_t)iFrameID, eError);

  if(bTriggerCB && pCtx->tDecCB.errorCB.func)
    pCtx->tDecCB.errorCB.func(eError, pCtx->tDecCB.errorCB.userParam);
}

/*****************************************************************************/
static void InitAUP(AL_TDecCtx* pCtx)
{
  (void)pCtx;

  if(isAVC(pCtx->pChanParam->eCodec))
    AL_AVC_InitAUP(&pCtx->aup.avcAup);

  if(isHEVC(pCtx->pChanParam->eCodec))
    AL_HEVC_InitAUP(&pCtx->aup.hevcAup);

  AL_HDRSEIs_Reset(&pCtx->aup.tParsedHDRSEIs);
  AL_HDRSEIs_Reset(&pCtx->aup.tActiveHDRSEIs);

}

static AL_TBuffer* AllocEosBuffer(AL_ECodec eCodec, bool bSplitInput, AL_TAllocator* pAllocator)
{
  (void)bSplitInput;
  (void)pAllocator;
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AllocEosBufferAVC(bSplitInput, pAllocator);
  case AL_CODEC_HEVC:
    return AllocEosBufferHEVC(bSplitInput, pAllocator);
  default:
    return NULL;
  }
}

/*****************************************************************************/
static AL_TFeeder* CreateFeeder(AL_TDecoder const* pDec, AL_TDecSettings* pSettings, AL_TAllocator* pAllocator)
{
  AL_TDecCtx const* pCtx = &pDec->ctx;
  switch(pCtx->eInputMode)
  {
  case AL_DEC_SPLIT_INPUT:
  {
    bool bEOSParsingCB = false;
    int32_t iInputFifoSize = 256;
    return AL_SplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pCtx->eosBuffer, bEOSParsingCB);
  }

  case AL_DEC_UNSPLIT_INPUT:
  {
    int32_t iInputFifoSize = 256;
    int32_t iBufferStreamSize = pSettings->iStreamBufSize;

    if(iBufferStreamSize == 0)
      iBufferStreamSize = GetCircularBufferSize(pCtx->pChanParam->eCodec, pCtx->iStackSize, &pCtx->tCurrentStreamSettings);

    bool bForceAccessUnitDestroy = true;

    return AL_UnsplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pAllocator, iBufferStreamSize, pCtx->eosBuffer, bForceAccessUnitDestroy);
  }

  default:
    Rtos_Assert(false);
  }

  return NULL;
}

/*****************************************************************************/
static void UpdateAup(AL_TDecCtx* pBaseCtx, AL_TAup* pAup)
{
  switch(pBaseCtx->pChanParam->eCodec)
  {
  case AL_CODEC_AVC:
    pAup->avcAup.pSPS = pBaseCtx->aup.avcAup.SPSs;
    pAup->avcAup.pPPS = pBaseCtx->aup.avcAup.PPSs;
    break;
  case AL_CODEC_HEVC:
    pAup->hevcAup.pVPS = pBaseCtx->aup.hevcAup.VPSs;
    pAup->hevcAup.pSPS = pBaseCtx->aup.hevcAup.SPSs;
    pAup->hevcAup.pPPS = pBaseCtx->aup.hevcAup.PPSs;
    break;
  default:
    break;
  }
}

/*****************************************************************************/

/*****************************************************************************/
AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  if(!CheckSettings(pSettings))
    return AL_ERR_REQUEST_MALFORMED;

  if((pCB != NULL) && !CheckCallBacks(pCB))
    return AL_ERR_REQUEST_MALFORMED;

  AL_TDecoder* pDec = Rtos_Calloc(1, sizeof(*pDec));
  AL_ERR errorCode = AL_ERROR;

  if(pDec == NULL)
    return AL_ERR_NO_MEMORY;

  AL_TDecCtx* pCtx = &pDec->ctx;

  pCtx->pStream = &pCtx->StreamPrivate;
  pCtx->pScheduler = pScheduler;
  pCtx->hChannel = NULL;
  pCtx->hStartCodeChannel = NULL;
  pCtx->pAllocator = pAllocator;

  if(!AL_PictMngr_PreInit(&pCtx->PictMngr))
    return AL_ERR_NO_MEMORY;

#define SAFE_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
    { \
      errorCode = AL_ERR_NO_MEMORY; \
      goto cleanup; \
    } \
  } while(0)

  SAFE_ALLOC(pCtx, &pCtx->tMDChanParam, sizeof(AL_TDecChanParam), "chp");
  pCtx->pChanParam = (AL_TDecChanParam*)pCtx->tMDChanParam.pVirtualAddr;
  Rtos_Memset(pCtx->pChanParam, 0, sizeof(*pCtx->pChanParam));

  AL_DecFrameCtx_Reset(&pCtx->tCurrentFrameCtx);

  AssignSettings(pCtx, pSettings);

  if(pCB)
    AssignCallBacks(pCtx, pCB);

  if(AL_IS_ITU_CODEC(pSettings->eCodec))
    UpdateAup(pCtx, &pCtx->aup);

  pCtx->Sem = Rtos_CreateSemaphore(pCtx->iStackSize);
  pCtx->ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->hDecOutSettingsConfiguredEvt = Rtos_CreateEvent(0);
  pCtx->DecMutex = Rtos_CreateMutex();

  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, "Ref", 0, 0, false, false);

  // initialize decoder context
  pCtx->bIntraOnlyProfile = false;
  pCtx->bStillPictureProfile = false;
  pCtx->bFirstIsValid = false;
  pCtx->uNoRaslOutputFlag = 1;
  pCtx->bIsFirstPicture = true;
  pCtx->bAreFirstStreamSettingsChecked = false;
  pCtx->bAreBuffersAllocated = false;
  pCtx->iNumSlicesRemaining = 0;

  AL_Conceal_Init(&pCtx->tConceal);
  // initialize decoder counters
  pCtx->iCurPocLsb = INT32_MAX;
  pCtx->uToggle = 0;
  pCtx->iNumFrmBlk1 = 0;
  pCtx->iNumFrmBlk2 = 0;
  pCtx->iCurOffset = 0;
  pCtx->iTraceCounter = 0;
  pCtx->eChanState = CHAN_UNINITIALIZED;

  // initialize slice toggle information
  pCtx->uCurID = 0;

  InitAUP(pCtx);

  bool bIsITU = AL_IS_ITU_CODEC(pSettings->eCodec);
  bool bUseScdBuffer = bIsITU;

  if(bUseScdBuffer)
  {
    uint32_t uScdBufSize = 0;

    if(AL_IS_ITU_CODEC(pSettings->eCodec) || AL_IS_JPEG_OR_JPEG_XS_CODEC(pSettings->eCodec))
      uScdBufSize = ITU_JPEG_SCD_SIZE;

    // Alloc Start Code Detector buffer
    SAFE_ALLOC(pCtx, &pCtx->BufSCD.tMD, uScdBufSize, "scd");
    AL_CleanupMemory(pCtx->BufSCD.tMD.pVirtualAddr, pCtx->BufSCD.tMD.uSize);

    SAFE_ALLOC(pCtx, &pCtx->SCTable.tMD, MAX_NAL_UNIT * sizeof(AL_TNal), "sctable");
    AL_CleanupMemory(pCtx->SCTable.tMD.pVirtualAddr, pCtx->SCTable.tMD.uSize);

    AL_SearchDecUnit_Init(&pCtx->SearchCtx, pSettings->eCodec, pSettings->eDecUnit, (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr, MAX_NAL_UNIT);
  }

  if(AL_IS_ITU_CODEC(pSettings->eCodec))
  {
    // Alloc Decoder Deanti-emulated buffer for high level syntax parsing
    pCtx->BufNoAE.tMD.pVirtualAddr = Rtos_Malloc(NON_VCL_NAL_SIZE);

    if(!pCtx->BufNoAE.tMD.pVirtualAddr)
    {
      errorCode = AL_ERR_NO_MEMORY;
      goto cleanup;
    }

    pCtx->BufNoAE.tMD.uSize = NON_VCL_NAL_SIZE;
  }

  pCtx->eosBuffer = AllocEosBuffer(pCtx->pChanParam->eCodec, pCtx->eInputMode == AL_DEC_SPLIT_INPUT, pAllocator);
  Rtos_Assert(pCtx->eosBuffer);

  if(!pCtx->eosBuffer)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto cleanup;
  }

  AL_Buffer_Ref(pCtx->eosBuffer);

  pCtx->Feeder = CreateFeeder(pDec, pSettings, pAllocator);

  if(pCtx->Feeder == NULL)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto cleanup;
  }

  bool useStartCode = true;

  pCtx->tOutputPosition = pSettings->tOutputPosition;

  if(pCtx->pScheduler && useStartCode)
  {
    errorCode = AL_IDecScheduler_CreateStartCodeChannel(&pCtx->hStartCodeChannel, pCtx->pScheduler);

    if(!AL_IS_SUCCESS_CODE(errorCode))
    {
      AL_Feeder_Destroy(pCtx->Feeder);
      pCtx->Feeder = NULL;
      goto cleanup;
    }

  }

  AL_Default_Decoder_SetError(pCtx, AL_SUCCESS, -1, false);

  *hDec = (AL_TDecoder*)pDec;

  return AL_SUCCESS;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pDec);
  return errorCode;
}

/*****************************************************************************/
AL_EFbStorageMode AL_Default_Decoder_GetDisplayStorageMode(AL_TDecCtx const* pCtx, int32_t iBitDepth, bool* pEnableCompression)
{
  (void)iBitDepth;
  AL_EFbStorageMode eDisplayStorageMode = pCtx->pChanParam->eFBStorageMode;
  *pEnableCompression = pCtx->pChanParam->bFrameBufferCompression;

  return eDisplayStorageMode;
}

/*!@}*/
