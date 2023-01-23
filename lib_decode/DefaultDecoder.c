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

#include <limits.h>

#include "DefaultDecoder.h"
#include "NalUnitParser.h"
#include "NalUnitParserPrivate.h"
#include "UnsplitBufferFeeder.h"
#include "SplitBufferFeeder.h"
#include "DecSettingsInternal.h"

#include "lib_common/Error.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/HardwareConfig.h"
#include "lib_common/SEI.h"
#include "lib_common_dec/IpDecFourCC.h"

#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/AvcUtils.h"
#include "lib_common/HevcLevelsLimit.h"
#include "lib_common/HevcUtils.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_decode/I_DecScheduler.h"
#include "lib_common/CodecHook.h"
#include "lib_common_dec/HDRMeta.h"

#include "lib_assert/al_assert.h"

#define TraceRecordEnd(pTraceHooks, unit, frameId, coreId, modId, chanId) \
  do { \
    if(pTraceHooks && pTraceHooks->RecordEnd) \
      pTraceHooks->RecordEnd(pTraceHooks->pUserParam, unit, frameId, coreId, modId, chanId, false); \
  } while(0) \

#define TraceRecordStart(pTraceHooks, unit, frameId, coreId, modId, chanId) \
  do { \
    if(pTraceHooks && pTraceHooks->RecordStart) \
      pTraceHooks->RecordStart(pTraceHooks->pUserParam, unit, frameId, coreId, modId, chanId, false); \
  } while(0) \

#define SetActiveWorker(pTraceHooks, unit, coreId, modId) \
  do { \
    if(pTraceHooks && pTraceHooks->SetActiveWorker) \
      pTraceHooks->SetActiveWorker(pTraceHooks->pUserParam, unit, coreId, modId); \
  } while(0) \


static int const LAST_VCL_NAL_IN_AU_NOT_PRESENT = -1;

/*****************************************************************************/
static void ResetStartCodes(AL_TDecCtx* pCtx)
{
  pCtx->uNumSC = 0;
}

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

/*****************************************************************************/
static bool isITU(AL_ECodec eCodec)
{
  bool bIsITU = false;
  bIsITU |= isAVC(eCodec);
  bIsITU |= isHEVC(eCodec);
  (void)eCodec;
  return bIsITU;
}

/*****************************************************************************/
static int GetCircularBufferSize(AL_ECodec eCodec, int iStack, AL_TStreamSettings const* pStreamSettings)
{
  (void)eCodec;

  int zMaxCPBSize = 50 * 1024 * 1024; /* CPB default worst case */
  int zWorstCaseNalSize = 2 << 23; /* Single frame worst case */

  if(isAVC(eCodec))
  {
    zMaxCPBSize = 120 * 1024 * 1024;
    zWorstCaseNalSize = 2 << 24;
  }

  int circularBufferSize = 0;

  if(IsAllStreamSettingsSet(pStreamSettings))
  {
    /* Circular buffer always should be able to hold one frame, therefore compute the worst case and use it as a lower bound.  */
    int const zMaxNalSize = AL_GetMaxNalSize(pStreamSettings->tDim, pStreamSettings->eChroma, pStreamSettings->iBitDepth, pStreamSettings->eProfile, pStreamSettings->iLevel); /* Worst case: (5/3)*PCM + Worst case slice Headers */
    int const zRealworstcaseNalSize = AL_GetMitigatedMaxNalSize(pStreamSettings->tDim, pStreamSettings->eChroma, pStreamSettings->iBitDepth); /* Reasonnable: PCM + Slice Headers */
    circularBufferSize = UnsignedMax(zMaxNalSize, iStack * zRealworstcaseNalSize);
  }
  else
    circularBufferSize = zWorstCaseNalSize * iStack;

  /* Get minimum between absolute CPB worst case and computed value. */
  int const bufferSize = UnsignedMin(circularBufferSize, zMaxCPBSize);

  /* Round up for hardware. */
  return GetAlignedStreamBufferSize(bufferSize);
}

/*****************************************************************************/
static AL_TDecCtx* AL_sGetContext(AL_TDecoder* pDec)
{
  return &(pDec->ctx);
}

/*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, TMemDesc* pMD, uint32_t uSize, char const* name)
{
  return MemDesc_AllocNamed(pMD, pCtx->pAllocator, uSize, name);
}

/*****************************************************************************/
static void AL_Decoder_Free(TMemDesc* pMD)
{
  MemDesc_Free(pMD);
}

/*****************************************************************************/
static bool isSubframeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit == AL_VCL_NAL_UNIT;
}

/*****************************************************************************/
static void AL_sDecoder_CallEndParsing(AL_TDecCtx* pCtx, AL_TBuffer* pParsedFrame, int iParsingID)
{
  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
  {
    if(pCtx->tDecCB.endParsingCB.func)
      pCtx->tDecCB.endParsingCB.func(pParsedFrame, pCtx->tDecCB.endParsingCB.userParam, iParsingID);
    return;
  }
  int const numHandles = AL_HandleMetaData_GetNumHandles(pHandlesMeta);

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
  {
    (void)numHandles;
    AL_Assert(numHandles == 1);
  }
  AL_Assert(iParsingID < numHandles);

  // The handles should be stored in the slice order.
  AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, iParsingID);

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSING)
  {
    AL_TBuffer* pStream = pDecMetaHandle->pHandle;
    pDecMetaHandle->eState = AL_DEC_HANDLE_STATE_PROCESSED;

    if(pCtx->tDecCB.endParsingCB.func)
      pCtx->tDecCB.endParsingCB.func(pParsedFrame, pCtx->tDecCB.endParsingCB.userParam, iParsingID);
    AL_Feeder_FreeBuf(pCtx->Feeder, pStream);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallDecode(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
  AL_Assert(pDecodedFrame);

  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pDecodedFrame, AL_META_TYPE_HANDLE);

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    AL_sDecoder_CallEndParsing(pCtx, pDecodedFrame, 0);
  else if(pHandlesMeta)
  {
    int numHandles = AL_HandleMetaData_GetNumHandles(pHandlesMeta);

    for(int handle = 0; handle < numHandles; ++handle)
    {
      AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, handle);
      AL_TBuffer* pStream = pDecMetaHandle->pHandle;

      if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSING)
      {
        AL_Feeder_FreeBuf(pCtx->Feeder, pStream);
      }
    }
  }

  if(!pCtx->pChanParam->bUseEarlyCallback)
    pCtx->tDecCB.endDecodingCB.func(pDecodedFrame, pCtx->tDecCB.endDecodingCB.userParam);

  if(!pHandlesMeta)
    return;

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
static bool AL_sDecoder_TryDisplayOneFrame(AL_TDecCtx* pCtx, int iFrameID)
{
  bool bEarlyDisplay = iFrameID != -1;
  AL_TInfoDecode tInfo = { 0 };
  bool bStartsNewCVS = false;
  AL_TBuffer* pFrameToDisplay = NULL;

  if(bEarlyDisplay)
    pFrameToDisplay = AL_PictMngr_ForceDisplayBuffer(&pCtx->PictMngr, &tInfo, &bStartsNewCVS, iFrameID);
  else
    pFrameToDisplay = AL_PictMngr_GetDisplayBuffer(&pCtx->PictMngr, &tInfo, &bStartsNewCVS);

  if(pFrameToDisplay == NULL)
    return false;

  uint8_t* pPtrIsNotNull = AL_Buffer_GetData(pFrameToDisplay);
  (void)pPtrIsNotNull;
  AL_Assert(pPtrIsNotNull != NULL);

  BuildCurrentHRD(pCtx, pFrameToDisplay, bStartsNewCVS);

  if(bEarlyDisplay || !pCtx->pChanParam->bUseEarlyCallback)
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
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_sDecoder_CallDecode(pCtx, iFrameID);
  AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Default_Decoder_EndParsing(void* pUserParam, int iFrameID, int iParsingID)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    return;

  AL_TBuffer* pParsedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
  AL_Assert(pParsedFrame);

  AL_sDecoder_CallEndParsing(pCtx, pParsedFrame, iParsingID);
}

/*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus const* pStatus)
{
  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_CMD_INVALID))
  {
    Rtos_Log(AL_LOG_CRITICAL, "\n***** /!\\ Error trying to conceal bitstream - ending decoding /!\\ *****\n");
    AL_Assert(0);
  }

  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  int iFrameID = pStatus->tBufIDs.FrmID;

  AL_PictMngr_UpdateDisplayBufferCRC(&pCtx->PictMngr, iFrameID, pStatus->uCRC);

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_NOT_FINISHED))
  {
    /* we want to notify the user, but we don't want to update the decoder state */
    AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
    AL_Assert(pDecodedFrame);
    pCtx->tDecCB.endDecodingCB.func(pDecodedFrame, pCtx->tDecCB.endDecodingCB.userParam);
    bool const bSuccess = AL_sDecoder_TryDisplayOneFrame(pCtx, iFrameID);
    (void)bSuccess;
    AL_Assert(bSuccess);
    return;
  }

  AL_PictMngr_EndDecoding(&pCtx->PictMngr, iFrameID);
  int iOffset = pCtx->iNumFrmBlk2 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->uFrameIDRefList[iOffset], pCtx->uMvIDRefList[iOffset]);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = pCtx->iStreamOffset[pCtx->iNumFrmBlk2 % pCtx->iStackSize];
  ++pCtx->iNumFrmBlk2;

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_HANGED))
    Rtos_Log(AL_LOG_CRITICAL, "\n***** /!\\ Timeout - resetting the decoder /!\\ *****\n");

  Rtos_ReleaseMutex(pCtx->DecMutex);

  AL_Feeder_Signal(pCtx->Feeder);
  AL_sDecoder_CallBacks(pCtx, iFrameID);

  Rtos_GetMutex(pCtx->DecMutex);
  int iMotionVectorID = pStatus->tBufIDs.MvID;
  AL_PictMngr_UnlockID(&pCtx->PictMngr, iFrameID, iMotionVectorID);
  Rtos_ReleaseMutex(pCtx->DecMutex);

  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseStreamBuffer(void* pUserParam, AL_TBuffer* pBufStream)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  AL_Feeder_FreeBuf(pCtx->Feeder, pBufStream);
}

/***************************************************************************/
/*                           Lib functions                                 */
/***************************************************************************/

/*****************************************************************************/
static void InitInternalBuffers(AL_TDecCtx* pCtx)
{
  MemDesc_Init(&pCtx->BufNoAE.tMD);
  MemDesc_Init(&pCtx->BufSCD.tMD);
  MemDesc_Init(&pCtx->SCTable.tMD);

  for(int i = 0; i < MAX_STACK_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PoolSclLst[i].tMD);
    MemDesc_Init(&pCtx->PoolCompData[i].tMD);
    MemDesc_Init(&pCtx->PoolCompMap[i].tMD);
    MemDesc_Init(&pCtx->PoolSP[i].tMD);
    MemDesc_Init(&pCtx->PoolWP[i].tMD);
    MemDesc_Init(&pCtx->PoolListRefAddr[i].tMD);
  }

  for(int i = 0; i < MAX_DPB_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
  }

  MemDesc_Init(&pCtx->tMDChanParam);
  pCtx->pChanParam = NULL;
}

/*****************************************************************************/
static void DeinitBuffers(AL_TDecCtx* pCtx)
{
  for(int i = 0; i < MAX_DPB_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
  }

  for(int i = 0; i < MAX_STACK_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PoolCompData[i].tMD);
    AL_Decoder_Free(&pCtx->PoolCompMap[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolWP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolListRefAddr[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSclLst[i].tMD);
  }

  AL_Decoder_Free(&pCtx->BufSCD.tMD);
  AL_Decoder_Free(&pCtx->SCTable.tMD);
  AL_Decoder_Free(&pCtx->BufNoAE.tMD);
  AL_Decoder_Free(&pCtx->tMDChanParam);
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
    AL_Assert(pPtrIsNotNull != NULL);

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
  AL_Assert(pCtx);

  AL_PictMngr_DecommitPool(&pCtx->PictMngr);

  if(pCtx->Feeder)
    AL_Feeder_Destroy(pCtx->Feeder);

  if(pCtx->eosBuffer)
    AL_Buffer_Unref(pCtx->eosBuffer);

  AL_IDecScheduler_DestroyStartCodeChannel(pCtx->pScheduler, pCtx->hStartCodeChannel);
  AL_IDecScheduler_DestroyChannel(pCtx->pScheduler, pCtx->hChannel);
  DeinitPictureManager(pCtx);
  Rtos_Free(pCtx->BufNoAE.tMD.pVirtualAddr);
  DeinitBuffers(pCtx);

  Rtos_DeleteSemaphore(pCtx->Sem);
  Rtos_DeleteEvent(pCtx->ScDetectionComplete);
  Rtos_DeleteMutex(pCtx->DecMutex);

  Rtos_Free(pDec);
}

/*****************************************************************************/
void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, const char* sPrefix, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool bShouldPrintFrameDelimiter)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  for(size_t i = 0; i < sizeof(pCtx->sTracePrefix) && sPrefix[i]; ++i)
    pCtx->sTracePrefix[i] = sPrefix[i];

  pCtx->sTracePrefix[sizeof(pCtx->sTracePrefix) - 1] = '\000';

  pCtx->iTraceFirstFrame = iFrmID;
  pCtx->iTraceLastFrame = iFrmID + iNumFrm;
  pCtx->bShouldPrintFrameDelimiter = bShouldPrintFrameDelimiter;

  if(iNumFrm > 0 || bForceCleanBuffers)
    AL_CLEAN_BUFFERS = 1;
  else
    AL_CLEAN_BUFFERS = 0;
}

/*****************************************************************************/
static bool enoughStartCode(int iNumStartCode)
{
  return iNumStartCode > 1;
}

/******************************************************************************/
static int NalHeaderSize(AL_ECodec eCodec)
{

  if(isAVC(eCodec))
  {
    return AL_AVC_NAL_HDR_SIZE;
  }

  if(isHEVC(eCodec))
  {
    return AL_HEVC_NAL_HDR_SIZE;
  }

  (void)eCodec;
  return -1;
}

/*****************************************************************************/
static bool isAud(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_AUD;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_AUD;

  (void)eCodec;
  (void)eNut;
  return false;
}

/*****************************************************************************/
static bool isEosOrEob(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return (eNut == AL_AVC_NUT_EOS) || (eNut == AL_AVC_NUT_EOB);

  if(isHEVC(eCodec))
    return (eNut == AL_HEVC_NUT_EOS) || (eNut == AL_HEVC_NUT_EOB);

  (void)eCodec;
  (void)eNut;
  return false;
}

/*****************************************************************************/
static bool isFd(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_FD;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_FD;

  (void)eCodec;
  (void)eNut;
  return false;
}

/*****************************************************************************/
static bool isPrefixSei(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_PREFIX_SEI;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_PREFIX_SEI;

  (void)eCodec;
  (void)eNut;
  return false;
}

/*****************************************************************************/
static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int iNalHdrSize = NalHeaderSize(eCodec);
  AL_Assert(iNalHdrSize);
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
static bool checkSeiUUID(uint8_t const* pBufs, AL_TNal const* pNal, AL_ECodec eCodec, int iTotalSize)
{
  (void)eCodec;
  int iTotalUUIDSize = 26;

  if(isAVC(eCodec))
    iTotalUUIDSize = 25;

  if((int)pNal->uSize != iTotalUUIDSize)
    return false;

  int iStart = 7;

  if(isAVC(eCodec))
    iStart = 6;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);

  for(int i = 0; i < iSize; i++)
  {
    int iPosition = (pNal->tStartCode.uPosition + iStart + i) % iTotalSize;

    if(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID[i] != pBufs[iPosition])
      return false;
  }

  return true;
}

/*****************************************************************************/
static bool isStartCode(uint8_t* pBuf, uint32_t uSize, uint32_t uPos)
{
  return (pBuf[uPos % uSize] == 0x00) &&
         (pBuf[(uPos + 1) % uSize] == 0x00) &&
         (pBuf[(uPos + 2) % uSize] == 0x01);
}

/*****************************************************************************/
static int getNumSliceInSei(uint8_t* pBufs, AL_TNal* pNal, AL_ECodec eCodec, int iTotalSize)
{
  (void)eCodec;
  AL_Assert(checkSeiUUID(pBufs, pNal, eCodec, iTotalSize));
  int iStart = 7;

  if(isAVC(eCodec))
    iStart = 6;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);
  int iPosition = (pNal->tStartCode.uPosition + iStart + iSize) % iTotalSize;
  return pBufs[iPosition];
}

/*****************************************************************************/
static bool isVcl(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return AL_AVC_IsVcl(eNut);

  if(isHEVC(eCodec))
    return AL_HEVC_IsVcl(eNut);

  (void)eCodec;
  (void)eNut;
  return false;
}

/* should only be used when the position is right after the nal header */
/*****************************************************************************/
static bool isFirstSlice(uint8_t* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return (pBuf[uPos] & 0x80) != 0;
}

/*****************************************************************************/
static bool isFirstSliceStatusAvailable(int iSize, int iNalHdrSize)
{
  return iSize > iNalHdrSize;
}

/*****************************************************************************/
static bool isFirstSliceNAL(AL_TNal* pNal, AL_TBuffer* pStream, AL_ECodec eCodec)
{
  uint8_t* pBuf = AL_Buffer_GetData(pStream);
  uint32_t uPos = pNal->tStartCode.uPosition;
  uint32_t uSize = AL_Buffer_GetSize(pStream);
  bool const bIsStartCode = isStartCode(pBuf, uSize, uPos);
  (void)bIsStartCode;
  AL_Assert(bIsStartCode);
  uPos = skipNalHeader(uPos, eCodec, uSize);
  return isFirstSlice(pBuf, uPos);
}

/*****************************************************************************/
static bool SearchNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* pLastStartCodeInDecodingUnit, int* iLastVclNalInDecodingUnit)
{
  (void)pStream;
  (void)pLastStartCodeInDecodingUnit;
  (void)iLastVclNalInDecodingUnit;

  if(!enoughStartCode(pCtx->uNumSC))
    return false;

  AL_TNal* pTable = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  uint8_t* pBuf = AL_Buffer_GetData(pStream);
  bool bVCLNalSeen = false;
  int iNalFound = 0;
  AL_ECodec const eCodec = pCtx->pChanParam->eCodec;
  int const iNalCount = (int)pCtx->uNumSC;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_TNal* pNal = &pTable[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(iNal > 0)
      iNalFound++;

    if(!isVcl(eCodec, eNUT))
    {
      if(isAud(eCodec, eNUT) || isEosOrEob(eCodec, eNUT))
      {
        if(bVCLNalSeen)
        {
          iNalFound--;
          *pLastStartCodeInDecodingUnit = iNalFound;
          return true;
        }
      }

      if(isPrefixSei(eCodec, eNUT) && checkSeiUUID(pBuf, pNal, eCodec, AL_Buffer_GetSize(pStream)))
      {
        pCtx->iNumSlicesRemaining = getNumSliceInSei(pBuf, pNal, eCodec, AL_Buffer_GetSize(pStream));
        AL_Assert(pCtx->iNumSlicesRemaining > 0);
      }

      if(isFd(eCodec, eNUT) && isSubframeUnit(pCtx->pChanParam->eDecUnit))
      {
        bool bIsLastSlice = pCtx->iNumSlicesRemaining == 1;

        if(bIsLastSlice && bVCLNalSeen)
        {
          *pLastStartCodeInDecodingUnit = iNalFound;
          pCtx->iNumSlicesRemaining = 0;
          return true;
        }
      }

    }

    if(isVcl(eCodec, eNUT))
    {
      int iNalHdrSize = NalHeaderSize(eCodec);
      AL_Assert(iNalHdrSize > 0);

      if(isFirstSliceStatusAvailable(pNal->uSize, iNalHdrSize))
      {
        bool bIsFirstSlice = isFirstSliceNAL(pNal, pStream, eCodec);

        if(bVCLNalSeen)
        {
          if(bIsFirstSlice)
          {
            iNalFound--;
            *pLastStartCodeInDecodingUnit = iNalFound;
            return true;
          }

          if(isSubframeUnit(pCtx->pChanParam->eDecUnit))
          {
            pCtx->iNumSlicesRemaining--;
            iNalFound--;
            *pLastStartCodeInDecodingUnit = iNalFound;
            int const iIsNotLastSlice = -1;
            *iLastVclNalInDecodingUnit = iIsNotLastSlice;
            return true;
          }
        }
      }

      bVCLNalSeen = true;
      *iLastVclNalInDecodingUnit = iNal;
    }
  }

  return false;
}

/*****************************************************************************/
int AL_AVC_GetMaxDpbBuffers(AL_TStreamSettings const* pStreamSettings, int iSPSMaxRefFrames);
int AL_HEVC_GetMaxDpbBuffers(AL_TStreamSettings const* pStreamSettings);

/*****************************************************************************/
static bool CheckAvailSpace(AL_TDecCtx* pCtx, AL_TSeiMetaData* pMeta)
{
  uint32_t uLengthNAL = GetNonVclSize(&pCtx->Stream);
  int Offset = (uintptr_t)AL_SeiMetaData_GetBuffer(pMeta) - (uintptr_t)pMeta->pBuf;
  return Offset + uLengthNAL <= pMeta->maxBufSize;
}

/*****************************************************************************/
static void CheckNALParserResult(AL_TDecCtx* pCtx, AL_PARSE_RESULT eParseResult)
{
  if(eParseResult == AL_OK)
    return;

  AL_Default_Decoder_SetError(pCtx, eParseResult == AL_UNSUPPORTED ? AL_WARN_UNSUPPORTED_NAL : AL_WARN_CONCEAL_DETECT, -1, true);
}

/*****************************************************************************/
static AL_TSeiMetaData* GetSeiMetaData(AL_TDecCtx* pCtx)
{
  if(!pCtx->pInputBuffer)
    return NULL;

  return (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pCtx->pInputBuffer, AL_META_TYPE_SEI);
}

/*****************************************************************************/
bool AL_DecodeOneNal(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut nut, bool bIsLastAUNal, int* iNumSlice)
{
  if(pCtx->parser.isNutError(nut))
    return false;

  AL_NalParser parser = pCtx->parser;
  AL_NonVclNuts nuts = parser.getNonVclNuts();

  if(parser.isSliceData(nut))
  {
    return parser.decodeSliceData(pAUP, pCtx, nut, bIsLastAUNal, iNumSlice);
  }

  if((nut == nuts.seiPrefix || (nut == nuts.seiSuffix && pCtx->bIsBuffersAllocated)) && parser.parseSei)
  {
    bool bIsPrefix = (nut == nuts.seiPrefix);
    AL_TSeiMetaData* pMeta = GetSeiMetaData(pCtx);

    if(pMeta && !CheckAvailSpace(pCtx, pMeta))
    {
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, -1, true);
      return false;
    }

    AL_TRbspParser rp = pMeta ? getParserOnNonVclNal(pCtx, AL_SeiMetaData_GetBuffer(pMeta)) : getParserOnNonVclNalInternalBuf(pCtx);

    if(!parser.parseSei(pAUP, &rp, bIsPrefix, &pCtx->tDecCB.parsedSeiCB, pMeta))
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, -1, true);
  }

  if(nut == nuts.sps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_PARSE_RESULT eParserResult = parser.parseSps(pAUP, &rp, pCtx);
    CheckNALParserResult(pCtx, eParserResult);
  }

  if(nut == nuts.pps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_PARSE_RESULT eParserResult = parser.parsePps(pAUP, &rp, pCtx);
    CheckNALParserResult(pCtx, eParserResult);
  }

  if(nut == nuts.vps && parser.parseVps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_PARSE_RESULT eParserResult = parser.parseVps(pAUP, &rp);
    CheckNALParserResult(pCtx, eParserResult);
  }

  if((nut == nuts.apsPrefix || nut == nuts.apsSuffix) && parser.parseAps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_PARSE_RESULT eParserResult = parser.parseAps(pAUP, &rp, pCtx);
    CheckNALParserResult(pCtx, eParserResult);
  }

  if(nut == nuts.ph && parser.parsePh)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    AL_PARSE_RESULT eParserResult = parser.parsePh(pAUP, &rp, pCtx);
    CheckNALParserResult(pCtx, eParserResult);
  }

  if((nut == nuts.eos) || (nut == nuts.eob))
  {
    if(pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid)
      parser.finishPendingRequest(pCtx);
    pCtx->bIsFirstPicture = true;
  }

  return false;
}

/*****************************************************************************/
static bool DecodeOneNAL(AL_TDecCtx* pCtx, AL_TNal* pNal, int* pNumSlice, bool bIsLastVclNal)
{
  if(pCtx->PictMngr.uNumSlice > 0 && *pNumSlice > pCtx->pChanParam->iMaxSlices)
    return false;

  return AL_DecodeOneNal(&pCtx->aup, pCtx, pNal->tStartCode.uNUT, bIsLastVclNal, pNumSlice);

  (void)pNal;
  (void)bIsLastVclNal;
  return false;
}

/*****************************************************************************/
static bool canStoreMoreStartCodes(AL_TDecCtx* pCtx)
{
  return (pCtx->uNumSC + 1) * sizeof(AL_TNal) <= pCtx->SCTable.tMD.uSize - pCtx->BufSCD.tMD.uSize;
}

/*****************************************************************************/
static size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  return uSize + uSecondPos - uFirstPos;
}

/*****************************************************************************/
static void updateStartCodeNumber(AL_TDecCtx* pCtx, AL_TScBufferAddrs* ScdBuffer, AL_TBuffer* pStream, AL_TCircMetaData* pMeta, TMemDesc* startCodeArray, uint16_t numSC)
{
  pMeta->iOffset = (pMeta->iOffset + pCtx->ScdStatus.uNumBytes) % AL_Buffer_GetSize(pStream);
  pMeta->iAvailSize -= pCtx->ScdStatus.uNumBytes;

  AL_TStartCode* src = (AL_TStartCode*)startCodeArray->pVirtualAddr;

  AL_TNal* dst = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  if(pCtx->uNumSC && numSC)
    dst[pCtx->uNumSC - 1].uSize = DeltaPosition(dst[pCtx->uNumSC - 1].tStartCode.uPosition, src[0].uPosition, ScdBuffer->uMaxSize);

  for(int i = 0; i < numSC; i++)
  {
    dst[pCtx->uNumSC].tStartCode = src[i];

    if(i + 1 == numSC)
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, pMeta->iOffset, ScdBuffer->uMaxSize);
    else
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, src[i + 1].uPosition, ScdBuffer->uMaxSize);

    pCtx->uNumSC++;
  }
}

/*****************************************************************************/
static AL_TScBufferAddrs initScdBuffer(AL_TBuffer* pStream, AL_TCircMetaData* pMeta, TMemDesc* startCodeOutputArray)
{
  AL_TScBufferAddrs ScdBuffer = { 0 };
  ScdBuffer.pBufOut = startCodeOutputArray->uPhysicalAddr;
  ScdBuffer.pStream = AL_Buffer_GetPhysicalAddress(pStream);
  ScdBuffer.uMaxSize = AL_Buffer_GetSize(pStream);
  ScdBuffer.uOffset = pMeta->iOffset;
  ScdBuffer.uAvailSize = pMeta->iAvailSize;

  return ScdBuffer;
}

/*****************************************************************************/
static void GenerateScdIpTraces(AL_TDecCtx* pCtx, AL_TScBufferAddrs ScdBuffer, AL_TBuffer* pStream, TMemDesc scBuffer)
{
  (void)pCtx;
  (void)ScdBuffer;
  (void)pStream;
  (void)scBuffer;

}

/*************************************************************************//*!
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current start code searching status
*****************************************************************************/
static void EndScd(void* pUserParam, AL_TScStatus const* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  pCtx->ScdStatus.uNumBytes = pStatus->uNumBytes;
  pCtx->ScdStatus.uNumSC = pStatus->uNumSC;

  Rtos_SetEvent(pCtx->ScDetectionComplete);
}

/*****************************************************************************/
static bool RefillStartCodes(AL_TDecCtx* pCtx, AL_TBuffer* pStream)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);
  int const SCDHardwareConstraintMinSize = 5;

  if(pMeta->iAvailSize < SCDHardwareConstraintMinSize)
    return false;

  TMemDesc startCodeOutputArray = pCtx->BufSCD.tMD;
  AL_TScBufferAddrs ScdBuffer = initScdBuffer(pStream, pMeta, &startCodeOutputArray);

  AL_CleanupMemory(startCodeOutputArray.pVirtualAddr, startCodeOutputArray.uSize);

  AL_TDecScheduler_CB_EndStartCode callback = { EndScd, pCtx };

  AL_TScParam ScParam = { 0 };
  ScParam.MaxSize = startCodeOutputArray.uSize / sizeof(AL_TStartCode);
  ScParam.eCodec = pCtx->pChanParam->eCodec;

  /* if the start code couldn't be launched because the start code queue is full,
   * we have to retry as failing to do so will make us return ERR_UNIT_NOT_FOUND which
   * means that the whole chunk of data doesn't have a startcode in it and
   * for example that we can stop sending new decoding request if there isn't
   * more input data. */

  do
  {
    AL_IDecScheduler_SearchSC(pCtx->pScheduler, pCtx->hStartCodeChannel, &ScParam, &ScdBuffer, callback);
    Rtos_WaitEvent(pCtx->ScDetectionComplete, AL_WAIT_FOREVER);

    if(pCtx->ScdStatus.uNumBytes == 0)
    {
      Rtos_Log(AL_LOG_CRITICAL, "***** /!\\ Warning: Start code queue was full, degraded mode, retrying. /!\\ *****\n");
      Rtos_Sleep(1);
    }
  }
  while(pCtx->ScdStatus.uNumBytes == 0);

  AL_TStartCode* src = (AL_TStartCode*)startCodeOutputArray.pVirtualAddr;
  Rtos_InvalidateCacheMemory(src, startCodeOutputArray.uSize);
  GenerateScdIpTraces(pCtx, ScdBuffer, pStream, startCodeOutputArray);
  updateStartCodeNumber(pCtx, &ScdBuffer, pStream, pMeta, &startCodeOutputArray, pCtx->ScdStatus.uNumSC);

  return pCtx->ScdStatus.uNumSC > 0;
  return false;
}

/*****************************************************************************/
static int FindNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* iLastVclNalInAU)
{
  int iLastStartCodeIdx = 0;

  while(!SearchNextDecodingUnit(pCtx, pStream, &iLastStartCodeIdx, iLastVclNalInAU))
  {
    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      ResetStartCodes(pCtx);
    }

    if(!RefillStartCodes(pCtx, pStream))
      return 0;
  }

  return iLastStartCodeIdx + 1;
}

/*****************************************************************************/
static int FillNalInfo(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* iLastVclNalInAU)
{
  (void)iLastVclNalInAU;
  pCtx->pInputBuffer = pStream;

  while(RefillStartCodes(pCtx, pStream) != false)
    ;

  int iNalCount = pCtx->uNumSC;

  if(isITU(pCtx->pChanParam->eCodec))
  {
    AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
    bool bSearchLastVCLNal = false;

    for(int curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection)
    {
      if(pStreamMeta->pSections[curSection].eFlags & AL_SECTION_END_FRAME_FLAG)
      {
        bSearchLastVCLNal = true;
        break;
      }
    }

    AL_TNal* pTable = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
    AL_ECodec const eCodec = pCtx->pChanParam->eCodec;

    for(int iNal = iNalCount - 1; iNal >= 0; --iNal)
    {
      AL_TNal* pNal = &pTable[iNal];
      AL_ENut eNUT = pNal->tStartCode.uNUT;

      if(isVcl(eCodec, eNUT))
      {
        if(bSearchLastVCLNal)
        {
          *iLastVclNalInAU = iNal;
          bSearchLastVCLNal = false;
        }

        if(isFirstSliceNAL(pNal, pStream, eCodec))
        {
          break;
        }
      }
    }
  }

  return iNalCount;
}

/*****************************************************************************/
static void ConsumeNals(AL_TDecCtx* pCtx, int iNumNal)
{
  AL_TNal* nals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  if(iNumNal)
    pCtx->iCurNalStreamOffset = (nals[iNumNal - 1].tStartCode.uPosition + nals[iNumNal - 1].uSize) % pCtx->Stream.tMD.uSize;

  pCtx->uNumSC -= iNumNal;
  Rtos_Memmove(nals, nals + iNumNal, pCtx->uNumSC * sizeof(AL_TNal));
}

/*****************************************************************************/
static void ResetValidFlags(AL_TDecCtx* pCtx)
{
  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;

}

/*****************************************************************************/
static void GetNextNal(AL_TDecCtx* pCtx, AL_TNal* nals, int iNalCount, int iLastVclNalInAU, int* iNal, AL_DecodeNalStep* step)
{
  (void)nals;
  (void)iNalCount;
  (void)step;

  if((pCtx->eInputMode != AL_DEC_SPLIT_INPUT) || (iLastVclNalInAU == LAST_VCL_NAL_IN_AU_NOT_PRESENT))
  {
    (*iNal)++;
    return;
  }

  AL_Assert(isITU(pCtx->pChanParam->eCodec) && "Unsupported codec");

  while(true)
  {
    switch(*step)
    {
    case SEND_NAL_UNTIL_LAST_VCL:
    {
      (*iNal)++;

      if(*iNal < iLastVclNalInAU)
        return;

      (*step) = SEND_REORDERED_SUFFIX;
      continue;
    }
    case SEND_REORDERED_SUFFIX:
    {
      do
        (*iNal)++;
      while(*iNal < iNalCount && !pCtx->parser.canNalBeReordered(nals[*iNal].tStartCode.uNUT));

      if(*iNal >= iNalCount)
      {
        (*iNal) = iLastVclNalInAU;
        (*step) = SEND_LAST_VCL;
        continue;
      }
      return;
    }
    case SEND_LAST_VCL:
    {
      AL_Assert(*iNal == iLastVclNalInAU);
      (*step) = SEND_REMAINING_NAL;
      return;
    }
    case SEND_REMAINING_NAL:
    {
      do
        (*iNal)++;
      while(*iNal < iNalCount && pCtx->parser.canNalBeReordered(nals[*iNal].tStartCode.uNUT));

      if(*iNal < iNalCount)
      {
        AL_ENut const nal = nals[*iNal].tStartCode.uNUT;
        AL_NonVclNuts nuts = pCtx->parser.getNonVclNuts();
        (void)nal;
        (void)nuts;
        AL_Assert(nal == nuts.fd || nal == nuts.eos || nal == nuts.eob || nal == nuts.apsSuffix);
      }

      return;
    }
    default:
    {
      AL_Assert(0);
      break;
    }
    }
  }

}

/*****************************************************************************/
static UNIT_ERROR DecodeOneUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int iNalCount, int iLastVclNalInAU)
{
  AL_TNal* nals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  /* copy start code buffer stream information into decoder stream buffer */
  pCtx->Stream.tMD.uSize = AL_Buffer_GetSize(pStream);
  pCtx->Stream.tMD.pAllocator = pStream->pAllocator;
  pCtx->Stream.tMD.hAllocBuf = pStream->hBufs[0];
  pCtx->Stream.tMD.pVirtualAddr = AL_Buffer_GetData(pStream);
  pCtx->Stream.tMD.uPhysicalAddr = AL_Buffer_GetPhysicalAddress(pStream);
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);

  int iNumSlice = 0;
  bool bIsEndOfFrame = false;

  // In split input, after we sent the last vcl nal, we can get the end parsing
  // for the current buffer, which will release it. This means accessing that
  // buffer + meta after that is an use after free error.
  //
  // The only non vcl suffix we parse are the sei suffixes, so we need to
  // parse them before the end parsing.
  //
  // We reorder the sei nal to parse them before the non vcl suffix
  // (This assume they do not change the vcl parsing / decoding process)
  //
  // We do not reorder the filler data suffix because in split input,
  // it is used like an eos / eob and it would affect the parsing / decoding process
  // and we do not use the stream in that case.
  uint32_t const StartCodeDataEnd = pMeta->iOffset;
  uint32_t const StreamSize = AL_Buffer_GetSize(pStream);

  AL_Assert(iNalCount < MAX_NAL_UNIT);
  int iNal = -1;
  AL_DecodeNalStep step = SEND_NAL_UNTIL_LAST_VCL;

  bool bIsNalProcessed = false;

  for(int iNalIdx = 0; iNalIdx < iNalCount; ++iNalIdx)
  {
    GetNextNal(pCtx, nals, iNalCount, iLastVclNalInAU, &iNal, &step);
    AL_Assert(iNal < iNalCount);
    AL_TNal CurrentNal = nals[iNal];
    AL_TStartCode CurrentStartCode = CurrentNal.tStartCode;
    AL_TStartCode NextStartCode;

    if(iNal + 1 < pCtx->uNumSC)
    {
      NextStartCode = nals[iNal + 1].tStartCode;
    }
    else /* if we didn't wait for the next start code to arrive to decode the current NAL */
    {
      /* If there isn't a next start code, we take the end of the data processed
       * by the start code detector */
      NextStartCode.uPosition = StartCodeDataEnd;
    }

    pCtx->Stream.iOffset = CurrentStartCode.uPosition;
    pCtx->Stream.iAvailSize = DeltaPosition(CurrentStartCode.uPosition, NextStartCode.uPosition, StreamSize);

    bool bIsLastVclNal = (iNal == iLastVclNalInAU);

    if(bIsLastVclNal)
      bIsEndOfFrame = true;
    bIsNalProcessed |= DecodeOneNAL(pCtx, &CurrentNal, &iNumSlice, bIsLastVclNal);

    if(pCtx->eChanState == CHAN_INVALID)
      return ERR_UNIT_INVALID_CHANNEL;

    if(pCtx->error == AL_ERR_NO_MEMORY)
      return ERR_UNIT_DYNAMIC_ALLOC;

    if(bIsLastVclNal)
      ResetValidFlags(pCtx);

    if(pCtx->eChanState == CHAN_DESTROYING)
    {
      ConsumeNals(pCtx, iNalCount);
      ResetValidFlags(pCtx);
      return ERR_UNIT_FAILED;
    }
  }

  ConsumeNals(pCtx, iNalCount);

  if(!bIsNalProcessed && (pCtx->eInputMode == AL_DEC_SPLIT_INPUT))
    return bIsEndOfFrame ? ERR_INVALID_ACCESS_UNIT : ERR_INVALID_NAL_UNIT;

  return bIsEndOfFrame ? SUCCESS_ACCESS_UNIT : SUCCESS_NAL_UNIT;
}

/*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_TryDecodeOneUnit(AL_TDecoder* pAbsDec, AL_TBuffer* pStream)
{
  AL_TDecCtx* pCtx = AL_sGetContext(pAbsDec);

  if(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec) || AL_IS_JPEG_CODEC(pCtx->pChanParam->eCodec))
  {

    int iLastVclNalInAU = LAST_VCL_NAL_IN_AU_NOT_PRESENT;
    int iNalCount = pCtx->eInputMode == AL_DEC_SPLIT_INPUT ? FillNalInfo(pCtx, pStream, &iLastVclNalInAU)
                    : FindNextDecodingUnit(pCtx, pStream, &iLastVclNalInAU);

    if(iNalCount == 0)
      return ERR_UNIT_NOT_FOUND;

    return DecodeOneUnit(pCtx, pStream, iNalCount, iLastVclNalInAU);
  }
  return ERR_UNIT_FAILED;
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

  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_STREAM);

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
    for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
      Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);

    for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
      Rtos_ReleaseSemaphore(pCtx->Sem);
  }
}

/*****************************************************************************/
bool AL_Default_Decoder_HasOngoingFrame(AL_TDecCtx* pCtx)
{
  return pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid;
}

/*****************************************************************************/
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  ResetStartCodes(pCtx);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = 0;
  pCtx->iCurNalStreamOffset = 0;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  AL_Feeder_Reset(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  AL_Default_Decoder_WaitFrameSent(pAbsDec, 0xFFFFFFFF);

  AL_PictMngr_Flush(&pCtx->PictMngr);

  AL_Default_Decoder_FlushInput(pAbsDec);

  // Send eos & get last frames in dpb if any
  AL_sDecoder_CallDisplay(pCtx);

  pCtx->tDecCB.displayCB.func(NULL, NULL, pCtx->tDecCB.displayCB.userParam);

  AL_PictMngr_Terminate(&pCtx->PictMngr);
}

/*****************************************************************************/
static bool CheckDisplayBufferCanBeUsed(AL_TDecCtx* pCtx, AL_TBuffer* pBuf)
{
  if(!pBuf)
    return false;

  int const iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);

  if(iPitchY <= 0)
    return false;

  if(iPitchY % 64 != 0)
    return false;

  AL_TStreamSettings const* pSettings = &pCtx->tStreamSettings;
  bool bEnableDisplayCompression;
  AL_EFbStorageMode const eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, pSettings->iBitDepth, &bEnableDisplayCompression);

  if(iPitchY < (int)AL_Decoder_GetMinPitch(pSettings->tDim.iWidth, pSettings->iBitDepth, eDisplayStorageMode))
    return false;

  AL_TDimension tOutputDim = pSettings->tDim;

  if(iPitchY < (int)AL_Decoder_GetMinPitch(tOutputDim.iWidth, pSettings->iBitDepth, eDisplayStorageMode))
    return false;

  if(pCtx->tStreamSettings.eChroma == AL_CHROMA_4_4_4)
  {
    int const iPitchU = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_U);
    int const iPitchV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_V);

    if((iPitchY != iPitchU) || (iPitchU != iPitchV))
      return false;
  }
  else if(pCtx->tStreamSettings.eChroma != AL_CHROMA_MONO)
  {
    int const iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);

    if(iPitchY != iPitchUV)
      return false;
  }
  return true;
}

/*****************************************************************************/
bool AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  bool bSucceed = CheckDisplayBufferCanBeUsed(pCtx, pDecPict);

  bSucceed = AL_PictMngr_PutDisplayBuffer(&pCtx->PictMngr, pDecPict);

  if(bSucceed)
    AL_Feeder_Signal(pCtx->Feeder);

  return bSucceed;
}

/*****************************************************************************/
int AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return pCtx->tStreamSettings.iBitDepth;
}

/*****************************************************************************/
static int Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int iCurOffset;
  Rtos_GetMutex(pCtx->DecMutex);
  iCurOffset = pCtx->iCurOffset;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return iCurOffset;
}

/*****************************************************************************/
int AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  return Secure_GetStrOffset(pCtx);
}

/*****************************************************************************/
int AL_Default_Decoder_SkipParsedNals(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = pCtx->iCurNalStreamOffset;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return pCtx->iCurNalStreamOffset;
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
AL_ERR AL_Default_Decoder_GetFrameError(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  AL_ERR error;
  bool bUseLastError = (pBuf == NULL) || (!AL_PictMngr_GetFrameEncodingError(&pCtx->PictMngr, pBuf, &error));

  if(bUseLastError)
    error = AL_Default_Decoder_GetLastError(pAbsDec);

  return error;
}

/*****************************************************************************/
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_Assert(!pCtx->bIsBuffersAllocated);

  AL_ERR error = AL_ERR_NO_MEMORY;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tStreamSettings;

  int iSPSMaxSlices = RoundUp(pCtx->pChanParam->iHeight, 16) / 16;
  int iSizeALF = 0;
  int iSizeLmcs = 0;
  int iSizeCQp = 0;

  if(isAVC(pCtx->pChanParam->eCodec))
    iSPSMaxSlices = AL_AVC_GetMaxNumberOfSlices(pStreamSettings->eProfile, pStreamSettings->iLevel,
                                                pCtx->pChanParam->uClkRatio, pCtx->pChanParam->uFrameRate,
                                                GetSquareBlkNumber(pStreamSettings->tDim, 16));

  if(isHEVC(pCtx->pChanParam->eCodec))
    iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(pStreamSettings->iLevel);

  int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int iSizeCompData = 0;

  if(isAVC(pCtx->pChanParam->eCodec))
    iSizeCompData = AL_GetAllocSize_AvcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);

  if(isHEVC(pCtx->pChanParam->eCodec))
    iSizeCompData = AL_GetAllocSize_HevcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);
  int const iSizeCompMap = AL_GetAllocSize_DecCompMap(pStreamSettings->tDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeALF, iSizeLmcs, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap, iSizeCQp))
    goto fail_alloc;

  int iDpbMaxBuf = 0, iMaxBuf = 0, iSizeMV = 0;

  if(isAVC(pCtx->pChanParam->eCodec))
  {
    iDpbMaxBuf = AL_AVC_GetMaxDpbBuffers(pStreamSettings, 0);
    iMaxBuf = AL_AVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
    iSizeMV = AL_GetAllocSize_AvcMV(pStreamSettings->tDim);
  }

  if(isHEVC(pCtx->pChanParam->eCodec))
  {
    iDpbMaxBuf = AL_HEVC_GetMaxDpbBuffers(pStreamSettings);
    iMaxBuf = AL_HEVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
    iSizeMV = AL_GetAllocSize_HevcMV(pStreamSettings->tDim);
  }

  int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  AL_TPictMngrParam tPictMngrParam =
  {
    iDpbMaxBuf, pCtx->eDpbMode, pCtx->pChanParam->eFBStorageMode, pStreamSettings->iBitDepth,
    iMaxBuf, iSizeMV,
    pCtx->pChanParam->bUseEarlyCallback,
    pCtx->tOutputPosition,
  };

  AL_PictMngr_Init(&pCtx->PictMngr, &tPictMngrParam);

  bool bEnableDisplayCompression;
  AL_EFbStorageMode const eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, pStreamSettings->iBitDepth, &bEnableDisplayCompression);

  int iSizeYuv = AL_GetAllocSize_Frame(pStreamSettings->tDim, pStreamSettings->eChroma, pStreamSettings->iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);
  AL_TCropInfo tCropInfo = { false, 0, 0, 0, 0 };

  error = pCtx->tDecCB.resolutionFoundCB.func(iMaxBuf, iSizeYuv, pStreamSettings, &tCropInfo, pCtx->tDecCB.resolutionFoundCB.userParam);

  if(!AL_IS_SUCCESS_CODE(error))
    goto fail_alloc;

  pCtx->bIsBuffersAllocated = true;

  return true;
  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1, false);
  return false;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferHEVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x4A, 0
  }; // simulate a end_of_bitstream
  int iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iSize, &AL_Buffer_Destroy, "eos-buffer");
  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);
  return pEOS;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferAVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x0B, 0
  }; // simulate end_of_stream
  int iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iSize, &AL_Buffer_Destroy, "eos-buffer");
  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);
  return pEOS;
}

/*****************************************************************************/
static bool CheckStreamSettings(AL_TStreamSettings const* pStreamSettings)
{
  if(!IsAtLeastOneStreamSettingsSet(pStreamSettings))
    return true;

  if(!IsAllStreamSettingsSet(pStreamSettings))
    return false;

  if(pStreamSettings->iBitDepth > AL_HWConfig_Dec_GetSupportedBitDepth())
    return false;

  return true;
}

/*****************************************************************************/
static bool CheckDecodeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit != AL_DEC_UNIT_MAX_ENUM;
}

/*****************************************************************************/
static bool CheckAVCSettings(AL_TDecSettings const* pSettings)
{
  AL_Assert(isAVC(pSettings->eCodec));

  if(pSettings->bParallelWPP)
    return false;

  if((pSettings->tStream.eSequenceMode != AL_SM_UNKNOWN) &&
     (pSettings->tStream.eSequenceMode != AL_SM_PROGRESSIVE) &&
     (pSettings->tStream.eSequenceMode != AL_SM_MAX_ENUM))
    return false;

  return true;
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

  int const iStack = pSettings->iStackSize;

  if((iStack < 1) || (iStack > MAX_STACK_SIZE))
    return false;

  if((pSettings->uDDRWidth != 16) && (pSettings->uDDRWidth != 32) && (pSettings->uDDRWidth != 64))
    return false;

  if((pSettings->uFrameRate == 0) && pSettings->bForceFrameRate)
    return false;

  if(!CheckDecodeUnit(pSettings->eDecUnit))
    return false;

  if(isSubframeUnit(pSettings->eDecUnit) && (pSettings->bParallelWPP || (pSettings->eDpbMode != AL_DPB_NO_REORDERING)))
    return false;

  if(isAVC(pSettings->eCodec))
  {
    if(!CheckAVCSettings(pSettings))
      return false;
  }

  if(!CheckRecStorageMode(pSettings))
    return false;

  return true;
}

/*****************************************************************************/
static void AssignSettings(AL_TDecCtx* const pCtx, AL_TDecSettings const* const pSettings)
{
  pCtx->eInputMode = pSettings->eInputMode;
  pCtx->iStackSize = pSettings->iStackSize;
  pCtx->bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->uConcealMaxFps = pSettings->uConcealMaxFps;
  pCtx->eDpbMode = pSettings->eDpbMode;
  pCtx->tStreamSettings = pSettings->tStream;
  pCtx->bUseIFramesAsSyncPoint = pSettings->bUseIFramesAsSyncPoint;

  if(pCtx->tStreamSettings.bDecodeIntraOnly)
    pCtx->bIsIFrame = true;

  AL_TDecChanParam* pChan = pCtx->pChanParam;
  pChan->uMaxLatency = pSettings->iStackSize;
  pChan->uNumCore = pSettings->uNumCore;
  pChan->bNonRealtime = pSettings->bNonRealtime;
  pChan->uClkRatio = pSettings->uClkRatio;
  pChan->uFrameRate = pSettings->uFrameRate == 0 ? pSettings->uClkRatio : pSettings->uFrameRate;
  pChan->bDisableCache = pSettings->bDisableCache;
  pChan->bFrameBufferCompression = pSettings->bFrameBufferCompression;
  pChan->eFBStorageMode = pSettings->eFBStorageMode;
  pChan->uDDRWidth = pSettings->uDDRWidth == 16 ? 0 : pSettings->uDDRWidth == 32 ? 1 : 2;
  pChan->bParallelWPP = pSettings->bParallelWPP;
  pChan->bLowLat = pSettings->bLowLat;
  pChan->bUseEarlyCallback = pSettings->bUseEarlyCallback;
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
static void AssignCallBacks(AL_TDecCtx* const pCtx, AL_TDecCallBacks* pCB)
{
  pCtx->tDecCB = *pCB;
}

/*****************************************************************************/
bool AL_Default_Decoder_AllocPool(AL_TDecCtx* pCtx, int iALFSize, int iLmcsSize, int iWPSize, int iSPSize, int iCompDataSize, int iCompMapSize, int iCQpSize)
{
  (void)iALFSize;
  (void)iLmcsSize;
  (void)iCQpSize;

#define SAFE_POOL_ALLOC(pCtx, pMD, iSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, iSize, name)) \
      return false; \
  } while(0)

  int iPoolSize = pCtx->bStillPictureProfile ? 1 : pCtx->iStackSize;

  AL_ECodec const eCodec = pCtx->pChanParam->eCodec;
  AL_EChromaOrder eChromaOrder = AL_ChromaModeToChromaOrder(pCtx->tStreamSettings.eChroma);
  const uint32_t uRefListSize = AL_GetRefListOffsets(NULL, eCodec, eChromaOrder, sizeof(AL_PADDR));

  // Alloc Decoder buffers
  for(int i = 0; i < iPoolSize; ++i)
  {
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolListRefAddr[i].tMD, uRefListSize, "reflist");
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolSclLst[i].tMD, SCLST_SIZE_DEC, "scllst");
    AL_CleanupMemory(pCtx->PoolSclLst[i].tMD.pVirtualAddr, pCtx->PoolSclLst[i].tMD.uSize);

    Rtos_Memset(&pCtx->PoolPP[i], 0, sizeof(pCtx->PoolPP[0]));
    Rtos_Memset(&pCtx->PoolPB[i], 0, sizeof(pCtx->PoolPB[0]));
    AL_SET_DEC_OPT(&pCtx->PoolPP[i], IntraOnly, 1);
  }

  for(int i = 0; i < iPoolSize; ++i)
  {
    if(!pCtx->bIntraOnlyProfile)
    {
      SAFE_POOL_ALLOC(pCtx, &pCtx->PoolWP[i].tMD, iWPSize, "wp");
      AL_CleanupMemory(pCtx->PoolWP[i].tMD.pVirtualAddr, pCtx->PoolWP[i].tMD.uSize);
    }
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolSP[i].tMD, iSPSize, "sp");
    {
      SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompData[i].tMD, iCompDataSize, "comp data");
      SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompMap[i].tMD, iCompMapSize, "comp map");
    }
  }

  return true;
}

/*****************************************************************************/
bool AL_Default_Decoder_AllocMv(AL_TDecCtx* pCtx, int iMVSize, int iPOCSize, int iNum)
{
#define SAFE_MV_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < iNum; ++i)
  {
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD, iMVSize, "mv");
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD, iPOCSize, "poc");
  }

  return true;
}

/*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError, int iFrameID, bool bTriggerCB)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->error = eError;
  Rtos_ReleaseMutex(pCtx->DecMutex);

  if(iFrameID != -1)
    AL_PictMngr_UpdateDisplayBufferError(&pCtx->PictMngr, iFrameID, eError);

  if(bTriggerCB)
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

/*****************************************************************************/
AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  bool res;

  *hDec = NULL;

  if(!CheckSettings(pSettings))
    return AL_ERR_REQUEST_MALFORMED;

  if(!CheckCallBacks(pCB))
    return AL_ERR_REQUEST_MALFORMED;

  AL_TDecoder* const pDec = (AL_TDecoder*)Rtos_Malloc(sizeof(AL_TDecoder));
  AL_ERR errorCode = AL_ERROR;

  if(!pDec)
    return AL_ERR_NO_MEMORY;

  Rtos_Memset(pDec, 0, sizeof(*pDec));

  AL_TDecCtx* const pCtx = &pDec->ctx;

  pCtx->pScheduler = pScheduler;
  pCtx->hChannel = NULL;
  pCtx->hStartCodeChannel = NULL;
  pCtx->pAllocator = pAllocator;

  InitInternalBuffers(pCtx);
  res = AL_PictMngr_PreInit(&pCtx->PictMngr);

  if(!res)
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

  AssignSettings(pCtx, pSettings);
  AssignCallBacks(pCtx, pCB);

  pCtx->Sem = Rtos_CreateSemaphore(pCtx->iStackSize);
  pCtx->ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->DecMutex = Rtos_CreateMutex();

  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, "Ref", 0, 0, false, false);

  // initialize decoder context
  pCtx->bIntraOnlyProfile = false;
  pCtx->bStillPictureProfile = false;
  pCtx->bFirstIsValid = false;
  pCtx->uNoRaslOutputFlag = 1;
  pCtx->bIsFirstPicture = true;
  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
  pCtx->bIsFirstSPSChecked = false;
  pCtx->bIsBuffersAllocated = false;
  pCtx->uNumSC = 0;
  pCtx->iNumSlicesRemaining = 0;

  AL_Conceal_Init(&pCtx->tConceal);
  // initialize decoder counters
  pCtx->uCurPocLsb = 0xFFFFFFFF;
  pCtx->uToggle = 0;
  pCtx->iNumFrmBlk1 = 0;
  pCtx->iNumFrmBlk2 = 0;
  pCtx->iCurOffset = 0;
  pCtx->iCurNalStreamOffset = 0;
  pCtx->iTraceCounter = 0;
  pCtx->eChanState = CHAN_UNINITIALIZED;

  // initialize tile information
  pCtx->uCurTileID = 0;
  pCtx->bTileSupToSlice = false;

  // initialize slice toggle information
  pCtx->uCurID = 0;

  InitAUP(pCtx);

  bool bIsITU = isITU(pSettings->eCodec);
  bool hasSCD = bIsITU;

  if(hasSCD)
  {
    // Alloc Start Code Detector buffer
    SAFE_ALLOC(pCtx, &pCtx->BufSCD.tMD, SCD_SIZE, "scd");
    AL_CleanupMemory(pCtx->BufSCD.tMD.pVirtualAddr, pCtx->BufSCD.tMD.uSize);

    SAFE_ALLOC(pCtx, &pCtx->SCTable.tMD, pCtx->iStackSize * MAX_NAL_UNIT * sizeof(AL_TNal), "sctable");
    AL_CleanupMemory(pCtx->SCTable.tMD.pVirtualAddr, pCtx->SCTable.tMD.uSize);
  }

  // Alloc Decoder Deanti-emulated buffer for high level syntax parsing
  pCtx->BufNoAE.tMD.pVirtualAddr = Rtos_Malloc(NON_VCL_NAL_SIZE);

  if(!pCtx->BufNoAE.tMD.pVirtualAddr)
    goto cleanup;

  pCtx->BufNoAE.tMD.uSize = NON_VCL_NAL_SIZE;

  if(isAVC(pCtx->pChanParam->eCodec))
    pCtx->eosBuffer = AllocEosBufferAVC(pCtx->eInputMode == AL_DEC_SPLIT_INPUT, pAllocator);

  if(isHEVC(pCtx->pChanParam->eCodec))
    pCtx->eosBuffer = AllocEosBufferHEVC(pCtx->eInputMode == AL_DEC_SPLIT_INPUT, pAllocator);

  AL_Assert(pCtx->eosBuffer);

  if(!pCtx->eosBuffer)
    goto cleanup;

  AL_Buffer_Ref(pCtx->eosBuffer);

  int iInputFifoSize = 256;

  if(pCtx->eInputMode == AL_DEC_SPLIT_INPUT)
  {
    bool bEOSParsingCB = false;
    pCtx->Feeder = AL_SplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pCtx->eosBuffer, bEOSParsingCB);
  }
  else if(pCtx->eInputMode == AL_DEC_UNSPLIT_INPUT)
  {
    int iBufferStreamSize = pSettings->iStreamBufSize;

    if(iBufferStreamSize == 0)
      iBufferStreamSize = GetCircularBufferSize(pCtx->pChanParam->eCodec, pCtx->iStackSize, &pCtx->tStreamSettings);

    bool bForceAccessUnitDestroy = true;

    pCtx->Feeder = AL_UnsplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pAllocator, iBufferStreamSize, pCtx->eosBuffer, bForceAccessUnitDestroy);
  }

  if(!pCtx->Feeder)
    goto cleanup;

  bool useStartCode = true;

  pCtx->tOutputPosition = pSettings->tOutputPosition;

  if(useStartCode)
    AL_IDecScheduler_CreateStartCodeChannel(&pCtx->hStartCodeChannel, pCtx->pScheduler);

  AL_Default_Decoder_SetError(pCtx, AL_SUCCESS, -1, false);

  *hDec = (AL_TDecoder*)pDec;

  return AL_SUCCESS;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pDec);
  return errorCode;
}

/*****************************************************************************/
AL_EFbStorageMode AL_Default_Decoder_GetDisplayStorageMode(AL_TDecCtx* pCtx, int iBitDepth, bool* pEnableCompression)
{
  (void)iBitDepth;
  AL_EFbStorageMode eDisplayStorageMode = pCtx->pChanParam->eFBStorageMode;
  *pEnableCompression = pCtx->pChanParam->bFrameBufferCompression;

  return eDisplayStorageMode;
}

/*@}*/

