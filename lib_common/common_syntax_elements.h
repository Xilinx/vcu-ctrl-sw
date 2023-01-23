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

#include "ScalingList.h"
#include "lib_common/BufCommonInternal.h"

#define MAX_NUM_CPB 32

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.1.2
*****************************************************************************/
typedef struct t_Profilevel
{
  uint8_t general_profile_space;
  uint8_t general_tier_flag;
  uint8_t general_profile_idc;
  uint8_t general_profile_compatibility_flag[32];

  uint8_t general_progressive_source_flag;
  uint8_t general_interlaced_source_flag;
  uint8_t general_non_packed_constraint_flag;
  uint8_t general_frame_only_constraint_flag;

  uint16_t general_rext_profile_flags;

  uint8_t general_max_12bit_constraint_flag;
  uint8_t general_max_10bit_constraint_flag;
  uint8_t general_max_8bit_constraint_flag;
  uint8_t general_max_422chroma_constraint_flag;
  uint8_t general_max_420chroma_constraint_flag;
  uint8_t general_max_monochrome_constraint_flag;
  uint8_t general_intra_constraint_flag;
  uint8_t general_one_picture_only_constraint_flag;
  uint8_t general_lower_bit_rate_constraint_flag;
  uint8_t general_max_14bit_constraint_flag;

  uint8_t general_inbld_flag;

  uint8_t general_level_idc;

  uint8_t sub_layer_profile_present_flag[8];
  uint8_t sub_layer_level_present_flag[8];
  uint8_t sub_layer_profile_space[8];
  uint8_t sub_layer_tier_flag[8];
  uint8_t sub_layer_profile_idc[8];
  uint8_t sub_layer_profile_compatibility_flag[8][32];

  uint8_t sub_layer_progressive_source_flag[8];
  uint8_t sub_layer_interlaced_source_flag[8];
  uint8_t sub_layer_non_packed_constraint_flag[8];
  uint8_t sub_layer_frame_only_constraint_flag[8];
  uint8_t sub_layer_max_12bit_constraint_flag[8];
  uint8_t sub_layer_max_10bit_constraint_flag[8];
  uint8_t sub_layer_max_8bit_constraint_flag[8];
  uint8_t sub_layer_max_422chroma_constraint_flag[8];
  uint8_t sub_layer_max_420chroma_constraint_flag[8];
  uint8_t sub_layer_max_monochrome_constraint_flag[8];
  uint8_t sub_layer_intra_constraint_flag[8];
  uint8_t sub_layer_one_picture_only_constraint_flag[8];
  uint8_t sub_layer_lower_bit_rate_constraint_flag[8];
  uint8_t sub_layer_max_14bit_constraint_flag[8];
  uint8_t sub_layer_inbld_flag[8];

  uint8_t sub_layer_level_idc[8];
}AL_THevcProfilevel;

/*************************************************************************//*!
   \brief Mimics structure to represent scaling list syntax elements
*****************************************************************************/
typedef struct t_SCLParam
{
  uint8_t scaling_list_pred_mode_flag[4][6];
  uint8_t scaling_list_pred_matrix_id_delta[4][6];
  uint8_t scaling_list_dc_coeff[2][6];
  uint8_t ScalingList[4][6][64]; // [SizeID][MatrixID][Coeffs]
  uint8_t UseDefaultScalingMatrixFlag[20];
}AL_TSCLParam;

/*************************************************************************//*!
   \brief Mimics structure to represent ref pic set syntax elements
*****************************************************************************/
typedef struct t_RefPicSet
{
  uint8_t inter_ref_pic_set_prediction_flag;
  uint8_t delta_idx_minus1;
  uint8_t delta_rps_sign;
  uint16_t abs_delta_rps_minus1;
  uint8_t used_by_curr_pic_flag[MAX_REF];
  uint8_t use_delta_flag[MAX_REF];
  uint8_t num_negative_pics;
  uint8_t num_positive_pics;
  uint16_t delta_poc_s0_minus1[MAX_REF];
  uint16_t delta_poc_s1_minus1[MAX_REF];
  uint8_t used_by_curr_pic_s0_flag[MAX_REF];
  uint8_t used_by_curr_pic_s1_flag[MAX_REF];
}AL_TRefPicSet;

/*************************************************************************//*!
   \brief Mimics structure to represent reordering syntax elements
*****************************************************************************/
typedef struct t_RefPicModif
{
  uint8_t ref_pic_list_modification_flag_l0;
  uint8_t list_entry_l0[MAX_REF];
  uint8_t ref_pic_list_modification_flag_l1;
  uint8_t list_entry_l1[MAX_REF];
}AL_TRefPicModif;

/*************************************************************************//*!
   \brief Mimics structure to represent weighted pred syntax elements
*****************************************************************************/
typedef struct t_WPCoeff
{
  uint8_t luma_weight_flag[MAX_REF];
  int8_t luma_delta_weight[MAX_REF];
  int16_t luma_offset[MAX_REF];

  uint8_t chroma_weight_flag[MAX_REF];
  int8_t chroma_delta_weight[MAX_REF][2];
  int16_t chroma_offset[MAX_REF][2];
}AL_TWPCoeff;

typedef struct t_WPTable
{
  uint8_t luma_log2_weight_denom;
  int8_t chroma_log2_weight_denom;

  AL_TWPCoeff tWpCoeff[2];
  uint8_t NumWeights[2];
}AL_TWPTable;

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. E.1.2
*****************************************************************************/
typedef struct t_SubHrdParam
{
  uint32_t bit_rate_value_minus1[MAX_NUM_CPB];
  uint32_t cpb_size_value_minus1[MAX_NUM_CPB];
  uint32_t cpb_size_du_value_minus1[MAX_NUM_CPB];
  uint32_t bit_rate_du_value_minus1[MAX_NUM_CPB];
  uint8_t cbr_flag[MAX_NUM_CPB];
}AL_TSubHrdParam;

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. E.1.2
*****************************************************************************/
typedef struct t_HrdParam
{
  uint8_t nal_hrd_parameters_present_flag;
  uint8_t vcl_hrd_parameters_present_flag;
  uint8_t sub_pic_hrd_params_present_flag;
  uint8_t tick_divisor_minus2;
  uint8_t du_cpb_removal_delay_increment_length_minus1;
  uint8_t sub_pic_cpb_params_in_pic_timing_sei_flag;
  uint8_t dpb_output_delay_du_length_minus1;

  uint8_t bit_rate_scale;
  uint8_t cpb_size_scale;
  uint8_t cpb_size_du_scale;
  uint8_t initial_cpb_removal_delay_length_minus1;
  uint8_t au_cpb_removal_delay_length_minus1;
  uint8_t dpb_output_delay_length_minus1;
  uint8_t time_offset_length;

  uint8_t fixed_pic_rate_general_flag[8];
  uint8_t fixed_pic_rate_within_cvs_flag[8];
  uint32_t elemental_duration_in_tc_minus1[8];
  uint8_t low_delay_hrd_flag[8];
  uint32_t cpb_cnt_minus1[8];

  AL_TSubHrdParam nal_sub_hrd_param;
  AL_TSubHrdParam vcl_sub_hrd_param;
}AL_THrdParam;

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. E.1.1
*****************************************************************************/
typedef struct t_VuiParam
{
  uint8_t aspect_ratio_info_present_flag;
  uint8_t aspect_ratio_idc;
  uint16_t sar_width;
  uint16_t sar_height;

  uint8_t overscan_info_present_flag;
  uint8_t overscan_appropriate_flag;

  uint8_t video_signal_type_present_flag;
  uint8_t video_format;
  uint8_t video_full_range_flag;
  uint8_t colour_description_present_flag;
  uint8_t colour_primaries;
  uint8_t transfer_characteristics;
  uint8_t matrix_coefficients;

  uint8_t chroma_loc_info_present_flag;
  uint8_t chroma_sample_loc_type_top_field;
  uint8_t chroma_sample_loc_type_bottom_field;
  uint8_t neutral_chroma_indication_flag;
  uint8_t field_seq_flag;
  uint8_t frame_field_info_present_flag;

  uint8_t default_display_window_flag;
  uint32_t def_disp_win_left_offset;
  uint32_t def_disp_win_right_offset;
  uint32_t def_disp_win_top_offset;
  uint32_t def_disp_win_bottom_offset;

  uint8_t vui_timing_info_present_flag;
  uint32_t vui_num_units_in_tick;
  uint32_t vui_time_scale;
  uint8_t vui_poc_proportional_to_timing_flag;
  uint32_t vui_num_ticks_poc_diff_one_minus1;

  uint8_t vui_hrd_parameters_present_flag;
  AL_THrdParam hrd_param;

  uint8_t poc_proportional_to_timing_flag;

  uint8_t fixed_frame_rate_flag;

  uint8_t low_delay_hrd_flag;
  uint8_t pic_struct_present_flag;
  uint32_t num_reorder_frames;
  uint32_t max_dec_frame_buffering;

  uint8_t bitstream_restriction_flag;
  uint8_t tiles_fixed_structure_flag;
  uint8_t motion_vectors_over_pic_boundaries_flag;
  uint8_t restricted_ref_pic_lists_flag;
  uint8_t min_spatial_segmentation_idc;
  uint8_t max_bytes_per_pic_denom;
  uint8_t max_bits_per_min_cu_denom;
  uint8_t log2_max_mv_length_horizontal;
  uint8_t log2_max_mv_length_vertical;
}AL_TVuiParam;

/*@}*/

