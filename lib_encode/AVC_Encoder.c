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

#include "Com_Encoder.h"
#include "AVC_Sections.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/Utils.h"
#include "lib_common/Error.h"
#include "lib_preprocess/LoadLda.h"

#define DEBUG_PATH "."

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
static void EndEncoding(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream)
{
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;
  assert(pCtx);
  assert(pPicStatus->iNumParts > 0);
  int iPoolID = pPicStatus->UserParam;
  AL_TFrameInfo* pFI = &pCtx->m_Pool[iPoolID];

  Rtos_GetMutex(pCtx->m_Mutex);
  pCtx->m_eError = pPicStatus->eErrorCode;
  Rtos_ReleaseMutex(pCtx->m_Mutex);

  if(!(pPicStatus->eErrorCode & AL_ERROR || pPicStatus->bSkip))
  {
    AL_AVC_UpdatePPS(&pCtx->m_pps, pPicStatus);
    AVC_GenerateSections(pCtx, pStream, pPicStatus);

    if(pPicStatus->eType == SLICE_I)
      pCtx->m_seiData.cpbRemovalDelay = 0;
    pCtx->m_seiData.cpbRemovalDelay += PictureDisplayToFieldNumber[pPicStatus->ePicStruct];
  }

  AL_TBuffer* pSrc = (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;
  AL_Common_Encoder_EndEncoding(pCtx, pStream, pSrc, pFI->pQpTable, pPicStatus->bIsLastSlice);
}

static void EndEncodingWrap(void* pUserParam, AL_TEncPicStatus* pPicStatus, AL_64U streamUserPtr)
{
  AL_TEncCtx* pCtx = (AL_TEncCtx*)pUserParam;

  if(!pPicStatus)
  {
    AL_Common_Encoder_EndEncoding(pCtx, NULL, NULL, NULL, false);
    return;
  }

  int streamId = (int)streamUserPtr;

  /* we require the stream to come back in the same order we sent them */
  assert(streamId >= 0 && streamId < AL_MAX_STREAM_BUFFER);
  assert(pCtx->m_iCurStreamRecv == streamId);

  pCtx->m_iCurStreamRecv = (pCtx->m_iCurStreamRecv + 1) % AL_MAX_STREAM_BUFFER;
  EndEncoding(pUserParam, pPicStatus, pCtx->m_StreamSent[streamId]);
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
  AL_Common_Encoder_SetME(AVC_MAX_HORIZONTAL_RANGE_P, AVC_MAX_VERTICAL_RANGE_P, AVC_MAX_HORIZONTAL_RANGE_B, AVC_MAX_VERTICAL_RANGE_B, pChParam);
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

  if(pChParam->tRCParam.iMaxQP < pChParam->tRCParam.iMinQP)
    pChParam->tRCParam.iMaxQP = pChParam->tRCParam.iMinQP;

  if(pCtx->m_Settings.eQpCtrlMode == RANDOM_QP
     || pCtx->m_Settings.eQpCtrlMode == BORDER_QP
     || pCtx->m_Settings.eQpCtrlMode == RAMP_QP)
  {
    int iCbOffset = pChParam->iCbPicQpOffset;
    int iCrOffset = pChParam->iCrPicQpOffset;

    pChParam->tRCParam.iMinQP = Max(0, 0 - (iCbOffset < iCrOffset ? iCbOffset : iCrOffset));
    pChParam->tRCParam.iMaxQP = Min(51, 51 - (iCbOffset > iCrOffset ? iCbOffset : iCrOffset));
  }
  pChParam->tRCParam.iInitialQP = Clip3(pChParam->tRCParam.iInitialQP,
                                        pChParam->tRCParam.iMinQP,
                                        pChParam->tRCParam.iMaxQP);
}

AL_ERR AL_AVC_Encoder_Create(AL_TEncCtx** hCtx, TScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TEncSettings const* pSettings)
{
  assert(pSettings);
  AL_ERR errorCode = AL_ERROR;

  // Create New encoder context --------------------------------------------
  *hCtx = Rtos_Malloc(sizeof(AL_TEncCtx));
  AL_TEncCtx* pCtx = *hCtx;

  if(!pCtx)
    return errorCode;

  AL_TEncChanParam* pChParam = AL_Common_Encoder_InitChannelParam(pCtx, pSettings);
  AL_Common_Encoder_InitCtx(pCtx, pChParam, pAllocator);

  UpdateHls(pChParam);
  UpdateMotionEstimationRange(pChParam);
  ComputeQPInfo(pCtx, pChParam);

  // Initialize Scheduler -------------------------------------------------
  pCtx->m_pScheduler = pScheduler;

  if(pSettings->eScalingList != AL_SCL_FLAT)
    pChParam->eOptions |= AL_OPT_SCL_LST;

  if(pSettings->tChParam.eLdaCtrlMode != DEFAULT_LDA)
    pChParam->eOptions |= AL_OPT_CUSTOM_LDA;


  AL_TISchedulerCallBacks CBs = { 0 };
  CBs.pfnEndEncodingCallBack = EndEncodingWrap;
  CBs.pEndEncodingCBParam = pCtx;

  /* We don't want to send a cmd the scheduler can't store */
  pCtx->m_PendingEncodings = Rtos_CreateSemaphore(ENC_MAX_CMD - 1);

  GenerateSkippedPictureData(pCtx);

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

  if(pCtx->m_hChannel == AL_INVALID_CHANNEL)
    goto fail_create_channel; // cannot create channel, probably not enough core for the resolution


  AL_Common_Encoder_SetMaxNumRef(pCtx, pChParam);

  AL_AVC_GenerateSPS(&pCtx->m_sps, &pCtx->m_Settings, pCtx->m_iMaxNumRef, pChParam->tRCParam.uCPBSize);
  AL_AVC_GeneratePPS(&pCtx->m_pps, &pCtx->m_Settings, pCtx->m_iMaxNumRef);

  if(pSettings->eScalingList != AL_SCL_FLAT)
    AL_AVC_PreprocessScalingList(&pCtx->m_sps.m_AvcSPS.scaling_list_param, &pCtx->m_tBufEP1);

  // Return pointer to the Encoder Context as handle ------------------------
  return AL_SUCCESS;

  fail_create_channel:
  AL_Common_Encoder_DestroyCtx(pCtx);
  *hCtx = NULL;
  return errorCode;
}


