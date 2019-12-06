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

#include "AvcParser.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
#include "lib_common/SeiInternal.h"
#include "lib_common_dec/RbspParser.h"
#include <string.h>

static void initPps(AL_TAvcPps* pPPS)
{
  Rtos_Memset(pPPS->UseDefaultScalingMatrix4x4Flag, 0, 6);
  Rtos_Memset(pPPS->UseDefaultScalingMatrix8x8Flag, 0, 2);
  pPPS->transform_8x8_mode_flag = 0;
  pPPS->pic_scaling_matrix_present_flag = 0;
  pPPS->bConceal = true;
}

AL_PARSE_RESULT AL_AVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId)
{
  uint16_t pps_id, QpBdOffset;
  AL_TAvcPps tempPPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 8); // Skip NUT

  pps_id = ue(pRP);

  COMPLY(pps_id < AL_AVC_MAX_PPS);

  if(pPpsId)
    *pPpsId = pps_id;

  initPps(&tempPPS);

  tempPPS.pic_parameter_set_id = pps_id;
  tempPPS.seq_parameter_set_id = ue(pRP);

  COMPLY(tempPPS.seq_parameter_set_id < AL_AVC_MAX_SPS);

  tempPPS.pSPS = &pIAup->avcAup.pSPS[tempPPS.seq_parameter_set_id];

  COMPLY(!tempPPS.pSPS->bConceal);

  tempPPS.entropy_coding_mode_flag = u(pRP, 1);
  tempPPS.bottom_field_pic_order_in_frame_present_flag = u(pRP, 1);
  tempPPS.num_slice_groups_minus1 = ue(pRP); // num_slice_groups_minus1

  if(tempPPS.num_slice_groups_minus1 > 0)
  {
    tempPPS.slice_group_map_type = ue(pRP);

    if(tempPPS.slice_group_map_type == 0)
    {
      for(int iGroup = 0; iGroup <= tempPPS.num_slice_groups_minus1; iGroup++)
        tempPPS.run_length_minus1[iGroup] = ue(pRP);
    }
    else if(tempPPS.slice_group_map_type == 2)
    {
      for(int iGroup = 0; iGroup <= tempPPS.num_slice_groups_minus1; iGroup++)
      {
        tempPPS.top_left[iGroup] = ue(pRP);
        tempPPS.bottom_right[iGroup] = ue(pRP);
      }
    }
    else if(tempPPS.slice_group_map_type == 3 ||
            tempPPS.slice_group_map_type == 4 ||
            tempPPS.slice_group_map_type == 5)
    {
      tempPPS.slice_group_change_direction_flag = u(pRP, 1);
      tempPPS.slice_group_change_rate_minus1 = ue(pRP);
    }
    else if(tempPPS.slice_group_map_type == 6)
    {
      tempPPS.pic_size_in_map_units_minus1 = ue(pRP);

      for(int i = 0; i <= tempPPS.pic_size_in_map_units_minus1; i++)
      {
        int slicegroupsize = tempPPS.pic_size_in_map_units_minus1 + 1;
        tempPPS.slice_group_id[i] = u(pRP, slicegroupsize);
      }
    }
  }

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  tempPPS.num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);
  tempPPS.num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);

  tempPPS.weighted_pred_flag = u(pRP, 1);
  tempPPS.weighted_bipred_idc = Clip3(u(pRP, 2), 0, AL_MAX_WP_IDC);

  QpBdOffset = 6 * tempPPS.pSPS->bit_depth_luma_minus8;
  tempPPS.pic_init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);
  tempPPS.pic_init_qs_minus26 = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);

  tempPPS.chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  tempPPS.second_chroma_qp_index_offset = tempPPS.chroma_qp_index_offset;

  tempPPS.deblocking_filter_control_present_flag = u(pRP, 1);
  tempPPS.constrained_intra_pred_flag = u(pRP, 1);
  skip(pRP, 1);
  tempPPS.redundant_pic_cnt_present_flag = 0;

  if(more_rbsp_data(pRP))
  {
    tempPPS.transform_8x8_mode_flag = u(pRP, 1);
    tempPPS.pic_scaling_matrix_present_flag = u(pRP, 1);

    if(tempPPS.pic_scaling_matrix_present_flag)
    {
      for(int i = 0; i < 6 + 2 * tempPPS.transform_8x8_mode_flag; i++)
      {
        if(i < 6)
          tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 0;
        else
          tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 0;

        tempPPS.pic_scaling_list_present_flag[i] = u(pRP, 1);

        if(tempPPS.pic_scaling_list_present_flag[i])
        {
          if(i < 6)
            avc_scaling_list_data(tempPPS.ScalingList4x4[i], pRP, 16, &tempPPS.UseDefaultScalingMatrix4x4Flag[i]);
          else
            avc_scaling_list_data(tempPPS.ScalingList8x8[i - 6], pRP, 64, &tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
        else
        {
          if(!tempPPS.pSPS->seq_scaling_matrix_present_flag)
          {
            if(i < 6)
            {
              if(i == 0 || i == 3)
                tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
              {
                if(tempPPS.UseDefaultScalingMatrix4x4Flag[i - 1])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.ScalingList4x4[i - 1], 16);
              }
            }
            else
              tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
          }
          else
          {
            if(i < 6)
            {
              if(i == 0 || i == 3)
              {
                if(tempPPS.pSPS->UseDefaultScalingMatrix4x4Flag[i])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.pSPS->ScalingList4x4[i], 16);
              }
              else
              {
                if(tempPPS.UseDefaultScalingMatrix4x4Flag[i - 1])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.ScalingList4x4[i - 1], 16);
              }
            }
            else
            {
              if(tempPPS.pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
                tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
              else
                Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.pSPS->ScalingList8x8[i - 6], 64);
            }
          }
        }
      }
    }
    else if(tempPPS.pSPS)
    {
      for(int i = 0; i < 8; ++i)
      {
        if(i < 6)
        {
          if(tempPPS.pSPS->UseDefaultScalingMatrix4x4Flag[i])
            tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
          else
            Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.pSPS->ScalingList4x4[i], 16);
        }
        else
        {
          if(tempPPS.pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
            tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
          else
            Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.pSPS->ScalingList8x8[i - 6], 64);
        }
      }
    }

    tempPPS.second_chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);
  }
  else
  {
    for(int i = 0; i < 8; ++i)
    {
      if(i < 6)
      {
        if(tempPPS.pSPS->UseDefaultScalingMatrix4x4Flag[i])
          tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
        else
          Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.pSPS->ScalingList4x4[i], 16);
      }
      else
      {
        if(tempPPS.pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
          tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        else
          Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.pSPS->ScalingList8x8[i - 6], 64);
      }
    }
  }

  /*dummy information to ensure there's no zero value in scaling list structure (div by zero prevention)*/
  if(!tempPPS.transform_8x8_mode_flag)
    tempPPS.UseDefaultScalingMatrix8x8Flag[0] =
      tempPPS.UseDefaultScalingMatrix8x8Flag[1] = 1;

  tempPPS.bConceal = rbsp_trailing_bits(pRP) ? false : true;

  COMPLY(tempPPS.num_slice_groups_minus1 == 0); // baseline profile only

  pIAup->avcAup.pPPS[pps_id] = tempPPS;

  return AL_OK;
}

static void initSps(AL_TAvcSps* pSPS)
{
  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 23;

  Rtos_Memset(pSPS->UseDefaultScalingMatrix4x4Flag, 0, 6);
  Rtos_Memset(pSPS->UseDefaultScalingMatrix8x8Flag, 0, 2);

  pSPS->mb_adaptive_frame_field_flag = 0;
  pSPS->chroma_format_idc = 1;
  pSPS->bit_depth_chroma_minus8 = 0;
  pSPS->bit_depth_luma_minus8 = 0;
  pSPS->seq_scaling_matrix_present_flag = 0;
  pSPS->qpprime_y_zero_transform_bypass_flag = 0;
  pSPS->vui_param.max_bytes_per_pic_denom = 2;
  pSPS->vui_param.max_bits_per_min_cu_denom = 1;
  pSPS->vui_param.log2_max_mv_length_horizontal = 16;
  pSPS->vui_param.log2_max_mv_length_vertical = 16;

  pSPS->bConceal = true;
}

static bool isProfileSupported(uint8_t profile_idc)
{
  switch(profile_idc)
  {
  case AVC_PROFILE_IDC_BASELINE:
  case AVC_PROFILE_IDC_MAIN:
  case AVC_PROFILE_IDC_HIGH:
    return true;

  case AVC_PROFILE_IDC_HIGH_422:
  case AVC_PROFILE_IDC_HIGH10:
    return HW_IP_BIT_DEPTH >= 10;

  default:
    return false;
  }
}

static int fieldToFrameHeight(int iFieldHeight)
{
  return ((iFieldHeight + 1) * 2) - 1;
}

AL_PARSE_RESULT AL_AVC_ParseSPS(AL_TRbspParser* pRP, AL_TAvcSps* pSPS)
{
  Rtos_Memset(pSPS, 0, sizeof(AL_TAvcSps));

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 8); // Skip NUT

  uint8_t profile_idc = u(pRP, 8);
  skip(pRP, 1); // constraint_set0_flag
  uint8_t constr_set1_flag = u(pRP, 1); // constraint_set1_flag

  if(!isProfileSupported(profile_idc) && !constr_set1_flag)
    return AL_UNSUPPORTED;
  skip(pRP, 1); // constraint_set2_flag
  uint8_t constr_set3_flag = u(pRP, 1);
  skip(pRP, 1); // constraint_set4_flag
  skip(pRP, 1); // constraint_set5_flag
  skip(pRP, 2); // reserved_zero_bits
  uint8_t level_idc = u(pRP, 8);
  uint8_t sps_id = ue(pRP);

  COMPLY(sps_id < AL_AVC_MAX_SPS);

  initSps(pSPS);

  pSPS->profile_idc = profile_idc;
  pSPS->constraint_set3_flag = constr_set3_flag;
  pSPS->level_idc = level_idc;
  pSPS->seq_parameter_set_id = sps_id;

  if(pSPS->profile_idc == 44 || pSPS->profile_idc == 83 ||
     pSPS->profile_idc == 86 || pSPS->profile_idc == 100 ||
     pSPS->profile_idc == 110 || pSPS->profile_idc == 118 ||
     pSPS->profile_idc == 122 || pSPS->profile_idc == 128 ||
     pSPS->profile_idc == 244)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));

    pSPS->chroma_format_idc = ue(pRP);

    if(pSPS->chroma_format_idc == 3)
      pSPS->separate_colour_plane_flag = u(pRP, 1);
    pSPS->bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);
    pSPS->bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);

    pSPS->qpprime_y_zero_transform_bypass_flag = u(pRP, 1);
    pSPS->seq_scaling_matrix_present_flag = u(pRP, 1);

    if(pSPS->seq_scaling_matrix_present_flag)
    {
      for(int i = 0; i < 8; ++i)
      {
        pSPS->seq_scaling_list_present_flag[i] = u(pRP, 1);

        if(pSPS->seq_scaling_list_present_flag[i])
        {
          if(i < 6)
            avc_scaling_list_data(pSPS->ScalingList4x4[i], pRP, 16, &pSPS->UseDefaultScalingMatrix4x4Flag[i]);
          else
            avc_scaling_list_data(pSPS->ScalingList8x8[i - 6], pRP, 64, &pSPS->UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
        else
        {
          if(i < 6)
          {
            if(i == 0 || i == 3)
              pSPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
            else
            {
              if(pSPS->UseDefaultScalingMatrix4x4Flag[i - 1])
                pSPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
                Rtos_Memcpy(pSPS->ScalingList4x4[i], pSPS->ScalingList4x4[i - 1], 16);
            }
          }
          else
            pSPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        }
      }
    }
    else
    {
      for(int i = 0; i < 8; ++i)
      {
        if(i < 6)
          Rtos_Memset(pSPS->ScalingList4x4[i], 16, 16);
        else
          Rtos_Memset(pSPS->ScalingList8x8[i - 6], 16, 64);
      }
    }
  }

  pSPS->log2_max_frame_num_minus4 = ue(pRP);

  COMPLY(pSPS->log2_max_frame_num_minus4 <= MAX_FRAME_NUM);

  pSPS->pic_order_cnt_type = ue(pRP);

  COMPLY(pSPS->pic_order_cnt_type <= MAX_POC_TYPE);

  if(pSPS->pic_order_cnt_type == 0)
  {
    pSPS->log2_max_pic_order_cnt_lsb_minus4 = ue(pRP);

    COMPLY(pSPS->log2_max_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB);

    pSPS->delta_pic_order_always_zero_flag = 1;
  }
  else if(pSPS->pic_order_cnt_type == 1)
  {
    pSPS->delta_pic_order_always_zero_flag = u(pRP, 1);
    pSPS->offset_for_non_ref_pic = se(pRP);
    pSPS->offset_for_top_to_bottom_field = se(pRP);
    pSPS->num_ref_frames_in_pic_order_cnt_cycle = ue(pRP);

    for(int i = 0; i < pSPS->num_ref_frames_in_pic_order_cnt_cycle; i++)
      pSPS->offset_for_ref_frame[i] = se(pRP);
  }

  pSPS->max_num_ref_frames = ue(pRP);
  pSPS->gaps_in_frame_num_value_allowed_flag = u(pRP, 1);

  pSPS->pic_width_in_mbs_minus1 = ue(pRP);
  pSPS->pic_height_in_map_units_minus1 = ue(pRP);

  COMPLY(pSPS->pic_width_in_mbs_minus1 >= 1);
  COMPLY(pSPS->pic_height_in_map_units_minus1 >= 1);

  pSPS->frame_mbs_only_flag = u(pRP, 1);

  if(!pSPS->frame_mbs_only_flag)
  {
    pSPS->mb_adaptive_frame_field_flag = u(pRP, 1);

    if(pSPS->mb_adaptive_frame_field_flag)
      return AL_UNSUPPORTED;

    pSPS->pic_height_in_map_units_minus1 = fieldToFrameHeight(pSPS->pic_height_in_map_units_minus1);
  }

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  pSPS->direct_8x8_inference_flag = u(pRP, 1);
  pSPS->frame_cropping_flag = u(pRP, 1);

  if(pSPS->frame_cropping_flag)
  {
    pSPS->frame_crop_left_offset = ue(pRP);
    pSPS->frame_crop_right_offset = ue(pRP);
    pSPS->frame_crop_top_offset = ue(pRP);
    pSPS->frame_crop_bottom_offset = ue(pRP);
  }

  pSPS->vui_parameters_present_flag = u(pRP, 1);

  if(pSPS->vui_parameters_present_flag)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    COMPLY(avc_vui_parameters(&pSPS->vui_param, pRP));
  }

  // validate current SPS
  pSPS->bConceal = false;

  return AL_OK;
}

static bool sei_buffering_period(AL_TRbspParser* pRP, AL_TAvcSps* pSpsTable, AL_TAvcBufPeriod* pBufPeriod, AL_TAvcSps** pActiveSps)
{
  AL_TAvcSps* pSPS = NULL;

  pBufPeriod->seq_parameter_set_id = ue(pRP);

  if(pBufPeriod->seq_parameter_set_id >= 32)
    return false;

  pSPS = &pSpsTable[pBufPeriod->seq_parameter_set_id];

  if(pSPS->bConceal)
    return false;

  *pActiveSps = pSPS;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(uint8_t i = 0; i <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      pBufPeriod->initial_cpb_removal_delay[i] = u(pRP, syntax_size);
      pBufPeriod->initial_cpb_removal_delay_offset[i] = u(pRP, syntax_size);
    }
  }

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(uint8_t i = 0; i <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      pBufPeriod->initial_cpb_removal_delay[i] = u(pRP, syntax_size);
      pBufPeriod->initial_cpb_removal_delay_offset[i] = u(pRP, syntax_size);
    }
  }

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool sei_pic_timing(AL_TRbspParser* pRP, AL_TAvcSps* pSPS, AL_TAvcPicTiming* pPicTiming)
{
  uint8_t time_offset_length = 0;

  if(pSPS == NULL || pSPS->bConceal)
    return false;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->cpb_removal_delay = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->dpb_output_delay = u(pRP, syntax_size);

    time_offset_length = pSPS->vui_param.hrd_param.time_offset_length;
  }
  else if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->cpb_removal_delay = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->dpb_output_delay = u(pRP, syntax_size);

    time_offset_length = pSPS->vui_param.hrd_param.time_offset_length;
  }

  if(pSPS->vui_param.pic_struct_present_flag)
  {
    const int NumClockTS[] =
    {
      1, 1, 1, 2, 2, 3, 3, 2, 3
    }; // Table D-1
    AL_TSeiClockTS* pClockTS;

    pPicTiming->pic_struct = u(pRP, 4);

    for(uint8_t iter = 0; iter < NumClockTS[pPicTiming->pic_struct]; ++iter)
    {
      pClockTS = &pPicTiming->clock_ts[iter];

      pClockTS->clock_time_stamp_flag = u(pRP, 1);

      if(pClockTS->clock_time_stamp_flag)
      {
        pClockTS->ct_type = u(pRP, 2);
        pClockTS->nuit_field_based_flag = u(pRP, 1);
        pClockTS->counting_type = u(pRP, 5);
        pClockTS->full_time_stamp_flag = u(pRP, 1);
        pClockTS->discontinuity_flag = u(pRP, 1);
        pClockTS->cnt_dropped_flag = u(pRP, 1);
        pClockTS->n_frames = u(pRP, 8);

        if(pClockTS->full_time_stamp_flag)
        {
          pClockTS->seconds_value = u(pRP, 6);
          pClockTS->minutes_value = u(pRP, 6);
          pClockTS->hours_value = u(pRP, 5);
        }
        else
        {
          pClockTS->seconds_flag = u(pRP, 1);

          if(pClockTS->seconds_flag)
          {
            pClockTS->seconds_value = u(pRP, 6);  /* range 0..59 */
            pClockTS->minutes_flag = u(pRP, 1);

            if(pClockTS->minutes_flag)
            {
              pClockTS->minutes_value = u(pRP, 6); /* 0..59 */

              pClockTS->hours_flag = u(pRP, 1);

              if(pClockTS->hours_flag)
                pClockTS->hours_value = u(pRP, 5); /* 0..23 */
            }
          }
        }
        pClockTS->time_offset = i(pRP, time_offset_length);
      } // clock_timestamp_flag
    }
  }

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool sei_recovery_point(AL_TRbspParser* pRP, AL_TRecoveryPoint* pRecoveryPoint)
{
  Rtos_Memset(pRecoveryPoint, 0, sizeof(*pRecoveryPoint));

  pRecoveryPoint->recovery_cnt = ue(pRP);
  pRecoveryPoint->exact_match = u(pRP, 1);
  pRecoveryPoint->broken_link = u(pRP, 1);

  /*changing_slice_group_idc = */ u(pRP, 2);

  return byte_alignment(pRP);
}

#define SEI_PTYPE_BUFFERING_PERIOD 0
#define SEI_PTYPE_USER_DATA_UNREGISTERED 5

#define PARSE_OR_SKIP(ParseCmd) \
  uint32_t uOffset = offset(pRP); \
  bool bRet = ParseCmd; \
  if(!bRet) \
  { \
    uOffset = offset(pRP) - uOffset; \
    if(uOffset > payload_size << 3) \
      return false; \
    skip(pRP, (payload_size << 3) - uOffset); \
    break; \
  }

/*****************************************************************************/
bool AL_AVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta)
{
  AL_TAvcSei sei;
  AL_TAvcAup* aup = &pIAup->avcAup;
  sei.present_flags = 0;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 8); // Skip NUT

  while(more_rbsp_data(pRP))
  {
    uint32_t payload_type = 0;
    uint32_t payload_size = 0;

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

    payload_size += byte;

    bool bCBAndSEIMeta = true; // Don't call CB or add in SEI metadata for HDR related SEIs, HDR settings are provided through dedicated metadata

    /* get payload data address, at this point we may not have the whole payload
     * data loaded in the rbsp parser */
    uint8_t* payload_data = get_raw_data(pRP);
    switch(payload_type)
    {
    case SEI_PTYPE_BUFFERING_PERIOD: // buffering_period parsing
    {
      PARSE_OR_SKIP(sei_buffering_period(pRP, aup->pSPS, &sei.buffering_period, &aup->pActiveSPS))
      sei.present_flags |= AL_SEI_BP;
      break;
    }
    case SEI_PTYPE_PIC_TIMING: // picture_timing parsing
    {
      PARSE_OR_SKIP(sei_pic_timing(pRP, aup->pActiveSPS, &sei.picture_timing))
      sei.present_flags |= AL_SEI_PT;
      break;
    }
    case SEI_PTYPE_USER_DATA_UNREGISTERED: // user_data_unregistered parsing
    {
      skip(pRP, payload_size << 3); // skip data
      break;
    }
    case SEI_PTYPE_RECOVERY_POINT: // picture_timing parsing
    {
      PARSE_OR_SKIP(sei_recovery_point(pRP, &sei.recovery_point));
      pIAup->iRecoveryCnt = sei.recovery_point.recovery_cnt + 1; // +1 for non-zero value when SEI_RP is present
      break;
    }
    case SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
    {
      PARSE_OR_SKIP(sei_mastering_display_colour_volume(&pIAup->tHDRSEIs.tMDCV, pRP));
      pIAup->tHDRSEIs.bHasMDCV = true;
      bCBAndSEIMeta = false;
      break;
    }
    case SEI_PTYPE_CONTENT_LIGHT_LEVEL:
    {
      PARSE_OR_SKIP(sei_content_light_level(&pIAup->tHDRSEIs.tCLL, pRP));
      pIAup->tHDRSEIs.bHasCLL = true;
      bCBAndSEIMeta = false;
      break;
    }
    default: // payload not supported
    {
      skip(pRP, payload_size << 3); // skip data
      break;
    }
    }

    if(bCBAndSEIMeta && pMeta)
    {
      if(!AL_SeiMetaData_AddPayload(pMeta, (AL_TSeiMessage) {bIsPrefix, payload_type, payload_data, payload_size }))
        return false;
    }

    if(bCBAndSEIMeta && cb->func)
      cb->func(bIsPrefix, payload_type, payload_data, payload_size, cb->userParam);
  }

  return true;
}

