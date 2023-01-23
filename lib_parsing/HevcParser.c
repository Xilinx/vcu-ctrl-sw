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

#include "HevcParser.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
#include "lib_common/SeiInternal.h"
#include "lib_common_dec/RbspParser.h"

#define CONCEAL_LEVEL_IDC 60 * 3

/*****************************************************************************/
static void initPps(AL_THevcPps* pPPS)
{
  Rtos_Memset(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, sizeof(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag));
  Rtos_Memset(pPPS->cb_qp_offset_list, 0, sizeof(pPPS->cb_qp_offset_list));
  Rtos_Memset(pPPS->cr_qp_offset_list, 0, sizeof(pPPS->cr_qp_offset_list));

  pPPS->loop_filter_across_tiles_enabled_flag = 1;
  pPPS->lists_modification_present_flag = 1;
  pPPS->num_tile_columns_minus1 = 0;
  pPPS->num_tile_rows_minus1 = 0;
  pPPS->deblocking_filter_override_enabled_flag = 0;
  pPPS->pps_deblocking_filter_disabled_flag = 0;
  pPPS->pps_beta_offset_div2 = 0;
  pPPS->pps_tc_offset_div2 = 0;

  pPPS->pps_extension_7bits = 0;
  pPPS->pps_range_extension_flag = 0;
  pPPS->log2_transform_skip_block_size_minus2 = 0;
  pPPS->cross_component_prediction_enabled_flag = 0;
  pPPS->diff_cu_chroma_qp_offset_depth = 0;
  pPPS->chroma_qp_offset_list_enabled_flag = 0;
  pPPS->log2_sao_offset_scale_luma = 0;
  pPPS->log2_sao_offset_scale_chroma = 0;

  pPPS->bConceal = true;
}

/*****************************************************************************/
void AL_HEVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId)
{
  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  uint16_t pps_id = ue(pRP);

  if(pps_id >= AL_HEVC_MAX_PPS)
    return;

  AL_THevcAup* aup = &pIAup->hevcAup;
  AL_THevcPps* pPPS = &aup->pPPS[pps_id];

  if(pPpsId)
    *pPpsId = pps_id;

  // default values
  initPps(pPPS);

  pPPS->pps_pic_parameter_set_id = pps_id;
  pPPS->pps_seq_parameter_set_id = ue(pRP);

  if(pPPS->pps_seq_parameter_set_id >= AL_HEVC_MAX_SPS)
    return;

  pPPS->pSPS = &aup->pSPS[pPPS->pps_seq_parameter_set_id];

  if(pPPS->pSPS->bConceal)
    return;

  uint16_t uLCUPicWidth = pPPS->pSPS->PicWidthInCtbs;
  uint16_t uLCUPicHeight = pPPS->pSPS->PicHeightInCtbs;

  pPPS->dependent_slice_segments_enabled_flag = u(pRP, 1);
  pPPS->output_flag_present_flag = u(pRP, 1);
  pPPS->num_extra_slice_header_bits = u(pRP, 3);
  pPPS->sign_data_hiding_flag = u(pRP, 1);
  pPPS->cabac_init_present_flag = u(pRP, 1);

  pPPS->num_ref_idx_l0_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
  pPPS->num_ref_idx_l1_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

  uint16_t QpBdOffset = 6 * pPPS->pSPS->bit_depth_luma_minus8;
  pPPS->init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);

  pPPS->constrained_intra_pred_flag = u(pRP, 1);
  pPPS->transform_skip_enabled_flag = u(pRP, 1);

  pPPS->cu_qp_delta_enabled_flag = u(pRP, 1);

  if(pPPS->cu_qp_delta_enabled_flag)
    pPPS->diff_cu_qp_delta_depth = Clip3(ue(pRP), 0, pPPS->pSPS->log2_diff_max_min_luma_coding_block_size);

  pPPS->pps_cb_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_cr_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_slice_chroma_qp_offsets_present_flag = u(pRP, 1);

  pPPS->weighted_pred_flag = u(pRP, 1);
  pPPS->weighted_bipred_flag = u(pRP, 1);

  pPPS->transquant_bypass_enabled_flag = u(pRP, 1);
  pPPS->tiles_enabled_flag = u(pRP, 1);
  pPPS->entropy_coding_sync_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return;

  if(pPPS->tiles_enabled_flag)
  {
    pPPS->num_tile_columns_minus1 = ue(pRP);
    pPPS->num_tile_rows_minus1 = ue(pRP);
    pPPS->uniform_spacing_flag = u(pRP, 1);

    if(pPPS->num_tile_columns_minus1 >= uLCUPicWidth || pPPS->num_tile_rows_minus1 >= uLCUPicHeight ||
       pPPS->num_tile_columns_minus1 >= AL_MAX_COLUMNS_TILE || pPPS->num_tile_rows_minus1 >= AL_MAX_ROWS_TILE)
      return;

    if(!pPPS->uniform_spacing_flag)
    {
      uint16_t uClmnOffset = 0;
      uint16_t uLineOffset = 0;

      for(uint8_t i = 0; i < pPPS->num_tile_columns_minus1; ++i)
      {
        pPPS->tile_column_width[i] = ue(pRP) + 1;
        uClmnOffset += pPPS->tile_column_width[i];
      }

      for(uint8_t i = 0; i < pPPS->num_tile_rows_minus1; ++i)
      {
        pPPS->tile_row_height[i] = ue(pRP) + 1;
        uLineOffset += pPPS->tile_row_height[i];
      }

      if(uClmnOffset >= uLCUPicWidth || uLineOffset >= uLCUPicHeight)
        return;

      pPPS->tile_column_width[pPPS->num_tile_columns_minus1] = uLCUPicWidth - uClmnOffset;
      pPPS->tile_row_height[pPPS->num_tile_rows_minus1] = uLCUPicHeight - uLineOffset;
    }
    else /* tile of same size */
    {
      uint16_t num_clmn = pPPS->num_tile_columns_minus1 + 1;
      uint16_t num_line = pPPS->num_tile_rows_minus1 + 1;

      for(uint8_t i = 0; i <= pPPS->num_tile_columns_minus1; ++i)
        pPPS->tile_column_width[i] = (((i + 1) * uLCUPicWidth) / num_clmn) - ((i * uLCUPicWidth) / num_clmn);

      for(uint8_t i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
        pPPS->tile_row_height[i] = (((i + 1) * uLCUPicHeight) / num_line) - ((i * uLCUPicHeight) / num_line);
    }

    /* register tile topology within the frame */
    for(uint8_t i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
    {
      for(uint8_t j = 0; j <= pPPS->num_tile_columns_minus1; ++j)
      {
        uint8_t line = 0;
        uint8_t clmn = 0;
        uint8_t uClmn = 0;
        uint8_t uLine = 0;

        while(line < i)
          uLine += pPPS->tile_row_height[line++];

        while(clmn < j)
          uClmn += pPPS->tile_column_width[clmn++];

        pPPS->TileTopology[(i * (pPPS->num_tile_columns_minus1 + 1)) + j] = uLine * uLCUPicWidth + uClmn;
      }
    }

    pPPS->loop_filter_across_tiles_enabled_flag = u(pRP, 1);
  }

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return;

  pPPS->loop_filter_across_slices_enabled_flag = u(pRP, 1);

  pPPS->deblocking_filter_control_present_flag = u(pRP, 1);

  if(pPPS->deblocking_filter_control_present_flag)
  {
    pPPS->deblocking_filter_override_enabled_flag = u(pRP, 1);
    pPPS->pps_deblocking_filter_disabled_flag = u(pRP, 1);

    if(!pPPS->pps_deblocking_filter_disabled_flag)
    {
      pPPS->pps_beta_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
      pPPS->pps_tc_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
    }
  }

  pPPS->pps_scaling_list_data_present_flag = u(pRP, 1);

  if(pPPS->pps_scaling_list_data_present_flag)
    hevc_scaling_list_data(&pPPS->scaling_list_param, pRP);
  else // get scaling_list data from associated sps
    pPPS->scaling_list_param = pPPS->pSPS->scaling_list_param;

  pPPS->lists_modification_present_flag = u(pRP, 1);
  pPPS->log2_parallel_merge_level_minus2 = Clip3(ue(pRP), 0, pPPS->pSPS->Log2CtbSize - 2);

  pPPS->slice_segment_header_extension_present_flag = u(pRP, 1);
  pPPS->pps_extension_present_flag = u(pRP, 1);

  if(pPPS->pps_extension_present_flag)
  {
    pPPS->pps_range_extension_flag = u(pRP, 1);
    pPPS->pps_extension_7bits = u(pRP, 7);
  }

  if(pPPS->pps_range_extension_flag)
  {
    if(pPPS->transform_skip_enabled_flag)
      pPPS->log2_transform_skip_block_size_minus2 = ue(pRP);
    pPPS->cross_component_prediction_enabled_flag = u(pRP, 1);
    pPPS->chroma_qp_offset_list_enabled_flag = u(pRP, 1);

    if(pPPS->chroma_qp_offset_list_enabled_flag)
    {
      pPPS->diff_cu_chroma_qp_offset_depth = ue(pRP);
      pPPS->chroma_qp_offset_list_len_minus1 = ue(pRP);

      for(int i = 0; i <= pPPS->chroma_qp_offset_list_len_minus1; ++i)
      {
        pPPS->cb_qp_offset_list[i] = se(pRP);
        pPPS->cr_qp_offset_list[i] = se(pRP);
      }
    }
    pPPS->log2_sao_offset_scale_luma = ue(pRP);
    pPPS->log2_sao_offset_scale_chroma = ue(pRP);
  }

  if(pPPS->pps_extension_7bits) // pps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // pps_extension_data_flag
  }

  pPPS->bConceal = rbsp_trailing_bits(pRP) ? false : true;
}

/*****************************************************************************/
static void AL_HEVC_sComputeRefPicSetVariables(AL_THevcSps* pSPS, uint8_t RefIdx)
{
  uint8_t num_negative = 0, num_positive = 0;
  AL_TRefPicSet ref_pic_set = pSPS->short_term_ref_pic_set[RefIdx];

  uint8_t RIdx = RefIdx - (ref_pic_set.delta_idx_minus1 + 1);
  int32_t DeltaRPS = (1 - (ref_pic_set.delta_rps_sign << 1)) * (ref_pic_set.abs_delta_rps_minus1 + 1);

  if(ref_pic_set.inter_ref_pic_set_prediction_flag)
  {
    // num negative pics computation
    for(int j = pSPS->NumPositivePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        pSPS->DeltaPocS0[RefIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    if(DeltaRPS < 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      pSPS->DeltaPocS0[RefIdx][num_negative] = DeltaRPS;
      pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(int j = 0; j < pSPS->NumNegativePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[j])
      {
        pSPS->DeltaPocS0[RefIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    pSPS->NumNegativePics[RefIdx] = num_negative;

    // num positive pics computation
    for(int j = pSPS->NumNegativePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[j])
      {
        pSPS->DeltaPocS1[RefIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    if(DeltaRPS > 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      pSPS->DeltaPocS1[RefIdx][num_positive] = DeltaRPS;
      pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(int j = 0; j < pSPS->NumPositivePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        pSPS->DeltaPocS1[RefIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    pSPS->NumPositivePics[RefIdx] = num_positive;
  }
  else
  {
    pSPS->NumNegativePics[RefIdx] = ref_pic_set.num_negative_pics;
    pSPS->NumPositivePics[RefIdx] = ref_pic_set.num_positive_pics;

    pSPS->UsedByCurrPicS0[RefIdx][0] = ref_pic_set.used_by_curr_pic_s0_flag[0];
    pSPS->UsedByCurrPicS1[RefIdx][0] = ref_pic_set.used_by_curr_pic_s1_flag[0];

    pSPS->DeltaPocS0[RefIdx][0] = -(ref_pic_set.delta_poc_s0_minus1[0] + 1);
    pSPS->DeltaPocS1[RefIdx][0] = ref_pic_set.delta_poc_s1_minus1[0] + 1;

    for(int j = 1; j < ref_pic_set.num_negative_pics; ++j)
    {
      pSPS->UsedByCurrPicS0[RefIdx][j] = ref_pic_set.used_by_curr_pic_s0_flag[j];
      pSPS->DeltaPocS0[RefIdx][j] = pSPS->DeltaPocS0[RefIdx][j - 1] - (ref_pic_set.delta_poc_s0_minus1[j] + 1);
    }

    for(int j = 1; j < ref_pic_set.num_positive_pics; ++j)
    {
      pSPS->UsedByCurrPicS1[RefIdx][j] = ref_pic_set.used_by_curr_pic_s1_flag[j];
      pSPS->DeltaPocS1[RefIdx][j] = pSPS->DeltaPocS1[RefIdx][j - 1] + (ref_pic_set.delta_poc_s1_minus1[j] + 1);
    }
  }
  pSPS->NumDeltaPocs[RefIdx] = pSPS->NumNegativePics[RefIdx] + pSPS->NumPositivePics[RefIdx];
}

/*****************************************************************************/
static void initSps(AL_THevcSps* pSPS)
{
  pSPS->chroma_format_idc = 1;
  pSPS->separate_colour_plane_flag = 0;
  pSPS->bit_depth_chroma_minus8 = 0;
  pSPS->bit_depth_luma_minus8 = 0;
  pSPS->scaling_list_enabled_flag = 0;
  pSPS->conf_win_left_offset = 0;
  pSPS->conf_win_right_offset = 0;
  pSPS->conf_win_top_offset = 0;
  pSPS->conf_win_bottom_offset = 0;
  pSPS->sps_scaling_list_data_present_flag = 0;
  pSPS->pcm_loop_filter_disabled_flag = 0;

  pSPS->sps_range_extension_flag = 0;
  pSPS->sps_extension_7bits = 0;
  pSPS->transform_skip_rotation_enabled_flag = 0;
  pSPS->transform_skip_context_enabled_flag = 0;
  pSPS->implicit_rdpcm_enabled_flag = 0;
  pSPS->explicit_rdpcm_enabled_flag = 0;
  pSPS->extended_precision_processing_flag = 0;
  pSPS->intra_smoothing_disabled_flag = 0;
  pSPS->high_precision_offsets_enabled_flag = 0;
  pSPS->persistent_rice_adaptation_enabled_flag = 0;
  pSPS->cabac_bypass_alignment_enabled_flag = 0;

  pSPS->vui_param.aspect_ratio_info_present_flag = 0;
  pSPS->vui_param.aspect_ratio_idc = 0;
  pSPS->vui_param.sar_width = 0;
  pSPS->vui_param.sar_height = 0;
  pSPS->vui_param.overscan_info_present_flag = 0;
  pSPS->vui_param.video_signal_type_present_flag = 0;
  pSPS->vui_param.video_format = 5;
  pSPS->vui_param.video_full_range_flag = 0;
  pSPS->vui_param.colour_description_present_flag = 0;
  pSPS->vui_param.colour_primaries = 2;
  pSPS->vui_param.transfer_characteristics = 2;
  pSPS->vui_param.matrix_coefficients = 0;
  pSPS->vui_param.chroma_loc_info_present_flag = 0;

  pSPS->vui_param.chroma_sample_loc_type_top_field = 0;
  pSPS->vui_param.chroma_sample_loc_type_bottom_field = 0;

  pSPS->vui_param.neutral_chroma_indication_flag = 0;
  pSPS->vui_param.field_seq_flag = 0;
  pSPS->vui_param.frame_field_info_present_flag = (pSPS->profile_and_level.general_progressive_source_flag && pSPS->profile_and_level.general_interlaced_source_flag) ? 1 : 0;

  pSPS->vui_param.default_display_window_flag = 0;
  pSPS->vui_param.def_disp_win_left_offset = 0;
  pSPS->vui_param.def_disp_win_right_offset = 0;
  pSPS->vui_param.def_disp_win_top_offset = 0;
  pSPS->vui_param.def_disp_win_bottom_offset = 0;

  pSPS->vui_param.min_spatial_segmentation_idc = 0;
  pSPS->vui_param.max_bytes_per_pic_denom = 2;
  pSPS->vui_param.max_bits_per_min_cu_denom = 1;
  pSPS->vui_param.log2_max_mv_length_horizontal = 15;
  pSPS->vui_param.log2_max_mv_length_vertical = 15;

  pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 = 23;
  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 23;

  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 0;
  pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag = 0;

  Rtos_Memset(pSPS->sps_max_dec_pic_buffering_minus1, 0, sizeof(pSPS->sps_max_dec_pic_buffering_minus1));
  Rtos_Memset(pSPS->sps_max_num_reorder_pics, 0, sizeof(pSPS->sps_max_num_reorder_pics));
  Rtos_Memset(pSPS->sps_max_latency_increase_plus1, 0, sizeof(pSPS->sps_max_latency_increase_plus1));

  Rtos_Memset(pSPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, 20);
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_TRbspParser* pRP, AL_THevcSps* pSPS)
{
  pSPS->bConceal = true;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  int vps_id = u(pRP, 4);

  COMPLY(vps_id < AL_HEVC_MAX_VPS);

  int max_sub_layers = Clip3(u(pRP, 3), 0, MAX_SUB_LAYER - 1);
  int temp_id_nesting_flag = u(pRP, 1);

  hevc_profile_tier_level(&pSPS->profile_and_level, max_sub_layers, pRP);

  if(pSPS->profile_and_level.general_level_idc == 0)
    pSPS->profile_and_level.general_level_idc = CONCEAL_LEVEL_IDC;

  int sps_id = ue(pRP);

  if(sps_id >= AL_HEVC_MAX_SPS)
    pSPS->sps_seq_parameter_set_id = AL_SPS_UNKNOWN_ID;

  COMPLY(sps_id < AL_HEVC_MAX_SPS);

  pSPS->bConceal = true;
  pSPS->sps_video_parameter_set_id = vps_id;
  pSPS->sps_max_sub_layers_minus1 = max_sub_layers;
  pSPS->sps_temporal_id_nesting_flag = temp_id_nesting_flag;
  pSPS->sps_seq_parameter_set_id = sps_id;

  // default values
  initSps(pSPS);

  pSPS->chroma_format_idc = ue(pRP);

  if(pSPS->chroma_format_idc == 3)
    pSPS->separate_colour_plane_flag = u(pRP, 1);

  if(pSPS->separate_colour_plane_flag)
    pSPS->ChromaArrayType = 0;
  else
    pSPS->ChromaArrayType = pSPS->chroma_format_idc;

  pSPS->pic_width_in_luma_samples = ue(pRP);
  pSPS->pic_height_in_luma_samples = ue(pRP);

  pSPS->conformance_window_flag = u(pRP, 1);

  if(pSPS->conformance_window_flag)
  {
    pSPS->conf_win_left_offset = ue(pRP);
    pSPS->conf_win_right_offset = ue(pRP);
    pSPS->conf_win_top_offset = ue(pRP);
    pSPS->conf_win_bottom_offset = ue(pRP);

    if(pSPS->conf_win_bottom_offset + pSPS->conf_win_top_offset >= pSPS->pic_height_in_luma_samples)
    {
      pSPS->conf_win_top_offset = 0;
      pSPS->conf_win_bottom_offset = 0;
    }

    if(pSPS->conf_win_left_offset + pSPS->conf_win_right_offset >= pSPS->pic_width_in_luma_samples)
    {
      pSPS->conf_win_left_offset = 0;
      pSPS->conf_win_right_offset = 0;
    }
  }

  pSPS->bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);
  pSPS->bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);

  pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 = ue(pRP);

  COMPLY(pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB);

  pSPS->sps_sub_layer_ordering_info_present_flag = u(pRP, 1);
  int layer_offset = pSPS->sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers;

  for(int i = layer_offset; i <= max_sub_layers; ++i)
  {
    pSPS->sps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    pSPS->sps_max_num_reorder_pics[i] = ue(pRP);
    pSPS->sps_max_latency_increase_plus1[i] = ue(pRP);
  }

  pSPS->log2_min_luma_coding_block_size_minus3 = ue(pRP);
  pSPS->Log2MinCbSize = pSPS->log2_min_luma_coding_block_size_minus3 + 3;

  COMPLY(pSPS->Log2MinCbSize <= 6);

  pSPS->log2_diff_max_min_luma_coding_block_size = ue(pRP);
  pSPS->Log2CtbSize = pSPS->Log2MinCbSize + pSPS->log2_diff_max_min_luma_coding_block_size;

  COMPLY(pSPS->Log2CtbSize <= 6);
  COMPLY(pSPS->Log2CtbSize >= 4);

  pSPS->log2_min_transform_block_size_minus2 = ue(pRP);
  int Log2MinTransfoSize = pSPS->log2_min_transform_block_size_minus2 + 2;
  pSPS->log2_diff_max_min_transform_block_size = ue(pRP);

  COMPLY(pSPS->log2_min_transform_block_size_minus2 <= pSPS->log2_min_luma_coding_block_size_minus3);
  COMPLY(pSPS->log2_diff_max_min_transform_block_size <= Min(pSPS->Log2CtbSize, 5) - Log2MinTransfoSize);

  pSPS->max_transform_hierarchy_depth_inter = ue(pRP);
  pSPS->max_transform_hierarchy_depth_intra = ue(pRP);

  COMPLY(pSPS->max_transform_hierarchy_depth_inter <= (pSPS->Log2CtbSize - Log2MinTransfoSize));
  COMPLY(pSPS->max_transform_hierarchy_depth_intra <= (pSPS->Log2CtbSize - Log2MinTransfoSize));

  pSPS->scaling_list_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(pSPS->scaling_list_enabled_flag)
  {
    pSPS->sps_scaling_list_data_present_flag = u(pRP, 1);

    if(pSPS->sps_scaling_list_data_present_flag)
      hevc_scaling_list_data(&pSPS->scaling_list_param, pRP);
    else
      for(int i = 0; i < 20; ++i)
        pSPS->scaling_list_param.UseDefaultScalingMatrixFlag[i] = 1;
  }

  pSPS->amp_enabled_flag = u(pRP, 1);
  pSPS->sample_adaptive_offset_enabled_flag = u(pRP, 1);

  pSPS->pcm_enabled_flag = u(pRP, 1);

  if(pSPS->pcm_enabled_flag)
  {
    pSPS->pcm_sample_bit_depth_luma_minus1 = u(pRP, 4);
    pSPS->pcm_sample_bit_depth_chroma_minus1 = u(pRP, 4);

    COMPLY(pSPS->pcm_sample_bit_depth_luma_minus1 <= pSPS->bit_depth_luma_minus8 + 7);
    COMPLY(pSPS->pcm_sample_bit_depth_chroma_minus1 <= pSPS->bit_depth_chroma_minus8 + 7);

    pSPS->log2_min_pcm_luma_coding_block_size_minus3 = Clip3(ue(pRP), Min(pSPS->log2_min_luma_coding_block_size_minus3, 2), Min(pSPS->Log2CtbSize - 3, 2));

    COMPLY(pSPS->log2_min_pcm_luma_coding_block_size_minus3 >= Min(pSPS->log2_min_luma_coding_block_size_minus3, 2));
    COMPLY(pSPS->log2_min_pcm_luma_coding_block_size_minus3 <= Min(pSPS->Log2CtbSize - 3, 2));

    pSPS->log2_diff_max_min_pcm_luma_coding_block_size = Clip3(ue(pRP), 0, Min(pSPS->Log2CtbSize - 3, 2) - pSPS->log2_min_pcm_luma_coding_block_size_minus3);

    COMPLY(pSPS->log2_diff_max_min_pcm_luma_coding_block_size <= (Min(pSPS->Log2CtbSize - 3, 2) - pSPS->log2_min_pcm_luma_coding_block_size_minus3));

    pSPS->pcm_loop_filter_disabled_flag = u(pRP, 1);
  }

  pSPS->num_short_term_ref_pic_sets = ue(pRP);

  COMPLY(pSPS->num_short_term_ref_pic_sets <= MAX_REF_PIC_SET);

  for(int i = 0; i < pSPS->num_short_term_ref_pic_sets; ++i)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    AL_HEVC_short_term_ref_pic_set(pSPS, i, pRP);

    pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] =
      Max(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1], pSPS->NumDeltaPocs[i]);
  }

  pSPS->long_term_ref_pics_present_flag = u(pRP, 1);

  if(pSPS->long_term_ref_pics_present_flag)
  {
    uint8_t syntax_size = pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
    pSPS->num_long_term_ref_pics_sps = ue(pRP);

    COMPLY(pSPS->num_long_term_ref_pics_sps <= MAX_LONG_TERM_PIC);

    for(int i = 0; i < pSPS->num_long_term_ref_pics_sps; ++i)
    {
      pSPS->lt_ref_pic_poc_lsb_sps[i] = u(pRP, syntax_size);
      pSPS->used_by_curr_pic_lt_sps_flag[i] = u(pRP, 1);
    }
  }
  pSPS->sps_temporal_mvp_enabled_flag = u(pRP, 1);
  pSPS->strong_intra_smoothing_enabled_flag = u(pRP, 1);

  pSPS->vui_parameters_present_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(pSPS->vui_parameters_present_flag)
    hevc_vui_parameters(&pSPS->vui_param, pSPS->sps_max_sub_layers_minus1, pRP);

  pSPS->sps_extension_present_flag = u(pRP, 1);

  if(pSPS->sps_extension_present_flag)
  {
    pSPS->sps_range_extension_flag = u(pRP, 1);
    pSPS->sps_extension_7bits = u(pRP, 7);
  }

  if(pSPS->sps_range_extension_flag)
  {
    pSPS->transform_skip_rotation_enabled_flag = u(pRP, 1);
    pSPS->transform_skip_context_enabled_flag = u(pRP, 1);
    pSPS->implicit_rdpcm_enabled_flag = u(pRP, 1);
    pSPS->explicit_rdpcm_enabled_flag = u(pRP, 1);
    pSPS->extended_precision_processing_flag = u(pRP, 1);
    pSPS->intra_smoothing_disabled_flag = u(pRP, 1);
    pSPS->high_precision_offsets_enabled_flag = u(pRP, 1);
    pSPS->persistent_rice_adaptation_enabled_flag = u(pRP, 1);
    pSPS->cabac_bypass_alignment_enabled_flag = u(pRP, 1);
  }

  if(pSPS->sps_extension_7bits) // sps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // sps_extension_data_flag
  }

  // Compute variables
  pSPS->PicWidthInCtbs = (pSPS->pic_width_in_luma_samples + ((1 << pSPS->Log2CtbSize) - 1)) >> pSPS->Log2CtbSize;
  pSPS->PicHeightInCtbs = (pSPS->pic_height_in_luma_samples + ((1 << pSPS->Log2CtbSize) - 1)) >> pSPS->Log2CtbSize;

  COMPLY_WITH_LOG(pSPS->PicWidthInCtbs >= 2, "SPS width less than 2 CTBs is not supported\n");
  COMPLY_WITH_LOG(pSPS->PicHeightInCtbs >= 2, "SPS height less than 2 CTBs is not supported\n");

  pSPS->PicWidthInMinCbs = pSPS->pic_width_in_luma_samples >> pSPS->Log2MinCbSize;
  pSPS->PicHeightInMinCbs = pSPS->pic_height_in_luma_samples >> pSPS->Log2MinCbSize;

  pSPS->SpsMaxLatency = pSPS->sps_max_latency_increase_plus1[max_sub_layers] ? pSPS->sps_max_num_reorder_pics[max_sub_layers] + pSPS->sps_max_latency_increase_plus1[max_sub_layers] - 1 : UINT32_MAX;

  pSPS->MaxPicOrderCntLsb = 1 << (pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 + 4);

  pSPS->WpOffsetBdShiftY = pSPS->high_precision_offsets_enabled_flag ? 0 : pSPS->bit_depth_luma_minus8;
  pSPS->WpOffsetBdShiftC = pSPS->high_precision_offsets_enabled_flag ? 0 : pSPS->bit_depth_chroma_minus8;
  pSPS->WpOffsetHalfRangeY = 1 << (pSPS->high_precision_offsets_enabled_flag ? pSPS->bit_depth_luma_minus8 + 7 : 7);
  pSPS->WpOffsetHalfRangeC = 1 << (pSPS->high_precision_offsets_enabled_flag ? pSPS->bit_depth_chroma_minus8 + 7 : 7);

  COMPLY(rbsp_trailing_bits(pRP));

  pSPS->bConceal = false;

  return AL_OK;
}

/*****************************************************************************/
void AL_HEVC_short_term_ref_pic_set(AL_THevcSps* pSPS, uint8_t RefIdx, AL_TRbspParser* pRP)
{
  uint8_t RIdx;
  AL_TRefPicSet* pRefPicSet = &pSPS->short_term_ref_pic_set[RefIdx];

  // default values
  pRefPicSet->delta_idx_minus1 = 0;
  pRefPicSet->inter_ref_pic_set_prediction_flag = 0;

  if(RefIdx)
    pRefPicSet->inter_ref_pic_set_prediction_flag = u(pRP, 1);

  if(pRefPicSet->inter_ref_pic_set_prediction_flag)
  {
    if(RefIdx == pSPS->num_short_term_ref_pic_sets)
      pRefPicSet->delta_idx_minus1 = ue(pRP);
    pRefPicSet->delta_rps_sign = u(pRP, 1);
    pRefPicSet->abs_delta_rps_minus1 = ue(pRP);

    RIdx = RefIdx - (pRefPicSet->delta_idx_minus1 + 1);

    for(uint8_t j = 0; j <= pSPS->NumDeltaPocs[RIdx]; ++j)
    {
      pRefPicSet->use_delta_flag[j] = 1;
      pRefPicSet->used_by_curr_pic_flag[j] = u(pRP, 1);

      if(!pRefPicSet->used_by_curr_pic_flag[j])
        pRefPicSet->use_delta_flag[j] = u(pRP, 1);
    }
  }
  else
  {
    pRefPicSet->num_negative_pics = ue(pRP);
    pRefPicSet->num_positive_pics = ue(pRP);

    for(uint8_t j = 0; j < pRefPicSet->num_negative_pics; ++j)
    {
      pRefPicSet->delta_poc_s0_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s0_flag[j] = u(pRP, 1);
    }

    for(uint8_t j = 0; j < pRefPicSet->num_positive_pics; ++j)
    {
      pRefPicSet->delta_poc_s1_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s1_flag[j] = u(pRP, 1);
    }
  }
  AL_HEVC_sComputeRefPicSetVariables(pSPS, RefIdx);
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseVPS(AL_TAup* pIAup, AL_TRbspParser* pRP)
{
  AL_THevcVps* pVPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  int vps_id = u(pRP, 4);

  if(vps_id >= AL_HEVC_MAX_VPS)
    return AL_UNSUPPORTED;

  pVPS = &pIAup->hevcAup.pVPS[vps_id];
  pVPS->vps_video_parameter_set_id = vps_id;

  pVPS->vps_base_layer_internal_flag = u(pRP, 1);
  pVPS->vps_base_layer_available_flag = u(pRP, 1);
  pVPS->vps_max_layers_minus1 = u(pRP, 6);

  pVPS->vps_max_sub_layers_minus1 = u(pRP, 3);
  pVPS->vps_temporal_id_nesting_flag = u(pRP, 1);
  skip(pRP, 16); // vps_reserved_0xffff_16bits

  hevc_profile_tier_level(&pVPS->profile_and_level[0], pVPS->vps_max_sub_layers_minus1, pRP);

  if(pVPS->profile_and_level[0].general_level_idc == 0)
    pVPS->profile_and_level[0].general_level_idc = CONCEAL_LEVEL_IDC;

  pVPS->vps_sub_layer_ordering_info_present_flag = u(pRP, 1);

  int layer_offset = pVPS->vps_sub_layer_ordering_info_present_flag ? 0 : pVPS->vps_max_sub_layers_minus1;

  for(int i = layer_offset; i <= pVPS->vps_max_sub_layers_minus1; ++i)
  {
    pVPS->vps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    pVPS->vps_max_num_reorder_pics[i] = ue(pRP);
    pVPS->vps_max_latency_increase_plus1[i] = ue(pRP);
  }

  pVPS->vps_max_layer_id = u(pRP, 6);
  pVPS->vps_num_layer_sets_minus1 = ue(pRP);

  for(int i = 1; i <= pVPS->vps_num_layer_sets_minus1; ++i)
  {
    uint16_t uOffset = Min(i, 1);

    for(int j = 0; j <= pVPS->vps_max_layer_id; j++)
      pVPS->layer_id_included_flag[uOffset][j] = u(pRP, 1);
  }

  pVPS->vps_timing_info_present_flag = u(pRP, 1);

  if(pVPS->vps_timing_info_present_flag)
  {
    pVPS->vps_num_units_in_tick = u(pRP, 32);
    pVPS->vps_time_scale = u(pRP, 32);
    pVPS->vps_poc_proportional_to_timing_flag = u(pRP, 1);

    if(pVPS->vps_poc_proportional_to_timing_flag)
      pVPS->vps_num_ticks_poc_diff_one_minus1 = ue(pRP);

    pVPS->vps_num_hrd_parameters = ue(pRP);

    for(int i = 0; i < pVPS->vps_num_hrd_parameters; ++i)
    {
      uint16_t uOffset = Min(i, 1);
      pVPS->hrd_layer_set_idx[uOffset] = ue(pRP);

      if(uOffset)
        pVPS->cprms_present_flag[uOffset] = u(pRP, 1);
      else
        pVPS->cprms_present_flag[uOffset] = 1;
      hevc_hrd_parameters(&pVPS->hrd_parameter[uOffset], pVPS->cprms_present_flag[uOffset], pVPS->vps_max_sub_layers_minus1, pRP);
    }
  }

  if(u(pRP, 1)) // vps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // vps_extension_data_flag
  }
  rbsp_trailing_bits(pRP);

  return AL_OK;
}

/*****************************************************************************/
static bool sei_active_parameter_sets(AL_TRbspParser* pRP, AL_THevcAup* aup, uint8_t* pSpsId)
{
  uint8_t active_video_parameter_set_id = u(pRP, 4);
  /*self_Containd_cvs_flag =*/ u(pRP, 1);
  /*no_parameter_set_update_flag =*/ u(pRP, 1);
  uint8_t num_sps_ids_minus1 = ue(pRP);

  uint8_t active_seq_parameter_set_id[16];

  for(int i = 0; i <= num_sps_ids_minus1; ++i)
    active_seq_parameter_set_id[i] = ue(pRP);

  AL_THevcVps* pVPS = &aup->pVPS[active_video_parameter_set_id];
  uint8_t MaxLayersMinus1 = Min(62, pVPS->vps_max_layers_minus1);

  for(int i = pVPS->vps_base_layer_internal_flag; i <= MaxLayersMinus1; ++i)
    /*layer_sps_idx[i] =*/ ue(pRP);

  *pSpsId = active_seq_parameter_set_id[0];

  return true;
}

/*****************************************************************************/
static bool sei_pic_timing(AL_TRbspParser* pRP, AL_THevcSps* pSPS, AL_THevcPicTiming* pPicTiming)
{
  if(pSPS == NULL || pSPS->bConceal)
    return false;

  Rtos_Memset(pPicTiming, 0, sizeof(*pPicTiming));

  if(pSPS->vui_param.frame_field_info_present_flag)
  {
    pPicTiming->pic_struct = u(pRP, 4);
    pPicTiming->source_scan_type = u(pRP, 2);
    pPicTiming->duplicate_flag = u(pRP, 1);
    pSPS->sei_source_scan_type = pPicTiming->source_scan_type;
  }

  bool CpbDpbDelaysPresentFlag = pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag
                                 || pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag;

  if(CpbDpbDelaysPresentFlag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->au_cpb_removal_delay_minus1 = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->pic_dpb_output_delay = u(pRP, syntax_size);

    if(pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    {
      syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_du_length_minus1 + 1;
      pPicTiming->pic_dpb_output_du_delay = u(pRP, syntax_size);

      if(pSPS->vui_param.hrd_param.sub_pic_cpb_params_in_pic_timing_sei_flag)
      {
        pPicTiming->num_decoding_units_minus1 = ue(pRP);
        pPicTiming->du_common_cpb_removal_delay_flag = u(pRP, 1);

        if(pPicTiming->du_common_cpb_removal_delay_flag)
        {
          syntax_size = pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1;
          pPicTiming->du_common_cpb_removal_delay_increment_minus1 = u(pRP, syntax_size);
        }

        for(uint8_t i = 0; i <= pPicTiming->num_decoding_units_minus1; ++i)
        {
          /*pPicTiming->num_nalus_in_du_minus1[i] = */
          ue(pRP);

          if(!pPicTiming->du_common_cpb_removal_delay_flag && i < pPicTiming->num_decoding_units_minus1)
            /*pPicTiming->du_cpb_removal_delay_increment_minus1[i] = */ u(pRP, syntax_size);
        }
      }
    }
  }
  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool sei_recovery_point(AL_TRbspParser* pRP, AL_TRecoveryPoint* pRecoveryPoint)
{
  Rtos_Memset(pRecoveryPoint, 0, sizeof(*pRecoveryPoint));

  pRecoveryPoint->recovery_cnt = se(pRP);
  pRecoveryPoint->exact_match = u(pRP, 1);
  pRecoveryPoint->broken_link = u(pRP, 1);

  return byte_alignment(pRP);
}

#define SEI_PTYPE_ACTIVE_PARAMETER_SETS 129

#define CONTIUE_PARSE_OR_SKIP(ParseCmd) \
  bRet = ParseCmd; \
  if(!bRet) \
  { \
    uOffset = offset(pRP) - uOffset; \
    if(uOffset > (uint32_t)payload_size << 3) \
      return false; \
    skip(pRP, (payload_size << 3) - uOffset); \
    break; \
  }

#define PARSE_OR_SKIP(ParseCmd) \
  uint32_t uOffset = offset(pRP); \
  bool bRet; \
  CONTIUE_PARSE_OR_SKIP(ParseCmd)

/*****************************************************************************/
bool AL_HEVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta)
{
  AL_THevcSei sei;
  AL_THevcAup* aup = &pIAup->hevcAup;
  sei.present_flags = 0;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  do
  {
    uint32_t payload_type = 0;
    int32_t payload_size = 0;

    // get payload type
    if(!byte_aligned(pRP))
      return false;

    uint8_t byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_type += 255;
      byte = getbyte(pRP);
    }

    payload_type += byte;

    // get payload size
    byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_size += 255;
      byte = getbyte(pRP);
    }

    uint32_t offsetBefore = offset(pRP);

    payload_size += byte;

    bool bCBAndSEIMeta = true; // Don't call CB or add in SEI metadata for HDR related SEIs, HDR settings are provided through dedicated metadata

    /* get payload data address, at this point we may not have the whole payload
     * data loaded in the rbsp parser */
    uint8_t* payload_data = get_raw_data(pRP);
    switch(payload_type)
    {
    case SEI_PTYPE_PIC_TIMING: // picture_timing parsing
    {
      if(aup->pActiveSPS)
      {
        PARSE_OR_SKIP(sei_pic_timing(pRP, aup->pActiveSPS, &sei.picture_timing));
        sei.present_flags |= AL_SEI_PT;
        aup->ePicStruct = sei.picture_timing.pic_struct;
      }
      else
        skip(pRP, payload_size << 3);
      break;
    }
    case SEI_PTYPE_RECOVERY_POINT: // picture_timing parsing
    {
      PARSE_OR_SKIP(sei_recovery_point(pRP, &sei.recovery_point));
      pIAup->iRecoveryCnt = sei.recovery_point.recovery_cnt + 1; // +1 for non-zero value when AL_SEI_RP is present
      break;
    }
    case SEI_PTYPE_ACTIVE_PARAMETER_SETS:
    {
      uint8_t uSpsId;
      PARSE_OR_SKIP(sei_active_parameter_sets(pRP, aup, &uSpsId));

      if(uSpsId >= AL_HEVC_MAX_SPS)
        return false;

      if(!aup->pSPS[uSpsId].bConceal)
        aup->pActiveSPS = &aup->pSPS[uSpsId];
      break;
    }
    case SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
    {
      PARSE_OR_SKIP(sei_mastering_display_colour_volume(&pIAup->tParsedHDRSEIs.tMDCV, pRP));
      pIAup->tParsedHDRSEIs.bHasMDCV = true;
      bCBAndSEIMeta = false;
      break;
    }
    case SEI_PTYPE_CONTENT_LIGHT_LEVEL:
    {
      PARSE_OR_SKIP(sei_content_light_level(&pIAup->tParsedHDRSEIs.tCLL, pRP));
      pIAup->tParsedHDRSEIs.bHasCLL = true;
      bCBAndSEIMeta = false;
      break;
    }
    case SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
    {
      PARSE_OR_SKIP(sei_alternative_transfer_characteristics(&pIAup->tParsedHDRSEIs.tATC, pRP));
      pIAup->tParsedHDRSEIs.bHasATC = true;
      bCBAndSEIMeta = false;
      break;
    }
    case SEI_PTYPE_USER_DATA_REGISTERED:
    {
      AL_EUserDataRegisterSEIType eSEIType;
      PARSE_OR_SKIP((eSEIType = sei_user_data_registered(pRP, payload_size)) != AL_UDR_SEI_UNKNOWN);
      switch(eSEIType)
      {
      case AL_UDR_SEI_ST2094_10:
      {
        CONTIUE_PARSE_OR_SKIP(sei_st2094_10(&pIAup->tParsedHDRSEIs.tST2094_10, pRP));
        pIAup->tParsedHDRSEIs.bHasST2094_10 = true;
        break;
      }
      case AL_UDR_SEI_ST2094_40:
      {
        CONTIUE_PARSE_OR_SKIP(sei_st2094_40(&pIAup->tParsedHDRSEIs.tST2094_40, pRP));
        pIAup->tParsedHDRSEIs.bHasST2094_40 = true;
        break;
      }
      default:
      {
        AL_Assert(0);
        break;
      }
      }

      bCBAndSEIMeta = false;
      break;
    }
    default: // payload not supported
    {
      skip(pRP, payload_size << 3); // skip data
      break;
    }
    }

    if(bCBAndSEIMeta && pMeta)
    {
      if(!AL_SeiMetaData_AddPayload(pMeta, (AL_TSeiMessage) {bIsPrefix, payload_type, payload_data, payload_size }))
        return false;
    }

    if(bCBAndSEIMeta && cb->func)
      cb->func(bIsPrefix, payload_type, payload_data, payload_size, cb->userParam);

    uint32_t offsetAfter = offset(pRP);
    // Skip payload extension data if any
    int32_t remainingPayload = (payload_size << 3) - (int32_t)(offsetAfter - offsetBefore);

    if(remainingPayload > 0)
      skip(pRP, remainingPayload);
  }
  while(more_rbsp_data(pRP));

  rbsp_trailing_bits(pRP);

  return true;
}

/*****************************************************************************/
AL_TCropInfo AL_HEVC_GetCropInfo(AL_THevcSps const* pSPS)
{
  // update cropping information
  AL_TCropInfo cropInfo =
  {
    0
  };
  cropInfo.bCropping = pSPS->conformance_window_flag;

  if(pSPS->conformance_window_flag)
  {
    if(pSPS->chroma_format_idc == 1 || pSPS->chroma_format_idc == 2)
    {
      cropInfo.uCropOffsetLeft += 2 * pSPS->conf_win_left_offset;
      cropInfo.uCropOffsetRight += 2 * pSPS->conf_win_right_offset;
    }
    else
    {
      cropInfo.uCropOffsetLeft += pSPS->conf_win_left_offset;
      cropInfo.uCropOffsetRight += pSPS->conf_win_right_offset;
    }

    if(pSPS->chroma_format_idc == 1)
    {
      cropInfo.uCropOffsetTop += 2 * pSPS->conf_win_top_offset;
      cropInfo.uCropOffsetBottom += 2 * pSPS->conf_win_bottom_offset;
    }
    else
    {
      cropInfo.uCropOffsetTop += pSPS->conf_win_top_offset;
      cropInfo.uCropOffsetBottom += pSPS->conf_win_bottom_offset;
    }
  }

  return cropInfo;
}
