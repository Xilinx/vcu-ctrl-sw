/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include "Com_Encoder.h"
#include "HEVC_Sections.h"
#include "lib_common_enc/PictureInfo.h"

static void updateHlsAndWriteSections(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream, int iLayerID, int iPicID)
{
  AL_HLSInfo* pHLSInfo = AL_GetHLSInfo(pCtx, iPicID);
  AL_HEVC_UpdateSPS(&pCtx->tLayerCtx[iLayerID].sps, pCtx->pSettings, pPicStatus, pHLSInfo, &pCtx->tHeadersCtx[iLayerID], iLayerID);
  bool bMustWritePPS = AL_HEVC_UpdatePPS(&pCtx->tLayerCtx[iLayerID].pps, pCtx->pSettings, pPicStatus, pHLSInfo, &pCtx->tHeadersCtx[iLayerID], iLayerID);
  bool bMustWriteAUD = AL_HEVC_UpdateAUD(&pCtx->tLayerCtx[iLayerID].aud, pCtx->pSettings, pPicStatus, iLayerID);
  HEVC_GenerateSections(pCtx, pStream, pPicStatus, iLayerID, iPicID, bMustWritePPS, bMustWriteAUD);

  pCtx->initialCpbRemovalDelay = pPicStatus->uInitialRemovalDelay;
  pCtx->cpbRemovalDelay += PicStructToFieldNumber[pPicStatus->ePicStruct];

  if(pPicStatus->eType == AL_SLICE_I)
  {
    int const iDefaultCpbRemovalDelay = 0;
    pCtx->cpbRemovalDelay = iDefaultCpbRemovalDelay;
  }
}

static bool shouldReleaseSource(AL_TEncPicStatus* p)
{
  (void)p;
  return true;
}

static void initHls(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  // Update SPS & PPS Flags ------------------------------------------------
  uint32_t* pSpsParam = &pChParam->uSpsParam;
  *pSpsParam = AL_SPS_TEMPORAL_MVP_EN_FLAG; // TODO

  int log2_max_poc = (pChParam->tRCParam.eOptions & AL_RC_OPT_ENABLE_SKIP) ? 16 : 10;
  AL_SET_SPS_LOG2_MAX_POC(pSpsParam, log2_max_poc);

  int num_short_term_ref_pic_sets_log2 = 0;

  AL_SET_SPS_LOG2_NUM_SHORT_TERM_RPS(pSpsParam, num_short_term_ref_pic_sets_log2);
  pChParam->uPpsParam |= AL_PPS_ENABLE_REORDERING;

  AL_Common_Encoder_SetHlsParam(pChParam);

  if(pCtx->pSettings->bDependentSlice)
    pChParam->uPpsParam |= AL_PPS_SLICE_SEG_EN_FLAG;

  if(pChParam->tGopParam.bEnableLT)
    pChParam->uSpsParam |= AL_SPS_LOG2_NUM_LONG_TERM_RPS_MASK;

  bool bOverrideLF = false;

  if(pChParam->eEncTools & AL_OPT_LF)
  {
    bOverrideLF = true;
  }

  if(bOverrideLF)
    pChParam->uPpsParam |= AL_PPS_OVERRIDE_LF;

  if(!(pChParam->eEncTools & AL_OPT_LF))
    pChParam->uPpsParam |= AL_PPS_DISABLE_LF;

  if((AL_GET_CHROMA_MODE(pChParam->ePicFormat) != AL_CHROMA_MONO) && (pChParam->iCbSliceQpOffset || pChParam->iCrSliceQpOffset))
    pChParam->uPpsParam |= AL_PPS_SLICE_CHROMA_QP_OFFSET_PRES_FLAG;

}

static void SetMotionEstimationRange(AL_TEncChanParam* pChParam)
{
  AL_Common_Encoder_SetME(HEVC_MAX_HORIZONTAL_RANGE_P, HEVC_MAX_VERTICAL_RANGE_P, HEVC_MAX_HORIZONTAL_RANGE_B, HEVC_MAX_VERTICAL_RANGE_B, pChParam);
}

static void ComputeQPInfo(AL_TEncChanParam* pChParam)
{
  int iCbOffset = pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;
  int iCrOffset = pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;
  AL_Common_Encoder_ComputeRCParam(iCbOffset, iCrOffset, 10, pChParam);
}

static void generateNals(AL_TEncCtx* pCtx, int iLayerID, bool bWriteVps)
{
  AL_TEncChanParam* pChParam = &pCtx->pSettings->tChParam[iLayerID];

  uint32_t uCpbBitSize = (uint32_t)((uint64_t)pChParam->tRCParam.uCPBSize * (uint64_t)pChParam->tRCParam.uMaxBitRate / 90000LL);
  AL_HEVC_GenerateSPS(&pCtx->tLayerCtx[iLayerID].sps, pCtx->pSettings, pChParam, pCtx->iMaxNumRef, uCpbBitSize, iLayerID);
  AL_HEVC_GeneratePPS(&pCtx->tLayerCtx[iLayerID].pps, pCtx->pSettings, pChParam, pCtx->iMaxNumRef, iLayerID);

  if(bWriteVps)
    AL_HEVC_GenerateVPS(&pCtx->vps, pCtx->pSettings, pCtx->iMaxNumRef);
}

static void ConfigureChannel(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TEncSettings const* pSettings)
{
  initHls(pCtx, pChParam);
  SetMotionEstimationRange(pChParam);
  ComputeQPInfo(pChParam);

  if(pSettings->eScalingList != AL_SCL_FLAT)
    pChParam->eEncTools |= AL_OPT_SCL_LST;
}

static void preprocessEp1(AL_TEncCtx* pCtx, TBufferEP* pEp1)
{
  if(pCtx->pSettings->eScalingList != AL_SCL_FLAT)
    AL_HEVC_PreprocessScalingList(&pCtx->tLayerCtx[0].sps.HevcSPS.scaling_list_param, pEp1);
}

void AL_CreateHevcEncoder(HighLevelEncoder* pCtx)
{
  pCtx->shouldReleaseSource = &shouldReleaseSource;
  pCtx->preprocessEp1 = &preprocessEp1;
  pCtx->configureChannel = &ConfigureChannel;
  pCtx->generateNals = &generateNals;
  pCtx->updateHlsAndWriteSections = &updateHlsAndWriteSections;
}

