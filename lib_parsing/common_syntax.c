// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include "lib_common/Utils.h"
#include "lib_common/SyntaxConversion.h"

#include "common_syntax.h"
#include <string.h>

/*****************************************************************************/
void hevc_profile_tier_level(AL_THevcProfilevel* pPrfLvl, int iMaxSubLayersMinus1, AL_TRbspParser* pRP)
{
  pPrfLvl->general_profile_space = u(pRP, 2);
  pPrfLvl->general_tier_flag = u(pRP, 1);
  pPrfLvl->general_profile_idc = u(pRP, 5);

  for(int i = 0; i < 32; ++i)
    pPrfLvl->general_profile_compatibility_flag[i] = u(pRP, 1);

  pPrfLvl->general_progressive_source_flag = u(pRP, 1);
  pPrfLvl->general_interlaced_source_flag = u(pRP, 1);
  pPrfLvl->general_non_packed_constraint_flag = u(pRP, 1);
  pPrfLvl->general_frame_only_constraint_flag = u(pRP, 1);

  pPrfLvl->general_max_12bit_constraint_flag = 0;
  pPrfLvl->general_max_10bit_constraint_flag = 0;
  pPrfLvl->general_max_8bit_constraint_flag = 0;
  pPrfLvl->general_max_422chroma_constraint_flag = 0;
  pPrfLvl->general_max_420chroma_constraint_flag = 0;
  pPrfLvl->general_max_monochrome_constraint_flag = 0;
  pPrfLvl->general_intra_constraint_flag = 0;
  pPrfLvl->general_one_picture_only_constraint_flag = 0;
  pPrfLvl->general_lower_bit_rate_constraint_flag = 0;
  pPrfLvl->general_max_14bit_constraint_flag = 0;
  pPrfLvl->general_inbld_flag = 0;

  if(pPrfLvl->general_profile_idc == 4 || pPrfLvl->general_profile_compatibility_flag[4] ||
     pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
     pPrfLvl->general_profile_idc == 6 || pPrfLvl->general_profile_compatibility_flag[6] ||
     pPrfLvl->general_profile_idc == 7 || pPrfLvl->general_profile_compatibility_flag[7] ||
     pPrfLvl->general_profile_idc == 8 || pPrfLvl->general_profile_compatibility_flag[8] ||
     pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
     pPrfLvl->general_profile_idc == 10 || pPrfLvl->general_profile_compatibility_flag[10] ||
     pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
  {
    pPrfLvl->general_max_12bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_10bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_8bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_422chroma_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_420chroma_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_monochrome_constraint_flag = u(pRP, 1);
    pPrfLvl->general_intra_constraint_flag = u(pRP, 1);
    pPrfLvl->general_one_picture_only_constraint_flag = u(pRP, 1);
    pPrfLvl->general_lower_bit_rate_constraint_flag = u(pRP, 1);

    if(pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
       pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
       pPrfLvl->general_profile_idc == 10 || pPrfLvl->general_profile_compatibility_flag[10] ||
       pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
    {
      pPrfLvl->general_max_14bit_constraint_flag = u(pRP, 1);
      skip(pRP, 33); // general_reserved_zero_33bits
    }
    else
      skip(pRP, 34); // general_reserved_zero_34bits
  }
  else if(pPrfLvl->general_profile_idc == 2 || pPrfLvl->general_profile_compatibility_flag[2])
  {
    skip(pRP, 7); // general_reserved_zero_7bits
    pPrfLvl->general_one_picture_only_constraint_flag = u(pRP, 1);
    skip(pRP, 35); // general_reserved_zero_35bits
  }
  else
    skip(pRP, 43); // general_reserved_zero_43bits

  if(pPrfLvl->general_profile_idc == 1 || pPrfLvl->general_profile_compatibility_flag[1] ||
     pPrfLvl->general_profile_idc == 2 || pPrfLvl->general_profile_compatibility_flag[2] ||
     pPrfLvl->general_profile_idc == 3 || pPrfLvl->general_profile_compatibility_flag[3] ||
     pPrfLvl->general_profile_idc == 4 || pPrfLvl->general_profile_compatibility_flag[4] ||
     pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
     pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
     pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
  {
    pPrfLvl->general_inbld_flag = u(pRP, 1);
  }
  else
    skip(pRP, 1); // general_reserved_zero_bit

  pPrfLvl->general_level_idc = u(pRP, 8);

  for(int i = 0; i < iMaxSubLayersMinus1; ++i)
  {
    pPrfLvl->sub_layer_profile_present_flag[i] = u(pRP, 1);
    pPrfLvl->sub_layer_level_present_flag[i] = u(pRP, 1);
  }

  if(iMaxSubLayersMinus1 > 0)
  {
    for(int i = iMaxSubLayersMinus1; i < 8; i++)
      skip(pRP, 2); // reserved_zero_2_bits
  }

  for(int i = 0; i < iMaxSubLayersMinus1; ++i)
  {
    if(pPrfLvl->sub_layer_profile_present_flag[i])
    {
      pPrfLvl->sub_layer_profile_space[i] = u(pRP, 2);
      pPrfLvl->sub_layer_tier_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_profile_idc[i] = u(pRP, 5);

      for(int j = 0; j < 32; ++j)
        pPrfLvl->sub_layer_profile_compatibility_flag[i][j] = u(pRP, 1);

      pPrfLvl->sub_layer_progressive_source_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_interlaced_source_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_non_packed_constraint_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_frame_only_constraint_flag[i] = u(pRP, 1);

      pPrfLvl->sub_layer_max_12bit_constraint_flag[i] = pPrfLvl->general_max_12bit_constraint_flag;
      pPrfLvl->sub_layer_max_10bit_constraint_flag[i] = pPrfLvl->general_max_10bit_constraint_flag;
      pPrfLvl->sub_layer_max_8bit_constraint_flag[i] = pPrfLvl->general_max_8bit_constraint_flag;
      pPrfLvl->sub_layer_max_422chroma_constraint_flag[i] = pPrfLvl->general_max_422chroma_constraint_flag;
      pPrfLvl->sub_layer_max_420chroma_constraint_flag[i] = pPrfLvl->general_max_420chroma_constraint_flag;
      pPrfLvl->sub_layer_max_monochrome_constraint_flag[i] = pPrfLvl->general_max_monochrome_constraint_flag;
      pPrfLvl->sub_layer_intra_constraint_flag[i] = pPrfLvl->general_intra_constraint_flag;
      pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = pPrfLvl->general_one_picture_only_constraint_flag;
      pPrfLvl->sub_layer_lower_bit_rate_constraint_flag[i] = pPrfLvl->general_lower_bit_rate_constraint_flag;
      pPrfLvl->sub_layer_max_14bit_constraint_flag[i] = pPrfLvl->general_max_14bit_constraint_flag;
      pPrfLvl->sub_layer_inbld_flag[i] = pPrfLvl->general_inbld_flag;

      if(pPrfLvl->sub_layer_profile_idc[i] == 4 || pPrfLvl->sub_layer_profile_compatibility_flag[i][4] ||
         pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
         pPrfLvl->sub_layer_profile_idc[i] == 6 || pPrfLvl->sub_layer_profile_compatibility_flag[i][6] ||
         pPrfLvl->sub_layer_profile_idc[i] == 7 || pPrfLvl->sub_layer_profile_compatibility_flag[i][7] ||
         pPrfLvl->sub_layer_profile_idc[i] == 8 || pPrfLvl->sub_layer_profile_compatibility_flag[i][8] ||
         pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
         pPrfLvl->sub_layer_profile_idc[i] == 10 || pPrfLvl->sub_layer_profile_compatibility_flag[i][10] ||
         pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
      {
        pPrfLvl->sub_layer_max_12bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_10bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_8bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_422chroma_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_420chroma_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_monochrome_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_intra_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_lower_bit_rate_constraint_flag[i] = u(pRP, 1);

        if(pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
           pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
           pPrfLvl->sub_layer_profile_idc[i] == 10 || pPrfLvl->sub_layer_profile_compatibility_flag[i][10] ||
           pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
        {
          pPrfLvl->sub_layer_max_14bit_constraint_flag[i] = u(pRP, 1);
          skip(pRP, 33); // sub_layer_reserved_zero_33bits
        }
        else
          skip(pRP, 34); // sub_layer_reserved_zero_34bits
      }
      else if(pPrfLvl->sub_layer_profile_idc[i] == 2 || pPrfLvl->sub_layer_profile_compatibility_flag[i][2])
      {
        skip(pRP, 7); // sub_layer_reserved_zero_7bits
        pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = u(pRP, 1);
        skip(pRP, 35); // sub_layer_reserved_zero_35bits
      }
      else
        skip(pRP, 43); // sub_layer_reserved_zero_43bits

      if(pPrfLvl->sub_layer_profile_idc[i] == 1 || pPrfLvl->sub_layer_profile_compatibility_flag[i][1] ||
         pPrfLvl->sub_layer_profile_idc[i] == 2 || pPrfLvl->sub_layer_profile_compatibility_flag[i][2] ||
         pPrfLvl->sub_layer_profile_idc[i] == 3 || pPrfLvl->sub_layer_profile_compatibility_flag[i][3] ||
         pPrfLvl->sub_layer_profile_idc[i] == 4 || pPrfLvl->sub_layer_profile_compatibility_flag[i][4] ||
         pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
         pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
         pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
      {
        pPrfLvl->sub_layer_inbld_flag[i] = u(pRP, 1);
      }
      else
        skip(pRP, 1); // sub_layer_reserved_zero_bit
    }

    if(pPrfLvl->sub_layer_level_present_flag[i])
      pPrfLvl->sub_layer_level_idc[i] = u(pRP, 8);
  }
}

/*****************************************************************************/
void avc_scaling_list_data(uint8_t* pScalingList, AL_TRbspParser* pRP, int iSize, uint8_t* pUseDefaultScalingMatrixFlag)
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
  int iLastScale = 8;
  int iNextScale = 8;

  for(int j = 0; j < iSize; j++)
  {
    uint8_t i = pDecScanBlock[j];

    if(iNextScale != 0)
    {
      int iDeltaScale = Clip3(se(pRP), -128, 127);
      iNextScale = (iLastScale + iDeltaScale + 256) % 256;
      *pUseDefaultScalingMatrixFlag = (j == 0 && iNextScale == 0);
    }

    pScalingList[i] = iNextScale == 0 ? iLastScale : iNextScale;
    iLastScale = pScalingList[i];
  }
}

/*****************************************************************************/
void hevc_scaling_list_data(AL_TSCLParam* pSCLParam, AL_TRbspParser* pRP)
{
  for(uint8_t uSizeID = 0; uSizeID < 4; ++uSizeID)
  {
    uint8_t uIncr = (uSizeID == 3) ? 3 : 1;

    for(uint8_t uMatrixID = 0; uMatrixID < 6; uMatrixID += uIncr)
    {
      pSCLParam->scaling_list_pred_mode_flag[uSizeID][uMatrixID] = u(pRP, 1);

      if(!pSCLParam->scaling_list_pred_mode_flag[uSizeID][uMatrixID])
      {
        pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID] = ue(pRP);

        if(!pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID])
        {
          if(uSizeID > 1) /* default dc coeff */
            pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = 16;

          if(uSizeID) /* superior to 4x4 */
            memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], AL_HEVC_DefaultScalingLists8x8[uMatrixID / 3], 64);
          else /* equal to 4x4 */
            memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], AL_HEVC_DefaultScalingLists4x4[uMatrixID / 3], 16);
        }
        else
        {
          uint8_t uPredMatrixID = Clip3(uMatrixID - pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID], 0, ((uSizeID == 3) ? 0 : 4));

          if(uSizeID > 1) /* default dc coeff */
            pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uPredMatrixID];

          if(uSizeID) /* superior to 4x4 */
            memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], pSCLParam->ScalingList[uSizeID][uPredMatrixID], 64);
          else /* equal to 4x4 */
            memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], pSCLParam->ScalingList[uSizeID][uPredMatrixID], 16);
        }
      }
      else
      {
        int16_t uNextCoeff = 8;
        uint16_t uCoeffNum = Min(64, (1 << (4 + (uSizeID << 1))));
        uint8_t const* pScanOrder = (uCoeffNum == 64) ? AL_HEVC_ScanOrder8x8 : AL_HEVC_ScanOrder4x4;

        if(uSizeID > 1)
        {
          uNextCoeff = Clip3(se(pRP), -7, 247) + 8;
          pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = uNextCoeff;
        }

        for(uint8_t uCoeff = 0; uCoeff < uCoeffNum; ++uCoeff)
        {
          uNextCoeff = (uNextCoeff + Clip3(se(pRP), -128, 127) + 256) % 256; // scaling_list_delta_coeff
          pSCLParam->ScalingList[uSizeID][uMatrixID][pScanOrder[uCoeff]] = uNextCoeff;
        }
      }
    }
  }
}

/*****************************************************************************/
static bool avc_hrd_parameters(AL_TRbspParser* pRP, AL_THrdParam* pHrdParam, AL_TSubHrdParam* pSubHrdParam)
{
  for(int i = 0; i < 32; ++i)
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
static void hevc_sub_hrd_parameters(AL_TSubHrdParam* pSubHrdParam, int cpb_cnt, uint8_t sub_pic_hrd_params_present_flag, AL_TRbspParser* pRP)
{
  for(int i = 0; i <= cpb_cnt; ++i)
  {
    pSubHrdParam->bit_rate_value_minus1[i] = ue(pRP);
    pSubHrdParam->cpb_size_value_minus1[i] = ue(pRP);

    if(sub_pic_hrd_params_present_flag)
    {
      pSubHrdParam->cpb_size_du_value_minus1[i] = ue(pRP);
      pSubHrdParam->bit_rate_du_value_minus1[i] = ue(pRP);
    }
    pSubHrdParam->cbr_flag[i] = u(pRP, 1);
  }
}

/*****************************************************************************/
void hevc_hrd_parameters(AL_THrdParam* pHrdParam, bool bInfoFlag, int iMaxSubLayersMinus1, AL_TRbspParser* pRP)
{
  if(bInfoFlag)
  {
    pHrdParam->nal_hrd_parameters_present_flag = u(pRP, 1);
    pHrdParam->vcl_hrd_parameters_present_flag = u(pRP, 1);

    if(pHrdParam->nal_hrd_parameters_present_flag || pHrdParam->vcl_hrd_parameters_present_flag)
    {
      pHrdParam->sub_pic_hrd_params_present_flag = u(pRP, 1);

      if(pHrdParam->sub_pic_hrd_params_present_flag)
      {
        pHrdParam->tick_divisor_minus2 = u(pRP, 8);
        pHrdParam->du_cpb_removal_delay_increment_length_minus1 = u(pRP, 5);
        pHrdParam->sub_pic_cpb_params_in_pic_timing_sei_flag = u(pRP, 1);
        pHrdParam->dpb_output_delay_du_length_minus1 = u(pRP, 5);
      }

      pHrdParam->bit_rate_scale = u(pRP, 4);
      pHrdParam->cpb_size_du_scale = u(pRP, 4);

      if(pHrdParam->sub_pic_hrd_params_present_flag)
        pHrdParam->cpb_size_scale = u(pRP, 4);
      pHrdParam->initial_cpb_removal_delay_length_minus1 = u(pRP, 5);
      pHrdParam->au_cpb_removal_delay_length_minus1 = u(pRP, 5);
      pHrdParam->dpb_output_delay_length_minus1 = u(pRP, 5);
    }
  }
  else
  {
    pHrdParam->nal_hrd_parameters_present_flag = 0;
    pHrdParam->vcl_hrd_parameters_present_flag = 0;
  }

  for(int i = 0; i <= iMaxSubLayersMinus1; ++i)
  {
    /* initialization */
    pHrdParam->low_delay_hrd_flag[i] = 0;
    pHrdParam->cpb_cnt_minus1[i] = 0;

    pHrdParam->fixed_pic_rate_general_flag[i] = u(pRP, 1);

    if(!pHrdParam->fixed_pic_rate_general_flag[i])
      pHrdParam->fixed_pic_rate_within_cvs_flag[i] = u(pRP, 1);
    else
      pHrdParam->fixed_pic_rate_within_cvs_flag[i] = 1;

    if(pHrdParam->fixed_pic_rate_within_cvs_flag[i])
      pHrdParam->elemental_duration_in_tc_minus1[i] = ue(pRP);
    else
      pHrdParam->low_delay_hrd_flag[i] = u(pRP, 1);

    if(!pHrdParam->low_delay_hrd_flag[i])
      pHrdParam->cpb_cnt_minus1[i] = ue(pRP);

    /* Concealment: E.2.2 : cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive */
    pHrdParam->cpb_cnt_minus1[i] = UnsignedMin(pHrdParam->cpb_cnt_minus1[i], 31);

    if(pHrdParam->nal_hrd_parameters_present_flag)
      hevc_sub_hrd_parameters(&pHrdParam->nal_sub_hrd_param, pHrdParam->cpb_cnt_minus1[i], pHrdParam->sub_pic_hrd_params_present_flag, pRP);

    if(pHrdParam->vcl_hrd_parameters_present_flag)
      hevc_sub_hrd_parameters(&pHrdParam->vcl_sub_hrd_param, pHrdParam->cpb_cnt_minus1[i], pHrdParam->sub_pic_hrd_params_present_flag, pRP);
  }
}

/*****************************************************************************/
bool avc_vui_parameters(AL_TVuiParam* pVuiParam, AL_TRbspParser* pRP)
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
    pVuiParam->num_reorder_frames = ue(pRP);
    pVuiParam->max_dec_frame_buffering = ue(pRP);
  }
  return true;
}

/*****************************************************************************/
void hevc_vui_parameters(AL_TVuiParam* pVuiParam, int iMaxSubLayersMinus1, AL_TRbspParser* pRP)
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
    pVuiParam->overscan_appropriate_flag = u(pRP, 1);

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
  pVuiParam->neutral_chroma_indication_flag = u(pRP, 1);
  pVuiParam->field_seq_flag = u(pRP, 1);
  pVuiParam->frame_field_info_present_flag = u(pRP, 1);

  pVuiParam->default_display_window_flag = u(pRP, 1);

  if(pVuiParam->default_display_window_flag)
  {
    pVuiParam->def_disp_win_left_offset = ue(pRP);
    pVuiParam->def_disp_win_right_offset = ue(pRP);
    pVuiParam->def_disp_win_top_offset = ue(pRP);
    pVuiParam->def_disp_win_bottom_offset = ue(pRP);
  }

  pVuiParam->vui_timing_info_present_flag = u(pRP, 1);

  if(pVuiParam->vui_timing_info_present_flag)
  {
    pVuiParam->vui_num_units_in_tick = u(pRP, 32);
    pVuiParam->vui_time_scale = u(pRP, 32);
    pVuiParam->vui_poc_proportional_to_timing_flag = u(pRP, 1);

    if(pVuiParam->vui_poc_proportional_to_timing_flag)
      pVuiParam->vui_num_ticks_poc_diff_one_minus1 = ue(pRP);

    pVuiParam->vui_hrd_parameters_present_flag = u(pRP, 1);

    if(pVuiParam->vui_hrd_parameters_present_flag)
      hevc_hrd_parameters(&pVuiParam->hrd_param, true, iMaxSubLayersMinus1, pRP);
  }

  pVuiParam->bitstream_restriction_flag = u(pRP, 1);

  if(pVuiParam->bitstream_restriction_flag)
  {
    pVuiParam->tiles_fixed_structure_flag = u(pRP, 1);
    pVuiParam->motion_vectors_over_pic_boundaries_flag = u(pRP, 1);
    pVuiParam->restricted_ref_pic_lists_flag = u(pRP, 1);
    pVuiParam->min_spatial_segmentation_idc = ue(pRP);
    pVuiParam->max_bytes_per_pic_denom = ue(pRP);
    pVuiParam->max_bits_per_min_cu_denom = ue(pRP);
    pVuiParam->log2_max_mv_length_horizontal = ue(pRP);
    pVuiParam->log2_max_mv_length_vertical = ue(pRP);
  }
}

/*@}*/

