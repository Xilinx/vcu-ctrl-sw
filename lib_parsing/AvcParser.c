// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "AvcParser.h"

#include "lib_common/Arithmetics.h"
#include "lib_common/Utils.h"
#include "lib_common/SeiInternal.h"
#include "lib_common_dec/ParseResult.h"
#include "lib_common_dec/RbspParser.h"
#include "SeiParser.h"

/*****************************************************************************/
static void initPps(AL_TAvcPps* pPPS)
{
  Rtos_Memset(pPPS->UseDefaultScalingMatrix4x4Flag, 0, sizeof(pPPS->UseDefaultScalingMatrix4x4Flag));
  Rtos_Memset(pPPS->UseDefaultScalingMatrix8x8Flag, 0, sizeof(pPPS->UseDefaultScalingMatrix8x8Flag));
  pPPS->transform_8x8_mode_flag = 0;
  pPPS->pic_scaling_matrix_present_flag = 0;
  pPPS->bConceal = true;
}

/*****************************************************************************/
static void propagate_scaling_list(int32_t iSclID, uint8_t* Default4x4Flag, uint8_t* Default8x8Flag, uint8_t ScalingList4x4[][16], uint8_t ScalingList8x8[][64])
{
  if(iSclID < 6)
  {
    if(iSclID == 0 || iSclID == 3)
      Default4x4Flag[iSclID] = 1;
    else
    {
      if(Default4x4Flag[iSclID - 1])
        Default4x4Flag[iSclID] = 1;
      else
        Rtos_Memcpy(ScalingList4x4[iSclID], ScalingList4x4[iSclID - 1], 16);
    }
  }
  else
  {
    if(iSclID < 8)
      Default8x8Flag[iSclID - 6] = 1;
    else
    {
      if(Default8x8Flag[iSclID - 8])
        Default8x8Flag[iSclID - 6] = 1;
      else
        Rtos_Memcpy(ScalingList8x8[iSclID - 6], ScalingList8x8[iSclID - 8], 64);
    }
  }
}

/*****************************************************************************/
void AL_AVC_ComputeSclParameters(AL_TAvcPps* pPPS)
{
  if(pPPS->pic_scaling_matrix_present_flag)
  {
    for(int32_t i = 0; i < 6 + (pPPS->pSPS->chroma_format_idc != 3 ? 2 : 6) * pPPS->transform_8x8_mode_flag; i++)
    {
      if(!pPPS->pic_scaling_list_present_flag[i])
      {
        if(i < 6)
          pPPS->UseDefaultScalingMatrix4x4Flag[i] = 0;
        else
          pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 0;

        if(!pPPS->pSPS->seq_scaling_matrix_present_flag)
          propagate_scaling_list(i, pPPS->UseDefaultScalingMatrix4x4Flag, pPPS->UseDefaultScalingMatrix8x8Flag, pPPS->ScalingList4x4, pPPS->ScalingList8x8);
        else
        {
          if(i < 6)
          {
            if(i == 0 || i == 3)
            {
              if(pPPS->pSPS->UseDefaultScalingMatrix4x4Flag[i])
                pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
                Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->pSPS->ScalingList4x4[i], 16);
            }
            else
            {
              if(pPPS->UseDefaultScalingMatrix4x4Flag[i - 1])
                pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
                Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->ScalingList4x4[i - 1], 16);
            }
          }
          else
          {
            if(i < 8)
            {
              if(pPPS->pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
                pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
              else
                Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->pSPS->ScalingList8x8[i - 6], 64);
            }
            else
            {
              if(pPPS->UseDefaultScalingMatrix8x8Flag[i - 8])
                pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
              else
                Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->ScalingList8x8[i - 8], 64);
            }
          }
        }
      }
    }
  }
  else if(pPPS->pSPS)
  {
    for(int32_t i = 0; i < (pPPS->pSPS->chroma_format_idc != 3 ? 8 : 12); ++i)
    {
      if(i < 6)
      {
        if(pPPS->pSPS->UseDefaultScalingMatrix4x4Flag[i])
          pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
        else
          Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->pSPS->ScalingList4x4[i], 16);
      }
      else
      {
        if(pPPS->pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
          pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        else
          Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->pSPS->ScalingList8x8[i - 6], 64);
      }
    }
  }
}

/*****************************************************************************/
static void avc_scaling_list_data(uint8_t* pScalingList, AL_TRbspParser* pRP, int32_t iSize, uint8_t* pUseDefaultScalingMatrixFlag)
{
  // spec. 8.5.4
  static const uint8_t pDecScanBlock4x4[16] =
  {
    0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
  };
  static const uint8_t pDecScanBlock8x8[64] =
  {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
  };

  const uint8_t* pDecScanBlock = (iSize == 16) ? pDecScanBlock4x4 : pDecScanBlock8x8;
  int32_t iLastScale = 8;
  int32_t iNextScale = 8;

  for(int32_t j = 0; j < iSize; j++)
  {
    uint8_t i = pDecScanBlock[j];

    if(iNextScale != 0)
    {
      int32_t iDeltaScale = Clip3(se(pRP), -128, 127);
      iNextScale = (iLastScale + iDeltaScale + 256) % 256;
      *pUseDefaultScalingMatrixFlag = (j == 0 && iNextScale == 0);
    }

    pScalingList[i] = iNextScale == 0 ? iLastScale : iNextScale;
    iLastScale = pScalingList[i];
  }
}

/*****************************************************************************/
AL_EParseResult AL_AVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId)
{
  skipAllZerosAndTheNextByte(pRP);
  u(pRP, 8); // Skip NUT

  uint16_t pps_id = ue(pRP);

  if(pPpsId)
    *pPpsId = pps_id;

  COMPLY(pps_id < AL_AVC_MAX_PPS);

  AL_TAvcAup* aup = &pIAup->avcAup;
  AL_TAvcPps* pPPS = &aup->pPPS[pps_id];

  // default values
  initPps(pPPS);

  pPPS->pic_parameter_set_id = pps_id;
  pPPS->seq_parameter_set_id = ue(pRP);

  COMPLY(pPPS->seq_parameter_set_id < AL_AVC_MAX_SPS);

  pPPS->pSPS = &pIAup->avcAup.pSPS[pPPS->seq_parameter_set_id];

  COMPLY(!pPPS->pSPS->bConceal);

  pPPS->entropy_coding_mode_flag = u(pRP, 1);
  pPPS->bottom_field_pic_order_in_frame_present_flag = u(pRP, 1);
  pPPS->num_slice_groups_minus1 = Clip3(ue(pRP), 0, 7);

  COMPLY(pPPS->num_slice_groups_minus1 == 0); // should always be 0 in supported profiles (see comment right below)

  pPPS->num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);
  pPPS->num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);

  pPPS->weighted_pred_flag = u(pRP, 1);
  pPPS->weighted_bipred_idc = Clip3(u(pRP, 2), 0, AL_AVC_MAX_WP_IDC);

  uint16_t QpBdOffset = 6 * pPPS->pSPS->bit_depth_luma_minus8;
  pPPS->pic_init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);
  pPPS->pic_init_qs_minus26 = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);

  pPPS->chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->second_chroma_qp_index_offset = pPPS->chroma_qp_index_offset;

  pPPS->deblocking_filter_control_present_flag = u(pRP, 1);
  pPPS->constrained_intra_pred_flag = u(pRP, 1);
  pPPS->redundant_pic_cnt_present_flag = u(pRP, 1);

  if(more_rbsp_data(pRP))
  {
    pPPS->transform_8x8_mode_flag = u(pRP, 1);
    pPPS->pic_scaling_matrix_present_flag = u(pRP, 1);

    if(pPPS->pic_scaling_matrix_present_flag)
    {
      for(int32_t i = 0; i < 6 + (pPPS->pSPS->chroma_format_idc != 3 ? 2 : 6) * pPPS->transform_8x8_mode_flag; i++)
      {
        if(i < 6)
          pPPS->UseDefaultScalingMatrix4x4Flag[i] = 0;
        else
          pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 0;

        pPPS->pic_scaling_list_present_flag[i] = u(pRP, 1);

        if(pPPS->pic_scaling_list_present_flag[i])
        {
          if(i < 6)
            avc_scaling_list_data(pPPS->ScalingList4x4[i], pRP, 16, &pPPS->UseDefaultScalingMatrix4x4Flag[i]);
          else
            avc_scaling_list_data(pPPS->ScalingList8x8[i - 6], pRP, 64, &pPPS->UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
        else
        {
          if(!pPPS->pSPS->seq_scaling_matrix_present_flag)
            propagate_scaling_list(i, pPPS->UseDefaultScalingMatrix4x4Flag, pPPS->UseDefaultScalingMatrix8x8Flag, pPPS->ScalingList4x4, pPPS->ScalingList8x8);
          else
          {
            if(i < 6)
            {
              if(i == 0 || i == 3)
              {
                if(pPPS->pSPS->UseDefaultScalingMatrix4x4Flag[i])
                  pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->pSPS->ScalingList4x4[i], 16);
              }
              else
              {
                if(pPPS->UseDefaultScalingMatrix4x4Flag[i - 1])
                  pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->ScalingList4x4[i - 1], 16);
              }
            }
            else
            {
              if(i < 8)
              {
                if(pPPS->pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
                  pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
                else
                  Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->pSPS->ScalingList8x8[i - 6], 64);
              }
              else
              {
                if(pPPS->UseDefaultScalingMatrix8x8Flag[i - 8])
                  pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
                else
                  Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->ScalingList8x8[i - 8], 64);
              }
            }
          }
        }
      }
    }
    else if(pPPS->pSPS)
    {
      for(int32_t i = 0; i < (pPPS->pSPS->chroma_format_idc != 3 ? 8 : 12); ++i)
      {
        if(i < 6)
        {
          if(pPPS->pSPS->UseDefaultScalingMatrix4x4Flag[i])
            pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
          else
            Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->pSPS->ScalingList4x4[i], 16);
        }
        else
        {
          if(pPPS->pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
            pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
          else
            Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->pSPS->ScalingList8x8[i - 6], 64);
        }
      }
    }

    pPPS->second_chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);
  }
  else
  {
    for(int32_t i = 0; i < (pPPS->pSPS->chroma_format_idc != 3 ? 8 : 12); ++i)
    {
      if(i < 6)
      {
        if(pPPS->pSPS->UseDefaultScalingMatrix4x4Flag[i])
          pPPS->UseDefaultScalingMatrix4x4Flag[i] = 1;
        else
          Rtos_Memcpy(pPPS->ScalingList4x4[i], pPPS->pSPS->ScalingList4x4[i], 16);
      }
      else
      {
        if(pPPS->pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
          pPPS->UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        else
          Rtos_Memcpy(pPPS->ScalingList8x8[i - 6], pPPS->pSPS->ScalingList8x8[i - 6], 64);
      }
    }
  }

  /*dummy information to ensure there's no zero value in scaling list structure (div by zero prevention)*/
  if(!pPPS->transform_8x8_mode_flag)
  {
    for(int32_t i = 0; i < 6; ++i)
      pPPS->UseDefaultScalingMatrix8x8Flag[i] = 1;
  }

  COMPLY(rbsp_trailing_bits(pRP));

  return AL_PARSE_OK;
}

/*****************************************************************************/
static void initSps(AL_TAvcSps* pSPS)
{
  Rtos_Memset(pSPS, 0, sizeof(AL_TAvcSps));

  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 23;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 23;

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

/*****************************************************************************/
static bool isProfileSupported(uint8_t profile_idc)
{
  switch(profile_idc)
  {
  case AVC_PROFILE_IDC_BASELINE: // We do not support FMO / ASO, however, we try to decode with concealment
    return true;

  case AVC_PROFILE_IDC_MAIN:
  case AVC_PROFILE_IDC_HIGH:
    return true;

  case AVC_PROFILE_IDC_HIGH_422:
  case AVC_PROFILE_IDC_HIGH10:
    return true;

  default:
    return false;
  }
}

/*****************************************************************************/
static bool avc_hrd_parameters(AL_TRbspParser* pRP, AL_THrdParam* pHrdParam, AL_TSubHrdParam* pSubHrdParam)
{
  for(int32_t i = 0; i < 32; ++i)
  {
    pSubHrdParam->bit_rate_value_minus1[i] =
      pSubHrdParam->cpb_size_value_minus1[i] =
        pSubHrdParam->cbr_flag[i] = 0;
  }

  pHrdParam->cpb_cnt_minus1[0] = ue(pRP);

  if(pHrdParam->cpb_cnt_minus1[0] > 31)
    return false;

  pHrdParam->bit_rate_scale = u(pRP, 4);
  pHrdParam->cpb_size_scale = u(pRP, 4);

  for(uint8_t i = 0; i <= pHrdParam->cpb_cnt_minus1[0]; i++)
  {
    pSubHrdParam->bit_rate_value_minus1[i] = ue(pRP);
    pSubHrdParam->cpb_size_value_minus1[i] = ue(pRP);
    pSubHrdParam->cbr_flag[i] = u(pRP, 1);
  }

  pHrdParam->initial_cpb_removal_delay_length_minus1 = u(pRP, 5);
  pHrdParam->au_cpb_removal_delay_length_minus1 = u(pRP, 5);
  pHrdParam->dpb_output_delay_length_minus1 = u(pRP, 5);
  pHrdParam->time_offset_length = u(pRP, 5);

  return true;
}

/*****************************************************************************/
static bool avc_vui_parameters(AL_TVuiParam* pVuiParam, AL_TRbspParser* pRP)
{
  pVuiParam->aspect_ratio_info_present_flag = u(pRP, 1);

  if(pVuiParam->aspect_ratio_info_present_flag)
  {
    pVuiParam->aspect_ratio_idc = u(pRP, 8);

    if(pVuiParam->aspect_ratio_idc == 255)
    {
      pVuiParam->sar_width = u(pRP, 16);
      pVuiParam->sar_height = u(pRP, 16);
    }
  }

  pVuiParam->overscan_info_present_flag = u(pRP, 1);

  if(pVuiParam->overscan_info_present_flag)
  {
    pVuiParam->overscan_appropriate_flag = u(pRP, 1);
  }

  pVuiParam->video_signal_type_present_flag = u(pRP, 1);

  if(pVuiParam->video_signal_type_present_flag)
  {
    pVuiParam->video_format = u(pRP, 3);
    pVuiParam->video_full_range_flag = u(pRP, 1);
    pVuiParam->colour_description_present_flag = u(pRP, 1);

    if(pVuiParam->colour_description_present_flag)
    {
      pVuiParam->colour_primaries = u(pRP, 8);
      pVuiParam->transfer_characteristics = u(pRP, 8);
      pVuiParam->matrix_coefficients = u(pRP, 8);
    }
  }

  pVuiParam->chroma_loc_info_present_flag = u(pRP, 1);

  if(pVuiParam->chroma_loc_info_present_flag)
  {
    pVuiParam->chroma_sample_loc_type_top_field = ue(pRP);
    pVuiParam->chroma_sample_loc_type_bottom_field = ue(pRP);
  }

  pVuiParam->vui_timing_info_present_flag = u(pRP, 1);

  if(pVuiParam->vui_timing_info_present_flag)
  {
    pVuiParam->vui_num_units_in_tick = u(pRP, 32);
    pVuiParam->vui_time_scale = u(pRP, 32);
    pVuiParam->fixed_frame_rate_flag = u(pRP, 1);
  }

  pVuiParam->hrd_param.nal_hrd_parameters_present_flag = u(pRP, 1);

  if(pVuiParam->hrd_param.nal_hrd_parameters_present_flag)
  {
    if(!avc_hrd_parameters(pRP, &pVuiParam->hrd_param, &pVuiParam->hrd_param.nal_sub_hrd_param))
      return false;
  }

  pVuiParam->hrd_param.vcl_hrd_parameters_present_flag = u(pRP, 1);

  if(pVuiParam->hrd_param.vcl_hrd_parameters_present_flag)
  {
    if(!avc_hrd_parameters(pRP, &pVuiParam->hrd_param, &pVuiParam->hrd_param.vcl_sub_hrd_param))
      return false;
  }

  if(pVuiParam->hrd_param.nal_hrd_parameters_present_flag || pVuiParam->hrd_param.vcl_hrd_parameters_present_flag)
    pVuiParam->low_delay_hrd_flag = u(pRP, 1);

  pVuiParam->pic_struct_present_flag = u(pRP, 1);
  pVuiParam->bitstream_restriction_flag = u(pRP, 1);

  if(pVuiParam->bitstream_restriction_flag)
  {
    pVuiParam->motion_vectors_over_pic_boundaries_flag = u(pRP, 1);
    pVuiParam->max_bytes_per_pic_denom = ue(pRP);
    pVuiParam->max_bits_per_min_cu_denom = ue(pRP);
    pVuiParam->log2_max_mv_length_horizontal = ue(pRP);
    pVuiParam->log2_max_mv_length_vertical = ue(pRP);
    pVuiParam->max_num_reorder_frames = ue(pRP);
    pVuiParam->max_dec_frame_buffering = ue(pRP);
  }
  return true;
}

/*****************************************************************************/
AL_EParseResult AL_AVC_ParseSPS(AL_TRbspParser* pRP, AL_TAvcSps* pSPS)
{
  skipAllZerosAndTheNextByte(pRP);
  u(pRP, 8); // Skip NUT

  initSps(pSPS);

  pSPS->profile_idc = u(pRP, 8);
  pSPS->constraint_set0_flag = u(pRP, 1);
  pSPS->constraint_set1_flag = u(pRP, 1);
  pSPS->constraint_set2_flag = u(pRP, 1);
  pSPS->constraint_set3_flag = u(pRP, 1);
  pSPS->constraint_set4_flag = u(pRP, 1);
  pSPS->constraint_set5_flag = u(pRP, 1);
  skip(pRP, 2); // reserved_zero_bits

  pSPS->level_idc = u(pRP, 8);

  pSPS->seq_parameter_set_id = ue(pRP);
  COMPLY_ID(pSPS->seq_parameter_set_id < AL_AVC_MAX_SPS);

  if(!isProfileSupported(pSPS->profile_idc) && !pSPS->constraint_set1_flag)
  {
    Rtos_Log(AL_LOG_ERROR, "Unsupported profile\n");
    return AL_PARSE_UNSUPPORTED;
  }

  if(pSPS->profile_idc == 44 || pSPS->profile_idc == 83 ||
     pSPS->profile_idc == 86 || pSPS->profile_idc == 100 ||
     pSPS->profile_idc == 110 || pSPS->profile_idc == 118 ||
     pSPS->profile_idc == 122 || pSPS->profile_idc == 128 ||
     pSPS->profile_idc == 244)
  {
    pSPS->chroma_format_idc = ue(pRP);

    COMPLY(pSPS->chroma_format_idc <= AL_CHROMA_4_4_4); // 0 to 3 inclusive range

    if(pSPS->chroma_format_idc == 3)
      pSPS->separate_colour_plane_flag = u(pRP, 1);
    pSPS->bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH_MINUS_8);
    pSPS->bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH_MINUS_8);

    pSPS->qpprime_y_zero_transform_bypass_flag = u(pRP, 1);
    pSPS->seq_scaling_matrix_present_flag = u(pRP, 1);

    if(pSPS->seq_scaling_matrix_present_flag)
    {
      for(int32_t i = 0; i < (pSPS->chroma_format_idc != 3 ? 8 : 12); ++i)
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
          propagate_scaling_list(i, pSPS->UseDefaultScalingMatrix4x4Flag, pSPS->UseDefaultScalingMatrix8x8Flag, pSPS->ScalingList4x4, pSPS->ScalingList8x8);
      }
    }
    else
    {
      for(int32_t i = 0; i < (pSPS->chroma_format_idc != 3 ? 8 : 12); ++i)
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

    COMPLY(pSPS->log2_max_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB_MINUS_4);

    pSPS->delta_pic_order_always_zero_flag = 1;
  }
  else if(pSPS->pic_order_cnt_type == 1)
  {
    pSPS->delta_pic_order_always_zero_flag = u(pRP, 1);
    pSPS->offset_for_non_ref_pic = se(pRP);
    pSPS->offset_for_top_to_bottom_field = se(pRP);
    pSPS->num_ref_frames_in_pic_order_cnt_cycle = ue(pRP);

    for(int32_t i = 0; i < pSPS->num_ref_frames_in_pic_order_cnt_cycle; i++)
      pSPS->offset_for_ref_frame[i] = se(pRP);
  }

  pSPS->max_num_ref_frames = Clip3(ue(pRP), 0, AL_MAX_REF);
  pSPS->gaps_in_frame_num_value_allowed_flag = u(pRP, 1);

  pSPS->pic_width_in_mbs_minus1 = ue(pRP);
  pSPS->pic_height_in_map_units_minus1 = ue(pRP);

  pSPS->frame_mbs_only_flag = u(pRP, 1);

  if(!pSPS->frame_mbs_only_flag)
  {
    pSPS->mb_adaptive_frame_field_flag = u(pRP, 1);

    if(pSPS->mb_adaptive_frame_field_flag)
    {
      Rtos_Log(AL_LOG_ERROR, "MBAFF is not supported\n");
      return AL_PARSE_UNSUPPORTED;
    }
  }

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
    COMPLY(avc_vui_parameters(&pSPS->vui_param, pRP));

  // validate current SPS
  pSPS->bConceal = false;

  return AL_PARSE_OK;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiBufferingPeriod(AL_TRbspParser* pRP, AL_TAvcSps* pSpsTable, AL_TAvcBufPeriod* pBufPeriod, AL_TAvcSps** pActiveSps)
{
  AL_TAvcSps* pSPS = NULL;

  pBufPeriod->seq_parameter_set_id = ue(pRP);

  if(pBufPeriod->seq_parameter_set_id >= AL_AVC_MAX_SPS)
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  pSPS = &pSpsTable[pBufPeriod->seq_parameter_set_id];

  if(pSPS->bConceal)
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

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

  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static AL_ESeiParseResult ParseAvcSpecificSeis(AL_TRbspParser* pRP, AL_ESeiPayloadType ePayloadType, uint32_t uPayloadSize, AL_TAup* pOutputAup, bool* pCanSendToUser)
{
  (void)uPayloadSize;

  AL_ESeiParseResult eParseResult = AL_SEI_PARSE_RESULT_UNKNOWN_SEI;
  *pCanSendToUser = true;
  switch(ePayloadType)
  {
  case SEI_PTYPE_BUFFERING_PERIOD:
  {
    AL_TAvcBufPeriod tBufferingPeriod;
    eParseResult = SeiBufferingPeriod(pRP, pOutputAup->avcAup.pSPS, &tBufferingPeriod, &pOutputAup->avcAup.pActiveSPS);
    break;
  }
  default:
    break;
  }

  return eParseResult;
}

/*****************************************************************************/
bool AL_AVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta)
{
  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 8); // Skip NUT

  AL_TSeiParserCtx tParserCtx;
  AL_SeiParser_Init(&tParserCtx, pIAup, pMeta, cb);
  AL_SeiParser_AddCustomSeiParsing(&tParserCtx, ParseAvcSpecificSeis);

  do
  {
    if(!AL_SeiParser_Parse(&tParserCtx, pRP, bIsPrefix))
      return false;
  }
  while(more_rbsp_data(pRP));

  rbsp_trailing_bits(pRP);

  return true;
}

/*****************************************************************************/
static AL_EParseResult AL_AVC_ParseHdrSvcExt(AL_TRbspParser* pRP, AL_TAvcHdrSvcExt* pHdrSvcExt)
{
  pHdrSvcExt->idr_flag = u(pRP, 1);
  pHdrSvcExt->priority_id = u(pRP, 6);
  pHdrSvcExt->no_inter_layer_pred_flag = u(pRP, 1);
  pHdrSvcExt->dependency_id = u(pRP, 3);
  pHdrSvcExt->quality_id = u(pRP, 4);
  pHdrSvcExt->temporal_id = u(pRP, 3);
  pHdrSvcExt->use_ref_base_pic_flag = u(pRP, 1);
  pHdrSvcExt->discardable_flag = u(pRP, 1);
  pHdrSvcExt->output_flag = u(pRP, 1);
  COMPLY(3 == u(pRP, 2)); // reserved_three_2bits

  return AL_PARSE_OK;
}

/*****************************************************************************/
AL_EParseResult AL_AVC_ParseNal(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_ENut eNut)
{
  AL_EParseResult eRes = AL_PARSE_UNSUPPORTED;
  switch(eNut)
  {
  case AL_AVC_NUT_PREFIX:
  case AL_AVC_NUT_EXT:
  {
    skipAllZerosAndTheNextByte(pRP);
    u(pRP, 8); // Skip NUT

    if(u(pRP, 1)) // svc_extension_flag
    {
      AL_TAvcHdrSvcExt tHdrSvcExt;
      eRes = AL_AVC_ParseHdrSvcExt(pRP, &tHdrSvcExt);

      if(eRes == AL_PARSE_OK)
        pIAup->avcAup.uCurTemporalID = tHdrSvcExt.temporal_id;
    }
  } break;

  default:
    break;
  }

  return eRes;
}

/*****************************************************************************/
void AL_AVC_GetCropInfo(AL_TAvcSps const* pSPS, AL_TCropInfo* pCropInfo)
{
  if(pSPS->frame_cropping_flag)
  {
    pCropInfo->bCropping = true;

    int32_t iCropUnitX = 0;
    int32_t iCropUnitY = 0;
    switch(pSPS->chroma_format_idc)
    {
    case 0:  // monochrome
      Rtos_Assert((pSPS->separate_colour_plane_flag == 0) && " pSPS->separate_colour_plane_flag != 0 is not allowed in Monochrome");
      iCropUnitX = 1;
      iCropUnitY = 1;
      break;

    case 1:  // 4:2:0
      Rtos_Assert((pSPS->separate_colour_plane_flag == 0) && " pSPS->separate_colour_plane_flag != 0 is not allowed in 4:2:0");
      iCropUnitX = 2;
      iCropUnitY = 2;
      break;

    case 2:  // 4:2:2
      Rtos_Assert((pSPS->separate_colour_plane_flag == 0) && " pSPS->separate_colour_plane_flag != 0 is not allowed in 4:2:2");
      iCropUnitX = 2;
      iCropUnitY = 1;
      break;

    case 3:  // 4:4:4
      iCropUnitX = 1;
      iCropUnitY = 1;
      break;

    default:
      Rtos_Assert(false && "invalid pSPS->chroma_format_idc");
    }

    iCropUnitY *= (2 - pSPS->frame_mbs_only_flag);

    pCropInfo->uCropOffsetLeft = iCropUnitX * pSPS->frame_crop_left_offset;
    pCropInfo->uCropOffsetRight = iCropUnitX * pSPS->frame_crop_right_offset;

    pCropInfo->uCropOffsetTop = iCropUnitY * pSPS->frame_crop_top_offset;
    pCropInfo->uCropOffsetBottom = iCropUnitY * pSPS->frame_crop_bottom_offset;

    if(!pSPS->frame_mbs_only_flag)
    {
      // Decoder output independent field, not interlaced frame
      pCropInfo->uCropOffsetTop /= 2;
      pCropInfo->uCropOffsetBottom /= 2;
    }
  }
  else
  {
    ResetCropInfo(pCropInfo);
  }
}
