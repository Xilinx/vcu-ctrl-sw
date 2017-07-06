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
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "common_syntax_elements.h"

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

  AL_TProfilevel profile_and_level;

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
}AL_THevcVps;

/****************************************************************************/

/*@}*/

