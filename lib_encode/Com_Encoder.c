/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include "lib_encode/lib_encoder.h"
#include "Com_Encoder.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/StreamSection.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferPictureMeta.h"
#include "lib_common_enc/IpEncFourCC.h"
#include <assert.h>
#include "lib_common/Utils.h"
#include "lib_preprocess/LoadLda.h"

#define DEBUG_PATH "."


/***************************************************************************/
void AL_Common_Encoder_WaitReadiness(AL_TEncCtx* pCtx)
{
  Rtos_GetSemaphore(pCtx->m_PendingEncodings, AL_WAIT_FOREVER);
}

/***************************************************************************/
static void RemoveSourceSent(AL_TEncCtx* pCtx, AL_TBuffer const* const pSrc)
{
  Rtos_GetMutex(pCtx->m_Mutex);

  for(int i = 0; i < AL_MAX_SOURCE_BUFFER; i++)
  {
    if(pCtx->m_SourceSent[i] == pSrc)
    {
      pCtx->m_SourceSent[i] = NULL;
      Rtos_ReleaseMutex(pCtx->m_Mutex);
      return;
    }
  }

  assert(0);
  Rtos_ReleaseMutex(pCtx->m_Mutex);
}

static void releaseSource(AL_TEncCtx* pCtx, AL_TBuffer* pSrc, AL_TBuffer* pQpTable)
{
  RemoveSourceSent(pCtx, pSrc);
  AL_Buffer_Unref(pSrc);

  if(pQpTable)
    AL_Buffer_Unref(pQpTable);
}

/***************************************************************************/
static void AL_sEncoder_DestroySkippedPictureData(AL_TEncCtx* pCtx)
{
  Rtos_Free(pCtx->m_pSkippedPicture.pBuffer);
  pCtx->m_pSkippedPicture.pBuffer = NULL;
  pCtx->m_pSkippedPicture.iBufSize = 0;
  pCtx->m_pSkippedPicture.iNumBits = 0;
  pCtx->m_pSkippedPicture.iNumBins = 0;
}

/****************************************************************************/
static void AL_sEncoder_DefaultEndEncodingCB(void* pUserParam, AL_TBuffer* pStream, AL_TBuffer const* const pSrc)
{
  (void)pSrc;
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;
  assert(pCtx);

  Rtos_GetMutex(pCtx->m_Mutex);

  AL_TEncoder enc;
  enc.pCtx = pCtx;

  if(AL_Common_Encoder_PutStreamBuffer(&enc, pStream))
    AL_Buffer_Unref(pStream);

  Rtos_ReleaseMutex(pCtx->m_Mutex);
}

static void AL_Common_Encoder_InitBuffers(AL_TEncCtx* pCtx, AL_TAllocator* pAllocator)
{
  bool bRet = MemDesc_AllocNamed(&pCtx->m_tBufEP1.tMD, pAllocator, GetAllocSizeEP1(), "ep1");
  assert(bRet);

  pCtx->m_tBufEP1.uFlags = 0;
  pCtx->m_iCurPool = 0;
}

static void init(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TAllocator* pAllocator)
{
  AL_Common_Encoder_InitBuffers(pCtx, pAllocator);

  // default callback
  pCtx->m_callback.func = AL_sEncoder_DefaultEndEncodingCB;
  pCtx->m_callback.userParam = pCtx;

  AL_SrcBuffersChecker_Init(&pCtx->m_srcBufferChecker, pChParam);


  pCtx->m_iLastIdrId = 0;

  pCtx->m_seiData.initialCpbRemovalDelay = pChParam->tRCParam.uInitialRemDelay;
  pCtx->m_seiData.cpbRemovalDelay = 0;

  pCtx->m_iCurStreamSent = 0;
  pCtx->m_iCurStreamRecv = 0;
  pCtx->m_iFrameCountDone = 0;

  pCtx->m_eError = AL_SUCCESS;

  int const iWidthInLcu = (pChParam->uWidth + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;
  int const iHeightInLcu = (pChParam->uHeight + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;

  pCtx->m_iNumLCU = iWidthInLcu * iHeightInLcu;

  Rtos_Memset(pCtx->m_Pool, 0, sizeof pCtx->m_Pool);
  Rtos_Memset(pCtx->m_SourceSent, 0, sizeof(pCtx->m_SourceSent));

  pCtx->m_Mutex = Rtos_CreateMutex();
  assert(pCtx->m_Mutex);
}


/***************************************************************************/
static AL_TEncRequestInfo* getCurrentCommands(AL_TEncCtx* pCtx)
{
  return &pCtx->m_currentRequestInfo;
}

/***************************************************************************/
void AL_Common_Encoder_NotifySceneChange(AL_TEncoder* pEnc, int iAhead)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pEnc->pCtx);
  pReqInfo->eReqOptions |= AL_OPT_SCENE_CHANGE;
  pReqInfo->uSceneChangeDelay = iAhead;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyLongTerm(AL_TEncoder* pEnc)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pEnc->pCtx);
  pReqInfo->eReqOptions |= AL_OPT_USE_LONG_TERM;
}


/***************************************************************************/
bool AL_Common_Encoder_PutStreamBuffer(AL_TEncoder* pEnc, AL_TBuffer* pStream)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  assert(pMetaData);
  assert(pCtx);

  AL_StreamMetaData_ClearAllSections(pMetaData);
  Rtos_GetMutex(pCtx->m_Mutex);
  pCtx->m_StreamSent[pCtx->m_iCurStreamSent] = pStream;
  int curStreamSent = pCtx->m_iCurStreamSent;
  pCtx->m_iCurStreamSent = (pCtx->m_iCurStreamSent + 1) % AL_MAX_STREAM_BUFFER;
  AL_Buffer_Ref(pStream);

  /* Can call AL_Common_Encoder_PutStreamBuffer again */
  AL_ISchedulerEnc_PutStreamBuffer(pCtx->m_pScheduler, pCtx->m_hChannel, pStream, curStreamSent, ENC_MAX_HEADER_SIZE);
  Rtos_ReleaseMutex(pCtx->m_Mutex);

  return true;
}

/***************************************************************************/
bool AL_Common_Encoder_GetRecPicture(AL_TEncoder* pEnc, TRecPic* pRecPic)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  assert(pCtx);

  return AL_ISchedulerEnc_GetRecPicture(pCtx->m_pScheduler, pCtx->m_hChannel, pRecPic);
}

/***************************************************************************/
void AL_Common_Encoder_ReleaseRecPicture(AL_TEncoder* pEnc, TRecPic* pRecPic)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  assert(pCtx);

  AL_ISchedulerEnc_ReleaseRecPicture(pCtx->m_pScheduler, pCtx->m_hChannel, pRecPic);
}

void AL_Common_Encoder_ConfigureZapper(AL_TEncCtx* pCtx, AL_TEncInfo* pEncInfo);


/* +1 / -1 business is needed as 0 == NULL is the error value of the fifo
 * this also means that GetNextPoolId returns -1 on error */

/* not static because of VP9 repeats */
int AL_Common_Encoder_GetNextPoolId(AL_TFifo* poolIds)
{
  return (int)((intptr_t)AL_Fifo_Dequeue(poolIds, AL_NO_WAIT)) - 1;
}

static void GiveIdBackToPool(AL_TFifo* poolIds, int iPoolID)
{
  AL_Fifo_Queue(poolIds, (void*)((intptr_t)(iPoolID + 1)), AL_NO_WAIT);
}

static void DeinitPoolIds(AL_TEncCtx* pCtx)
{
  AL_Fifo_Deinit(&pCtx->m_iPoolIds);
}

static bool InitPoolIds(AL_TEncCtx* pCtx)
{
  if(!AL_Fifo_Init(&pCtx->m_iPoolIds, ENC_MAX_CMD))
    return false;

  for(int i = 0; i < ENC_MAX_CMD; ++i)
    GiveIdBackToPool(&pCtx->m_iPoolIds, i);

  return true;
}

static bool EndOfStream(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  return AL_ISchedulerEnc_EncodeOneFrame(pCtx->m_pScheduler, pCtx->m_hChannel, NULL, NULL, NULL);
}

/***************************************************************************/
static void AddSourceSent(AL_TEncCtx* pCtx, AL_TBuffer* pSrc)
{
  Rtos_GetMutex(pCtx->m_Mutex);

  for(int i = 0; i < AL_MAX_SOURCE_BUFFER; i++)
  {
    if(pCtx->m_SourceSent[i] == NULL)
    {
      pCtx->m_SourceSent[i] = pSrc;
      Rtos_ReleaseMutex(pCtx->m_Mutex);
      return;
    }
  }

  assert(0);
  Rtos_ReleaseMutex(pCtx->m_Mutex);
}

void AL_Common_Encoder_SetEncodingOptions(AL_TEncCtx* pCtx, AL_TFrameInfo* pFI);

/***************************************************************************/
bool AL_Common_Encoder_Process(AL_TEncoder* pEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  if(!pFrame)
    return EndOfStream(pEnc);

  AL_Common_Encoder_WaitReadiness(pCtx);
  pCtx->m_iCurPool = AL_Common_Encoder_GetNextPoolId(&pCtx->m_iPoolIds);

  const int AL_DEFAULT_PPS_QP_26 = 26;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[pCtx->m_iCurPool];
  AL_TEncInfo* pEI = &pFI->tEncInfo;
  AL_TEncPicBufAddrs addresses;
  AL_TSrcMetaData* pMetaData = NULL;

  pEI->UserParam = pCtx->m_iCurPool;
  pEI->iPpsQP = AL_DEFAULT_PPS_QP_26;

  AL_Common_Encoder_SetEncodingOptions(pCtx, pFI);

  pMetaData = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_SOURCE);

  pFI->pQpTable = pQpTable;

  if(pQpTable)
  {
    AL_Buffer_Ref(pQpTable);
    addresses.pEP2 = AL_Allocator_GetPhysicalAddr(pQpTable->pAllocator, pQpTable->hBuf);
    addresses.pEP2_v = (AL_PTR64)(uintptr_t)AL_Allocator_GetVirtualAddr(pQpTable->pAllocator, pQpTable->hBuf);
    pEI->eEncOptions |= AL_OPT_USE_QP_TABLE;
  }
  else
  {
    addresses.pEP2_v = 0;
    addresses.pEP2 = 0;
  }

  addresses.pSrc_Y = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf);
  addresses.pSrc_UV = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf) + AL_SrcMetaData_GetOffsetC(pMetaData);
  addresses.uPitchSrc = pMetaData->tPitches.iLuma;

  if(AL_GetBitDepth(pMetaData->tFourCC) > 8)
    addresses.uPitchSrc = 0x80000000 | addresses.uPitchSrc;

  if(!AL_SrcBuffersChecker_CanBeUsed(&pCtx->m_srcBufferChecker, pFrame))
    return false;

  AL_Buffer_Ref(pFrame);
  pEI->SrcHandle = (AL_64U)(uintptr_t)pFrame;


  AddSourceSent(pCtx, pFrame);

  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pCtx);
  bool bRet = AL_ISchedulerEnc_EncodeOneFrame(pCtx->m_pScheduler, pCtx->m_hChannel, pEI, pReqInfo, &addresses);

  Rtos_Memset(pReqInfo, 0, sizeof(*pReqInfo));
  Rtos_Memset(pEI, 0, sizeof(*pEI));
  return bRet;
}

/***************************************************************************/
AL_ERR AL_Common_Encoder_GetLastError(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  Rtos_GetMutex(pCtx->m_Mutex);
  AL_ERR eError = pEnc->pCtx->m_eError;
  Rtos_ReleaseMutex(pCtx->m_Mutex);

  return eError;
}

static void setMaxNumRef(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  pCtx->m_iMaxNumRef = AL_GET_PPS_NUM_ACT_REF_L0(pChParam->uPpsParam);

  if(pCtx->m_iMaxNumRef)
    pCtx->m_iMaxNumRef += 1;
}

void AL_Common_Encoder_SetHlsParam(AL_TEncChanParam* pChParam)
{
  if(pChParam->uCabacInitIdc)
    pChParam->uPpsParam |= AL_PPS_CABAC_INIT_PRES_FLAG;
}

int8_t AL_Common_Encoder_GetInitialQP(uint32_t iBitPerPixel)
{
  int8_t iInitQP = AL_BitPerPixelQP[MAX_IDX_BIT_PER_PEL][1];

  for(int i = 0; i < MAX_IDX_BIT_PER_PEL; i++)
  {
    if(iBitPerPixel <= (uint32_t)AL_BitPerPixelQP[i][0])
    {
      iInitQP = AL_BitPerPixelQP[i][1];
      break;
    }
  }

  return iInitQP;
}

bool AL_Common_Encoder_IsInitialQpProvided(AL_TEncChanParam* pChParam)
{
  return pChParam->tRCParam.eRCMode == AL_RC_CONST_QP || pChParam->tRCParam.iInitialQP >= 0;
}

uint32_t AL_Common_Encoder_ComputeBitPerPixel(AL_TEncChanParam* pChParam)
{
  return pChParam->tRCParam.uTargetBitRate / pChParam->tRCParam.uFrameRate * 1000 / (pChParam->uWidth * pChParam->uHeight);
}

/****************************************************************************/
void AL_Common_Encoder_SetEncodingOptions(AL_TEncCtx* pCtx, AL_TFrameInfo* pFI)
{
  AL_TEncInfo* pEncInfo = &pFI->tEncInfo;

  if(pCtx->m_Settings.bForceLoad)
    pEncInfo->eEncOptions |= AL_OPT_FORCE_LOAD;

  if(pCtx->m_Settings.bDisIntra)
    pEncInfo->eEncOptions |= AL_OPT_DISABLE_INTRA;

  if(pCtx->m_Settings.bDependentSlice)
    pEncInfo->eEncOptions |= AL_OPT_DEPENDENT_SLICES;

  if(pCtx->m_Settings.tChParam.uL2PrefetchMemSize)
    pEncInfo->eEncOptions |= AL_OPT_USE_L2;

}

/****************************************************************************/
static AL_TEncChanParam* initChannelParam(AL_TEncCtx* pCtx, AL_TEncSettings const* pSettings)
{
  AL_TEncChanParam* pChParam;

  // Keep Settings and source video information -----------------------
  pCtx->m_Settings = *pSettings;

  pChParam = &pCtx->m_Settings.tChParam;

  pChParam->uWidth = pSettings->tChParam.uWidth;
  pChParam->uHeight = pSettings->tChParam.uHeight;
  pChParam->uMaxCuSize = pSettings->tChParam.uMaxCuSize;
  pChParam->eProfile = pSettings->tChParam.eProfile;
  pChParam->uLevel = pSettings->tChParam.uLevel;
  pChParam->uTier = pSettings->tChParam.uTier;

  pChParam->tGopParam.uNumB = pSettings->tChParam.tGopParam.uNumB;


  if(pChParam->eOptions & AL_OPT_FORCE_MV_CLIP)
  {
    pChParam->uClipHrzRange = pSettings->uClipHrzRange;
    pChParam->uClipVrtRange = pSettings->uClipVrtRange;
  }
  else
  {
    pChParam->uClipHrzRange = 0;
    pChParam->uClipVrtRange = 0;
  }


  // Update L2 cache param -------------------------------------------
  if(pSettings->iPrefetchLevel2 > 0)
  {
    pChParam->bL2PrefetchAuto = true;
    pChParam->uL2PrefetchMemSize = pSettings->iPrefetchLevel2;
  }
  else
  {
    pChParam->bL2PrefetchAuto = false;
    pChParam->uL2PrefetchMemSize = 0;
  }
  pChParam->uL2PrefetchMemOffset = 0;

  // Update Auto QP param -------------------------------------------
  if(pSettings->eQpCtrlMode & MASK_AUTO_QP)
  {
    pChParam->eOptions |= AL_OPT_ENABLE_AUTO_QP;

    if(pSettings->eQpCtrlMode & ADAPTIVE_AUTO_QP)
      pChParam->eOptions |= AL_OPT_ADAPT_AUTO_QP;
  }

  // Update QP table param -------------------------------------------
  if(pSettings->eQpCtrlMode & RELATIVE_QP)
    pChParam->eOptions |= AL_OPT_QP_TAB_RELATIVE;

  return &pCtx->m_Settings.tChParam;
}

/****************************************************************************/
void AL_Common_Encoder_SetME(int iHrzRange_P, int iVrtRange_P, int iHrzRange_B, int iVrtRange_B, AL_TEncChanParam* pChParam)
{
  if(pChParam->pMeRange[SLICE_P][0] < 0)
    pChParam->pMeRange[SLICE_P][0] = iHrzRange_P;

  if(pChParam->pMeRange[SLICE_P][1] < 0)
    pChParam->pMeRange[SLICE_P][1] = iVrtRange_P;

  if(pChParam->pMeRange[SLICE_B][0] < 0)
    pChParam->pMeRange[SLICE_B][0] = iHrzRange_B;

  if(pChParam->pMeRange[SLICE_B][1] < 0)
    pChParam->pMeRange[SLICE_B][1] = iVrtRange_B;
}

void AL_Common_Encoder_DeinitBuffers(AL_TEncCtx* pCtx)
{
  MemDesc_Free(&pCtx->m_tBufEP1.tMD);
}

static void releaseStreams(AL_TEncCtx* pCtx)
{
  for(int streamId = pCtx->m_iCurStreamRecv; streamId != pCtx->m_iCurStreamSent; streamId = (streamId + 1) % AL_MAX_STREAM_BUFFER)
  {
    AL_TBuffer* pStream = pCtx->m_StreamSent[streamId];
    pCtx->m_callback.func(pCtx->m_callback.userParam, pStream, NULL);
    AL_Buffer_Unref(pStream);
  }
}

static void releaseSources(AL_TEncCtx* pCtx)
{
  for(int sourceId = 0; sourceId < AL_MAX_SOURCE_BUFFER; sourceId++)
  {
    AL_TBuffer* pSource = pCtx->m_SourceSent[sourceId];

    if(pSource != NULL)
    {
      pCtx->m_callback.func(pCtx->m_callback.userParam, NULL, pSource);
      releaseSource(pCtx, pSource, NULL);
    }
  }
}

/***************************************************************************/
static void destroy(AL_TEncCtx* pCtx)
{
  AL_ISchedulerEnc_DestroyChannel(pCtx->m_pScheduler, pCtx->m_hChannel);

  releaseStreams(pCtx);
  releaseSources(pCtx);

  Rtos_DeleteMutex(pCtx->m_Mutex);
  Rtos_DeleteSemaphore(pCtx->m_PendingEncodings);

  AL_sEncoder_DestroySkippedPictureData(pCtx);
  AL_Common_Encoder_DeinitBuffers(pCtx);
  DeinitPoolIds(pCtx);
}

/***************************************************************************/
void AL_Common_Encoder_Destroy(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  destroy(pCtx);
  Rtos_Free(pCtx);
}

/****************************************************************************/
void AL_Common_Encoder_RestartGop(AL_TEncoder* pEnc)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pEnc->pCtx);
  pReqInfo->eReqOptions |= AL_OPT_RESTART_GOP;
}

static void sendNewParams(AL_TEncCtx* pCtx)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(pCtx);
  pReqInfo->eReqOptions |= AL_OPT_UPDATE_PARAMS;
  pReqInfo->smartParams.rc = pCtx->m_Settings.tChParam.tRCParam;
  pReqInfo->smartParams.gop = pCtx->m_Settings.tChParam.tGopParam;
}

/****************************************************************************/
void AL_Common_Encoder_SetGopLength(AL_TEncoder* pEnc, int iGopLength)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  pCtx->m_Settings.tChParam.tGopParam.uGopLength = iGopLength;
  sendNewParams(pCtx);
}

/****************************************************************************/
void AL_Common_Encoder_SetGopNumB(AL_TEncoder* pEnc, int iNumB)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  pCtx->m_Settings.tChParam.tGopParam.uNumB = iNumB;
  sendNewParams(pCtx);
}

/****************************************************************************/
void AL_Common_Encoder_SetBitRate(AL_TEncoder* pEnc, int iBitRate)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  pCtx->m_Settings.tChParam.tRCParam.uTargetBitRate = iBitRate;

  if(pCtx->m_Settings.tChParam.tRCParam.eRCMode == AL_RC_CBR)
    pCtx->m_Settings.tChParam.tRCParam.uMaxBitRate = iBitRate;
  sendNewParams(pCtx);
}

/****************************************************************************/
void AL_Common_Encoder_SetFrameRate(AL_TEncoder* pEnc, uint16_t uFrameRate, uint16_t uClkRatio)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  pCtx->m_Settings.tChParam.tRCParam.uFrameRate = uFrameRate;
  pCtx->m_Settings.tChParam.tRCParam.uClkRatio = uClkRatio;
  sendNewParams(pCtx);
}


static bool isSeiEnable(uint32_t uFlags)
{
  return uFlags != SEI_NONE;
}

NalsData AL_ExtractNalsData(AL_TEncCtx* pCtx)
{
  NalsData data = { 0 };
  data.vps = &pCtx->m_vps;
  data.sps = &pCtx->m_sps;
  data.pps = &pCtx->m_pps;

  AL_TEncSettings const* pSettings = &pCtx->m_Settings;

  data.shouldWriteAud = pSettings->bEnableAUD;
  data.shouldWriteFillerData = pSettings->bEnableFillerData;
  data.seiFlags = pSettings->uEnableSEI;

  if(pSettings->tChParam.bSubframeLatency)
    data.seiFlags |= SEI_EOF;

  if(isSeiEnable(data.seiFlags))
    data.seiData = &pCtx->m_seiData;

  return data;
}

/****************************************************************************/
static void EndEncoding(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_64U streamUserPtr)
{
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;

  if(!pPicStatus)
  {
    pCtx->m_callback.func(pCtx->m_callback.userParam, NULL, NULL);
    return;
  }

  int streamId = (int)streamUserPtr;

  /* we require the stream to come back in the same order we sent them */
  assert(streamId >= 0 && streamId < AL_MAX_STREAM_BUFFER);
  assert(pCtx->m_iCurStreamRecv == streamId);

  pCtx->m_iCurStreamRecv = (pCtx->m_iCurStreamRecv + 1) % AL_MAX_STREAM_BUFFER;

  Rtos_GetMutex(pCtx->m_Mutex);
  pCtx->m_eError = pPicStatus->eErrorCode;
  Rtos_ReleaseMutex(pCtx->m_Mutex);

  AL_TBuffer* pStream = pCtx->m_StreamSent[streamId];

  if(!(pPicStatus->eErrorCode & AL_ERROR || pPicStatus->bSkip))
    pCtx->encoder.updateHlsAndWriteSections(pCtx, pPicStatus, pStream);

  AL_TPictureMetaData* pPictureMeta = (AL_TPictureMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_PICTURE);

  if(pPictureMeta)
    pPictureMeta->eType = pPicStatus->eType;

  int iPoolID = pPicStatus->UserParam;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[iPoolID];

  AL_TBuffer* pSrc = (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;
  pCtx->m_callback.func(pCtx->m_callback.userParam, pStream, pSrc);

  if(pPicStatus->bIsLastSlice)
  {
    releaseSource(pCtx, pSrc, pFI->pQpTable);

    Rtos_GetMutex(pCtx->m_Mutex);
    ++pCtx->m_iFrameCountDone;
    GiveIdBackToPool(&pCtx->m_iPoolIds, iPoolID);
    Rtos_ReleaseMutex(pCtx->m_Mutex);

    Rtos_ReleaseSemaphore(pCtx->m_PendingEncodings);
  }

  Rtos_GetMutex(pCtx->m_Mutex);
  AL_Buffer_Unref(pStream);
  Rtos_ReleaseMutex(pCtx->m_Mutex);
}

/****************************************************************************/
AL_ERR AL_Common_Encoder_CreateChannel(AL_TEncCtx* pCtx, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings)
{
  AL_TEncChanParam* pChParam = initChannelParam(pCtx, pSettings);
  init(pCtx, pChParam, pAlloc);

  AL_ERR errorCode = AL_ERROR;

  if(!InitPoolIds(pCtx))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  pCtx->m_pScheduler = pScheduler;

  pCtx->encoder.configureChannel(pCtx, pSettings);

  AL_TISchedulerCallBacks CBs = { 0 };
  CBs.pfnEndEncodingCallBack = EndEncoding;
  CBs.pEndEncodingCBParam = pCtx;

  // Lambdas ----------------------------------------------------------------
  if(pSettings->tChParam.eLdaCtrlMode != DEFAULT_LDA)
  {
    pCtx->m_tBufEP1.uFlags |= EP1_BUF_LAMBDAS.Flag;

    if(pSettings->tChParam.eLdaCtrlMode == LOAD_LDA)
    {
      char const* ldaFilename = DEBUG_PATH "/Lambdas.hex";
      LoadLambdaFromFile(ldaFilename, &pCtx->m_tBufEP1);
    }
    else
      GetLambda(pSettings->tChParam.eLdaCtrlMode, &pSettings->tChParam, pCtx->m_tBufEP1.tMD.pVirtualAddr, true);
  }

  errorCode = AL_ISchedulerEnc_CreateChannel(&pCtx->m_hChannel, pCtx->m_pScheduler, pChParam, pCtx->m_tBufEP1.tMD.uPhysicalAddr, &CBs);

  if(errorCode != AL_SUCCESS)
    goto fail;


  pCtx->encoder.generateSkippedPictureData(pCtx);
  pCtx->m_PendingEncodings = Rtos_CreateSemaphore(ENC_MAX_CMD - 1);

  setMaxNumRef(pCtx, pChParam);
  pCtx->encoder.generateNals(pCtx);

  return AL_SUCCESS;

  fail:
  destroy(pCtx);
  return errorCode;
}

