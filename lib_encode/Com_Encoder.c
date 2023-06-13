// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Com_Encoder.h"

#include "lib_encode/lib_encoder.h"
#include "lib_encode/LoadLda.h"

#include "lib_common/Utils.h"
#include "lib_common/PixMapBufferInternal.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/StreamSection.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferPictureMeta.h"
#include "lib_common/StreamBuffer.h"
#include "lib_rtos/types.h"
#include "lib_common/BufferLookAheadMeta.h"
#include "lib_common_enc/EncChanParamInternal.h"
#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_common_enc/DPBConstraints.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/QPTableInternal.h"
#include "lib_common_enc/ParamConstraints.h"
#include "lib_encode/Sections.h"

#define DEBUG_PATH "."

#include "lib_assert/al_assert.h"

/***************************************************************************/
static bool shouldUseDynamicLambda(AL_TEncChanParam const* pChParam)
{
  return pChParam->tRCParam.eRCMode == AL_RC_CONST_QP && (((pChParam->tGopParam.eMode & AL_GOP_FLAG_DEFAULT) && pChParam->tGopParam.uGopLength > 1 && pChParam->tGopParam.uNumB == 0) ||
                                                          pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_P);
}

/***************************************************************************/
static AL_ELdaCtrlMode GetFinalLdaMode(const AL_TEncChanParam* pChParam)
{
  if(pChParam->eLdaCtrlMode != AL_AUTO_LDA)
    return pChParam->eLdaCtrlMode;

  if(shouldUseDynamicLambda(pChParam))
    return AL_DYNAMIC_LDA;

  return AL_DEFAULT_LDA;
}

/***************************************************************************/
void AL_Common_Encoder_WaitReadiness(AL_TEncCtx* pCtx)
{
  Rtos_GetSemaphore(pCtx->PendingEncodings, AL_WAIT_FOREVER);
}

/***************************************************************************/
static void RemoveSourceSent(AL_TEncCtx* pCtx, AL_TBuffer const* const pSrc)
{
  Rtos_GetMutex(pCtx->Mutex);

  for(int i = 0; i < AL_MAX_SOURCE_BUFFER; i++)
  {
    if(pCtx->SourceSent[i].pSrc == pSrc)
    {
      pCtx->SourceSent[i].pSrc = NULL;
      pCtx->SourceSent[i].pFI = NULL;
      Rtos_ReleaseMutex(pCtx->Mutex);
      return;
    }
  }

  AL_Assert(0);
  Rtos_ReleaseMutex(pCtx->Mutex);
}

static void releaseSource(AL_TEncCtx* pCtx, AL_TBuffer* pSrc, AL_TFrameInfo* pFI)
{
  RemoveSourceSent(pCtx, pSrc);
  AL_Buffer_Unref(pSrc);

  if(pFI && pFI->pQpTable)
    AL_Buffer_Unref(pFI->pQpTable);

}

/****************************************************************************/
static bool AL_Common_Encoder_InitBuffers(AL_TAllocator* pAllocator, TBufferEP* pBufEP1, TBufferEP* pBufEP4, AL_ECodec eCodec)
{
  (void)pBufEP4;
  bool bRet = MemDesc_AllocNamed(&pBufEP1->tMD, pAllocator, AL_GetAllocSizeEP1(eCodec), "ep1");

  pBufEP1->uFlags = 0;
  return bRet;
}

/****************************************************************************/
static bool init(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TAllocator* pAllocator)
{
  TBufferEP* pEP1 = &pCtx->tLayerCtx[0].tBufEP1;
  TBufferEP* pEP4 = NULL;

  if(!AL_Common_Encoder_InitBuffers(pAllocator, pEP1, pEP4, AL_GET_CODEC(pChParam->eProfile)))
    return false;

  AL_SrcBuffersChecker_Init(&pCtx->tLayerCtx[0].srcBufferChecker, pChParam);

  for(int iLayer = 0; iLayer < MAX_NUM_LAYER; iLayer++)
    pCtx->bEndOfStreamReceived[iLayer] = false;

  pCtx->initialCpbRemovalDelay = pChParam->tRCParam.uInitialRemDelay;
  pCtx->cpbRemovalDelay = 0;

  pCtx->tLayerCtx[0].iCurStreamSent = 0;
  pCtx->tLayerCtx[0].iCurStreamRecv = 0;
  pCtx->iFrameCountDone = 0;

  pCtx->eError = AL_SUCCESS;

  Rtos_Memset(&pCtx->tFrameInfoPool.FrameInfos, 0, sizeof(pCtx->tFrameInfoPool.FrameInfos));
  Rtos_Memset(&pCtx->SourceSent, 0, sizeof(pCtx->SourceSent));

  pCtx->Mutex = Rtos_CreateMutex();
  AL_Assert(pCtx->Mutex);
  return true;
}

/***************************************************************************/
static AL_TEncRequestInfo* getCurrentCommands(AL_TLayerCtx* pCtx)
{
  return &pCtx->currentRequestInfo;
}

/***************************************************************************/
void AL_Common_Encoder_NotifySceneChange(AL_TEncCtx* pCtx, int iAhead)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_SCENE_CHANGE;
  pReqInfo->uSceneChangeDelay = iAhead;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyIsLongTerm(AL_TEncCtx* pCtx)
{
  if(!pCtx->pSettings->tChParam[0].tGopParam.bEnableLT)
    return;
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_IS_LONG_TERM;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyUseLongTerm(AL_TEncCtx* pCtx)
{
  if(!pCtx->pSettings->tChParam[0].tGopParam.bEnableLT)
    return;
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_USE_LONG_TERM;
}

/***************************************************************************/
bool AL_Common_Encoder_PutStreamBuffer(AL_TEncCtx* pCtx, AL_TBuffer* pStream, int iLayerID)
{
  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  AL_Assert(pMetaData);
  AL_Assert(pCtx);

  AL_StreamMetaData_ClearAllSections(pMetaData);
  Rtos_GetMutex(pCtx->Mutex);
  int iOffset = AL_ENC_MAX_HEADER_SIZE;

  size_t iMinSize = AL_ENC_MAX_HEADER_SIZE;

  if(AL_Buffer_GetSize(pStream) < iMinSize)
  {
    Rtos_ReleaseMutex(pCtx->Mutex);
    return false;
  }

  pCtx->tLayerCtx[iLayerID].StreamSent[pCtx->tLayerCtx[iLayerID].iCurStreamSent] = pStream;
  int curStreamSent = pCtx->tLayerCtx[iLayerID].iCurStreamSent;
  pCtx->tLayerCtx[iLayerID].iCurStreamSent = (pCtx->tLayerCtx[iLayerID].iCurStreamSent + 1) % AL_MAX_STREAM_BUFFER;
  Rtos_ReleaseMutex(pCtx->Mutex);
  AL_Buffer_Ref(pStream);

  AL_IEncScheduler_PutStreamBuffer(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pStream, curStreamSent, iOffset);

  return true;
}

/***************************************************************************/
bool AL_Common_Encoder_GetRecPicture(AL_TEncCtx* pCtx, AL_TRecPic* pRecPic, int iLayerID)
{
  AL_Assert(pCtx);

  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  return AL_IEncScheduler_GetRecPicture(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pRecPic);
}

/***************************************************************************/
bool AL_Common_Encoder_ReleaseRecPicture(AL_TEncCtx* pCtx, AL_TRecPic* pRecPic, int iLayerID)
{
  AL_Assert(pCtx);

  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  AL_IEncScheduler_ReleaseRecPicture(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pRecPic);

  for(int i = 0; i < pRecPic->pBuf->iChunkCnt; i++)
    pRecPic->pBuf->hBufs[i] = 0;

  AL_Buffer_Destroy(pRecPic->pBuf);

  return true;
}

/***************************************************************************/
static bool GetNextPoolId(AL_TIDPool* pPool)
{
  /* +1 / -1 business is needed as 0 == NULL is the error value of the fifo
   * this also means that GetNextPoolId returns -1 on error */
  pPool->iCurID = (int)((intptr_t)AL_Fifo_Dequeue(&pPool->tFreeIDs, AL_NO_WAIT)) - 1;
  return pPool->iCurID != INVALID_POOL_ID;
}

static void GiveIdBackToPool(AL_TIDPool* pPool, int iPoolID)
{
  AL_Fifo_Queue(&pPool->tFreeIDs, (void*)((intptr_t)(iPoolID + 1)), AL_NO_WAIT);
}

static void DeinitIDPool(AL_TIDPool* pPool)
{
  pPool->iCurID = INVALID_POOL_ID;
  AL_Fifo_Deinit(&pPool->tFreeIDs);
}

static bool InitIDPool(AL_TIDPool* pPool, int iPoolSize)
{
  pPool->iCurID = INVALID_POOL_ID;

  if(!AL_Fifo_Init(&pPool->tFreeIDs, iPoolSize))
    return false;

  for(int i = 0; i < iPoolSize; ++i)
    GiveIdBackToPool(pPool, i);

  return true;
}

/***************************************************************************/
static AL_TFrameInfo* GetNextFrameInfo(AL_TFrameInfoPool* pFrameInfoPool)
{
  bool const bNextPoolIDAvailable = GetNextPoolId(&pFrameInfoPool->tIDPool);

  if(!bNextPoolIDAvailable)
  {
    AL_Assert(bNextPoolIDAvailable);
    return NULL;
  }
  return &pFrameInfoPool->FrameInfos[pFrameInfoPool->tIDPool.iCurID];
}

/***************************************************************************/
static void IncrCurrentRefCount(AL_THDRPool* pHDRPool)
{
  pHDRPool->uRefCount[pHDRPool->tIDPool.iCurID]++;
}

static void DecrRefCount(AL_THDRPool* pHDRPool, int iID)
{
  AL_Assert(iID != INVALID_POOL_ID);
  pHDRPool->uRefCount[iID]--;

  if(pHDRPool->uRefCount[iID] == 0)
    GiveIdBackToPool(&pHDRPool->tIDPool, iID);
}

static AL_THDRSEIs* GetHDRSEIs(AL_THDRPool* pHDRPool, int iID)
{
  if(iID == INVALID_POOL_ID)
    return NULL;

  return &pHDRPool->HDRSEIs[iID];
}

static AL_THDRSEIs* GetNextHDRSEIs(AL_THDRPool* pHDRPool)
{
  if(pHDRPool->tIDPool.iCurID != INVALID_POOL_ID)
    DecrRefCount(pHDRPool, pHDRPool->tIDPool.iCurID);

  bool const bNextPoolIDAvailable = GetNextPoolId(&pHDRPool->tIDPool);

  if(!bNextPoolIDAvailable)
  {
    AL_Assert(bNextPoolIDAvailable);
    return NULL;
  }
  pHDRPool->uRefCount[pHDRPool->tIDPool.iCurID] = 1;
  return GetHDRSEIs(pHDRPool, pHDRPool->tIDPool.iCurID);
}

/***************************************************************************/
static bool EndOfStream(AL_TEncCtx* pCtx, int iLayerID)
{
  return AL_IEncScheduler_EncodeOneFrame(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, NULL, NULL, NULL);
}

/***************************************************************************/
static void AddSourceSent(AL_TEncCtx* pCtx, AL_TBuffer* pSrc, AL_TFrameInfo* pFI)
{
  Rtos_GetMutex(pCtx->Mutex);

  for(int i = 0; i < AL_MAX_SOURCE_BUFFER; i++)
  {
    if(pCtx->SourceSent[i].pSrc == NULL)
    {
      pCtx->SourceSent[i].pSrc = pSrc;
      pCtx->SourceSent[i].pFI = pFI;
      Rtos_ReleaseMutex(pCtx->Mutex);
      return;
    }
  }

  AL_Assert(0);
  Rtos_ReleaseMutex(pCtx->Mutex);
}

/****************************************************************************/
void AL_Common_Encoder_SetEncodingOptions(AL_TEncCtx* pCtx, AL_TFrameInfo* pFI, int iLayerID)
{
  (void)iLayerID;
  AL_TEncInfo* pEncInfo = &pFI->tEncInfo;

  if(pCtx->pSettings->bForceLoad)
    pEncInfo->eEncOptions |= AL_OPT_FORCE_LOAD;

  if(pCtx->pSettings->bDisIntra)
    pEncInfo->eEncOptions |= AL_OPT_DISABLE_INTRA;

  if(pCtx->pSettings->bDependentSlice)
    pEncInfo->eEncOptions |= AL_OPT_DEPENDENT_SLICES;

  if(pCtx->pSettings->tChParam[iLayerID].uL2PrefetchMemSize)
    pEncInfo->eEncOptions |= AL_OPT_USE_L2;

}

/****************************************************************************/
void AL_Common_Encoder_ProcessLookAheadParam(AL_TEncCtx* pCtx, AL_TEncInfo* pEI, AL_TBuffer* pFrame)
{
  (void)pEI;
  // Process first pass informations from the metadata, notifies scene changes and transmits parameters for the RateCtrl
  AL_TLookAheadMetaData* pMetaDataLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_LOOKAHEAD);

  if(pMetaDataLA && pMetaDataLA->eSceneChange == AL_SC_NEXT)
    AL_Common_Encoder_NotifySceneChange(pCtx, 1);
  else if(pMetaDataLA && pMetaDataLA->eSceneChange == AL_SC_CURRENT)
    AL_Common_Encoder_NotifySceneChange(pCtx, 0);

  if(pMetaDataLA && pMetaDataLA->iPictureSize != -1)
  {
    pEI->tLAParam.iSCPictureSize = pMetaDataLA->iPictureSize;
    pEI->tLAParam.iSCIPRatio = pMetaDataLA->iIPRatio;
    pEI->tLAParam.iComplexity = pMetaDataLA->iComplexity;
    pEI->tLAParam.iTargetLevel = pMetaDataLA->iTargetLevel;
  }
}

/***************************************************************************/
static uint8_t UpdateEncoderInfos(AL_TEncCtx* pCtx, AL_TEncInfo* pEI, AL_TEncChanParam* pChParam, int iLayerID)
{
  uint8_t uSpsId = 0;

  if(!AL_IS_AOM(pChParam->eProfile))
  {
    Rtos_GetMutex(pCtx->Mutex);
    uSpsId = AL_GetSps(&pCtx->tHeadersCtx[iLayerID], pChParam->uEncWidth, pChParam->uEncHeight);
    pEI->uPpsId = AL_GetPps(&pCtx->tHeadersCtx[iLayerID], uSpsId, pChParam->iCbPicQpOffset, pChParam->iCrPicQpOffset);
    Rtos_ReleaseMutex(pCtx->Mutex);
  }

  if(!AL_IS_AOM(pChParam->eProfile))
  {
    pEI->iQp1Offset = pChParam->iCbPicQpOffset;
    pEI->iQp2Offset = pChParam->iCrPicQpOffset;
  }

  return uSpsId;
}

/***************************************************************************/
static void SetHLSInfos(AL_TEncCtx* pCtx, AL_TEncRequestInfo* pReqInfo, AL_TFrameInfo* pFI, AL_TEncChanParam* pChParam, uint8_t uSpsId)
{
  (void)pCtx;
  (void)pReqInfo;
  (void)pChParam;

  pFI->tHLSUpdateInfo = (const AL_HLSInfo) {
    0
  };

  pFI->tHLSUpdateInfo.uSpsId = uSpsId;
  pFI->tHLSUpdateInfo.uPpsId = pFI->tEncInfo.uPpsId;

  pFI->tHLSUpdateInfo.iCbPicQpOffset = pChParam->iCbPicQpOffset;
  pFI->tHLSUpdateInfo.iCrPicQpOffset = pChParam->iCrPicQpOffset;

  if(pReqInfo->eReqOptions & AL_OPT_SET_LF_OFFSETS)
  {
    pFI->tHLSUpdateInfo.bLFOffsetChanged = true;
    pFI->tHLSUpdateInfo.iLFBetaOffset = pReqInfo->smartParams.iLFBetaOffset;
    pFI->tHLSUpdateInfo.iLFTcOffset = pReqInfo->smartParams.iLFTcOffset;
  }

  if(pReqInfo->eReqOptions & AL_OPT_UPDATE_RC_GOP_PARAMS)
  {
    pFI->tHLSUpdateInfo.uFrameRate = pReqInfo->smartParams.rc.uFrameRate;
    pFI->tHLSUpdateInfo.uClkRatio = pReqInfo->smartParams.rc.uClkRatio;
  }

  pFI->tHLSUpdateInfo.bHDRChanged = pCtx->tHDRPool.bHDRChanged;
  pFI->tHLSUpdateInfo.iHDRID = pCtx->tHDRPool.tIDPool.iCurID;

  if(pFI->tHLSUpdateInfo.iHDRID != INVALID_POOL_ID)
  {
    Rtos_GetMutex(pCtx->Mutex);
    IncrCurrentRefCount(&pCtx->tHDRPool);
    Rtos_ReleaseMutex(pCtx->Mutex);
    pCtx->tHDRPool.bHDRChanged = false;
  }
}

/***************************************************************************/
AL_ERR AL_Common_Encoder_GetLastError(AL_TEncCtx* pCtx)
{
  Rtos_GetMutex(pCtx->Mutex);
  AL_ERR eError = pCtx->eError;
  Rtos_ReleaseMutex(pCtx->Mutex);

  return eError;
}

/***************************************************************************/
void AL_Common_SetError(AL_TEncCtx* pCtx, AL_ERR eErrorCode)
{
  Rtos_GetMutex(pCtx->Mutex);
  pCtx->eError = eErrorCode;
  Rtos_ReleaseMutex(pCtx->Mutex);
}

/***************************************************************************/
bool AL_Common_Encoder_Process(AL_TEncCtx* pCtx, AL_TBuffer* pFrame, AL_TBuffer* pQpTable, int iLayerID)
{
  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  if(pCtx->bEndOfStreamReceived[iLayerID])
    return false;

  if(!pFrame)
  {
    bool bRes = true;

    for(int i = 0; i <= iLayerID; ++i)
    {
      if(pCtx->bEndOfStreamReceived[i] == false)
      {
        pCtx->bEndOfStreamReceived[i] = true;
        bRes &= EndOfStream(pCtx, i);
      }
    }

    return bRes;
  }

  if(!AL_SrcBuffersChecker_CanBeUsed(&pCtx->tLayerCtx[iLayerID].srcBufferChecker, pFrame))
    return false;

  if(pQpTable != NULL && pCtx->pSettings->bDiagnostic)
  {
    AL_TDimension tEncDim = { pCtx->pSettings->tChParam[iLayerID].uEncWidth, pCtx->pSettings->tChParam[iLayerID].uEncHeight };
    AL_ECodec eCodec = AL_GET_CODEC(pCtx->pSettings->tChParam[iLayerID].eProfile);

    int iQPTableDepth = 0;
    uint32_t uExpectedSize = AL_GetAllocSizeEP2(tEncDim, eCodec, pCtx->pSettings->tChParam[iLayerID].uLog2MaxCuSize);
    uint32_t uRealSize = AL_Buffer_GetSize(pQpTable);

    if(uRealSize < uExpectedSize)
    {
      AL_Common_SetError(pCtx, AL_ERR_QPLOAD_NOT_ENOUGH_DATA);
      return false;
    }

    uint8_t* pQP = AL_Buffer_GetData(pQpTable) + EP2_BUF_SEG_CTRL.Offset;
    AL_ERR eErr = AL_QPTable_CheckValidity(pQP, tEncDim, AL_GET_CODEC(pCtx->pSettings->tChParam[iLayerID].eProfile), iQPTableDepth,
                                           pCtx->pSettings->tChParam[iLayerID].uLog2MaxCuSize, pCtx->pSettings->bDisIntra, pCtx->pSettings->tChParam[iLayerID].eEncOptions & AL_OPT_QP_TAB_RELATIVE);

    if(eErr != AL_SUCCESS)
    {
      AL_Common_SetError(pCtx, eErr);
      return false;
    }
  }

  AL_Common_Encoder_WaitReadiness(pCtx);

  const int AL_DEFAULT_PPS_QP_26 = 26;
  AL_TFrameInfo* pFI = GetNextFrameInfo(&pCtx->tFrameInfoPool);
  AL_TEncInfo* pEI = &pFI->tEncInfo;
  AL_TEncPicBufAddrs addresses = { 0 };

  pEI->UserParam = pCtx->tFrameInfoPool.tIDPool.iCurID;
  pEI->iPpsQP = AL_DEFAULT_PPS_QP_26;

  AL_Common_Encoder_SetEncodingOptions(pCtx, pFI, iLayerID);

  pFI->pQpTable = pQpTable;

  if(pQpTable)
  {
    AL_Buffer_Ref(pQpTable);
    addresses.pEP2 = AL_Buffer_GetPhysicalAddress(pQpTable);
    addresses.pEP2_v = (AL_PTR64)(uintptr_t)AL_Buffer_GetData(pQpTable);
    pEI->eEncOptions |= AL_OPT_USE_QP_TABLE;

    Rtos_FlushCacheMemory(AL_Buffer_GetData(pQpTable), AL_Buffer_GetSize(pQpTable));
  }
  else
  {
    addresses.pEP2_v = 0;
    addresses.pEP2 = 0;
  }

  AL_TEncChanParam* pChParam = &pCtx->pSettings->tChParam[iLayerID];
  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(AL_PixMapBuffer_GetFourCC(pFrame), &tPicFormat);

  if(!bSuccess)
  {
    AL_Assert(bSuccess);
    return false;
  }

  Rtos_FlushCacheMemory(AL_Buffer_GetData(pFrame), AL_Buffer_GetSize(pFrame));

  addresses.tSrcAddrs.pY = AL_PixMapBuffer_GetPlanePhysicalAddress(pFrame, AL_PLANE_Y);
  addresses.tSrcAddrs.pC1 = 0;

  if(tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    addresses.tSrcAddrs.pC1 = AL_PixMapBuffer_GetPlanePhysicalAddress(pFrame, AL_PLANE_UV);

  addresses.tSrcInfo.uPitch = AL_PixMapBuffer_GetPlanePitch(pFrame, AL_PLANE_Y);

  if(pChParam->bEnableSrcCrop)
  {
    int iPosX = pChParam->uSrcCropPosX;
    int iPosY = pChParam->uSrcCropPosY;

    if(AL_GET_BITDEPTH(pChParam->ePicFormat) > 8)
    {
      iPosX = (iPosX * 4) / 3;
      iPosY *= 2;
    }

    addresses.tSrcAddrs.pY += iPosY * AL_PixMapBuffer_GetPlanePitch(pFrame, AL_PLANE_Y) + iPosX;

    if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == AL_CHROMA_4_2_0)
      iPosY /= 2;

    if(tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
      addresses.tSrcAddrs.pC1 += iPosY * AL_PixMapBuffer_GetPlanePitch(pFrame, AL_PLANE_UV) + iPosX;
  }

  addresses.tSrcInfo.uBitDepth = tPicFormat.uBitDepth;
  AL_ESrcMode srcMode = pChParam->eSrcMode;

  addresses.tSrcInfo.uFormat = AL_GET_SRC_FMT(srcMode);

  AL_Buffer_Ref(pFrame);
  pEI->SrcHandle = (AL_64U)(uintptr_t)pFrame;
  AddSourceSent(pCtx, pFrame, pFI);

  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[iLayerID]);

  if(pCtx->pSettings->LookAhead > 0 || pCtx->pSettings->TwoPass == 2)
    AL_Common_Encoder_ProcessLookAheadParam(pCtx, pEI, pFrame);

  uint8_t uSpsId;
  uSpsId = UpdateEncoderInfos(pCtx, pEI, pChParam, iLayerID);
  SetHLSInfos(pCtx, pReqInfo, pFI, pChParam, uSpsId);

  bool bRet = AL_IEncScheduler_EncodeOneFrame(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pEI, pReqInfo, &addresses);

  if(!bRet)
    releaseSource(pCtx, pFrame, pFI);

  Rtos_Memset(pReqInfo, 0, sizeof(*pReqInfo));
  Rtos_Memset(pEI, 0, sizeof(*pEI));
  return bRet;
}

static void setMaxNumRef(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  if(AL_IS_AVC(pChParam->eProfile))
    pCtx->iMaxNumRef = AL_IS_INTRA_PROFILE(pChParam->eProfile) ? 0 : AL_DPBConstraint_GetMaxDPBSize(pChParam);
  else
    pCtx->iMaxNumRef = AL_GetNumberOfRef(pChParam->uPpsParam);
}

void AL_Common_Encoder_SetHlsParam(AL_TEncChanParam* pChParam)
{
  if(pChParam->uCabacInitIdc)
    pChParam->uPpsParam |= AL_PPS_CABAC_INIT_PRES_FLAG;
}

#if AL_ENABLE_ENC_WATCHDOG

/***************************************************************************/
void AL_Common_Encoder_SetWatchdogCB(AL_TEncCtx* pCtx, const AL_TEncSettings* pSettings)
{
  (void)pCtx;
  (void)pSettings;
}

#endif

/***************************************************************************/
uint8_t AL_Common_Encoder_GetInitialQP(uint32_t iBitPerPixel, AL_EProfile eProfile)
{
  int iTableIdx = AL_IS_AVC(eProfile) ? 0 : 1;
  uint8_t InitQP = AL_BitPerPixelQP[iTableIdx][MAX_IDX_BIT_PER_PEL][1];

  for(int i = 0; i < MAX_IDX_BIT_PER_PEL; i++)
  {
    if(iBitPerPixel <= (uint32_t)AL_BitPerPixelQP[iTableIdx][i][0])
    {
      InitQP = AL_BitPerPixelQP[iTableIdx][i][1];
      break;
    }
  }

  return InitQP;
}

bool AL_Common_Encoder_IsInitialQpProvided(AL_TEncChanParam* pChParam)
{
  return pChParam->tRCParam.eRCMode == AL_RC_CONST_QP || pChParam->tRCParam.iInitialQP >= 0;
}

static uint32_t ComputeBitPerPixel(AL_TEncChanParam* pChParam)
{
  return pChParam->tRCParam.uTargetBitRate / pChParam->tRCParam.uFrameRate * 1000 / (pChParam->uEncWidth * pChParam->uEncHeight);
}

static void SetGoldenRefFrequency(AL_TEncChanParam* pChParam)
{
  if(!pChParam->tRCParam.bUseGoldenRef)
  {
    pChParam->tGopParam.uFreqGoldenRef = 0;
    return;
  }

  if(pChParam->eLdaCtrlMode != AL_DYNAMIC_LDA)
  {
    pChParam->tGopParam.uFreqGoldenRef = 0;
    pChParam->tRCParam.bUseGoldenRef = false;
    return;
  }

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_DEFAULT)
  {
    if(pChParam->tGopParam.uNumB)
    {
      pChParam->tGopParam.uFreqGoldenRef = 0;
      pChParam->tRCParam.bUseGoldenRef = false;
      return;
    }

    if(pChParam->tRCParam.uGoldenRefFrequency < -1)
      pChParam->tGopParam.uFreqGoldenRef = 4;
    else
      pChParam->tGopParam.uFreqGoldenRef = pChParam->tRCParam.uGoldenRefFrequency;
  }
  else if(pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_P)
  {
    if(pChParam->tRCParam.uGoldenRefFrequency < -1)
      pChParam->tGopParam.uFreqGoldenRef = 5;
    else
      pChParam->tGopParam.uFreqGoldenRef = pChParam->tRCParam.uGoldenRefFrequency;
  }
}

/****************************************************************************/
static AL_TEncChanParam* TransferChannelParameters(AL_TEncSettings const* pSettings, AL_TEncChanParam* pChParamOut)
{

  pChParamOut->uClipHrzRange = (pChParamOut->eEncOptions & AL_OPT_FORCE_MV_CLIP) ? pSettings->uClipHrzRange : 0;
  pChParamOut->uClipVrtRange = (pChParamOut->eEncOptions & AL_OPT_FORCE_MV_CLIP) ? pSettings->uClipVrtRange : 0;

  pChParamOut->uL2PrefetchMemSize = pSettings->iPrefetchLevel2;
  pChParamOut->uL2PrefetchMemOffset = 0;

  if(AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode))
  {
    pChParamOut->eEncOptions |= AL_OPT_ENABLE_AUTO_QP;

    if(pSettings->eQpCtrlMode == AL_QP_CTRL_ADAPTIVE_AUTO)
      pChParamOut->eEncOptions |= AL_OPT_ADAPT_AUTO_QP;
  }

  if(pSettings->eQpTableMode == AL_QP_TABLE_RELATIVE)
    pChParamOut->eEncOptions |= AL_OPT_QP_TAB_RELATIVE;

  pChParamOut->eLdaCtrlMode = GetFinalLdaMode(pChParamOut);
  pChParamOut->eEncOptions |= AL_OPT_CUSTOM_LDA;

  SetGoldenRefFrequency(pChParamOut);
  return pChParamOut;
}

/****************************************************************************/
static AL_TEncChanParam* initChannelParam(AL_TEncCtx* pCtx, AL_TEncSettings const* pSettings)
{
  // Keep Settings and source video information -----------------------
  *(pCtx->pSettings) = *pSettings;
  return TransferChannelParameters(pSettings, &pCtx->pSettings->tChParam[0]);
}

/****************************************************************************/
void AL_Common_Encoder_SetME(int iHrzRange_P, int iVrtRange_P, int iHrzRange_B, int iVrtRange_B, AL_TEncChanParam* pChParam)
{
  if(pChParam->pMeRange[AL_SLICE_P][0] < 0)
    pChParam->pMeRange[AL_SLICE_P][0] = iHrzRange_P;

  if(pChParam->pMeRange[AL_SLICE_P][1] < 0)
    pChParam->pMeRange[AL_SLICE_P][1] = iVrtRange_P;

  if(pChParam->pMeRange[AL_SLICE_B][0] < 0)
    pChParam->pMeRange[AL_SLICE_B][0] = iHrzRange_B;

  if(pChParam->pMeRange[AL_SLICE_B][1] < 0)
    pChParam->pMeRange[AL_SLICE_B][1] = iVrtRange_B;
}

static void DeinitBuffers(AL_TLayerCtx* pCtx)
{
  MemDesc_Free(&pCtx->tBufEP1.tMD);
}

/****************************************************************************/
void AL_Common_Encoder_ComputeRCParam(int iCbOffset, int iCrOffset, int iIntraOnlyOff, AL_TEncChanParam* pChParam)
{
  // Calculate Initial QP if not provided ----------------------------------
  if(!AL_Common_Encoder_IsInitialQpProvided(pChParam))
  {
    uint32_t iBitPerPixel = ComputeBitPerPixel(pChParam);
    int iInitQP = AL_Common_Encoder_GetInitialQP(iBitPerPixel, pChParam->eProfile);

    if(pChParam->tGopParam.uGopLength <= 1)
      iInitQP += iIntraOnlyOff;
    else if(pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
      iInitQP -= 6;
    pChParam->tRCParam.iInitialQP = iInitQP;
  }

  if(pChParam->tRCParam.eRCMode != AL_RC_CONST_QP // && pChParam->tRCParam.iMinQP < 10
     )
  {
    for(int i = 0; i < ARRAY_SIZE(pChParam->tRCParam.iMinQP); i++)
    {
      if(pChParam->tRCParam.iMinQP[i] < 10)
        pChParam->tRCParam.iMinQP[i] = 10;
    }
  }

  for(int i = 0; i < ARRAY_SIZE(pChParam->tRCParam.iMaxQP); i++)
  {
    if(pChParam->tRCParam.iMaxQP[i] < pChParam->tRCParam.iMinQP[i])
      pChParam->tRCParam.iMaxQP[i] = pChParam->tRCParam.iMinQP[i];
  }

  int iMinQP = 0;
  int iMaxQP = 51;

  for(int i = 0; i < ARRAY_SIZE(pChParam->tRCParam.iMaxQP); i++)
  {
    pChParam->tRCParam.iMinQP[i] = Max(pChParam->tRCParam.iMinQP[i], (iMinQP - Min(iCbOffset, iCrOffset)));
    pChParam->tRCParam.iMaxQP[i] = Min(pChParam->tRCParam.iMaxQP[i], (iMaxQP - Max(iCbOffset, iCrOffset)));
  }

  pChParam->tRCParam.iInitialQP = Clip3(pChParam->tRCParam.iInitialQP, pChParam->tRCParam.iMinQP[AL_SLICE_I], pChParam->tRCParam.iMaxQP[AL_SLICE_I]);

  bool bIsCappedVBR = (pChParam->tRCParam.eRCMode == AL_RC_CAPPED_VBR);
  bool bIsCappedQuality = false;

  if(bIsCappedVBR || bIsCappedQuality)
  {
    pChParam->tRCParam.uMaxPelVal = AL_GET_BITDEPTH(pChParam->ePicFormat) == 8 ? 255 : 1023;
    pChParam->tRCParam.uNumPel = pChParam->uEncWidth * pChParam->uEncHeight;
  }
}

/****************************************************************************/
static bool PreprocessEncoderParam(AL_TEncCtx* pCtx, TBufferEP* pEP1, TBufferEP* pEP4, int iLayerID)
{
  (void)pEP4;
  AL_TEncSettings* pSettings = pCtx->pSettings;
  AL_TEncChanParam* pChParam = &pSettings->tChParam[iLayerID];

  pEP1->uFlags = 0;
  AL_CleanupMemory(pEP1->tMD.pVirtualAddr, pEP1->tMD.uSize);

  AL_ELdaCtrlMode* eLdaMode = &pChParam->eLdaCtrlMode;

  if(*eLdaMode == AL_LOAD_LDA)
  {
    char const* ldaFilename = DEBUG_PATH "/Lambdas.hex";

    if(!LoadLambdaFromFile(ldaFilename, pEP1))
      return false;
  }
  /* deprecation warning: custom lda are used for hw tests and will be passed
   * via a file in the future.*/
  else if(*eLdaMode == AL_CUSTOM_LDA)
  {
    LoadCustomLda(pEP1);
    *eLdaMode = AL_LOAD_LDA;
  }

  pCtx->encoder.preprocessEp1(pCtx, pEP1);
  return true;
}

static void releaseStreams(AL_TEncCtx* pCtx, int iLayerID)
{
  for(int streamId = pCtx->tLayerCtx[iLayerID].iCurStreamRecv; streamId != pCtx->tLayerCtx[iLayerID].iCurStreamSent; streamId = (streamId + 1) % AL_MAX_STREAM_BUFFER)
  {
    AL_TBuffer* pStream = pCtx->tLayerCtx[iLayerID].StreamSent[streamId];
    pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.func(pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.userParam, pStream, NULL, iLayerID);
    AL_Buffer_Unref(pStream);
  }
}

static void releaseSources(AL_TEncCtx* pCtx, int iLayerID)
{
  for(int sourceId = 0; sourceId < AL_MAX_SOURCE_BUFFER; sourceId++)
  {
    AL_TFrameCtx* pFrameCtx = &pCtx->SourceSent[sourceId];
    AL_TBuffer* pSource = pFrameCtx->pSrc;

    if(pSource != NULL)
    {
      pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.func(pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.userParam, NULL, pSource, iLayerID);
      releaseSource(pCtx, pSource, pFrameCtx->pFI);
    }
  }
}

static void FillSettingsPointers(AL_TEncCtx* pCtx)
{
  pCtx->pSettings = (AL_TEncSettings*)pCtx->tMDSettings.pVirtualAddr;

  for(int i = 0; i < MAX_NUM_LAYER; i++)
  {
    uint32_t uOffset = ((AL_VADDR)&pCtx->pSettings->tChParam[i]) - ((AL_VADDR)pCtx->pSettings);
    pCtx->tLayerCtx[i].tMDChParam.pAllocator = pCtx->tMDSettings.pAllocator;
    pCtx->tLayerCtx[i].tMDChParam.pVirtualAddr = pCtx->tMDSettings.pVirtualAddr + uOffset;
    pCtx->tLayerCtx[i].tMDChParam.uPhysicalAddr = pCtx->tMDSettings.uPhysicalAddr + uOffset;
    pCtx->tLayerCtx[i].tMDChParam.uSize = sizeof(AL_TEncSettings);
  }
}

static void ResetSettings(AL_TEncCtx* pCtx)
{
  Rtos_Memset(pCtx->tMDSettings.pVirtualAddr, 0, pCtx->tMDSettings.uSize);
}

static void SetupHeaders(AL_TEncCtx* pCtx)
{
  int i;

  for(i = 0; i < MAX_NUM_LAYER; i++)
  {
    pCtx->tHeadersCtx[i].iPrevSps = (MAX_SPS_IDS - 1 + i) % MAX_SPS_IDS;
    pCtx->tHeadersCtx[i].iPrevPps = (MAX_PPS_IDS - 1 + i) % MAX_PPS_IDS;
    pCtx->tHeadersCtx[i].ppsCtx[(MAX_PPS_IDS - 1 + i) % MAX_PPS_IDS].uSpsId = ~0;
  }
}

/***************************************************************************/
AL_TEncCtx* AL_Common_Encoder_Create(AL_TAllocator* pAlloc)
{
  AL_TEncCtx* pCtx = Rtos_Malloc(sizeof(AL_TEncCtx));

  if(!pCtx)
    goto fail;

  Rtos_Memset(pCtx, 0, sizeof(*pCtx));

  if(!MemDesc_Alloc(&pCtx->tMDSettings, pAlloc, sizeof(*pCtx->pSettings)))
    goto fail;

  FillSettingsPointers(pCtx);
  ResetSettings(pCtx);
  SetupHeaders(pCtx);

  return pCtx;

  fail:
  AL_Common_Encoder_Destroy(pCtx);
  return NULL;
}

/***************************************************************************/
static void destroyChannels(AL_TEncCtx* pCtx)
{
  if(!pCtx->pSettings || pCtx->pSettings->NumLayer == 0)
    return;

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_IEncScheduler_DestroyChannel(pCtx->pScheduler, pCtx->tLayerCtx[i].hChannel);
    releaseStreams(pCtx, i);
    releaseSources(pCtx, i);
  }

  Rtos_DeleteMutex(pCtx->Mutex);
  Rtos_DeleteSemaphore(pCtx->PendingEncodings);

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
    DeinitBuffers(&pCtx->tLayerCtx[i]);

  DeinitIDPool(&pCtx->tFrameInfoPool.tIDPool);
  DeinitIDPool(&pCtx->tHDRPool.tIDPool);

  ResetSettings(pCtx);
}

/***************************************************************************/
void AL_Common_Encoder_Destroy(AL_TEncCtx* pCtx)
{
  if(!pCtx)
    return;

  destroyChannels(pCtx);
  MemDesc_Free(&pCtx->tMDSettings);
  Rtos_Free(pCtx);
}

/***************************************************************************/
bool AL_Common_Encoder_GetInfo(AL_TEncCtx* pCtx, AL_TEncoderInfo* pEncInfo)
{
  pEncInfo->uNumCore = pCtx->pSettings->tChParam[pCtx->pSettings->NumLayer - 1].uNumCore;
  return true;
}

#define AL_RETURN_ERROR(e) { AL_Common_SetError(pCtx, e); return false; }
#define AL_CRIT_SECTION_RETURN_ERROR(mutex, e) { Rtos_ReleaseMutex(mutex); \
                                                 AL_Common_SetError(pCtx, e); \
                                                 return false; }
/***************************************************************************/
bool AL_Common_Encoder_SetCostMode(AL_TEncCtx* pCtx, bool costMode)
{

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_UPDATE_COST_MODE;

    if(costMode)
      pCtx->pSettings->tChParam[i].eEncOptions |= AL_OPT_RDO_COST_MODE;
    else
      pCtx->pSettings->tChParam[i].eEncOptions &= ~AL_OPT_RDO_COST_MODE;

    pReqInfo->smartParams.costMode = costMode;
  }

  return true;
}

/****************************************************************************/
static void setRcGopParams(AL_TEncCtx* pCtx, int iLayerID)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[iLayerID]);
  pReqInfo->eReqOptions |= AL_OPT_UPDATE_RC_GOP_PARAMS;

  pReqInfo->smartParams.rc = pCtx->pSettings->tChParam[iLayerID].tRCParam;
  pReqInfo->smartParams.gop = pCtx->pSettings->tChParam[iLayerID].tGopParam;
}

/****************************************************************************/
bool AL_Common_Encoder_SetMaxPictureSize(AL_TEncCtx* pCtx, uint32_t uMaxPictureSize, AL_ESliceType sliceType)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    // MaxPictureSize can't be enable or disable dynamically. It needs to be enable in cfg file, to change the value
    if(pCtx->pSettings->tChParam[i].tRCParam.pMaxPictureSize[sliceType] == 0 || uMaxPictureSize == 0)
      return false;

    pCtx->pSettings->tChParam[i].tRCParam.pMaxPictureSize[sliceType] = uMaxPictureSize;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
static bool IsGopRestartForbidden(AL_TEncChanParam* pChParam)
{
  bool isAdaptive = (pChParam->tGopParam.eMode == AL_GOP_MODE_ADAPTIVE);
  bool isBypass = (pChParam->tGopParam.eMode == AL_GOP_MODE_BYPASS);
  return isAdaptive || isBypass;
}

/****************************************************************************/
bool AL_Common_Encoder_RestartGop(AL_TEncCtx* pCtx)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    if(IsGopRestartForbidden(&pCtx->pSettings->tChParam[i]))
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_RESTART_GOP;
  }

  return true;
}

/****************************************************************************/
static bool IsGopRestartRecoveryPointForbidden(AL_TEncChanParam* pChParam)
{
  return !(pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY);
}

/****************************************************************************/
bool AL_Common_Encoder_RestartGopRecoveryPoint(AL_TEncCtx* pCtx)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    if(IsGopRestartRecoveryPointForbidden(&pCtx->pSettings->tChParam[i]))
    {
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);
    }

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_RECOVERY_POINT;
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetGopLength(AL_TEncCtx* pCtx, int iGopLength)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    bool bCmdValid = (pCtx->pSettings->tChParam[i].tGopParam.eMode & AL_GOP_FLAG_DEFAULT) != 0;

    if(!bCmdValid)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    pCtx->pSettings->tChParam[i].tGopParam.uGopLength = iGopLength;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetGopNumB(AL_TEncCtx* pCtx, int iNumB)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    if((pCtx->pSettings->tChParam[i].tGopParam.eMode & AL_GOP_FLAG_DEFAULT) == 0)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    if(iNumB > pCtx->iInitialNumB)
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

    pCtx->pSettings->tChParam[i].tGopParam.uNumB = iNumB;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetFreqIDR(AL_TEncCtx* pCtx, int iFreqIDR)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    bool bCmdValid = (pCtx->pSettings->tChParam[i].tGopParam.eMode & AL_GOP_FLAG_DEFAULT) != 0;

    if(!bCmdValid)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    if(iFreqIDR < -1)
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

    pCtx->pSettings->tChParam[i].tGopParam.uFreqIDR = iFreqIDR;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetBitRate(AL_TEncCtx* pCtx, int iBitRate, int iLayerID)
{
  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  pCtx->pSettings->tChParam[iLayerID].tRCParam.uTargetBitRate = iBitRate;

  if(AL_IS_CBR(pCtx->pSettings->tChParam[iLayerID].tRCParam.eRCMode))
    pCtx->pSettings->tChParam[iLayerID].tRCParam.uMaxBitRate = iBitRate;

  setRcGopParams(pCtx, iLayerID);

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetMaxBitRate(AL_TEncCtx* pCtx, int iTargetBitRate, int iMaxBitRate, int iLayerID)
{
  if(iLayerID >= MAX_NUM_LAYER)
    return false;

  pCtx->pSettings->tChParam[iLayerID].tRCParam.uTargetBitRate = iTargetBitRate;

  if(iTargetBitRate <= iMaxBitRate)
  {
    if(AL_IS_CBR(pCtx->pSettings->tChParam[iLayerID].tRCParam.eRCMode))
      pCtx->pSettings->tChParam[iLayerID].tRCParam.uMaxBitRate = iTargetBitRate;
    else
      pCtx->pSettings->tChParam[iLayerID].tRCParam.uMaxBitRate = iMaxBitRate;

    setRcGopParams(pCtx, iLayerID);
  }
  else
  {
    AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetFrameRate(AL_TEncCtx* pCtx, uint16_t uFrameRate, uint16_t uClkRatio)
{
  if(uFrameRate > pCtx->uInitialFrameRate)
    AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

  Rtos_GetMutex(pCtx->Mutex);

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    pCtx->pSettings->tChParam[i].tRCParam.uFrameRate = uFrameRate;
    pCtx->pSettings->tChParam[i].tRCParam.uClkRatio = uClkRatio;
  }

  Rtos_ReleaseMutex(pCtx->Mutex);

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
    setRcGopParams(pCtx, i);

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQP(AL_TEncCtx* pCtx, int16_t iQP)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_SET_QP;
    pReqInfo->smartParams.iQPSet = iQP;
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQPOffset(AL_TEncCtx* pCtx, int16_t iQPOffset)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_SET_QP_OFFSET;
    pReqInfo->smartParams.iQPOffset = iQPOffset;
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQPBounds(AL_TEncCtx* pCtx, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    pCtx->pSettings->tChParam[i].tRCParam.iMinQP[sliceType] = iMinQP;
    pCtx->pSettings->tChParam[i].tRCParam.iMaxQP[sliceType] = iMaxQP;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQPIPDelta(AL_TEncCtx* pCtx, int16_t uIPDelta)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    pCtx->pSettings->tChParam[i].tRCParam.uIPDelta = uIPDelta;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQPPBDelta(AL_TEncCtx* pCtx, int16_t uPBDelta)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    pCtx->pSettings->tChParam[i].tRCParam.uPBDelta = uPBDelta;
    setRcGopParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetAutoQP(AL_TEncCtx* pCtx, bool useAutoQP)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_SET_AUTO_QP;

    if(useAutoQP)
    {
      pCtx->pSettings->tChParam[i].eEncOptions |= AL_OPT_ENABLE_AUTO_QP;
    }
    else
      pCtx->pSettings->tChParam[i].eEncOptions &= ~AL_OPT_ENABLE_AUTO_QP;

    pReqInfo->smartParams.useAutoQP = useAutoQP;
  }

  return true;
}

/****************************************************************************/
static bool AL_Common_Encoder_SetChannelResolution(AL_TLayerCtx* pLayerCtx, AL_TEncChanParam* pChanParam, AL_TDimension tDim)
{
  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChanParam->ePicFormat);

  if(
    (AL_ParamConstraints_CheckResolution(pChanParam->eProfile, eChromaMode, 1 << pChanParam->uLog2MaxCuSize, tDim.iWidth, tDim.iHeight) != CRERROR_OK)
    || (!AL_SrcBuffersChecker_UpdateResolution(&pLayerCtx->srcBufferChecker, tDim))
    )
    return false;
  pChanParam->uEncWidth = tDim.iWidth;
  pChanParam->uEncHeight = tDim.iHeight;
  return true;
}

bool AL_Common_Encoder_SetInputResolution(AL_TEncCtx* pCtx, AL_TDimension tDim)
{
  Rtos_GetMutex(pCtx->Mutex);

  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncChanParam* pChanParam = &pCtx->pSettings->tChParam[i];
    AL_TLayerCtx* pLayerCtx = &pCtx->tLayerCtx[i];

    if((IsGopRestartForbidden(&pCtx->pSettings->tChParam[i])) || (!AL_Common_Encoder_SetChannelResolution(pLayerCtx, pChanParam, tDim)))
      AL_CRIT_SECTION_RETURN_ERROR(pCtx->Mutex, AL_ERR_CMD_NOT_ALLOWED);

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pLayerCtx);
    pReqInfo->eReqOptions |= AL_OPT_SET_INPUT_RESOLUTION;

    {
      pReqInfo->eReqOptions |= AL_OPT_RESTART_GOP;
    }

    pReqInfo->dynResParams.tInputResolution = tDim;
  }

  Rtos_ReleaseMutex(pCtx->Mutex);

  return true;
}

static bool AL_Common_Encoder_SetLoopFilterOffset(AL_TEncCtx* pCtx, bool bBeta, int8_t iOffset)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncChanParam* pChanParam = &pCtx->pSettings->tChParam[i];

    if(pChanParam->tGopParam.eGdrMode != AL_GDR_OFF)
    {
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);
    }
    AL_TLayerCtx* pLayerCtx = &pCtx->tLayerCtx[i];

    bool bValidCmd = bBeta ? AL_ParamConstraints_CheckLFBetaOffset(pChanParam->eProfile, iOffset) : AL_ParamConstraints_CheckLFTcOffset(pChanParam->eProfile, iOffset);

    if(!bValidCmd)
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

    int8_t* pTargetOffset = bBeta ? &pChanParam->iBetaOffset : &pChanParam->iTcOffset;
    *pTargetOffset = iOffset;

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pLayerCtx);
    pReqInfo->eReqOptions |= AL_OPT_SET_LF_OFFSETS;
    pReqInfo->smartParams.iLFBetaOffset = pCtx->pSettings->tChParam[i].iBetaOffset;
    pReqInfo->smartParams.iLFTcOffset = pCtx->pSettings->tChParam[i].iTcOffset;
  }

  return true;
}

bool AL_Common_Encoder_SetLoopFilterBetaOffset(AL_TEncCtx* pCtx, int8_t iBetaOffset)
{
  return AL_Common_Encoder_SetLoopFilterOffset(pCtx, true, iBetaOffset);
}

bool AL_Common_Encoder_SetLoopFilterTcOffset(AL_TEncCtx* pCtx, int8_t iTcOffset)
{
  return AL_Common_Encoder_SetLoopFilterOffset(pCtx, false, iTcOffset);
}

bool AL_Common_Encoder_SetQPChromaOffsets(AL_TEncCtx* pCtx, int8_t iQp1Offset, int8_t iQp2Offset)
{
  for(int i = 0; i < pCtx->pSettings->NumLayer; ++i)
  {
    AL_TEncChanParam* pChanParam = &pCtx->pSettings->tChParam[i];

    if(AL_IS_HEVC(pChanParam->eProfile) || AL_IS_VVC(pChanParam->eProfile))
    {
      iQp1Offset -= pChanParam->iCbSliceQpOffset;
      iQp2Offset -= pChanParam->iCrSliceQpOffset;
    }

    if(!AL_ParamConstraints_CheckChromaOffsets(pChanParam->eProfile, iQp1Offset, iQp2Offset,
                                               pChanParam->iCbSliceQpOffset, pChanParam->iCrSliceQpOffset))
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

    pChanParam->iCbPicQpOffset = iQp1Offset;
    pChanParam->iCrPicQpOffset = iQp2Offset;
  }

  return true;
}

bool AL_Common_Encoder_SetHDRSEIs(AL_TEncCtx* pCtx, AL_THDRSEIs* pHDRSEIs)
{
  if((pHDRSEIs->bHasMDCV && !(pCtx->pSettings->uEnableSEI & AL_SEI_MDCV)) ||
     (pHDRSEIs->bHasCLL && !(pCtx->pSettings->uEnableSEI & AL_SEI_CLL)) ||
     (pHDRSEIs->bHasATC && !(pCtx->pSettings->uEnableSEI & AL_SEI_ATC)) ||
     (pHDRSEIs->bHasST2094_10 && !(pCtx->pSettings->uEnableSEI & AL_SEI_ST2094_10)) ||
     (pHDRSEIs->bHasST2094_40 && !(pCtx->pSettings->uEnableSEI & AL_SEI_ST2094_40)))
    AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

  Rtos_GetMutex(pCtx->Mutex);
  AL_THDRSEIs* pPoolSEIs = GetNextHDRSEIs(&pCtx->tHDRPool);
  Rtos_ReleaseMutex(pCtx->Mutex);

  AL_HDRSEIs_Copy(pHDRSEIs, pPoolSEIs);
  pCtx->tHDRPool.bHDRChanged = true;

  return true;
}

/****************************************************************************/
AL_HLSInfo* AL_GetHLSInfo(AL_TEncCtx* pCtx, int iPicID)
{
  return &pCtx->tFrameInfoPool.FrameInfos[iPicID].tHLSUpdateInfo;
}

static bool isSeiEnable(AL_ESeiFlag eFlags)
{
  return eFlags != AL_SEI_NONE;
}

AL_TNalsData AL_ExtractNalsData(AL_TEncCtx* pCtx, int iLayerID, int iPicID)
{
  (void)iPicID;
  AL_TEncSettings const* pSettings = pCtx->pSettings;
  AL_TNalsData data = { 0 };
  data.vps = &pCtx->vps;
  data.sps = &pCtx->tLayerCtx[iLayerID].sps;
  data.pps = &pCtx->tLayerCtx[iLayerID].pps;
  data.aud = &pCtx->tLayerCtx[iLayerID].aud;

  data.eStartCodeBytesAligned = pSettings->tChParam[iLayerID].eStartCodeBytesAligned;
  data.fillerCtrlMode = pSettings->eEnableFillerData;
  data.seiFlags = (AL_ESeiFlag)pSettings->uEnableSEI;

  if(pSettings->tChParam[0].bSubframeLatency)
    data.fillerCtrlMode = AL_FILLER_ENC;

  AL_TFrameInfo* pFI = &pCtx->tFrameInfoPool.FrameInfos[iPicID];
  AL_THDRSEIs* pHDRSEIs = GetHDRSEIs(&pCtx->tHDRPool, pFI->tHLSUpdateInfo.iHDRID);

  data.seiData.pHDRSEIs = pHDRSEIs;
  data.bMustWriteDynHDR = pFI->tHLSUpdateInfo.bHDRChanged;

  if(pHDRSEIs == NULL || !pHDRSEIs->bHasMDCV)
    data.seiFlags &= ~AL_SEI_MDCV;

  if(pHDRSEIs == NULL || !pHDRSEIs->bHasCLL)
    data.seiFlags &= ~AL_SEI_CLL;

  if(pHDRSEIs == NULL || !pHDRSEIs->bHasATC)
    data.seiFlags &= ~AL_SEI_ATC;

  if(pHDRSEIs == NULL || !pHDRSEIs->bHasST2094_10)
    data.seiFlags &= ~AL_SEI_ST2094_10;

  if(pHDRSEIs == NULL || !pHDRSEIs->bHasST2094_40)
    data.seiFlags &= ~AL_SEI_ST2094_40;

  if(isSeiEnable(data.seiFlags))
  {
    data.seiData.initialCpbRemovalDelay = pCtx->initialCpbRemovalDelay;
    data.seiData.cpbRemovalDelay = pCtx->cpbRemovalDelay;
  }

  return data;
}

/****************************************************************************/
static void EndEncoding(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_64U streamUserPtr)
{
  AL_TCbUserParam* pCbUserParam = (AL_TCbUserParam*)pUserParam;
  AL_TEncCtx* pCtx = pCbUserParam->pCtx;
  int iLayerID = pCbUserParam->iLayerID;

  if(!pPicStatus)
  {
    pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.func(pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.userParam, NULL, NULL, iLayerID);
    return;
  }
  int streamId = (int)streamUserPtr;
  bool bFlushing = false;

  /* we require the stream to come back in the same order we sent them */
  AL_Assert(streamId >= 0 && streamId < AL_MAX_STREAM_BUFFER);
  AL_Assert(pCtx->tLayerCtx[iLayerID].iCurStreamRecv == streamId);

  if(!bFlushing)
    pCtx->tLayerCtx[iLayerID].iCurStreamRecv = (pCtx->tLayerCtx[iLayerID].iCurStreamRecv + 1) % AL_MAX_STREAM_BUFFER;

  AL_Common_SetError(pCtx, pPicStatus->eErrorCode);

  AL_TBuffer* pStream = pCtx->tLayerCtx[iLayerID].StreamSent[streamId];
  AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(AL_Buffer_GetData(pStream) + pPicStatus->uStreamPartOffset);

  for(int iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
    Rtos_InvalidateCacheMemory(AL_Buffer_GetData(pStream) + pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize);

  int iPoolID = pPicStatus->UserParam;
  AL_TFrameInfo* pFI = &pCtx->tFrameInfoPool.FrameInfos[iPoolID];

  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  AL_Assert(pStreamMeta);
  pStreamMeta->uTemporalID = pPicStatus->uTempId;

  if(!AL_IS_ERROR_CODE(pPicStatus->eErrorCode) || (pPicStatus->eErrorCode == AL_ERR_WATCHDOG_TIMEOUT))
  {
    Rtos_GetMutex(pCtx->Mutex);
    pCtx->encoder.updateHlsAndWriteSections(pCtx, pPicStatus, pStream, iLayerID, iPoolID);
    Rtos_ReleaseMutex(pCtx->Mutex);
  }

  AL_TPictureMetaData* pPictureMeta = (AL_TPictureMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_PICTURE);

  if(pPictureMeta)
  {
    pPictureMeta->eType = pPicStatus->eType;
    pPictureMeta->bSkipped = pPicStatus->bSkip;
  }

  AL_TBuffer* pSrc = (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;

  if(pCtx->pSettings->LookAhead > 0 || pCtx->pSettings->TwoPass == 1)
  {
    // Transmits first pass information in the metadata
    AL_TLookAheadMetaData* pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_LOOKAHEAD);

    if(pPictureMetaLA)
    {
      pPictureMetaLA->iPictureSize = pPicStatus->iPictureSize;

      for(int8_t i = 0; i < 5; i++)
        pPictureMetaLA->iPercentIntra[i] = pPicStatus->iPercentIntra[i];
    }
  }

  AL_TRateCtrlMetaData* pRCMeta = (AL_TRateCtrlMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_RATECTRL);

  if(pRCMeta != NULL)
  {
    pRCMeta->bFilled = pPicStatus->bIsLastSlice;

    if(pRCMeta->bFilled)
      pRCMeta->tRateCtrlStats = pPicStatus->tRateCtrlStats;
  }

  {
    pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.func(pCtx->tLayerCtx[iLayerID].tEndEncodingCallback.userParam, pStream, pSrc, iLayerID);

    if(pPicStatus->bIsLastSlice)
    {
      if(pCtx->encoder.shouldReleaseSource(pPicStatus))
        releaseSource(pCtx, pSrc, pFI);

      Rtos_GetMutex(pCtx->Mutex);
      ++pCtx->iFrameCountDone;

      if(pCtx->encoder.shouldReleaseSource(pPicStatus))
      {

        if(pFI->tHLSUpdateInfo.iHDRID != INVALID_POOL_ID)
          DecrRefCount(&pCtx->tHDRPool, pFI->tHLSUpdateInfo.iHDRID);
        GiveIdBackToPool(&pCtx->tFrameInfoPool.tIDPool, iPoolID);
      }
      Rtos_ReleaseMutex(pCtx->Mutex);

      Rtos_ReleaseSemaphore(pCtx->PendingEncodings);
    }
  }

  if(!bFlushing)
  {
    Rtos_GetMutex(pCtx->Mutex);
    AL_Buffer_Unref(pStream);
    Rtos_ReleaseMutex(pCtx->Mutex);
  }
}

/****************************************************************************/
AL_ERR AL_Common_Encoder_CreateChannel(AL_TEncCtx* pCtx, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings)
{
  AL_Assert(pSettings->NumLayer > 0);

  AL_ERR errorCode = AL_ERROR;

  if(!pCtx->encoder.configureChannel)
    goto fail;

  AL_TEncChanParam* pChParam = initChannelParam(pCtx, pSettings);

  pCtx->pScheduler = pScheduler;

  if(!init(pCtx, pChParam, pAlloc))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  if(!InitIDPool(&pCtx->tFrameInfoPool.tIDPool, MAX_NUM_LAYER * ENC_MAX_CMD))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  if(!InitIDPool(&pCtx->tHDRPool.tIDPool, ENC_MAX_CMD))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  pCtx->encoder.configureChannel(pCtx, pChParam, pSettings);

  pCtx->tLayerCtx[0].callback_user_param.pCtx = pCtx;
  pCtx->tLayerCtx[0].callback_user_param.iLayerID = 0;

  AL_TEncScheduler_CB_EndEncoding CBs = { 0 };
  CBs.func = EndEncoding;
  CBs.userParam = &pCtx->tLayerCtx[0].callback_user_param;
#if AL_ENABLE_ENC_WATCHDOG
  AL_Common_Encoder_SetWatchdogCB(pCtx, pSettings);
#endif

  // HACK: needed to preprocess scaling list, but doesn't generate the good nals
  // because we are missing some value populated by AL_IEncScheduler_CreateChannel
  pCtx->encoder.generateNals(pCtx, 0, true);

  TBufferEP* pEP1 = &pCtx->tLayerCtx[0].tBufEP1;
  TBufferEP* pEP4 = NULL;

  if(!PreprocessEncoderParam(pCtx, pEP1, pEP4, 0))
    goto fail;

  AL_HANDLE hRcPluginDmaContext = NULL;
  hRcPluginDmaContext = pCtx->pSettings->hRcPluginDmaContext;
  errorCode = AL_IEncScheduler_CreateChannel(&pCtx->tLayerCtx[0].hChannel, pCtx->pScheduler, &pCtx->tLayerCtx[0].tMDChParam, &pEP1->tMD, hRcPluginDmaContext, &CBs);

  if(AL_IS_ERROR_CODE(errorCode))
    goto fail;

  pCtx->PendingEncodings = Rtos_CreateSemaphore(ENC_MAX_CMD - 1);

  setMaxNumRef(pCtx, pChParam);
  pCtx->encoder.generateNals(pCtx, 0, true);

  pCtx->iInitialNumB = pChParam->tGopParam.uNumB;
  pCtx->uInitialFrameRate = pChParam->tRCParam.uFrameRate;

  return errorCode;

  fail:
  destroyChannels(pCtx);
  return errorCode;
}

int AL_Common_Encoder_AddSei(AL_TEncCtx* pCtx, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId)
{
  AL_TEncChanParam* pChannel = &pCtx->pSettings->tChParam[0];

  AL_TNuts nuts;
  AL_EProfile eProfile = pChannel->eProfile;
  bool exists = AL_CreateNuts(&nuts, eProfile);

  if(!exists)
    return -1;
  return AL_WriteSeiSection(AL_GET_CODEC(eProfile), nuts, pStream, isPrefix, iPayloadType, pPayload, iPayloadSize, iTempId, pChannel->eStartCodeBytesAligned);
}

