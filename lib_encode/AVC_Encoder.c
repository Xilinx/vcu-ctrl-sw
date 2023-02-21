// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Com_Encoder.h"
#include "AVC_Sections.h"
#include "lib_common_enc/PictureInfo.h"

static void updateHlsAndWriteSections(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream, int iLayerID, int iPicID)
{
  AL_HLSInfo* pHLSInfo = AL_GetHLSInfo(pCtx, iPicID);
  AL_AVC_UpdateSPS(&pCtx->tLayerCtx[iLayerID].sps, pCtx->pSettings, pPicStatus, pHLSInfo, &pCtx->tHeadersCtx[iLayerID]);
  bool bMustWritePPS = AL_AVC_UpdatePPS(&pCtx->tLayerCtx[iLayerID].pps, pPicStatus, pHLSInfo, &pCtx->tHeadersCtx[iLayerID], pCtx->pSettings->tChParam[iLayerID].bForcePpsIdToZero);
  bool bMustWriteAUD = AL_AVC_UpdateAUD(&pCtx->tLayerCtx[iLayerID].aud, pCtx->pSettings, pPicStatus, iLayerID);
  AVC_GenerateSections(pCtx, pStream, pPicStatus, iPicID, bMustWritePPS, bMustWriteAUD);

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

static void initHlsSps(AL_TEncChanParam* pChParam, uint32_t* pSpsParam)
{
  (void)pChParam;
  *pSpsParam = AL_SPS_TEMPORAL_MVP_EN_FLAG; // TODO

  int log2_max_poc = (pChParam->tRCParam.eOptions & AL_RC_OPT_ENABLE_SKIP) ? 16 : 10;

  if(AL_IS_XAVC(pChParam->eProfile))
    log2_max_poc = 4;
  AL_SET_SPS_LOG2_MAX_POC(pSpsParam, log2_max_poc);
  int log2_max_frame_num_minus4 = 0; // This value SHOULD be equals to IP_Utils SPS

  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && pChParam->tGopParam.uNumB == 15)
    log2_max_frame_num_minus4 = 1;

  else if(AL_IsGdrEnabled(pChParam))
    log2_max_frame_num_minus4 = 6; // 6 is to support AVC 8K GDR.

  AL_SET_SPS_LOG2_MAX_FRAME_NUM(pSpsParam, log2_max_frame_num_minus4 + 4);
}

static void initHls(AL_TEncChanParam* pChParam)
{
  initHlsSps(pChParam, &pChParam->uSpsParam);
  pChParam->uPpsParam = AL_PPS_DBF_OVR_EN_FLAG; // TODO
  AL_Common_Encoder_SetHlsParam(pChParam);
}

static void SetMotionEstimationRange(AL_TEncChanParam* pChParam)
{
  AL_Common_Encoder_SetME(AVC_MAX_HORIZONTAL_RANGE_P, AVC_MAX_VERTICAL_RANGE_P, AVC_MAX_HORIZONTAL_RANGE_B, AVC_MAX_VERTICAL_RANGE_B, pChParam);
}

static void ComputeQPInfo(AL_TEncChanParam* pChParam)
{
  int iCbOffset = pChParam->iCbPicQpOffset;
  int iCrOffset = pChParam->iCrPicQpOffset;
  AL_Common_Encoder_ComputeRCParam(iCbOffset, iCrOffset, 12, pChParam);
}

static void generateNals(AL_TEncCtx* pCtx, int iLayerID, bool bWriteVps)
{
  (void)bWriteVps;
  AL_TEncChanParam* pChParam = &pCtx->pSettings->tChParam[iLayerID];

  uint32_t uCpbBitSize = (uint32_t)((uint64_t)pChParam->tRCParam.uCPBSize * (uint64_t)pChParam->tRCParam.uMaxBitRate / 90000LL);
  AL_AVC_GenerateSPS(&pCtx->tLayerCtx[0].sps, pCtx->pSettings, pCtx->iMaxNumRef, uCpbBitSize);
  AL_AVC_GeneratePPS(&pCtx->tLayerCtx[0].pps, pCtx->pSettings, &pCtx->tLayerCtx[0].sps);
}

static void ConfigureChannel(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TEncSettings const* pSettings)
{
  (void)pCtx;
  initHls(pChParam);
  SetMotionEstimationRange(pChParam);
  ComputeQPInfo(pChParam);

  if(pSettings->eScalingList != AL_SCL_FLAT)
    pChParam->eEncTools |= AL_OPT_SCL_LST;

}

static void preprocessEp1(AL_TEncCtx* pCtx, TBufferEP* pEp1)
{
  if(pCtx->pSettings->eScalingList != AL_SCL_FLAT)
    AL_AVC_PreprocessScalingList(&pCtx->tLayerCtx[0].sps.AvcSPS.scaling_list_param, pCtx->tLayerCtx[0].sps.AvcSPS.chroma_format_idc, pEp1);
}

void AL_CreateAvcEncoder(HighLevelEncoder* pCtx)
{
  pCtx->shouldReleaseSource = &shouldReleaseSource;
  pCtx->preprocessEp1 = &preprocessEp1;
  pCtx->configureChannel = &ConfigureChannel;
  pCtx->generateNals = &generateNals;
  pCtx->updateHlsAndWriteSections = &updateHlsAndWriteSections;
}

