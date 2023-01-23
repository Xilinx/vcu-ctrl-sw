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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "common_syntax_elements.h"
#include "SPS.h"

#define AL_MAX_WP_IDC 2
#define AL_MIN_INIT_QP -26
#define AL_MAX_INIT_QP 25
#define AL_MIN_QP_OFFSET -12
#define AL_MAX_QP_OFFSET 12
#define AL_MIN_DBF_PARAM -6
#define AL_MAX_DBF_PARAM 6

/*****************************************************************************/
#define AL_AVC_MAX_PPS 256
#define AL_AVC_MAX_REF_IDX 15

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.2.
*****************************************************************************/
typedef struct t_Avc_Pps
{
  uint8_t pic_parameter_set_id;
  uint8_t seq_parameter_set_id;
  uint8_t entropy_coding_mode_flag;
  uint8_t bottom_field_pic_order_in_frame_present_flag;

  int num_slice_groups_minus1;
  uint8_t slice_group_map_type;
  uint16_t run_length_minus1[8]; // num_slice_groups_minus1 range 0 to 7 inclusive
  uint16_t top_left[8]; // num_slice_groups_minus1 range 0 to 7 inclusive
  uint16_t bottom_right[8]; // num_slice_groups_minus1 range 0 to 7 inclusive
  uint8_t slice_group_change_direction_flag;
  uint16_t slice_group_change_rate_minus1;
  uint16_t pic_size_in_map_units_minus1;
  int slice_group_id[8160]; // the max of pic_size_in_map_units in Full-HD is 8160(120*68)

  uint8_t num_ref_idx_l0_active_minus1;
  uint8_t num_ref_idx_l1_active_minus1;

  uint8_t weighted_pred_flag;
  uint8_t weighted_bipred_idc;

  int8_t pic_init_qp_minus26;
  int8_t pic_init_qs_minus26;
  int8_t chroma_qp_index_offset;
  int8_t second_chroma_qp_index_offset;

  uint8_t deblocking_filter_control_present_flag;
  uint8_t constrained_intra_pred_flag;
  uint8_t redundant_pic_cnt_present_flag;

  uint8_t transform_8x8_mode_flag;

  uint8_t pic_scaling_matrix_present_flag;
  uint8_t pic_scaling_list_present_flag[12];
  uint8_t ScalingList4x4[6][16];
  uint8_t ScalingList8x8[6][64];
  uint8_t UseDefaultScalingMatrix4x4Flag[6];
  uint8_t UseDefaultScalingMatrix8x8Flag[6];

  AL_TAvcSps* pSPS;

  // concealment flag
  bool bConceal;
}AL_TAvcPps;

/*****************************************************************************/
#define AL_HEVC_MAX_PPS 64
#define AL_HEVC_MAX_REF_IDX 14

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.3.
*****************************************************************************/
typedef struct t_Hevc_Pps
{
  uint8_t pps_pic_parameter_set_id;
  uint8_t pps_seq_parameter_set_id;
  uint8_t dependent_slice_segments_enabled_flag;

  uint8_t sign_data_hiding_flag;
  uint8_t cabac_init_present_flag;

  uint8_t num_ref_idx_l0_default_active_minus1;
  uint8_t num_ref_idx_l1_default_active_minus1;

  int8_t init_qp_minus26;
  uint8_t constrained_intra_pred_flag;
  uint8_t transform_skip_enabled_flag;
  uint8_t cu_qp_delta_enabled_flag;
  uint8_t diff_cu_qp_delta_depth;

  int8_t pps_cb_qp_offset;
  int8_t pps_cr_qp_offset;
  uint8_t pps_slice_chroma_qp_offsets_present_flag;

  uint8_t weighted_pred_flag;
  uint8_t weighted_bipred_flag;

  uint8_t output_flag_present_flag;
  uint8_t transquant_bypass_enabled_flag;

  uint8_t tiles_enabled_flag;
  uint8_t entropy_coding_sync_enabled_flag;
  uint16_t num_tile_columns_minus1;
  uint16_t num_tile_rows_minus1;

  uint8_t uniform_spacing_flag;
  uint16_t tile_column_width[AL_MAX_COLUMNS_TILE];
  uint16_t tile_row_height[AL_MAX_ROWS_TILE];
  uint32_t TileTopology[AL_MAX_NUM_TILE];
  uint8_t loop_filter_across_tiles_enabled_flag;

  uint8_t loop_filter_across_slices_enabled_flag;
  uint8_t deblocking_filter_control_present_flag;
  uint8_t deblocking_filter_override_enabled_flag;
  uint8_t pps_deblocking_filter_disabled_flag;
  int8_t pps_beta_offset_div2;
  int8_t pps_tc_offset_div2;

  uint8_t pps_scaling_list_data_present_flag;
  AL_TSCLParam scaling_list_param;

  uint8_t lists_modification_present_flag;
  uint8_t log2_parallel_merge_level_minus2;

  uint8_t num_extra_slice_header_bits;
  uint8_t slice_segment_header_extension_present_flag;

  uint8_t pps_extension_present_flag;
  uint8_t pps_range_extension_flag;
  uint8_t pps_multilayer_extension_flag;
  uint8_t pps_3d_extension_flag;
  uint8_t pps_scc_extension_flag;
  uint8_t pps_extension_4bits;
  uint8_t pps_extension_7bits;
  uint8_t log2_transform_skip_block_size_minus2;
  uint8_t cross_component_prediction_enabled_flag;
  uint8_t chroma_qp_offset_list_enabled_flag;
  uint8_t diff_cu_chroma_qp_offset_depth;
  uint8_t chroma_qp_offset_list_len_minus1;
  int8_t cb_qp_offset_list[6];
  int8_t cr_qp_offset_list[6];
  uint8_t log2_sao_offset_scale_luma;
  uint8_t log2_sao_offset_scale_chroma;

  uint8_t poc_reset_info_present_flag;
  uint8_t pps_infer_scaling_list_flag;
  uint8_t pps_sclaing_list_ref_layer_id;
  uint32_t num_ref_loc_offsets;
  uint8_t colour_mapping_enabled_flag;

  AL_THevcSps* pSPS;

  /* concealment flag */
  bool bConceal;
}AL_THevcPps;

/****************************************************************************/
typedef union
{
  AL_TAvcPps AvcPPS;
  AL_THevcPps HevcPPS;
}AL_TPps;

/****************************************************************************/

/*@}*/

