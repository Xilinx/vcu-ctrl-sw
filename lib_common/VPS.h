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

#include "common_syntax_elements.h"

typedef struct t_RepFormat
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

/****************************************************************************/
#define AL_HEVC_MAX_VPS 16

/*************************************************************************//*!
   \brief Mimics structure described in spec sec. 7.3.2.1.
*****************************************************************************/
typedef struct t_HevcVps
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
  uint8_t layer_id_included_flag[2][64];

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
typedef union
{
  AL_THevcVps HevcVPS;
}AL_TVps;

/****************************************************************************/

/*@}*/

