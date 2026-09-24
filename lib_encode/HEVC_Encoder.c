// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Common_Encoder.h"
#include "HEVC_Sections.h"
#include "lib_common_enc/PictureInfo.h"

static void updateHlsAndWriteSections(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream, int32_t iLayerID, int32_t iPicID)
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
    int32_t const iDefaultCpbRemovalDelay = 0;
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
  *pSpsParam = pChParam->TemporalMVP ? AL_SPS_TEMPORAL_MVP_EN_FLAG : 0;

  int32_t log2_max_poc = (pChParam->tRCParam.eOptions & AL_RC_OPT_ENABLE_SKIP) ? 16 : 10;
  AL_SET_SPS_LOG2_MAX_POC(pSpsParam, log2_max_poc);

  int32_t num_short_term_ref_pic_sets_log2 = 0;

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

  if(pChParam->eEncTools & AL_OPT_LF_X_SLICE)
    pChParam->uPpsParam |= AL_PPS_LF_X_SLICE_EN_FLAG;

  if((AL_GET_CHROMA_MODE(pChParam->ePicFormat) != AL_CHROMA_MONO) && (pChParam->iCbSliceQpOffset || pChParam->iCrSliceQpOffset))
    pChParam->uPpsParam |= AL_PPS_SLICE_CHROMA_QP_OFFSET_PRES_FLAG;

}

static void SetMotionEstimationRange(AL_TEncChanParam* pChParam)
{
  AL_Common_Encoder_SetME(HEVC_MAX_HORIZONTAL_RANGE_P, HEVC_MAX_VERTICAL_RANGE_P, HEVC_MAX_HORIZONTAL_RANGE_B, HEVC_MAX_VERTICAL_RANGE_B, pChParam);
}

static void ComputeQPInfo(AL_TEncChanParam* pChParam)
{
  int32_t iCbOffset = pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;
  int32_t iCrOffset = pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;
  AL_Common_Encoder_ComputeRCParam(iCbOffset, iCrOffset, 10, pChParam);
}

static void generateNals(AL_TEncCtx* pCtx, int32_t iLayerID, bool bWriteVps)
{
  AL_TEncChanParam* pChParam = &pCtx->pSettings->tChParam[iLayerID];

  uint32_t uCpbBitSize = (uint32_t)((AL_64U)pChParam->tRCParam.uCPBSize * (AL_64U)pChParam->tRCParam.uMaxBitRate / 90000uLL);
  AL_HEVC_GenerateSPS(&pCtx->tLayerCtx[iLayerID].sps, pCtx->pSettings, pChParam, pCtx->iMaxBuffering, pCtx->iMaxReordering, uCpbBitSize, iLayerID);
  AL_HEVC_GeneratePPS(&pCtx->tLayerCtx[iLayerID].pps, pCtx->pSettings, pChParam, iLayerID);

  if(bWriteVps)
    AL_HEVC_GenerateVPS(&pCtx->vps, pCtx->pSettings, pCtx->iMaxBuffering, pCtx->iMaxReordering);
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
