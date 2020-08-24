/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "EncUtils.h"
#include "lib_common_enc/EncHwScalingList.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "IP_EncoderCtx.h"

/****************************************************************************/
static int getScalingListPresentId(int iSizeId, int iIntraInter, int iYCbCr)
{
  return iSizeId == 0 ? ((iIntraInter * 3) + iYCbCr) : (6 + iIntraInter + iYCbCr * 2);
}

/****************************************************************************/
static void fillScalingList(AL_TEncSettings const* pSettings, AL_TAvcSps* pSPS, int iSizeId, int iIntraInter, int iYCbCr)
{
  int iMatrixId = (3 * iIntraInter) + iYCbCr;
  int iMatrixSize = iSizeId == 0 ? 16 : 64;
  uint8_t* pSL = pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId];
  int iSLPresentId = getScalingListPresentId(iSizeId, iIntraInter, iYCbCr);

  if(pSettings->SclFlag[iSizeId][iMatrixId] != 0)
  {
    pSPS->seq_scaling_list_present_flag[iSLPresentId] = 1; // scaling list present in file
    Rtos_Memcpy(pSL, pSettings->ScalingList[iSizeId][iMatrixId], iMatrixSize);
    return;
  }

  pSPS->seq_scaling_list_present_flag[iSLPresentId] = 0;

  if(iYCbCr == 0 || !pSPS->seq_scaling_list_present_flag[getScalingListPresentId(iSizeId, iIntraInter, iYCbCr - 1)])
    Rtos_Memcpy(pSL, iSizeId == 0 ? AL_AVC_DefaultScalingLists4x4[iIntraInter] : AL_AVC_DefaultScalingLists8x8[iIntraInter], iMatrixSize);
  else
  {
    pSPS->seq_scaling_list_present_flag[iSLPresentId] = 1;
    Rtos_Memcpy(pSL, pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId - 1], iMatrixSize);
  }
}

/****************************************************************************/
void AL_AVC_SelectScalingList(AL_TSps* pISPS, AL_TEncSettings const* pSettings)
{
  AL_TAvcSps* pSPS = (AL_TAvcSps*)pISPS;
  AL_EScalingList eScalingList = pSettings->eScalingList;

  assert(eScalingList != AL_SCL_MAX_ENUM);

  static const int iMaxScalingList = 12;

  if(eScalingList == AL_SCL_FLAT)
  {
    pSPS->seq_scaling_matrix_present_flag = 0;

    for(int i = 0; i < iMaxScalingList; i++)
      pSPS->seq_scaling_list_present_flag[i] = 0;

    return;
  }

  pSPS->seq_scaling_matrix_present_flag = 1;

  AL_TEncChanParam const* pChannel = &pSettings->tChParam[0];

  if(AL_IS_XAVC_VBR(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    // Scaling matrix shall be in PPS.
    // PPS scaling matrix we use the SPS's one
    pSPS->seq_scaling_matrix_present_flag = 0;
  }

  int iNb8x8SCL = AL_GET_CHROMA_MODE(pSettings->tChParam[0].ePicFormat) == AL_CHROMA_4_4_4 ? 3 : 1;

  if(eScalingList == AL_SCL_CUSTOM)
  {
    for(int iIntraInter = 0; iIntraInter < 2; ++iIntraInter)
    {
      for(int iYCbCr = 0; iYCbCr < 3; iYCbCr++)
        fillScalingList(pSettings, pSPS, 0, iIntraInter, iYCbCr);

      for(int iYCbCr = 0; iYCbCr < iNb8x8SCL; iYCbCr++)
        fillScalingList(pSettings, pSPS, 1, iIntraInter, iYCbCr);
    }
  }
  else if(eScalingList == AL_SCL_DEFAULT)
  {
    for(int i = 0; i < iMaxScalingList; i++)
      pSPS->seq_scaling_list_present_flag[i] = 0;

    for(int iIntraInter = 0; iIntraInter < 2; ++iIntraInter)
    {
      for(int iYCbCr = 0; iYCbCr < 3; iYCbCr++)
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iIntraInter) + iYCbCr], AL_AVC_DefaultScalingLists4x4[iIntraInter], 16);

      for(int iYCbCr = 0; iYCbCr < iNb8x8SCL; iYCbCr++)
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iIntraInter) + iYCbCr], AL_AVC_DefaultScalingLists8x8[iIntraInter], 64);
    }
  }
}

/****************************************************************************/
void AL_AVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, uint8_t chroma_format_idc, TBufferEP* pBufEP)
{
  AL_THwScalingList HwSclLst;

  AL_AVC_GenerateHwScalingList(pSclLst, chroma_format_idc, &HwSclLst);
  AL_AVC_WriteEncHwScalingList(pSclLst, (AL_THwScalingList const*)&HwSclLst, chroma_format_idc, pBufEP->tMD.pVirtualAddr + EP1_BUF_SCL_LST.Offset);

  pBufEP->uFlags |= EP1_BUF_SCL_LST.Flag;
}

/****************************************************************************/
static void AL_AVC_UpdateHrdParameters(AL_TAvcSps* pSPS, AL_TSubHrdParam* pSubHrdParam, int const iCpbSize, AL_TEncSettings const* pSettings)
{
  pSubHrdParam->bit_rate_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  pSPS->vui_param.hrd_param.cpb_cnt_minus1[0] = 0;
  AL_Decomposition(&(pSubHrdParam->bit_rate_value_minus1[0]), &pSPS->vui_param.hrd_param.bit_rate_scale);

  assert(pSubHrdParam->bit_rate_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->cpb_size_value_minus1[0] = iCpbSize >> 4;
  AL_Decomposition(&(pSubHrdParam->cpb_size_value_minus1[0]), &pSPS->vui_param.hrd_param.cpb_size_scale);

  assert(pSubHrdParam->cpb_size_value_minus1[0] <= (UINT32_MAX - 1));

  pSubHrdParam->cbr_flag[0] = (pSettings->tChParam[0].tRCParam.eRCMode == AL_RC_CBR) ? 1 : 0;

  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 31; // int(log((double)iCurrDelay) / log(2.0));
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 31;
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 31;
  pSPS->vui_param.hrd_param.time_offset_length = 0;
}

/****************************************************************************/
static void AL_AVC_GenerateSPS_Resolution(AL_TAvcSps* pSPS, uint16_t uWidth, uint16_t uHeight, AL_TEncSettings const* pSettings)
{
  uint8_t uLog2MaxCuSize = pSettings->tChParam->uLog2MaxCuSize;

  int iMBWidth = ROUND_UP_POWER_OF_TWO(uWidth, uLog2MaxCuSize);
  int iMBHeight = ROUND_UP_POWER_OF_TWO(uHeight, uLog2MaxCuSize);

  int iCropLeft = 0;
  int iCropTop = 0;

  iCropLeft = pSettings->tChParam->uOutputCropPosX;
  iCropTop = pSettings->tChParam->uOutputCropPosY;

  if(pSettings->tChParam->uOutputCropWidth)
    uWidth = pSettings->tChParam->uOutputCropWidth;

  if(pSettings->tChParam->uOutputCropHeight)
    uHeight = pSettings->tChParam->uOutputCropHeight;

  int iCropRight = (iMBWidth << uLog2MaxCuSize) - (iCropLeft + uWidth);
  int iCropBottom = (iMBHeight << uLog2MaxCuSize) - (iCropTop + uHeight);

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pSettings->tChParam->ePicFormat);

  int iCropUnitX = eChromaMode == AL_CHROMA_4_2_0 || eChromaMode == AL_CHROMA_4_2_2 ? 2 : 1;
  int iCropUnitY = eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;

  pSPS->pic_width_in_mbs_minus1 = iMBWidth - 1;

  // When frame_mbs_only_flag == 0, height in MB is always counted for a *field* picture,
  // even if we are encoding frame pictures
  // (see spec sec.7.4.2.1 and eq.7-15)
  pSPS->pic_height_in_map_units_minus1 = iMBHeight - 1;

  pSPS->frame_crop_left_offset = iCropLeft / iCropUnitX;
  pSPS->frame_crop_right_offset = iCropRight / iCropUnitX;
  pSPS->frame_crop_top_offset = iCropTop / iCropUnitY;
  pSPS->frame_crop_bottom_offset = iCropBottom / iCropUnitY;
  pSPS->frame_cropping_flag = (pSPS->frame_crop_left_offset || pSPS->frame_crop_right_offset
                               || pSPS->frame_crop_top_offset || pSPS->frame_crop_bottom_offset) ? 1 : 0;

  AL_TEncChanParam const* pChannel = &pSettings->tChParam[0];

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
    AL_UpdateSarAspectRatio(&pSPS->vui_param, uWidth, uHeight, pSettings->eAspectRatio);
  else
  {
    AL_UpdateAspectRatio(&pSPS->vui_param, uWidth, uHeight, pSettings->eAspectRatio);
  }
}

/****************************************************************************/
void AL_AVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, int iMaxRef, int iCpbSize)
{
  AL_TAvcSps* pSPS = (AL_TAvcSps*)pISPS;
  AL_TEncChanParam const* pChannel = &pSettings->tChParam[0];

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChannel->ePicFormat);

  uint32_t uCSFlags = AL_GET_CS_FLAGS(pChannel->eProfile);

  // --------------------------------------------------------------------------
  pSPS->profile_idc = AL_GET_PROFILE_IDC(pChannel->eProfile);

  pSPS->constraint_set0_flag = (uCSFlags) & 1;
  pSPS->constraint_set1_flag = (uCSFlags >> 1) & 1;
  pSPS->constraint_set2_flag = (uCSFlags >> 2) & 1;
  pSPS->constraint_set3_flag = (uCSFlags >> 3) & 1;
  pSPS->constraint_set4_flag = (uCSFlags >> 4) & 1;
  pSPS->constraint_set5_flag = (uCSFlags >> 5) & 1;
  pSPS->chroma_format_idc = eChromaMode;
  pSPS->separate_colour_plane_flag = 0;
  pSPS->bit_depth_luma_minus8 = AL_GET_BITDEPTH_LUMA(pChannel->ePicFormat) - 8;
  pSPS->bit_depth_chroma_minus8 = AL_GET_BITDEPTH_CHROMA(pChannel->ePicFormat) - 8;
  pSPS->qpprime_y_zero_transform_bypass_flag = 0;

  AL_AVC_SelectScalingList(pISPS, pSettings);

  pSPS->level_idc = pChannel->uLevel;
  pSPS->seq_parameter_set_id = 0;
  pSPS->pic_order_cnt_type = 0; // TDMB = 2;

  pSPS->max_num_ref_frames = iMaxRef;
  pSPS->gaps_in_frame_num_value_allowed_flag = 0;
  pSPS->log2_max_pic_order_cnt_lsb_minus4 = AL_GET_SPS_LOG2_MAX_POC(pChannel->uSpsParam) - 4;

  if(AL_IS_XAVC(pChannel->eProfile))
    pSPS->log2_max_pic_order_cnt_lsb_minus4 = 0;

  pSPS->log2_max_frame_num_minus4 = 0;

  if((pChannel->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && pChannel->tGopParam.uNumB == 15)
    pSPS->log2_max_frame_num_minus4 = 1;

  else if(AL_IsGdrEnabled(pSettings))
    pSPS->log2_max_frame_num_minus4 = 6; // 6 is to support AVC 8K GDR

  // frame_mbs_only_flag:
  // - is set to 0 whenever possible (we allow field pictures).
  // - must be set to 1 in Baseline (sec. A.2.1), or for certain levels (Table A-4).

  // m_SPS.frame_mbs_only_flag = ((cp.iProfile == 66) || (cp.iLevel <= 20) || (cp.iLevel >= 42)) ? 1 : 0;
  pSPS->frame_mbs_only_flag = 1;

  // direct_8x8_inference_flag:
  // - is set to 1 whenever possible.
  // - must be set to 1 when level >= 3.0 (Table A-4), or when frame_mbs_only_flag == 0 (sec. 7.4.2.1).
  pSPS->direct_8x8_inference_flag = 1;

  pSPS->mb_adaptive_frame_field_flag = 0;

  pSPS->vui_parameters_present_flag = 1;
#if defined(ANDROID) || defined(__ANDROID_API__)
  pSPS->vui_parameters_present_flag = 0;
#endif

  pSPS->vui_param.chroma_loc_info_present_flag = (eChromaMode == AL_CHROMA_4_2_0) ? 1 : 0;

  if(AL_IS_XAVC(pChannel->eProfile))
    pSPS->vui_param.chroma_loc_info_present_flag = 0;
  pSPS->vui_param.chroma_sample_loc_type_top_field = 0;
  pSPS->vui_param.chroma_sample_loc_type_bottom_field = 0;

  AL_AVC_GenerateSPS_Resolution(pSPS, pChannel->uEncWidth, pChannel->uEncHeight, pSettings);

  pSPS->vui_param.overscan_info_present_flag = 0;

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile) && (pSPS->pic_width_in_mbs_minus1 <= 119) && (pSPS->pic_height_in_map_units_minus1 <= 67))
  {
    pSPS->vui_param.overscan_info_present_flag = 1;
    pSPS->vui_param.overscan_appropriate_flag = 1;
  }

  pSPS->vui_param.video_signal_type_present_flag = 1;

  pSPS->vui_param.video_format = VIDEO_FORMAT_UNSPECIFIED;

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
    pSPS->vui_param.video_format = VIDEO_FORMAT_COMPONENT;

  pSPS->vui_param.video_full_range_flag = 0;

  // Colour parameter information
  pSPS->vui_param.colour_description_present_flag = 1;
  pSPS->vui_param.colour_primaries = AL_H273_ColourDescToColourPrimaries(pSettings->eColourDescription);
  pSPS->vui_param.transfer_characteristics = pSettings->eTransferCharacteristics;
  pSPS->vui_param.matrix_coefficients = pSettings->eColourMatrixCoeffs;

  // Timing information
  AL_UpdateVuiTimingInfo(&pSPS->vui_param, 0, &pSettings->tChParam[0].tRCParam, 2);

  pSPS->vui_param.fixed_frame_rate_flag = 0;

  if(AL_IS_XAVC(pChannel->eProfile))
    pSPS->vui_param.fixed_frame_rate_flag = 1;

  // NAL HRD
  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;

  if(AL_IS_XAVC(pChannel->eProfile) && !(AL_IS_INTRA_PROFILE(pChannel->eProfile)))
  {
    pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 1;
    AL_AVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.nal_sub_hrd_param), iCpbSize, pSettings);
  }

  // VCL HRD
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 1;

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
    pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 0;

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
    AL_AVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.vcl_sub_hrd_param), iCpbSize, pSettings);

  // low Delay
  pSPS->vui_param.hrd_param.low_delay_hrd_flag[0] = 0;

  // Picture structure information
  pSPS->vui_param.pic_struct_present_flag = 1;

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
    pSPS->vui_param.pic_struct_present_flag = 0;

  pSPS->vui_param.bitstream_restriction_flag = 0;

  // MVC Extension
}

/****************************************************************************/
void AL_AVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TSps const* pSPS)
{
  AL_TAvcPps* pPPS = (AL_TAvcPps*)pIPPS;
  AL_TEncChanParam const* pChannel = &pSettings->tChParam[0];

  pPPS->pic_parameter_set_id = 0;
  pPPS->seq_parameter_set_id = 0;

  pPPS->entropy_coding_mode_flag = (pChannel->eEntropyMode == AL_MODE_CABAC) ? 1 : 0;
  pPPS->bottom_field_pic_order_in_frame_present_flag = 0;

  pPPS->num_slice_groups_minus1 = 0;
  int iNumRef = 1;

  pPPS->num_ref_idx_l0_active_minus1 = iNumRef - 1;
  pPPS->num_ref_idx_l1_active_minus1 = iNumRef - 1;

  pPPS->weighted_pred_flag = (pChannel->eWPMode == AL_WP_EXPLICIT) ? 1 : 0;
  pPPS->weighted_bipred_idc = pChannel->eWPMode;

  pPPS->pic_init_qp_minus26 = 0;
  pPPS->pic_init_qs_minus26 = 0;
  pPPS->chroma_qp_index_offset = Clip3(pChannel->iCbPicQpOffset, -12, 12);
  pPPS->second_chroma_qp_index_offset = Clip3(pChannel->iCrPicQpOffset, -12, 12);

  pPPS->deblocking_filter_control_present_flag = 1; // TDMB = 0;

  pPPS->constrained_intra_pred_flag = (pChannel->eEncTools & AL_OPT_CONST_INTRA_PRED) ? 1 : 0;
  pPPS->redundant_pic_cnt_present_flag = 0;
  pPPS->transform_8x8_mode_flag = pChannel->uLog2MaxTuSize > 2 ? 1 : 0;
  pPPS->pic_scaling_matrix_present_flag = 0;

  if(AL_IS_XAVC_VBR(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    // Scaling matrix are disabled in SPS but enabled in PPS.
    // PPS scaling matrix use SPS's one
    AL_EScalingList eScalingList = pSettings->eScalingList;

    if(eScalingList != AL_SCL_FLAT)
      pPPS->pic_scaling_matrix_present_flag = 1;
  }
  pPPS->pSPS = (AL_TAvcSps*)pSPS;
}

/***************************************************************************/
void AL_AVC_UpdateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo)
{
  if(!pHLSInfo->bResolutionChanged)
    return;

  AL_TAvcSps* pSPS = (AL_TAvcSps*)pISPS;

  AL_TBuffer* pBuf = AL_GetSrcBufferFromStatus(pPicStatus);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  AL_AVC_GenerateSPS_Resolution(pSPS, tDim.iWidth, tDim.iHeight, pSettings);

  pSPS->seq_parameter_set_id = pHLSInfo->uNalID;
}

/***************************************************************************/
bool AL_AVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo)
{
  (void)pHLSInfo;
  AL_TAvcPps* pPPS = (AL_TAvcPps*)pIPPS;
  bool bMustWritePPS = false;

  pPPS->pic_init_qp_minus26 = pPicStatus->iPpsQP - 26;

  if(pHLSInfo->bResolutionChanged)
  {
    pPPS->pic_parameter_set_id = pHLSInfo->uNalID;
    pPPS->seq_parameter_set_id = pHLSInfo->uNalID;
    bMustWritePPS = true;
  }

  return bMustWritePPS;
}
