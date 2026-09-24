// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "FrameParam.h"
#include "DefaultDecoder.h"
#include "SliceDataParsing.h"
#include "NalUnitParserPrivate.h"

#include "lib_common/Nuts.h"
#include "lib_common/Round.h"
#include "lib_common/Arithmetics.h"
#include "lib_common/HevcLevelsLimit.h"
#include "lib_common/HevcUtils.h"
#include "lib_common/Error.h"
#include "lib_common/SyntaxConversion.h"

#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecInfoInternal.h"
#include "lib_common_dec/Scheduler_Defs.h"
#include "lib_common_dec/DecHardwareConfig.h"
#include "lib_common_dec/StreamSettingsInternal.h"
#include "lib_decode/NalUnitParser.h"
#include "lib_rtos/assert.h"

#include "lib_common_dec/HDRMeta.h"

#include "lib_decode/I_DecSchedulerInfo.h"

#include "lib_parsing/HevcParser.h"
#include "lib_parsing/Hevc_PictMngr.h"
#include "lib_parsing/Hevc_SliceHeaderParsing.h"

/*************************************************************************/
static int32_t getMaxBitDepthFromSPSPBitDepth(AL_THevcSps const* pSPS)
{
  int32_t iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;
  int32_t iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;
  int32_t iMaxSPSBitDepth = Max(iSPSLumaBitDepth, iSPSChromaBitDepth);
  int32_t iMaxBitDepth = iMaxSPSBitDepth;

  if((iMaxBitDepth % 2) != 0)
    iMaxBitDepth++;
  return iMaxBitDepth;
}

/*****************************************************************************/
static bool isRandomAccessPoint(AL_ENut eNUT)
{
  return AL_HEVC_IsCRA(eNUT) || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL22) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL23);
}

/*****************************************************************************/
static int32_t calculatePocBasic(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice, uint8_t uNoRasOutputFlag)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

  if(pSlice->IdrPicFlag)
  {
    if(pDpb)
      pDpb->iPrevPOC = 0;

    return 0;
  }

  int32_t iPOCMsb = 0;

  if(!isRandomAccessPoint(pSlice->nal_unit_type) || !uNoRasOutputFlag)
  {
    int32_t iMaxPOCLsb = pSlice->pSPS->MaxPicOrderCntLsb;
    int32_t iMaxPOCLsb_div2 = iMaxPOCLsb >> 1;

    int32_t iPrevPocLsb = pDpb ? pDpb->iPrevPOC & (iMaxPOCLsb - 1) : 0;
    int32_t iPrevPocMsb = pDpb ? pDpb->iPrevPOC - iPrevPocLsb : 0;

    if((pSlice->slice_pic_order_cnt_lsb < iPrevPocLsb) &&
       ((iPrevPocLsb - pSlice->slice_pic_order_cnt_lsb) >= iMaxPOCLsb_div2))
      iPOCMsb = iPrevPocMsb + iMaxPOCLsb;

    else if((pSlice->slice_pic_order_cnt_lsb > iPrevPocLsb) &&
            ((pSlice->slice_pic_order_cnt_lsb - iPrevPocLsb) > iMaxPOCLsb_div2))
      iPOCMsb = iPrevPocMsb - iMaxPOCLsb;

    else
      iPOCMsb = iPrevPocMsb;
  }

  int32_t iPicOrderCntVal = pSlice->slice_pic_order_cnt_lsb + iPOCMsb;

  if(pDpb && !(pSlice->nuh_temporal_id_plus1 - 1) && !AL_HEVC_IsRASL_RADL_SLNR(pSlice->nal_unit_type))
    pDpb->iPrevPOC = iPicOrderCntVal;

  return iPicOrderCntVal;
}

/*****************************************************************************/
static int32_t calculatePOC(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice, uint8_t uNoRasOutputFlag)
{
  int32_t iPOC;
  iPOC = calculatePocBasic(pCtx, pSlice, uNoRasOutputFlag);

  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

  if(pDpb)
    pDpb->iCurFramePOC = iPOC;

  return iPOC;
}

/*****************************************************************************/
static AL_ESequenceMode GetSequenceModeFromScanType(uint8_t source_scan_type)
{
  switch(source_scan_type)
  {
  case 0: return AL_SM_INTERLACED;
  case 1: return AL_SM_PROGRESSIVE;

  case 2:
  case 3: return AL_SM_UNKNOWN;

  default: return AL_SM_MAX_ENUM;
  }
}

/*****************************************************************************/
static AL_ESequenceMode getSequenceMode(AL_THevcSps const* pSPS)
{
  AL_THevcProfilevel const* pProfileLevel = &pSPS->profile_and_level;

  if(pSPS->vui_parameters_present_flag)
  {
    AL_TVuiParam const* pVUI = &pSPS->vui_param;

    if(pVUI->field_seq_flag)
      return AL_SM_INTERLACED;
  }

  if(pSPS->profile_and_level.general_frame_only_constraint_flag)
    return AL_SM_PROGRESSIVE;

  if((pProfileLevel->general_progressive_source_flag == 0) && (pProfileLevel->general_interlaced_source_flag == 0))
    return AL_SM_UNKNOWN;

  if(pProfileLevel->general_progressive_source_flag && (pProfileLevel->general_interlaced_source_flag == 0))
    return AL_SM_PROGRESSIVE;

  if((pProfileLevel->general_progressive_source_flag == 0) && pProfileLevel->general_interlaced_source_flag)
    return AL_SM_INTERLACED;

  if(pProfileLevel->general_progressive_source_flag && pProfileLevel->general_interlaced_source_flag)
    return GetSequenceModeFromScanType(pSPS->sei_source_scan_type);

  return AL_SM_MAX_ENUM;
}

/******************************************************************************/
static void extractStreamSettings(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS, AL_TStreamSettings* pStreamSettings)
{
  (void)pCtx;
  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };
  uint32_t uFlags = (pSPS->profile_and_level.general_max_12bit_constraint_flag << 15) |
                    (pSPS->profile_and_level.general_max_10bit_constraint_flag << 14) |
                    (pSPS->profile_and_level.general_max_8bit_constraint_flag << 13) |
                    (pSPS->profile_and_level.general_max_422chroma_constraint_flag << 12) |
                    (pSPS->profile_and_level.general_max_420chroma_constraint_flag << 11) |
                    (pSPS->profile_and_level.general_max_monochrome_constraint_flag << 10) |
                    (pSPS->profile_and_level.general_intra_constraint_flag << 9) |
                    (pSPS->profile_and_level.general_one_picture_only_constraint_flag << 8) |
                    (pSPS->profile_and_level.general_lower_bit_rate_constraint_flag << 7) |
                    (pSPS->profile_and_level.general_max_14bit_constraint_flag << 6);

  pStreamSettings->tDim = tSPSDim;
  pStreamSettings->eChroma = (AL_EChromaMode)pSPS->chroma_format_idc;
  pStreamSettings->iBitDepth = getMaxBitDepthFromSPSPBitDepth(pSPS);
  pStreamSettings->iLevel = pSPS->profile_and_level.general_level_idc / 3;
  pStreamSettings->eProfile = AL_PROFILE_HEVC | pSPS->profile_and_level.general_profile_idc | AL_RExt_FLAGS(uFlags);
  pStreamSettings->eSequenceMode = getSequenceMode(pSPS);
  pStreamSettings->iMaxRef = 0;
}

/*****************************************************************************/
static AL_ERR resolutionFound(AL_TDecCtx* pCtx, AL_TStreamSettings const* pCurrentStreamSettings, AL_TCropInfo const* pCropInfo)
{
  int32_t iMaxBuf = AL_HEVC_GetMinOutputBuffersNeeded(pCurrentStreamSettings, pCtx->iStackSize);
  return pCtx->tDecCB.resolutionFoundCB.func(iMaxBuf, pCurrentStreamSettings, pCropInfo, pCtx->tDecCB.resolutionFoundCB.userParam);
}

/******************************************************************************/
static AL_TDimension getMaxDimension(AL_THevcSps const* pSPS)
{
  /* Constrain further based on the profile?*/
  int32_t iMaxHeight = AL_DEC_MAX_HEIGHT;
  int32_t iMaxWidth = AL_DEC_MAX_WIDTH;

  (void)pSPS;

  return (AL_TDimension) {
           iMaxWidth, iMaxHeight
  };
}

/*****************************************************************************/
static AL_ERR isSPSCompatibleWithHardware(AL_THevcSps const* pSPS, bool bDisableMinimalResolutionCheck)
{
  (void)bDisableMinimalResolutionCheck;

  int32_t iSPSMaxBitDepth = getMaxBitDepthFromSPSPBitDepth(pSPS);
  AL_ERR eErr = AL_HWConfig_Dec_CheckBitDepth(iSPSMaxBitDepth);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  eErr = AL_HWConfig_Dec_CheckFrameResolution(tSPSDim, pSPS->Log2CtbSize, pSPS->Log2MinCbSize);

  AL_TDimension tMaxDim = getMaxDimension(pSPS);
  eErr = AL_HWConfig_Dec_CheckMaxFrameResolution(tSPSDim, tMaxDim);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;
  eErr = AL_HWConfig_Dec_CheckChromaMode(eSPSChromaMode);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  AL_ESequenceMode sequenceMode = getSequenceMode(pSPS);

  if(sequenceMode >= AL_SM_MAX_ENUM)
    return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  return AL_SUCCESS;
}

/*****************************************************************************/
static AL_ERR isSPSCompatibleWithInitialStreamSettings(AL_TDecCtx const* pCtx, AL_THevcSps const* pSPS)
{
  AL_TStreamSettings const* pStreamSettings = &pCtx->tInitialStreamSettings;

  if(!CheckStreamSettings(pStreamSettings))
    return AL_ERR_REQUEST_MALFORMED;

  int32_t iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;

  if(pStreamSettings->iBitDepth < iSPSLumaBitDepth)
  {
    Rtos_Log(AL_LOG_ERROR, "Bitdepth luma '%i' is not supported by the CHANNEL. Maximum supported bitdepth is '%i'.\n", iSPSLumaBitDepth, pStreamSettings->iBitDepth);
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  int32_t iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;

  if(pStreamSettings->iBitDepth < iSPSChromaBitDepth)
  {
    Rtos_Log(AL_LOG_ERROR, "Bitdepth chroma '%i' is not supported by the CHANNEL. Maximum supported bitdepth is '%i'.\n", iSPSChromaBitDepth, pStreamSettings->iBitDepth);
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  int32_t iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;
  int32_t iCurDPBSize = Max(AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, false, false, false), pStreamSettings->iMaxRef);
  int32_t iNewDPBSize = Min(AL_HEVC_GetMaxDPBSize(iSPSLevel, tSPSDim.iWidth, tSPSDim.iHeight, false, false, false), pSPS->sps_max_dec_pic_buffering_minus1[0] + 1);

  if(iNewDPBSize > iCurDPBSize)
    return AL_WARN_SPS_LEVEL_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if(pStreamSettings->eChroma < eSPSChromaMode)
  {
    Rtos_Log(AL_LOG_ERROR, "Chromamode '%i' is not supported by the CHANNEL. Maximum chromamode supported is '%i'.\n", eSPSChromaMode, pStreamSettings->eChroma);
    return AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(pStreamSettings->tDim.iWidth < tSPSDim.iWidth)
  {
    Rtos_Log(AL_LOG_ERROR, "Width '%i' is not supported by the CHANNEL. Maximum supported width is '%i'.\n", tSPSDim.iWidth, pStreamSettings->tDim.iWidth);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(pStreamSettings->tDim.iHeight < tSPSDim.iHeight)
  {
    Rtos_Log(AL_LOG_ERROR, "Height '%i' is not supported by the CHANNEL. Maximum supported height is '%i'.\n", tSPSDim.iHeight, pStreamSettings->tDim.iHeight);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  AL_ESequenceMode sequenceMode = getSequenceMode(pSPS);

  if((sequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != sequenceMode))
  {
    Rtos_Log(AL_LOG_ERROR, "Sequence '%i' is not supported by the CHANNEL. Current sequence mode is '%i'.\n", sequenceMode, pStreamSettings->eSequenceMode);
    return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
static AL_ERR isPPSCompatibleWidthCurrentDecodingMode(AL_TDecCtx const* pCtx, AL_THevcPps const* pPPS)
{

  if(pCtx->pChanParam->bLowLat &&
     pPPS->tiles_enabled_flag &&
     pPPS->num_tile_columns_minus1 >= pCtx->pChanParam->uNumCore * AL_DEC_NUM_PARSER_PER_CORE)
  {
    Rtos_Log(AL_LOG_ERROR, "Pictures with %d tile column(s) are not supported in lowlatency mode with only %d core(s)\n", pPPS->num_tile_columns_minus1 + 1, pCtx->pChanParam->uNumCore);
    return AL_WARN_PPS_TILEMAP_NOT_COMPATIBLE_WITH_DECODING_MODE;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;
  int32_t iMaxLCU = AL_RoundUp(pStreamSettings->tDim.iWidth, 64) * AL_RoundUp(pStreamSettings->tDim.iHeight, 64) / 256; // (64 * 64 / 16);
  int32_t iSPSMaxSlices = Min(AL_HEVC_GetMaxNumberOfSlices(pStreamSettings->iLevel), iMaxLCU);

  AL_TDecoderPoolSizes tSizes = { 0 };

  tSizes.iWPSize = iSPSMaxSlices * WP_SLICE_SIZE;
  tSizes.iSPSize = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  tSizes.iCompDataSize = AL_GetAllocSize_HevcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);
  tSizes.iCompMapSize = AL_GetAllocSize_DecCompMap(pStreamSettings->tDim);
  AL_ERR error = AL_ERR_NO_MEMORY;

  if(!AL_Decoder_AllocPool(pCtx, &tSizes))
    goto fail_alloc;

  int32_t const iDpbMaxBuf = AL_HEVC_GetMaxDpbBuffers(pStreamSettings);
  int32_t iDpbRef = Min(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);

  if(pCtx->bIntraOnlyProfile)
    iDpbRef = 1;

  AL_TPictMngrParam tPictMngrParam;
  tPictMngrParam.eCodec = AL_CODEC_HEVC;
  tPictMngrParam.uNumRef = iDpbRef;
  tPictMngrParam.eFbStorageMode = pCtx->pChanParam->eFBStorageMode;
  tPictMngrParam.uNumAnnexBuf = AL_HEVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
  tPictMngrParam.uNumSubBuf = ITU_ANNEX_NUM_BUF;
  size_t zSizes[ITU_ANNEX_NUM_BUF];
  zSizes[ITU_ANNEX_BUF_MV_IDX] = AL_GetAllocSize_HevcMV(pStreamSettings->tDim);
  zSizes[ITU_ANNEX_BUF_POC_IDX] = POCBUFF_PL_SIZE;
  tPictMngrParam.zSubBufSizes = zSizes;
  tPictMngrParam.bForceDisplay = pCtx->pChanParam->bCallDecodeCallbackAtStart;
  tPictMngrParam.tOutputPosition = pCtx->tOutputPosition;

  AL_TDpbInitParam tDpbParams;
  tDpbParams.uNumRef = iDpbRef;
  tDpbParams.eMode = pCtx->eDpbMode;
  AL_IReferenceManager* pRefMngr = AL_Dpb_Create(&tDpbParams);

  if(!AL_PictMngr_BasicInit(&pCtx->PictMngr, pRefMngr, &tPictMngrParam, pCtx->pAllocator))
    goto fail_alloc;

  AL_TCropInfo tCropInfo;
  AL_HEVC_GetCropInfo(pSPS, &tCropInfo);
  error = resolutionFound(pCtx, pStreamSettings, &tCropInfo);

  if(AL_IS_ERROR_CODE(error))
    goto fail_alloc;

  Rtos_WaitEvent(pCtx->hDecOutSettingsConfiguredEvt, AL_WAIT_FOREVER);

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1, true);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  AL_TDecChanParam* pChan = pCtx->pChanParam;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;
  pChan->iWidth = pStreamSettings->tDim.iWidth;
  pChan->iHeight = pStreamSettings->tDim.iHeight;
  pChan->uLog2MaxCuSize = pSPS->Log2CtbSize;
  pChan->eMaxChromaMode = pStreamSettings->eChroma;

  const int32_t iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(pStreamSettings->iLevel);
  pChan->iMaxSlices = iSPSMaxSlices;
  const int32_t iSPSMaxTileRows = AL_HEVC_GetMaxTileRows(pStreamSettings->iLevel);
  const int32_t iSPSMaxTileCols = AL_HEVC_GetMaxTileColumns(pStreamSettings->iLevel);
  const int32_t iPicMaxTileRows = AL_RoundUp(pChan->iHeight, 64) / 64;
  const int32_t iPicMaxTileCols = AL_RoundUp(pChan->iWidth, 256) / 256;
  pChan->iMaxTiles = Min(iSPSMaxTileCols, iPicMaxTileCols) * Min(iSPSMaxTileRows, iPicMaxTileRows);

  if(!pCtx->bForceFrameRate && pSPS->vui_parameters_present_flag && pSPS->vui_param.vui_timing_info_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale;
    pChan->uClkRatio = pSPS->vui_param.vui_num_units_in_tick;

    if(pCtx->uConcealMaxFps && pChan->uFrameRate / pChan->uClkRatio > pCtx->uConcealMaxFps)
    {
      pChan->uFrameRate = pCtx->uConcealMaxFps;
      pChan->uClkRatio = 1;
    }
  }

  return AL_Default_Decoder_CreateChannel(pCtx, AL_Default_Decoder_EndParsing, AL_Default_Decoder_EndDecoding);
}

/******************************************************************************/
static int32_t slicePpsId(AL_THevcSliceHdr const* pSlice)
{
  return pSlice->slice_pic_parameter_set_id;
}

/******************************************************************************/
static int32_t sliceSpsId(AL_THevcPps const* pPps, AL_THevcSliceHdr const* pSlice)
{
  int32_t const ppsid = slicePpsId(pSlice);
  return pPps[ppsid].pps_seq_parameter_set_id;
}

/******************************************************************************/
static int32_t sliceVpsId(AL_THevcSps const* pSps, AL_THevcPps const* pPps, AL_THevcSliceHdr const* pSlice)
{
  int32_t const spsid = sliceSpsId(pPps, pSlice);
  return pSps[spsid].sps_video_parameter_set_id;
}

/*****************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice)
{
  AL_TAup* pIAup = &pCtx->aup;
  AL_THevcAup* pAup = &pIAup->hevcAup;

  int32_t const spsid = sliceSpsId(pAup->pPPS, pSlice);
  int32_t const vpsid = sliceVpsId(pAup->pSPS, pAup->pPPS, pSlice);
  const uint8_t uMaxLayersMinus1 = pAup->pVPS[vpsid].vps_max_layers_minus1;
  (void)uMaxLayersMinus1;
  pAup->pActiveSPS = &pAup->pSPS[spsid];

  if(!pCtx->bAreFirstStreamSettingsChecked)
  {
    bool bDisableMinimalResolutionCheck = false;
    AL_ERR ret = !pCtx->bAreBuffersAllocated ?
                 isSPSCompatibleWithHardware(pSlice->pSPS, bDisableMinimalResolutionCheck) :
                 isSPSCompatibleWithInitialStreamSettings(pCtx, pSlice->pSPS);

    if(!AL_IS_SUCCESS_CODE(ret))
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      pSlice->pPPS = &pAup->pPPS[pCtx->tConceal.iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
      AL_Default_Decoder_SetError(pCtx, ret, -1, true);
      return false;
    }

    ret = isPPSCompatibleWidthCurrentDecodingMode(pCtx, pSlice->pPPS);

    if(!AL_IS_SUCCESS_CODE(ret))
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode this tile pattern with selected decoding mode\n");
      pSlice->pPPS = &pAup->pPPS[pCtx->tConceal.iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
      AL_Default_Decoder_SetError(pCtx, ret, -1, true);
      return false;
    }

    if(!pCtx->bAreBuffersAllocated)
    {
      extractStreamSettings(pCtx, pSlice->pSPS, &pCtx->tCurrentStreamSettings);
      pCtx->tInitialStreamSettings = pCtx->tCurrentStreamSettings;
    }

    pCtx->bAreFirstStreamSettingsChecked = true;
    pCtx->bStillPictureProfile = AL_IS_STILL_PROFILE(pCtx->tCurrentStreamSettings.eProfile);
    pCtx->bIntraOnlyProfile = pCtx->bStillPictureProfile || AL_IS_INTRA_PROFILE(pCtx->tCurrentStreamSettings.eProfile);

    if(!pCtx->bAreBuffersAllocated)
      if(!allocateBuffers(pCtx, pSlice->pSPS))
        return false;

    if(!initChannel(pCtx, pSlice->pSPS))
      return false;

    pCtx->bAreBuffersAllocated = true;
  }

  pAup->pActiveSPS = &pAup->pSPS[spsid];
  pAup->pActivePPS = pSlice->pPPS;

  const AL_TDimension tDim = { pSlice->pSPS->pic_width_in_luma_samples, pSlice->pSPS->pic_height_in_luma_samples };
  const int32_t iLevel = pSlice->pSPS->profile_and_level.general_level_idc / 3;
  const int32_t iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(iLevel, tDim.iWidth, tDim.iHeight, false, false, false);
  const int32_t iDpbRef = Min(pSlice->pSPS->sps_max_dec_pic_buffering_minus1[pSlice->pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
  AL_Dpb_SetNumRef(pDpb, iDpbRef);

  if(!pSlice->dependent_slice_segment_flag)
  {
    if(pSlice->slice_type != AL_SLICE_I && (pIAup->iRecoveryCnt == 0) && !AL_HEVC_Dpb_HasPictInDPB(pDpb) && pSlice->nuh_layer_id == 0)
      return false;

    if(!pCtx->tConceal.bValidFrame)
      pSlice->iCurrPOC = calculatePOC(pCtx, pSlice, pCtx->uNoRaslOutputFlag);
    else
      pSlice->iCurrPOC = pDpb->iCurFramePOC;

    if(!pCtx->tConceal.bValidFrame && !pSlice->RefListBuilt)
      AL_HEVC_Dpb_InitRefPictSet(pDpb, pSlice);

    if(!pCtx->tConceal.bValidFrame)
    {
      /* at least one active reference on inter slice */
      if(pSlice->slice_type != AL_SLICE_I && !pSlice->NumPicTotalCurr && !AL_HEVC_Dpb_HasPictInDPB(pDpb) && pSlice->nuh_layer_id == 0)
        return false;
    }
  }

  return true;
}

/*****************************************************************************/
static void copyScalingList(AL_TSCLParam const* pScalingListParam, AL_TScalingList* pSCL)
{
  Rtos_Memcpy((*pSCL)[0].t4x4Y, pScalingListParam->UseDefaultScalingMatrixFlag[0] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pScalingListParam->ScalingList[0][0], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cb, pScalingListParam->UseDefaultScalingMatrixFlag[1] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pScalingListParam->ScalingList[0][1], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cr, pScalingListParam->UseDefaultScalingMatrixFlag[2] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pScalingListParam->ScalingList[0][2], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Y, pScalingListParam->UseDefaultScalingMatrixFlag[3] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pScalingListParam->ScalingList[0][3], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cb, pScalingListParam->UseDefaultScalingMatrixFlag[4] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pScalingListParam->ScalingList[0][4], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cr, pScalingListParam->UseDefaultScalingMatrixFlag[5] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pScalingListParam->ScalingList[0][5], 16);

  Rtos_Memcpy((*pSCL)[0].t8x8Y, pScalingListParam->UseDefaultScalingMatrixFlag[6] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[1][0], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cb, pScalingListParam->UseDefaultScalingMatrixFlag[7] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[1][1], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cr, pScalingListParam->UseDefaultScalingMatrixFlag[8] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[1][2], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Y, pScalingListParam->UseDefaultScalingMatrixFlag[9] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[1][3], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cb, pScalingListParam->UseDefaultScalingMatrixFlag[10] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[1][4], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cr, pScalingListParam->UseDefaultScalingMatrixFlag[11] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[1][5], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Y, pScalingListParam->UseDefaultScalingMatrixFlag[12] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[2][0], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cb, pScalingListParam->UseDefaultScalingMatrixFlag[13] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[2][1], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cr, pScalingListParam->UseDefaultScalingMatrixFlag[14] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[2][2], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Y, pScalingListParam->UseDefaultScalingMatrixFlag[15] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[2][3], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cb, pScalingListParam->UseDefaultScalingMatrixFlag[16] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[2][4], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cr, pScalingListParam->UseDefaultScalingMatrixFlag[17] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[2][5], 64);

  Rtos_Memcpy((*pSCL)[0].t32x32, pScalingListParam->UseDefaultScalingMatrixFlag[18] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pScalingListParam->ScalingList[3][0], 64);

  Rtos_Memcpy((*pSCL)[1].t32x32, pScalingListParam->UseDefaultScalingMatrixFlag[19] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pScalingListParam->ScalingList[3][3], 64);

  (*pSCL)[0].tDC[0] = pScalingListParam->UseDefaultScalingMatrixFlag[12] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][0];
  (*pSCL)[0].tDC[1] = pScalingListParam->UseDefaultScalingMatrixFlag[13] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][1];
  (*pSCL)[0].tDC[2] = pScalingListParam->UseDefaultScalingMatrixFlag[14] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][2];
  (*pSCL)[0].tDC[3] = pScalingListParam->UseDefaultScalingMatrixFlag[18] ? 16 : pScalingListParam->scaling_list_dc_coeff[1][0];
  (*pSCL)[1].tDC[0] = pScalingListParam->UseDefaultScalingMatrixFlag[15] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][3];
  (*pSCL)[1].tDC[1] = pScalingListParam->UseDefaultScalingMatrixFlag[16] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][4];
  (*pSCL)[1].tDC[2] = pScalingListParam->UseDefaultScalingMatrixFlag[17] ? 16 : pScalingListParam->scaling_list_dc_coeff[0][5];
  (*pSCL)[1].tDC[3] = pScalingListParam->UseDefaultScalingMatrixFlag[19] ? 16 : pScalingListParam->scaling_list_dc_coeff[1][3];
}

/*****************************************************************************/
static bool isFirstSliceSegmentInPicture(AL_THevcSliceHdr* pSlice)
{
  return pSlice->first_slice_segment_in_pic_flag == 1;
}

/*****************************************************************************/
static void processScalingList(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice, AL_TScalingList* pScl)
{
  (void)pCtx;

  AL_CleanupMemory(pScl, sizeof(*pScl));

  if(!isFirstSliceSegmentInPicture(pSlice))
    return;

  AL_TSCLParam const* pSclParam;

  // Save ScalingList
  if(pSlice->pPPS->pps_scaling_list_data_present_flag)
    pSclParam = &pSlice->pPPS->scaling_list_param;
  else
    pSclParam = &pSlice->pSPS->scaling_list_param;

  copyScalingList(pSclParam, pScl);
}

/*****************************************************************************/
extern void AL_HEVC_SetDefaultSliceHeader(AL_THevcSliceHdr* pSlice);

/*****************************************************************************/
void AL_HEVC_ClearSliceHeader(AL_THevcSliceHdr* pSlice)
{
  uint8_t first_slice_segment_in_pic_flag = pSlice->first_slice_segment_in_pic_flag;
  uint8_t no_output_prior_pics_flag = pSlice->no_output_of_prior_pics_flag;
  uint8_t pic_parameter_set_id = pSlice->slice_pic_parameter_set_id;
  uint8_t nal_unit_type = pSlice->nal_unit_type;
  uint8_t nuh_layer_id = pSlice->nuh_layer_id;
  uint8_t nuh_temporal_id_plus1 = pSlice->nuh_temporal_id_plus1;
  uint8_t RapFlag = pSlice->RapPicFlag;
  uint8_t IdrFlag = pSlice->IdrPicFlag;
  int32_t SliceSegAddr = pSlice->slice_segment_address;
  uint16_t NumEntryPointoffsets = pSlice->num_entry_point_offsets;
  AL_THevcPps const* pPPS = pSlice->pPPS;
  AL_THevcSps* pSPS = pSlice->pSPS;

  AL_HEVC_SetDefaultSliceHeader(pSlice);

  pSlice->first_slice_segment_in_pic_flag = first_slice_segment_in_pic_flag;
  pSlice->no_output_of_prior_pics_flag = no_output_prior_pics_flag;
  pSlice->slice_pic_parameter_set_id = pic_parameter_set_id;
  pSlice->dependent_slice_segment_flag = 0;
  pSlice->nal_unit_type = nal_unit_type;
  pSlice->nuh_layer_id = nuh_layer_id;
  pSlice->nuh_temporal_id_plus1 = nuh_temporal_id_plus1;
  pSlice->RapPicFlag = RapFlag;
  pSlice->IdrPicFlag = IdrFlag;
  pSlice->slice_segment_address = SliceSegAddr;
  pSlice->num_entry_point_offsets = NumEntryPointoffsets;
  pSlice->pPPS = pPPS;
  pSlice->pSPS = pSPS;
}

/*****************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam, AL_THevcSliceHdr* pSlice, bool bSliceHdrValid)
{
  if(!bSliceHdrValid)
    AL_HEVC_ClearSliceHeader(pSlice);

  pSlice->slice_type = AL_SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT, pPicParam->tBufIDs.tFrmID, true);

  AL_HEVC_FillPictParameters(pSlice, pCtx, pPicParam);
  AL_HEVC_FillSliceParameters(pSlice, pCtx, pSliceParam);

  AL_SetConcealParameters(pCtx, pSliceParam);

  if(bSliceHdrValid)
    pSliceParam->uFirstLcuSliceSegment = pSliceParam->uFirstLcuSlice = pSliceParam->uSliceFirstLcu = pSlice->slice_segment_address;

  AL_SET_DEC_OPT(pPicParam, IntraOnly, 0);
}

/*****************************************************************************/
static void createConcealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam, AL_THevcSliceHdr* pSlice)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  concealSlice(pCtx, pPicParam, pSliceParam, pSlice, false);
  pSliceParam->uFirstLcuSliceSegment = 0;
  pSliceParam->uFirstLcuSlice = 0;
  pSliceParam->uSliceFirstLcu = 0;
  pSliceParam->uNextSliceSegment = pSlice->slice_segment_address;
  pSliceParam->uSliceNumLcu = pSlice->slice_segment_address;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void reallyEndFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, bool bPicOutputFlag, bool bHasPreviousSlice)
{
  AL_HEVC_PictMngr_EndFrame(&pCtx->PictMngr, pSlice->slice_pic_order_cnt_lsb, eNUT, pSlice, bPicOutputFlag);

  if(pCtx->pChanParam->eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);

  if(pCtx->pChanParam->eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, true, bHasPreviousSlice);

  UpdateContextAtEndOfFrame(pCtx);
}

static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, bool bPicOutputFlag)
{
  reallyEndFrame(pCtx, eNUT, pSlice, bPicOutputFlag, true);
}

static void endFrameConceal(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, bool bPicOutputFlag)
{
  reallyEndFrame(pCtx, eNUT, pSlice, bPicOutputFlag, false);
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[pCtx->uCurID];
  AL_TDecPicParam* pPicParam = &pCtx->tPoolPicParams[pCtx->uToggle];
  AL_TDecSliceParam* pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->tCurrentFrameCtx.uNumSlice - 1]);
  AL_TDecBuffers* pPicBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];

  AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, true, false);

  // copy stream offset from previous command
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = pCtx->iStreamOffset[(pCtx->iNumFrmBlk1 + pCtx->iStackSize - 1) % pCtx->iStackSize];

  /* The slice is its own previous slice as we changed it in the last slice
   * This means that in AL_VCL_NAL_UNIT we don't want to send a previous slice at all. */
  endFrameConceal(pCtx, pSlice->nal_unit_type, pSlice, pSlice->pic_output_flag);

  AL_DecFrameCtx_Reset(&pCtx->tCurrentFrameCtx);
}

/*****************************************************************************/
static bool isValidSyncPoint(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_ESliceType ePicType, int32_t iRecoveryCnt)
{
  if(isRandomAccessPoint(eNUT))
    return true;

  if(iRecoveryCnt != 0)
    return true;

  if(pCtx->bUseIFramesAsSyncPoint && eNUT == AL_HEVC_NUT_TRAIL_R && ePicType == AL_SLICE_I)
    return true;

  return false;
}

/*****************************************************************************/
static bool hevcInitFrameBuffers(AL_TDecCtx* pCtx, bool bStartsNewCVS, const AL_THevcSps* pSPS, AL_TDecPicParam* pPicParam, AL_TDecBuffers* pPicBuffers)
{
  (void)bStartsNewCVS;
  AL_TDimension const tDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  if(!AL_InitFrameBuffers(pCtx, pPicBuffers, bStartsNewCVS, tDim, (AL_EChromaMode)pSPS->chroma_format_idc, pPicParam))
    return false;

  AL_TBuffer* pDispBuf = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, pPicParam->tBufIDs.tFrmID);
  AL_THDRMetaData* pMeta = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pDispBuf, AL_META_TYPE_HDR);

  if(pMeta != NULL)
  {
    AL_THDRSEIs* pHDRSEIs = &pCtx->aup.tParsedHDRSEIs;
    pMeta->eColourDescription = AL_H273_ColourPrimariesToColourDesc(pSPS->vui_param.colour_primaries);
    pMeta->eTransferCharacteristics = AL_VUIValueToTransferCharacteristics(pSPS->vui_param.transfer_characteristics);
    pMeta->eColourMatrixCoeffs = AL_VUIValueToColourMatrixCoefficients(pSPS->vui_param.matrix_coefficients);
    AL_HDRSEIs_Copy(pHDRSEIs, &pMeta->tHDRSEIs);
    AL_HDRSEIs_Reset(pHDRSEIs);
  }

  return true;
}

/*****************************************************************************/
static void hevcGetCropInfo(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS, AL_TCropInfo* pCropInfo)
{
  (void)pCtx;

  AL_HEVC_GetCropInfo(pSPS, pCropInfo);
}

/*****************************************************************************/
static bool decodeSliceCheckItBelongs(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, bool* isValid)
{
  AL_TDecFrameCtx* pFrmCtx = &pCtx->tCurrentFrameCtx;
  // AL_TDecPicParam* pPicParam = &pCtx->tPoolPicParams[pCtx->uToggle];
  bool bSliceBelongsToSameFrame = true;
  AL_TConceal* pConceal = &pCtx->tConceal;

  if((pSlice->slice_pic_order_cnt_lsb != pCtx->iCurPocLsb) && !isFirstSliceSegmentInPicture(pSlice))
    bSliceBelongsToSameFrame = false;
  else if((pSlice->slice_segment_address <= pConceal->iFirstLCU) && !pSlice->pPPS->tiles_enabled_flag && !pSlice->pPPS->entropy_coding_sync_enabled_flag)
  {
    if(!(AL_HEVC_IsIDR(eNUT)))
      *isValid = false;
    else
      bSliceBelongsToSameFrame = false;
  }
  else if(!isFirstSliceSegmentInPicture(pSlice)) // AL_GET_DEC_OPT(pPicParam, Tile) ?
  {
    /* check slice_segment_address not repeated, can't check in order as hevc allows out of order, bounded range */
    for(int32_t i = 0; i < (pFrmCtx->uNumSlice % AL_MAX_FIRST_LCU_POS_CACHE); i++)
    {
      if(pConceal->iFirstLCUStore[i] == pSlice->slice_segment_address)
      {
        Rtos_Log(AL_LOG_ERROR, "decodeSliceData Invalid slice_segment_address=%d against slice(%d) slice_segment_address of %d\n", pSlice->slice_segment_address, i, pConceal->iFirstLCUStore[i]);
        bSliceBelongsToSameFrame = false;
        break;
      }
    }
  }

  return bSliceBelongsToSameFrame;
}

static bool IsSliceSegAddrValid(AL_THevcSliceHdr const* pSlice)
{
  int iTileCount = (pSlice->pPPS->num_tile_columns_minus1 + 1) * (pSlice->pPPS->num_tile_rows_minus1 + 1);

  for(int i = 0; i < iTileCount; i++)
  {
    if(pSlice->pPPS->TileTopology[i] == (uint32_t)pSlice->slice_segment_address)
    {
      return true;
    }
  }

  Rtos_Log(AL_LOG_ERROR, "decodeSliceData: Invalid slice_segment_address=%d - Does not match TileTopology\n", pSlice->slice_segment_address);
  return false;
}

/*****************************************************************************/
static bool decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int32_t* iNumSlice)
{
  AL_TDecFrameCtx* pFrmCtx = &pCtx->tCurrentFrameCtx;

  if(pFrmCtx->bFirstSliceValid && *iNumSlice > pCtx->pChanParam->iMaxSlices)
    return false;

  // ignore RASL picture associated with an IRAP picture that has NoRaslOutputFlag = 1
  if(AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag)
  {
    if(bIsLastAUNal)
    {
      if(pFrmCtx->eBufStatus & DEC_FRAME_BUF_RESERVED)
        AL_CancelFrameBuffers(pCtx);
      else
        UpdateContextAtEndOfFrame(pCtx);
    }
    return false;
  }

  bool const bIsRAP = isRandomAccessPoint(eNUT);

  if(bIsRAP)
    pCtx->uNoRaslOutputFlag = (pCtx->bIsFirstPicture || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT)) ? 1 : 0;

  AL_THevcAup* pAUP = &pIAUP->hevcAup;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;

  AL_TCircBuffer* pBufStream = pCtx->pStream;
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pCtx->BufNoAE.tMD.pVirtualAddr, pCtx->BufNoAE.tMD.uSize, true, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->uCurID) & 0x01;
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[uToggleID];
  AL_TConceal* pConceal = &pCtx->tConceal;
  AL_THevcRefListBuilderCB* pRefListBuilderCB = NULL;

  bool isSliceHdrValid = AL_HEVC_ParseSliceHeader(pSlice, &pCtx->HevcSliceHdr[pCtx->uCurID], &rp, pConceal, pAUP->pPPS, pRefListBuilderCB);
  bool bSliceBelongsToSameFrame = true;

  if((!isSliceHdrValid || !pSlice->pPPS) && pConceal->iLastPPSId >= 0)
    pSlice->pPPS = &pAUP->pPPS[pConceal->iLastPPSId];

  if(!isSliceHdrValid || !pSlice->pSPS)
    pSlice->pSPS = pAUP->pActiveSPS;

  if(pSlice->pPPS && pSlice->pPPS->tiles_enabled_flag)
    pCtx->pChanParam->uNumTileColumns = pSlice->pPPS->num_tile_columns_minus1 + 1;
  else
    pCtx->pChanParam->uNumTileColumns = 1;

  if(isSliceHdrValid && (pSlice->pPPS->tiles_enabled_flag && pSlice->num_entry_point_offsets))
    isSliceHdrValid = IsSliceSegAddrValid(pSlice);

  bool isValid = isSliceHdrValid;

  if(isValid)
  {
    bSliceBelongsToSameFrame = decodeSliceCheckItBelongs(pCtx, eNUT, pSlice, &isValid);
  }
  else
  {
    Rtos_Log(AL_LOG_ERROR, "Return from AL_HEVC_ParseSliceHeader() Invalid SliceNum=%d\n", *iNumSlice);
  }

  if(isValid)
  {
    pConceal->iFirstLCU = pSlice->slice_segment_address; // @todo drop this member and use only the array
    pConceal->iFirstLCUStore[pFrmCtx->uNumSlice % AL_MAX_FIRST_LCU_POS_CACHE] = pSlice->slice_segment_address;
  }

  bool* bFirstIsValid = &pCtx->bFirstIsValid;

  if(!bSliceBelongsToSameFrame && AL_Default_Decoder_HasOngoingFrame(pCtx))
  {
    finishPreviousFrame(pCtx);

    if(pCtx->eInputMode == AL_DEC_SPLIT_INPUT)
    {
      pConceal->bSkipRemainingNals = true;
      return true;
    }
  }

  AL_TDecPicParam* pPicParam = &pCtx->tPoolPicParams[pCtx->uToggle];

  if(isValid && pCtx->bAreBuffersAllocated)
  {
    AL_THevcSps* pSPS = pSlice->pSPS;
    AL_ERR const ret = isSPSCompatibleWithInitialStreamSettings(pCtx, pSPS);
    isValid = AL_IS_SUCCESS_CODE(ret);
    AL_TStreamSettings spsSettings;

    if(isValid)
    {
      spsSettings.bDecodeIntraOnly = pCtx->tCurrentStreamSettings.bDecodeIntraOnly;
      extractStreamSettings(pCtx, pSPS, &spsSettings);
      // get value from pre alloc
      spsSettings.iMaxRef = pCtx->tCurrentStreamSettings.iMaxRef;
    }

    if(!isValid)
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      AL_Default_Decoder_SetError(pCtx, ret, pPicParam->tBufIDs.tFrmID, true);
      pSPS->bConceal = true;
    }
    else if(spsSettings.tDim.iWidth != pStreamSettings->tDim.iWidth || spsSettings.tDim.iHeight != pStreamSettings->tDim.iHeight)
    {
      AL_TCropInfo tCropInfo;
      AL_HEVC_GetCropInfo(pSPS, &tCropInfo);
      AL_ERR error = resolutionFound(pCtx, &spsSettings, &tCropInfo);

      if(!AL_IS_SUCCESS_CODE(error))
      {
        Rtos_Log(AL_LOG_ERROR, "ResolutionFound callback returns with error\n");
        AL_Default_Decoder_SetError(pCtx, AL_WARN_RES_FOUND_CB, pPicParam->tBufIDs.tFrmID, false);
        pSPS->bConceal = true;
        isValid = false;
      }
      pCtx->tCurrentStreamSettings = spsSettings;
    }
  }

  if(isValid)
  {
    pCtx->iCurPocLsb = pSlice->slice_pic_order_cnt_lsb;
    isValid = initSlice(pCtx, pSlice);
  }

  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

  if(!isValid)
  {
    if(!*bFirstIsValid)
    {
      if(bIsLastAUNal)
      {
        if(pFrmCtx->eBufStatus & DEC_FRAME_BUF_RESERVED)
          AL_CancelFrameBuffers(pCtx);
        else
          UpdateContextAtEndOfFrame(pCtx);
      }
      return false;
    }
    AL_HEVC_Dpb_RemoveHeadFrame(pDpb);
  }

  if(isValid && isFirstSliceSegmentInPicture(pSlice) && pFrmCtx->bFirstSliceValid)
    isValid = false;

  if(isValid && pSlice->slice_type != AL_SLICE_I)
    AL_SET_DEC_OPT(pPicParam, IntraOnly, 0);

  pCtx->uCurID = (pCtx->uCurID + 1) & 1;

  AL_TDecSliceParam* pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[pFrmCtx->uNumSlice]);

  AL_TDecBuffers* pPicBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];
  pPicBuffers->tStream.tMD = pCtx->pStream->tMD;

  if(isValid)
  {
    if(isFirstSliceSegmentInPicture(pSlice) && pFrmCtx->eBufStatus == DEC_FRAME_BUF_NONE)
    {
      bool bClearRef = (bIsRAP && pCtx->uNoRaslOutputFlag); // IRAP picture with NoRaslOutputFlag = 1
      bool bNoOutputPrior = (AL_HEVC_IsCRA(eNUT) || ((AL_HEVC_IsIDR(eNUT) || AL_HEVC_IsBLA(eNUT)) && pSlice->no_output_of_prior_pics_flag));

      if(pStreamSettings->bDecodeIntraOnly)
        bClearRef = true;
      AL_HEVC_PictMngr_ClearDPB(&pCtx->PictMngr, pSlice->pSPS, bClearRef, bNoOutputPrior);
    }

    if(pSlice->slice_type != AL_SLICE_I && (pIAUP->iRecoveryCnt == 0) && !AL_HEVC_Dpb_HasPictInDPB(pDpb) && pSlice->nuh_layer_id == 0)
      isValid = false;
    else if(isValid && !pFrmCtx->bFirstSliceValid && pSlice->slice_segment_address)
    {
      if(pSlice->slice_segment_address <= (int)pSliceParam->uNextSliceSegment && !pStreamSettings->bDecodeIntraOnly)
      {
        createConcealSlice(pCtx, pPicParam, pSliceParam, pSlice);

        pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[++pFrmCtx->uNumSlice]);
        pFrmCtx->bFirstSliceValid = true;
      }
      else
        isValid = false;
    }
  }

  if(pCtx->bAreBuffersAllocated && pFrmCtx->eBufStatus == DEC_FRAME_BUF_NONE && pSlice->pSPS)
  {
    if(!hevcInitFrameBuffers(pCtx, bIsRAP, pSlice->pSPS, pPicParam, pPicBuffers))
      return false;

    AL_TCropInfo tCropInfo;
    hevcGetCropInfo(pCtx, pSlice->pSPS, &tCropInfo);
    AL_HEVC_PictMngr_UpdateRecInfo(&pCtx->PictMngr, &tCropInfo, pAUP->ePicStruct);
  }

  bool bLastSlice = *iNumSlice >= pCtx->pChanParam->iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    isValid = false;

  AL_TScalingList ScalingList = { 0 };

  pFrmCtx->bIsIntraOnly &= pSlice->slice_type == AL_SLICE_I;

  if(bIsLastAUNal && !pFrmCtx->bIsIntraOnly)
  {
    if(pCtx->bIntraOnlyProfile)
      AL_Default_Decoder_SetError(pCtx, AL_WARN_UNEXPECTED_SLICE_IN_INTRA_PROFILE, pPicParam->tBufIDs.tFrmID, true);

    isValid &= !(pCtx->bIntraOnlyProfile || pCtx->tCurrentStreamSettings.bDecodeIntraOnly);
  }

  if(isValid)
  {
    if(!(*bFirstIsValid))
    {
      if(!isValidSyncPoint(pCtx, eNUT, pSlice->slice_type, pIAUP->iRecoveryCnt))
      {
        pFrmCtx->eBufStatus = DEC_FRAME_BUF_NONE;
        AL_CancelFrameBuffers(pCtx);
        return false;
      }
      *bFirstIsValid = true;
    }

    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pCtx, pSlice, &ScalingList);

    if(pFrmCtx->uNumSlice == 0)
      AL_HEVC_FillPictParameters(pSlice, pCtx, pPicParam);
    AL_HEVC_FillSliceParameters(pSlice, pCtx, pSliceParam);

    if(!pSlice->RefListBuilt)
    {
      pSlice->RefListBuilt = AL_HEVC_Dpb_BuildPictureList(pDpb, pSlice, &pCtx->ListRef);
    }

    if(!pSlice->RefListBuilt && (pIAUP->iRecoveryCnt == 0))
    {
      concealSlice(pCtx, pPicParam, pSliceParam, pSlice, isSliceHdrValid);
    }
    else
    {
      pConceal->bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSliceParam);
    }
  }
  else if((bIsLastAUNal || isFirstSliceSegmentInPicture(pSlice) || bLastSlice) &&
          (*bFirstIsValid) && pFrmCtx->bFirstSliceValid &&
          !(pStreamSettings->bDecodeIntraOnly && !pFrmCtx->bIsIntraOnly)) /* conceal the current slice data */
  {
    concealSlice(pCtx, pPicParam, pSliceParam, pSlice, isSliceHdrValid);

    if(bLastSlice)
      pSliceParam->uNextSliceSegment = DecPicParam_GetNumLcuInFrame(pPicParam);
  }
  else // skip slice
  {
    if(bIsLastAUNal)
    {
      if(pFrmCtx->eBufStatus & DEC_FRAME_BUF_RESERVED)
      {
        AL_CancelFrameBuffers(pCtx);
        pFrmCtx->bIsIntraOnly = true;
      }
      else
        UpdateContextAtEndOfFrame(pCtx);
    }

    return false;
  }

  if(isValid && isFirstSliceSegmentInPicture(pSlice))
    pFrmCtx->bFirstSliceValid = true;

  // Launch slice decoding
  AL_HEVC_PrepareCommand(pCtx, &ScalingList, pPicParam, pPicBuffers, pSliceParam, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pFrmCtx->uNumSlice;
  ++(*iNumSlice);

  if(bIsLastAUNal || bLastSlice)
  {
    bool bPicOutputFlag = (AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag) ? 0 : pSlice->pic_output_flag;
    endFrame(pCtx, eNUT, pSlice, bPicOutputFlag);
    return true;
  }

  if(pCtx->pChanParam->eDecUnit == AL_VCL_NAL_UNIT)
  {
    AL_LaunchSliceDecoding(pCtx, false, true);
    return true;
  }

  return false;
}

/*****************************************************************************/
static bool isSliceData(AL_ENut nut)
{
  switch(nut)
  {
  case AL_HEVC_NUT_TRAIL_N:
  case AL_HEVC_NUT_TRAIL_R:
  case AL_HEVC_NUT_TSA_N:
  case AL_HEVC_NUT_TSA_R:
  case AL_HEVC_NUT_STSA_N:
  case AL_HEVC_NUT_STSA_R:
  case AL_HEVC_NUT_RADL_N:
  case AL_HEVC_NUT_RADL_R:
  case AL_HEVC_NUT_RASL_N:
  case AL_HEVC_NUT_RASL_R:
  case AL_HEVC_NUT_BLA_W_LP:
  case AL_HEVC_NUT_BLA_W_RADL:
  case AL_HEVC_NUT_BLA_N_LP:
  case AL_HEVC_NUT_IDR_W_RADL:
  case AL_HEVC_NUT_IDR_N_LP:
  case AL_HEVC_NUT_CRA:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
static AL_EParseResult parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  uint16_t PpsId;
  AL_EParseResult result = AL_HEVC_ParsePPS(IAup, rp, &PpsId);

  if(PpsId >= AL_HEVC_MAX_PPS)
    return AL_PARSE_UNSUPPORTED;

  AL_THevcAup* aup = &IAup->hevcAup;

  aup->pPPS[PpsId].bConceal = (result != AL_PARSE_OK);

  if(!aup->pPPS[PpsId].bConceal)
  {
    pCtx->tConceal.bHasPPS = true;

    if(pCtx->tConceal.iLastPPSId <= PpsId)
      pCtx->tConceal.iLastPPSId = PpsId;
  }

  return result;
}

/*****************************************************************************/
static bool isActiveSPSChanging(AL_THevcSps* pNewSPS, AL_THevcSps* pActiveSPS)
{
  // Only check resolution change - but any change is forgiven, according to the specification
  return pActiveSPS != NULL &&
         pNewSPS->sps_seq_parameter_set_id == pActiveSPS->sps_seq_parameter_set_id &&
         (pNewSPS->pic_height_in_luma_samples != pActiveSPS->pic_height_in_luma_samples ||
          pNewSPS->pic_width_in_luma_samples != pActiveSPS->pic_width_in_luma_samples);
}

/*****************************************************************************/
static AL_EParseResult parseAndApplySPS(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_TDecCtx* pCtx)
{
  AL_THevcSps tNewSPS;
  AL_HEVC_InitSps(&tNewSPS);
  tNewSPS.pVPS = pIAup->hevcAup.pVPS;
  AL_EParseResult eParseResult = AL_HEVC_ParseSPS(pRP, &tNewSPS);

  if(eParseResult != AL_PARSE_BAD_ID)
  {
    if(eParseResult == AL_PARSE_OK)
    {
      if(AL_Default_Decoder_HasOngoingFrame(pCtx) && isActiveSPSChanging(&tNewSPS, pIAup->hevcAup.pActiveSPS))
      {
        // An active SPS should not be modified unless it is the end of the CVS (spec 7.4.2.4).
        // So we consider we received the full frame.
        finishPreviousFrame(pCtx);

        eParseResult = AL_PARSER_LAUNCHED_OK;
      }

      pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id] = tNewSPS;
    }
    else
      pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id].bConceal = true;
  }
  return eParseResult;
}

/*****************************************************************************/
static AL_NonVclNuts getNonVclNuts(void)
{
  AL_NonVclNuts nuts =
  {
    AL_HEVC_NUT_ERR, // NUT does not exist in HEVC
    AL_HEVC_NUT_VPS,
    AL_HEVC_NUT_SPS,
    AL_HEVC_NUT_PPS,
    AL_HEVC_NUT_FD,
    AL_HEVC_NUT_ERR, // NUT does not exist in HEVC
    AL_HEVC_NUT_ERR, // NUT does not exist in HEVC
    AL_HEVC_NUT_ERR, // NUT does not exist in HEVC
    AL_HEVC_NUT_PREFIX_SEI,
    AL_HEVC_NUT_SUFFIX_SEI,
    AL_HEVC_NUT_EOS,
    AL_HEVC_NUT_EOB,
  };
  return nuts;
}

/*****************************************************************************/
static bool isNutError(AL_ENut nut)
{
  if(nut >= AL_HEVC_NUT_ERR)
    return true;
  return false;
}

/*****************************************************************************/
static bool canNalBeReordered(AL_ENut nut)
{
  switch(nut)
  {
  case AL_HEVC_NUT_SUFFIX_SEI:
  case AL_HEVC_NUT_RSV_NVCL45:
  case AL_HEVC_NUT_RSV_NVCL46:
  case AL_HEVC_NUT_RSV_NVCL47:
  case AL_HEVC_NUT_UNSPEC_56:
  case AL_HEVC_NUT_UNSPEC_57:
  case AL_HEVC_NUT_UNSPEC_58:
  case AL_HEVC_NUT_UNSPEC_59:
  case AL_HEVC_NUT_UNSPEC_60:
  case AL_HEVC_NUT_UNSPEC_61:
  case AL_HEVC_NUT_UNSPEC_62:
  case AL_HEVC_NUT_UNSPEC_63:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
void AL_HEVC_InitParser(AL_NalParser* pParser)
{
  pParser->parseDps = NULL;
  pParser->parseVps = AL_HEVC_ParseVPS;
  pParser->parseSps = parseAndApplySPS;
  pParser->parsePps = parsePPSandUpdateConcealment;
  pParser->parseAps = NULL;
  pParser->parsePh = NULL;
  pParser->parseSei = AL_HEVC_ParseSEI;
  pParser->parseOtherNal = NULL;
  pParser->decodeSliceData = decodeSliceData;
  pParser->isSliceData = isSliceData;
  pParser->finishPendingRequest = finishPreviousFrame;
  pParser->getNonVclNuts = getNonVclNuts;
  pParser->isNutError = isNutError;
  pParser->canNalBeReordered = canNalBeReordered;
}

/*****************************************************************************/
void AL_HEVC_InitAUP(AL_THevcAup* pAUP)
{
  for(int32_t i = 0; i < AL_HEVC_MAX_PPS; ++i)
    pAUP->pPPS[i].bConceal = true;

  for(int32_t i = 0; i < AL_HEVC_MAX_SPS; ++i)
    pAUP->pSPS[i].bConceal = true;

  pAUP->pActiveSPS = NULL;
}

/*****************************************************************************/

AL_ERR CreateHevcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  pSettings->tStream.iMaxRef = Min(AL_MAX_REF, pSettings->tStream.iMaxRef);
  AL_ERR errorCode = AL_CreateDefaultDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  if(!AL_IS_ERROR_CODE(errorCode))
  {
    AL_TDecoder* pDec = *hDec;
    AL_TDecCtx* pCtx = &pDec->ctx;

    AL_HEVC_InitParser(&pCtx->nalParser);
  }

  return errorCode;
}
