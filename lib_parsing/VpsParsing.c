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
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include "lib_common/Utils.h"

#include "common_syntax.h"
#include "VpsParsing.h"

/*****************************************************************************/
void ParseVPS(AL_THevcVps pVPSTable[], AL_TRbspParser* pRP)
{
  int i, j, layer_offset;
  uint8_t vps_id;
  AL_THevcVps* pVPS;

  // Parse bitstream
  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 16); // Skip NUT + temporal_id

  vps_id = u(pRP, 4);
  pVPS = &pVPSTable[vps_id];
  pVPS->vps_video_parameter_set_id = vps_id;

  skip(pRP, 2); // vps_reserved_three_2bits
  pVPS->vps_max_layers_minus1 = u(pRP, 6);

  pVPS->vps_max_sub_layers_minus1 = u(pRP, 3);
  pVPS->vps_temporal_id_nesting_flag = u(pRP, 1);
  skip(pRP, 16); // vps_reserved_0xffff_16bits

  profile_tier_level(&pVPS->profile_and_level, pVPS->vps_max_sub_layers_minus1, pRP);
  pVPS->vps_sub_layer_ordering_info_present_flag = u(pRP, 1);

  layer_offset = pVPS->vps_sub_layer_ordering_info_present_flag ? 0 : pVPS->vps_max_sub_layers_minus1;

  for(i = layer_offset; i <= pVPS->vps_max_sub_layers_minus1; ++i)
  {
    pVPS->vps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    pVPS->vps_max_num_reorder_pics[i] = ue(pRP);
    pVPS->vps_max_latency_increase_plus1[i] = ue(pRP);
  }

  pVPS->vps_max_layer_id = u(pRP, 6);
  pVPS->vps_num_layer_sets_minus1 = ue(pRP);

  for(i = 1; i <= pVPS->vps_num_layer_sets_minus1; ++i)
  {
    uint16_t uOffset = Min(i, 1);

    for(j = 0; j <= pVPS->vps_max_layer_id; j++)
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

    for(i = 0; i < pVPS->vps_num_hrd_parameters; ++i)
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

/*@}*/

