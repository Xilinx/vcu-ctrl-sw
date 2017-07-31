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

#include <assert.h>
#include <string.h>

#include "lib_rtos/lib_rtos.h"

#include "lib_common/Utils.h"

#include "common_syntax.h"
#include "SpsParsing.h"

/****************************************************************************
                A V C   S P S   P A R S I N G   F U N C T I O N
****************************************************************************/

/*************************************************************************//*!
   \brief The InitParserSPS function intialize an SPS Parser structure
   \param[out] pSPS Pointer to the SPS structure that will be initialized
*****************************************************************************/
static void AL_AVC_sInitSPS(AL_TAvcSps* pSPS)
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

bool AL_AVC_IsSupportedProfile(uint8_t profile_idc)
{
  switch(profile_idc)
  {
  case 88:
  case 44:
  case 244:
  case 86:
    return false;
  default:
    break;
  }

  return true;
}

/*************************************************************************//*!
   \brief The ParseSPS function parse an SPS NAL
   \param[out] pSPSTable Pointer to the table holding the parsed SPS
   \param[in]  pRP  Pointer to NAL parser
*****************************************************************************/
AL_PARSE_RESULT AL_AVC_ParseSPS(AL_TAvcSps pSPSTable[], AL_TRbspParser* pRP)
{
  AL_TAvcSps tempSPS;

  memset(&tempSPS, 0, sizeof(AL_TAvcSps));

  // Parse bitstream
  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 8); // Skip NUT

  uint8_t profile_idc = u(pRP, 8);
  skip(pRP, 1); // constraint_set0_flag
  uint8_t constr_set1_flag = u(pRP, 1); // constraint_set1_flag

  if(!AL_AVC_IsSupportedProfile(profile_idc) && !constr_set1_flag)
    return AL_UNSUPPORTED;
  skip(pRP, 1); // constraint_set2_flag
  uint8_t constr_set3_flag = u(pRP, 1);
  skip(pRP, 1); // constraint_set4_flag
  skip(pRP, 1); // constraint_set5_flag
  skip(pRP, 2); // reserved_zero_bits
  uint8_t level_idc = u(pRP, 8);
  uint8_t sps_id = ue(pRP);

  COMPLY(sps_id < AL_AVC_MAX_SPS);

  AL_AVC_sInitSPS(&tempSPS);

  tempSPS.profile_idc = profile_idc;
  tempSPS.constraint_set3_flag = constr_set3_flag;
  tempSPS.level_idc = level_idc;
  tempSPS.seq_parameter_set_id = sps_id;

  if(tempSPS.profile_idc == 44 || tempSPS.profile_idc == 83 ||
     tempSPS.profile_idc == 86 || tempSPS.profile_idc == 100 ||
     tempSPS.profile_idc == 110 || tempSPS.profile_idc == 118 ||
     tempSPS.profile_idc == 122 || tempSPS.profile_idc == 128 ||
     tempSPS.profile_idc == 244)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));

    tempSPS.chroma_format_idc = ue(pRP);

    if(tempSPS.chroma_format_idc == 3)
      tempSPS.separate_colour_plane_flag = u(pRP, 1);
    tempSPS.bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);
    tempSPS.bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);

    tempSPS.qpprime_y_zero_transform_bypass_flag = u(pRP, 1);
    tempSPS.seq_scaling_matrix_present_flag = u(pRP, 1);

    if(tempSPS.seq_scaling_matrix_present_flag)
    {
      for(int i = 0; i < 8; ++i)
      {
        tempSPS.seq_scaling_list_present_flag[i] = u(pRP, 1);

        if(tempSPS.seq_scaling_list_present_flag[i])
        {
          if(i < 6)
            avc_scaling_list_data(tempSPS.ScalingList4x4[i], pRP, 16, &tempSPS.UseDefaultScalingMatrix4x4Flag[i]);
          else
            avc_scaling_list_data(tempSPS.ScalingList8x8[i - 6], pRP, 64, &tempSPS.UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
        else
        {
          if(i < 6)
          {
            if(i == 0 || i == 3)
              tempSPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
            else
            {
              if(tempSPS.UseDefaultScalingMatrix4x4Flag[i - 1])
                tempSPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
                Rtos_Memcpy(tempSPS.ScalingList4x4[i], tempSPS.ScalingList4x4[i - 1], 16);
            }
          }
          else
            tempSPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        }
      }
    }
    else
    {
      for(int i = 0; i < 8; ++i)
      {
        if(i < 6)
          Rtos_Memset(tempSPS.ScalingList4x4[i], 16, 16);
        else
          Rtos_Memset(tempSPS.ScalingList8x8[i - 6], 16, 64);
      }
    }
  }

  tempSPS.log2_max_frame_num_minus4 = ue(pRP);

  COMPLY(tempSPS.log2_max_frame_num_minus4 <= MAX_FRAME_NUM);

  tempSPS.pic_order_cnt_type = ue(pRP);

  COMPLY(tempSPS.pic_order_cnt_type <= MAX_POC_TYPE);

  if(tempSPS.pic_order_cnt_type == 0)
  {
    tempSPS.log2_max_pic_order_cnt_lsb_minus4 = ue(pRP);

    COMPLY(tempSPS.log2_max_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB);

    tempSPS.delta_pic_order_always_zero_flag = 1;
  }
  else if(tempSPS.pic_order_cnt_type == 1)
  {
    tempSPS.delta_pic_order_always_zero_flag = u(pRP, 1);
    tempSPS.offset_for_non_ref_pic = se(pRP);
    tempSPS.offset_for_top_to_bottom_field = se(pRP);
    tempSPS.num_ref_frames_in_pic_order_cnt_cycle = ue(pRP);

    for(int i = 0; i < tempSPS.num_ref_frames_in_pic_order_cnt_cycle; i++)
      tempSPS.offset_for_ref_frame[i] = se(pRP);
  }

  tempSPS.max_num_ref_frames = ue(pRP);
  tempSPS.gaps_in_frame_num_value_allowed_flag = u(pRP, 1);

  tempSPS.pic_width_in_mbs_minus1 = ue(pRP);
  tempSPS.pic_height_in_map_units_minus1 = ue(pRP);

  COMPLY(tempSPS.pic_width_in_mbs_minus1 >= 1);
  COMPLY(tempSPS.pic_height_in_map_units_minus1 >= 1);

  tempSPS.frame_mbs_only_flag = u(pRP, 1);

  if(!tempSPS.frame_mbs_only_flag)
  {
    tempSPS.mb_adaptive_frame_field_flag = u(pRP, 1);
  }

  if(!tempSPS.frame_mbs_only_flag)
    return AL_UNSUPPORTED;

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  tempSPS.direct_8x8_inference_flag = u(pRP, 1);
  tempSPS.frame_cropping_flag = u(pRP, 1);

  if(tempSPS.frame_cropping_flag)
  {
    tempSPS.frame_crop_left_offset = ue(pRP);
    tempSPS.frame_crop_right_offset = ue(pRP);
    tempSPS.frame_crop_top_offset = ue(pRP);
    tempSPS.frame_crop_bottom_offset = ue(pRP);
  }

  tempSPS.vui_parameters_present_flag = u(pRP, 1);

  if(tempSPS.vui_parameters_present_flag)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    COMPLY(avc_vui_parameters(&tempSPS.vui_param, pRP));
  }

  // validate current SPS
  tempSPS.bConceal = false;

  pSPSTable[sps_id] = tempSPS;
  return AL_OK;
}

/****************************************************************************
              H E V C   S P S   P A R S I N G   F U N C T I O N
****************************************************************************/

/*************************************************************************//*!
   \brief Computes the ref_pic_set variables
   \param[out] pSPS Pointer to the SPS structure containing the ref_pic_set structure and variables
   \param[in]  RefIdx Idx of the current ref_pic_set
*****************************************************************************/
static void AL_HEVC_sComputeRefPicSetVariables(AL_THevcSps* pSPS, uint8_t RefIdx)
{
  uint8_t num_negative = 0, num_positive = 0;
  int j;
  AL_TRefPicSet ref_pic_set = pSPS->short_term_ref_pic_set[RefIdx];

  uint8_t RIdx = RefIdx - (ref_pic_set.delta_idx_minus1 + 1);
  int32_t DeltaRPS = (1 - (ref_pic_set.delta_rps_sign << 1)) * (ref_pic_set.abs_delta_rps_minus1 + 1);

  if(ref_pic_set.inter_ref_pic_set_prediction_flag)
  {
    // num negative pics computation
    for(j = pSPS->NumPositivePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        pSPS->DeltaPocS0[RefIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    if(DeltaRPS < 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      pSPS->DeltaPocS0[RefIdx][num_negative] = DeltaRPS;
      pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(j = 0; j < pSPS->NumNegativePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[j])
      {
        pSPS->DeltaPocS0[RefIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RefIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    pSPS->NumNegativePics[RefIdx] = num_negative;

    // num positive pics computation
    for(j = pSPS->NumNegativePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[j])
      {
        pSPS->DeltaPocS1[RefIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    if(DeltaRPS > 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      pSPS->DeltaPocS1[RefIdx][num_positive] = DeltaRPS;
      pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(j = 0; j < pSPS->NumPositivePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        pSPS->DeltaPocS1[RefIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RefIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    pSPS->NumPositivePics[RefIdx] = num_positive;
  }
  else
  {
    pSPS->NumNegativePics[RefIdx] = ref_pic_set.num_negative_pics;
    pSPS->NumPositivePics[RefIdx] = ref_pic_set.num_positive_pics;

    pSPS->UsedByCurrPicS0[RefIdx][0] = ref_pic_set.used_by_curr_pic_s0_flag[0];
    pSPS->UsedByCurrPicS1[RefIdx][0] = ref_pic_set.used_by_curr_pic_s1_flag[0];

    pSPS->DeltaPocS0[RefIdx][0] = -(ref_pic_set.delta_poc_s0_minus1[0] + 1);
    pSPS->DeltaPocS1[RefIdx][0] = ref_pic_set.delta_poc_s1_minus1[0] + 1;

    for(j = 1; j < ref_pic_set.num_negative_pics; ++j)
    {
      pSPS->UsedByCurrPicS0[RefIdx][j] = ref_pic_set.used_by_curr_pic_s0_flag[j];
      pSPS->DeltaPocS0[RefIdx][j] = pSPS->DeltaPocS0[RefIdx][j - 1] - (ref_pic_set.delta_poc_s0_minus1[j] + 1);
    }

    for(j = 1; j < ref_pic_set.num_positive_pics; ++j)
    {
      pSPS->UsedByCurrPicS1[RefIdx][j] = ref_pic_set.used_by_curr_pic_s1_flag[j];
      pSPS->DeltaPocS1[RefIdx][j] = pSPS->DeltaPocS1[RefIdx][j - 1] + (ref_pic_set.delta_poc_s1_minus1[j] + 1);
    }
  }
  pSPS->NumDeltaPocs[RefIdx] = pSPS->NumNegativePics[RefIdx] + pSPS->NumPositivePics[RefIdx];
}

/*************************************************************************//*!
   \brief The InitSPS function intialize an SPS structure
   \param[out] pSPS Pointer to the SPS structure that will be initialized
*****************************************************************************/
static void AL_HEVC_sInitSPS(AL_THevcSps* pSPS)
{
  pSPS->chroma_format_idc = 1;
  pSPS->separate_colour_plane_flag = 0;
  pSPS->bit_depth_chroma_minus8 = 0;
  pSPS->bit_depth_luma_minus8 = 0;
  pSPS->scaling_list_enabled_flag = 0;
  pSPS->conf_win_left_offset = 0;
  pSPS->conf_win_right_offset = 0;
  pSPS->conf_win_top_offset = 0;
  pSPS->conf_win_bottom_offset = 0;
  pSPS->sps_scaling_list_data_present_flag = 0;
  pSPS->pcm_loop_filter_disabled_flag = 0;

  pSPS->sps_range_extensions_flag = 0;
  pSPS->sps_extension_7bits = 0;
  pSPS->transform_skip_rotation_enabled_flag = 0;
  pSPS->transform_skip_context_enabled_flag = 0;
  pSPS->implicit_rdpcm_enabled_flag = 0;
  pSPS->explicit_rdpcm_enabled_flag = 0;
  pSPS->extended_precision_processing_flag = 0;
  pSPS->intra_smoothing_disabled_flag = 0;
  pSPS->high_precision_offsets_enabled_flag = 0;
  pSPS->persistent_rice_adaptation_enabled_flag = 0;
  pSPS->cabac_bypass_alignment_enabled_flag = 0;

  pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 = 23;
  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 23;

  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 0;

  pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag = 0;
  Rtos_Memset(pSPS->sps_max_dec_pic_buffering_minus1, 0, sizeof(pSPS->sps_max_dec_pic_buffering_minus1));
  Rtos_Memset(pSPS->sps_num_reorder_pics, 0, sizeof(pSPS->sps_num_reorder_pics));
  Rtos_Memset(pSPS->sps_max_latency_increase_plus1, 0, sizeof(pSPS->sps_max_latency_increase_plus1));

  Rtos_Memset(pSPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, 20);
}

/*************************************************************************//*!
   \brief the short term reference picture computation
   \param[out] pSPS Pointer to the SPS structure containing the ref_pic_set structure and variables
   \param[in]  RefIdx Idx of the current ref_pic_set
   \param[in]  pRP  Pointer to NAL parser
*****************************************************************************/
void AL_HEVC_short_term_ref_pic_set(AL_THevcSps* pSPS, uint8_t RefIdx, AL_TRbspParser* pRP)
{
  uint8_t j, RIdx;
  AL_TRefPicSet* pRefPicSet = &pSPS->short_term_ref_pic_set[RefIdx];

  // default values
  pRefPicSet->delta_idx_minus1 = 0;
  pRefPicSet->inter_ref_pic_set_prediction_flag = 0;

  if(RefIdx)
    pRefPicSet->inter_ref_pic_set_prediction_flag = u(pRP, 1);

  if(pRefPicSet->inter_ref_pic_set_prediction_flag)
  {
    if(RefIdx == pSPS->num_short_term_ref_pic_sets)
      pRefPicSet->delta_idx_minus1 = ue(pRP);
    pRefPicSet->delta_rps_sign = u(pRP, 1);
    pRefPicSet->abs_delta_rps_minus1 = ue(pRP);

    RIdx = RefIdx - (pRefPicSet->delta_idx_minus1 + 1);

    for(j = 0; j <= pSPS->NumDeltaPocs[RIdx]; ++j)
    {
      pRefPicSet->use_delta_flag[j] = 1;
      pRefPicSet->used_by_curr_pic_flag[j] = u(pRP, 1);

      if(!pRefPicSet->used_by_curr_pic_flag[j])
        pRefPicSet->use_delta_flag[j] = u(pRP, 1);
    }
  }
  else
  {
    pRefPicSet->num_negative_pics = Clip3(ue(pRP), 0, pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1]);
    pRefPicSet->num_positive_pics = Clip3(ue(pRP), 0, pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1]);

    for(j = 0; j < pRefPicSet->num_negative_pics; ++j)
    {
      pRefPicSet->delta_poc_s0_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s0_flag[j] = u(pRP, 1);
    }

    for(j = 0; j < pRefPicSet->num_positive_pics; ++j)
    {
      pRefPicSet->delta_poc_s1_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s1_flag[j] = u(pRP, 1);
    }
  }
  AL_HEVC_sComputeRefPicSetVariables(pSPS, RefIdx);
}

/*************************************************************************//*!
   \brief The ParseSPS function parses a SPS NAL
   \param[out] pSPSTable Pointer to the table holding the parsed SPS
   \param[in]  pRP       Pointer to NAL parser
   \param[out] SpsId     ID of the SPS
*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_THevcSps pSPSTable[], AL_TRbspParser* pRP, uint8_t* SpsId)
{
  AL_THevcSps tempSPS;

  // Parse bitstream
  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 16); // Skip NUT + temporal_id

  int vps_id = u(pRP, 4);
  int max_sub_layers = Clip3(u(pRP, 3), 0, MAX_SUB_LAYER - 1);
  int temp_id_nesting_flag = u(pRP, 1);

  profile_tier_level(&tempSPS.profile_and_level, max_sub_layers, pRP);
  uint8_t sps_id = ue(pRP);
  *SpsId = sps_id;

  pSPSTable[sps_id].bConceal = true;

  COMPLY(sps_id < AL_HEVC_MAX_SPS);

  tempSPS.bConceal = true;
  tempSPS.sps_video_parameter_set_id = vps_id;
  tempSPS.sps_max_sub_layers_minus1 = max_sub_layers;
  tempSPS.sps_temporal_id_nesting_flag = temp_id_nesting_flag;
  tempSPS.sps_seq_parameter_set_id = sps_id;

  // default values
  AL_HEVC_sInitSPS(&tempSPS);

  tempSPS.chroma_format_idc = ue(pRP);

  if(tempSPS.chroma_format_idc == 3)
    tempSPS.separate_colour_plane_flag = u(pRP, 1);

  if(tempSPS.separate_colour_plane_flag)
    tempSPS.ChromaArrayType = 0;
  else
    tempSPS.ChromaArrayType = tempSPS.chroma_format_idc;

  tempSPS.pic_width_in_luma_samples = ue(pRP);
  tempSPS.pic_height_in_luma_samples = ue(pRP);

  tempSPS.conformance_window_flag = u(pRP, 1);

  if(tempSPS.conformance_window_flag)
  {
    tempSPS.conf_win_left_offset = ue(pRP);
    tempSPS.conf_win_right_offset = ue(pRP);
    tempSPS.conf_win_top_offset = ue(pRP);
    tempSPS.conf_win_bottom_offset = ue(pRP);

    if(tempSPS.conf_win_bottom_offset + tempSPS.conf_win_top_offset >= tempSPS.pic_height_in_luma_samples)
    {
      tempSPS.conf_win_top_offset = 0;
      tempSPS.conf_win_bottom_offset = 0;
    }

    if(tempSPS.conf_win_left_offset + tempSPS.conf_win_right_offset >= tempSPS.pic_width_in_luma_samples)
    {
      tempSPS.conf_win_left_offset = 0;
      tempSPS.conf_win_right_offset = 0;
    }
  }

  tempSPS.bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);
  tempSPS.bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH);

  tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 = ue(pRP);

  COMPLY(tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB);

  tempSPS.sps_sub_layer_ordering_info_present_flag = u(pRP, 1);
  int layer_offset = tempSPS.sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers;

  for(int i = layer_offset; i <= max_sub_layers; ++i)
  {
    tempSPS.sps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    tempSPS.sps_num_reorder_pics[i] = ue(pRP);
    tempSPS.sps_max_latency_increase_plus1[i] = ue(pRP);
  }

  tempSPS.log2_min_luma_coding_block_size_minus3 = ue(pRP);
  tempSPS.Log2MinCbSize = tempSPS.log2_min_luma_coding_block_size_minus3 + 3;

  COMPLY(tempSPS.Log2MinCbSize <= 6);

  tempSPS.log2_diff_max_min_luma_coding_block_size = ue(pRP);
  tempSPS.Log2CtbSize = tempSPS.Log2MinCbSize + tempSPS.log2_diff_max_min_luma_coding_block_size;

  COMPLY(tempSPS.Log2CtbSize <= 6);
  COMPLY(tempSPS.Log2CtbSize >= 4);

  tempSPS.log2_min_transform_block_size_minus2 = ue(pRP);
  int Log2MinTransfoSize = tempSPS.log2_min_transform_block_size_minus2 + 2;
  tempSPS.log2_diff_max_min_transform_block_size = ue(pRP);

  COMPLY(tempSPS.log2_min_transform_block_size_minus2 <= tempSPS.log2_min_luma_coding_block_size_minus3);
  COMPLY(tempSPS.log2_diff_max_min_transform_block_size <= Min(tempSPS.Log2CtbSize, 5) - Log2MinTransfoSize);

  tempSPS.max_transform_hierarchy_depth_inter = ue(pRP);
  tempSPS.max_transform_hierarchy_depth_intra = ue(pRP);

  COMPLY(tempSPS.max_transform_hierarchy_depth_inter <= (tempSPS.Log2CtbSize - Log2MinTransfoSize));
  COMPLY(tempSPS.max_transform_hierarchy_depth_intra <= (tempSPS.Log2CtbSize - Log2MinTransfoSize));

  tempSPS.scaling_list_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(tempSPS.scaling_list_enabled_flag)
  {
    tempSPS.sps_scaling_list_data_present_flag = u(pRP, 1);

    if(tempSPS.sps_scaling_list_data_present_flag)
      hevc_scaling_list_data(&tempSPS.scaling_list_param, pRP);
    else
      for(int i = 0; i < 20; ++i)
        tempSPS.scaling_list_param.UseDefaultScalingMatrixFlag[i] = 1;
  }

  tempSPS.amp_enabled_flag = u(pRP, 1);
  tempSPS.sample_adaptive_offset_enabled_flag = u(pRP, 1);

  tempSPS.pcm_enabled_flag = u(pRP, 1);

  if(tempSPS.pcm_enabled_flag)
  {
    tempSPS.pcm_sample_bit_depth_luma_minus1 = u(pRP, 4);
    tempSPS.pcm_sample_bit_depth_chroma_minus1 = u(pRP, 4);

    COMPLY(tempSPS.pcm_sample_bit_depth_luma_minus1 <= tempSPS.bit_depth_luma_minus8 + 7);
    COMPLY(tempSPS.pcm_sample_bit_depth_chroma_minus1 <= tempSPS.bit_depth_chroma_minus8 + 7);

    tempSPS.log2_min_pcm_luma_coding_block_size_minus3 = Clip3(ue(pRP), Min(tempSPS.log2_min_luma_coding_block_size_minus3, 2), Min(tempSPS.Log2CtbSize - 3, 2));

    COMPLY(tempSPS.log2_min_pcm_luma_coding_block_size_minus3 >= Min(tempSPS.log2_min_luma_coding_block_size_minus3, 2));
    COMPLY(tempSPS.log2_min_pcm_luma_coding_block_size_minus3 <= Min(tempSPS.Log2CtbSize - 3, 2));

    tempSPS.log2_diff_max_min_pcm_luma_coding_block_size = Clip3(ue(pRP), 0, Min(tempSPS.Log2CtbSize - 3, 2) - tempSPS.log2_min_pcm_luma_coding_block_size_minus3);

    COMPLY(tempSPS.log2_diff_max_min_pcm_luma_coding_block_size <= (Min(tempSPS.Log2CtbSize - 3, 2) - tempSPS.log2_min_pcm_luma_coding_block_size_minus3));

    tempSPS.pcm_loop_filter_disabled_flag = u(pRP, 1);
  }

  tempSPS.num_short_term_ref_pic_sets = ue(pRP);

  COMPLY(tempSPS.num_short_term_ref_pic_sets <= MAX_REF_PIC_SET);

  for(int i = 0; i < tempSPS.num_short_term_ref_pic_sets; ++i)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    AL_HEVC_short_term_ref_pic_set(&tempSPS, i, pRP);
  }

  tempSPS.long_term_ref_pics_present_flag = u(pRP, 1);

  if(tempSPS.long_term_ref_pics_present_flag)
  {
    uint8_t syntax_size = tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
    tempSPS.num_long_term_ref_pics_sps = ue(pRP);

    COMPLY(tempSPS.num_long_term_ref_pics_sps <= MAX_LONG_TERM_PIC);

    for(int i = 0; i < tempSPS.num_long_term_ref_pics_sps; ++i)
    {
      tempSPS.lt_ref_pic_poc_lsb_sps[i] = u(pRP, syntax_size);
      tempSPS.used_by_curr_pic_lt_sps_flag[i] = u(pRP, 1);
    }
  }
  tempSPS.sps_temporal_mvp_enabled_flag = u(pRP, 1);
  tempSPS.strong_intra_smoothing_enabled_flag = u(pRP, 1);

  tempSPS.vui_parameters_present_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(tempSPS.vui_parameters_present_flag)
    hevc_vui_parameters(&tempSPS.vui_param, tempSPS.sps_max_sub_layers_minus1, pRP);

  tempSPS.sps_extension_present_flag = u(pRP, 1);

  if(tempSPS.sps_extension_present_flag)
  {
    tempSPS.sps_range_extensions_flag = u(pRP, 1);
    tempSPS.sps_extension_7bits = u(pRP, 7);
  }

  if(tempSPS.sps_range_extensions_flag)
  {
    tempSPS.transform_skip_rotation_enabled_flag = u(pRP, 1);
    tempSPS.transform_skip_context_enabled_flag = u(pRP, 1);
    tempSPS.implicit_rdpcm_enabled_flag = u(pRP, 1);
    tempSPS.explicit_rdpcm_enabled_flag = u(pRP, 1);
    tempSPS.extended_precision_processing_flag = u(pRP, 1);
    tempSPS.intra_smoothing_disabled_flag = u(pRP, 1);
    tempSPS.high_precision_offsets_enabled_flag = u(pRP, 1);
    tempSPS.persistent_rice_adaptation_enabled_flag = u(pRP, 1);
    tempSPS.cabac_bypass_alignment_enabled_flag = u(pRP, 1);
  }

  if(tempSPS.sps_extension_7bits) // sps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // sps_extension_data_flag
  }

  // Compute variables
  tempSPS.PicWidthInCtbs = (tempSPS.pic_width_in_luma_samples + ((1 << tempSPS.Log2CtbSize) - 1)) >> tempSPS.Log2CtbSize;
  tempSPS.PicHeightInCtbs = (tempSPS.pic_height_in_luma_samples + ((1 << tempSPS.Log2CtbSize) - 1)) >> tempSPS.Log2CtbSize;

  COMPLY(tempSPS.PicWidthInCtbs > 1);
  COMPLY(tempSPS.PicHeightInCtbs > 1);

  tempSPS.PicWidthInMinCbs = tempSPS.pic_width_in_luma_samples >> tempSPS.Log2MinCbSize;
  tempSPS.PicHeightInMinCbs = tempSPS.pic_height_in_luma_samples >> tempSPS.Log2MinCbSize;

  tempSPS.SpsMaxLatency = tempSPS.sps_max_latency_increase_plus1[max_sub_layers] ? tempSPS.sps_num_reorder_pics[max_sub_layers] + tempSPS.sps_max_latency_increase_plus1[max_sub_layers] - 1 : UINT32_MAX;

  tempSPS.MaxPicOrderCntLsb = 1 << (tempSPS.log2_max_slice_pic_order_cnt_lsb_minus4 + 4);

  tempSPS.WpOffsetBdShiftY = tempSPS.high_precision_offsets_enabled_flag ? 0 : tempSPS.bit_depth_luma_minus8;
  tempSPS.WpOffsetBdShiftC = tempSPS.high_precision_offsets_enabled_flag ? 0 : tempSPS.bit_depth_chroma_minus8;
  tempSPS.WpOffsetHalfRangeY = 1 << (tempSPS.high_precision_offsets_enabled_flag ? tempSPS.bit_depth_luma_minus8 + 7 : 7);
  tempSPS.WpOffsetHalfRangeC = 1 << (tempSPS.high_precision_offsets_enabled_flag ? tempSPS.bit_depth_chroma_minus8 + 7 : 7);

  COMPLY(rbsp_trailing_bits(pRP));

  tempSPS.bConceal = false;
  pSPSTable[sps_id] = tempSPS;

  return AL_OK;
}

void AL_AVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS)
{
  pCfg->iLevelIdc = pSPS->level_idc;
  pCfg->iPicWidth = pSPS->pic_width_in_mbs_minus1 + 1;
  pCfg->iPicHeight = pSPS->pic_height_in_map_units_minus1 + 1;
  pCfg->eChromaMode = pSPS->chroma_format_idc;
  pCfg->iMaxBitDepth = 8 + Max(pSPS->bit_depth_luma_minus8, pSPS->bit_depth_chroma_minus8);

  pCfg->bInit = true;
}

bool AL_AVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS)
{
  if(!pCfg->bInit)
    return true;

  if(pSPS->level_idc > pCfg->iLevelIdc)
    return false;

  if((pSPS->pic_width_in_mbs_minus1 + 1) != pCfg->iPicWidth
     || (pSPS->pic_height_in_map_units_minus1 + 1) != pCfg->iPicHeight)
    return false;

  if(pSPS->chroma_format_idc != pCfg->eChromaMode)
    return false;

  if((pSPS->bit_depth_luma_minus8 + 8) > pCfg->iMaxBitDepth
     || (pSPS->bit_depth_chroma_minus8 + 8) > pCfg->iMaxBitDepth)
    return false;

  return true;
}

void AL_HEVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS)
{
  pCfg->iLevelIdc = pSPS->profile_and_level.general_level_idc;
  pCfg->iPicWidth = pSPS->pic_width_in_luma_samples;
  pCfg->iPicHeight = pSPS->pic_height_in_luma_samples;
  pCfg->eChromaMode = pSPS->chroma_format_idc;
  pCfg->iMaxBitDepth = 8 + Max(pSPS->bit_depth_luma_minus8, pSPS->bit_depth_chroma_minus8);

  pCfg->bInit = true;
}

bool AL_HEVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS)
{
  if(!pCfg->bInit)
    return true;

  if(pSPS->profile_and_level.general_level_idc > pCfg->iLevelIdc)
    return false;

  if(pSPS->pic_width_in_luma_samples != pCfg->iPicWidth
     || pSPS->pic_height_in_luma_samples != pCfg->iPicHeight)
    return false;

  if(pSPS->chroma_format_idc != pCfg->eChromaMode)
    return false;

  if((pSPS->bit_depth_luma_minus8 + 8) > pCfg->iMaxBitDepth
     || (pSPS->bit_depth_chroma_minus8 + 8) > pCfg->iMaxBitDepth)
    return false;

  return true;
}

/*@}*/

