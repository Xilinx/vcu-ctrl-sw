// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/HDR.h"
#include "lib_common/Index.h"
#include "lib_common/AomDefinesInternal.h"
#include "lib_common_dec/Av2Defines.h"
#include "lib_common/ScalingList.h"
#include "lib_common/PicFormat.h"

/*****************************************************************************/
typedef struct AL_TAv2SegmentationParams
{
  bool FeatureEnabled[AL_AV2_SEG_LVL_MAX];
  int16_t FeatureData[AL_AV2_SEG_LVL_MAX];
}AL_TAv2SegmentationParams;

/*****************************************************************************/
typedef struct AL_TAv2LrWienerNsParams
{
  int8_t tParams[3][AL_AV2_WIENER_NS_CLASSES][AL_AV2_WIENER_NS_CHROMA_COEFFS];
}AL_TAv2LrWienerNsParams;

/*****************************************************************************/
typedef struct AL_TAv2TileInfo
{
  uint8_t uniform_tile_spacing_flag;
  uint16_t SbMiColStarts[AOM_MAX_TILE_COLS + 1];
  uint16_t SbMiRowStarts[AOM_MAX_TILE_ROWS + 1];
  uint16_t MiColStarts[AOM_MAX_TILE_COLS + 1];
  uint16_t MiRowStarts[AOM_MAX_TILE_ROWS + 1];
  uint8_t TileColsLog2;
  uint8_t TileRowsLog2;
  uint8_t TileCols;
  uint8_t TileRows;
  AL_TDimension tFrameSbDim;
  uint8_t sbRows;
  uint8_t uSbShift;
}AL_TAv2TileInfo;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 5.5. Sequence header OBU syntax
*****************************************************************************/
typedef struct AL_TAv2SeqHeader
{
  bool bConceal;

  uint8_t seq_header_id;

  uint8_t seq_profile_idc;
  uint8_t seq_level_idx;
  uint8_t seq_tier;
  uint8_t single_picture_header_flag;
  uint8_t seq_lcr_id;
  uint8_t still_picture;

  uint8_t frame_width_bits_minus_1;
  uint8_t frame_height_bits_minus_1;
  uint32_t max_frame_width_minus_1;
  uint32_t max_frame_height_minus_1;

  uint8_t seq_cropping_window_present_flag;
  uint32_t seq_cropping_win_left_offset;
  uint32_t seq_cropping_win_right_offset;
  uint32_t seq_cropping_win_top_offset;
  uint32_t seq_cropping_win_bottom_offset;

  uint8_t chroma_format_idc;
  uint8_t bit_depth_idc;

  uint8_t max_initial_display_delay_minus_1;
  uint8_t max_tlayer_id;
  uint8_t max_mlayer_id;
  uint8_t SeqMaxMlayerCnt;
  uint8_t monotonic_output_order_flag;

  uint32_t operating_points_cnt_minus_1;
  uint32_t operating_point_idc[AL_AV2_MAX_NUM_OPERATING_POINTS];

  uint8_t decoder_model_present_for_this_op[AL_AV2_MAX_NUM_OPERATING_POINTS];
  uint8_t initial_display_delay_present_for_this_op[AL_AV2_MAX_NUM_OPERATING_POINTS];
  uint32_t initial_display_delay_minus_1[AL_AV2_MAX_NUM_OPERATING_POINTS];

  uint8_t use_256x256_superblock;
  uint8_t use_128x128_superblock;
  uint8_t enable_sdp;
  uint8_t enable_extended_sdp;
  uint8_t enable_ext_partitions;
  uint8_t enable_uneven_4way_partitions;
  uint8_t MaxAspectRatio;

  uint8_t enable_dip;
  uint8_t enable_intra_edge_filter;
  uint8_t seq_frame_motion_modes_present_flag;
  uint8_t seq_enabled_motion_modes[AL_AV2_MOTION_MODES];
  uint8_t enable_six_param_warp_delta;
  uint8_t enable_masked_compound;
  uint8_t enable_ref_frame_mvs;
  uint8_t reduced_ref_frame_mvs_mode;
  uint8_t seq_force_screen_content_tools;
  uint8_t seq_choose_integer_mv;
  uint8_t seq_force_integer_mv;
  uint8_t order_hint_bits_minus_1;

  uint8_t enable_cdef;
  uint8_t enable_gdf;
  uint8_t gdf_unit_matches_sb_size;

  uint8_t enable_restoration;
  uint8_t lr_tools_disable[2][AL_AV2_RESTORE_SWITCHABLE_TYPES];
  uint8_t lr_tools_uv_present;

  uint8_t separate_uv_delta_q;
  uint8_t equal_ac_dc_q;
  uint8_t y_dc_delta_q_enabled;
  uint8_t uv_dc_delta_q_enabled;
  uint8_t uv_ac_delta_q_enabled;

  uint8_t film_grain_params_present;
  uint8_t enable_refmvbank;
  AL_EAv2DrlReorder DrlOrder;
  uint8_t CdefOnSkipTxfm;
  uint8_t enable_avg_cdf;
  uint8_t avg_cdf_type;

  uint8_t explicit_ref_frame_map;
  uint8_t NumRefFrames;
  uint8_t ActiveNumRefFrames;
  uint8_t long_term_frame_id_bits;
  uint8_t seq_max_drl_bits_minus1;
  uint8_t allow_frame_max_drl_bits;
  uint8_t seq_max_bvp_drl_bits_minus1;
  uint8_t allow_frame_max_bvp_drl_bits;
  uint8_t num_same_ref_compound;
  uint8_t enable_intra_ist;
  uint8_t enable_inter_ist;
  uint8_t enable_chroma_dctonly;
  uint8_t enable_inter_ddt;
  uint8_t reduced_tx_part_set;
  uint8_t enable_cctx;
  uint8_t enable_mrls;
  uint8_t enable_cfl_intra;
  uint8_t enable_mhccp;
  uint8_t enable_tip;
  uint8_t enable_tip_refinemv;
  uint8_t enable_tip_hole_fill;
  uint8_t EnableTipOutput;
  uint8_t enable_tip_explicit_qp;
  uint8_t enable_mv_traj;
  uint8_t enable_bawp;
  uint8_t enable_cwp;
  uint8_t enable_imp_msk_bld;
  uint8_t enable_fsc;
  uint8_t enable_idtx_intra;
  uint8_t enable_ccso;
  uint8_t ccso_unit_matches_sb_size;
  uint8_t enable_df_sub_pu;
  AL_EAv2OpticalFilterRefine enable_opfl_refine;
  uint8_t enable_ibp;
  uint8_t enable_adaptive_mvd;
  uint8_t enable_refinemv;
  uint8_t enable_bru;
  uint8_t enable_mvd_sign_derive;
  uint8_t enable_flex_mvres;
  uint8_t cfl_ds_filter_index;
  uint8_t enable_tcq;
  uint8_t choose_tcq_per_frame;
  uint8_t enable_parity_hiding;
  uint8_t enable_global_motion;
  uint8_t df_par_bits_minus2;
  uint8_t enable_ext_seg;
  uint8_t seq_seg_info_present_flag;
  uint8_t seq_allow_seg_info_change;
  uint8_t enable_short_refresh_frame_flags;
  uint8_t disable_loopfilters_across_tiles;
  uint8_t seq_tile_info_present_flag;
  uint8_t allow_tile_info_change;
  int8_t BaseYDcDeltaQ;
  int8_t BaseUVDcDeltaQ;
  int8_t BaseUVAcDeltaQ;
  uint8_t OrderHintBits;
  AL_TAv2TileInfo tTileInfo;

  uint8_t MaxSegments;
  AL_TAv2SegmentationParams vSegmentParams[AL_AV2_MAX_SEGMENTS];

  uint8_t TLayerDependencyMap[AL_AV2_MAX_NUM_MLAYERS][AL_AV2_MAX_NUM_TLAYERS][AL_AV2_MAX_NUM_TLAYERS];
  uint8_t MLayerDependencyMap[AL_AV2_MAX_NUM_MLAYERS][AL_AV2_MAX_NUM_MLAYERS];
  uint8_t MLayerPresenceMap[AL_AV2_MAX_NUM_MLAYERS][AL_AV2_MAX_NUM_MLAYERS];

  uint8_t qm_data_present[AL_AV2_NUM_CUSTOM_QMS];
  AL_TAv2ScalingList vScalingList[AL_AV2_NUM_CUSTOM_QMS][3];

  uint32_t num_units_in_decoding_tick;
  uint32_t decoder_buffer_delay;
  uint32_t encoder_buffer_delay;
  bool low_delay_mode_flag;
}AL_TAv2SeqHeader;

/*****************************************************************************/
typedef struct AL_TAv2MultiFrameHeader
{
  uint8_t mfh_seq_header_id;
  uint8_t mfhId;
  uint8_t MfhTLayerId;
  uint8_t MfhMLayerId;
  uint8_t mfh_frame_size_present_flag;
  uint16_t mfh_frame_width_minus_1;
  uint16_t mfh_frame_height_minus_1;
  uint8_t mfh_deblocking_filter_update;
  uint8_t mfh_apply_deblocking_filter[4];
  uint8_t mfh_seg_info_present_flag;
  uint8_t mfh_ext_seg_flag;
  uint8_t mfh_allow_seg_info_change;
  AL_TAv2SegmentationParams mfh_seg_infos[AL_AV2_MAX_SEGMENTS];
}AL_TAv2MultiFrameHeader;

typedef int32_t AL_TAv2GmParams[AL_AV2_REFS_PER_FRAME][AL_AV2_NUM_GLOBAL_MOTION_PARAMS];

/*****************************************************************************/
typedef struct AL_TAv2FrameHeader
{
  AL_TAv2SeqHeader const* pSeqHeader;
  AL_TAv2MultiFrameHeader const* pMfHeader;

  uint8_t restricted_prediction_switch;
  AL_EAomFrameType FrameType;
  int8_t LongTermId;
  uint8_t num_key_ref_frames;
  uint8_t ref_long_term_id[AL_AV2_NUM_LONGTERM_REF_FRAMES];

  AL_TIndex film_grain_params_ref_idx;

  uint8_t immediate_output_frame;
  uint8_t implicit_output_frame;
  uint8_t disable_cdf_update;
  uint8_t allow_screen_content_tools;
  uint8_t force_integer_mv;
  uint8_t frame_size_override_flag;
  uint8_t signal_primary_ref_frame;
  uint8_t primary_ref_frame;
  uint8_t DerivedPrimaryRefFrame;
  uint8_t blendFrame;
  uint8_t disable_cross_frame_cdf_init;
  uint16_t frame_width_minus_1;
  uint16_t frame_height_minus_1;
  uint16_t refresh_frame_flags;
  AL_TIndex ref_frame_idx[AL_AV2_REFS_PER_FRAME];
  uint8_t refresh_frame_context;
  uint8_t allow_intrabc;
  uint8_t allow_global_intrabc;
  uint8_t allow_local_intrabc;
  uint8_t max_drl_bits_minus_1;
  uint8_t max_bvp_drl_bits_minus_1;
  AL_EAv2TipFrameMode TipFrameMode;
  uint8_t tip_global_wtd_index;
  uint8_t allow_tip_hole_fill;
  int8_t TipGlobalMvCol;
  int8_t TipGlobalMvRow;
  AL_EAv2InterpolationFilter TipInterpFilter;

  uint8_t disable_frame_end_update_cdf;
  uint8_t use_ref_frame_mvs;
  uint8_t ProjStep;
  AL_EAv2InterpolationFilter interpolation_filter;
  AL_EAv2OpticalFilterRefine opfl_refine_type;

  uint8_t increment_tile_cols_log2;
  uint8_t increment_tile_rows_log2;
  AL_TAv2TileInfo tTileInfo;

  uint16_t base_q_idx;
  int8_t y_dc_delta_q;
  int8_t u_dc_delta_q;
  int8_t u_ac_delta_q;
  int8_t v_dc_delta_q;
  int8_t v_ac_delta_q;
  uint8_t using_qmatrix;
  uint8_t pic_qm_num_minus1;
  uint8_t qm_index[16];
  uint8_t qm_y[4];
  uint8_t qm_u[4];
  uint8_t qm_v[4];
  uint8_t segmentation_enabled;
  uint8_t segmentation_update_map;
  uint8_t segmentation_temporal_update;
  uint8_t delta_q_present;
  uint8_t delta_q_res;
  int8_t LfDeltaQ[4];
  uint8_t CodedLossless;
  uint8_t LosslessArray[AL_AV2_MAX_SEGMENTS];
  uint8_t apply_deblocking_filter[4];
  uint8_t apply_deblocking_filter_tip;
  uint8_t gdf_frame_enable;
  uint8_t allow_df_sub_pu;
  AL_EAv2GdfMode gdf_mode;
  uint16_t gdf_block_num;
  uint8_t gdf_per_block;
  uint8_t gdf_pic_qc_idx;
  uint8_t GdfQpIndex;
  uint8_t gdf_pic_scale_idx;
  uint16_t GdfBlkSize;
  uint8_t cdef_frame_enable;
  uint8_t allow_parity_hiding;
  uint8_t tx_mode_select;
  AL_EAomTxMode TxMode;
  uint8_t reference_select;
  uint8_t skip_mode_present;
  uint8_t allow_bawp;
  uint8_t reduced_tx_set;
  uint8_t tcq_mode;
  uint8_t allow_tcq;
  uint8_t frame_enabled_motion_modes[AL_AV2_MOTION_MODES];
  uint8_t allow_warpmv_mode;

  uint8_t apply_grain;
  uint8_t fgm_id;
  uint16_t grain_seed;

  uint16_t context_update_tile_id;
  uint32_t tile_size_bytes_minus_1;
  uint8_t NumTotalRefs;
  uint16_t AllowedFrames;
  AL_EAv2FrameMvPrecision FrameMvPrecision;

  uint16_t OrderHintLsbs;
  int32_t OrderHint;
  uint8_t frame_to_show_map_idx;

  uint16_t tg_start;
  uint16_t tg_end;

  uint8_t ccso_frame_flag;
  uint8_t reuse_ccso[3];
  uint8_t sb_reuse_ccso[3];
  uint8_t ccso_ref_idx[3];

  uint8_t cdef_damping_minus_3;
  uint8_t cdef_strengths_minus1;
  uint8_t cdef_on_skip_txfm_frame_enable;

  uint8_t cdef_y_sec_strength[8];
  uint8_t cdef_y_pri_strength[8];
  uint8_t cdef_uv_pri_strength[8];
  uint8_t cdef_uv_sec_strength[8];

  AL_TIndex FurthestFuture;
  AL_TIndex ClosestFuture;
  AL_TIndex ClosestPast;
  AL_TIndex OrigClosestFuture;
  AL_TIndex OrigClosestPast;
  AL_TIndex SkipSegFrame;
  uint8_t NumFutureRefs;
  uint8_t NumPastRefs;

  AL_TAv2GmParams GmParams;
  EAv2MotionTransformationType GmType[AOM_NUM_REF_FRAMES];

  uint8_t LoopRestorationSizeShift[3];
  uint8_t uses_lr;
  uint8_t uses_luma_lr;
  uint8_t uses_chroma_lr;
  uint8_t frame_restoration_type[3];
  uint8_t frame_filters_on[3];
  uint8_t temporal_pred_flag[3];
  uint8_t num_filter_classes;
  uint8_t rst_ref_pic_idx;
  AL_TAv2LrWienerNsParams FrameLrWienerNs;
  AL_TAv2LrWienerNsParams RefLrWienerNs;

  uint8_t use_bru;
  uint8_t bru_ref;
  uint8_t bru_inactive;
  bool bru_tile_active[AOM_MAX_TILE_COLS][AOM_MAX_TILE_ROWS];

  uint8_t bridge_frame_ref_idx;
  uint8_t bridge_frame_overwrite_flag;
  bool process_sef_through_ip;
}AL_TAv2FrameHeader;

/*****************************************************************************/
typedef struct AL_TAv2FilmGrainModel
{
  uint16_t grain_seed;
  uint8_t num_y_points;
  uint8_t chroma_scaling_from_luma;
  uint8_t num_cb_points;
  uint8_t num_cr_points;
  uint8_t grain_scaling_minus_8;
  uint8_t ar_coeff_lag;
  int8_t ar_coeffs_y[24];
  int8_t ar_coeffs_cb[25];
  int8_t ar_coeffs_cr[25];
  uint8_t ar_coeff_shift_minus_6;
  uint8_t grain_scale_shift;
  uint8_t cb_mult;
  uint8_t cb_luma_mult;
  uint16_t cb_offset;
  uint8_t cr_mult;
  uint8_t cr_luma_mult;
  uint16_t cr_offset;
  uint8_t overlap_flag;
  uint8_t clip_to_restricted_range;
  uint8_t scaling_lut[3][256];
  uint8_t film_grain_block_size;
  uint8_t is_mc_identity;
}AL_TAv2FilmGrainModel;

/*****************************************************************************/
typedef struct AL_TAv2FilmGrainParams
{
  uint8_t FilmGrainPresent;
  uint8_t fgm_chroma_idc;
  uint8_t obu_tlayer_id;
  uint8_t obu_mlayer_id;
  AL_TAv2FilmGrainModel tModel;
}AL_TAv2FilmGrainParams;

/*****************************************************************************/
typedef struct AL_TAv2QuantizerMatrixParams
{
  uint8_t QmProtected;
  uint8_t QmNumPlanes;
  uint8_t QmDataPresent;
  uint8_t QmSeen;
  uint8_t obu_tlayer_id;
  uint8_t obu_mlayer_id;
  AL_TAv2ScalingList vScalingList[3];
}AL_TAv2QuantizerMatrixParams;

/*****************************************************************************/
typedef struct AL_TAv2LRFilterParams
{
  uint8_t uNumClasses;
  uint8_t uNumRefFilters;
  uint8_t uMatchIdx;
  uint8_t uMatchCls;
  uint8_t uMatchPlane;
}AL_TAv2LRFilterParams;
