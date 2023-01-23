/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "FrameParam.h"
#include "I_DecoderCtx.h"
#include "I_DecSchedulerInfo.h"
#include "DefaultDecoder.h"
#include "SliceDataParsing.h"
#include "NalUnitParserPrivate.h"

#include "lib_common/Utils.h"
#include "lib_common/HevcLevelsLimit.h"
#include "lib_common/HevcUtils.h"
#include "lib_common/Error.h"
#include "lib_common/SyntaxConversion.h"
#include "lib_common/HardwareConfig.h"

#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/Defines_mcu.h"
#include "lib_common_dec/HDRMeta.h"

#include "lib_parsing/HevcParser.h"
#include "lib_parsing/Hevc_PictMngr.h"
#include "lib_parsing/Hevc_SliceHeaderParsing.h"

#include "lib_assert/al_assert.h"

/*************************************************************************/
static uint8_t getMaxRextBitDepth(AL_THevcProfilevel pf)
{
  if(pf.general_max_8bit_constraint_flag)
    return 8;

  if(pf.general_max_10bit_constraint_flag)
    return 10;

  if(pf.general_max_12bit_constraint_flag)
    return 12;
  return 16;
}

/*************************************************************************/
static int getMaxBitDepthFromProfile(AL_THevcProfilevel pf)
{
  if(pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt))
    return getMaxRextBitDepth(pf);

  if((pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)) || (pf.general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)] == 1))
    return 8;

  if((pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)) || (pf.general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)] == 1))
    return 8;

  return 10;
}

/*************************************************************************/
static int getMaxBitDepth(AL_THevcSps const* pSPS)
{
  int iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;
  int iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;
  int iMaxSPSBitDepth = Max(iSPSLumaBitDepth, iSPSChromaBitDepth);
  int iMaxBitDepth = iMaxSPSBitDepth;

  iMaxBitDepth = Max(iMaxSPSBitDepth, getMaxBitDepthFromProfile(pSPS->profile_and_level));

  if((iMaxBitDepth % 2) != 0)
    iMaxBitDepth++;
  return iMaxBitDepth;
}

/*****************************************************************************/
static int calculatePOC(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr* pSlice, uint8_t uNoRasOutputFlag)
{
  int32_t POCMsb = 0;
  uint32_t MaxPOCLsb = pSlice->pSPS->MaxPicOrderCntLsb;
  uint32_t MaxPOCLsb_div2 = MaxPOCLsb >> 1;

  if((!AL_HEVC_IsBLA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsCRA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsIDR(pSlice->nal_unit_type)) || !uNoRasOutputFlag)
  {
    if((pSlice->slice_pic_order_cnt_lsb < pCtx->uPrevPocLSB) &&
       ((pCtx->uPrevPocLSB - pSlice->slice_pic_order_cnt_lsb) >= MaxPOCLsb_div2))
      POCMsb = pCtx->iPrevPocMSB + MaxPOCLsb;

    else if((pSlice->slice_pic_order_cnt_lsb > pCtx->uPrevPocLSB) &&
            ((pSlice->slice_pic_order_cnt_lsb - pCtx->uPrevPocLSB) > MaxPOCLsb_div2))
      POCMsb = pCtx->iPrevPocMSB - MaxPOCLsb;

    else
      POCMsb = pCtx->iPrevPocMSB;
  }

  if(!(pSlice->nuh_temporal_id_plus1 - 1) && !AL_HEVC_IsRASL_RADL_SLNR(pSlice->nal_unit_type))
  {
    pCtx->uPrevPocLSB = pSlice->slice_pic_order_cnt_lsb;
    pCtx->iPrevPocMSB = POCMsb;
  }

  return pSlice->slice_pic_order_cnt_lsb + POCMsb;
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
static AL_TStreamSettings extractStreamSettings(AL_THevcSps const* pSPS, bool bDecodeIntraOnly)
{
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

  AL_TStreamSettings tStreamSettings;
  tStreamSettings.tDim = tSPSDim;
  tStreamSettings.eChroma = (AL_EChromaMode)pSPS->chroma_format_idc;
  tStreamSettings.iBitDepth = getMaxBitDepth(pSPS);
  tStreamSettings.iLevel = pSPS->profile_and_level.general_level_idc / 3;
  tStreamSettings.eProfile = AL_PROFILE_HEVC | pSPS->profile_and_level.general_profile_idc | AL_RExt_FLAGS(uFlags);
  tStreamSettings.eSequenceMode = getSequenceMode(pSPS);
  AL_Assert(tStreamSettings.eSequenceMode != AL_SM_MAX_ENUM);
  tStreamSettings.bDecodeIntraOnly = bDecodeIntraOnly;
  return tStreamSettings;
}

/*****************************************************************************/
static bool isStillPictureProfileSPS(AL_THevcSps const* pSPS)
{
  return (pSPS->profile_and_level.general_profile_idc == HEVC_PROFILE_IDC_MAIN_STILL) ||
         (pSPS->profile_and_level.general_profile_idc == HEVC_PROFILE_IDC_MAIN10 && pSPS->profile_and_level.general_one_picture_only_constraint_flag) ||
         (pSPS->profile_and_level.general_profile_idc >= HEVC_PROFILE_IDC_RExt && pSPS->profile_and_level.general_one_picture_only_constraint_flag);
}

static bool isIntraProfileSPS(AL_THevcSps const* pSPS)
{
  return isStillPictureProfileSPS(pSPS) ||
         (pSPS->profile_and_level.general_profile_idc >= 4 && pSPS->profile_and_level.general_intra_constraint_flag);
}

/*****************************************************************************/
int AL_HEVC_GetMaxDpbBuffers(AL_TStreamSettings const* pStreamSettings)
{
  return AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), AL_IS_STILL_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly);
}

/*****************************************************************************/
static AL_ERR resolutionFound(AL_TDecCtx* pCtx, AL_TStreamSettings const* pSpsSettings, AL_TCropInfo const* pCropInfo)
{
  int iMaxBuf = AL_HEVC_GetMinOutputBuffersNeeded(pSpsSettings, pCtx->iStackSize);
  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, pSpsSettings->iBitDepth, &bEnableDisplayCompression);
  int iSizeYuv = AL_GetAllocSize_Frame(pSpsSettings->tDim, pSpsSettings->eChroma, pSpsSettings->iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);

  return pCtx->tDecCB.resolutionFoundCB.func(iMaxBuf, iSizeYuv, pSpsSettings, pCropInfo, pCtx->tDecCB.resolutionFoundCB.userParam);
}

/*****************************************************************************/
static AL_ERR isSPSCompatibleWithStreamSettings(AL_THevcSps const* pSPS, AL_TStreamSettings const* pStreamSettings)
{
  int iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSLumaBitDepth))
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  int iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSChromaBitDepth))
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  int iSPSMaxBitDepth = getMaxBitDepth(pSPS);

  if(iSPSMaxBitDepth > AL_HWConfig_Dec_GetSupportedBitDepth())
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  if(pStreamSettings->iLevel > 0)
  {
    int iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;
    int iCurDPBSize = AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, false, false, false);
    int iNewDPBSize = AL_HEVC_GetMaxDPBSize(iSPSLevel, tSPSDim.iWidth, tSPSDim.iHeight, false, false, false);

    if(iNewDPBSize > iCurDPBSize)
      return AL_WARN_SPS_LEVEL_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if(eSPSChromaMode > AL_HWConfig_Dec_GetSupportedChromaMode())
    return AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  if((pStreamSettings->eChroma != AL_CHROMA_MAX_ENUM) && (pStreamSettings->eChroma < eSPSChromaMode))
  {
    Rtos_Log(AL_LOG_ERROR, "Invalid chroma-mode, incompatible with initial setting used for allocation.\n");
    return AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if((pStreamSettings->tDim.iWidth > 0) && (pStreamSettings->tDim.iWidth < tSPSDim.iWidth))
  {
    Rtos_Log(AL_LOG_ERROR, "Invalid resolution. Width greater than initial setting in decoder.\n");
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  int32_t iMinResolution = ((AL_CORE_MIN_CU_NB - 1) << pSPS->Log2CtbSize) + (1 << pSPS->Log2MinCbSize);

  if(tSPSDim.iWidth < iMinResolution)
  {
    Rtos_Log(AL_LOG_ERROR, "Invalid resolution: minimum width is 2 CTBs.\n");
    return AL_WARN_SPS_MIN_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if((pStreamSettings->tDim.iHeight > 0) && (pStreamSettings->tDim.iHeight < tSPSDim.iHeight))
  {
    Rtos_Log(AL_LOG_ERROR, "Invalid resolution. Height greater than initial setting in decoder.\n");
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(tSPSDim.iHeight < iMinResolution)
  {
    Rtos_Log(AL_LOG_ERROR, "Invalid resolution: minimum height is 2 CTBs.\n");
    return AL_WARN_SPS_MIN_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  AL_ESequenceMode sequenceMode = getSequenceMode(pSPS);

  if(sequenceMode == AL_SM_MAX_ENUM)
    return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  if(sequenceMode != AL_SM_UNKNOWN)
  {
    if(((pStreamSettings->eSequenceMode != AL_SM_MAX_ENUM) && pStreamSettings->eSequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != sequenceMode))
      return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  int iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(pCtx->tStreamSettings.iLevel);
  int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int iSizeCompData = AL_GetAllocSize_HevcCompData(pCtx->tStreamSettings.tDim, pCtx->tStreamSettings.eChroma);
  int iSizeCompMap = AL_GetAllocSize_DecCompMap(pCtx->tStreamSettings.tDim);
  AL_ERR error = AL_ERR_NO_MEMORY;

  if(!AL_Default_Decoder_AllocPool(pCtx, 0, 0, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap, 0))
    goto fail_alloc;

  int const iDpbMaxBuf = AL_HEVC_GetMaxDpbBuffers(&pCtx->tStreamSettings);
  int iMaxBuf = AL_HEVC_GetMinOutputBuffersNeeded(&pCtx->tStreamSettings, pCtx->iStackSize);
  int iSizeMV = AL_GetAllocSize_HevcMV(pCtx->tStreamSettings.tDim);
  int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  int iDpbRef = Min(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);

  if(isIntraProfileSPS(pSPS))
    iDpbRef = 1;

  AL_TPictMngrParam tPictMngrParam =
  {
    iDpbRef, pCtx->eDpbMode, pCtx->pChanParam->eFBStorageMode, pCtx->tStreamSettings.iBitDepth,
    iMaxBuf, iSizeMV,
    pCtx->pChanParam->bUseEarlyCallback,
    pCtx->tOutputPosition,
  };

  AL_PictMngr_Init(&pCtx->PictMngr, &tPictMngrParam);

  AL_TCropInfo tCropInfo = AL_HEVC_GetCropInfo(pSPS);

  error = resolutionFound(pCtx, &pCtx->tStreamSettings, &tCropInfo);

  if(AL_IS_ERROR_CODE(error))
    goto fail_alloc;

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1, true);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  AL_TDecChanParam* pChan = pCtx->pChanParam;
  pChan->iWidth = pCtx->tStreamSettings.tDim.iWidth;
  pChan->iHeight = pCtx->tStreamSettings.tDim.iHeight;
  pChan->uLog2MaxCuSize = pSPS->Log2CtbSize;
  pChan->eMaxChromaMode = pCtx->tStreamSettings.eChroma;

  const int iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(pCtx->tStreamSettings.iLevel);
  pChan->iMaxSlices = iSPSMaxSlices;
  const int iSPSMaxTileRows = AL_HEVC_GetMaxTileRows(pCtx->tStreamSettings.iLevel);
  const int iSPSMaxTileCols = AL_HEVC_GetMaxTileColumns(pCtx->tStreamSettings.iLevel);
  const int iPicMaxTileRows = RoundUp(pChan->iHeight, 64) / 64;
  const int iPicMaxTileCols = RoundUp(pChan->iWidth, 256) / 256;
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

  AL_TDecScheduler_CB_EndParsing endParsingCallback = { AL_Default_Decoder_EndParsing, pCtx };
  AL_TDecScheduler_CB_EndDecoding endDecodingCallback = { AL_Default_Decoder_EndDecoding, pCtx };
  AL_ERR eError = AL_IDecScheduler_CreateChannel(&pCtx->hChannel, pCtx->pScheduler, &pCtx->tMDChanParam, endParsingCallback, endDecodingCallback);

  if(AL_IS_ERROR_CODE(eError))
  {
    AL_Default_Decoder_SetError(pCtx, eError, -1, true);
    pCtx->eChanState = CHAN_INVALID;
    return false;
  }

  pCtx->eChanState = CHAN_CONFIGURED;

  return true;
}

/******************************************************************************/
static int slicePpsId(AL_THevcSliceHdr const* pSlice)
{
  return pSlice->slice_pic_parameter_set_id;
}

/******************************************************************************/
static int sliceSpsId(AL_THevcPps const* pPps, AL_THevcSliceHdr const* pSlice)
{
  int const ppsid = slicePpsId(pSlice);
  return pPps[ppsid].pps_seq_parameter_set_id;
}

/*****************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice)
{
  AL_TAup* pIAup = &pCtx->aup;
  AL_THevcAup* pAup = &pIAup->hevcAup;

  if(!pCtx->bIsFirstSPSChecked)
  {
    AL_ERR const ret = isSPSCompatibleWithStreamSettings(pSlice->pSPS, &pCtx->tStreamSettings);

    if(ret != AL_SUCCESS)
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      pSlice->pPPS = &pAup->pPPS[pCtx->tConceal.iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
      AL_Default_Decoder_SetError(pCtx, ret, -1, true);
      return false;
    }

    if(!pCtx->bIsBuffersAllocated)
      pCtx->tStreamSettings = extractStreamSettings(pSlice->pSPS, pCtx->tStreamSettings.bDecodeIntraOnly);

    pCtx->bIsFirstSPSChecked = true;
    pCtx->bIntraOnlyProfile = isIntraProfileSPS(pSlice->pSPS);
    pCtx->bStillPictureProfile = isStillPictureProfileSPS(pSlice->pSPS);

    if(!initChannel(pCtx, pSlice->pSPS))
      return false;

    if(!pCtx->bIsBuffersAllocated)
    {
      if(!allocateBuffers(pCtx, pSlice->pSPS))
        return false;
    }

    pCtx->bIsBuffersAllocated = true;
  }

  int const spsid = sliceSpsId(pAup->pPPS, pSlice);

  pAup->pActiveSPS = &pAup->pSPS[spsid];

  const AL_TDimension tDim = { pSlice->pSPS->pic_width_in_luma_samples, pSlice->pSPS->pic_height_in_luma_samples };
  const int iLevel = pSlice->pSPS->profile_and_level.general_level_idc / 3;
  const int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(iLevel, tDim.iWidth, tDim.iHeight, false, false, false);
  const int iDpbRef = Min(pSlice->pSPS->sps_max_dec_pic_buffering_minus1[pSlice->pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  AL_PictMngr_UpdateDPBInfo(&pCtx->PictMngr, iDpbRef);

  if(!pSlice->dependent_slice_segment_flag)
  {
    if(pSlice->slice_type != AL_SLICE_I && !pIAup->iRecoveryCnt && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
      return false;

    if(pSlice->IdrPicFlag)
    {
      pCtx->PictMngr.iCurFramePOC = 0;
      pCtx->PictMngr.uPrevPocLSB = 0;
      pCtx->PictMngr.iPrevPocMSB = 0;
    }
    else if(!pCtx->tConceal.bValidFrame)
      pCtx->PictMngr.iCurFramePOC = calculatePOC(&pCtx->PictMngr, pSlice, pCtx->uNoRaslOutputFlag);

    if(!pCtx->tConceal.bValidFrame)
    {
      AL_HEVC_PictMngr_InitRefPictSet(&pCtx->PictMngr, pSlice);

      /* at least one active reference on inter slice */
      if(pSlice->slice_type != AL_SLICE_I && !pSlice->NumPocTotalCurr && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
        return false;
    }
  }

  return true;
}

/*****************************************************************************/
static void copyScalingList(AL_THevcPps* pPPS, AL_TScl* pSCL)
{
  Rtos_Memcpy((*pSCL)[0].t4x4Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[0] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][0], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[1] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][1], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[2] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][2], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[3] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][3], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[4] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][4], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[5] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][5], 16);

  Rtos_Memcpy((*pSCL)[0].t8x8Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[6] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][0], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[7] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][1], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[8] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][2], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[9] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][3], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[10] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][4], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[11] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][5], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[12] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][0], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[13] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][1], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[14] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][2], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[15] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][3], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[16] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][4], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[17] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][5], 64);

  Rtos_Memcpy((*pSCL)[0].t32x32, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[18] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[3][0], 64);

  Rtos_Memcpy((*pSCL)[1].t32x32, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[19] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[3][3], 64);

  (*pSCL)[0].tDC[0] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[12] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][0];
  (*pSCL)[0].tDC[1] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[13] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][1];
  (*pSCL)[0].tDC[2] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[14] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][2];
  (*pSCL)[0].tDC[3] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[18] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[1][0];
  (*pSCL)[1].tDC[0] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[15] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][3];
  (*pSCL)[1].tDC[1] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[16] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][4];
  (*pSCL)[1].tDC[2] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[17] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][5];
  (*pSCL)[1].tDC[3] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[19] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[1][3];
}

/*****************************************************************************/
static bool isFirstSliceSegmentInPicture(AL_THevcSliceHdr* pSlice)
{
  return pSlice->first_slice_segment_in_pic_flag == 1;
}

/*****************************************************************************/
static void processScalingList(AL_THevcAup* pAUP, AL_THevcSliceHdr* pSlice, AL_TScl* pScl)
{
  int const ppsid = slicePpsId(pSlice);

  AL_CleanupMemory(pScl, sizeof(*pScl));

  // Save ScalingList
  if(pAUP->pPPS[ppsid].pSPS->scaling_list_enabled_flag && isFirstSliceSegmentInPicture(pSlice))
    copyScalingList(&pAUP->pPPS[ppsid], pScl);
}

/*****************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bSliceHdrValid)
{
  pSlice->slice_type = AL_SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT, pPP->tBufIDs.FrmID, true);

  AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP);

  AL_SetConcealParameters(pCtx, pSP);

  if(bSliceHdrValid)
    pSP->FirstLcuSliceSegment = pSP->FirstLcuSlice = pSP->SliceFirstLCU = pSlice->slice_segment_address;

  AL_SET_DEC_OPT(pPP, IntraOnly, 0);
}

/*****************************************************************************/
static void createConcealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  concealSlice(pCtx, pPP, pSP, pSlice, false);
  pSP->FirstLcuSliceSegment = 0;
  pSP->FirstLcuSlice = 0;
  pSP->SliceFirstLCU = 0;
  pSP->NextSliceSegment = pSlice->slice_segment_address;
  pSP->SliceNumLCU = pSlice->slice_segment_address;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, uint8_t pic_output_flag, bool bHasPreviousSlice)
{
  AL_HEVC_PictMngr_EndFrame(&pCtx->PictMngr, pSlice->slice_pic_order_cnt_lsb, eNUT, pSlice, pic_output_flag);

  if(pCtx->pChanParam->eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);

  if(pCtx->pChanParam->eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, true, bHasPreviousSlice);
  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[pCtx->uCurID];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice - 1]);

  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = pCtx->iStreamOffset[(pCtx->iNumFrmBlk1 + pCtx->iStackSize - 1) % pCtx->iStackSize];

  endFrame(pCtx, pSlice->nal_unit_type, pSlice, pSlice->pic_output_flag, false);

  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
}

/*****************************************************************************/
static bool isRandomAccessPoint(AL_ENut eNUT)
{
  return AL_HEVC_IsCRA(eNUT) || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL22) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL23);
}

/*****************************************************************************/
static bool isValidSyncPoint(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_ESliceType ePicType, int iRecoveryCnt)
{
  if(isRandomAccessPoint(eNUT))
    return true;

  if(iRecoveryCnt)
    return true;

  if(pCtx->bUseIFramesAsSyncPoint && eNUT == AL_HEVC_NUT_TRAIL_R && ePicType == AL_SLICE_I)
    return true;

  return false;
}

/*****************************************************************************/
static bool hevcInitFrameBuffers(AL_TDecCtx* pCtx, bool bStartsNewCVS, const AL_THevcSps* pSPS, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs)
{
  (void)bStartsNewCVS;
  AL_TDimension const tDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  if(!AL_InitFrameBuffers(pCtx, pBufs, bStartsNewCVS, tDim, (AL_EChromaMode)pSPS->chroma_format_idc, pPP))
    return false;

  AL_TBuffer* pDispBuf = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, pPP->tBufIDs.FrmID);
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
static bool decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  // ignore RASL picture associated with an IRAP picture that has NoRaslOutputFlag = 1
  if(AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag)
    return false;

  bool const bIsRAP = isRandomAccessPoint(eNUT);

  if(bIsRAP)
    pCtx->uNoRaslOutputFlag = (pCtx->bIsFirstPicture || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT)) ? 1 : 0;

  AL_THevcAup* pAUP = &pIAUP->hevcAup;
  bool bCheckDynResChange = pCtx->bIsBuffersAllocated;
  AL_TDimension tLastDim = pCtx->tStreamSettings.tDim;

  if(pCtx->bIsFirstSPSChecked)
  {
    tLastDim.iWidth = pAUP->pActiveSPS->pic_width_in_luma_samples;
    tLastDim.iHeight = pAUP->pActiveSPS->pic_height_in_luma_samples;
  }

  TCircBuffer* pBufStream = &pCtx->Stream;
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pCtx->BufNoAE.tMD.pVirtualAddr, true, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->uCurID) & 0x01;
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, offsetof(AL_THevcSliceHdr, entry_point_offset_minus1));
  AL_TConceal* pConceal = &pCtx->tConceal;
  bool isValid = AL_HEVC_ParseSliceHeader(pSlice, &pCtx->HevcSliceHdr[pCtx->uCurID], &rp, pConceal, pAUP->pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(isValid)
  {
    if(pSlice->slice_pic_order_cnt_lsb != pCtx->uCurPocLsb && !isFirstSliceSegmentInPicture(pSlice))
      bSliceBelongsToSameFrame = false;
    else if(pSlice->slice_segment_address <= pConceal->iFirstLCU && !pSlice->pPPS->tiles_enabled_flag && !pSlice->pPPS->entropy_coding_sync_enabled_flag)
      isValid = false;
  }

  if(isValid)
    pConceal->iFirstLCU = pSlice->slice_segment_address;

  bool* bFirstSliceInFrameIsValid = &pCtx->bFirstSliceInFrameIsValid;
  bool* bFirstIsValid = &pCtx->bFirstIsValid;
  bool* bBeginFrameIsValid = &pCtx->bBeginFrameIsValid;

  if(!bSliceBelongsToSameFrame && AL_Default_Decoder_HasOngoingFrame(pCtx))
  {
    finishPreviousFrame(pCtx);
  }

  pCtx->bIsIFrame &= pSlice->slice_type == AL_SLICE_I;

  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];

  if(isValid)
  {
    pCtx->uCurPocLsb = pSlice->slice_pic_order_cnt_lsb;
    isValid = initSlice(pCtx, pSlice);
  }

  if(!isValid)
  {
    if(!*bFirstIsValid)
    {
      if(bIsLastAUNal && *bBeginFrameIsValid)
        AL_CancelFrameBuffers(pCtx);

      UpdateContextAtEndOfFrame(pCtx);
      return false;
    }
    AL_HEVC_PictMngr_RemoveHeadFrame(&pCtx->PictMngr);
  }

  if(isValid)
  {
    int const spsid = sliceSpsId(pAUP->pPPS, pSlice);
    AL_THevcSps* pSPS = &pAUP->pSPS[spsid];
    AL_ERR const ret = isSPSCompatibleWithStreamSettings(pSPS, &pCtx->tStreamSettings);
    isValid = ret == AL_SUCCESS;
    AL_TStreamSettings spsSettings = extractStreamSettings(pSPS, pCtx->tStreamSettings.bDecodeIntraOnly);

    if(!isValid)
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      AL_Default_Decoder_SetError(pCtx, ret, pPP->tBufIDs.FrmID, true);
      pSPS->bConceal = true;
    }
    else if(bCheckDynResChange && (spsSettings.tDim.iWidth != tLastDim.iWidth || spsSettings.tDim.iHeight != tLastDim.iHeight))
    {
      AL_TCropInfo tCropInfo = AL_HEVC_GetCropInfo(pSPS);
      AL_ERR error = resolutionFound(pCtx, &spsSettings, &tCropInfo);

      if(error != AL_SUCCESS)
      {
        Rtos_Log(AL_LOG_ERROR, "ResolutionFound callback returns with error\n");
        AL_Default_Decoder_SetError(pCtx, AL_WARN_RES_FOUND_CB, pPP->tBufIDs.FrmID, false);
        pSPS->bConceal = true;
        isValid = false;
      }
    }
  }

  if(isFirstSliceSegmentInPicture(pSlice) && *bFirstSliceInFrameIsValid)
    isValid = false;

  if(isValid && isFirstSliceSegmentInPicture(pSlice))
    *bFirstSliceInFrameIsValid = true;

  if(isValid && pSlice->slice_type != AL_SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  pCtx->uCurID = (pCtx->uCurID + 1) & 1;

  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice]);

  pBufs->tStream.tMD = pCtx->Stream.tMD;

  if(!pSlice->pPPS)
    pSlice->pPPS = &pAUP->pPPS[pConceal->iLastPPSId];

  if(!pSlice->pSPS)
    pSlice->pSPS = pAUP->pActiveSPS;

  if(*bFirstSliceInFrameIsValid)
  {
    if(isFirstSliceSegmentInPicture(pSlice) && !(*bBeginFrameIsValid))
    {
      bool bClearRef = (bIsRAP && pCtx->uNoRaslOutputFlag); // IRAP picture with NoRaslOutputFlag = 1
      bool bNoOutputPrior = (AL_HEVC_IsCRA(eNUT) || ((AL_HEVC_IsIDR(eNUT) || AL_HEVC_IsBLA(eNUT)) && pSlice->no_output_of_prior_pics_flag));

      AL_HEVC_PictMngr_ClearDPB(&pCtx->PictMngr, pSlice->pSPS, bClearRef, bNoOutputPrior);
    }
  }

  if(pSlice->slice_type != AL_SLICE_I && !pIAUP->iRecoveryCnt && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
    isValid = false;
  else if(!(*bFirstSliceInFrameIsValid) && pSlice->slice_segment_address)
  {
    if(pSlice->slice_segment_address <= (int)pSP->NextSliceSegment)
    {
      createConcealSlice(pCtx, pPP, pSP, pSlice);

      pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[++pCtx->PictMngr.uNumSlice]);
      *bFirstSliceInFrameIsValid = true;
    }
    else
      isValid = false;
  }

  if(pCtx->bIsBuffersAllocated && !(*bBeginFrameIsValid) && pSlice->pSPS)
  {
    if(!hevcInitFrameBuffers(pCtx, bIsRAP, pSlice->pSPS, pPP, pBufs))
      return false;
    *bBeginFrameIsValid = true;
    AL_HEVC_PictMngr_UpdateRecInfo(&pCtx->PictMngr, pSlice->pSPS, pAUP->ePicStruct);
  }

  bool bLastSlice = *iNumSlice >= pCtx->pChanParam->iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    isValid = false;

  AL_TScl ScalingList = { 0 };

  if(pCtx->tStreamSettings.bDecodeIntraOnly && !pCtx->bIsIFrame && bIsLastAUNal)
    isValid = false;

  if(isValid)
  {
    if(!(*bFirstIsValid))
    {
      if(!isValidSyncPoint(pCtx, eNUT, pSlice->slice_type, pIAUP->iRecoveryCnt))
      {
        *bBeginFrameIsValid = false;
        AL_CancelFrameBuffers(pCtx);
        return false;
      }
      *bFirstIsValid = true;
    }

    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(pCtx->PictMngr.uNumSlice == 0)
      AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
    AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP);

    if(!AL_HEVC_PictMngr_BuildPictureList(&pCtx->PictMngr, pSlice, &pCtx->ListRef) && !pIAUP->iRecoveryCnt)
    {
      concealSlice(pCtx, pPP, pSP, pSlice, true);
    }
    else
    {
      AL_HEVC_FillSlicePicIdRegister(pSlice, pCtx, pPP, pSP);
      pConceal->bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSP);
    }
  }
  else if((bIsLastAUNal || isFirstSliceSegmentInPicture(pSlice) || bLastSlice) && (*bFirstIsValid) && (*bFirstSliceInFrameIsValid) && !(pCtx->tStreamSettings.bDecodeIntraOnly && !pCtx->bIsIFrame)) /* conceal the current slice data */
  {
    concealSlice(pCtx, pPP, pSP, pSlice, true);

    if(bLastSlice)
      pSP->NextSliceSegment = pPP->LcuPicWidth * pPP->LcuPicHeight;
  }
  else // skip slice
  {
    if(bIsLastAUNal)
    {
      if(*bBeginFrameIsValid)
        AL_CancelFrameBuffers(pCtx);

      UpdateContextAtEndOfFrame(pCtx);
      pCtx->bIsIFrame = true;
    }

    return false;
  }

  // Launch slice decoding
  AL_HEVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pCtx->PictMngr.uNumSlice;
  ++(*iNumSlice);

  if(bIsLastAUNal || bLastSlice)
  {
    uint8_t pic_output_flag = (AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag) ? 0 : pSlice->pic_output_flag;
    endFrame(pCtx, eNUT, pSlice, pic_output_flag, true);
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
static AL_PARSE_RESULT parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  uint16_t PpsId;
  AL_HEVC_ParsePPS(IAup, rp, &PpsId);

  if(PpsId >= AL_HEVC_MAX_PPS)
    return AL_UNSUPPORTED;

  AL_THevcAup* aup = &IAup->hevcAup;

  if(!aup->pPPS[PpsId].bConceal)
  {
    pCtx->tConceal.bHasPPS = true;

    if(pCtx->tConceal.iLastPPSId <= PpsId)
      pCtx->tConceal.iLastPPSId = PpsId;
  }

  return AL_OK;
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
static AL_PARSE_RESULT parseAndApplySPS(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_TDecCtx* pCtx)
{
  AL_THevcSps tNewSPS;
  AL_PARSE_RESULT eParseResult = AL_HEVC_ParseSPS(pRP, &tNewSPS);

  if(tNewSPS.sps_seq_parameter_set_id >= AL_HEVC_MAX_SPS || tNewSPS.sps_seq_parameter_set_id == AL_SPS_UNKNOWN_ID)
    return AL_UNSUPPORTED;

  if(eParseResult != AL_OK)
  {
    pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id].bConceal = true;
    return eParseResult;
  }

  if(AL_Default_Decoder_HasOngoingFrame(pCtx) && isActiveSPSChanging(&tNewSPS, pIAup->hevcAup.pActiveSPS))
  {
    // An active SPS should not be modified unless it is the end of the CVS (spec 7.4.2.4).
    // So we consider we received the full frame.
    finishPreviousFrame(pCtx);
  }

  pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id] = tNewSPS;

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
  for(int i = 0; i < AL_HEVC_MAX_PPS; ++i)
    pAUP->pPPS[i].bConceal = true;

  for(int i = 0; i < AL_HEVC_MAX_SPS; ++i)
    pAUP->pSPS[i].bConceal = true;

  pAUP->pActiveSPS = NULL;
}

/*****************************************************************************/
AL_ERR CreateHevcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  AL_ERR errorCode = AL_CreateDefaultDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  if(!AL_IS_ERROR_CODE(errorCode))
  {
    AL_TDecoder* pDec = *hDec;
    AL_TDecCtx* pCtx = &pDec->ctx;

    AL_HEVC_InitParser(&pCtx->parser);
  }

  return errorCode;
}

