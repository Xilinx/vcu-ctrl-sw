/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include "lib_common/common_syntax_elements.h"

#define MAX_BIT_DEPTH 4
#define MAX_POC_LSB 12
#define MAX_SUB_LAYER 7

/****************************************************************************/
#define AL_AVC_MAX_SPS 32

// define max sps value respect to AVC semantics
#define MAX_FRAME_NUM 12
#define MAX_POC_TYPE 2

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.1.
*****************************************************************************/
typedef struct t_Avc_Sps
{
  int profile_idc;
  uint8_t constraint_set0_flag;
  uint8_t constraint_set1_flag;
  uint8_t constraint_set2_flag;
  uint8_t constraint_set3_flag;
  uint8_t constraint_set4_flag;
  uint8_t constraint_set5_flag;
  uint8_t reserved_zero_2bits;
  int level_idc;
  uint8_t seq_parameter_set_id;

  uint8_t chroma_format_idc;
  uint8_t separate_colour_plane_flag;

  uint8_t bit_depth_luma_minus8;
  uint8_t bit_depth_chroma_minus8;

  uint8_t qpprime_y_zero_transform_bypass_flag;

  uint8_t seq_scaling_matrix_present_flag;
  uint8_t seq_scaling_list_present_flag[12]; // we can define eight matrices: Sl_4x4_Intra_Y, Sl_4x4_Intra_Cb, Sl_4x4_Intra_Cr, Sl_4x4_Inter_Y, Sl_4x4_Inter_Cb, Sl_4x4_Inter_Cr, Sl_8x8_Intra_Y, Sl_8x8_Inter_Y.
  uint8_t ScalingList4x4[6][16]; // use in decoding
  uint8_t ScalingList8x8[6][64];
  AL_TSCLParam scaling_list_param;  // use in encoding

  uint8_t log2_max_frame_num_minus4;
  uint8_t pic_order_cnt_type;
  uint8_t log2_max_pic_order_cnt_lsb_minus4;
  uint8_t delta_pic_order_always_zero_flag;
  int offset_for_non_ref_pic;
  int offset_for_top_to_bottom_field;
  uint8_t num_ref_frames_in_pic_order_cnt_cycle;
  int offset_for_ref_frame[256];
  uint32_t max_num_ref_frames;
  uint8_t gaps_in_frame_num_value_allowed_flag;

  uint16_t pic_width_in_mbs_minus1;
  uint16_t pic_height_in_map_units_minus1;
  uint8_t frame_mbs_only_flag;
  uint8_t field_pic_flag;
  uint8_t bottom_field_flag;
  uint8_t mb_adaptive_frame_field_flag;
  uint8_t direct_8x8_inference_flag;
  uint8_t frame_cropping_flag;
  int frame_crop_left_offset;
  int frame_crop_right_offset;
  int frame_crop_top_offset;
  int frame_crop_bottom_offset;

  uint8_t vui_parameters_present_flag;
  AL_TVuiParam vui_param;

  int iCurrInitialCpbRemovalDelay; // Used in the current picture buffering_period
  uint8_t UseDefaultScalingMatrix4x4Flag[6];
  uint8_t UseDefaultScalingMatrix8x8Flag[6];
  // TSpsExtMVC mvc_ext;

  // concealment flag
  bool bConceal;
}AL_TAvcSps;

/****************************************************************************/
#include "lib_common/VPS.h"

#define AL_HEVC_MAX_SPS 16
#define AL_SPS_UNKNOWN_ID 0xFF

// define max sps value respect to HEVC semantics
#define MAX_REF_PIC_SET 64
#define MAX_LONG_TERM_PIC 32

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.2.
*****************************************************************************/
typedef struct t_Hevc_Sps
{
  uint8_t sps_video_parameter_set_id;
  uint8_t sps_max_sub_layers_minus1;
  uint8_t sps_ext_or_max_sub_layers_minus1;
  uint8_t sps_temporal_id_nesting_flag;
  AL_THevcProfilevel profile_and_level;

  uint8_t sps_seq_parameter_set_id;
  uint8_t update_rep_format_flag;
  uint8_t sps_rep_format_idx;
  uint8_t chroma_format_idc;
  uint8_t separate_colour_plane_flag;

  uint16_t pic_width_in_luma_samples;
  uint16_t pic_height_in_luma_samples;
  uint8_t conformance_window_flag;
  uint16_t conf_win_left_offset;
  uint16_t conf_win_right_offset;
  uint16_t conf_win_top_offset;
  uint16_t conf_win_bottom_offset;

  uint8_t bit_depth_luma_minus8;
  uint8_t bit_depth_chroma_minus8;

  uint8_t log2_max_slice_pic_order_cnt_lsb_minus4;

  uint8_t sps_sub_layer_ordering_info_present_flag;
  uint8_t sps_max_dec_pic_buffering_minus1[MAX_SUB_LAYER + 1];
  uint8_t sps_max_num_reorder_pics[MAX_SUB_LAYER + 1];
  uint32_t sps_max_latency_increase_plus1[MAX_SUB_LAYER + 1];

  uint8_t log2_min_luma_coding_block_size_minus3;
  uint8_t log2_diff_max_min_luma_coding_block_size;
  uint8_t log2_min_transform_block_size_minus2;
  uint8_t log2_diff_max_min_transform_block_size;

  uint8_t max_transform_hierarchy_depth_intra;
  uint8_t max_transform_hierarchy_depth_inter;

  uint8_t scaling_list_enabled_flag;
  uint8_t sps_infer_scaling_list_flag;
  uint8_t sps_scaling_list_ref_layer_id;
  uint8_t sps_scaling_list_data_present_flag;
  AL_TSCLParam scaling_list_param;

  uint8_t amp_enabled_flag;
  uint8_t sample_adaptive_offset_enabled_flag;

  uint8_t pcm_enabled_flag;
  uint8_t pcm_sample_bit_depth_luma_minus1;
  uint8_t pcm_sample_bit_depth_chroma_minus1;
  uint8_t log2_min_pcm_luma_coding_block_size_minus3;
  uint8_t log2_diff_max_min_pcm_luma_coding_block_size;
  uint8_t pcm_loop_filter_disabled_flag;

  uint8_t num_short_term_ref_pic_sets;
  AL_TRefPicSet short_term_ref_pic_set[65];
  uint8_t long_term_ref_pics_present_flag;
  uint8_t num_long_term_ref_pics_sps;
  uint16_t lt_ref_pic_poc_lsb_sps[33];
  uint8_t used_by_curr_pic_lt_sps_flag[33];

  uint8_t sps_temporal_mvp_enabled_flag;
  uint8_t strong_intra_smoothing_enabled_flag;
  uint8_t vui_parameters_present_flag;
  AL_TVuiParam vui_param;

  uint8_t sps_extension_present_flag;
  uint8_t sps_range_extension_flag;
  uint8_t sps_multilayer_extension_flag;
  uint8_t sps_3d_extension_flag;
  uint8_t sps_scc_extension_flag;
  uint8_t sps_extension_4bits;
  uint8_t sps_extension_7bits;
  uint8_t inter_view_mv_vert_constraint_flag;
  uint8_t transform_skip_rotation_enabled_flag;
  uint8_t transform_skip_context_enabled_flag;
  uint8_t implicit_rdpcm_enabled_flag;
  uint8_t explicit_rdpcm_enabled_flag;
  uint8_t extended_precision_processing_flag;
  uint8_t intra_smoothing_disabled_flag;
  uint8_t high_precision_offsets_enabled_flag;
  uint8_t persistent_rice_adaptation_enabled_flag;
  uint8_t cabac_bypass_alignment_enabled_flag;

  uint8_t WpOffsetBdShiftY;
  uint8_t WpOffsetBdShiftC;
  uint16_t WpOffsetHalfRangeY;
  uint16_t WpOffsetHalfRangeC;

  AL_THevcVps* pVPS;

  /* concealment flag */
  bool bConceal;

  /* picture size variables */
  uint8_t Log2MinCbSize;
  uint8_t Log2CtbSize;
  uint16_t PicWidthInCtbs;
  uint16_t PicHeightInCtbs;
  uint16_t PicWidthInMinCbs;
  uint16_t PicHeightInMinCbs;

  /* picture dpb latency variable */
  uint32_t SpsMaxLatency;

  /* short term reference picture set variables */
  uint8_t NumNegativePics[65];
  int32_t DeltaPocS0[65][16];
  uint8_t UsedByCurrPicS0[65][16];
  uint8_t NumPositivePics[65];
  int32_t DeltaPocS1[65][16];
  uint8_t UsedByCurrPicS1[65][16];
  uint8_t NumDeltaPocs[65];

  /* picture order count variable */
  uint32_t MaxPicOrderCntLsb;
  uint8_t ChromaArrayType;

  uint8_t sei_source_scan_type;
}AL_THevcSps;

/****************************************************************************/
typedef union
{
  AL_TAvcSps AvcSPS;
  AL_THevcSps HevcSPS;
}AL_TSps;

/****************************************************************************/

/*@}*/

