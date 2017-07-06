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
#include "lib_common_enc/IpEncFourCC.h"
#include <assert.h>
#include "lib_common/Utils.h"


/***************************************************************************/
bool AL_Common_Encoder_WaitReadiness(AL_TEncCtx* pCtx, uint32_t uTimeOut)
{
#if ENABLE_RTOS_SYNC
  return AL_IP_ENCODER_GET_SEM(pCtx, uTimeOut);
#else
  return pCtx->m_uNumBufStreamUsed >= ENC_MAX_CMD;
#endif
}

/***************************************************************************/
void AL_Common_Encoder_EndEncoding(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TBuffer const* const pSrc)
{
  if(pCtx->m_callback.func)
    (*pCtx->m_callback.func)(pCtx->m_callback.userParam, pStream, pSrc);
}

/***************************************************************************/
static void AL_sEncoder_DestroySkippedPictureData(AL_TEncCtx* pCtx)
{
  if(pCtx->m_pSkippedPicture.pBuffer)
  {
    Rtos_Free(pCtx->m_pSkippedPicture.pBuffer);
    pCtx->m_pSkippedPicture.pBuffer = NULL;
    pCtx->m_pSkippedPicture.iBufSize = 0;
    pCtx->m_pSkippedPicture.iNumBits = 0;
    pCtx->m_pSkippedPicture.iNumBins = 0;
  }
}

/****************************************************************************/
static void AL_sEncoder_DefaultEndEncodingCB(void* pUserParam, AL_TBuffer* pStream, AL_TBuffer const* const pSrc)
{
  (void)pSrc;
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;
  assert(pCtx);

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);

  AL_TEncoder enc;
  enc.pCtx = pCtx;

  if(AL_Common_Encoder_PutStreamBuffer(&enc, pStream))
    AL_Buffer_Unref(pStream);

  AL_IP_ENCODER_END_MUTEX(pCtx);
}

/***************************************************************************/
/*                           Lib functions                                 */
/***************************************************************************/

void AL_Common_Encoder_InitCtx(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TAllocator* pAllocator)
{
  AL_Common_Encoder_InitBuffers(pCtx, pAllocator);

  // default callback
  pCtx->m_callback.func = AL_sEncoder_DefaultEndEncodingCB;
  pCtx->m_callback.userParam = pCtx;

  AL_Common_Encoder_InitRbspEncoder(pCtx);


  pCtx->m_iLastIdrId = 0;

  pCtx->m_uCpbRemovalDelay = 0;

  pCtx->m_iFrameCountDone = 0;
  pCtx->m_iFrameCountSent = 0;

  pCtx->m_eError = AL_SUCCESS;

  int const iWidthInLcu = (pChParam->uWidth + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;
  int const iHeightInLcu = (pChParam->uHeight + ((1 << pChParam->uMaxCuSize) - 1)) >> pChParam->uMaxCuSize;

  pCtx->m_iNumLCU = iWidthInLcu * iHeightInLcu;
  pCtx->m_uInitialCpbRemovalDelay = pChParam->tRCParam.uInitialRemDelay;

  Rtos_Memset(pCtx->m_Pool, 0, sizeof pCtx->m_Pool);

#if ENABLE_RTOS_SYNC
  pCtx->m_Mutex = Rtos_CreateMutex(false);
  assert(pCtx->m_Mutex);
#endif
}

#if ENABLE_WATCHDOG
void AL_Common_Encoder_SetWatchdogCB(AL_TISchedulerCallBacks* CBs, const AL_TEncSettings* pSettings)
{
  if(pSettings->bEnableWatchdog)
    CBs->pfnWatchdogCallBack = AL_Common_Encoder_Watchdog_Catch;
  else
    CBs->pfnWatchdogCallBack = NULL;
}

#endif

/***************************************************************************/
void AL_Common_Encoder_InitRbspEncoder(AL_TEncCtx* pCtx)
{
  pCtx->m_pHdrBuf = Rtos_Malloc(ENC_MAX_HEADER_SIZE);
  assert(pCtx->m_pHdrBuf);

  AL_BitStreamLite_Init(&pCtx->m_BS, pCtx->m_pHdrBuf, ENC_MAX_HEADER_SIZE);
  AL_RbspEncoding_Init(&pCtx->m_RE, &pCtx->m_BS);
}

/***************************************************************************/
void AL_Common_Encoder_DeInitRbspEncoder(AL_TEncCtx* pCtx)
{
  AL_RbspEncoding_Deinit(&pCtx->m_RE);
  AL_BitStreamLite_Deinit(&pCtx->m_BS);
  Rtos_Free(pCtx->m_pHdrBuf);
}

/***************************************************************************/
void AL_Common_Encoder_NotifySceneChange(AL_TEncoder* pEnc, int uAhead)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[pCtx->m_iCurPool];
  AL_TEncRequestInfo* pReqInfo = &pFI->tRequestInfo;
  pReqInfo->eReqOptions |= AL_OPT_SCENE_CHANGE;
  pReqInfo->uSceneChangeDelay = uAhead;
}

/***************************************************************************/
void AL_Common_Encoder_NotifyLongTerm(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[pCtx->m_iCurPool];
  AL_TEncRequestInfo* pReqInfo = &pFI->tRequestInfo;
  pReqInfo->eReqOptions |= AL_OPT_USE_LONG_TERM;
}


/***************************************************************************/
bool AL_Common_Encoder_PutStreamBuffer(AL_TEncoder* pEnc, AL_TBuffer* pStream)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  assert(pMetaData);
  assert(pCtx);

  ClearAllSections(pStream);
  pMetaData->uMaxSize = AL_Buffer_GetSizeData(pStream);
  pMetaData->uAvailSize = pMetaData->uMaxSize;
  pMetaData->uOffset = 0;

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);
  bool bRet = AL_ISchedulerEnc_PutStreamBuffer(pCtx->m_pScheduler, pCtx->m_hChannel, pStream, ENC_MAX_HEADER_SIZE);

  if(bRet)
    AL_Buffer_Ref(pStream);
  AL_IP_ENCODER_END_MUTEX(pCtx);

  return bRet;
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

/***************************************************************************/
bool AL_Common_Encoder_Process(AL_TEncoder* pEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  if(!pFrame)
    return AL_ISchedulerEnc_EncodeOneFrame(pCtx->m_pScheduler, pCtx->m_hChannel, NULL, NULL, NULL);

  const int AL_DEFAULT_PPS_QP_26 = 26;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[pCtx->m_iCurPool];
  AL_TEncRequestInfo* pReqInfo = &pFI->tRequestInfo;
  AL_TEncInfo* pEI = &pFI->tEncInfo;
  AL_TEncPicBufAddrs* pBufAddrs = &pFI->tBufAddrs;
  AL_TSrcMetaData* pMetaData = NULL;
  bool bReadiness = AL_Common_Encoder_WaitReadiness(pCtx, AL_WAIT_FOREVER);
  assert(bReadiness);

  pEI->UserParam = pCtx->m_iCurPool;
  pEI->iPpsQP = AL_DEFAULT_PPS_QP_26;

  if(pCtx->m_Settings.bForceLoad)
    pEI->eEncOptions |= AL_OPT_FORCE_LOAD;

  if(pCtx->m_Settings.bDisIntra)
    pEI->eEncOptions |= AL_OPT_DISABLE_INTRA;

  if(pCtx->m_Settings.bDependentSlice)
    pEI->eEncOptions |= AL_OPT_DEPENDENT_SLICES;

  if(pCtx->m_Settings.tChParam.uL2CacheMemSize)
    pEI->eEncOptions |= AL_OPT_USE_L2_CACHE;
  ++pCtx->m_iFrameCountSent;

  pMetaData = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_SOURCE);

  pFI->pQpTable = pQpTable;

  if(pQpTable)
  {
    AL_Buffer_Ref(pQpTable);
    pBufAddrs->pEP2 = AL_Allocator_GetPhysicalAddr(pQpTable->pAllocator, pQpTable->hBuf);
    pBufAddrs->pEP2_v = (AL_PTR64)(uintptr_t)AL_Allocator_GetVirtualAddr(pQpTable->pAllocator, pQpTable->hBuf);
    pEI->eEncOptions |= AL_OPT_USE_QP_TABLE;
  }
  else
  {
    pBufAddrs->pEP2_v = 0;
    pBufAddrs->pEP2 = 0;
  }

  pBufAddrs->pSrc_Y = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf);
  pBufAddrs->pSrc_UV = AL_Allocator_GetPhysicalAddr(pFrame->pAllocator, pFrame->hBuf) + AL_SrcMetaData_GetOffsetC(pMetaData);
  pBufAddrs->uPitchSrc = pMetaData->tPitches.iLuma;

  if(AL_GetBitDepth(pMetaData->tFourCC) > 8)
    pBufAddrs->uPitchSrc = 0x80000000 | pBufAddrs->uPitchSrc;

  AL_Buffer_Ref(pFrame);
  pEI->SrcHandle = (AL_64U)(uintptr_t)pFrame;


  pCtx->m_iCurPool = (pCtx->m_iCurPool + 1) % ENC_MAX_CMD;

  bool bRet = AL_ISchedulerEnc_EncodeOneFrame(pCtx->m_pScheduler, pCtx->m_hChannel, pEI, pReqInfo, pBufAddrs);

  Rtos_Memset(pReqInfo, 0, sizeof(*pReqInfo));
  Rtos_Memset(pEI, 0, sizeof(*pEI));
  return bRet;
}

/***************************************************************************/
AL_ERR AL_Common_Encoder_GetLastError(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;

  AL_ERR eError;

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);
  eError = pEnc->pCtx->m_eError;
  AL_IP_ENCODER_END_MUTEX(pCtx);

  return eError;
}

void AL_Common_Encoder_SetMaxNumRef(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
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

  int i;

  for(i = 0; i < MAX_IDX_BIT_PER_PEL; i++)
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
AL_TEncChanParam* AL_Common_Encoder_InitChannelParam(AL_TEncCtx* pCtx, AL_TEncSettings const* pSettings)
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
  if(pSettings->iCacheLevel2 > 0)
  {
    pChParam->bL2CacheAuto = true;
    pChParam->uL2CacheMemSize = pSettings->iCacheLevel2;
  }
  else
  {
    pChParam->bL2CacheAuto = false;
    pChParam->uL2CacheMemSize = 0;
  }
  pChParam->uL2CacheMemOffset = 0;

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

/***************************************************************************/
void AL_Common_Encoder_InitBuffers(AL_TEncCtx* pCtx, AL_TAllocator* pAllocator)
{
  bool bRet = MemDesc_Alloc(&pCtx->m_tBufEP1.tMD, pAllocator, GetAllocSizeEP1());
  assert(bRet);

  pCtx->m_tBufEP1.uFlags = 0;
  pCtx->m_iCurPool = 0;
}

void AL_Common_Encoder_DeinitBuffers(AL_TEncCtx* pCtx)
{
  MemDesc_Free(&pCtx->m_tBufEP1.tMD);
}

/***************************************************************************/
void AL_Common_Encoder_DestroyCtx(AL_TEncCtx* pCtx)
{
  if(pCtx->m_hChannel != AL_INVALID_CHANNEL)
    AL_ISchedulerEnc_DestroyChannel(pCtx->m_pScheduler, pCtx->m_hChannel);

#if ENABLE_RTOS_SYNC
  Rtos_DeleteMutex(pCtx->m_Mutex);
  Rtos_DeleteSemaphore(pCtx->m_Semaphore);
#endif

  AL_sEncoder_DestroySkippedPictureData(pCtx);

  AL_Common_Encoder_DeInitRbspEncoder(pCtx);
  AL_Common_Encoder_DeinitBuffers(pCtx);

  Rtos_Free(pCtx);
}

/***************************************************************************/
void AL_Common_Encoder_Destroy(AL_TEncoder* pEnc)
{
  AL_TEncCtx* pCtx = pEnc->pCtx;
  AL_Common_Encoder_DestroyCtx(pCtx);
}

/*****************************************************************************/
uint32_t AL_Common_Encoder_AddFillerData(AL_TBuffer* pStream, uint32_t* pOffset, uint16_t uSectionID, uint32_t uNumBytes, bool bAVC)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  uint32_t uSize = (uNumBytes < 5) ? 5 : uNumBytes;
  uint32_t uAvailSize = pMetaData->uAvailSize - *pOffset;

  if(uSize <= uAvailSize)
  {
    uint32_t uOffset = (pMetaData->uOffset + *pOffset) % pMetaData->uMaxSize;
    uint32_t uRemSize = pMetaData->uMaxSize - uOffset;
    uint32_t uTmpSize = uSize - 5;

    uint8_t* pBuf = pStream->pData + uOffset;

    if(uRemSize < 4)
    {
      pBuf = pStream->pData;
      uAvailSize -= uRemSize;
      *pOffset += uRemSize;
      assert(uSize <= (uAvailSize - uRemSize));
    }

    // Write NAL unit type
    *pBuf++ = 0x00;
    *pBuf++ = 0x00;
    *pBuf++ = 0x01;
    *pBuf++ = (bAVC ? AL_AVC_NUT_FD : AL_HEVC_NUT_FD) << 1;
    *pBuf++ = 0x01;

    uOffset = (pMetaData->uOffset + *pOffset) % pMetaData->uMaxSize;
    uRemSize = pMetaData->uMaxSize - uOffset - 5;

    // Write filler data
    if(uRemSize <= uTmpSize)
    {
      uTmpSize -= uRemSize;

      while(uRemSize-- > 0)
        *pBuf++ = 0xff;

      pBuf = pStream->pData;
    }

    while(uTmpSize-- > 0)
      *pBuf++ = 0xff;

    // Write trailing bits
    *pBuf = 0x80;

    ChangeSection(pStream, uSectionID, uOffset, uSize);
    SetSectionFlags(pStream, uSectionID, SECTION_COMPLETE_FLAG);

    *pOffset += uSize;
  }
  return uSize;
}


#if ENABLE_WATCHDOG
/***************************************************************************/
void AL_Common_Encoder_Watchdog_Catch(void* pUserParam)
{
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;
  pCtx->m_eError = AL_ERR_WATCHDOG_TIMEOUT;
}

#endif

