/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "EncUtils.h"
#include "lib_common_enc/EncHwScalingList.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "IP_EncoderCtx.h"
#include "lib_common/SyntaxConversion.h"
#include "lib_common/Utils.h"

/****************************************************************************/
static void AL_sUpdateProfileTierLevel(AL_THevcProfilevel* pPTL, AL_TEncChanParam const* pChParam, bool profilePresentFlag, int iLayerId)
{
  (void)iLayerId;
  Rtos_Memset(pPTL, 0, sizeof(AL_THevcProfilevel));

  if(profilePresentFlag)
  {
    pPTL->general_profile_idc = AL_GET_PROFILE_IDC(pChParam->eProfile);
    pPTL->general_tier_flag = pChParam->uTier;

    if(pChParam->eVideoMode == AL_VM_PROGRESSIVE)
    {
      pPTL->general_progressive_source_flag = 1;
      pPTL->general_frame_only_constraint_flag = 1;
    }
    else
    {
      pPTL->general_progressive_source_flag = 0;
      pPTL->general_interlaced_source_flag = 1;
      pPTL->general_frame_only_constraint_flag = 0;
    }

    if(pPTL->general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt))
    {
      int16_t iExtFlags = (int16_t)AL_GET_RExt_FLAGS(pChParam->eProfile);
      pPTL->general_max_12bit_constraint_flag = (iExtFlags & 0x8000) ? 1 : 0;
      pPTL->general_max_10bit_constraint_flag = (iExtFlags & 0x4000) ? 1 : 0;
      pPTL->general_max_8bit_constraint_flag = (iExtFlags & 0x2000) ? 1 : 0;
      pPTL->general_max_422chroma_constraint_flag = (iExtFlags & 0x1000) ? 1 : 0;
      pPTL->general_max_420chroma_constraint_flag = (iExtFlags & 0x0800) ? 1 : 0;
      pPTL->general_max_monochrome_constraint_flag = (iExtFlags & 0x0400) ? 1 : 0;
      pPTL->general_intra_constraint_flag = AL_IS_INTRA_PROFILE(pChParam->eProfile);
      pPTL->general_one_picture_only_constraint_flag = AL_IS_STILL_PROFILE(pChParam->eProfile);
      pPTL->general_lower_bit_rate_constraint_flag = AL_IS_LOW_BITRATE_PROFILE(pChParam->eProfile);
    }
  }

  if(pChParam->uLevel == (uint8_t)-1)
    pPTL->general_level_idc = 0;
  else
    pPTL->general_level_idc = pChParam->uLevel * 3;
  pPTL->general_profile_compatibility_flag[pPTL->general_profile_idc] = 1;

  pPTL->general_rext_profile_flags = AL_GET_RExt_FLAGS(pChParam->eProfile);
}

/****************************************************************************/
static void AL_HEVC_SelectScalingList(AL_TSps* pISPS, AL_TEncSettings const* pSettings, int MultiLayerExtSpsFlag)
{
  AL_THevcSps* pSPS = (AL_THevcSps*)pISPS;

  AL_EScalingList eScalingList = pSettings->eScalingList;
  pSPS->scaling_list_enabled_flag = eScalingList == AL_SCL_FLAT ? 0 : 1;

  if(MultiLayerExtSpsFlag)
    pSPS->sps_infer_scaling_list_flag = eScalingList == AL_SCL_CUSTOM ? 1 : 0;

  if(pSPS->sps_infer_scaling_list_flag)
  {
    pSPS->sps_scaling_list_ref_layer_id = 0;
    return;
  }

  pSPS->sps_scaling_list_data_present_flag = eScalingList == AL_SCL_CUSTOM ? 1 : 0;

  if(eScalingList == AL_SCL_CUSTOM)
  {
    // update scaling list with settings
    for(int iSizeId = 0; iSizeId < 4; ++iSizeId)
    {
      for(int iMatrixId = 0; iMatrixId < 6; iMatrixId += (iSizeId == 3) ? 3 : 1)
      {
        // by default use default scaling list
        pSPS->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId] = 0;
        pSPS->scaling_list_param.scaling_list_pred_matrix_id_delta[iSizeId][iMatrixId] = 0;

        // parse DC coef
        if(iSizeId > 1)
        {
          int iSizeMatrixID = (iSizeId == 3 && iMatrixId == 3) ? 7 : (iSizeId - 2) * 6 + iMatrixId;

          if(pSettings->DcCoeffFlag) // if not in config file, keep default values
            pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = pSettings->DcCoeff[iSizeMatrixID];
          else
            pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = 16;
        }

        // parse AC coef
        if(pSettings->SclFlag[iSizeId][iMatrixId]) // if not in config file, keep default values
        {
          pSPS->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId] = 1; // scaling list present in file
          Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], pSettings->ScalingList[iSizeId][iMatrixId], iSizeId == 0 ? 16 : 64);
        }
        else
        {
          if(iSizeId == 0)
            Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], AL_HEVC_DefaultScalingLists4x4[iMatrixId / 3], 16);
          else
          {
            if(iSizeId > 1)
              pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = 16;
            Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], AL_HEVC_DefaultScalingLists8x8[iMatrixId / 3], 64);
          }
        }
      }
    }
  }
  else if(eScalingList == AL_SCL_DEFAULT)
  {
    for(int iDir = 0; iDir < 2; ++iDir)
    {
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[3][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir)], AL_HEVC_DefaultScalingLists4x4[iDir], 16);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists4x4[iDir], 16);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists4x4[iDir], 16);

      pSPS->scaling_list_param.scaling_list_dc_coeff[1][(3 * iDir)] = 16; // luma 32x32
      pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir)] = 16; // luma 16x16
      pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir) + 1] = 16; // Cb 16x16
      pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir) + 2] = 16; // Cr 16x16
    }
  }
}

/****************************************************************************/
void AL_HEVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP)
{
  AL_THwScalingList HwSclLst;

  AL_HEVC_GenerateHwScalingList(pSclLst, &HwSclLst);
  AL_HEVC_WriteEncHwScalingList(pSclLst, &HwSclLst, pBufEP->tMD.pVirtualAddr + EP1_BUF_SCL_LST.Offset);

  pBufEP->uFlags |= EP1_BUF_SCL_LST.Flag;
}

/****************************************************************************/
void AL_HEVC_GenerateVPS(AL_TVps* pIVPS, AL_TEncSettings const* pSettings, int iMaxRef)
{
  AL_THevcVps* pVPS = (AL_THevcVps*)pIVPS;
  pVPS->vps_video_parameter_set_id = 0;
  pVPS->vps_base_layer_internal_flag = 1;
  pVPS->vps_base_layer_available_flag = 1;
  int vps_max_layers_minus1 = 0;
  pVPS->vps_max_layers_minus1 = vps_max_layers_minus1;
  AL_TGopParam const* const pGopParam = &pSettings->tChParam[0].tGopParam;
  int const iNumTemporalLayer = DeduceNumTemporalLayer(pGopParam);
  pVPS->vps_max_sub_layers_minus1 = iNumTemporalLayer - 1;
  pVPS->vps_temporal_id_nesting_flag = 1;

  AL_sUpdateProfileTierLevel(&pVPS->profile_and_level[0], &pSettings->tChParam[0], true, 0);

  pVPS->vps_sub_layer_ordering_info_present_flag = iNumTemporalLayer > 1;

  for(int i = 0; i < iNumTemporalLayer; ++i)
  {
    int const iNumRef = iMaxRef - (iNumTemporalLayer - 1 - i);
    pVPS->vps_max_dec_pic_buffering_minus1[i] = iNumRef;
    bool const bIsLowDelayP = pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_P;
    pVPS->vps_max_num_reorder_pics[i] = bIsLowDelayP ? 0 : iNumRef;
    pVPS->vps_max_latency_increase_plus1[i] = 0;
  }

  pVPS->vps_max_layer_id = 0;
  pVPS->vps_num_layer_sets_minus1 = 0;

  pVPS->vps_timing_info_present_flag = 0;
  pVPS->vps_extension_flag = 0;
}

/****************************************************************************/
static void AL_HEVC_UpdateHrdParameters(AL_THevcSps* pSPS, AL_TSubHrdParam* pSubHrdParam, int const iCpbSize, AL_TEncSettings const* pSettings)
{
  pSubHrdParam->bit_rate_du_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  AL_Decomposition(&(pSubHrdParam->bit_rate_du_value_minus1[0]), &pSPS->vui_param.hrd_param.bit_rate_scale);

  assert(pSubHrdParam->bit_rate_du_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->bit_rate_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  AL_Decomposition(&(pSubHrdParam->bit_rate_value_minus1[0]), &pSPS->vui_param.hrd_param.bit_rate_scale);
  assert(pSubHrdParam->bit_rate_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->cpb_size_du_value_minus1[0] = iCpbSize >> 4;
  AL_Decomposition(&(pSubHrdParam->cpb_size_du_value_minus1[0]), &pSPS->vui_param.hrd_param.cpb_size_scale);
  assert(pSubHrdParam->cpb_size_du_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->cpb_size_value_minus1[0] = iCpbSize >> 4;
  AL_Decomposition(&(pSubHrdParam->cpb_size_value_minus1[0]), &pSPS->vui_param.hrd_param.cpb_size_scale);
  assert(pSubHrdParam->cpb_size_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->cbr_flag[0] = (pSettings->tChParam[0].tRCParam.eRCMode == AL_RC_CBR) ? 1 : 0;

  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 31;
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 30;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 30;

  AL_TGopParam const* const pGopParam = &pSettings->tChParam[0].tGopParam;

  for(int i = 0; i < DeduceNumTemporalLayer(pGopParam); ++i)
  {
    pSPS->vui_param.hrd_param.fixed_pic_rate_general_flag[i] = 0;
    pSPS->vui_param.hrd_param.fixed_pic_rate_within_cvs_flag[i] = 0;
    pSPS->vui_param.hrd_param.elemental_duration_in_tc_minus1[i] = 0;
    // low Delay
    pSPS->vui_param.hrd_param.low_delay_hrd_flag[i] = 0;
    pSPS->vui_param.hrd_param.cpb_cnt_minus1[i] = 0;
  }
}

/****************************************************************************/
static void InitHEVC_Sps(AL_THevcSps* pSPS)
{
  pSPS->sample_adaptive_offset_enabled_flag = 0;
  pSPS->pcm_enabled_flag = 0;
  pSPS->vui_parameters_present_flag = 1;
}

/****************************************************************************/
void AL_HEVC_GenerateSPS_Format(AL_THevcSps* pSPS, AL_EChromaMode eChromaMode, uint8_t uBdLuma, uint8_t uBdChroma, uint16_t uWidth, uint16_t uHeight, bool bMultiLayerExtSpsFlag, AL_TEncChanParam const* pChParam)
{
  (void)pChParam;

  if(bMultiLayerExtSpsFlag)
  {
    pSPS->update_rep_format_flag = 0;

    if(pSPS->update_rep_format_flag)
      pSPS->sps_rep_format_idx = 0;
    return;
  }
  pSPS->chroma_format_idc = eChromaMode;
  pSPS->separate_colour_plane_flag = 0;
  pSPS->pic_width_in_luma_samples = RoundUp(uWidth, 8);
  pSPS->pic_height_in_luma_samples = RoundUp(uHeight, 8);

  int iCropLeft = 0;
  int iCropTop = 0;
  iCropLeft = pChParam->uOutputCropPosX;
  iCropTop = pChParam->uOutputCropPosY;

  if(pChParam->uOutputCropWidth)
    uWidth = pChParam->uOutputCropWidth;

  if(pChParam->uOutputCropHeight)
    uHeight = pChParam->uOutputCropHeight;

  pSPS->conformance_window_flag = (iCropLeft || iCropTop || (pSPS->pic_width_in_luma_samples != uWidth) || (pSPS->pic_height_in_luma_samples != uHeight)) ? 1 : 0;

  if(pSPS->conformance_window_flag)
  {
    int iCropRight = pSPS->pic_width_in_luma_samples - (iCropLeft + uWidth);
    int iCropBottom = pSPS->pic_height_in_luma_samples - (iCropTop + uHeight);

    int iCropUnitX = eChromaMode == AL_CHROMA_4_2_0 || eChromaMode == AL_CHROMA_4_2_2 ? 2 : 1;
    int iCropUnitY = eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;

    pSPS->conf_win_left_offset = iCropLeft / iCropUnitX;
    pSPS->conf_win_right_offset = iCropRight / iCropUnitX;
    pSPS->conf_win_top_offset = iCropTop / iCropUnitY;
    pSPS->conf_win_bottom_offset = iCropBottom / iCropUnitY;
  }

  pSPS->bit_depth_luma_minus8 = uBdLuma - 8;
  pSPS->bit_depth_chroma_minus8 = uBdChroma - 8;
}

/****************************************************************************/
bool AL_HEVC_MultiLayerExtSpsFlag(AL_THevcSps* pSPS, int iLayerId)
{
  return (iLayerId != 0) && (pSPS->sps_ext_or_max_sub_layers_minus1 == 7);
}

/****************************************************************************/
void AL_HEVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam, int iMaxRef, int iCpbSize, int iLayerId)
{
  AL_THevcSps* pSPS = (AL_THevcSps*)pISPS;
  InitHEVC_Sps(pSPS);

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
  pSPS->sps_video_parameter_set_id = 0;
  AL_TGopParam const* const pGopParam = &pSettings->tChParam[0].tGopParam;
  int const iNumTemporalLayer = DeduceNumTemporalLayer(pGopParam);

  if(iLayerId == 0)
    pSPS->sps_max_sub_layers_minus1 = iNumTemporalLayer - 1;
  else
    pSPS->sps_ext_or_max_sub_layers_minus1 = 7;

  int MultiLayerExtSpsFlag = AL_HEVC_MultiLayerExtSpsFlag(pSPS, iLayerId);

  if(!MultiLayerExtSpsFlag)
  {
    pSPS->sps_temporal_id_nesting_flag = 1;
    AL_sUpdateProfileTierLevel(&pSPS->profile_and_level, pChParam, true, iLayerId);
  }
  pSPS->sps_seq_parameter_set_id = iLayerId;

  AL_HEVC_GenerateSPS_Format(pSPS, AL_GET_CHROMA_MODE(pChParam->ePicFormat), AL_GET_BITDEPTH_LUMA(pChParam->ePicFormat),
                             AL_GET_BITDEPTH_CHROMA(pChParam->ePicFormat), pChParam->uEncWidth, pChParam->uEncHeight, MultiLayerExtSpsFlag, pChParam);

  pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 = AL_GET_SPS_LOG2_MAX_POC(pChParam->uSpsParam) - 4;

  if(!MultiLayerExtSpsFlag)
  {
    pSPS->sps_sub_layer_ordering_info_present_flag = 1;

    for(int i = 0; i < iNumTemporalLayer; ++i)
    {
      int const iNumRef = iMaxRef - (iNumTemporalLayer - 1 - i);
      pSPS->sps_max_dec_pic_buffering_minus1[i] = iNumRef;
      bool const bIsLowDelayP = pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_P;
      pSPS->sps_max_num_reorder_pics[i] = bIsLowDelayP ? 0 : iNumRef;
      pSPS->sps_max_latency_increase_plus1[i] = 0;
    }
  }

  pSPS->log2_min_luma_coding_block_size_minus3 = pChParam->uLog2MinCuSize - 3;
  pSPS->log2_diff_max_min_luma_coding_block_size = pChParam->uLog2MaxCuSize - pChParam->uLog2MinCuSize;
  pSPS->log2_min_transform_block_size_minus2 = pChParam->uLog2MinTuSize - 2;
  pSPS->log2_diff_max_min_transform_block_size = pChParam->uLog2MaxTuSize - pChParam->uLog2MinTuSize;
  pSPS->max_transform_hierarchy_depth_inter = pChParam->uMaxTransfoDepthInter;
  pSPS->max_transform_hierarchy_depth_intra = pChParam->uMaxTransfoDepthIntra;

  AL_HEVC_SelectScalingList(pISPS, pSettings, MultiLayerExtSpsFlag);

  pSPS->amp_enabled_flag = 0;

  if(pSPS->pcm_enabled_flag)
  {
    pSPS->pcm_sample_bit_depth_luma_minus1 = AL_GET_BITDEPTH_LUMA(pChParam->ePicFormat) - 1;
    pSPS->pcm_sample_bit_depth_chroma_minus1 = AL_GET_BITDEPTH_CHROMA(pChParam->ePicFormat) - 1;
    pSPS->log2_min_pcm_luma_coding_block_size_minus3 = Min(pChParam->uLog2MaxCuSize - 3, 2);
    pSPS->log2_diff_max_min_pcm_luma_coding_block_size = 0;
    pSPS->pcm_loop_filter_disabled_flag = (pChParam->eEncTools & AL_OPT_LF) ? 0 : 1;
  }

  pSPS->num_short_term_ref_pic_sets = 0;

  pSPS->long_term_ref_pics_present_flag = AL_GET_SPS_LOG2_NUM_LONG_TERM_RPS(pChParam->uSpsParam) ? 1 : 0;
  pSPS->num_long_term_ref_pics_sps = 0;
  pSPS->sps_temporal_mvp_enabled_flag = 1;
  pSPS->strong_intra_smoothing_enabled_flag = (pChParam->uLog2MaxCuSize > 4) ? 1 : 0;

#if __ANDROID_API__
  pSPS->vui_parameters_present_flag = 0;
#endif

  pSPS->vui_param.chroma_loc_info_present_flag = (eChromaMode == AL_CHROMA_4_2_0) ? 1 : 0;
  pSPS->vui_param.chroma_sample_loc_type_top_field = 0;
  pSPS->vui_param.chroma_sample_loc_type_bottom_field = 0;

  pSPS->vui_param.neutral_chroma_indication_flag = 0;

  if(pChParam->eVideoMode == AL_VM_PROGRESSIVE)
  {
    pSPS->vui_param.field_seq_flag = 0;
    pSPS->vui_param.frame_field_info_present_flag = 0;
  }
  else
  {
    pSPS->vui_param.field_seq_flag = 1;
    pSPS->vui_param.frame_field_info_present_flag = 1;
  }
  pSPS->vui_param.default_display_window_flag = 0;

  AL_UpdateAspectRatio(&pSPS->vui_param, pChParam->uEncWidth, pChParam->uEncHeight, pSettings->eAspectRatio);

  pSPS->vui_param.overscan_info_present_flag = 0;

  // Video Signal type
  pSPS->vui_param.video_format = VIDEO_FORMAT_UNSPECIFIED;
  pSPS->vui_param.video_full_range_flag = pChParam->bVideoFullRange;

  // Colour description
  pSPS->vui_param.colour_primaries = AL_H273_ColourDescToColourPrimaries(pSettings->eColourDescription);
  pSPS->vui_param.transfer_characteristics = AL_TransferCharacteristicsToVUIValue(pSettings->eTransferCharacteristics);
  pSPS->vui_param.matrix_coefficients = AL_ColourMatrixCoefficientsToVUIValue(pSettings->eColourMatrixCoeffs);

  pSPS->vui_param.colour_description_present_flag =
    (pSPS->vui_param.colour_primaries != AL_H273_ColourDescToColourPrimaries(AL_COLOUR_DESC_UNSPECIFIED)) ||
    (pSPS->vui_param.transfer_characteristics != AL_TransferCharacteristicsToVUIValue(AL_TRANSFER_CHARAC_UNSPECIFIED)) ||
    (pSPS->vui_param.matrix_coefficients != AL_ColourMatrixCoefficientsToVUIValue(AL_COLOUR_MAT_COEFF_UNSPECIFIED));

  pSPS->vui_param.video_signal_type_present_flag =
    (pSPS->vui_param.video_format != VIDEO_FORMAT_UNSPECIFIED) ||
    (pSPS->vui_param.video_full_range_flag != 0) ||
    pSPS->vui_param.colour_description_present_flag;

  if(pSPS->vui_param.matrix_coefficients == 0)
  {
    bool bEnsureSpecification = ((pSPS->bit_depth_chroma_minus8 == pSPS->bit_depth_luma_minus8) || (pSPS->chroma_format_idc == 3));
    AL_Assert(bEnsureSpecification);
  }

  if(pSPS->vui_param.matrix_coefficients == 8)
  {
    bool bEnsureSpecification = (pSPS->bit_depth_chroma_minus8 == pSPS->bit_depth_luma_minus8);
    bEnsureSpecification |= ((pSPS->bit_depth_chroma_minus8 == (pSPS->bit_depth_luma_minus8 + 1)) && (pSPS->chroma_format_idc == 3));
    AL_Assert(bEnsureSpecification);
  }

  // Timing information
  AL_UpdateVuiTimingInfo(&pSPS->vui_param, iLayerId, &pChParam->tRCParam, 1);

  pSPS->vui_param.vui_poc_proportional_to_timing_flag = 0;

  // HRD
  pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag = 0; // TODO check if 0 is a correct value

  // NAL
  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;

  // VCL
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 1;
  AL_HEVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.vcl_sub_hrd_param), iCpbSize, pSettings);

  pSPS->vui_param.vui_hrd_parameters_present_flag = pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag + pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag;

  pSPS->vui_param.bitstream_restriction_flag = 1;

  pSPS->vui_param.tiles_fixed_structure_flag = 0;
  pSPS->vui_param.motion_vectors_over_pic_boundaries_flag = 1;
  pSPS->vui_param.restricted_ref_pic_lists_flag = 1;
  pSPS->vui_param.min_spatial_segmentation_idc = 0;
  pSPS->vui_param.max_bytes_per_pic_denom = 0;
  pSPS->vui_param.max_bits_per_min_cu_denom = 0;
  pSPS->vui_param.log2_max_mv_length_horizontal = 15;
  pSPS->vui_param.log2_max_mv_length_vertical = 15;

  pSPS->sps_extension_present_flag = iLayerId ? 1 : 0;

  if(pSPS->sps_extension_present_flag)
  {
    pSPS->sps_range_extension_flag = 0;
    pSPS->sps_multilayer_extension_flag = 1;
    pSPS->sps_3d_extension_flag = 0;
    pSPS->sps_scc_extension_flag = 0;
    pSPS->sps_extension_4bits = 0;
  }

  if(pSPS->sps_multilayer_extension_flag)
    pSPS->inter_view_mv_vert_constraint_flag = 0;
}

/****************************************************************************/
static void AL_HEVC_GenerateFilterParam(AL_THevcPps* pPPS, bool bIsGDR, AL_EChEncTool eEncTools, uint32_t uPPSParam, int8_t iBetaOffset, int8_t iTcOffset)
{
  (void)bIsGDR;
  pPPS->deblocking_filter_control_present_flag = (!(eEncTools & AL_OPT_LF) || AL_GET_PPS_OVERRIDE_LF(uPPSParam) || (iBetaOffset || iTcOffset)) ? 1 : 0;

  if(bIsGDR)
    pPPS->deblocking_filter_control_present_flag = 1;

  if(pPPS->deblocking_filter_control_present_flag)
  {
    pPPS->deblocking_filter_override_enabled_flag = AL_GET_PPS_OVERRIDE_LF(uPPSParam);
    pPPS->pps_deblocking_filter_disabled_flag = AL_GET_PPS_DISABLE_LF(uPPSParam);

    if(!pPPS->pps_deblocking_filter_disabled_flag)
    {
      pPPS->pps_beta_offset_div2 = iBetaOffset;
      pPPS->pps_tc_offset_div2 = iTcOffset;
    }
  }
}

/****************************************************************************/
void AL_HEVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam, int iMaxRef, int iLayerId)
{
  AL_THevcPps* pPPS = (AL_THevcPps*)pIPPS;
  pPPS->pps_pic_parameter_set_id = iLayerId;
  pPPS->pps_seq_parameter_set_id = iLayerId;

  pPPS->dependent_slice_segments_enabled_flag = pSettings->bDependentSlice ? 1 : 0;
  pPPS->output_flag_present_flag = 0;
  pPPS->num_extra_slice_header_bits = 0;
  pPPS->sign_data_hiding_flag = 0; // not supported yet
  pPPS->cabac_init_present_flag = AL_GET_PPS_CABAC_INIT_PRES_FLAG(pChParam->uPpsParam);
  pPPS->num_ref_idx_l0_default_active_minus1 = iMaxRef - 1;
  pPPS->num_ref_idx_l1_default_active_minus1 = iMaxRef - 1;
  pPPS->init_qp_minus26 = 0;
  pPPS->constrained_intra_pred_flag = (pChParam->eEncTools & AL_OPT_CONST_INTRA_PRED) ? 1 : 0;
  pPPS->transform_skip_enabled_flag = (pChParam->eEncTools & AL_OPT_TRANSFO_SKIP) ? 1 : 0;
  pPPS->cu_qp_delta_enabled_flag = HasCuQpDeltaDepthEnabled(pSettings, pChParam);
  pPPS->diff_cu_qp_delta_depth = pChParam->uCuQPDeltaDepth;

  pPPS->pps_cb_qp_offset = pChParam->iCbPicQpOffset;
  pPPS->pps_cr_qp_offset = pChParam->iCrPicQpOffset;
  pPPS->pps_slice_chroma_qp_offsets_present_flag = AL_GET_PPS_SLICE_CHROMA_QP_OFFSET_PRES_FLAG(pChParam->uPpsParam);

  pPPS->weighted_pred_flag = 0; // not supported yet
  pPPS->weighted_bipred_flag = 0; // not supported yet
  pPPS->transquant_bypass_enabled_flag = 0; // not supported yet
  pPPS->tiles_enabled_flag = (pChParam->eEncTools & AL_OPT_TILE) ? 1 : 0;
  pPPS->entropy_coding_sync_enabled_flag = (pChParam->eEncTools & AL_OPT_WPP) ? 1 : 0;

  pPPS->uniform_spacing_flag = 1;
  pPPS->loop_filter_across_tiles_enabled_flag = (pChParam->eEncTools & AL_OPT_LF_X_TILE) ? 1 : 0;

  pPPS->loop_filter_across_slices_enabled_flag = (pChParam->eEncTools & AL_OPT_LF_X_SLICE) ? 1 : 0;

  bool bIsGdr = false;
  bIsGdr |= AL_IsGdrEnabled(pChParam);

  AL_HEVC_GenerateFilterParam(pPPS, bIsGdr, pChParam->eEncTools, pChParam->uPpsParam, pChParam->iBetaOffset, pChParam->iTcOffset);

  pPPS->pps_scaling_list_data_present_flag = 0;
  pPPS->lists_modification_present_flag = AL_GET_PPS_ENABLE_REORDERING(pChParam->uPpsParam);
  pPPS->log2_parallel_merge_level_minus2 = 0; // parallel merge at 16x16 granularity
  pPPS->slice_segment_header_extension_present_flag = 0;

  bool bSaoScaleNeeded = false;

  pPPS->pps_extension_present_flag = iLayerId || bSaoScaleNeeded ? 1 : 0;

  if(pPPS->pps_extension_present_flag)
  {
    pPPS->pps_range_extension_flag = bSaoScaleNeeded;
    pPPS->pps_multilayer_extension_flag = iLayerId;
    pPPS->pps_3d_extension_flag = 0;
    pPPS->pps_scc_extension_flag = 0;
    pPPS->pps_extension_4bits = 0;
  }

  if(pPPS->pps_range_extension_flag)
  {
    pPPS->log2_sao_offset_scale_luma = Max(0, AL_GET_BITDEPTH(pChParam->ePicFormat) - 10);
    pPPS->log2_sao_offset_scale_chroma = pPPS->log2_sao_offset_scale_luma;
  }

  if(pPPS->pps_multilayer_extension_flag)
  {
    pPPS->poc_reset_info_present_flag = 1;
    pPPS->pps_infer_scaling_list_flag = 0;
    pPPS->num_ref_loc_offsets = 0;
    pPPS->colour_mapping_enabled_flag = 0;
  }
}

/***************************************************************************/
void AL_HEVC_UpdateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs, int iLayerId)
{
  if(!pPicStatus->bIsFirstSlice)
    return;

  if(AL_IsWriteSps(pHdrs, pHLSInfo->uSpsId) || pHLSInfo->uFrameRate) // In case frame rate update, regenerate the SPS and it will be inserted at next IDR
  {
    AL_THevcSps* pSPS = (AL_THevcSps*)pISPS;

    AL_EPicFormat ePicFormat = pSettings->tChParam[iLayerId].ePicFormat;
    AL_TDimension tDim = AL_GetPpsDim(pHdrs, pHLSInfo->uSpsId);

    int MultiLayerExtSpsFlag = AL_HEVC_MultiLayerExtSpsFlag(pSPS, iLayerId);
    AL_HEVC_GenerateSPS_Format(pSPS, AL_GET_CHROMA_MODE(ePicFormat), AL_GET_BITDEPTH_LUMA(ePicFormat), AL_GET_BITDEPTH_CHROMA(ePicFormat),
                               tDim.iWidth, tDim.iHeight, MultiLayerExtSpsFlag, &pSettings->tChParam[iLayerId]);

    if(pHLSInfo->uFrameRate)
    {
      AL_TRCParam rcParam;
      rcParam.uFrameRate = pHLSInfo->uFrameRate;
      rcParam.uClkRatio = pHLSInfo->uClkRatio;
      AL_UpdateVuiTimingInfo(&pSPS->vui_param, 0, &rcParam, 1);
    }

    pSPS->sps_seq_parameter_set_id = pHLSInfo->uSpsId;
  }

  AL_ReleaseSps(pHdrs, pHLSInfo->uSpsId);
}

/***************************************************************************/
bool AL_HEVC_UpdateAUD(AL_TAud* pAud, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, int iLayerID)
{
  pAud->eType = pPicStatus->eType;
  return pSettings->bEnableAUD && isBaseLayer(iLayerID);
}

/***************************************************************************/
bool AL_HEVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs, int iLayerId)
{
  (void)pHLSInfo;
  (void)pSettings;
  (void)iLayerId;
  AL_THevcPps* pPPS = (AL_THevcPps*)pIPPS;

  bool bMustWritePPS = false;

  pPPS->init_qp_minus26 = pPicStatus->iPpsQP - 26;
  int32_t const iNumClmn = pPicStatus->uNumClmn;
  int32_t const iNumRow = pPicStatus->uNumRow;
  int32_t const* pTileWidth = pPicStatus->iTileWidth;
  int32_t const* pTileHeight = pPicStatus->iTileHeight;

  pPPS->num_tile_columns_minus1 = iNumClmn - 1;
  pPPS->num_tile_rows_minus1 = iNumRow - 1;

  if(pPPS->num_tile_columns_minus1 == 0)
    pPPS->tiles_enabled_flag = 0;
  else
  {
    pPPS->tiles_enabled_flag = 1;

    for(int iClmn = 0; iClmn < iNumClmn - 1; ++iClmn)
      pPPS->tile_column_width[iClmn] = pTileWidth[iClmn];

    for(int iRow = 0; iRow < iNumRow - 1; ++iRow)
      pPPS->tile_row_height[iRow] = pTileHeight[iRow];
  }
  pPPS->diff_cu_qp_delta_depth = pPicStatus->uCuQpDeltaDepth;
  pPPS->pps_seq_parameter_set_id = pHLSInfo->uSpsId;
  pPPS->pps_pic_parameter_set_id = pHLSInfo->uPpsId;

  if(pPicStatus->bIsFirstSlice)
  {
    bMustWritePPS = AL_IsWritePps(pHdrs, pHLSInfo->uPpsId);
    AL_ReleasePps(pHdrs, pHLSInfo->uPpsId);
  }

  pPPS->pps_cb_qp_offset = pHLSInfo->iCbPicQpOffset;
  pPPS->pps_cr_qp_offset = pHLSInfo->iCrPicQpOffset;

  // For LF offsets, the PPS is updated directly, but we don't force PPS update now (offsets overwriten in slice headers).
  // Will be updated with the next IDR. Works under the assumption that we only insert PPS with IDRs, so that we match the
  // behaviour of the firmware.
  if(pHLSInfo->bLFOffsetChanged)
  {
    AL_TEncChanParam const* pChParam = &pSettings->tChParam[iLayerId];
    bool bIsGdr = false;
    bIsGdr |= AL_IsGdrEnabled(pChParam);

    AL_HEVC_GenerateFilterParam(pPPS, bIsGdr, pChParam->eEncTools, pChParam->uPpsParam, pHLSInfo->iLFBetaOffset, pHLSInfo->iLFTcOffset);
  }

  return bMustWritePPS;
}
