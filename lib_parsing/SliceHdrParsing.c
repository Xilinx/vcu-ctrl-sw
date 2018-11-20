/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <assert.h>

#include "lib_rtos/lib_rtos.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Utils.h"

#include "SliceHdrParsing.h"
#include "HevcParser.h"

static const int AVC_SLICE_TYPE[5] =
{
  1, 0, 2, 3, 4
};

static void skipAllZerosAndTheNextByte(AL_TRbspParser* pRP)
{
  while(u(pRP, 8) == 0x00)
    ;
}

/***************************************************************************/
/*   A V C   S L I C E   H E A D E R   P A R S I N G   f u n c t i o n s   */
/***************************************************************************/

/*****************************************************************************/
static void AL_AVC_sReadWPCoeff(AL_TRbspParser* pRP, AL_TAvcSliceHdr* pSlice, uint8_t uL0L1)
{
  uint8_t uNumRefIdx = uL0L1 ? pSlice->num_ref_idx_l1_active_minus1 : pSlice->num_ref_idx_l0_active_minus1;
  AL_TWPCoeff* pWpCoeff = &pSlice->pred_weight_table.tWpCoeff[uL0L1];

  for(uint8_t i = 0; i <= uNumRefIdx; i++)
  {
    pWpCoeff->luma_weight_flag[i] = u(pRP, 1);

    if(pWpCoeff->luma_weight_flag[i])
    {
      pWpCoeff->luma_delta_weight[i] = Clip3(se(pRP), AL_MIN_WP_LUMA_PARAM, AL_MAX_WP_LUMA_PARAM);
      pWpCoeff->luma_offset[i] = Clip3(se(pRP), AL_MIN_WP_LUMA_PARAM, AL_MAX_WP_LUMA_PARAM);
    }
    else
    {
      pWpCoeff->luma_delta_weight[i] = 0;
      pWpCoeff->luma_offset[i] = 0;
    }

    if(pSlice->pSPS->chroma_format_idc != 0)
    {
      pWpCoeff->chroma_weight_flag[i] = u(pRP, 1);

      if(pWpCoeff->chroma_weight_flag[i])
      {
        pWpCoeff->chroma_delta_weight[i][0] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
        pWpCoeff->chroma_offset[i][0] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
        pWpCoeff->chroma_delta_weight[i][1] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
        pWpCoeff->chroma_offset[i][1] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
      }
      else
      {
        pWpCoeff->chroma_delta_weight[i][0] = 0;
        pWpCoeff->chroma_offset[i][0] = 0;
        pWpCoeff->chroma_delta_weight[i][1] = 0;
        pWpCoeff->chroma_offset[i][1] = 0;
      }
    }
  }
}

/*****************************************************************************/
static void AL_AVC_spred_weight_table(AL_TRbspParser* pRP, AL_TAvcSliceHdr* pSlice)
{
  pSlice->pred_weight_table.luma_log2_weight_denom = Clip3(ue(pRP), 0, AL_MAX_WP_DENOM);

  if(pSlice->pSPS->chroma_format_idc != 0)
    pSlice->pred_weight_table.chroma_log2_weight_denom = Clip3(ue(pRP), 0, AL_MAX_WP_DENOM);

  AL_AVC_sReadWPCoeff(pRP, pSlice, 0);

  if(pSlice->slice_type == SLICE_B)
    AL_AVC_sReadWPCoeff(pRP, pSlice, 1);
}

/*************************************************************************//*!
   \brief the ref_pic_list_modification function parses the reordering syntax elements from a Slice Header NAL
   \param[in]  pRP             Pointer to NAL buffer
   \param[out] pSlice          Pointer to the slice header structure that will be filled
   \param[in]  NumPocTotalCurr Number of pictures available as reference for the current picture
*****************************************************************************/
static void AL_AVC_sref_pic_list_reordering(AL_TRbspParser* pRP, AL_TAvcSliceHdr* pSlice)
{
  if(pSlice->slice_type != SLICE_I)
  {
    int idx1 = -1;
    int idx2 = -1;
    int idx3 = -1;

    pSlice->ref_pic_list_reordering_flag_l0 = u(pRP, 1);

    if(pSlice->ref_pic_list_reordering_flag_l0)
      do
      {
        ++idx1;
        pSlice->reordering_of_pic_nums_idc_l0[idx1] = Clip3(ue(pRP), 0, AL_MAX_REORDER_IDC);

        if(pSlice->reordering_of_pic_nums_idc_l0[idx1] == 0
           || pSlice->reordering_of_pic_nums_idc_l0[idx1] == 1)
        {
          ++idx2;
          pSlice->abs_diff_pic_num_minus1_l0[idx2] = ue(pRP);
        }
        else if(pSlice->reordering_of_pic_nums_idc_l0[idx1] == 2)
        {
          ++idx3;
          pSlice->long_term_pic_num_l0[idx3] = ue(pRP);
        }
      }
      while(idx1 < AL_MAX_REFERENCE_PICTURE_REORDER && pSlice->reordering_of_pic_nums_idc_l0[idx1] != 3);
  }

  if(pSlice->slice_type == SLICE_B)
  {
    int idx1 = -1;
    int idx2 = -1;
    int idx3 = -1;

    pSlice->ref_pic_list_reordering_flag_l1 = u(pRP, 1);

    if(pSlice->ref_pic_list_reordering_flag_l1)
      do
      {
        ++idx1;
        pSlice->reordering_of_pic_nums_idc_l1[idx1] = Clip3(ue(pRP), 0, AL_MAX_REORDER_IDC);

        if(pSlice->reordering_of_pic_nums_idc_l1[idx1] == 0
           || pSlice->reordering_of_pic_nums_idc_l1[idx1] == 1)
        {
          ++idx2;
          pSlice->abs_diff_pic_num_minus1_l1[idx2] = ue(pRP);
        }
        else if(pSlice->reordering_of_pic_nums_idc_l1[idx1] == 2)
        {
          ++idx3;
          pSlice->long_term_pic_num_l1[idx3] = ue(pRP);
        }
      }
      while(idx1 < AL_MAX_REFERENCE_PICTURE_REORDER && pSlice->reordering_of_pic_nums_idc_l1[idx1] != 3);
  }
}

/*****************************************************************************/
static void AL_AVC_sdec_ref_pic_marking(AL_TRbspParser* pRP, AL_TAvcSliceHdr* pSlice)
{
  if(pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR)
  {
    pSlice->no_output_of_prior_pics_flag = u(pRP, 1);
    pSlice->long_term_reference_flag = u(pRP, 1);
    return;
  }

  pSlice->adaptive_ref_pic_marking_mode_flag = u(pRP, 1);

  if(!pSlice->adaptive_ref_pic_marking_mode_flag)
    return;

  int idx1 = 0;
  int idx2 = 0;
  int idx3 = 0;
  int idx4 = 0;
  int idx5 = 0;

  int op;

  do
  {
    op = ue(pRP);

    pSlice->memory_management_control_operation[idx1] = op;
    ++idx1;

    if(op == 1 || op == 3)
    {
      pSlice->difference_of_pic_nums_minus1[idx2] = ue(pRP);
      ++idx2;
    }

    if(op == 2)
    {
      pSlice->long_term_pic_num[idx3] = ue(pRP);
      ++idx3;
    }

    if(op == 3 || op == 6)
    {
      pSlice->long_term_frame_idx[idx4] = ue(pRP);
      ++idx4;
    }

    if(op == 4)
    {
      pSlice->max_long_term_frame_idx_plus1[idx5] = ue(pRP);
      ++idx5;
    }
  }
  while(op != 0);
}

/*****************************************************************************/
static bool ApplyAvcSPSAndReturn(AL_TAvcSliceHdr* pSlice, AL_TAvcPps const* pPPS)
{
  pSlice->pPPS = pPPS;
  pSlice->pSPS = pSlice->pPPS->pSPS;
  return false;
}

/*****************************************************************************/
static void setAvcSliceHeaderDefaultValues(AL_TAvcSliceHdr* pSlice)
{
  pSlice->field_pic_flag = 0;
  pSlice->redundant_pic_cnt = 0;
  pSlice->bottom_field_flag = 0;
  pSlice->num_ref_idx_l0_active_minus1 = 0;
  pSlice->num_ref_idx_l1_active_minus1 = 0;
  pSlice->delta_pic_order_cnt_bottom = 0;
  pSlice->disable_deblocking_filter_idc = 0;
  pSlice->slice_alpha_c0_offset_div2 = 0;
  pSlice->slice_beta_offset_div2 = 0;
  pSlice->direct_spatial_mv_pred_flag = 0;
}

/*****************************************************************************/
bool AL_AVC_ParseSliceHeader(AL_TAvcSliceHdr* pSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_TAvcPps pPPSTable[])
{
  if(!pConceal->bHasPPS)
    return false;

  setAvcSliceHeaderDefaultValues(pSlice);

  skipAllZerosAndTheNextByte(pRP);

  // Read NUT
  u(pRP, 1);
  pSlice->nal_ref_idc = u(pRP, 2);
  pSlice->nal_unit_type = u(pRP, 5);
  pSlice->first_mb_in_slice = ue(pRP);
  pSlice->slice_type = ue(pRP);

  int const currentPPSId = ue(pRP);

  AL_TAvcPps const* pFallbackPps = &pPPSTable[pConceal->iLastPPSId];

  if(currentPPSId > AL_AVC_MAX_PPS || pPPSTable[currentPPSId].bConceal)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  pSlice->pic_parameter_set_id = currentPPSId;

  int const MaxNumMb = (pPPSTable[currentPPSId].pSPS->pic_height_in_map_units_minus1 + 1) * (pPPSTable[currentPPSId].pSPS->pic_width_in_mbs_minus1 + 1);

  if(pSlice->first_mb_in_slice >= MaxNumMb)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  if(pSlice->slice_type > AL_AVC_MAX_SLICE_TYPE)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  pSlice->slice_type %= 5;
  pSlice->slice_type = AVC_SLICE_TYPE[pSlice->slice_type];

  // check slice_type coherency
  if((pSlice->slice_type > AL_AVC_MAX_SLICE_TYPE) || (pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR && pSlice->slice_type != SLICE_I && pSlice->slice_type != SLICE_SI))
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  if(pConceal->bValidFrame && (pSlice->pic_parameter_set_id != pConceal->iActivePPS))
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  if(!pConceal->bValidFrame)
    pConceal->iActivePPS = pSlice->pic_parameter_set_id;

  // check first MB offset coherency
  if(pSlice->first_mb_in_slice <= pConceal->iFirstLCU)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  pSlice->slice_type %= 5;

  // check slice_type coherency
  if(pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR && pSlice->slice_type != SLICE_I)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  // select the pps & sps for the current picture
  AL_TAvcPps const* pPps = pSlice->pPPS = &pPPSTable[currentPPSId];

  if(pPps->bConceal)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  AL_TAvcSps const* pSps = pSlice->pSPS = pPps->pSPS;

  if(pSps->bConceal)
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  int const iFrameNumSize = pSps->log2_max_frame_num_minus4 + 4;

  pSlice->frame_num = u(pRP, iFrameNumSize);

  if(!pSps->frame_mbs_only_flag)
  {
    pSlice->field_pic_flag = u(pRP, 1);

    if(pSlice->field_pic_flag)
      pSlice->bottom_field_flag = u(pRP, 1);
  }

  if(pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR)
    pSlice->idr_pic_id = Clip3(ue(pRP), 0, AL_MAX_IDR_PIC_ID);

  if(pSps->pic_order_cnt_type == 0)
  {
    int PicOrderCntSize = pSps->log2_max_pic_order_cnt_lsb_minus4 + 4;
    pSlice->pic_order_cnt_lsb = u(pRP, PicOrderCntSize);

    if(pPps->bottom_field_pic_order_in_frame_present_flag && !(pSlice->field_pic_flag))
      pSlice->delta_pic_order_cnt_bottom = se(pRP);
  }

  if(pSps->pic_order_cnt_type == 1 && !pSps->delta_pic_order_always_zero_flag)
  {
    pSlice->delta_pic_order_cnt[0] = se(pRP);

    if(pPps->bottom_field_pic_order_in_frame_present_flag && !(pSlice->field_pic_flag))
      pSlice->delta_pic_order_cnt[1] = se(pRP);
  }

  if(pPps->redundant_pic_cnt_present_flag)
    pSlice->redundant_pic_cnt = ue(pRP);

  if(pSlice->slice_type == SLICE_B)
    pSlice->direct_spatial_mv_pred_flag = u(pRP, 1);

  if(pSlice->slice_type != SLICE_I)
  {
    pSlice->num_ref_idx_active_override_flag = u(pRP, 1);

    if(pSlice->num_ref_idx_active_override_flag)
    {
      pSlice->num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);

      if(pSlice->slice_type == SLICE_B)
        pSlice->num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);
    }
    else
    {
      // infer values from ParserPPS
      pSlice->num_ref_idx_l0_active_minus1 = pPps->num_ref_idx_l0_active_minus1;
      pSlice->num_ref_idx_l1_active_minus1 = pPps->num_ref_idx_l1_active_minus1;
    }
  }

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

  AL_AVC_sref_pic_list_reordering(pRP, pSlice);

  if(
    (pPps->weighted_pred_flag && pSlice->slice_type == SLICE_P) ||
    (pPps->weighted_bipred_idc == 1 && pSlice->slice_type == SLICE_B)
    )
  {
    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
      return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

    AL_AVC_spred_weight_table(pRP, pSlice);
  }

  if(pSlice->nal_ref_idc != 0)
  {
    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
      return ApplyAvcSPSAndReturn(pSlice, pFallbackPps);

    AL_AVC_sdec_ref_pic_marking(pRP, pSlice);
  }

  if(pPps->entropy_coding_mode_flag && pSlice->slice_type != SLICE_I)
    pSlice->cabac_init_idc = Clip3(ue(pRP), 0, AL_MAX_CABAC_INIT_IDC);
  pSlice->slice_qp_delta = se(pRP);

  if(pPps->deblocking_filter_control_present_flag)
  {
    pSlice->disable_deblocking_filter_idc = ue(pRP);

    if(pSlice->disable_deblocking_filter_idc != 1)
    {
      pSlice->slice_alpha_c0_offset_div2 = se(pRP);
      pSlice->slice_beta_offset_div2 = se(pRP);
    }
  }

  if(pPps->num_slice_groups_minus1 > 0 && pPps->slice_group_map_type >= 3 && pPps->slice_group_map_type <= 5)
    assert(0);
  pConceal->iFirstLCU = pSlice->first_mb_in_slice;
  return true;
}

/***************************************************************************/
/* H E V C   S L I C E   H E A D E R   P A R S I N G   f u n c t i o n s   */
/***************************************************************************/

/*****************************************************************************/
static void AL_HEVC_sSetDefaultSliceHeader(AL_THevcSliceHdr* pSlice)
{
  uint8_t first_slice_segment_in_pic_flag = pSlice->first_slice_segment_in_pic_flag;
  uint8_t no_output_prior_pics_flag = pSlice->no_output_of_prior_pics_flag;
  uint8_t pic_parameter_set_id = pSlice->slice_pic_parameter_set_id;
  uint8_t nal_unit_type = pSlice->nal_unit_type;
  uint8_t nuh_layer_id = pSlice->nuh_layer_id;
  uint8_t temporal_id_plus1 = pSlice->temporal_id_plus1;
  uint8_t RapFlag = pSlice->RapPicFlag;
  uint8_t IdrFlag = pSlice->IdrPicFlag;
  const AL_THevcPps* pPPS = pSlice->pPPS;
  AL_THevcSps* pSPS = pSlice->pSPS;

  Rtos_Memset(pSlice, 0, sizeof(AL_THevcSliceHdr));

  pSlice->first_slice_segment_in_pic_flag = first_slice_segment_in_pic_flag;
  pSlice->no_output_of_prior_pics_flag = no_output_prior_pics_flag;
  pSlice->slice_pic_parameter_set_id = pic_parameter_set_id;
  pSlice->dependent_slice_segment_flag = 0;
  pSlice->nal_unit_type = nal_unit_type;
  pSlice->nuh_layer_id = nuh_layer_id;
  pSlice->temporal_id_plus1 = temporal_id_plus1;
  pSlice->RapPicFlag = RapFlag;
  pSlice->IdrPicFlag = IdrFlag;
  pSlice->pPPS = pPPS;
  pSlice->pSPS = pSPS;

  pSlice->pic_output_flag = 1;
  pSlice->collocated_from_l0_flag = 1;
}

/*****************************************************************************/
static void TransferRps(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pLastSlice)
{
  pSlice->NumPocStCurrBefore = pLastSlice->NumPocStCurrBefore;
  pSlice->NumPocStCurrAfter = pLastSlice->NumPocStCurrAfter;
  pSlice->NumPocLtCurr = pLastSlice->NumPocLtCurr;
  pSlice->NumPocStFoll = pLastSlice->NumPocStFoll;
  pSlice->NumPocLtFoll = pLastSlice->NumPocLtFoll;
  pSlice->NumPocTotalCurr = pLastSlice->NumPocTotalCurr;
}

/*****************************************************************************/
static void AL_HEVC_sInitSlice(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pLastSlice)
{
  *pSlice = *pLastSlice;

  pSlice->first_slice_segment_in_pic_flag = 0;
  pSlice->dependent_slice_segment_flag = 1;
}

/*************************************************************************/
static void AL_HEVC_sReadWPCoeff(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice, uint8_t uL0L1)
{
  uint8_t uNumRefIdx = uL0L1 ? pSlice->num_ref_idx_l1_active_minus1 : pSlice->num_ref_idx_l0_active_minus1;
  AL_TWPCoeff* pWpCoeff = &pSlice->pred_weight_table.tWpCoeff[uL0L1];

  for(uint8_t i = 0; i <= uNumRefIdx; i++)
    pWpCoeff->luma_weight_flag[i] = u(pRP, 1);

  if(pSlice->pSPS->ChromaArrayType)
  {
    for(uint8_t i = 0; i <= uNumRefIdx; i++)
      pWpCoeff->chroma_weight_flag[i] = u(pRP, 1);
  }

  for(uint8_t i = 0; i <= uNumRefIdx; i++)
  {
    // fast access
    int iOffsetY = pSlice->pSPS->WpOffsetHalfRangeY;
    int iOffsetC = pSlice->pSPS->WpOffsetHalfRangeC;

    // initial value
    pWpCoeff->luma_delta_weight[i] = 0;
    pWpCoeff->luma_offset[i] = 0;
    pWpCoeff->chroma_delta_weight[i][0] = 0;
    pWpCoeff->chroma_offset[i][0] = 0;
    pWpCoeff->chroma_delta_weight[i][1] = 0;
    pWpCoeff->chroma_offset[i][1] = 0;

    if(pWpCoeff->luma_weight_flag[i])
    {
      pWpCoeff->luma_delta_weight[i] = Clip3(se(pRP), AL_MIN_WP_LUMA_PARAM, AL_MAX_WP_LUMA_PARAM);
      pWpCoeff->luma_offset[i] = Clip3(se(pRP), -iOffsetY, iOffsetY - 1);
    }

    if(pWpCoeff->chroma_weight_flag[i])
    {
      int iChromaWeight;
      uint8_t uOffset = (1 << pSlice->pred_weight_table.chroma_log2_weight_denom);

      pWpCoeff->chroma_delta_weight[i][0] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
      iChromaWeight = uOffset + pWpCoeff->chroma_delta_weight[i][0];
      pWpCoeff->chroma_offset[i][0] = Clip3(Clip3(se(pRP), -4 * iOffsetC, 4 * iOffsetC - 1) + iOffsetC - ((iChromaWeight * iOffsetC) >> pSlice->pred_weight_table.chroma_log2_weight_denom), -iOffsetC, iOffsetC - 1);

      pWpCoeff->chroma_delta_weight[i][1] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
      iChromaWeight = uOffset + pWpCoeff->chroma_delta_weight[i][1];
      pWpCoeff->chroma_offset[i][1] = Clip3(Clip3(se(pRP), -4 * iOffsetC, 4 * iOffsetC - 1) + iOffsetC - ((iChromaWeight * iOffsetC) >> pSlice->pred_weight_table.chroma_log2_weight_denom), -iOffsetC, iOffsetC - 1);
    }
  }
}

/*************************************************************************//*!
                                                                           \brief the pred_weight_table function parses the weighted pred table syntax elements from a Slice Header NAL
                                                                           \param[in]  pRP    Pointer to NAL buffer
                                                                           \param[out] pSlice Pointer to the slice header structure that will be filled
*****************************************************************************/
static void AL_HEVC_spred_weight_table(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice)
{
  pSlice->pred_weight_table.luma_log2_weight_denom = Clip3(ue(pRP), 0, AL_MAX_WP_DENOM);
  pSlice->pred_weight_table.chroma_log2_weight_denom = Clip3(pSlice->pred_weight_table.luma_log2_weight_denom + (pSlice->pSPS->ChromaArrayType ? se(pRP) : 0), 0, AL_MAX_WP_DENOM);

  AL_HEVC_sReadWPCoeff(pRP, pSlice, 0);

  if(pSlice->slice_type == SLICE_B)
    AL_HEVC_sReadWPCoeff(pRP, pSlice, 1);
}

/*************************************************************************//*!
                                                                           \brief This function parses the reordering syntax elements from a Slice Header NAL
                                                                           \param[in]  pRP             Pointer to NAL buffer
                                                                           \param[out] pSlice          Pointer to the slice header structure that will be filled
                                                                           \param[in]  NumPocTotalCurr Number of pictures available as reference for the current picture
*****************************************************************************/
static void AL_HEVC_sref_pic_list_modification(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice, uint8_t NumPocTotalCurr)
{
  uint8_t list_entry_size = ceil_log2(NumPocTotalCurr);

  // default values
  pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 = 0;
  pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1 = 0;

  for(uint8_t i = 0; i < MAX_REF; ++i)
  {
    pSlice->ref_pic_modif.list_entry_l0[i] = 0;
    pSlice->ref_pic_modif.list_entry_l1[i] = 0;
  }

  if(pSlice->slice_type != SLICE_I)
  {
    pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 = u(pRP, 1);

    if(pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0)
    {
      for(uint8_t i = 0; i <= pSlice->num_ref_idx_l0_active_minus1; ++i)
        pSlice->ref_pic_modif.list_entry_l0[i] = u(pRP, list_entry_size);
    }

    if(pSlice->slice_type == SLICE_B)
    {
      pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1 = u(pRP, 1);

      if(pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1)
      {
        for(uint8_t i = 0; i <= pSlice->num_ref_idx_l1_active_minus1; ++i)
          pSlice->ref_pic_modif.list_entry_l1[i] = u(pRP, list_entry_size);
      }
    }
  }
}

/*****************************************************************************/
void InitRefPictSet(AL_THevcSliceHdr* pSlice)
{
  pSlice->NumPocStCurrBefore = 0;
  pSlice->NumPocStCurrAfter = 0;
  pSlice->NumPocStFoll = 0;
  pSlice->NumPocLtCurr = 0;
  pSlice->NumPocLtFoll = 0;

  // Fill the five lists of picture order count values
  if(!AL_HEVC_IsIDR(pSlice->nal_unit_type))
  {
    AL_THevcSps* pSPS = pSlice->pSPS;
    uint8_t StRpsIdx = pSlice->short_term_ref_pic_set_sps_flag ? pSlice->short_term_ref_pic_set_idx :
                       pSPS->num_short_term_ref_pic_sets;

    // compute short term reference picture variables
    for(uint8_t i = 0; i < pSPS->NumNegativePics[StRpsIdx]; ++i)
    {
      if(pSPS->UsedByCurrPicS0[StRpsIdx][i])
        ++pSlice->NumPocStCurrBefore;
      else
        ++pSlice->NumPocStFoll;
    }

    for(uint8_t i = 0; i < pSPS->NumPositivePics[StRpsIdx]; ++i)
    {
      if(pSPS->UsedByCurrPicS1[StRpsIdx][i])
        ++pSlice->NumPocStCurrAfter;
      else
        ++pSlice->NumPocStFoll;
    }

    // compute long term reference picture variables
    for(uint8_t i = 0; i < pSlice->num_long_term_sps + pSlice->num_long_term_pics; ++i)
    {
      if(pSlice->UsedByCurrPicLt[i])
        ++pSlice->NumPocLtCurr;
      else
        ++pSlice->NumPocLtFoll;
    }
  }

  pSlice->NumPocTotalCurr = pSlice->NumPocStCurrBefore + pSlice->NumPocStCurrAfter + pSlice->NumPocLtCurr;
}

static bool noValidPpsHasEverBeenParsed(AL_TConceal* pConceal)
{
  return pConceal->iLastPPSId == -1;
}

/*****************************************************************************/
static bool isHevcIDR(AL_ENut eNUT)
{
  return eNUT == AL_HEVC_NUT_IDR_W_RADL || eNUT == AL_HEVC_NUT_IDR_N_LP;
}

/*****************************************************************************/
bool AL_HEVC_ParseSliceHeader(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pIndSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_THevcPps pPPSTable[])
{
  if(noValidPpsHasEverBeenParsed(pConceal))
    return false;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 1); // forbidden_zero_bit
  pSlice->nal_unit_type = u(pRP, 6);
  pSlice->nuh_layer_id = u(pRP, 6);
  pSlice->temporal_id_plus1 = u(pRP, 3);

  pSlice->RapPicFlag = (pSlice->nal_unit_type >= AL_HEVC_NUT_BLA_W_LP && pSlice->nal_unit_type <= AL_HEVC_NUT_RSV_IRAP_VCL23) ? 1 : 0;
  pSlice->IdrPicFlag = isHevcIDR(pSlice->nal_unit_type) ? 1 : 0;

  if(!more_rbsp_data(pRP))
    return false;

  pSlice->first_slice_segment_in_pic_flag = u(pRP, 1);

  if(pSlice->RapPicFlag)
    pSlice->no_output_of_prior_pics_flag = u(pRP, 1);

  pSlice->slice_pic_parameter_set_id = ue(pRP);

  if(!pConceal->bValidFrame)
    pConceal->iActivePPS = pSlice->slice_pic_parameter_set_id;

  if(pSlice->first_slice_segment_in_pic_flag)
  {
    AL_HEVC_sSetDefaultSliceHeader(pSlice);
    pSlice->slice_segment_address = 0;
  }

  /* pps_id is invalid */
  if(pSlice->slice_pic_parameter_set_id > pConceal->iLastPPSId ||
     pSlice->slice_pic_parameter_set_id >= AL_HEVC_MAX_PPS ||
     pSlice->slice_pic_parameter_set_id != pConceal->iActivePPS)
  {
    if(pConceal->iLastPPSId >= 0)
    {
      pSlice->pPPS = &pPPSTable[pConceal->iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
    }
    return false;
  }

  // assign corresponding pps and sps
  pSlice->pPPS = &pPPSTable[pSlice->slice_pic_parameter_set_id];
  const AL_THevcPps* pPps = pSlice->pPPS;

  if(pPps->bConceal || pPps->pSPS->bConceal) // invalid slice header
    return false;

  pSlice->pSPS = pPps->pSPS;
  const AL_THevcSps* pSps = pSlice->pSPS;

  if(!pSlice->first_slice_segment_in_pic_flag)
  {
    uint32_t uMaxLcu = pSps->PicWidthInCtbs * pSps->PicHeightInCtbs;
    pSlice->dependent_slice_segment_flag = pPps->dependent_slice_segments_enabled_flag ? u(pRP, 1) : 0;

    if(pSlice->dependent_slice_segment_flag)
      AL_HEVC_sInitSlice(pSlice, pIndSlice);
    else
      AL_HEVC_sSetDefaultSliceHeader(pSlice);
    TransferRps(pSlice, pIndSlice);

    int syntax_size = ceil_log2(uMaxLcu);
    pSlice->slice_segment_address = Clip3(u(pRP, syntax_size), 1, uMaxLcu - 1);

    if(!pSlice->slice_segment_address)
    {
      ++pSlice->slice_segment_address;
      return false;
    }
  }

  if(pSlice->slice_segment_address <= pConceal->iFirstLCU && !pPps->tiles_enabled_flag && !pPps->entropy_coding_sync_enabled_flag)
    return false;

  if(!pConceal->bValidFrame && pSlice->slice_segment_address)
  {
    if(pSlice->dependent_slice_segment_flag)
      return false;
  }

  if(!pSlice->dependent_slice_segment_flag)
  {
    // header default values from associated pps
    pSlice->num_ref_idx_l0_active_minus1 = pPps->num_ref_idx_l0_default_active_minus1;
    pSlice->num_ref_idx_l1_active_minus1 = pPps->num_ref_idx_l1_default_active_minus1;
    pSlice->slice_deblocking_filter_disabled_flag = pPps->pps_deblocking_filter_disabled_flag;
    pSlice->slice_loop_filter_across_slices_enabled_flag = pPps->loop_filter_across_slices_enabled_flag;
    pSlice->slice_beta_offset_div2 = pPps->pps_beta_offset_div2;
    pSlice->slice_tc_offset_div2 = pPps->pps_tc_offset_div2;

    skip(pRP, pPps->num_extra_slice_header_bits); // slice_reserved_flag
    pSlice->slice_type = ue(pRP);

    // check slice_type coherency
    if((pSlice->slice_type > SLICE_I) || (pSlice->IdrPicFlag && pSlice->slice_type != SLICE_I))
      return false;

    if(pPps->output_flag_present_flag)
      pSlice->pic_output_flag = u(pRP, 1);
    else
      pSlice->pic_output_flag = 1;

    if(pSps->separate_colour_plane_flag)
      pSlice->colour_plane_id = u(pRP, 2);

    if(!pSlice->IdrPicFlag)
    {
      int syntax_size = pSps->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
      pSlice->slice_pic_order_cnt_lsb = u(pRP, syntax_size);

      pSlice->short_term_ref_pic_set_sps_flag = u(pRP, 1);

      // check if NAL isn't empty
      if(!more_rbsp_data(pRP))
        return false;

      if(!pSlice->short_term_ref_pic_set_sps_flag)
        AL_HEVC_short_term_ref_pic_set(pSlice->pSPS, pSps->num_short_term_ref_pic_sets, pRP);
      else if(pSps->num_short_term_ref_pic_sets > 1)
      {
        int syntax_size = ceil_log2(pSps->num_short_term_ref_pic_sets);
        pSlice->short_term_ref_pic_set_idx = u(pRP, syntax_size);
        pSlice->pSPS->short_term_ref_pic_set[64] = pSlice->pSPS->short_term_ref_pic_set[pSlice->short_term_ref_pic_set_idx];
      }

      if(pSps->long_term_ref_pics_present_flag)
      {
        if(pSps->num_long_term_ref_pics_sps > 0)
          pSlice->num_long_term_sps = ue(pRP);
        pSlice->num_long_term_pics = ue(pRP);

        for(int i = 0; i < pSlice->num_long_term_sps + pSlice->num_long_term_pics; ++i)
        {
          if(i < pSlice->num_long_term_sps)
          {
            if(pSps->num_long_term_ref_pics_sps > 1)
            {
              int syntax_size = ceil_log2(pSps->num_long_term_ref_pics_sps);
              pSlice->lt_idx_sps[i] = u(pRP, syntax_size);
            }

            // compute long term variables
            pSlice->PocLsbLt[i] = pSps->lt_ref_pic_poc_lsb_sps[pSlice->lt_idx_sps[i]];
            pSlice->UsedByCurrPicLt[i] = pSps->used_by_curr_pic_lt_sps_flag[pSlice->lt_idx_sps[i]];
          }
          else
          {
            int syntax_size = pSps->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
            pSlice->poc_lsb_lt[i] = u(pRP, syntax_size);
            pSlice->used_by_curr_pic_lt_flag[i] = u(pRP, 1);

            // compute long term variables
            pSlice->PocLsbLt[i] = pSlice->poc_lsb_lt[i];
            pSlice->UsedByCurrPicLt[i] = pSlice->used_by_curr_pic_lt_flag[i];
          }

          pSlice->delta_poc_msb_present_flag[i] = u(pRP, 1);

          if(pSlice->delta_poc_msb_present_flag[i])
            pSlice->delta_poc_msb_cycle_lt[i] = ue(pRP);

          if(!i || i == pSlice->num_long_term_sps)
            pSlice->DeltaPocMSBCycleLt[i] = pSlice->delta_poc_msb_cycle_lt[i];
          else
            pSlice->DeltaPocMSBCycleLt[i] = pSlice->delta_poc_msb_cycle_lt[i] + pSlice->DeltaPocMSBCycleLt[i - 1];
        }
      }

      if(pSps->sps_temporal_mvp_enabled_flag)
        pSlice->slice_temporal_mvp_enable_flag = u(pRP, 1);
    }

    if(!pConceal->bValidFrame)
      InitRefPictSet(pSlice);

    if(pSps->sample_adaptive_offset_enabled_flag)
    {
      pSlice->slice_sao_luma_flag = u(pRP, 1);

      if(pSlice->pSPS->ChromaArrayType)
        pSlice->slice_sao_chroma_flag = u(pRP, 1);
    }

    if(pSlice->slice_type != SLICE_I)
    {
      pSlice->num_ref_idx_active_override_flag = u(pRP, 1);

      if(pSlice->num_ref_idx_active_override_flag)
      {
        pSlice->num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

        if(pSlice->slice_type == SLICE_B)
          pSlice->num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
      }

      // check if NAL isn't empty
      if(!more_rbsp_data(pRP))
        return false;

      if(pPps->lists_modification_present_flag && pSlice->NumPocTotalCurr > 1)
        AL_HEVC_sref_pic_list_modification(pRP, pSlice, pSlice->NumPocTotalCurr);

      if(pSlice->slice_type == SLICE_B)
        pSlice->mvd_l1_zero_flag = u(pRP, 1);

      if(pPps->cabac_init_present_flag)
        pSlice->cabac_init_flag = u(pRP, 1);

      if(pSlice->slice_temporal_mvp_enable_flag)
      {
        if(pSlice->slice_type == SLICE_B)
          pSlice->collocated_from_l0_flag = u(pRP, 1);

        if(pSlice->slice_type != SLICE_I)
        {
          if(pSlice->collocated_from_l0_flag && pSlice->num_ref_idx_l0_active_minus1 > 0)
            pSlice->collocated_ref_idx = Clip3(ue(pRP), 0, pSlice->num_ref_idx_l0_active_minus1);
          else if(!pSlice->collocated_from_l0_flag && pSlice->num_ref_idx_l1_active_minus1 > 0)
            pSlice->collocated_ref_idx = Clip3(ue(pRP), 0, pSlice->num_ref_idx_l1_active_minus1);
        }
      }

      // check if NAL isn't empty
      if(!more_rbsp_data(pRP))
        return false;

      if((pPps->weighted_pred_flag && pSlice->slice_type == SLICE_P) ||
         (pPps->weighted_bipred_flag && pSlice->slice_type == SLICE_B))
        AL_HEVC_spred_weight_table(pRP, pSlice);

      pSlice->five_minus_max_num_merge_cand = ue(pRP);

      if(pSlice->five_minus_max_num_merge_cand > 4)
        return false;
    }

    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
      return false;

    int iQpBdOffset = 6 * pSps->bit_depth_luma_minus8;
    pSlice->slice_qp_delta = Clip3(se(pRP), -26 - pPps->init_qp_minus26 - iQpBdOffset, 25 - pPps->init_qp_minus26);

    if(pPps->pps_slice_chroma_qp_offsets_present_flag)
    {
      pSlice->slice_cb_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
      pSlice->slice_cr_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
    }

    if(pPps->chroma_qp_offset_list_enabled_flag)
      pSlice->cu_chroma_qp_offset_enabled_flag = u(pRP, 1);

    if(pPps->deblocking_filter_control_present_flag)
    {
      if(pPps->deblocking_filter_override_enabled_flag)
        pSlice->deblocking_filter_override_flag = u(pRP, 1);

      if(pSlice->deblocking_filter_override_flag)
      {
        pSlice->slice_deblocking_filter_disabled_flag = u(pRP, 1);

        if(!pSlice->slice_deblocking_filter_disabled_flag)
        {
          pSlice->slice_beta_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
          pSlice->slice_tc_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
        }
      }
    }

    if(pPps->loop_filter_across_slices_enabled_flag &&
       (pSlice->slice_sao_luma_flag ||
        pSlice->slice_sao_chroma_flag ||
        !pSlice->slice_deblocking_filter_disabled_flag))
      pSlice->slice_loop_filter_across_slices_enabled_flag = u(pRP, 1);
  }

  if(pPps->tiles_enabled_flag || pPps->entropy_coding_sync_enabled_flag)
  {
    pSlice->num_entry_point_offsets = ue(pRP);

    if((!pPps->tiles_enabled_flag && pPps->entropy_coding_sync_enabled_flag && pSlice->num_entry_point_offsets >= pSps->PicHeightInCtbs) ||
       (!pPps->entropy_coding_sync_enabled_flag && pPps->tiles_enabled_flag && pSlice->num_entry_point_offsets >= ((pPps->num_tile_columns_minus1 + 1) * (pPps->num_tile_rows_minus1 + 1))) ||
       (pPps->entropy_coding_sync_enabled_flag && pPps->tiles_enabled_flag && pSlice->num_entry_point_offsets >= ((pPps->num_tile_columns_minus1 + 1) * pSps->PicHeightInCtbs)))
      return false;

    if(pSlice->num_entry_point_offsets > 0)
    {
      pSlice->offset_len_minus1 = ue(pRP);

      if(pSlice->offset_len_minus1 > 31)
        return false;

      int syntax_size = pSlice->offset_len_minus1 + 1;

      for(int i = 1; i <= pSlice->num_entry_point_offsets; ++i)
        pSlice->entry_point_offset_minus1[i] = u(pRP, syntax_size) + 1;
    }
  }

  if(pPps->slice_segment_header_extension_present_flag)
  {
    uint32_t slice_header_extension_length = ue(pRP);
    skip(pRP, slice_header_extension_length << 3); // slice_header_extension_data_byte
  }

  if(!byte_alignment(pRP))
    return false;

  if(!more_rbsp_data(pRP))
    return false;

  pConceal->iFirstLCU = pSlice->slice_segment_address;
  return true;
}

