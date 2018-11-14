/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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
#include "lib_common/BufferLookAheadMeta.h"
#include "lib_common_enc/IpEncFourCC.h"
#include <assert.h>
#include "lib_common/Utils.h"
#include "lib_encode/LoadLda.h"



#define DEBUG_PATH "."


/***************************************************************************/
static AL_ELdaCtrlMode GetFinalLdaMode(const AL_TEncChanParam* pChParam)
{
  if(pChParam->eLdaCtrlMode == AUTO_LDA)
  {
    if(pChParam->tRCParam.eRCMode == AL_RC_CONST_QP && ((pChParam->tGopParam.eMode == AL_GOP_MODE_DEFAULT && pChParam->tGopParam.uGopLength > 1 && pChParam->tGopParam.uNumB == 0) ||
                                                        pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_P))
      return DYNAMIC_LDA;
    else
      return DEFAULT_LDA;
  }

  return pChParam->eLdaCtrlMode;
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

  assert(0);
  Rtos_ReleaseMutex(pCtx->Mutex);
}

static void releaseSource(AL_TEncCtx* pCtx, AL_TBuffer* pSrc, AL_TFrameInfo* pFI)
{
  RemoveSourceSent(pCtx, pSrc);
  AL_Buffer_Unref(pSrc);

  if(pFI && pFI->pQpTable)
    AL_Buffer_Unref(pFI->pQpTable);

}

/***************************************************************************/
static void AL_sEncoder_DestroySkippedPictureData(AL_TSkippedPicture* pSkipPicture)
{
  Rtos_Free(pSkipPicture->pBuffer);
  pSkipPicture->pBuffer = NULL;
  pSkipPicture->iBufSize = 0;
  pSkipPicture->iNumBits = 0;
  pSkipPicture->iNumBins = 0;
}

/****************************************************************************/
static bool AL_Common_Encoder_InitBuffers(AL_TEncCtx* pCtx, AL_TAllocator* pAllocator, TBufferEP* pBufEP1)
{
  bool bRet = MemDesc_AllocNamed(&pBufEP1->tMD, pAllocator, AL_GetAllocSizeEP1(), "ep1");

  pBufEP1->uFlags = 0;
  pCtx->iCurPool = 0;
  return bRet;
}

static bool init(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TAllocator* pAllocator)
{
  if(!AL_Common_Encoder_InitBuffers(pCtx, pAllocator, &pCtx->tLayerCtx[0].tBufEP1))
    return false;

  AL_SrcBuffersChecker_Init(&pCtx->tLayerCtx[0].srcBufferChecker, pChParam);


  pCtx->iLastIdrId = 0;

  pCtx->seiData.initialCpbRemovalDelay = pChParam->tRCParam.uInitialRemDelay;
  pCtx->seiData.cpbRemovalDelay = 0;

  pCtx->tLayerCtx[0].iCurStreamSent = 0;
  pCtx->tLayerCtx[0].iCurStreamRecv = 0;
  pCtx->iFrameCountDone = 0;

  pCtx->eError = AL_SUCCESS;

  int const iWidthInLcu = (pChParam->uWidth + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;
  int const iHeightInLcu = (pChParam->uHeight + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;

  pCtx->iNumLCU = iWidthInLcu * iHeightInLcu;

  Rtos_Memset(pCtx->Pool, 0, sizeof pCtx->Pool);
  Rtos_Memset(pCtx->SourceSent, 0, sizeof(pCtx->SourceSent));

  pCtx->Mutex = Rtos_CreateMutex();
  assert(pCtx->Mutex);
  return true;
}

/***************************************************************************/
static AL_TEncRequestInfo* getCurrentCommands(AL_TLayerCtx* pCtx)
{
  return &pCtx->currentRequestInfo;
}

/***************************************************************************/
void AL_Common_Encoder_NotifySceneChange(AL_TEncoder* pEnc, int iAhead)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_SCENE_CHANGE;
  pReqInfo->uSceneChangeDelay = iAhead;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyIsLongTerm(AL_TEncoder* pEnc)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pEnc->pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_IS_LONG_TERM;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyUseLongTerm(AL_TEncoder* pEnc)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pEnc->pCtx->tLayerCtx[0]);
  pReqInfo->eReqOptions |= AL_OPT_USE_LONG_TERM;
}




/***************************************************************************/
bool AL_Common_Encoder_PutStreamBuffer(AL_TEncoder* pEnc, AL_TBuffer* pStream, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  assert(pMetaData);
  assert(pCtx);

  AL_StreamMetaData_ClearAllSections(pMetaData);
  Rtos_GetMutex(pCtx->Mutex);
  pCtx->tLayerCtx[iLayerID].StreamSent[pCtx->tLayerCtx[iLayerID].iCurStreamSent] = pStream;
  int curStreamSent = pCtx->tLayerCtx[iLayerID].iCurStreamSent;
  pCtx->tLayerCtx[iLayerID].iCurStreamSent = (pCtx->tLayerCtx[iLayerID].iCurStreamSent + 1) % AL_MAX_STREAM_BUFFER;
  AL_Buffer_Ref(pStream);

  /* Can call AL_Common_Encoder_PutStreamBuffer again */
  int iOffset = ENC_MAX_HEADER_SIZE;
  AL_ISchedulerEnc_PutStreamBuffer(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pStream, curStreamSent, iOffset);
  Rtos_ReleaseMutex(pCtx->Mutex);

  return true;
}

/***************************************************************************/
bool AL_Common_Encoder_GetRecPicture(AL_TEncoder* pEnc, TRecPic* pRecPic, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  assert(pCtx);

  return AL_ISchedulerEnc_GetRecPicture(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pRecPic);
}

/***************************************************************************/
void AL_Common_Encoder_ReleaseRecPicture(AL_TEncoder* pEnc, TRecPic* pRecPic, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  assert(pCtx);

  AL_ISchedulerEnc_ReleaseRecPicture(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pRecPic);
}

void AL_Common_Encoder_ConfigureZapper(AL_TEncCtx* pCtx, AL_TEncInfo* pEncInfo);


/* +1 / -1 business is needed as 0 == NULL is the error value of the fifo
 * this also means that GetNextPoolId returns -1 on error */

static int GetNextPoolId(AL_TFifo* poolIds)
{
  return (int)((intptr_t)AL_Fifo_Dequeue(poolIds, AL_NO_WAIT)) - 1;
}

static void GiveIdBackToPool(AL_TFifo* poolIds, int iPoolID)
{
  AL_Fifo_Queue(poolIds, (void*)((intptr_t)(iPoolID + 1)), AL_NO_WAIT);
}

static void DeinitPoolIds(AL_TEncCtx* pCtx)
{
  AL_Fifo_Deinit(&pCtx->iPoolIds);
}

static bool InitPoolIds(AL_TEncCtx* pCtx)
{
  if(!AL_Fifo_Init(&pCtx->iPoolIds, MAX_NUM_LAYER * ENC_MAX_CMD))
    return false;

  for(int i = 0; i < MAX_NUM_LAYER * ENC_MAX_CMD; ++i)
    GiveIdBackToPool(&pCtx->iPoolIds, i);

  return true;
}

static bool EndOfStream(AL_TEncoder* pEnc, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  return AL_ISchedulerEnc_EncodeOneFrame(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, NULL, NULL, NULL);
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

  assert(0);
  Rtos_ReleaseMutex(pCtx->Mutex);
}

/****************************************************************************/
void AL_Common_Encoder_SetEncodingOptions(AL_TEncCtx* pCtx, AL_TFrameInfo* pFI, int iLayerID)
{
  AL_TEncInfo* pEncInfo = &pFI->tEncInfo;

  if(pCtx->Settings.bForceLoad)
    pEncInfo->eEncOptions |= AL_OPT_FORCE_LOAD;

  if(pCtx->Settings.bDisIntra)
    pEncInfo->eEncOptions |= AL_OPT_DISABLE_INTRA;

  if(pCtx->Settings.bDependentSlice)
    pEncInfo->eEncOptions |= AL_OPT_DEPENDENT_SLICES;

  if(pCtx->Settings.tChParam[iLayerID].uL2PrefetchMemSize)
    pEncInfo->eEncOptions |= AL_OPT_USE_L2;

}

/****************************************************************************/
void AL_Common_Encoder_ProcessLookAheadParam(AL_TEncoder* pEnc, AL_TEncInfo* pEI, AL_TBuffer* pFrame)
{
  // Process first pass informations from the metadata, notifies scene changes and transmits parameters for the RateCtrl
  AL_TLookAheadMetaData* pMetaDataLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_LOOKAHEAD);

  if(pMetaDataLA && pMetaDataLA->bNextSceneChange)
    AL_Common_Encoder_NotifySceneChange(pEnc, 1);


  if(pMetaDataLA && pMetaDataLA->iPictureSize != -1)
  {
    pEI->tLAParam.iSCPictureSize = pMetaDataLA->iPictureSize;
    pEI->tLAParam.iSCIPRatio = pMetaDataLA->iIPRatio;
    pEI->tLAParam.iComplexity = pMetaDataLA->iComplexity;
    pEI->tLAParam.iTargetLevel = pMetaDataLA->iTargetLevel;
  }
}


/***************************************************************************/
bool AL_Common_Encoder_Process(AL_TEncoder* pEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  if(!pFrame)
    return EndOfStream(pEnc, iLayerID);

  if(!AL_SrcBuffersChecker_CanBeUsed(&pCtx->tLayerCtx[iLayerID].srcBufferChecker, pFrame))
    return false;

  AL_Common_Encoder_WaitReadiness(pCtx);
  pCtx->iCurPool = GetNextPoolId(&pCtx->iPoolIds);

  const int AL_DEFAULT_PPS_QP_26 = 26;
  AL_TFrameInfo* pFI = &pCtx->Pool[pCtx->iCurPool];
  AL_TEncInfo* pEI = &pFI->tEncInfo;
  AL_TEncPicBufAddrs addresses = { 0 };
  AL_TSrcMetaData* pMetaData = NULL;

  pEI->UserParam = pCtx->iCurPool;
  pEI->iPpsQP = AL_DEFAULT_PPS_QP_26;

  AL_Common_Encoder_SetEncodingOptions(pCtx, pFI, iLayerID);

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

  addresses.pSrc_Y = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf) + AL_SrcMetaData_GetOffsetY(pMetaData);
  addresses.pSrc_UV = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf) + AL_SrcMetaData_GetOffsetUV(pMetaData);
  addresses.uPitchSrc = pMetaData->tPlanes[AL_PLANE_Y].iPitch;

  AL_TEncChanParam* pChParam = &pCtx->Settings.tChParam[iLayerID];


  if(AL_GetBitDepth(pMetaData->tFourCC) > 8)
    addresses.uPitchSrc = 0x80000000 | addresses.uPitchSrc;

  addresses.uPitchSrc |= (AL_GET_SRC_FMT(pChParam->eSrcMode)) << 28;

  AL_Buffer_Ref(pFrame);
  pEI->SrcHandle = (AL_64U)(uintptr_t)pFrame;
  AddSourceSent(pCtx, pFrame, pFI);

  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[iLayerID]);


  if(pCtx->Settings.LookAhead > 0 || pCtx->Settings.TwoPass == 2)
    AL_Common_Encoder_ProcessLookAheadParam(pEnc, pEI, pFrame);


  bool bRet = AL_ISchedulerEnc_EncodeOneFrame(pCtx->pScheduler, pCtx->tLayerCtx[iLayerID].hChannel, pEI, pReqInfo, &addresses);

  if(!bRet)
    releaseSource(pCtx, pFrame, pFI);

  Rtos_Memset(pReqInfo, 0, sizeof(*pReqInfo));
  Rtos_Memset(pEI, 0, sizeof(*pEI));
  return bRet;
}

/***************************************************************************/
AL_ERR AL_Common_Encoder_GetLastError(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  Rtos_GetMutex(pCtx->Mutex);
  AL_ERR eError = pEnc->pCtx->eError;
  Rtos_ReleaseMutex(pCtx->Mutex);

  return eError;
}

static void setMaxNumRef(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  pCtx->iMaxNumRef = AL_GetNumberOfRef(pChParam->uPpsParam);
}

void AL_Common_Encoder_SetHlsParam(AL_TEncChanParam* pChParam)
{
  if(pChParam->uCabacInitIdc)
    pChParam->uPpsParam |= AL_PPS_CABAC_INIT_PRES_FLAG;
}

/***************************************************************************/
void AL_Common_SetError(AL_TEncCtx* pCtx, AL_ERR eErrorCode)
{
  Rtos_GetMutex(pCtx->Mutex);
  pCtx->eError = eErrorCode;
  Rtos_ReleaseMutex(pCtx->Mutex);
}


/***************************************************************************/
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

static void SetGoldenRefFrequency(AL_TEncChanParam* pChParam)
{
  pChParam->tGopParam.uFreqGoldenRef = 0;

  if(pChParam->eLdaCtrlMode == DYNAMIC_LDA || pChParam->tRCParam.bUseGoldenRef)
  {
    switch(pChParam->tGopParam.eMode)
    {
    case AL_GOP_MODE_DEFAULT:

      if(pChParam->tGopParam.uNumB)
        pChParam->tGopParam.uFreqGoldenRef = 0;
      else
      {
        if(pChParam->tRCParam.uGoldenRefFrequency < -1)
          pChParam->tGopParam.uFreqGoldenRef = 4;
        else
          pChParam->tGopParam.uFreqGoldenRef = pChParam->tRCParam.uGoldenRefFrequency;
      }
      break;
    case AL_GOP_MODE_LOW_DELAY_P:

      if(pChParam->tRCParam.uGoldenRefFrequency < -1)
        pChParam->tGopParam.uFreqGoldenRef = 5;
      else
        pChParam->tGopParam.uFreqGoldenRef = pChParam->tRCParam.uGoldenRefFrequency;
      break;
    case AL_GOP_MODE_LOW_DELAY_B:
    case AL_GOP_MODE_PYRAMIDAL:
    default:
      pChParam->tGopParam.uFreqGoldenRef = 0;
      break;
    }
  }
}

/****************************************************************************/
static AL_TEncChanParam* TransferChannelParameters(AL_TEncSettings const* pSettings, AL_TEncChanParam* pChParamOut)
{

  pChParamOut->uClipHrzRange = (pChParamOut->eOptions & AL_OPT_FORCE_MV_CLIP) ? pSettings->uClipHrzRange : 0;
  pChParamOut->uClipVrtRange = (pChParamOut->eOptions & AL_OPT_FORCE_MV_CLIP) ? pSettings->uClipVrtRange : 0;

  pChParamOut->uL2PrefetchMemSize = pSettings->iPrefetchLevel2;
  pChParamOut->uL2PrefetchMemOffset = 0;

  // Update Auto QP param -------------------------------------------
  if(pSettings->eQpCtrlMode & MASK_AUTO_QP)
  {
    pChParamOut->eOptions |= AL_OPT_ENABLE_AUTO_QP;

    if(pSettings->eQpCtrlMode & ADAPTIVE_AUTO_QP)
      pChParamOut->eOptions |= AL_OPT_ADAPT_AUTO_QP;
  }

  // Update QP table param -------------------------------------------
  if(pSettings->eQpCtrlMode & RELATIVE_QP)
    pChParamOut->eOptions |= AL_OPT_QP_TAB_RELATIVE;

  pChParamOut->eLdaCtrlMode = GetFinalLdaMode(pChParamOut);
  pChParamOut->eOptions |= AL_OPT_CUSTOM_LDA;

  SetGoldenRefFrequency(pChParamOut);
  return pChParamOut;
}

/****************************************************************************/
static AL_TEncChanParam* initChannelParam(AL_TEncCtx* pCtx, AL_TEncSettings const* pSettings)
{
  // Keep Settings and source video information -----------------------
  pCtx->Settings = *pSettings;
  return TransferChannelParameters(pSettings, &pCtx->Settings.tChParam[0]);
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

static void AL_Common_Encoder_DeinitBuffers(AL_TLayerCtx* pCtx)
{
  MemDesc_Free(&pCtx->tBufEP1.tMD);
}


/****************************************************************************/
static bool PreprocessEncoderParam(AL_TEncCtx* pCtx, TBufferEP* pEP1, int iLayerID)
{
  AL_TEncSettings* pSettings = &pCtx->Settings;
  AL_TEncChanParam* pChParam = &pSettings->tChParam[iLayerID];

  pEP1->uFlags = 0;
  AL_CleanupMemory(pEP1->tMD.pVirtualAddr, pEP1->tMD.uSize);

  AL_ELdaCtrlMode* eLdaMode = &pChParam->eLdaCtrlMode;

  if(*eLdaMode == LOAD_LDA)
  {
    char const* ldaFilename = DEBUG_PATH "/Lambdas.hex";

    if(!LoadLambdaFromFile(ldaFilename, pEP1))
      return false;
  }
  /* deprecation warning: custom lda are used for hw tests and will be passed
   * via a file in the future.*/
  else if(*eLdaMode == CUSTOM_LDA)
  {
    LoadCustomLda(pEP1);
    *eLdaMode = LOAD_LDA;
  }


  pCtx->encoder.preprocessEp1(pCtx, pEP1);
  return true;
}

static void releaseStreams(AL_TEncCtx* pCtx, int iLayerID)
{
  for(int streamId = pCtx->tLayerCtx[iLayerID].iCurStreamRecv; streamId != pCtx->tLayerCtx[iLayerID].iCurStreamSent; streamId = (streamId + 1) % AL_MAX_STREAM_BUFFER)
  {
    AL_TBuffer* pStream = pCtx->tLayerCtx[iLayerID].StreamSent[streamId];
    pCtx->tLayerCtx[iLayerID].callback.func(pCtx->tLayerCtx[iLayerID].callback.userParam, pStream, NULL, iLayerID);
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
      pCtx->tLayerCtx[iLayerID].callback.func(pCtx->tLayerCtx[iLayerID].callback.userParam, NULL, pSource, iLayerID);
      releaseSource(pCtx, pSource, pFrameCtx->pFI);
    }
  }
}

/***************************************************************************/
static void destroy(AL_TEncCtx* pCtx)
{
  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    AL_ISchedulerEnc_DestroyChannel(pCtx->pScheduler, pCtx->tLayerCtx[i].hChannel);
    releaseStreams(pCtx, i);
    releaseSources(pCtx, i);
  }

  Rtos_DeleteMutex(pCtx->Mutex);
  Rtos_DeleteSemaphore(pCtx->PendingEncodings);

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    AL_sEncoder_DestroySkippedPictureData(&pCtx->tLayerCtx[i].pSkippedPicture);
    AL_Common_Encoder_DeinitBuffers(&pCtx->tLayerCtx[i]);
  }

  DeinitPoolIds(pCtx);
}

/***************************************************************************/
void AL_Common_Encoder_Destroy(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  destroy(pCtx);
  Rtos_Free(pCtx);
}

#define AL_RETURN_ERROR(e) { AL_Common_SetError(pCtx, e); return false; }

/****************************************************************************/
bool AL_Common_Encoder_RestartGop(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    if(pCtx->Settings.tChParam[i].tGopParam.eMode != AL_GOP_MODE_DEFAULT &&
       pCtx->Settings.tChParam[i].tGopParam.eMode != AL_GOP_MODE_LOW_DELAY_P &&
       pCtx->Settings.tChParam[i].tGopParam.eMode != AL_GOP_MODE_LOW_DELAY_B)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_RESTART_GOP;
  }

  return true;
}

/****************************************************************************/
static void setNewParams(AL_TEncCtx* pCtx, int iLayerID)
{
  AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[iLayerID]);
  pReqInfo->eReqOptions |= AL_OPT_UPDATE_PARAMS;

  pReqInfo->smartParams.rc = pCtx->Settings.tChParam[iLayerID].tRCParam;
  pReqInfo->smartParams.gop = pCtx->Settings.tChParam[iLayerID].tGopParam;
}

/****************************************************************************/
bool AL_Common_Encoder_SetGopLength(AL_TEncoder* pEnc, int iGopLength)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    if(pCtx->Settings.tChParam[i].tGopParam.eMode != AL_GOP_MODE_DEFAULT)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    pCtx->Settings.tChParam[i].tGopParam.uGopLength = iGopLength;
    setNewParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetGopNumB(AL_TEncoder* pEnc, int iNumB)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    if(pCtx->Settings.tChParam[i].tGopParam.eMode != AL_GOP_MODE_DEFAULT)
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);

    if(iNumB > pCtx->iInitialNumB)
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

    pCtx->Settings.tChParam[i].tGopParam.uNumB = iNumB;
    setNewParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetBitRate(AL_TEncoder* pEnc, int iBitRate, int iLayerID)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  pCtx->Settings.tChParam[iLayerID].tRCParam.uTargetBitRate = iBitRate;

  if(pCtx->Settings.tChParam[iLayerID].tRCParam.eRCMode == AL_RC_CBR)
    pCtx->Settings.tChParam[iLayerID].tRCParam.uMaxBitRate = iBitRate;
  setNewParams(pCtx, iLayerID);

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetFrameRate(AL_TEncoder* pEnc, uint16_t uFrameRate, uint16_t uClkRatio)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  if(uFrameRate > pCtx->uInitialFrameRate)
    AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    pCtx->Settings.tChParam[i].tRCParam.uFrameRate = uFrameRate;
    pCtx->Settings.tChParam[i].tRCParam.uClkRatio = uClkRatio;
    setNewParams(pCtx, i);
  }

  return true;
}

/****************************************************************************/
bool AL_Common_Encoder_SetQP(AL_TEncoder* pEnc, int16_t iQP)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  for(int i = 0; i < pCtx->Settings.NumLayer; ++i)
  {
    if(pCtx->Settings.tChParam[i].tRCParam.eRCMode != AL_RC_BYPASS &&
       pCtx->Settings.tChParam[i].tRCParam.eRCMode != AL_RC_CBR &&
       pCtx->Settings.tChParam[i].tRCParam.eRCMode != AL_RC_VBR)
    {
      AL_RETURN_ERROR(AL_ERR_CMD_NOT_ALLOWED);
    }

    if(iQP < pCtx->Settings.tChParam[i].tRCParam.iMinQP || iQP > pCtx->Settings.tChParam[i].tRCParam.iMaxQP)
    {
      AL_RETURN_ERROR(AL_ERR_INVALID_CMD_VALUE);
    }

    AL_TEncRequestInfo* pReqInfo = getCurrentCommands(&pCtx->tLayerCtx[i]);
    pReqInfo->eReqOptions |= AL_OPT_SET_QP;
    pReqInfo->smartParams.iQPSet = iQP;
  }

  return true;
}


static bool isSeiEnable(uint32_t uFlags)
{
  return uFlags != SEI_NONE;
}

static bool isBaseLayer(int iLayer)
{
  return iLayer == 0;
}

NalsData AL_ExtractNalsData(AL_TEncCtx* pCtx, int iLayerID)
{
  NalsData data = { 0 };
  data.vps = &pCtx->vps;
  data.sps[0] = &pCtx->tLayerCtx[0].sps;
  data.pps[0] = &pCtx->tLayerCtx[0].pps;

  AL_TEncSettings const* pSettings = &pCtx->Settings;

  data.shouldWriteAud = pSettings->bEnableAUD && isBaseLayer(iLayerID);
  data.shouldWriteFillerData = pSettings->bEnableFillerData;
  data.seiFlags = pSettings->uEnableSEI;


  if(pSettings->tChParam[0].bSubframeLatency)
    data.seiFlags |= SEI_EOF;

  if(isSeiEnable(data.seiFlags))
    data.seiData = &pCtx->seiData;

  return data;
}

/****************************************************************************/
void AL_Common_Encoder_InitSkippedPicture(AL_TSkippedPicture* pSkipPicture)
{
  pSkipPicture->pBuffer = (uint8_t*)Rtos_Malloc(2 * 1024);

  assert(pSkipPicture->pBuffer);

  pSkipPicture->iBufSize = 2 * 1024;
  pSkipPicture->iNumBits = 0;
  pSkipPicture->iNumBins = 0;
}

/****************************************************************************/
static void EndEncoding(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_64U streamUserPtr)
{
  AL_TCbUserParam* pCbUserParam = (AL_TCbUserParam*)pUserParam;
  AL_TEncCtx* pCtx = pCbUserParam->pCtx;
  int iLayerID = pCbUserParam->iLayerID;

  if(!pPicStatus)
  {
    pCtx->tLayerCtx[iLayerID].callback.func(pCtx->tLayerCtx[iLayerID].callback.userParam, NULL, NULL, iLayerID);
    return;
  }
  int streamId = (int)streamUserPtr;

  /* we require the stream to come back in the same order we sent them */
  assert(streamId >= 0 && streamId < AL_MAX_STREAM_BUFFER);
  assert(pCtx->tLayerCtx[iLayerID].iCurStreamRecv == streamId);

  pCtx->tLayerCtx[iLayerID].iCurStreamRecv = (pCtx->tLayerCtx[iLayerID].iCurStreamRecv + 1) % AL_MAX_STREAM_BUFFER;

  AL_Common_SetError(pCtx, pPicStatus->eErrorCode);

  AL_TBuffer* pStream = pCtx->tLayerCtx[iLayerID].StreamSent[streamId];

  if(!(pPicStatus->eErrorCode & AL_ERROR || pPicStatus->bSkip))
    pCtx->encoder.updateHlsAndWriteSections(pCtx, pPicStatus, pStream, iLayerID);

  AL_TPictureMetaData* pPictureMeta = (AL_TPictureMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_PICTURE);

  if(pPictureMeta)
    pPictureMeta->eType = pPicStatus->eType;

  int iPoolID = pPicStatus->UserParam;
  AL_TFrameInfo* pFI = &pCtx->Pool[iPoolID];

  AL_TBuffer* pSrc = (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;


  if(pCtx->Settings.LookAhead > 0 || pCtx->Settings.TwoPass == 1)
  {
    // Transmits first pass information in the metadata
    AL_TLookAheadMetaData* pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_LOOKAHEAD);

    if(pPictureMetaLA)
    {
      pPictureMetaLA->iPictureSize = pPicStatus->iPictureSize;
      pPictureMetaLA->iPercentIntra = pPicStatus->iPercentIntra;
      pPictureMetaLA->iPercentSkip = pPicStatus->iPercentSkip;
    }
  }

  pCtx->tLayerCtx[iLayerID].callback.func(pCtx->tLayerCtx[iLayerID].callback.userParam, pStream, pSrc, iLayerID);

  if(pPicStatus->bIsLastSlice)
  {
    if(pCtx->encoder.shouldReleaseSource(pPicStatus))
      releaseSource(pCtx, pSrc, pFI);

    Rtos_GetMutex(pCtx->Mutex);
    ++pCtx->iFrameCountDone;

    if(pCtx->encoder.shouldReleaseSource(pPicStatus))
      GiveIdBackToPool(&pCtx->iPoolIds, iPoolID);
    Rtos_ReleaseMutex(pCtx->Mutex);

    Rtos_ReleaseSemaphore(pCtx->PendingEncodings);
  }

  Rtos_GetMutex(pCtx->Mutex);
  AL_Buffer_Unref(pStream);
  Rtos_ReleaseMutex(pCtx->Mutex);
}

/****************************************************************************/
AL_ERR AL_Common_Encoder_CreateChannel(AL_TEncCtx* pCtx, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings)
{
  AL_TEncChanParam* pChParam = initChannelParam(pCtx, pSettings);
  AL_ERR errorCode = AL_ERROR;

  pCtx->pScheduler = pScheduler;

  if(!init(pCtx, pChParam, pAlloc))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  if(!InitPoolIds(pCtx))
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail;
  }

  pCtx->encoder.configureChannel(pCtx, pChParam, pSettings);

  pCtx->tLayerCtx[0].callback_user_param.pCtx = pCtx;
  pCtx->tLayerCtx[0].callback_user_param.iLayerID = 0;

  AL_TISchedulerCallBacks CBs = { 0 };
  CBs.pfnEndEncodingCallBack = EndEncoding;
  CBs.pEndEncodingCBParam = &pCtx->tLayerCtx[0].callback_user_param;

  // HACK: needed to preprocess scaling list, but doesn't generate the good nals
  // because we are missing some value populated by AL_ISchedulerEnc_CreateChannel
  pCtx->encoder.generateNals(pCtx, 0, true);

  if(!PreprocessEncoderParam(pCtx, &pCtx->tLayerCtx[0].tBufEP1, 0))
    goto fail;

  AL_ERR chanError = AL_ISchedulerEnc_CreateChannel(&pCtx->tLayerCtx[0].hChannel, pCtx->pScheduler, pChParam, &pCtx->tLayerCtx[0].tBufEP1.tMD, &CBs);

  if(chanError != AL_SUCCESS)
  {
    errorCode = chanError;
    goto fail;
  }


  pCtx->encoder.generateSkippedPictureData(pCtx, pChParam, &pCtx->tLayerCtx[0].pSkippedPicture);
  pCtx->PendingEncodings = Rtos_CreateSemaphore(ENC_MAX_CMD - 1);

  setMaxNumRef(pCtx, pChParam);
  pCtx->encoder.generateNals(pCtx, 0, true);

  pCtx->iInitialNumB = pChParam->tGopParam.uNumB;
  pCtx->uInitialFrameRate = pChParam->tRCParam.uFrameRate;

  return AL_SUCCESS;

  fail:
  destroy(pCtx);
  return errorCode;
}


Nuts CreateAvcNuts(void);
Nuts CreateHevcNuts(void);
bool CreateNuts(Nuts* nuts, AL_EProfile eProfile)
{
  if(AL_IS_AVC(eProfile))
  {
    *nuts = CreateAvcNuts();
    /* sei suffix do not really exist in AVC. use a prefix nut */
    nuts->seiSuffixNut = nuts->seiPrefixNut;
  }
  else if(AL_IS_HEVC(eProfile))
    *nuts = CreateHevcNuts();
  else
    return false;
  return true;
}

#include "lib_encode/Sections.h"

int AL_Encoder_AddSei(AL_HEncoder hEnc, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_TEncCtx* pCtx = pEnc->pCtx;

  Nuts nuts;
  bool exists = CreateNuts(&nuts, pCtx->Settings.tChParam[0].eProfile);

  if(!exists)
    return -1;
  return AL_WriteSeiSection(nuts, pStream, isPrefix, iPayloadType, pPayload, iPayloadSize);
}

