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

#include "SPS.h"
#include "PPS.h"

#define AL_AVC_MAX_SLICE_TYPE 9
#define AL_MAX_IDR_PIC_ID 65535
#define AL_MAX_REORDER_IDC 3
#define AL_MAX_CABAC_INIT_IDC 2

#define AL_MAX_WP_DENOM 7

#define AL_MIN_WP_LUMA_PARAM -128
#define AL_MAX_WP_LUMA_PARAM 127

#define AL_MIN_WP_CHROMA_DELTA_WEIGHT -128
#define AL_MAX_WP_CHROMA_DELTA_WEIGHT 127

#define AL_MAX_REFERENCE_PICTURE_REORDER 17

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.3
*****************************************************************************/
typedef struct t_AvcSliceHeader
{
  uint16_t num_line_in_slice;
  int first_mb_in_slice;
  uint8_t slice_type; // 0 = P, 1 = B, 2 = I.
  uint8_t pic_parameter_set_id;
  uint8_t field_pic_flag;
  int frame_num;
  int redundant_pic_cnt;
  int idr_pic_id;
  uint32_t pic_order_cnt_lsb;
  int delta_pic_order_cnt_bottom;
  int delta_pic_order_cnt[2];
  uint8_t bottom_field_flag;
  uint8_t nal_ref_idc;
  uint8_t direct_spatial_mv_pred_flag;
  uint8_t num_ref_idx_active_override_flag;
  int num_ref_idx_l0_active_minus1; // This member should always contain the current num_ref_idx_l0_active_minus1 applicable for this slice, even when num_ref_idx_active_override_flag == 1.
  int num_ref_idx_l1_active_minus1; // This member should always contain the current num_ref_idx_l1_active_minus1 applicable for this slice, even when num_ref_idx_active_override_flag == 1.

  // reference picture list reordering syntax elements
  uint8_t reordering_of_pic_nums_idc_l0[AL_MAX_REFERENCE_PICTURE_REORDER];
  uint8_t reordering_of_pic_nums_idc_l1[AL_MAX_REFERENCE_PICTURE_REORDER];
  int abs_diff_pic_num_minus1_l0[AL_MAX_REFERENCE_PICTURE_REORDER];
  int abs_diff_pic_num_minus1_l1[AL_MAX_REFERENCE_PICTURE_REORDER];
  int long_term_pic_num_l0[AL_MAX_REFERENCE_PICTURE_REORDER];
  int long_term_pic_num_l1[AL_MAX_REFERENCE_PICTURE_REORDER];
  uint8_t ref_pic_list_reordering_flag_l0;
  uint8_t ref_pic_list_reordering_flag_l1;

  // prediction weight table syntax elements
  AL_TWPTable pred_weight_table;

  // reference picture marking syntax elements
  uint8_t memory_management_control_operation[32];
  int difference_of_pic_nums_minus1[32];
  int long_term_pic_num[32];
  int long_term_frame_idx[32];
  int max_long_term_frame_idx_plus1[32];

  uint8_t no_output_of_prior_pics_flag;
  uint8_t long_term_reference_flag;
  uint8_t adaptive_ref_pic_marking_mode_flag;
  uint8_t cabac_init_idc;
  int slice_qp_delta;
  uint8_t disable_deblocking_filter_idc;
  uint8_t nal_unit_type;
  int8_t slice_alpha_c0_offset_div2;
  int8_t slice_beta_offset_div2;
  int slice_header_length;

  const AL_TAvcPps* pPPS;
  const AL_TAvcSps* pSPS;
}AL_TAvcSliceHdr;

typedef struct t_AvcHdrSvcExt // nal_unit_header_svc_extension
{
  uint8_t idr_flag;
  uint8_t priority_id;
  uint8_t no_inter_layer_pred_flag;
  uint8_t dependency_id;
  uint8_t quality_id;
  uint8_t temporal_id;
  uint8_t use_ref_base_pic_flag;
  uint8_t discardable_flag;
  uint8_t output_flag;
}AL_TAvcHdrSvcExt;

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.3
*****************************************************************************/
typedef struct t_HevcSliceHeader
{
  uint8_t nal_unit_type;
  uint8_t nuh_layer_id;
  uint8_t nuh_temporal_id_plus1;

  uint8_t first_slice_segment_in_pic_flag;
  uint8_t no_output_of_prior_pics_flag;
  uint8_t slice_pic_parameter_set_id;
  int slice_segment_address;

  uint8_t dependent_slice_segment_flag;
  uint8_t slice_type;
  uint8_t pic_output_flag;
  uint8_t colour_plane_id;

  uint32_t slice_pic_order_cnt_lsb;
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
  uint8_t NumPocTotalCurr;

  int slice_header_length;
  /* Keep this at last position of structure since it allows to memset
   * AL_THevcSliceHdr without clearing entry_point_offset_minus1.
   */
  uint32_t entry_point_offset_minus1[AL_MAX_ENTRY_POINT];
}AL_THevcSliceHdr;

/******************************************************************************/

/*@}*/

