/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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
****************************************************************************/
#include "AVC_RbspEncod.h"
#include "RbspEncod.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/SliceHeader.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/Nuts.h"
#include "lib_common/ScalingList.h"
#include "lib_assert/al_assert.h"

/******************************************************************************/
static int writeScalingList(AL_TBitStreamLite* pBS, uint8_t const* pScalingList, int iSize)
{
  int32_t const* pDecScanBlock = (iSize == 16) ? AL_DecScanBlock4x4[0] : AL_DecScanBlock8x8[0];
  int32_t iLastScale = 8;
  uint8_t iNextScale = 8;
  /* if iSize == 0, we return an error code */
  int iUseDefault = -1;

  for(int j = 0; j < iSize; j++)
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
static void writeScalingMatrix(AL_TBitStreamLite* pBS, uint8_t isScalingMatrixPresent, AL_TAvcSps const* pSps)
{
  if(isScalingMatrixPresent == 0)
    return;

  int iNbScalingList = pSps->chroma_format_idc != 3 ? 8 : 12;

  for(int i = 0; i < iNbScalingList; i++)
  {
    AL_BitStreamLite_PutU(pBS, 1, pSps->seq_scaling_list_present_flag[i]);

    if(pSps->seq_scaling_list_present_flag[i] == 0)
      continue;

    int row, size, column;

    if(i < 6)
    {
      row = 0;
      size = 16;
      column = i;
    }
    else
    {
      row = 1;
      size = 64;
      column = (i % 2) * 3 + ((i - 6) / 2);
    }

    writeScalingList(pBS, pSps->scaling_list_param.ScalingList[row][column], size);
  }
}

/******************************************************************************/
static void writeHrdParam(AL_TBitStreamLite* pBS, AL_THrdParam const* pHrd, AL_TSubHrdParam const* pSubHrd)
{
  AL_Assert(pHrd->cpb_cnt_minus1[0] < MAX_NUM_CPB);

  AL_BitStreamLite_PutUE(pBS, pHrd->cpb_cnt_minus1[0]);
  AL_BitStreamLite_PutU(pBS, 4, pHrd->bit_rate_scale);
  AL_BitStreamLite_PutU(pBS, 4, pHrd->cpb_size_scale);

  for(uint32_t uSchedSelIdx = 0; uSchedSelIdx <= pHrd->cpb_cnt_minus1[0]; uSchedSelIdx++)
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
static void writeSpsData(AL_TBitStreamLite* pBS, AL_TAvcSps const* pSps)
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

  if(
    (pSps->profile_idc == AVC_PROFILE_IDC_HIGH)
    || (pSps->profile_idc == AVC_PROFILE_IDC_HIGH10)
    || (pSps->profile_idc == AVC_PROFILE_IDC_HIGH_422)
    || (pSps->profile_idc == AVC_PROFILE_IDC_HIGH_444_PRED)
    || (pSps->profile_idc == AVC_PROFILE_IDC_CAVLC_444_INTRA)
    || (pSps->profile_idc == 83) || (pSps->profile_idc == 86)
    || (pSps->profile_idc == 118) || (pSps->profile_idc == 128)
    )
  {
    AL_BitStreamLite_PutUE(pBS, pSps->chroma_format_idc);

    if(pSps->chroma_format_idc == 3)
      AL_BitStreamLite_PutBit(pBS, pSps->separate_colour_plane_flag);
    AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_luma_minus8);
    AL_BitStreamLite_PutUE(pBS, pSps->bit_depth_chroma_minus8);
    AL_BitStreamLite_PutU(pBS, 1, pSps->qpprime_y_zero_transform_bypass_flag);
    AL_BitStreamLite_PutU(pBS, 1, pSps->seq_scaling_matrix_present_flag);

    writeScalingMatrix(pBS, pSps->seq_scaling_matrix_present_flag, pSps);
  }

  AL_BitStreamLite_PutUE(pBS, pSps->log2_max_frame_num_minus4);
  AL_BitStreamLite_PutUE(pBS, pSps->pic_order_cnt_type);

  if(pSps->pic_order_cnt_type == 0)
    AL_BitStreamLite_PutUE(pBS, pSps->log2_max_pic_order_cnt_lsb_minus4);

  AL_Assert(pSps->pic_order_cnt_type != 1);

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

    if(pSps->vui_param.overscan_info_present_flag)
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.overscan_appropriate_flag);
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
      writeHrdParam(pBS, &pSps->vui_param.hrd_param, &pSps->vui_param.hrd_param.nal_sub_hrd_param);

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag);

    if(pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
      writeHrdParam(pBS, &pSps->vui_param.hrd_param, &pSps->vui_param.hrd_param.vcl_sub_hrd_param);

    if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag || pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
      AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.hrd_param.low_delay_hrd_flag[0]);

    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.pic_struct_present_flag);
    AL_BitStreamLite_PutU(pBS, 1, pSps->vui_param.bitstream_restriction_flag);
    AL_Assert(pSps->vui_param.bitstream_restriction_flag == 0);
  }
}

/******************************************************************************/
static void writeSps(AL_TBitStreamLite* pBS, AL_TSps const* pSps, int iLayerId)
{
  (void)iLayerId;
  writeSpsData(pBS, &pSps->AvcSPS);

  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/*****************************************************************************/
static void writePps(AL_TBitStreamLite* pBS, AL_TPps const* pIPps)
{
  AL_TAvcPps* pPps = (AL_TAvcPps*)pIPps;

  // 1 - Write PPS following spec 7.3.2.2.

  AL_BitStreamLite_PutUE(pBS, pPps->pic_parameter_set_id);
  AL_BitStreamLite_PutUE(pBS, pPps->seq_parameter_set_id);
  AL_BitStreamLite_PutU(pBS, 1, pPps->entropy_coding_mode_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->bottom_field_pic_order_in_frame_present_flag);
  AL_BitStreamLite_PutUE(pBS, pPps->num_slice_groups_minus1);
  AL_Assert(pPps->num_slice_groups_minus1 == 0);
  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l0_active_minus1);
  AL_BitStreamLite_PutUE(pBS, pPps->num_ref_idx_l1_active_minus1);
  AL_BitStreamLite_PutU(pBS, 1, pPps->weighted_pred_flag);
  AL_BitStreamLite_PutU(pBS, 2, pPps->weighted_bipred_idc);
  AL_BitStreamLite_PutSE(pBS, pPps->pic_init_qp_minus26);
  AL_BitStreamLite_PutSE(pBS, pPps->pic_init_qs_minus26);
  AL_Assert(pPps->chroma_qp_index_offset >= -12 && pPps->chroma_qp_index_offset <= 12);
  AL_BitStreamLite_PutSE(pBS, pPps->chroma_qp_index_offset);
  AL_BitStreamLite_PutU(pBS, 1, pPps->deblocking_filter_control_present_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->constrained_intra_pred_flag);
  AL_BitStreamLite_PutU(pBS, 1, pPps->redundant_pic_cnt_present_flag);

  if(pPps->transform_8x8_mode_flag
     || (pPps->second_chroma_qp_index_offset != pPps->chroma_qp_index_offset))
  {
    AL_BitStreamLite_PutU(pBS, 1, pPps->transform_8x8_mode_flag);
    AL_BitStreamLite_PutU(pBS, 1, pPps->pic_scaling_matrix_present_flag);
    AL_Assert(pPps->pSPS != NULL);
    writeScalingMatrix(pBS, pPps->pic_scaling_matrix_present_flag, pPps->pSPS);
    AL_Assert(pPps->second_chroma_qp_index_offset >= -12 && pPps->second_chroma_qp_index_offset <= 12);
    AL_BitStreamLite_PutSE(pBS, pPps->second_chroma_qp_index_offset);
  }

  // 2 - Write rbsp_trailing_bits.

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
static void writeSeiBufferingPeriod(AL_TBitStreamLite* pBS, AL_TSps const* pISps, int iCpbInitialDelay, int iCpbInitialOffset)
{
  (void)iCpbInitialOffset;
  AL_TAvcSps* pSps = (AL_TAvcSps*)pISps;

  // buffering_period
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 0);

  AL_BitStreamLite_PutUE(pBS, pSps->seq_parameter_set_id);

  int iInitialCpbLength = pSps->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

  if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    for(int i = 0; i <= (int)pSps->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      AL_BitStreamLite_PutU(pBS, iInitialCpbLength, iCpbInitialDelay);
      AL_BitStreamLite_PutU(pBS, iInitialCpbLength, iCpbInitialOffset);
    }
  }

  if(pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    for(int i = 0; i <= (int)pSps->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      AL_BitStreamLite_PutU(pBS, iInitialCpbLength, iCpbInitialDelay);
      AL_BitStreamLite_PutU(pBS, iInitialCpbLength, iCpbInitialOffset);
    }
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static void writeSeiRecoveryPoint(AL_TBitStreamLite* pBS, int iRecoveryFrameCnt)
{
  // recovery_point
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 6);

  AL_BitStreamLite_PutUE(pBS, iRecoveryFrameCnt);
  AL_BitStreamLite_PutBit(pBS, 1); // exact_match_flag
  AL_BitStreamLite_PutBit(pBS, 0); // broken_link_flag
  AL_BitStreamLite_PutBits(pBS, 2, 0); // changing_slice_group_idc

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static void writeSeiPictureTiming(AL_TBitStreamLite* pBS, AL_TSps const* pISps, int iCpbRemovalDelay, int iDpbOutputDelay, int iPicStruct)
{
  AL_TAvcSps* pSps = (AL_TAvcSps*)pISps;
  // Table D-1
  static const int PicStructToNumClockTS[9] =
  {
    1, 1, 1, 2, 2, 3, 3, 2, 3
  };

  // pic_timing
  int bookmark = AL_RbspEncoding_BeginSEI(pBS, 1);

  if(pSps->vui_param.hrd_param.nal_hrd_parameters_present_flag || pSps->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iCpbRemovalDelay);
    AL_BitStreamLite_PutU(pBS, pSps->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1, iDpbOutputDelay);
  }

  if(pSps->vui_param.pic_struct_present_flag)
  {
    AL_BitStreamLite_PutU(pBS, 4, iPicStruct);

    AL_Assert(iPicStruct <= 8 && iPicStruct >= 0);
    AL_BitStreamLite_PutBits(pBS, PicStructToNumClockTS[iPicStruct], 0x0);
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
static AL_ECodec getCodec(void)
{
  return AL_CODEC_AVC;
}

static inline bool isForce4BytesCode(AL_EStartCodeBytesAlignedMode eMode, int nut)
{
  switch(eMode)
  {
  case AL_START_CODE_AUTO: return nut >= AL_AVC_NUT_PREFIX_SEI && nut < AL_AVC_NUT_UNSPEC_24;
  case AL_START_CODE_3_BYTES: return false;
  case AL_START_CODE_4_BYTES: return true;
  default: return nut >= AL_AVC_NUT_PREFIX_SEI && nut < AL_AVC_NUT_UNSPEC_24;
  }

  return nut >= AL_AVC_NUT_PREFIX_SEI && nut < AL_AVC_NUT_UNSPEC_24;
}

static void writeStartCode(AL_TBitStreamLite* pBS, int nut, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  if(isForce4BytesCode(eStartCodeBytesAligned, nut))
    AL_BitStreamLite_PutBits(pBS, 8, 0x00);

  // don't count start code in case of "VCL Compliance"
  AL_BitStreamLite_PutBits(pBS, 8, 0x00);
  AL_BitStreamLite_PutBits(pBS, 8, 0x00);
  AL_BitStreamLite_PutBits(pBS, 8, 0x01);
}

/******************************************************************************/
static IRbspWriter writer =
{
  getCodec,
  AL_RbspEncoding_WriteAUD,
  writeStartCode,
  NULL, /* writeVps */
  writeSps,
  writePps,
  NULL, /* writeSeiActiveParameterSets */
  writeSeiBufferingPeriod,
  writeSeiRecoveryPoint,
  writeSeiPictureTiming,
  AL_RbspEncoding_WriteMasteringDisplayColourVolume,
  AL_RbspEncoding_WriteContentLightLevel,
  AL_RbspEncoding_WriteAlternativeTransferCharacteristics,
  AL_RbspEncoding_WriteST2094_10,
  AL_RbspEncoding_WriteST2094_40,
  AL_RbspEncoding_WriteUserDataUnregistered,
};

IRbspWriter* AL_GetAvcRbspWriter()
{
  return &writer;
}

