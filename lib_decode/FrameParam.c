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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include <assert.h>

#include "lib_common/Utils.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecChanParam.h"

#include "DefaultDecoder.h"
#include "I_DecoderCtx.h"
#include "FrameParam.h"

/******************************************************************************/
static void FillRefPicID(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  AL_TDpb* pDpb = &pCtx->PictMngr.DPB;
  TBufferListRef* pListRef = &pCtx->ListRef;

  // Reg 9 ~ C
  for(uint8_t uRef = 0; uRef < MAX_REF; ++uRef)
  {
    uint8_t uNodeIDL0 = (*pListRef)[0][uRef].uNodeID;
    uint8_t uNodeIDL1 = (*pListRef)[1][uRef].uNodeID;
    pSP->PicIDL0[uRef] = (uNodeIDL0 == 0xFF) ? 0x00 : AL_Dpb_GetPicID_FromNode(pDpb, uNodeIDL0);
    pSP->PicIDL1[uRef] = (uNodeIDL1 == 0xFF) ? 0x00 : AL_Dpb_GetPicID_FromNode(pDpb, uNodeIDL1);
  }
}

/******************************************************************************/
static void FillConcealValue(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  AL_TDpb* pDpb = &pCtx->PictMngr.DPB;

  if(pDpb->uLastPOC == 0xFF)
    pSP->ValidConceal = false;
  else
  {
    pSP->ValidConceal = true;
    pSP->ColocPicID = pDpb->Nodes[pCtx->PictMngr.DPB.uLastPOC].uPicID;
  }
}

/******************************************************************************/
void AL_AVC_FillPictParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPP)
{
  const AL_TAvcSps* pSps = pSlice->pSPS;
  const AL_TAvcPps* pPps = pSlice->pPPS;

  pPP->num_tile_columns = 1;
  pPP->num_tile_rows = 1;

  // Reg 0
  pPP->MaxTUSize = pPps->transform_8x8_mode_flag ? 1 : 0; // log2 transfo_size - 2
  pPP->MaxCUSize = 4;
  pPP->Codec = AL_CODEC_AVC;

  // Reg 1
  pPP->PicWidth = (pSps->pic_width_in_mbs_minus1 + 1) << 1;
  pPP->PicHeight = (pSps->pic_height_in_map_units_minus1 + 1) << 1;

  // Reg 2
  pPP->eEntMode = pPps->entropy_coding_mode_flag ? AL_MODE_CABAC : AL_MODE_CAVLC;
  pPP->BitDepthLuma = pSps->bit_depth_luma_minus8 + 8;
  pPP->BitDepthChroma = pSps->bit_depth_chroma_minus8 + 8;
  pPP->ChromaMode = pSps->chroma_format_idc;
  AL_SET_DEC_OPT(pPP, IntraPCM, 1);
  AL_SET_DEC_OPT(pPP, LossLess, pSps->qpprime_y_zero_transform_bypass_flag);
  AL_SET_DEC_OPT(pPP, Direct8x8Infer, pSps->direct_8x8_inference_flag);

  // StandBy
  pPP->LcuWidth = pSps->pic_width_in_mbs_minus1 + 1;
  pPP->LcuHeight = pSps->pic_height_in_map_units_minus1 + 1;

  // Reg 0x10
  if(!pSps->seq_scaling_matrix_present_flag && !pPps->pic_scaling_matrix_present_flag)
  {
    AL_SET_DEC_OPT(pPP, EnableSC, 0);
    AL_SET_DEC_OPT(pPP, LoadSC, 0);
  }
  else
  {
    AL_SET_DEC_OPT(pPP, EnableSC, 1);
    AL_SET_DEC_OPT(pPP, LoadSC, 1);
  }
  AL_SET_DEC_OPT(pPP, ConstrainedIntraPred, pPps->constrained_intra_pred_flag);

  // Reg 0x13
  pPP->CurrentPOC = pCtx->PictMngr.iCurFramePOC;

  pPP->ePicStruct = PS_FRM;

  AL_SET_DEC_OPT(pPP, Tile, 0);
}

/******************************************************************************/
void AL_AVC_FillSliceParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP, AL_TDecPicParam* pPP, bool bConceal)
{
  const AL_TAvcPps* pPps = pSlice->pPPS;

  Rtos_Memset(pSP, 0, sizeof(AL_TDecSliceParam));

  // Reg 2
  pSP->CabacInitIdc = pSlice->cabac_init_idc;
  pSP->DirectSpatial = (bool)pSlice->direct_spatial_mv_pred_flag;

  // Reg 3
  pSP->CbQpOffset = pPps->chroma_qp_index_offset;
  pSP->CrQpOffset = pPps->second_chroma_qp_index_offset;
  pSP->SliceQP = pPps->pic_init_qp_minus26 + pSlice->slice_qp_delta + 26;

  if(!bConceal)
    pSP->eSliceType = (AL_ESliceType)pSlice->slice_type;

  // Reg 4
  pSP->tc_offset_div2 = pSlice->slice_alpha_c0_offset_div2;
  pSP->beta_offset_div2 = pSlice->slice_beta_offset_div2;
  pSP->LoopFilter = (pSlice->disable_deblocking_filter_idc & FILT_DISABLE);
  pSP->XSliceLoopFilter = !(pSlice->disable_deblocking_filter_idc & FILT_DIS_SLICE);
  pSP->WPTableID = pCtx->PictMngr.uNumSlice;

  // Reg 5
  if(pSP->eSliceType == SLICE_CONCEAL)
  {
    pSP->FirstLCU = pCtx->PictMngr.uNumSlice;
    pSP->FirstLcuSliceSegment = pCtx->PictMngr.uNumSlice;
    pSP->FirstLcuSlice = pCtx->PictMngr.uNumSlice;
  }
  else
  {
    pSP->FirstLCU = pSlice->first_mb_in_slice;
    pSP->FirstLcuSliceSegment = pSlice->first_mb_in_slice;
    pSP->FirstLcuSlice = pSlice->first_mb_in_slice;
  }

  pSP->NumRefIdxL0Minus1 = pSlice->num_ref_idx_l0_active_minus1;
  pSP->NumRefIdxL1Minus1 = pSlice->num_ref_idx_l1_active_minus1;

  // Reg 6
  pSP->NumLCU = pPP->LcuWidth * pPP->LcuHeight;
  pSP->SliceHeaderLength = pSlice->slice_header_length;

  // Reg 0x11
  if(pSP->eSliceType == SLICE_P)
  {
    pSP->WeightedPred = pPps->weighted_pred_flag;
    pSP->WeightedBiPred = 0;
  }
  else if(pSP->eSliceType == SLICE_B)
  {
    switch(pSlice->pPPS->weighted_bipred_idc)
    {
    case 0: // WP_DEFAULT
      pSP->WeightedPred = 0;
      pSP->WeightedBiPred = 0;
      break;

    case 1: // WP_EXPLICIT
      pSP->WeightedPred = 1;
      pSP->WeightedBiPred = 0;
      break;

    case 2: // WP_IMPLICIT
      pSP->WeightedPred = 0;
      pSP->WeightedBiPred = 1;
      break;
    }
  }

  pSP->DependentSlice = false;
}

/******************************************************************************/
void AL_AVC_FillSlicePicIdRegister(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  AL_TDpb* pDpb = &pCtx->PictMngr.DPB;
  TBufferListRef* pListRef = &pCtx->ListRef;

  FillRefPicID(pCtx, pSP);

  // Reg 0x12
  pSP->ColocPicID = 0;

  if(pSP->eSliceType == SLICE_B)
    pSP->ColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[1][0].uNodeID);

  FillConcealValue(pCtx, pSP);
}

/******************************************************************************/
void AL_HEVC_FillPictParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPP)
{
  // fast access
  AL_THevcSps* pSps = pSlice->pSPS;
  const AL_THevcPps* pPps = pSlice->pPPS;

  pPP->MaxTransfoDepthIntra = pSps->max_transform_hierarchy_depth_intra;
  pPP->MaxTransfoDepthInter = pSps->max_transform_hierarchy_depth_inter;
  pPP->num_tile_columns = pPps->num_tile_columns_minus1 + 1;
  pPP->num_tile_rows = pPps->num_tile_rows_minus1 + 1;
  pPP->MaxTUSkipSize = pPps->log2_transform_skip_block_size_minus2;
  pPP->MinTUSize = pSps->log2_min_transform_block_size_minus2;
  pPP->MaxTUSize = pPP->MinTUSize + pSps->log2_diff_max_min_transform_block_size;
  pPP->MinPCMSize = pSps->pcm_enabled_flag ? pSps->log2_min_pcm_luma_coding_block_size_minus3 + 3 : 0;
  pPP->MaxPCMSize = pSps->pcm_enabled_flag ? pPP->MinPCMSize + pSps->log2_diff_max_min_pcm_luma_coding_block_size : 0;
  pPP->MinCUSize = pSps->Log2MinCbSize;
  pPP->MaxCUSize = pSps->Log2CtbSize;
  pPP->Codec = AL_CODEC_HEVC;
  AL_SET_DEC_OPT(pPP, Tile, pPps->tiles_enabled_flag);

  // Reg 1
  pPP->PicWidth = pSps->pic_width_in_luma_samples >> 3;
  pPP->PicHeight = pSps->pic_height_in_luma_samples >> 3;
  pPP->PcmBitDepthY = pSps->pcm_enabled_flag ? pSps->pcm_sample_bit_depth_luma_minus1 + 1 : 0;
  pPP->PcmBitDepthC = pSps->pcm_enabled_flag ? pSps->pcm_sample_bit_depth_chroma_minus1 + 1 : 0;

  // Reg 2
  pPP->DeltaQPCUDepth = pPps->cu_qp_delta_enabled_flag ? pPps->diff_cu_qp_delta_depth : 0;
  AL_SET_DEC_OPT(pPP, CabacBypassAlign, pSps->cabac_bypass_alignment_enabled_flag);
  AL_SET_DEC_OPT(pPP, RiceAdapt, pSps->persistent_rice_adaptation_enabled_flag);
  pPP->eEntMode = AL_MODE_CABAC;
  AL_SET_DEC_OPT(pPP, ExplicitRdpcmFlag, pSps->explicit_rdpcm_enabled_flag);
  AL_SET_DEC_OPT(pPP, ImplicitRdpcmFlag, pSps->implicit_rdpcm_enabled_flag);
  AL_SET_DEC_OPT(pPP, ExtPrecisionFlag, pSps->extended_precision_processing_flag);
  AL_SET_DEC_OPT(pPP, TransfoSkipCtx, pSps->transform_skip_context_enabled_flag);
  pPP->ChromaMode = pSps->ChromaArrayType;
  pPP->BitDepthLuma = pSps->bit_depth_luma_minus8 + 8;
  pPP->BitDepthChroma = pSps->bit_depth_chroma_minus8 + 8;
  AL_SET_DEC_OPT(pPP, CuQPDeltaFlag, pPps->cu_qp_delta_enabled_flag);
  AL_SET_DEC_OPT(pPP, SignHiding, pPps->sign_data_hiding_flag);
  AL_SET_DEC_OPT(pPP, IntraPCM, pSps->pcm_enabled_flag);
  AL_SET_DEC_OPT(pPP, AMP, pSps->amp_enabled_flag);
  AL_SET_DEC_OPT(pPP, LossLess, pPps->transquant_bypass_enabled_flag);
  AL_SET_DEC_OPT(pPP, SkipTransfo, pPps->transform_skip_enabled_flag);

  // Reg 3
  pPP->QpOffLstSize = pPps->chroma_qp_offset_list_enabled_flag ? pPps->chroma_qp_offset_list_len_minus1 : 0;
  AL_SET_DEC_OPT(pPP, WaveFront, pPps->entropy_coding_sync_enabled_flag);

  // Reg 4
  pPP->ChromaQpOffsetDepth = pPps->chroma_qp_offset_list_enabled_flag ? pPps->diff_cu_chroma_qp_offset_depth : 0;

  // StandBy
  pPP->LcuWidth = (pSps->pic_width_in_luma_samples + (1 << pPP->MaxCUSize) - 1) >> pPP->MaxCUSize;
  pPP->LcuHeight = (pSps->pic_height_in_luma_samples + (1 << pPP->MaxCUSize) - 1) >> pPP->MaxCUSize;

  // Reg D
  AL_SET_DEC_OPT(pPP, IntraSmoothDisable, pSps->intra_smoothing_disabled_flag);
  AL_SET_DEC_OPT(pPP, EnableSC, pSps->scaling_list_enabled_flag);
  AL_SET_DEC_OPT(pPP, LoadSC, pSps->scaling_list_enabled_flag);

  // Reg E
  AL_SET_DEC_OPT(pPP, XTileLoopFilter, pPps->loop_filter_across_tiles_enabled_flag);
  AL_SET_DEC_OPT(pPP, DisPCMLoopFilter, pSps->pcm_loop_filter_disabled_flag);
  AL_SET_DEC_OPT(pPP, StrongIntraSmooth, pSps->strong_intra_smoothing_enabled_flag);
  AL_SET_DEC_OPT(pPP, ConstrainedIntraPred, pPps->constrained_intra_pred_flag);

  // Reg F
  pPP->ParallelMerge = pPps->log2_parallel_merge_level_minus2;
  pPP->PicCbQpOffset = pPps->pps_cb_qp_offset;
  pPP->PicCrQpOffset = pPps->pps_cr_qp_offset;
  AL_SET_DEC_OPT(pPP, TransfoSkipRot, pSps->transform_skip_rotation_enabled_flag);
  AL_SET_DEC_OPT(pPP, HighPrecOffset, pSps->high_precision_offsets_enabled_flag);

  // Reg G-H
  for(int i = 0; i < 6; ++i)
  {
    pPP->CbQpOffLst[i] = pPps->cb_qp_offset_list[i];
    pPP->CrQpOffLst[i] = pPps->cr_qp_offset_list[i];
  }

  // Reg J
  pPP->CurrentPOC = pCtx->PictMngr.iCurFramePOC;

  pPP->ePicStruct = (AL_EPicStruct)pCtx->aup.hevcAup.ePicStruct;

  if(pPps->tiles_enabled_flag)
  {
    for(int i = 0; i <= pPps->num_tile_columns_minus1; ++i)
      pPP->column_width[i] = pPps->column_width[i];

    for(int i = 0; i <= pPps->num_tile_rows_minus1; ++i)
      pPP->row_height[i] = pPps->row_height[i];
  }
}

/******************************************************************************/
void AL_HEVC_FillSliceParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP, bool bConceal)
{
  // fast access
  const AL_THevcPps* pPps = pSlice->pPPS;

  pSP->MaxMergeCand = 5 - pSlice->five_minus_max_num_merge_cand;
  pSP->ColocFromL0 = pSlice->collocated_from_l0_flag;
  pSP->mvd_l1_zero_flag = pSlice->mvd_l1_zero_flag;
  pSP->TemporalMVP = (bool)pSlice->slice_temporal_mvp_enable_flag;

  // Reg 3
  pSP->CbQpOffset = pSlice->slice_cb_qp_offset;
  pSP->CrQpOffset = pSlice->slice_cr_qp_offset;
  pSP->SliceQP = 26 + pPps->init_qp_minus26 + pSlice->slice_qp_delta;
  pSP->CabacInitIdc = pSlice->cabac_init_flag;

  if(!bConceal)
    pSP->eSliceType = (AL_ESliceType)pSlice->slice_type;
  pSP->DependentSlice = (bool)pSlice->dependent_slice_segment_flag;

  // Reg 4
  pSP->tc_offset_div2 = pSlice->slice_tc_offset_div2;
  pSP->beta_offset_div2 = pSlice->slice_beta_offset_div2;
  pSP->SAOFilterLuma = (bool)pSlice->slice_sao_luma_flag;
  pSP->SAOFilterChroma = (bool)pSlice->slice_sao_chroma_flag;
  pSP->LoopFilter = (bool)pSlice->slice_deblocking_filter_disabled_flag;
  pSP->XSliceLoopFilter = (bool)pSlice->slice_loop_filter_across_slices_enabled_flag;
  pSP->CuChromaQpOffset = (bool)pSlice->cu_chroma_qp_offset_enabled_flag;
  pSP->WPTableID = pCtx->PictMngr.uNumSlice;

  // Reg 5
  pSP->NumRefIdxL0Minus1 = pSlice->num_ref_idx_l0_active_minus1;
  pSP->NumRefIdxL1Minus1 = pSlice->num_ref_idx_l1_active_minus1;

  // Reg 6
  pSP->SliceHeaderLength = pSlice->slice_header_length;

  // Reg 8
  pSP->NumEntryPoint = pSlice->num_entry_point_offsets;

  // StandBy
  if(pSP->eSliceType == SLICE_CONCEAL)
  {
    // search prev slice
    uint16_t uSliceID = pCtx->PictMngr.uNumSlice;
    AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

    if(pPrevSP)
    {
      pSP->FirstLcuSliceSegment = pPrevSP->NextSliceSegment;
      pSP->FirstLcuSlice = pPrevSP->NextSliceSegment;
    }
    else
    {
      pSP->FirstLcuSliceSegment = pCtx->PictMngr.uNumSlice;
      pSP->FirstLcuSlice = pCtx->PictMngr.uNumSlice;
    }
  }
  else
  {
    pSP->FirstLcuSliceSegment = pSlice->slice_segment_address;
    pSP->FirstLcuSlice = pSlice->slice_segment_address;
  }
  pSP->FirstLcuTileID = pCtx->uCurTileID;

  // Reg E
  pSP->WeightedPred = pPps->weighted_pred_flag;
  pSP->WeightedBiPred = pPps->weighted_bipred_flag;

  for(int i = 0; i <= pSlice->num_entry_point_offsets; ++i)
    pSP->entry_point_offset[i] = pSlice->entry_point_offset_minus1[i];
}

/******************************************************************************/
void AL_HEVC_FillSlicePicIdRegister(const AL_THevcSliceHdr* pSlice, AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP)
{
  TBufferListRef* pListRef = &pCtx->ListRef;
  AL_TDpb* pDpb = &pCtx->PictMngr.DPB;

  FillRefPicID(pCtx, pSP);

  if(!pSP->FirstLcuSlice)
    pPP->ColocPicID = UndefID;

  // Reg 0x12
  if((pSP->eSliceType == SLICE_B && pSlice->collocated_from_l0_flag) || pSP->eSliceType == SLICE_P)
    pPP->ColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[0][pSlice->collocated_ref_idx].uNodeID);
  else if(pSP->eSliceType == SLICE_B)
    pPP->ColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[1][pSlice->collocated_ref_idx].uNodeID);

  FillConcealValue(pCtx, pSP);
}

/*@}*/

