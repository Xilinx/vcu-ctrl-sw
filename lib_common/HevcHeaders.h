// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "common_syntax_elements.h"

static int32_t const AL_HEVC_MIN_DBF_PARAM = -6;
static int32_t const AL_HEVC_MAX_DBF_PARAM = 6;

#define MAX_NUM_OUTPUT_LAYER 1024

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.1.2
*****************************************************************************/
typedef struct AL_THevcProfilevel
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

  uint8_t sub_layer_profile_present_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_level_present_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_profile_space[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_tier_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_profile_idc[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_profile_compatibility_flag[AL_MAX_SUB_LAYER + 1][32];

  uint8_t sub_layer_progressive_source_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_interlaced_source_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_non_packed_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_frame_only_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_12bit_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_10bit_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_8bit_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_422chroma_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_420chroma_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_monochrome_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_intra_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_one_picture_only_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_lower_bit_rate_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_max_14bit_constraint_flag[AL_MAX_SUB_LAYER + 1];
  uint8_t sub_layer_inbld_flag[AL_MAX_SUB_LAYER + 1];

  uint8_t sub_layer_level_idc[AL_MAX_SUB_LAYER + 1];
}AL_THevcProfilevel;
/****************************************************************************/
#define AL_HEVC_MAX_VPS 16

typedef struct AL_TRepFormat
{
  uint16_t pic_width_vps_in_luma_samples;
  uint16_t pic_height_vps_in_luma_samples;
  uint8_t chroma_and_bit_depth_vps_present_flag;
  uint8_t chroma_format_vps_idc;
  uint8_t separate_colour_plane_vps_flag;
  uint8_t bit_depth_vps_luma_minus8;
  uint8_t bit_depth_vps_chroma_minus8;
  uint8_t conformance_window_vps_flag;
  uint32_t conf_win_vps_left_offset;
  uint32_t conf_win_vps_right_offset;
  uint32_t conf_win_vps_top_offset;
  uint32_t conf_win_vps_bottom_offset;
}AL_TRepFormat;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.1.
*****************************************************************************/
typedef struct AL_THevcVps
{
  uint8_t vps_video_parameter_set_id;
  uint8_t vps_base_layer_internal_flag;
  uint8_t vps_base_layer_available_flag;
  uint8_t vps_max_layers_minus1;
  uint8_t vps_max_sub_layers_minus1;
  uint8_t vps_temporal_id_nesting_flag;
  uint8_t vps_sub_layer_ordering_info_present_flag;

  AL_THevcProfilevel profile_and_level[MAX_NUM_LAYER];

  uint8_t vps_max_dec_pic_buffering_minus1[8];
  uint8_t vps_max_num_reorder_pics[8];
  uint32_t vps_max_latency_increase_plus1[8];

  uint8_t vps_max_layer_id;
  uint16_t vps_num_layer_sets_minus1;
  uint8_t layer_id_included_flag[MAX_NUM_LAYER][64];

  uint8_t vps_timing_info_present_flag;
  uint32_t vps_num_units_in_tick;
  uint32_t vps_time_scale;
  uint8_t vps_poc_proportional_to_timing_flag;
  uint32_t vps_num_ticks_poc_diff_one_minus1;

  uint16_t vps_num_hrd_parameters;
  uint16_t hrd_layer_set_idx[2];
  uint8_t cprms_present_flag[2];

  AL_THrdParam hrd_parameter[2];

  uint8_t vps_extension_flag;
}AL_THevcVps;
/****************************************************************************/
// define max sps value respect to HEVC semantics
#define MAX_REF_PIC_SET 64
#define MAX_LONG_TERM_PIC 32
#define AL_HEVC_MAX_SPS 16

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.2.
*****************************************************************************/
typedef struct AL_THevcSps
{
  uint8_t nal_unit_type;
  uint8_t nuh_layer_id;
  uint8_t nuh_temporal_layer_id_plus_1;
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
  uint8_t sps_max_dec_pic_buffering_minus1[AL_MAX_SUB_LAYER + 1];
  uint8_t sps_max_num_reorder_pics[AL_MAX_SUB_LAYER + 1];
  uint32_t sps_max_latency_increase_plus1[AL_MAX_SUB_LAYER + 1];

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
  AL_TRefPicSet short_term_ref_pic_set[MAX_REF_PIC_SET + 1];
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
  uint8_t sps_extension_6bits;
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

  /* picture dpb latency variable */
  uint32_t SpsMaxLatency;

  /* short term reference picture set variables */
  uint8_t NumNegativePics[MAX_REF_PIC_SET + 1];
  int32_t DeltaPocS0[MAX_REF_PIC_SET + 1][AL_MAX_REF];
  uint8_t UsedByCurrPicS0[MAX_REF_PIC_SET + 1][AL_MAX_REF];
  uint8_t NumPositivePics[MAX_REF_PIC_SET + 1];
  int32_t DeltaPocS1[MAX_REF_PIC_SET + 1][AL_MAX_REF];
  uint8_t UsedByCurrPicS1[MAX_REF_PIC_SET + 1][AL_MAX_REF];
  uint8_t NumDeltaPocs[MAX_REF_PIC_SET + 1];

  /* picture order count variable */
  uint32_t MaxPicOrderCntLsb;
  uint8_t ChromaArrayType;

  uint8_t sei_source_scan_type;
}AL_THevcSps;

/*****************************************************************************/
#define AL_HEVC_MAX_PPS 64
static int32_t const AL_HEVC_MAX_REF_IDX = 14;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.3.
*****************************************************************************/
typedef struct AL_THevcPps
{
  uint8_t nal_unit_type;
  uint8_t nuh_layer_id;
  uint8_t nuh_temporal_layer_id_plus_1;
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
  uint16_t pTileColWidths[AL_MAX_COLUMNS_TILE];
  uint16_t pTileRowHeights[AL_MAX_ROWS_TILE];
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
  uint8_t pps_extension_6bits;
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
  uint8_t pps_scaling_list_ref_layer_id;
  uint32_t num_ref_loc_offsets;
  uint8_t ref_loc_offset_layer_id[MAX_NUM_LAYER]; // spec says 63 (XXX i think)

  bool scaled_ref_layer_offset_present_flag[MAX_NUM_LAYER];
  int32_t scaled_ref_layer_left_offset[MAX_NUM_LAYER];
  int32_t scaled_ref_layer_top_offset[MAX_NUM_LAYER];
  int32_t scaled_ref_layer_right_offset[MAX_NUM_LAYER];
  int32_t scaled_ref_layer_bottom_offset[MAX_NUM_LAYER];

  bool ref_region_offset_present_flag[MAX_NUM_LAYER];
  int32_t ref_region_left_offset[MAX_NUM_LAYER];
  int32_t ref_region_top_offset[MAX_NUM_LAYER];
  int32_t ref_region_right_offset[MAX_NUM_LAYER];
  int32_t ref_region_bottom_offset[MAX_NUM_LAYER];

  bool resample_phase_set_present_flag[MAX_NUM_LAYER];
  uint32_t phase_hor_luma[MAX_NUM_LAYER];
  uint32_t phase_ver_luma[MAX_NUM_LAYER];
  uint32_t phase_hor_chroma_plus8[MAX_NUM_LAYER];
  uint32_t phase_ver_chroma_plus8[MAX_NUM_LAYER];

  uint8_t colour_mapping_enabled_flag;
  AL_THevcSps* pSPS;

  /* concealment flag */
  bool bConceal;
}AL_THevcPps;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.3
*****************************************************************************/
typedef struct AL_THevcSliceHdr
{
  uint8_t nal_unit_type;
  uint8_t nuh_layer_id;
  uint8_t nuh_temporal_id_plus1;

  uint8_t first_slice_segment_in_pic_flag;
  uint8_t no_output_of_prior_pics_flag;
  uint8_t slice_pic_parameter_set_id;
  int32_t slice_segment_address;

  uint8_t dependent_slice_segment_flag;
  uint8_t slice_type;
  uint8_t pic_output_flag;
  uint8_t colour_plane_id;

  int32_t slice_pic_order_cnt_lsb;
  uint8_t short_term_ref_pic_set_sps_flag;
  uint8_t short_term_ref_pic_set_idx;
  uint8_t num_long_term_sps;
  uint8_t num_long_term_pics;
  uint8_t lt_idx_sps[32];
  uint32_t poc_lsb_lt[32];
  uint8_t used_by_curr_pic_lt_flag[32];
  uint8_t delta_poc_msb_present_flag[32];
  uint32_t delta_poc_msb_cycle_lt[32];

  uint8_t slice_sao_luma_flag;
  uint8_t slice_sao_chroma_flag;

  uint8_t slice_temporal_mvp_enable_flag;
  uint8_t num_ref_idx_active_override_flag;
  uint8_t num_ref_idx_l0_active_minus1;
  uint8_t num_ref_idx_l1_active_minus1;

  uint8_t inter_layer_pred_enabled_flag;
  uint8_t num_inter_layer_ref_pics_minus1;
  uint8_t interlayer_pred_layer_idc[MAX_NUM_LAYER];

  AL_TRefPicModif ref_pic_modif;

  uint8_t mvd_l1_zero_flag;
  uint8_t cabac_init_flag;
  uint8_t collocated_from_l0_flag;
  uint8_t collocated_ref_idx;

  AL_TWPTable pred_weight_table;

  uint8_t five_minus_max_num_merge_cand;
  int8_t slice_qp_delta;
  int8_t slice_cb_qp_offset;
  int8_t slice_cr_qp_offset;
  uint8_t cu_chroma_qp_offset_enabled_flag;

  uint8_t deblocking_filter_override_flag;
  uint8_t slice_deblocking_filter_disabled_flag;
  int8_t slice_beta_offset_div2;
  int8_t slice_tc_offset_div2;

  uint8_t slice_loop_filter_across_slices_enabled_flag;

  uint16_t num_entry_point_offsets;
  uint8_t offset_len_minus1;

  AL_THevcPps const* pPPS;
  AL_THevcSps* pSPS;

  // Variables
  uint8_t RapPicFlag;
  uint8_t IdrPicFlag;

  /* long term reference picture set variables */
  uint32_t PocLsbLt[32];
  uint8_t UsedByCurrPicLt[32];
  uint32_t DeltaPocMSBCycleLt[32];

  uint8_t NumPocStCurrBefore;
  uint8_t NumPocStCurrAfter;
  uint8_t NumPocStFoll;
  uint8_t NumPocLtCurr;
  uint8_t NumPocLtFoll;
  uint8_t NumPicTotalCurr;
  int32_t iCurrPOC;
  bool RefListBuilt;

  int32_t slice_header_length;
  /* Keep this at last position of structure since it allows to memset
   * AL_THevcSliceHdr without clearing entry_point_offset_minus1.
   */
  uint32_t entry_point_offset_minus1[AL_MAX_ENTRY_POINT];
}AL_THevcSliceHdr;
