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

/****************************************************************************
   -----------------------------------------------------------------------------
**************************************************************************//*!
   \addtogroup lib_encode
   @{
****************************************************************************/
#include "Com_Encoder.h"
#include "lib_common/StreamSection.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/Utils.h"

/***************************************************************************/
static void GenerateSkippedPictureData(AL_TEncCtx* pCtx)
{
  pCtx->m_pSkippedPicture.pBuffer = (uint8_t*)Rtos_Malloc(2 * 1024);

  assert(pCtx->m_pSkippedPicture.pBuffer);

  pCtx->m_pSkippedPicture.iBufSize = 2 * 1024;
  pCtx->m_pSkippedPicture.iNumBits = 0;
  pCtx->m_pSkippedPicture.iNumBins = 0;

  AL_AVC_GenerateSkippedPicture(&(pCtx->m_pSkippedPicture), pCtx->m_iNumLCU, true, 0);
}

/***************************************************************************/
static void WriteAUD(AL_TEncCtx* pCtx, AL_ESliceType eSliceType)
{
  int primary_pic_type;

  if((eSliceType == SLICE_P) || (eSliceType == SLICE_SKIP))
    primary_pic_type = 1; // The access unit will only contain I or P slices.
  else if(eSliceType == SLICE_B)
    primary_pic_type = 2; // The access unit will only contain I or P or B slices.
  else
  {
    assert(eSliceType == SLICE_I);
    primary_pic_type = 0; // The access unit will only contain I slices.
  }

  // Write an access unit delimiter
  AL_RbspEncoding_Reset(&pCtx->m_RE);

  AL_AVC_RbspEncoding_WriteAUD(&pCtx->m_RE, primary_pic_type);

  FlushNAL(&pCtx->m_Stream, AL_AVC_NUT_AUD, 0, 0,
           AL_RbspEncoding_GetData(&pCtx->m_RE),
           AL_RbspEncoding_GetBitsCount(&pCtx->m_RE), true, 0, true);
}

/****************************************************************************/
static void WriteSPS(AL_TEncCtx* pCtx)
{
  AL_RbspEncoding_Reset(&pCtx->m_RE);

  AL_AVC_RbspEncoding_WriteSPS(&pCtx->m_RE, &pCtx->m_AvcSPS);

  FlushNAL(&pCtx->m_Stream, AL_AVC_NUT_SPS, 0, 0,
           AL_RbspEncoding_GetData(&pCtx->m_RE),
           AL_RbspEncoding_GetBitsCount(&pCtx->m_RE), true, 1, true);
}

/****************************************************************************/
static void WritePPS(AL_TEncCtx* pCtx)
{
  AL_RbspEncoding_Reset(&pCtx->m_RE);

  AL_AVC_RbspEncoding_WritePPS(&pCtx->m_RE, &pCtx->m_AvcPPS);

  FlushNAL(&pCtx->m_Stream, AL_AVC_NUT_PPS, 0, 0,
           AL_RbspEncoding_GetData(&pCtx->m_RE),
           AL_RbspEncoding_GetBitsCount(&pCtx->m_RE), true, 1, true);
}

/****************************************************************************/
static void WriteSEI(AL_TEncCtx* pCtx, uint32_t uSeiFlags, bool bSeparated, AL_TEncPicStatus const* pPicStatus)
{
  int iOldBitsCount = 0;

  if(!uSeiFlags)
    return;

  AL_RbspEncoding_Reset(&pCtx->m_RE);

  while(uSeiFlags)
  {
    if(uSeiFlags & SEI_BP)
    {
      AL_AVC_RbspEncoding_WriteSEI_BP(&pCtx->m_RE, &pCtx->m_AvcSPS, pCtx->m_uInitialCpbRemovalDelay);
      uSeiFlags &= ~SEI_BP;
    }
    else if(uSeiFlags & SEI_RP)
    {
      AL_AVC_RbspEncoding_WriteSEI_RP(&pCtx->m_RE);
      uSeiFlags &= ~SEI_RP;
    }
    else if(uSeiFlags & SEI_PT)
    {
      AL_AVC_RbspEncoding_WriteSEI_PT(&pCtx->m_RE, &pCtx->m_AvcSPS,
                                      pCtx->m_uCpbRemovalDelay,
                                      pPicStatus->uDpbOutputDelay, pPicStatus->ePicStruct);
      uSeiFlags &= ~SEI_PT;
    }

    if(bSeparated || !uSeiFlags)
    {
      int iBitsCount;
      int iOffset = (iOldBitsCount + 7) / 8;

      AL_AVC_RbspEncoding_CloseSEI(&pCtx->m_RE);

      iBitsCount = AL_RbspEncoding_GetBitsCount(&pCtx->m_RE);

      FlushNAL(&pCtx->m_Stream, 6, 0, 0,
               AL_RbspEncoding_GetData(&pCtx->m_RE) + iOffset,
               iBitsCount - iOldBitsCount, true, 0, true);
      iOldBitsCount = iBitsCount;
    }
  }
}

/***************************************************************************/
static uint32_t GenerateAUD(AL_TEncCtx* pCtx, AL_TBuffer* pStream, uint32_t const uTotalSize, uint32_t const uFlags, AL_ESliceType const eType)
{
  WriteAUD(pCtx, eType);
  uint32_t uSize = StreamGetNumBytes(&pCtx->m_Stream) - uTotalSize;
  AddSection(pStream, uTotalSize, uSize, uFlags);
  return uSize;
}

/***************************************************************************/
static uint32_t GenerateSPS(AL_TEncCtx* pCtx, AL_TBuffer* pStream, uint32_t const uTotalSize, uint32_t const uFlags)
{
  uint32_t uSize;
  AL_AVC_UpdateSPS(&pCtx->m_AvcSPS, pCtx->m_iMaxNumRef, pCtx->m_Settings.tChParam.tRCParam.uCPBSize, &pCtx->m_Settings);
  pCtx->m_AvcSPS.seq_parameter_set_id = 0;
  WriteSPS(pCtx);
  uSize = StreamGetNumBytes(&pCtx->m_Stream) - uTotalSize;
  AddSection(pStream, uTotalSize, uSize, uFlags);
  return uSize;
}

/***************************************************************************/
static uint32_t GeneratePPS(AL_TEncCtx* pCtx, AL_TBuffer* pStream, uint32_t const uTotalSize, uint32_t const uFlags, AL_TEncPicStatus const* pPicStatus)
{
  uint32_t uSize;
  AL_AVC_UpdatePPS(&pCtx->m_AvcPPS, &pCtx->m_Settings, pCtx->m_iMaxNumRef, pPicStatus->iPpsQP);
  pCtx->m_AvcPPS.seq_parameter_set_id = 0;
  WritePPS(pCtx);
  uSize = StreamGetNumBytes(&pCtx->m_Stream) - uTotalSize;
  AddSection(pStream, uTotalSize, uSize, uFlags);
  return uSize;
}

/***************************************************************************/
static void GenerateSEI(AL_TEncCtx* pCtx, AL_TBuffer* pStream, uint32_t const uTotalSize, uint32_t const uFlags, AL_TEncPicStatus const* pPicStatus)
{
  uint32_t uSize;
  uint32_t uSeiFlags = SEI_PT;

  if(pPicStatus->eType == SLICE_I)
  {
    uSeiFlags |= SEI_BP;

    if(!pPicStatus->bIsIDR)
      uSeiFlags |= SEI_RP;
  }
  uSeiFlags &= pCtx->m_Settings.uEnableSEI; // Filter with allowed SEI

  pCtx->m_AvcSPS.seq_parameter_set_id = 0;
  WriteSEI(pCtx, uSeiFlags, false, pPicStatus);
  uSize = StreamGetNumBytes(&pCtx->m_Stream) - uTotalSize;

  if(uSize)
    AddSection(pStream, uTotalSize, uSize, uFlags);
}

/***************************************************************************/
static void GenerateNALUnits(AL_TEncCtx* pCtx, AL_TNALUnitsInfo const tNALUnitsInfo, AL_TEncPicStatus const* pPicStatus, AL_TBuffer* pStream)
{
  uint32_t uSumSize = 0;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  uint32_t uOffset = pMetaData->uOffset % pMetaData->uMaxSize;
  uint32_t uFlags = SECTION_CONFIG_FLAG | AL_StreamSection_GetCompleteFlags(tNALUnitsInfo.isSPSEnabled);

  assert((pMetaData->uMaxSize - uOffset) >= ENC_MAX_HEADER_SIZE);

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);
  {
    StreamInitBuffer(&pCtx->m_Stream, pStream->pData + uOffset, ENC_MAX_HEADER_SIZE);

    if(tNALUnitsInfo.isAUDEnabled)
      uSumSize += GenerateAUD(pCtx, pStream, uSumSize, uFlags, pPicStatus->eType);

    if(tNALUnitsInfo.isSPSEnabled)
      uSumSize += GenerateSPS(pCtx, pStream, uSumSize, uFlags);

    if(tNALUnitsInfo.isPPSEnabled)
      uSumSize += GeneratePPS(pCtx, pStream, uSumSize, uFlags, pPicStatus);

    GenerateSEI(pCtx, pStream, uSumSize, uFlags, pPicStatus);
  }
  AL_IP_ENCODER_END_MUTEX(pCtx);
}

/***************************************************************************/
static void EndEncoding(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream)
{
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;

  if(!pPicStatus || !pStream)
  {
    AL_Common_Encoder_EndEncoding(pCtx, NULL, NULL);
    return;
  }

  assert(pPicStatus->iNumParts > 0);
  int iPoolID = pPicStatus->UserParam;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[iPoolID];
  AL_TNALUnitsInfo tNALUnitsInfo =
  {
    pCtx->m_Settings.bEnableAUD, /* isAUDEnabled */
    pPicStatus->bIsIDR,            /* isSPSEnabled */
    (pPicStatus->eType == SLICE_I) /* isPPSEnabled */
  };
  const uint32_t uCompleteFlags = AL_StreamSection_GetCompleteFlags(pPicStatus->bIsIDR);

  assert(pStream);
  assert(pPicStatus->iNumParts);

  if(pPicStatus->eErrorCode & AL_ERROR)
  {
    AL_IP_ENCODER_BEGIN_MUTEX(pCtx);
    pCtx->m_eError = pPicStatus->eErrorCode;
    AL_IP_ENCODER_END_MUTEX(pCtx);
  }
  else if(pPicStatus->bSkip)
  {
  }
  else
  {
    int iPart;
    uint16_t uLastSecID = 0;
    AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(pStream->pData + pPicStatus->uStrmPartOffset);
    AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

    uint32_t uOffset = pStreamParts[pPicStatus->iNumParts - 1].uOffset
                       + pStreamParts[pPicStatus->iNumParts - 1].uSize;

    pMetaData->uMaxSize -= (pMetaData->uMaxSize - pPicStatus->uStrmPartOffset);
    pMetaData->uAvailSize = pMetaData->uMaxSize - pMetaData->uOffset;

    // High Level Syntax (AUD, SPS, PPS, SEI ...) ---------------------------
    pFI->uFirstSecID = AddSection(pStream, 0, 0, 0); // Reserve one section for filler data
    GenerateNALUnits(pCtx, tNALUnitsInfo, pPicStatus, pStream);

    if(pPicStatus->iFiller)
      AL_Common_Encoder_AddFillerData(pStream, &uOffset, pFI->uFirstSecID, pPicStatus->iFiller, true);

    for(iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
      uLastSecID = AddSection(pStream, pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize, uCompleteFlags);

    SetSectionFlags(pStream, uLastSecID, SECTION_END_FRAME_FLAG | uCompleteFlags);

    SetSectionFlags(pStream, pFI->uFirstSecID, uCompleteFlags);

    if(pPicStatus->eType == SLICE_I)
      pCtx->m_uCpbRemovalDelay = 0;
    pCtx->m_uCpbRemovalDelay += PictureDisplayToFieldNumber[pPicStatus->ePicStruct];
  }

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);

  if(pCtx->m_Settings.tChParam.tRCParam.eRCMode == AL_RC_LOW_LATENCY)
    pCtx->uNumGroup = pPicStatus->uNumGroup;
  AL_IP_ENCODER_END_MUTEX(pCtx);

  AL_TBuffer* pSrc = (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;
  AL_Common_Encoder_EndEncoding(pCtx, pStream, pSrc);
  AL_Buffer_Unref(pSrc);

  if(pFI->pQpTable)
    AL_Buffer_Unref(pFI->pQpTable);

  AL_IP_ENCODER_BEGIN_MUTEX(pCtx);
  ++pCtx->m_iFrameCountDone;

  AL_IP_ENCODER_END_MUTEX(pCtx);
  AL_IP_ENCODER_RELEASE_SEM(pCtx);
}

static void UpdateHls(AL_TEncChanParam* pChParam)
{
  // Update SPS & PPS Flags ------------------------------------------------
  pChParam->uSpsParam = 0x4A | AL_SPS_TEMPORAL_MVP_EN_FLAG; // TODO
  pChParam->uPpsParam = AL_PPS_DBF_OVR_EN_FLAG; // TODO
  AL_Common_Encoder_SetHlsParam(pChParam);
}

static void UpdateMotionEstimationRange(AL_TEncChanParam* pChParam)
{
  // Update Motion Estimation Range ----------------------------------------
#if (AL_VERSION_GEN >= AL_110 || AL_VERSION_GEN == AL_110L)
  AL_Common_Encoder_SetME(32, 32, 12, 12, pChParam);
#else
  AL_Common_Encoder_SetME(16, 16, 12, 12, pChParam);
#endif
}

static void ComputeQPInfo(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  // Calculate Initial QP if not provided ----------------------------------
  if(!AL_Common_Encoder_IsInitialQpProvided(pChParam))
  {
    uint32_t iBitPerPixel = AL_Common_Encoder_ComputeBitPerPixel(pChParam);
    int8_t iInitQP = AL_Common_Encoder_GetInitialQP(iBitPerPixel);

    if(pChParam->tGopParam.uGopLength <= 1)
      iInitQP += 12;
    pChParam->tRCParam.iInitialQP = iInitQP;
  }

  if(pChParam->tRCParam.eRCMode != AL_RC_CONST_QP && pChParam->tRCParam.iMinQP < 10)
    pChParam->tRCParam.iMinQP = 10;

  if(pCtx->m_Settings.eQpCtrlMode == RANDOM_QP
     || pCtx->m_Settings.eQpCtrlMode == BORDER_QP
     || pCtx->m_Settings.eQpCtrlMode == RAMP_QP)
  {
    int iCbOffset = pChParam->iCbPicQpOffset;
    int iCrOffset = pChParam->iCrPicQpOffset;

    pChParam->tRCParam.iMinQP = Max(0, -(iCbOffset < iCrOffset ? iCbOffset : iCrOffset));
    pChParam->tRCParam.iMaxQP = Min(51, 51 - (iCbOffset > iCrOffset ? iCbOffset : iCrOffset));
  }
  pChParam->tRCParam.iInitialQP = Clip3(pChParam->tRCParam.iInitialQP,
                                        pChParam->tRCParam.iMinQP,
                                        pChParam->tRCParam.iMaxQP);
}

AL_TEncCtx* AL_AVC_Encoder_Create(TScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TEncSettings const* pSettings)
{
  assert(pSettings);

  // Create New encoder context --------------------------------------------
  AL_TEncCtx* pCtx = (AL_TEncCtx*)Rtos_Malloc(sizeof(AL_TEncCtx));

  if(!pCtx)
    return NULL;

  AL_TEncChanParam* pChParam = AL_Common_Encoder_InitChannelParam(pCtx, pSettings);
  AL_Common_Encoder_InitCtx(pCtx, pChParam, pAllocator);

  UpdateHls(pChParam);
  UpdateMotionEstimationRange(pChParam);
  ComputeQPInfo(pCtx, pChParam);

  // Initialize Scheduler -------------------------------------------------
  pCtx->m_pScheduler = pScheduler;

  if(pSettings->eScalingList != FLAT)
    pChParam->eOptions |= AL_OPT_SCL_LST;

  if(pSettings->eLdaCtrlMode != DEFAULT_LDA)
    pChParam->eOptions |= AL_OPT_CUSTOM_LDA;


  AL_TISchedulerCallBacks CBs = { 0 };
  CBs.pfnEndEncodingCallBack = EndEncoding;
  CBs.pEndEncodingCBParam = pCtx;
#if ENABLE_WATCHDOG
  AL_Common_Encoder_SetWatchdogCB(&CBs, pSettings);
#endif

#if ENABLE_RTOS_SYNC
  /* We don't want to send a cmd the scheduler can't store */
  pCtx->m_Semaphore = Rtos_CreateSemaphore(ENC_MAX_CMD - 1);
#endif
  GenerateSkippedPictureData(pCtx);

  // Scaling List -----------------------------------------------------------
  AL_AVC_SelectScalingList(&pCtx->m_AvcSPS, &pCtx->m_Settings);

  if(pSettings->eScalingList != FLAT)
    AL_AVC_PreprocessScalingList(&pCtx->m_AvcSPS.scaling_list_param, &pCtx->m_tBufEP1);

  // Lambdas ----------------------------------------------------------------
  if(pSettings->eLdaCtrlMode != DEFAULT_LDA)
    GetLambda(pSettings, &pCtx->m_tBufEP1, 0);

  pCtx->m_hChannel = AL_ISchedulerEnc_CreateChannel(pCtx->m_pScheduler, pChParam, pCtx->m_tBufEP1.tMD.uPhysicalAddr, &CBs);

  if(pCtx->m_hChannel == AL_INVALID_CHANNEL)
    goto fail_create_channel; // cannot create channel, probably not enough core for the resolution


  AL_Common_Encoder_SetMaxNumRef(pCtx, pChParam);

  return pCtx;

  fail_create_channel:
  AL_Common_Encoder_DestroyCtx(pCtx);
  return NULL;
}


