/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include "HevcParser.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
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
  pPPS->log2_sao_offset_scale_chroma = 0;
  pPPS->log2_sao_offset_scale_chroma = 0;

  pPPS->bConceal = true;
}

/*****************************************************************************/
void AL_HEVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId)
{
  AL_THevcAup* aup = &pIAup->hevcAup;
  uint16_t pps_id, QpBdOffset;
  uint16_t uLCUWidth, uLCUHeight;
  AL_THevcPps* pPPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  pps_id = ue(pRP);
  pPPS = &aup->pPPS[pps_id];

  if(pPpsId)
    *pPpsId = pps_id;

  // default values
  initPps(pPPS);

  if(pps_id >= AL_HEVC_MAX_PPS)
    return;

  pPPS->pps_pic_parameter_set_id = pps_id;
  pPPS->pps_seq_parameter_set_id = ue(pRP);

  if(pPPS->pps_seq_parameter_set_id >= AL_HEVC_MAX_SPS)
    return;

  pPPS->pSPS = &aup->pSPS[pPPS->pps_seq_parameter_set_id];

  if(pPPS->pSPS->bConceal)
    return;

  uLCUWidth = pPPS->pSPS->PicWidthInCtbs;
  uLCUHeight = pPPS->pSPS->PicHeightInCtbs;

  pPPS->dependent_slice_segments_enabled_flag = u(pRP, 1);
  pPPS->output_flag_present_flag = u(pRP, 1);
  pPPS->num_extra_slice_header_bits = u(pRP, 3);
  pPPS->sign_data_hiding_flag = u(pRP, 1);
  pPPS->cabac_init_present_flag = u(pRP, 1);

  pPPS->num_ref_idx_l0_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
  pPPS->num_ref_idx_l1_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

  QpBdOffset = 6 * pPPS->pSPS->bit_depth_luma_minus8;
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

    if(pPPS->num_tile_columns_minus1 >= uLCUWidth || pPPS->num_tile_rows_minus1 >= uLCUHeight ||
       pPPS->num_tile_columns_minus1 >= AL_MAX_COLUMNS_TILE || pPPS->num_tile_rows_minus1 >= AL_MAX_ROWS_TILE)
      return;

    if(!pPPS->uniform_spacing_flag)
    {
      uint16_t uClmnOffset = 0;
      uint16_t uLineOffset = 0;

      for(uint8_t i = 0; i < pPPS->num_tile_columns_minus1; ++i)
      {
        pPPS->column_width[i] = ue(pRP) + 1;
        uClmnOffset += pPPS->column_width[i];
      }

      for(uint8_t i = 0; i < pPPS->num_tile_rows_minus1; ++i)
      {
        pPPS->row_height[i] = ue(pRP) + 1;
        uLineOffset += pPPS->row_height[i];
      }

      if(uClmnOffset >= uLCUWidth || uLineOffset >= uLCUHeight)
        return;

      pPPS->column_width[pPPS->num_tile_columns_minus1] = uLCUWidth - uClmnOffset;
      pPPS->row_height[pPPS->num_tile_rows_minus1] = uLCUHeight - uLineOffset;
    }
    else /* tile of same size */
    {
      uint16_t num_clmn = pPPS->num_tile_columns_minus1 + 1;
      uint16_t num_line = pPPS->num_tile_rows_minus1 + 1;

      for(uint8_t i = 0; i <= pPPS->num_tile_columns_minus1; ++i)
        pPPS->column_width[i] = (((i + 1) * uLCUWidth) / num_clmn) - ((i * uLCUWidth) / num_clmn);

      for(uint8_t i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
        pPPS->row_height[i] = (((i + 1) * uLCUHeight) / num_line) - ((i * uLCUHeight) / num_line);
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
          uLine += pPPS->row_height[line++];

        while(clmn < j)
          uClmn += pPPS->column_width[clmn++];

        pPPS->TileTopology[(i * (pPPS->num_tile_columns_minus1 + 1)) + j] = uLine * uLCUWidth + uClmn;
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

  pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 = 23;
  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 23;

  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 0;

  pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag = 0;
  Rtos_Memset(pSPS->sps_max_dec_pic_buffering_minus1, 0, sizeof(pSPS->sps_max_dec_pic_buffering_minus1));
  Rtos_Memset(pSPS->sps_num_reorder_pics, 0, sizeof(pSPS->sps_num_reorder_pics));
  Rtos_Memset(pSPS->sps_max_latency_increase_plus1, 0, sizeof(pSPS->sps_max_latency_increase_plus1));

  Rtos_Memset(pSPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, 20);
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_TAup* pIAup, AL_TRbspParser* pRP)
{
  AL_THevcSps tempSPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  int vps_id = u(pRP, 4);
  int max_sub_layers = Clip3(u(pRP, 3), 0, MAX_SUB_LAYER - 1);
  int temp_id_nesting_flag = u(pRP, 1);

  profile_tier_level(&tempSPS.profile_and_level, max_sub_layers, pRP);

  if(tempSPS.profile_and_level.general_level_idc == 0)
    tempSPS.profile_and_level.general_level_idc = CONCEAL_LEVEL_IDC;

  int sps_id = ue(pRP);

  pIAup->hevcAup.pSPS[sps_id].bConceal = true;

  COMPLY(sps_id < AL_HEVC_MAX_SPS);

  tempSPS.bConceal = true;
  tempSPS.sps_video_parameter_set_id = vps_id;
  tempSPS.sps_max_sub_layers_minus1 = max_sub_layers;
  tempSPS.sps_temporal_id_nesting_flag = temp_id_nesting_flag;
  tempSPS.sps_seq_parameter_set_id = sps_id;

  // default values
  initSps(&tempSPS);

  tempSPS.chroma_format_idc = ue(pRP);

  if(tempSPS.chroma_format_idc == 3)
    tempSPS.separate_colour_plane_flag = u(pRP, 1);

  if(tempSPS.separate_colour_plane_flag)
    tempSPS.ChromaArrayType = 0;
  else
    tempSPS.ChromaArrayType = tempSPS.chroma_format_idc;

  tempSPS.pic_width_in_luma_samples = ue(pRP);
  tempSPS.pic_height_in_luma_samples = ue(pRP);

  tempSPS.conformance_window_flag = u(pRP, 1);

  if(tempSPS.conformance_window_flag)
  {
    tempSPS.conf_win_left_offset = ue(pRP);
    tempSPS.conf_win_right_offset = ue(pRP);
    tempSPS.conf_win_top_offset = ue(pRP);
    tempSPS.conf_win_bottom_offset = ue(pRP);

    if(tempSPS.conf_win_bottom_offset + tempSPS.conf_win_top_offset >= tempSPS.pic_height_in_luma_samples)
    {
      tempSPS.conf_win_top_offset = 0;
      tempSPS.conf_win_bottom_offset = 0;
    }

    if(tempSPS.conf_win_left_offset + tempSPS.conf_win_right_offset >= tempSPS.pic_width_in_luma_samples)
    {
      tempSPS.conf_win_left_offset = 0;
      tempSPS.conf_win_right_offset = 0;
    }
  }

  tempSPS.bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);
  tempSPS.bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);

  tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 = ue(pRP);

  COMPLY(tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB);

  tempSPS.sps_sub_layer_ordering_info_present_flag = u(pRP, 1);
  int layer_offset = tempSPS.sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers;

  for(int i = layer_offset; i <= max_sub_layers; ++i)
  {
    tempSPS.sps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    tempSPS.sps_num_reorder_pics[i] = ue(pRP);
    tempSPS.sps_max_latency_increase_plus1[i] = ue(pRP);
  }

  tempSPS.log2_min_luma_coding_block_size_minus3 = ue(pRP);
  tempSPS.Log2MinCbSize = tempSPS.log2_min_luma_coding_block_size_minus3 + 3;

  COMPLY(tempSPS.Log2MinCbSize <= 6);

  tempSPS.log2_diff_max_min_luma_coding_block_size = ue(pRP);
  tempSPS.Log2CtbSize = tempSPS.Log2MinCbSize + tempSPS.log2_diff_max_min_luma_coding_block_size;

  COMPLY(tempSPS.Log2CtbSize <= 6);
  COMPLY(tempSPS.Log2CtbSize >= 4);

  tempSPS.log2_min_transform_block_size_minus2 = ue(pRP);
  int Log2MinTransfoSize = tempSPS.log2_min_transform_block_size_minus2 + 2;
  tempSPS.log2_diff_max_min_transform_block_size = ue(pRP);

  COMPLY(tempSPS.log2_min_transform_block_size_minus2 <= tempSPS.log2_min_luma_coding_block_size_minus3);
  COMPLY(tempSPS.log2_diff_max_min_transform_block_size <= Min(tempSPS.Log2CtbSize, 5) - Log2MinTransfoSize);

  tempSPS.max_transform_hierarchy_depth_inter = ue(pRP);
  tempSPS.max_transform_hierarchy_depth_intra = ue(pRP);

  COMPLY(tempSPS.max_transform_hierarchy_depth_inter <= (tempSPS.Log2CtbSize - Log2MinTransfoSize));
  COMPLY(tempSPS.max_transform_hierarchy_depth_intra <= (tempSPS.Log2CtbSize - Log2MinTransfoSize));

  tempSPS.scaling_list_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(tempSPS.scaling_list_enabled_flag)
  {
    tempSPS.sps_scaling_list_data_present_flag = u(pRP, 1);

    if(tempSPS.sps_scaling_list_data_present_flag)
      hevc_scaling_list_data(&tempSPS.scaling_list_param, pRP);
    else
      for(int i = 0; i < 20; ++i)
        tempSPS.scaling_list_param.UseDefaultScalingMatrixFlag[i] = 1;
  }

  tempSPS.amp_enabled_flag = u(pRP, 1);
  tempSPS.sample_adaptive_offset_enabled_flag = u(pRP, 1);

  tempSPS.pcm_enabled_flag = u(pRP, 1);

  if(tempSPS.pcm_enabled_flag)
  {
    tempSPS.pcm_sample_bit_depth_luma_minus1 = u(pRP, 4);
    tempSPS.pcm_sample_bit_depth_chroma_minus1 = u(pRP, 4);

    COMPLY(tempSPS.pcm_sample_bit_depth_luma_minus1 <= tempSPS.bit_depth_luma_minus8 + 7);
    COMPLY(tempSPS.pcm_sample_bit_depth_chroma_minus1 <= tempSPS.bit_depth_chroma_minus8 + 7);

    tempSPS.log2_min_pcm_luma_coding_block_size_minus3 = Clip3(ue(pRP), Min(tempSPS.log2_min_luma_coding_block_size_minus3, 2), Min(tempSPS.Log2CtbSize - 3, 2));

    COMPLY(tempSPS.log2_min_pcm_luma_coding_block_size_minus3 >= Min(tempSPS.log2_min_luma_coding_block_size_minus3, 2));
    COMPLY(tempSPS.log2_min_pcm_luma_coding_block_size_minus3 <= Min(tempSPS.Log2CtbSize - 3, 2));

    tempSPS.log2_diff_max_min_pcm_luma_coding_block_size = Clip3(ue(pRP), 0, Min(tempSPS.Log2CtbSize - 3, 2) - tempSPS.log2_min_pcm_luma_coding_block_size_minus3);

    COMPLY(tempSPS.log2_diff_max_min_pcm_luma_coding_block_size <= (Min(tempSPS.Log2CtbSize - 3, 2) - tempSPS.log2_min_pcm_luma_coding_block_size_minus3));

    tempSPS.pcm_loop_filter_disabled_flag = u(pRP, 1);
  }

  tempSPS.num_short_term_ref_pic_sets = ue(pRP);

  COMPLY(tempSPS.num_short_term_ref_pic_sets <= MAX_REF_PIC_SET);

  for(int i = 0; i < tempSPS.num_short_term_ref_pic_sets; ++i)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    AL_HEVC_short_term_ref_pic_set(&tempSPS, i, pRP);
  }

  tempSPS.long_term_ref_pics_present_flag = u(pRP, 1);

  if(tempSPS.long_term_ref_pics_present_flag)
  {
    uint8_t syntax_size = tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
    tempSPS.num_long_term_ref_pics_sps = ue(pRP);

    COMPLY(tempSPS.num_long_term_ref_pics_sps <= MAX_LONG_TERM_PIC);

    for(int i = 0; i < tempSPS.num_long_term_ref_pics_sps; ++i)
    {
      tempSPS.lt_ref_pic_poc_lsb_sps[i] = u(pRP, syntax_size);
      tempSPS.used_by_curr_pic_lt_sps_flag[i] = u(pRP, 1);
    }
  }
  tempSPS.sps_temporal_mvp_enabled_flag = u(pRP, 1);
  tempSPS.strong_intra_smoothing_enabled_flag = u(pRP, 1);

  tempSPS.vui_parameters_present_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(tempSPS.vui_parameters_present_flag)
    hevc_vui_parameters(&tempSPS.vui_param, tempSPS.sps_max_sub_layers_minus1, pRP);

  tempSPS.sps_extension_present_flag = u(pRP, 1);

  if(tempSPS.sps_extension_present_flag)
  {
    tempSPS.sps_range_extension_flag = u(pRP, 1);
    tempSPS.sps_extension_7bits = u(pRP, 7);
  }

  if(tempSPS.sps_range_extension_flag)
  {
    tempSPS.transform_skip_rotation_enabled_flag = u(pRP, 1);
    tempSPS.transform_skip_context_enabled_flag = u(pRP, 1);
    tempSPS.implicit_rdpcm_enabled_flag = u(pRP, 1);
    tempSPS.explicit_rdpcm_enabled_flag = u(pRP, 1);
    tempSPS.extended_precision_processing_flag = u(pRP, 1);
    tempSPS.intra_smoothing_disabled_flag = u(pRP, 1);
    tempSPS.high_precision_offsets_enabled_flag = u(pRP, 1);
    tempSPS.persistent_rice_adaptation_enabled_flag = u(pRP, 1);
    tempSPS.cabac_bypass_alignment_enabled_flag = u(pRP, 1);
  }

  if(tempSPS.sps_extension_7bits) // sps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // sps_extension_data_flag
  }

  // Compute variables
  tempSPS.PicWidthInCtbs = (tempSPS.pic_width_in_luma_samples + ((1 << tempSPS.Log2CtbSize) - 1)) >> tempSPS.Log2CtbSize;
  tempSPS.PicHeightInCtbs = (tempSPS.pic_height_in_luma_samples + ((1 << tempSPS.Log2CtbSize) - 1)) >> tempSPS.Log2CtbSize;

  COMPLY(tempSPS.PicWidthInCtbs > 1);
  COMPLY(tempSPS.PicHeightInCtbs > 1);

  tempSPS.PicWidthInMinCbs = tempSPS.pic_width_in_luma_samples >> tempSPS.Log2MinCbSize;
  tempSPS.PicHeightInMinCbs = tempSPS.pic_height_in_luma_samples >> tempSPS.Log2MinCbSize;

  tempSPS.SpsMaxLatency = tempSPS.sps_max_latency_increase_plus1[max_sub_layers] ? tempSPS.sps_num_reorder_pics[max_sub_layers] + tempSPS.sps_max_latency_increase_plus1[max_sub_layers] - 1 : UINT32_MAX;

  tempSPS.MaxPicOrderCntLsb = 1 << (tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 + 4);

  tempSPS.WpOffsetBdShiftY = tempSPS.high_precision_offsets_enabled_flag ? 0 : tempSPS.bit_depth_luma_minus8;
  tempSPS.WpOffsetBdShiftC = tempSPS.high_precision_offsets_enabled_flag ? 0 : tempSPS.bit_depth_chroma_minus8;
  tempSPS.WpOffsetHalfRangeY = 1 << (tempSPS.high_precision_offsets_enabled_flag ? tempSPS.bit_depth_luma_minus8 + 7 : 7);
  tempSPS.WpOffsetHalfRangeC = 1 << (tempSPS.high_precision_offsets_enabled_flag ? tempSPS.bit_depth_chroma_minus8 + 7 : 7);

  COMPLY(rbsp_trailing_bits(pRP));

  tempSPS.bConceal = false;
  pIAup->hevcAup.pSPS[sps_id] = tempSPS;

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
    pRefPicSet->num_negative_pics = Clip3(ue(pRP), 0, pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1]);
    pRefPicSet->num_positive_pics = Clip3(ue(pRP), 0, pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1]);

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
void ParseVPS(AL_TAup* pIAup, AL_TRbspParser* pRP)
{
  AL_THevcVps* pVPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  int vps_id = u(pRP, 4);
  pVPS = &pIAup->hevcAup.pVPS[vps_id];
  pVPS->vps_video_parameter_set_id = vps_id;

  pVPS->vps_base_layer_internal_flag = u(pRP, 1);
  pVPS->vps_base_layer_available_flag = u(pRP, 1);
  pVPS->vps_max_layers_minus1 = u(pRP, 6);

  pVPS->vps_max_sub_layers_minus1 = u(pRP, 3);
  pVPS->vps_temporal_id_nesting_flag = u(pRP, 1);
  skip(pRP, 16); // vps_reserved_0xffff_16bits

  profile_tier_level(&pVPS->profile_and_level[0], pVPS->vps_max_sub_layers_minus1, pRP);

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

#define PIC_TIMING 1
#define ACTIVE_PARAMETER_SETS 129
#define RECOVERY_POINT 6

#define PARSE_OR_SKIP(ParseCmd) \
  uint32_t uOffset = offset(pRP); \
  bool bRet = ParseCmd; \
  if(!bRet) \
  { \
    uOffset = offset(pRP) - uOffset; \
    if(uOffset > (uint32_t)payload_size << 3) \
      return false; \
    skip(pRP, (payload_size << 3) - uOffset); \
    break; \
  }

/*****************************************************************************/
bool AL_HEVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb)
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

    /* get payload data address, at this point we may not have the whole payload
     * data loaded in the rbsp parser */
    uint8_t* payload_data = get_raw_data(pRP);
    switch(payload_type)
    {
    case PIC_TIMING: // picture_timing parsing
    {
      if(aup->pActiveSPS)
      {
        PARSE_OR_SKIP(sei_pic_timing(pRP, aup->pActiveSPS, &sei.picture_timing));
        sei.present_flags |= SEI_PT;
        aup->ePicStruct = sei.picture_timing.pic_struct;
      }
      else
        skip(pRP, payload_size << 3);
      break;
    }
    case RECOVERY_POINT: // picture_timing parsing
    {
      PARSE_OR_SKIP(sei_recovery_point(pRP, &sei.recovery_point));
      aup->iRecoveryCnt = sei.recovery_point.recovery_cnt + 1; // +1 for non-zero value when SEI_RP is present
      break;
    }
    case ACTIVE_PARAMETER_SETS:
    {
      uint8_t uSpsId;
      PARSE_OR_SKIP(sei_active_parameter_sets(pRP, aup, &uSpsId));

      if(!aup->pSPS[uSpsId].bConceal)
        aup->pActiveSPS = &aup->pSPS[uSpsId];
      break;
    }
    default: // payload not supported
    {
      skip(pRP, payload_size << 3); // skip data
      break;
    }
    }

    if(cb->func)
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

