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
 *****************************************************************************/
#include "AVC_RbspEncod.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/SliceHeader.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/ScalingList.h"

#include <assert.h>

/*****************************************************************************/
void AL_AVC_RbspEncoding_WriteAUD(AL_TRbspEncoding* pRE, int primary_pic_type)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  // 1 - Write primary_pic_type.

  AL_BitStreamLite_PutU(pBS, 3, primary_pic_type);

  // 2 - Write rbsp_trailing_bits.

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static int AL_sAVC_RbspEncoding_WriteScalingList(AL_TBitStreamLite* pBS, uint8_t const* pScalingList, int iSize)
{
  int32_t const* pDecScanBlock = (iSize == 16) ? AL_DecScanBlock4x4[0] : AL_DecScanBlock8x8[0];
  int32_t iLastScale = 8;
  uint8_t iNextScale = 8;
  /* if iSize == 0, we return an error code */
  int iUseDefault = -1;

  int j;

  for(j = 0; j < iSize; j++)
  {
    int i = pDecScanBlock[j];

    if(iNextScale != 0)
    {
      int32_t iDeltaScale = pScalingList[i] - iLastScale;

      if(iDeltaScale > 127)
        iDeltaScale -= 256;
      else if(iDeltaScale < -128)
        iDeltaScale += 256;

      AL_BitStreamLite_PutSE(pBS, iDeltaScale);
      iNextScale = pScalingList[i];
      iUseDefault = (i == 0 && iNextScale == 0);
    }
    // pScalingList[i] = (iNextScale == 0) ? iLastScale : iNextScale;
    iLastScale = pScalingList[i];
  }

  return iUseDefault;
}

/******************************************************************************/
static void AL_sAVC_RbspEncoding_WriteHrdParam(AL_TBitStreamLite* pBS, AL_THrdParam const* pHrd, AL_TSubHrdParam const* pSubHrd)
{
  uint32_t uSchedSelIdx;

  assert(pHrd->cpb_cnt_minus1[0] < AL_MAX_NUM_CPB);

  AL_BitStreamLite_PutUE(pBS, pHrd->cpb_cnt_minus1[0]);
  AL_BitStreamLite_PutU(pBS, 4, pHrd->bit_rate_scale);
  AL_BitStreamLite_PutU(pBS, 4, pHrd->cpb_size_scale);

  for(uSchedSelIdx = 0; uSchedSelIdx <= pHrd->cpb_cnt_minus1[0]; uSchedSelIdx++)
  {
    AL_BitStreamLite_PutUE(pBS, pSubHrd->bit_rate_value_minus1[uSchedSelIdx]);
    AL_BitStreamLite_PutUE(pBS, pSubHrd->cpb_size_value_minus1[uSchedSelIdx]);
    AL_BitStreamLite_PutU(pBS, 1, pSubHrd->cbr_flag[uSchedSelIdx]);
  }

  AL_BitStreamLite_PutU(pBS, 5, pHrd->initial_cpb_removal_delay_length_minus1);
  AL_BitStreamLite_PutU(pBS, 5, pHrd->au_cpb_removal_delay_length_minus1);
  AL_BitStreamLite_PutU(pBS, 5, pHrd->dpb_output_delay_length_minus1);
  AL_BitStreamLite_PutU(pBS, 5, pHrd->time_offset_length);
}

/******************************************************************************/
static void AL_sAVC_RbspEncoding_WriteSpsData(AL_TBitStreamLite* pBS, AL_TAvcSps const* pSps)
{
  // 1 - Write SPS following spec 7.3.2.1.1

  AL_BitStreamLite_PutU(pBS, 8, pSps->profile_idc);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set0_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set1_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set2_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set3_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set4_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->constraint_set5_flag);
  AL_BitStreamLite_PutU(pBS, 2, 0);
  AL_BitStreamLite_PutU(pBS, 8, pSps->level_idc);
  AL_BitStreamLite_PutUE(pBS, pSps->seq_parameter_set_id);

  if((pSps->profile_idc == 100) || (pSps->profile_idc == 110)
     || (pSps->profile_idc == 122) || (pSps->profile_idc == 244)
     || (pSps->profile_idc == 44)
     || (pSps->profile_idc == 83) || (pSps->profile_idc == 86)
     || (pSps->profile_idc == 118) || (pSps->profile_idc == 128))
  {
    AL_BitStreamLite_PutUE(pBS, pSps->chroma_format_idc);
    assert(pSps->chroma_format_idc != 3);
    AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_luma_minus8);
    AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_chroma_minus8);
    AL_BitStreamLite_PutU(pBS, 1, pSps->qpprime_y_zero_transform_bypass_flag);
    AL_BitStreamLite_PutU(pBS, 1, pSps->seq_scaling_matrix_present_flag);

    if(pSps->seq_scaling_matrix_present_flag)
    {
      int i;

      for(i = 0; i < 8; i++)
      {
        AL_BitStreamLite_PutU(pBS, 1, pSps->seq_scaling_list_present_flag[i]);

        if(pSps->seq_scaling_list_present_flag[i])
        {
          switch(i)
          {
          case 0: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTRA)]), 16);
            break;
          case 1: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTRA) + 1]), 16);
            break;
          case 2: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTRA) + 2]), 16);
            break;
          case 3: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTER)]), 16);
            break;
          case 4: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTER) + 1]), 16);
            break;
          case 5: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[0][(3 * AL_SL_INTER) + 2]), 16);
            break;
          case 6: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[1][(3 * AL_SL_INTRA)]), 64);
            break;
          case 7: AL_sAVC_RbspEncoding_WriteScalingList(pBS, (pSps->scaling_list_param.ScalingList[1][(3 * AL_SL_INTER)]), 64);
            break;
          }
        }
      } // for (int i = 0; i < 8; i++)
    } // if (pSps->seq_scaling_matrix_present_flag)
  } // if (pSps->profile_idc >= 100).

  AL_BitStreamLite_PutUE(pBS, pSps->log2_max_frame_num_minus4);
  AL_BitStreamLite_PutUE(pBS, pSps->pic_order_cnt_type);

  if(pSps->pic_order_cnt_type == 0)
    AL_BitStreamLite_PutUE(pBS, pSps->log2_max_pic_order_cnt_lsb_minus4);
  else if(pSps->pic_order_cnt_type == 1)
    assert(0);
  AL_BitStreamLite_PutUE(pBS, pSps->max_num_ref_frames);
  AL_BitStreamLite_PutU(pBS, 1, pSps->gaps_in_frame_num_value_allowed_flag);
  AL_BitStreamLite_PutUE(pBS, pSps->pic_width_in_mbs_minus1);
  AL_BitStreamLite_PutUE(pBS, pSps->pic_height_in_map_units_minus1);
  AL_BitStreamLite_PutU(pBS, 1, pSps->frame_mbs_only_flag);

  if(!pSps->frame_mbs_only_flag)
    AL_BitStreamLite_PutU(pBS, 1, pSps->mb_adaptive_frame_field_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->direct_8x8_inference_flag);
  AL_BitStreamLite_PutU(pBS, 1, pSps->frame_cropping_flag);

  if(pSps->frame_cropping_flag)
  {
    AL_BitStreamLite_PutUE(pBS, pSps->frame_crop_left_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->frame_crop_right_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->frame_crop_top_offset);
    AL_BitStreamLite_PutUE(pBS, pSps->frame_crop_bottom_offset);
  }
  AL_BitStreamLite_PutU(pBS, 1, pSps->vui_parameters_present_flag);

  // 2 - Write VUI following spec E.1.1.

  if(pSps->vui_parameters_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.aspect_ratio_info_present_flag);

    if(pSps->vui_param.aspect_ratio_info_present_flag)
    {
      AL_BitStreamLite_PutU(pBS, 8, pSps->vui_param.aspect_ratio_idc);

      if(pSps->vui_param.aspect_ratio_idc == 255) // Extended_SAR
      {
        AL_BitStreamLite_PutU(pBS, 16, pSps->vui_param.sar_width);
        AL_BitStreamLite_PutU(pBS, 16, pSps->vui_param.sar_height);
      }
    }
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.overscan_info_present_flag);
    assert(pSps->vui_param.overscan_info_present_flag == 0);
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.video_signal_type_present_flag);

    if(pSps->vui_param.video_signal_type_present_flag)
    {
      AL_BitStreamLite_PutU(pBS, 3, pSps->vui_param.video_format);
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.video_full_range_flag);
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.colour_description_present_flag);

      if(pSps->vui_param.colour_description_present_flag)
      {
        AL_BitStreamLite_PutU(pBS, 8, pSps->vui_param.colour_primaries);
        AL_BitStreamLite_PutU(pBS, 8, pSps->vui_param.transfer_characteristics);
        AL_BitStreamLite_PutU(pBS, 8, pSps->vui_param.matrix_coefficients);
      }
    }
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.chroma_loc_info_present_flag);

    if(pSps->vui_param.chroma_loc_info_present_flag)
    {
      AL_BitStreamLite_PutUE(pBS, pSps->vui_param.chroma_sample_loc_type_top_field);
      AL_BitStreamLite_PutUE(pBS, pSps->vui_param.chroma_sample_loc_type_bottom_field);
    }

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.vui_timing_info_present_flag);

    if(pSps->vui_param.vui_timing_info_present_flag)
    {
      AL_BitStreamLite_PutU(pBS, 32, pSps->vui_param.vui_num_units_in_tick);
      AL_BitStreamLite_PutU(pBS, 32, pSps->vui_param.vui_time_scale);
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.fixed_frame_rate_flag);
    }

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag);

    if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag)
      AL_sAVC_RbspEncoding_WriteHrdParam(pBS, &pSps->vui_param.hrd_param, &pSps->vui_param.hrd_param.nal_sub_hrd_param);

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag);

    if(pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
      AL_sAVC_RbspEncoding_WriteHrdParam(pBS, &pSps->vui_param.hrd_param, &pSps->vui_param.hrd_param.vcl_sub_hrd_param);

    if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag || pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.hrd_param.low_delay_hrd_flag[0]);

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.pic_struct_present_flag);
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.bitstream_restriction_flag);
    assert(pSps->vui_param.bitstream_restriction_flag == 0);
  }
}

/******************************************************************************/
void AL_AVC_RbspEncoding_WriteSPS(AL_TRbspEncoding* pRE, AL_TAvcSps const* pSps)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  AL_sAVC_RbspEncoding_WriteSpsData(pBS, pSps);

  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/*****************************************************************************/
void AL_AVC_RbspEncoding_WritePPS(AL_TRbspEncoding* pRE, AL_TAvcPps const* pPps)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  // 1 - Write PPS following spec 7.3.2.2.

  AL_BitStreamLite_PutUE(pBS, pPps->pic_parameter_set_id);
  AL_BitStreamLite_PutUE(pBS, pPps->seq_parameter_set_id);
  AL_BitStreamLite_PutU(pBS, 1, pPps->entropy_coding_mode_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->bottom_field_pic_order_in_frame_present_flag);
  AL_BitStreamLite_PutUE(pBS, pPps->num_slice_groups_minus1);
  assert(pPps->num_slice_groups_minus1 == 0);
  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l0_active_minus1);
  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l1_active_minus1);
  AL_BitStreamLite_PutU(pBS, 1, pPps->weighted_pred_flag);
  AL_BitStreamLite_PutU(pBS, 2, pPps->weighted_bipred_idc);
  AL_BitStreamLite_PutSE(pBS, pPps->pic_init_qp_minus26);
  AL_BitStreamLite_PutSE(pBS, pPps->pic_init_qs_minus26);
  assert(pPps->chroma_qp_index_offset >= -12 && pPps->chroma_qp_index_offset <= 12);
  AL_BitStreamLite_PutSE(pBS, pPps->chroma_qp_index_offset);
  AL_BitStreamLite_PutU(pBS, 1, pPps->deblocking_filter_control_present_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->constrained_intra_pred_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->redundant_pic_cnt_present_flag);

  if(pPps->transform_8x8_mode_flag
     || (pPps->second_chroma_qp_index_offset != pPps->chroma_qp_index_offset))
  {
    AL_BitStreamLite_PutU(pBS, 1, pPps->transform_8x8_mode_flag);
    AL_BitStreamLite_PutU(pBS, 1, pPps->pic_scaling_matrix_present_flag);
    assert(pPps->pic_scaling_matrix_present_flag == 0);
    assert(pPps->second_chroma_qp_index_offset >= -12 && pPps->second_chroma_qp_index_offset <= 12);
    AL_BitStreamLite_PutSE(pBS, pPps->second_chroma_qp_index_offset);
  }

  // 2 - Write rbsp_trailing_bits.

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static void AL_sAVC_RbspEncoding_BeginSEI(AL_TRbspEncoding* pRE, uint8_t uPayloadType)
{
  assert(pRE->m_iBookmarkSEI == 0);
  AL_BitStreamLite_PutBits(pRE->m_pBS, 8, uPayloadType);
  pRE->m_iBookmarkSEI = AL_BitStreamLite_GetBitsCount(pRE->m_pBS);
  assert(pRE->m_iBookmarkSEI % 8 == 0);
  AL_BitStreamLite_PutBits(pRE->m_pBS, 8, 0xFF);
}

/******************************************************************************/
static void AL_sAVC_RbspEncoding_EndSEI(AL_TRbspEncoding* pRE, bool bMVCSN)
{
  uint8_t* pSize;

  assert(pRE->m_iBookmarkSEI != 0);

  pSize = AL_BitStreamLite_GetData(pRE->m_pBS) + (pRE->m_iBookmarkSEI + 7) / 8;
  assert(*pSize == 0xFF);
  *pSize = (AL_BitStreamLite_GetBitsCount(pRE->m_pBS) - pRE->m_iBookmarkSEI - 1) / 8;

  pRE->m_iBookmarkSEI = 0;

  if(pRE->m_pMVCSN)
  {
    *(pRE->m_pMVCSN) += *pSize + 2;
    pRE->m_pMVCSN = NULL;
  }
  else if(bMVCSN)
    pRE->m_pMVCSN = pSize;
}

/******************************************************************************/
void AL_AVC_RbspEncoding_WriteSEI_MVCSN(AL_TRbspEncoding* pRE, int iNumViews)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  // MVC scalable nesting
  AL_sAVC_RbspEncoding_BeginSEI(pRE, 37);

  AL_BitStreamLite_PutU(pBS, 1, 1); // operation point flag

  AL_BitStreamLite_PutUE(pBS, iNumViews - 1); // num_view_components_op_minus1

  while(iNumViews--)
    AL_BitStreamLite_PutU(pBS, 10, iNumViews); // sei_op_view_id

  AL_BitStreamLite_PutU(pBS, 3, 0); // sei_ip_temporal_id

  AL_BitStreamLite_AlignWithBits(pBS, 0); // sei_nesting_zero_bit

  AL_sAVC_RbspEncoding_EndSEI(pRE, true);
}

/******************************************************************************/
void AL_AVC_RbspEncoding_WriteSEI_BP(AL_TRbspEncoding* pRE, AL_TAvcSps const* pSps, int iCpbInitialDelay)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  uint32_t iSecParamSetId = pSps->seq_parameter_set_id;
  int iIniCPBLength = pSps->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

  // buffering_period
  AL_sAVC_RbspEncoding_BeginSEI(pRE, 0);

  AL_BitStreamLite_PutUE(pBS, iSecParamSetId);

  AL_BitStreamLite_PutU(pBS, iIniCPBLength, iCpbInitialDelay);
  AL_BitStreamLite_PutU(pBS, iIniCPBLength, 0); // offset

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_sAVC_RbspEncoding_EndSEI(pRE, false);
}

/******************************************************************************/
void AL_AVC_RbspEncoding_WriteSEI_RP(AL_TRbspEncoding* pRE)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  // recovery_point
  AL_sAVC_RbspEncoding_BeginSEI(pRE, 6);

  AL_BitStreamLite_PutUE(pBS, 0);
  AL_BitStreamLite_PutBits(pBS, 4, 0x8);

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_sAVC_RbspEncoding_EndSEI(pRE, false);
}

/******************************************************************************/
void AL_AVC_RbspEncoding_WriteSEI_PT(AL_TRbspEncoding* pRE, AL_TAvcSps const* pSps, int iCpbRemovalDelay, int iDpbOutputDelay, int iPicStruct)
{
  // Table D-1
  static const int PicStructToNumClockTS[9] =
  {
    1, 1, 1, 2, 2, 3, 3, 2, 3
  };

  AL_TBitStreamLite* pBS = pRE->m_pBS;

  // pic_timing
  AL_sAVC_RbspEncoding_BeginSEI(pRE, 1);

  if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag || pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 32, iCpbRemovalDelay);
    AL_BitStreamLite_PutU(pBS, 32, iDpbOutputDelay);
  }

  if(pSps->vui_param.pic_struct_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 4, iPicStruct);

    assert(iPicStruct <= 8 && iPicStruct >= 0);
    AL_BitStreamLite_PutBits(pBS, PicStructToNumClockTS[iPicStruct], 0x0);
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_sAVC_RbspEncoding_EndSEI(pRE, false);
}

/******************************************************************************/
void AL_AVC_RbspEncoding_CloseSEI(AL_TRbspEncoding* pRE)
{
  AL_TBitStreamLite* pBS = pRE->m_pBS;

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

