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

#include "Avc_SliceHeaderParsing.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Utils.h"
#include "lib_common/Nuts.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_rtos/lib_rtos.h"

static const int AVC_SLICE_TYPE[5] =
{
  1, 0, 2, 3, 4
};

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

  if(pSlice->slice_type == AL_SLICE_B)
    AL_AVC_sReadWPCoeff(pRP, pSlice, 1);
}

/*************************************************************************//*!
   \brief the ref_pic_list_modification function parses the reordering syntax elements from a Slice Header NAL
   \param[in]  pRP             Pointer to NAL buffer
   \param[out] pSlice          Pointer to the slice header structure that will be filled
   \param[in]  NumPocTotalCurr Number of pictures available as reference for the current picture
   \return return true if no error was detected in reordering syntax
*****************************************************************************/
static bool AL_AVC_sref_pic_list_reordering(AL_TRbspParser* pRP, AL_TAvcSliceHdr* pSlice)
{
  if(pSlice->slice_type != AL_SLICE_I)
  {
    int idx1 = -1;
    int idx2 = -1;
    int idx3 = -1;

    pSlice->ref_pic_list_reordering_flag_l0 = u(pRP, 1);

    if(pSlice->ref_pic_list_reordering_flag_l0)
    {
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

      if(pSlice->reordering_of_pic_nums_idc_l0[idx1] != 3)
        return false;
    }
  }

  if(pSlice->slice_type == AL_SLICE_B)
  {
    int idx1 = -1;
    int idx2 = -1;
    int idx3 = -1;

    pSlice->ref_pic_list_reordering_flag_l1 = u(pRP, 1);

    if(pSlice->ref_pic_list_reordering_flag_l1)
    {
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

      if(pSlice->reordering_of_pic_nums_idc_l1[idx1] != 3)
        return false;
    }
  }

  return true;
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
static void ApplyAvcSPS(AL_TAvcSliceHdr* pSlice, AL_TAvcPps const* pPPS)
{
  pSlice->pPPS = pPPS;
  pSlice->pSPS = pSlice->pPPS->pSPS;
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
AL_ERR AL_AVC_ParseSliceHeader(AL_TAvcSliceHdr* pSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_TAvcPps pPPSTable[])
{
  if(!pConceal->bHasPPS)
    return AL_WARN_CONCEAL_DETECT;

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
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  pSlice->pic_parameter_set_id = currentPPSId;

  int const MaxNumMb = (pPPSTable[currentPPSId].pSPS->pic_height_in_map_units_minus1 + 1) * (pPPSTable[currentPPSId].pSPS->pic_width_in_mbs_minus1 + 1);

  if(pSlice->first_mb_in_slice >= MaxNumMb)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  if(pSlice->slice_type > AL_AVC_MAX_SLICE_TYPE)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  pSlice->slice_type %= 5;
  pSlice->slice_type = AVC_SLICE_TYPE[pSlice->slice_type];

  // check slice_type coherency
  if((pSlice->slice_type > AL_AVC_MAX_SLICE_TYPE) || (pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR && pSlice->slice_type != AL_SLICE_I && pSlice->slice_type != AL_SLICE_SI))
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  if(pConceal->bValidFrame && (pSlice->pic_parameter_set_id != pConceal->iActivePPS))
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  if(!pConceal->bValidFrame)
    pConceal->iActivePPS = pSlice->pic_parameter_set_id;

  // check first MB offset coherency
  if(pSlice->first_mb_in_slice <= pConceal->iFirstLCU)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  pSlice->slice_type %= 5;

  // check slice_type coherency
  if(pSlice->nal_unit_type == AL_AVC_NUT_VCL_IDR && pSlice->slice_type != AL_SLICE_I)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  // select the pps & sps for the current picture
  AL_TAvcPps const* pPps = pSlice->pPPS = &pPPSTable[currentPPSId];

  if(pPps->bConceal)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  AL_TAvcSps const* pSps = pSlice->pSPS = pPps->pSPS;

  if(pSps->bConceal)
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  int const iFrameNumSize = pSps->log2_max_frame_num_minus4 + 4;

  pSlice->frame_num = u(pRP, iFrameNumSize);

  if(!pSps->frame_mbs_only_flag)
  {
    pSlice->field_pic_flag = u(pRP, 1);

    if(pSlice->field_pic_flag)
    {
      pSlice->bottom_field_flag = u(pRP, 1);

      bool bCheckValidity = true;

      if(bCheckValidity)
      {
        Rtos_Log(AL_LOG_ERROR, "Interlaced pictures are not supported\n");
        ApplyAvcSPS(pSlice, pFallbackPps);
        return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
      }
    }
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

  if(pSlice->slice_type == AL_SLICE_B)
    pSlice->direct_spatial_mv_pred_flag = u(pRP, 1);

  if(pSlice->slice_type != AL_SLICE_I)
  {
    pSlice->num_ref_idx_active_override_flag = u(pRP, 1);

    if(pSlice->num_ref_idx_active_override_flag)
    {
      pSlice->num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);

      if(pSlice->slice_type == AL_SLICE_B)
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
  {
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_CONCEAL_DETECT;
  }

  if(!AL_AVC_sref_pic_list_reordering(pRP, pSlice))
    return AL_WARN_CONCEAL_DETECT;

  if(
    (pPps->weighted_pred_flag && pSlice->slice_type == AL_SLICE_P) ||
    (pPps->weighted_bipred_idc == 1 && pSlice->slice_type == AL_SLICE_B)
    )
  {
    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
    {
      ApplyAvcSPS(pSlice, pFallbackPps);
      return AL_WARN_CONCEAL_DETECT;
    }

    AL_AVC_spred_weight_table(pRP, pSlice);
  }

  if(pSlice->nal_ref_idc != 0)
  {
    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
    {
      ApplyAvcSPS(pSlice, pFallbackPps);
      return AL_WARN_CONCEAL_DETECT;
    }

    AL_AVC_sdec_ref_pic_marking(pRP, pSlice);
  }

  if(pPps->entropy_coding_mode_flag && pSlice->slice_type != AL_SLICE_I)
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

  if(pPps->num_slice_groups_minus1 != 0)
  {
    Rtos_Log(AL_LOG_ERROR, "ASO/FMO is not supported\n");
    ApplyAvcSPS(pSlice, pFallbackPps);
    return AL_WARN_ASO_FMO_NOT_SUPPORTED;
  }

  pConceal->iFirstLCU = pSlice->first_mb_in_slice;
  return AL_SUCCESS;
}
