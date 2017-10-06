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
****************************************************************************/

#include <assert.h>
#include "HEVC_RbspEncod.h"
#include "RbspEncod.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/SliceHeader.h"
#include "lib_common/Utils.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/ScalingList.h"

/******************************************************************************/
static void writeSublayer(AL_TBitStreamLite* pBS, AL_THrdParam const* pHrd, AL_TSubHrdParam const* pSubHrd, uint8_t uLayer)
{
  for(int iIdx = 0; iIdx <= uLayer; ++iIdx)
  {
    AL_BitStreamLite_PutUE(pBS, pSubHrd->bit_rate_value_minus1[iIdx]);
    AL_BitStreamLite_PutUE(pBS, pSubHrd->cpb_size_value_minus1[iIdx]);

    if(pHrd->sub_pic_hrd_params_present_flag)
    {
      AL_BitStreamLite_PutUE(pBS, pSubHrd->cpb_size_du_value_minus1[iIdx]);
      AL_BitStreamLite_PutUE(pBS, pSubHrd->bit_rate_du_value_minus1[iIdx]);
    }
    AL_BitStreamLite_PutBit(pBS, pSubHrd->cbr_flag[iIdx]);
  }
}

/******************************************************************************/
static void writeHrdParam(AL_TBitStreamLite* pBS, AL_THrdParam const* pHrd, uint8_t uCommonInfoFlag, uint8_t uMaxLayersMinus1)
{
  if(uCommonInfoFlag)
  {
    AL_BitStreamLite_PutBit(pBS, pHrd->nal_hrd_parameters_present_flag);
    AL_BitStreamLite_PutBit(pBS, pHrd->vcl_hrd_parameters_present_flag);

    if(pHrd->nal_hrd_parameters_present_flag || pHrd->vcl_hrd_parameters_present_flag)
    {
      AL_BitStreamLite_PutBit(pBS, pHrd->sub_pic_hrd_params_present_flag);

      if(pHrd->sub_pic_hrd_params_present_flag)
      {
        AL_BitStreamLite_PutU(pBS, 8, pHrd->tick_divisor_minus2);
        AL_BitStreamLite_PutU(pBS, 5, pHrd->du_cpb_removal_delay_increment_length_minus1);
        AL_BitStreamLite_PutBit(pBS, pHrd->sub_pic_cpb_params_in_pic_timing_sei_flag);
        AL_BitStreamLite_PutU(pBS, 5, pHrd->dpb_output_delay_length_minus1);
      }
      AL_BitStreamLite_PutU(pBS, 4, pHrd->bit_rate_scale);
      AL_BitStreamLite_PutU(pBS, 4, pHrd->cpb_size_scale);

      if(pHrd->sub_pic_hrd_params_present_flag)
        AL_BitStreamLite_PutU(pBS, 4, pHrd->cpb_size_du_scale);
      AL_BitStreamLite_PutU(pBS, 5, pHrd->initial_cpb_removal_delay_length_minus1);
      AL_BitStreamLite_PutU(pBS, 5, pHrd->au_cpb_removal_delay_length_minus1);
      AL_BitStreamLite_PutU(pBS, 5, pHrd->dpb_output_delay_length_minus1);
    }
  }

  for(int iLayer = 0; iLayer <= uMaxLayersMinus1; ++iLayer)
  {
    AL_BitStreamLite_PutBit(pBS, pHrd->fixed_pic_rate_general_flag[iLayer]);

    if(!pHrd->fixed_pic_rate_general_flag[iLayer])
      AL_BitStreamLite_PutBit(pBS, pHrd->fixed_pic_rate_within_cvs_flag[iLayer]);

    if(pHrd->fixed_pic_rate_within_cvs_flag[iLayer])
      AL_BitStreamLite_PutUE(pBS, pHrd->elemental_duration_in_tc_minus1[iLayer]);
    else
      AL_BitStreamLite_PutBit(pBS, pHrd->low_delay_hrd_flag[iLayer]);

    if(!pHrd->low_delay_hrd_flag[iLayer])
      AL_BitStreamLite_PutUE(pBS, pHrd->cpb_cnt_minus1[iLayer]);

    if(pHrd->nal_hrd_parameters_present_flag)
      writeSublayer(pBS, pHrd, &pHrd->nal_sub_hrd_param, iLayer);

    if(pHrd->vcl_hrd_parameters_present_flag)
      writeSublayer(pBS, pHrd, &pHrd->vcl_sub_hrd_param, iLayer);
  }
}

/******************************************************************************/
static void writeProfileTierLevel(AL_TBitStreamLite* pBS, AL_TProfilevel const* pPTL, uint8_t MaxLayerMinus1)
{
  AL_BitStreamLite_PutU(pBS, 2, pPTL->general_profile_space);
  AL_BitStreamLite_PutBit(pBS, pPTL->general_tier_flag);
  AL_BitStreamLite_PutU(pBS, 5, pPTL->general_profile_idc);

  for(int j = 0; j < 32; ++j)
    AL_BitStreamLite_PutBit(pBS, pPTL->general_profile_compatibility_flag[j]);

  AL_BitStreamLite_PutBit(pBS, pPTL->general_progressive_source_flag);
  AL_BitStreamLite_PutBit(pBS, pPTL->general_interlaced_source_flag);
  AL_BitStreamLite_PutBit(pBS, pPTL->general_non_packed_constraint_flag);
  AL_BitStreamLite_PutBit(pBS, pPTL->general_frame_only_constraint_flag);

  if(pPTL->general_profile_idc == 4 || pPTL->general_profile_compatibility_flag[4] ||
     pPTL->general_profile_idc == 5 || pPTL->general_profile_compatibility_flag[5] ||
     pPTL->general_profile_idc == 6 || pPTL->general_profile_compatibility_flag[6] ||
     pPTL->general_profile_idc == 7 || pPTL->general_profile_compatibility_flag[7])
    AL_BitStreamLite_PutU(pBS, 16, pPTL->general_rext_profile_flags);
  else
    AL_BitStreamLite_PutU(pBS, 16, 0);
  AL_BitStreamLite_PutU(pBS, 28, 0);

  AL_BitStreamLite_PutU(pBS, 8, pPTL->general_level_idc);

  for(int j = 0; j < MaxLayerMinus1; ++j)
  {
    AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_profile_present_flag[j]);
    AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_level_present_flag[j]);
  }

  if(MaxLayerMinus1 > 0)
  {
    for(int j = MaxLayerMinus1; j < 8; ++j)
      AL_BitStreamLite_PutU(pBS, 2, 0);
  }

  for(int iLayerID = 0; iLayerID < MaxLayerMinus1; ++iLayerID)
  {
    if(pPTL->sub_layer_profile_present_flag[iLayerID])
    {
      AL_BitStreamLite_PutU(pBS, 2, pPTL->sub_layer_profile_space[iLayerID]);
      AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_tier_flag[iLayerID]);
      AL_BitStreamLite_PutU(pBS, 5, pPTL->sub_layer_profile_idc[iLayerID]);

      for(int j = 0; j < 32; ++j)
        AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_profile_compatibility_flag[iLayerID][j]);

      AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_progressive_source_flag[iLayerID]);
      AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_interlaced_source_flag[iLayerID]);
      AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_non_packed_constraint_flag[iLayerID]);
      AL_BitStreamLite_PutBit(pBS, pPTL->sub_layer_frame_only_constraint_flag[iLayerID]);
      AL_BitStreamLite_PutU(pBS, 32, 0);
      AL_BitStreamLite_PutU(pBS, 12, 0);
    }

    if(pPTL->sub_layer_level_present_flag[iLayerID])
      AL_BitStreamLite_PutU(pBS, 8, pPTL->sub_layer_level_idc[iLayerID]);
  }
}

/******************************************************************************/
static void writeVpsData(AL_TBitStreamLite* pBS, AL_THevcVps const* pVps)
{
  AL_BitStreamLite_PutU(pBS, 4, pVps->vps_video_parameter_set_id);
  AL_BitStreamLite_PutU(pBS, 1, pVps->vps_base_layer_internal_flag);
  AL_BitStreamLite_PutU(pBS, 1, pVps->vps_base_layer_available_flag);
  AL_BitStreamLite_PutU(pBS, 6, pVps->vps_max_layers_minus1);
  AL_BitStreamLite_PutU(pBS, 3, pVps->vps_max_sub_layers_minus1);
  AL_BitStreamLite_PutU(pBS, 1, pVps->vps_temporal_id_nesting_flag);
  AL_BitStreamLite_PutU(pBS, 16, 0xFFFF);

  writeProfileTierLevel(pBS, &pVps->profile_and_level, pVps->vps_max_sub_layers_minus1);

  AL_BitStreamLite_PutU(pBS, 1, pVps->vps_sub_layer_ordering_info_present_flag);

  int iIdx = pVps->vps_sub_layer_ordering_info_present_flag ? 0 : pVps->vps_max_sub_layers_minus1;

  for(; iIdx <= pVps->vps_max_sub_layers_minus1; ++iIdx)
  {
    AL_BitStreamLite_PutUE(pBS, pVps->vps_max_dec_pic_buffering_minus1[iIdx]);
    AL_BitStreamLite_PutUE(pBS, pVps->vps_max_num_reorder_pics[iIdx]);
    AL_BitStreamLite_PutUE(pBS, pVps->vps_max_latency_increase_plus1[iIdx]);
  }

  AL_BitStreamLite_PutU(pBS, 6, pVps->vps_max_layer_id);
  AL_BitStreamLite_PutUE(pBS, pVps->vps_num_layer_sets_minus1);

  for(iIdx = 1; iIdx <= pVps->vps_num_layer_sets_minus1; ++iIdx)
  {
    for(int iLayerID = 0; iLayerID <= pVps->vps_max_layer_id; ++iLayerID)
      AL_BitStreamLite_PutBit(pBS, pVps->layer_id_included_flag[iIdx][iLayerID]);
  }

  AL_BitStreamLite_PutBit(pBS, pVps->vps_timing_info_present_flag);

  if(pVps->vps_timing_info_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 32, pVps->vps_num_units_in_tick);
    AL_BitStreamLite_PutU(pBS, 32, pVps->vps_time_scale);
    AL_BitStreamLite_PutBit(pBS, pVps->vps_poc_proportional_to_timing_flag);

    if(pVps->vps_poc_proportional_to_timing_flag)
      AL_BitStreamLite_PutUE(pBS, pVps->vps_num_ticks_poc_diff_one_minus1);

    AL_BitStreamLite_PutUE(pBS, pVps->vps_num_hrd_parameters);

    for(iIdx = 0; iIdx < pVps->vps_num_hrd_parameters; ++iIdx)
    {
      AL_BitStreamLite_PutUE(pBS, pVps->hrd_layer_set_idx[iIdx]);

      if(iIdx > 0)
        AL_BitStreamLite_PutBit(pBS, pVps->cprms_present_flag[iIdx]);
      writeHrdParam(pBS, &pVps->hrd_parameter[iIdx], pVps->cprms_present_flag[iIdx], pVps->vps_max_sub_layers_minus1);
    }
  }
  AL_BitStreamLite_PutBit(pBS, 0); // vps_extension_flag
}

/******************************************************************************/
static void writeStRefPicSet(AL_TBitStreamLite* pBS, AL_TRefPicSet const* pRefPicSet, int iSetIdx)
{
  if(iSetIdx)
    AL_BitStreamLite_PutBit(pBS, pRefPicSet->inter_ref_pic_set_prediction_flag);

  assert(pRefPicSet->inter_ref_pic_set_prediction_flag == 0);

  AL_BitStreamLite_PutUE(pBS, pRefPicSet->num_negative_pics);
  AL_BitStreamLite_PutUE(pBS, pRefPicSet->num_positive_pics);

  for(int i = 0; i < pRefPicSet->num_negative_pics; ++i)
  {
    AL_BitStreamLite_PutUE(pBS, pRefPicSet->delta_poc_s0_minus1[i]);
    AL_BitStreamLite_PutBit(pBS, pRefPicSet->used_by_curr_pic_s0_flag[i]);
  }

  for(int i = 0; i < pRefPicSet->num_positive_pics; ++i)
  {
    AL_BitStreamLite_PutUE(pBS, pRefPicSet->delta_poc_s1_minus1[i]);
    AL_BitStreamLite_PutBit(pBS, pRefPicSet->used_by_curr_pic_s1_flag[i]);
  }
}

/******************************************************************************/
static void writeVui(AL_TBitStreamLite* pBS, AL_THevcSps const* pSps, AL_TVuiParam const* pVui)
{
  // 2 - Write VUI following spec E.1.1.

  AL_BitStreamLite_PutU(pBS, 1, pVui->aspect_ratio_info_present_flag);

  if(pVui->aspect_ratio_info_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 8, pVui->aspect_ratio_idc);

    if(pVui->aspect_ratio_idc == 255) // Extended_SAR
    {
      AL_BitStreamLite_PutU(pBS, 16, pVui->sar_width);
      AL_BitStreamLite_PutU(pBS, 16, pVui->sar_height);
    }
  }

  AL_BitStreamLite_PutU(pBS, 1, pVui->overscan_info_present_flag);
  assert(pVui->overscan_info_present_flag == 0);

  AL_BitStreamLite_PutU(pBS, 1, pVui->video_signal_type_present_flag);

  if(pVui->video_signal_type_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 3, pVui->video_format);
    AL_BitStreamLite_PutU(pBS, 1, pVui->video_full_range_flag);
    AL_BitStreamLite_PutU(pBS, 1, pVui->colour_description_present_flag);

    if(pVui->colour_description_present_flag)
    {
      AL_BitStreamLite_PutU(pBS, 8, pVui->colour_primaries);
      AL_BitStreamLite_PutU(pBS, 8, pVui->transfer_characteristics);
      AL_BitStreamLite_PutU(pBS, 8, pVui->matrix_coefficients);
    }
  }

  AL_BitStreamLite_PutU(pBS, 1, pVui->chroma_loc_info_present_flag);

  if(pVui->chroma_loc_info_present_flag)
  {
    AL_BitStreamLite_PutUE(pBS, pVui->chroma_sample_loc_type_top_field);
    AL_BitStreamLite_PutUE(pBS, pVui->chroma_sample_loc_type_bottom_field);
  }

  AL_BitStreamLite_PutBit(pBS, pVui->neutral_chroma_indication_flag);
  AL_BitStreamLite_PutBit(pBS, pVui->field_seq_flag);
  AL_BitStreamLite_PutBit(pBS, pVui->frame_field_info_present_flag);

  AL_BitStreamLite_PutBit(pBS, pVui->default_display_window_flag);

  if(pVui->default_display_window_flag)
  {
    AL_BitStreamLite_PutUE(pBS, pVui->def_disp_win_left_offset);
    AL_BitStreamLite_PutUE(pBS, pVui->def_disp_win_right_offset);
    AL_BitStreamLite_PutUE(pBS, pVui->def_disp_win_top_offset);
    AL_BitStreamLite_PutUE(pBS, pVui->def_disp_win_bottom_offset);
  }
  AL_BitStreamLite_PutBit(pBS, pVui->vui_timing_info_present_flag);

  if(pVui->vui_timing_info_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 32, pVui->vui_num_units_in_tick);
    AL_BitStreamLite_PutU(pBS, 32, pVui->vui_time_scale);
    AL_BitStreamLite_PutU(pBS, 1, pVui->vui_poc_proportional_to_timing_flag);

    if(pVui->vui_poc_proportional_to_timing_flag)
      AL_BitStreamLite_PutUE(pBS, pVui->vui_num_ticks_poc_diff_one_minus1);
    AL_BitStreamLite_PutBit(pBS, pVui->vui_hrd_parameters_present_flag);

    if(pVui->vui_hrd_parameters_present_flag)
      writeHrdParam(pBS, &pVui->hrd_param, 1, pSps->sps_max_sub_layers_minus1);
  }

  AL_BitStreamLite_PutBit(pBS, pVui->bitstream_restriction_flag);

  if(pVui->bitstream_restriction_flag)
  {
    AL_BitStreamLite_PutBit(pBS, pVui->tiles_fixed_structure_flag);
    AL_BitStreamLite_PutBit(pBS, pVui->motion_vectors_over_pic_boundaries_flag);
    AL_BitStreamLite_PutBit(pBS, pVui->restricted_ref_pic_lists_flag);
    AL_BitStreamLite_PutUE(pBS, pVui->min_spatial_segmentation_idc);
    AL_BitStreamLite_PutUE(pBS, pVui->max_bytes_per_pic_denom);
    AL_BitStreamLite_PutUE(pBS, pVui->max_bits_per_min_cu_denom);
    AL_BitStreamLite_PutUE(pBS, pVui->log2_max_mv_length_horizontal);
    AL_BitStreamLite_PutUE(pBS, pVui->log2_max_mv_length_vertical);
  }
}

/******************************************************************************/
static void writeScalingListData(AL_TBitStreamLite* pBS, AL_THevcSps const* pSps)
{
  for(int iSizeId = 0; iSizeId < 4; ++iSizeId)
  {
    for(int iMatrixId = 0; iMatrixId < 6; iMatrixId += (iSizeId == 3) ? 3 : 1)
    {
      AL_BitStreamLite_PutBit(pBS, pSps->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId]);

      if(!pSps->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId])
      {
        AL_BitStreamLite_PutUE(pBS, pSps->scaling_list_param.scaling_list_pred_matrix_id_delta[iSizeId][iMatrixId]);
      }
      else
      {
        uint8_t uCoeff;
        uint8_t uPrevCoef = 8;
        uint16_t uCoeffNum = Min(64, (1 << (4 + (iSizeId << 1))));
        uint8_t const* pScanOrder = (uCoeffNum == 64) ? AL_HEVC_ScanOrder8x8 : AL_HEVC_ScanOrder4x4;

        if(iSizeId > 1)
        {
          AL_BitStreamLite_PutSE(pBS, pSps->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] - 8);
          uPrevCoef = pSps->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId];
        }

        for(uCoeff = 0; uCoeff < uCoeffNum; ++uCoeff)
        {
          AL_BitStreamLite_PutSE(pBS, pSps->scaling_list_param.ScalingList[iSizeId][iMatrixId][pScanOrder[uCoeff]] - uPrevCoef);
          uPrevCoef = pSps->scaling_list_param.ScalingList[iSizeId][iMatrixId][pScanOrder[uCoeff]];
        }
      }
    }
  }
}

/******************************************************************************/
static void writeSpsData(AL_TBitStreamLite* pBS, AL_THevcSps const* pSps)
{
  // 1 - Write SPS following spec 7.3.2.2

  AL_BitStreamLite_PutU(pBS, 4, pSps->sps_video_parameter_set_id);
  AL_BitStreamLite_PutU(pBS, 3, pSps->sps_max_sub_layers_minus1);
  AL_BitStreamLite_PutBit(pBS, pSps->sps_temporal_id_nesting_flag);

  writeProfileTierLevel(pBS, &pSps->profile_and_level, pSps->sps_max_sub_layers_minus1);
  AL_BitStreamLite_PutUE(pBS, pSps->sps_seq_parameter_set_id);
  AL_BitStreamLite_PutUE(pBS, pSps->chroma_format_idc);
  assert(pSps->chroma_format_idc != 3);

  AL_BitStreamLite_PutUE(pBS, pSps->pic_width_in_luma_samples);
  AL_BitStreamLite_PutUE(pBS, pSps->pic_height_in_luma_samples);
  AL_BitStreamLite_PutBit(pBS, pSps->conformance_window_flag);

  if(pSps->conformance_window_flag)
  {
    AL_BitStreamLite_PutUE(pBS, pSps->conf_win_left_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->conf_win_right_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->conf_win_top_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->conf_win_bottom_offset);
  }

  AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_luma_minus8);
  AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_chroma_minus8);
  AL_BitStreamLite_PutUE(pBS, pSps->log2_max_slice_pic_order_cnt_lsb_minus4);
  AL_BitStreamLite_PutBit(pBS, pSps->sps_sub_layer_ordering_info_present_flag);

  int iLayerOffset = pSps->sps_sub_layer_ordering_info_present_flag ? 0 : pSps->sps_max_sub_layers_minus1;

  for(int i = iLayerOffset; i <= pSps->sps_max_sub_layers_minus1; ++i)
  {
    AL_BitStreamLite_PutUE(pBS, pSps->sps_max_dec_pic_buffering_minus1[i]);
    AL_BitStreamLite_PutUE(pBS, pSps->sps_num_reorder_pics[i]);
    AL_BitStreamLite_PutUE(pBS, pSps->sps_max_latency_increase_plus1[i]);
  }

  AL_BitStreamLite_PutUE(pBS, pSps->log2_min_luma_coding_block_size_minus3);
  AL_BitStreamLite_PutUE(pBS, pSps->log2_diff_max_min_luma_coding_block_size);
  AL_BitStreamLite_PutUE(pBS, pSps->log2_min_transform_block_size_minus2);
  AL_BitStreamLite_PutUE(pBS, pSps->log2_diff_max_min_transform_block_size);
  AL_BitStreamLite_PutUE(pBS, pSps->max_transform_hierarchy_depth_inter);
  AL_BitStreamLite_PutUE(pBS, pSps->max_transform_hierarchy_depth_intra);

  AL_BitStreamLite_PutBit(pBS, pSps->scaling_list_enabled_flag);

  if(pSps->scaling_list_enabled_flag)
  {
    AL_BitStreamLite_PutBit(pBS, pSps->sps_scaling_list_data_present_flag);

    if(pSps->sps_scaling_list_data_present_flag)
    {
      writeScalingListData(pBS, pSps);
    }
  }

  AL_BitStreamLite_PutBit(pBS, pSps->amp_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pSps->sample_adaptive_offset_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pSps->pcm_enabled_flag);

  if(pSps->pcm_enabled_flag)
  {
    // TODO
  }

  AL_BitStreamLite_PutUE(pBS, pSps->num_short_term_ref_pic_sets);

  for(int i = 0; i < pSps->num_short_term_ref_pic_sets; ++i)
    writeStRefPicSet(pBS, &pSps->short_term_ref_pic_set[i], i);

  AL_BitStreamLite_PutBit(pBS, pSps->long_term_ref_pics_present_flag);

  if(pSps->long_term_ref_pics_present_flag)
    AL_BitStreamLite_PutUE(pBS, pSps->num_long_term_ref_pics_sps);

  AL_BitStreamLite_PutBit(pBS, pSps->sps_temporal_mvp_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pSps->strong_intra_smoothing_enabled_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->vui_parameters_present_flag);

  if(pSps->vui_parameters_present_flag)
    writeVui(pBS, pSps, &pSps->vui_param);
  AL_BitStreamLite_PutBit(pBS, 0); // sps_extension_data_flag
}

/*****************************************************************************/
static void writePpsData(AL_TBitStreamLite* pBS, AL_THevcPps const* pPps)
{
  // 1 - Write PPS following spec 7.3.2.2.

  AL_BitStreamLite_PutUE(pBS, pPps->pps_pic_parameter_set_id);
  AL_BitStreamLite_PutUE(pBS, pPps->pps_seq_parameter_set_id);

  AL_BitStreamLite_PutBit(pBS, pPps->dependent_slice_segments_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->output_flag_present_flag);

  AL_BitStreamLite_PutU(pBS, 3, 0); // num_extra_slice_header_bits

  AL_BitStreamLite_PutBit(pBS, pPps->sign_data_hiding_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->cabac_init_present_flag);

  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l0_default_active_minus1);
  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l1_default_active_minus1);
  AL_BitStreamLite_PutSE(pBS, pPps->init_qp_minus26);

  AL_BitStreamLite_PutBit(pBS, pPps->constrained_intra_pred_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->transform_skip_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->cu_qp_delta_enabled_flag);

  if(pPps->cu_qp_delta_enabled_flag)
    AL_BitStreamLite_PutUE(pBS, pPps->diff_cu_qp_delta_depth);

  AL_BitStreamLite_PutSE(pBS, pPps->pps_cb_qp_offset);
  AL_BitStreamLite_PutSE(pBS, pPps->pps_cr_qp_offset);

  AL_BitStreamLite_PutBit(pBS, pPps->pps_slice_chroma_qp_offsets_present_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->weighted_pred_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->weighted_bipred_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->transquant_bypass_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->tiles_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->entropy_coding_sync_enabled_flag);

  if(pPps->tiles_enabled_flag)
  {
    int iClmn, iRow;
    AL_BitStreamLite_PutUE(pBS, pPps->num_tile_columns_minus1);
    AL_BitStreamLite_PutUE(pBS, pPps->num_tile_rows_minus1);
    AL_BitStreamLite_PutBit(pBS, 0);

    for(iClmn = 0; iClmn < pPps->num_tile_columns_minus1; ++iClmn)
      AL_BitStreamLite_PutUE(pBS, pPps->column_width[iClmn] - 1);

    for(iRow = 0; iRow < pPps->num_tile_rows_minus1; ++iRow)
      AL_BitStreamLite_PutUE(pBS, pPps->row_height[iRow] - 1);

    AL_BitStreamLite_PutBit(pBS, pPps->loop_filter_across_tiles_enabled_flag);
  }

  AL_BitStreamLite_PutBit(pBS, pPps->loop_filter_across_slices_enabled_flag);
  AL_BitStreamLite_PutBit(pBS, pPps->deblocking_filter_control_present_flag);

  if(pPps->deblocking_filter_control_present_flag)
  {
    AL_BitStreamLite_PutBit(pBS, pPps->deblocking_filter_override_enabled_flag);
    AL_BitStreamLite_PutBit(pBS, pPps->pps_deblocking_filter_disabled_flag);

    if(!pPps->pps_deblocking_filter_disabled_flag)
    {
      AL_BitStreamLite_PutSE(pBS, pPps->pps_beta_offset_div2);
      AL_BitStreamLite_PutSE(pBS, pPps->pps_tc_offset_div2);
    }
  }

  AL_BitStreamLite_PutBit(pBS, pPps->pps_scaling_list_data_present_flag);

  assert(pPps->pps_scaling_list_data_present_flag == 0);

  AL_BitStreamLite_PutBit(pBS, pPps->lists_modification_present_flag);
  AL_BitStreamLite_PutUE(pBS, pPps->log2_parallel_merge_level_minus2);
  AL_BitStreamLite_PutBit(pBS, 0); // slice_segment_header_extension_present_flag
  AL_BitStreamLite_PutBit(pBS, 0); // pps_extension_flag
}

/* Interface functions */

/******************************************************************************/
static void writeVps(AL_TBitStreamLite* pBS, AL_THevcVps const* pVps)
{
  writeVpsData(pBS, pVps);

  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static void writeSps(AL_TBitStreamLite* pBS, AL_TSps const* pSps)
{
  writeSpsData(pBS, &pSps->m_HevcSPS);

  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static void writePps(AL_TBitStreamLite* pBS, AL_TPps const* pPps)
{
  writePpsData(pBS, (AL_THevcPps*)pPps);

  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static void writeSeiAps(AL_TBitStreamLite* pBS, AL_THevcVps const* pVps, AL_TSps const* pISps)
{
  AL_THevcSps* pSps = (AL_THevcSps*)pISps;

  // Active Parameter Sets
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 129);

  AL_BitStreamLite_PutU(pBS, 4, pSps->sps_video_parameter_set_id);
  AL_BitStreamLite_PutU(pBS, 1, 0); // self_containd_cvs_flag
  AL_BitStreamLite_PutU(pBS, 1, 1); // no_parameter_set_update_flag
  AL_BitStreamLite_PutUE(pBS, 0); // num_sps_ids_minus1

  // for(int i=0; i <= num_sps_ids_minus1; ++i)
  AL_BitStreamLite_PutUE(pBS, pSps->sps_seq_parameter_set_id);

  for(int i = pVps->vps_base_layer_internal_flag; i <= pVps->vps_max_layers_minus1; ++i)
    AL_BitStreamLite_PutUE(pBS, 0);

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static void writeSeiBp(AL_TBitStreamLite* pBS, AL_TSps const* pISps, int iInitialCpbRemovalDelay, int iInitialCpbRemovalOffset)
{
  AL_THevcSps* pSps = (AL_THevcSps*)pISps;

  // buffering_period
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 0);

  AL_BitStreamLite_PutUE(pBS, pSps->sps_seq_parameter_set_id);

  uint8_t uIRAPCpbParamsPresentFLag = 0;

  if(!pSps->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    AL_BitStreamLite_PutU(pBS, 1, uIRAPCpbParamsPresentFLag);

  if(uIRAPCpbParamsPresentFLag)
  {
    uint32_t uCpbDelayOffset = 0;
    AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, uCpbDelayOffset);
    uint32_t uDpbDelayOffset = 0;
    AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, uDpbDelayOffset);
  }

  uint8_t uConcatenationFlag = 0;
  AL_BitStreamLite_PutU(pBS, 1, uConcatenationFlag);
  uint32_t uAuCpbRemovalDelayDelta = 1;
  AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, uAuCpbRemovalDelayDelta - 1);

  uint32_t iInitialAltCpbRemovalDelay = 0;
  uint32_t iInitialAltCpbRemovalOffset = 0;

  if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    for(int i = 0; i <= (int)pSps->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialCpbRemovalDelay);
      AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialCpbRemovalOffset);

      if(pSps->vui_param.hrd_param.sub_pic_hrd_params_present_flag || uIRAPCpbParamsPresentFLag)
      {
        AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialAltCpbRemovalDelay);
        AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialAltCpbRemovalOffset);
      }
    }
  }

  if(pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    for(int i = 0; i <= (int)pSps->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialCpbRemovalDelay);
      AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialCpbRemovalOffset);

      if(pSps->vui_param.hrd_param.sub_pic_hrd_params_present_flag || uIRAPCpbParamsPresentFLag)
      {
        AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialAltCpbRemovalDelay);
        AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iInitialAltCpbRemovalOffset);
      }
    }
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static void writeSeiRp(AL_TBitStreamLite* pBS)
{
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 6);

  AL_BitStreamLite_PutUE(pBS, 0);
  AL_BitStreamLite_PutBits(pBS, 4, 0x8);

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static void writeSeiPt(AL_TBitStreamLite* pBS, AL_TSps const* pISps, int iAuCpbRemovalDelay, int iPicDpbOutputDelay, int iPicStruct)
{
  AL_THevcSps* pSps = (AL_THevcSps*)pISps;

  int iSrcScanType = 0;
  int iDuplicateFlag = 0;

  int iPicDpbOutputDuDelay = 0;
  int iNumDecodingUnits = 1;
  int iDuCommonCpbRemovalDelayFlag = 0;
  int iDuCommonCpbRemovalDelayIncrement = 1;
  int iNumNalusInDu = 1;
  int iDuCpbRemovalDelayIncrement = 1;

  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 1);

  if(pSps->vui_param.frame_field_info_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 4, iPicStruct);
    AL_BitStreamLite_PutU(pBS, 2, iSrcScanType);
    AL_BitStreamLite_PutU(pBS, 1, iDuplicateFlag);
  }

  if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag || pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 32, iAuCpbRemovalDelay);
    AL_BitStreamLite_PutU(pBS, 32, iPicDpbOutputDelay);

    if(pSps->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    {
      AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1, iPicDpbOutputDuDelay);

      if(pSps->vui_param.hrd_param.sub_pic_cpb_params_in_pic_timing_sei_flag)
      {
        int i;
        AL_BitStreamLite_PutUE(pBS, iNumDecodingUnits - 1);
        AL_BitStreamLite_PutU(pBS, 1, iDuCommonCpbRemovalDelayFlag);

        if(iDuCommonCpbRemovalDelayFlag)
          AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1, iDuCommonCpbRemovalDelayIncrement - 1);

        for(i = 0; i < iNumDecodingUnits; ++i)
        {
          AL_BitStreamLite_PutUE(pBS, iNumNalusInDu - 1);

          if(!iDuCommonCpbRemovalDelayFlag && (i < iNumDecodingUnits - 1))
            AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1, iDuCpbRemovalDelayIncrement - 1);
        }
      }
    }
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

static IRbspWriter writer =
{
  AL_RbspEncoding_WriteAUD,
  writeVps,
  writeSps,
  writePps,
  writeSeiAps,
  writeSeiBp,
  writeSeiRp,
  writeSeiPt,
};

IRbspWriter* AL_GetHevcRbspWriter()
{
  return &writer;
}

