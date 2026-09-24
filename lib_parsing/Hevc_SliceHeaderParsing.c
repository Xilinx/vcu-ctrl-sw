// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Hevc_SliceHeaderParsing.h"
#include "HevcParser.h"
#include "lib_rtos/assert.h"
#include "lib_common/Arithmetics.h"
#include "lib_common/BufConst.h"
#include "lib_common/HevcUtils.h"
#include "lib_common/SliceConsts.h"

#include "lib_common/Nuts.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_rtos/lib_rtos.h"

/*****************************************************************************/
void AL_HEVC_SetDefaultSliceHeader(AL_THevcSliceHdr* pSlice)
{

  Rtos_Memset(pSlice, 0, offsetof(AL_THevcSliceHdr, entry_point_offset_minus1));

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
  pSlice->NumPicTotalCurr = pLastSlice->NumPicTotalCurr;
}

/*****************************************************************************/
static void AL_HEVC_sInitSlice(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pLastSlice)
{
  *pSlice = *pLastSlice;

  pSlice->first_slice_segment_in_pic_flag = 0;
  pSlice->dependent_slice_segment_flag = 1;
}

/*************************************************************************/
static bool AL_HEVC_sReadWPCoeff(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice, uint8_t uL0L1)
{
  uint8_t uNumRefIdx = uL0L1 ? pSlice->num_ref_idx_l1_active_minus1 : pSlice->num_ref_idx_l0_active_minus1;

  if(uNumRefIdx >= AL_MAX_REF)
    return false;

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
    int32_t iOffsetY = pSlice->pSPS->WpOffsetHalfRangeY;
    int32_t iOffsetC = pSlice->pSPS->WpOffsetHalfRangeC;

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
      int32_t iChromaWeight;
      uint8_t uOffset = (1 << pSlice->pred_weight_table.chroma_log2_weight_denom);

      pWpCoeff->chroma_delta_weight[i][0] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
      iChromaWeight = uOffset + pWpCoeff->chroma_delta_weight[i][0];
      pWpCoeff->chroma_offset[i][0] = Clip3(Clip3(se(pRP), -4 * iOffsetC, 4 * iOffsetC - 1) + iOffsetC - ((iChromaWeight * iOffsetC) >> pSlice->pred_weight_table.chroma_log2_weight_denom), -iOffsetC, iOffsetC - 1);

      pWpCoeff->chroma_delta_weight[i][1] = Clip3(se(pRP), AL_MIN_WP_CHROMA_DELTA_WEIGHT, AL_MAX_WP_CHROMA_DELTA_WEIGHT);
      iChromaWeight = uOffset + pWpCoeff->chroma_delta_weight[i][1];
      pWpCoeff->chroma_offset[i][1] = Clip3(Clip3(se(pRP), -4 * iOffsetC, 4 * iOffsetC - 1) + iOffsetC - ((iChromaWeight * iOffsetC) >> pSlice->pred_weight_table.chroma_log2_weight_denom), -iOffsetC, iOffsetC - 1);
    }
  }

  return true;
}

/*****************************************************************************
   \brief the pred_weight_table function parses the weighted pred table syntax
         elements from a Slice Header NAL
   \param[in]  pRP    Pointer to NAL buffer
   \param[out] pSlice Pointer to the slice header structure that will be filled
   \return True if weighted prediction are correctly read
*****************************************************************************/
static bool AL_HEVC_spread_weight_table(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice)
{
  pSlice->pred_weight_table.luma_log2_weight_denom = Clip3(ue(pRP), 0, AL_MAX_WP_DENOM);
  pSlice->pred_weight_table.chroma_log2_weight_denom = Clip3(pSlice->pred_weight_table.luma_log2_weight_denom + (pSlice->pSPS->ChromaArrayType ? se(pRP) : 0), 0, AL_MAX_WP_DENOM);

  if(!AL_HEVC_sReadWPCoeff(pRP, pSlice, 0))
    return false;

  if(pSlice->slice_type == AL_SLICE_B)
    if(!AL_HEVC_sReadWPCoeff(pRP, pSlice, 1))
      return false;

  return true;
}

/*****************************************************************************
   \brief This function parses the reordering syntax elements from a Slice
         Header NAL
   \param[in]  pRP             Pointer to NAL buffer
   \param[out] pSlice          Pointer to the slice header structure that will
                              be filled
   \param[in]  NumPicTotalCurr Number of pictures available as reference for
                              the current picture
*****************************************************************************/
static void AL_HEVC_sref_pic_list_modification(AL_TRbspParser* pRP, AL_THevcSliceHdr* pSlice, uint8_t NumPicTotalCurr)
{
  uint8_t list_entry_size = ceil_log2(NumPicTotalCurr);

  // default values
  pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 = 0;
  pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1 = 0;

  for(uint8_t i = 0; i < AL_MAX_REF; ++i)
  {
    pSlice->ref_pic_modif.list_entry_l0[i] = 0;
    pSlice->ref_pic_modif.list_entry_l1[i] = 0;
  }

  if(pSlice->slice_type != AL_SLICE_I)
  {
    pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 = u(pRP, 1);

    if(pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0)
    {
      for(uint8_t i = 0; i <= pSlice->num_ref_idx_l0_active_minus1; ++i)
        pSlice->ref_pic_modif.list_entry_l0[i] = u(pRP, list_entry_size);
    }

    if(pSlice->slice_type == AL_SLICE_B)
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
static void InitRefPictSet(AL_THevcSliceHdr* pSlice)
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

  pSlice->NumPicTotalCurr = pSlice->NumPocStCurrBefore + pSlice->NumPocStCurrAfter + pSlice->NumPocLtCurr;
}

/*****************************************************************************/
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
bool AL_HEVC_ParseSliceHeader(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pIndSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_THevcPps pPPSTable[], AL_THevcRefListBuilderCB* pRefListBuilderCB)
{
  int iTotalNalSize = pRP->iBufInAvailSize;

  AL_HEVC_SetDefaultSliceHeader(pSlice);

  if(noValidPpsHasEverBeenParsed(pConceal))
    return false;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 1); // forbidden_zero_bit
  pSlice->nal_unit_type = u(pRP, 6);
  pSlice->nuh_layer_id = u(pRP, 6);
  pSlice->nuh_temporal_id_plus1 = u(pRP, 3);

  if(pSlice->nuh_temporal_id_plus1 < 1 || pSlice->nuh_temporal_id_plus1 > AL_MAX_SUB_LAYER)
    return false;

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
    pSlice->slice_segment_address = 0;

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

  if(pSlice->nuh_temporal_id_plus1 - 1 > pSps->sps_max_sub_layers_minus1)
    return false;

  if(!pSlice->first_slice_segment_in_pic_flag)
  {
    uint32_t uMaxLcu = pSps->PicWidthInCtbs * pSps->PicHeightInCtbs;
    pSlice->dependent_slice_segment_flag = pPps->dependent_slice_segments_enabled_flag ? u(pRP, 1) : 0;

    if(pSlice->dependent_slice_segment_flag)
      AL_HEVC_sInitSlice(pSlice, pIndSlice);

    TransferRps(pSlice, pIndSlice);

    int32_t syntax_size = ceil_log2(uMaxLcu);
    pSlice->slice_segment_address = u(pRP, syntax_size);

    if((uint32_t)pSlice->slice_segment_address >= uMaxLcu)
      return false;
  }

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

    int i = 0;
    skip(pRP, pPps->num_extra_slice_header_bits - i); // slice_reserved_flag

    pSlice->slice_type = ue(pRP);

    // check slice_type coherency
    if((pSlice->slice_type > AL_SLICE_I) || (pSlice->IdrPicFlag && pSlice->slice_type != AL_SLICE_I && pSlice->nuh_layer_id == 0))
      return false;

    if(pPps->output_flag_present_flag)
      pSlice->pic_output_flag = u(pRP, 1);
    else
      pSlice->pic_output_flag = 1;

    if(pSps->separate_colour_plane_flag)
      pSlice->colour_plane_id = u(pRP, 2);

    bool bParsePOC = !pSlice->IdrPicFlag;

    if(bParsePOC)
    {
      int32_t syntax_size = pSps->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
      pSlice->slice_pic_order_cnt_lsb = u(pRP, syntax_size);
    }

    if(!pSlice->IdrPicFlag)
    {
      pSlice->short_term_ref_pic_set_sps_flag = u(pRP, 1);

      // check if NAL isn't empty
      if(!more_rbsp_data(pRP))
        return false;

      if(!pSlice->short_term_ref_pic_set_sps_flag)
      {
        if(!AL_HEVC_short_term_ref_pic_set(pSlice->pSPS, pSps->num_short_term_ref_pic_sets, pRP))
          return false;
      }
      else if(pSps->num_short_term_ref_pic_sets > 1)
      {
        int32_t syntax_size = ceil_log2(pSps->num_short_term_ref_pic_sets);
        pSlice->short_term_ref_pic_set_idx = u(pRP, syntax_size);
        pSlice->pSPS->short_term_ref_pic_set[64] = pSlice->pSPS->short_term_ref_pic_set[pSlice->short_term_ref_pic_set_idx];
      }

      if(pSps->long_term_ref_pics_present_flag)
      {
        if(pSps->num_long_term_ref_pics_sps > 0)
        {
          pSlice->num_long_term_sps = ue(pRP);

          if(pSlice->num_long_term_sps > pSps->num_long_term_ref_pics_sps)
            return false;
        }

        pSlice->num_long_term_pics = ue(pRP);

        int32_t CurrRpsIdx = pSlice->short_term_ref_pic_set_sps_flag ? pSlice->short_term_ref_pic_set_idx : pSps->num_short_term_ref_pic_sets;

        if(pSlice->num_long_term_pics > pSps->sps_max_dec_pic_buffering_minus1[pSlice->nuh_temporal_id_plus1 - 1] - pSlice->num_long_term_sps - pSps->NumDeltaPocs[CurrRpsIdx])
          return false;

        for(int32_t i = 0; i < pSlice->num_long_term_sps + pSlice->num_long_term_pics; ++i)
        {
          if(i < pSlice->num_long_term_sps)
          {
            if(pSps->num_long_term_ref_pics_sps > 1)
            {
              int32_t syntax_size = ceil_log2(pSps->num_long_term_ref_pics_sps);
              pSlice->lt_idx_sps[i] = u(pRP, syntax_size);
            }

            // compute long term variables
            pSlice->PocLsbLt[i] = pSps->lt_ref_pic_poc_lsb_sps[pSlice->lt_idx_sps[i]];
            pSlice->UsedByCurrPicLt[i] = pSps->used_by_curr_pic_lt_sps_flag[pSlice->lt_idx_sps[i]];
          }
          else
          {
            int32_t syntax_size = pSps->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
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

    if(pSlice->slice_type != AL_SLICE_I)
    {
      pSlice->num_ref_idx_active_override_flag = u(pRP, 1);

      if(pSlice->num_ref_idx_active_override_flag)
      {
        pSlice->num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

        if(pSlice->slice_type == AL_SLICE_B)
          pSlice->num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
      }

      // check if NAL isn't empty
      if(!more_rbsp_data(pRP))
        return false;

      if(pPps->lists_modification_present_flag && pSlice->NumPicTotalCurr > 1)
        AL_HEVC_sref_pic_list_modification(pRP, pSlice, pSlice->NumPicTotalCurr);

      if(pSlice->slice_type == AL_SLICE_B)
        pSlice->mvd_l1_zero_flag = u(pRP, 1);

      if(pPps->cabac_init_present_flag)
        pSlice->cabac_init_flag = u(pRP, 1);

      if(pSlice->slice_temporal_mvp_enable_flag)
      {
        if(pSlice->slice_type == AL_SLICE_B)
          pSlice->collocated_from_l0_flag = u(pRP, 1);

        if(pSlice->slice_type != AL_SLICE_I)
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

      if((pPps->weighted_pred_flag && pSlice->slice_type == AL_SLICE_P) ||
         (pPps->weighted_bipred_flag && pSlice->slice_type == AL_SLICE_B))
      {
        (void)pRefListBuilderCB;

        if(!AL_HEVC_spread_weight_table(pRP, pSlice))
          return false;
      }

      pSlice->five_minus_max_num_merge_cand = ue(pRP);

      if(pSlice->five_minus_max_num_merge_cand > 4)
        return false;
    }

    // check if NAL isn't empty
    if(!more_rbsp_data(pRP))
      return false;

    int32_t iQpBdOffset = 6 * pSps->bit_depth_luma_minus8;
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
          pSlice->slice_beta_offset_div2 = Clip3(se(pRP), AL_HEVC_MIN_DBF_PARAM, AL_HEVC_MAX_DBF_PARAM);
          pSlice->slice_tc_offset_div2 = Clip3(se(pRP), AL_HEVC_MIN_DBF_PARAM, AL_HEVC_MAX_DBF_PARAM);
        }
      }
    }

    if(pPps->loop_filter_across_slices_enabled_flag &&
       (pSlice->slice_sao_luma_flag ||
        pSlice->slice_sao_chroma_flag ||
        !pSlice->slice_deblocking_filter_disabled_flag))
      pSlice->slice_loop_filter_across_slices_enabled_flag = u(pRP, 1);
  }

  bool bNeedEntryPointResilienceCheck = false;

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

      int32_t syntax_size = pSlice->offset_len_minus1 + 1;

      for(int32_t i = 1; i <= pSlice->num_entry_point_offsets; ++i)
      {
        pSlice->entry_point_offset_minus1[i] = u(pRP, syntax_size);
        bNeedEntryPointResilienceCheck = true;
      }
    }
  }

  if(pPps->slice_segment_header_extension_present_flag)
  {
    uint32_t slice_header_extension_length = ue(pRP);
    uint32_t uBeginExt = 0;
    uint32_t uEndExt = 0;
    skip(pRP, (slice_header_extension_length << 3) - (uEndExt - uBeginExt)); // slice_header_extension_data_byte
  }

  if(!byte_alignment(pRP))
    return false;

  if(!more_rbsp_data(pRP))
    return false;

  if(bNeedEntryPointResilienceCheck)
  {
    uint32_t uChkStrSizeInBytes = ((iTotalNalSize << 3) - pRP->iCurrentBitIndex) >> 3;
    uint32_t uChkStrConsumed = 0;

    for(int32_t i = 1; i <= pSlice->num_entry_point_offsets; ++i)
    {
      uChkStrConsumed += (pSlice->entry_point_offset_minus1[i] + 1);

      if(uChkStrConsumed > uChkStrSizeInBytes)
      {
        Rtos_Log(AL_LOG_ERROR, "for pSlice->num_entry_point_offsets=%d, failure with last pSlice->entry_point_offset_minus1[]= %8.8x and uChkStrConsumed %8.8x vs uChkStrSize %8.8x\n", pSlice->num_entry_point_offsets, pSlice->entry_point_offset_minus1[i], uChkStrConsumed, uChkStrSizeInBytes);
        return false;
      }
    }
  }
  return true;
}
