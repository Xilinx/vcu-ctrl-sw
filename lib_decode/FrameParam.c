// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "I_DecoderCtx.h"
#include "FrameParam.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common/Index.h"

/******************************************************************************/
int32_t AL_AVC_GetFrameHeight(AL_TAvcSps const* pSPS, bool bHasFields)
{
  return (bHasFields || pSPS->frame_mbs_only_flag) ? (pSPS->pic_height_in_map_units_minus1 + 1) : ((pSPS->pic_height_in_map_units_minus1 + 1) * 2);
}

/******************************************************************************/
AL_EPicStruct AL_AVC_GetPicStruct(AL_TAvcSliceHdr const* pSlice)
{
  (void)pSlice;
  AL_EPicStruct ePicStruct = AL_PS_FRM;

  return ePicStruct;
}

/******************************************************************************/
void AL_AVC_FillPictParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam)
{
  const AL_TAvcSps* pSps = pSlice->pSPS;
  const AL_TAvcPps* pPps = pSlice->pPPS;

  pPicParam->uNumTileCols = 1;
  pPicParam->uNumTileRows = 1;

  pPicParam->uLog2MaxTuSize = pPps->transform_8x8_mode_flag ? 3 : 2;
  pPicParam->uLog2MaxCuSize = 4;
  pPicParam->eCodec = AL_CODEC_AVC;

  AL_TDimension const tDim = { (pSps->pic_width_in_mbs_minus1 + 1) << 4, AL_AVC_GetFrameHeight(pSps, pSlice->field_pic_flag) << 4 };
  DecPicParam_SetPicDim(pPicParam, tDim);

  pPicParam->eEntMode = pPps->entropy_coding_mode_flag ? AL_MODE_CABAC : AL_MODE_CAVLC;
  pPicParam->uBitDepthLuma = pSps->bit_depth_luma_minus8 + 8;
  pPicParam->uBitDepthChroma = pSps->bit_depth_chroma_minus8 + 8;
  pPicParam->eChromaMode = pSps->chroma_format_idc;
  AL_SET_DEC_OPT(pPicParam, IntraPCM, 1);
  AL_SET_DEC_OPT(pPicParam, LossLess, pSps->qpprime_y_zero_transform_bypass_flag);
  AL_SET_DEC_OPT(pPicParam, Direct8x8Infer, pSps->direct_8x8_inference_flag);

  if(!pSps->seq_scaling_matrix_present_flag && !pPps->pic_scaling_matrix_present_flag)
  {
    AL_SET_DEC_OPT(pPicParam, EnableSclLst, 0);
    AL_SET_DEC_OPT(pPicParam, LoadSclLst, 0);
  }
  else
  {
    AL_SET_DEC_OPT(pPicParam, EnableSclLst, 1);
    AL_SET_DEC_OPT(pPicParam, LoadSclLst, 1);
  }
  AL_SET_DEC_OPT(pPicParam, ConstrainedIntraPred, pPps->constrained_intra_pred_flag);

  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
  pPicParam->iCurrentPoc = pDpb->iCurFramePOC;

  pPicParam->ePicStruct = AL_PS_FRM;

  AL_SET_DEC_OPT(pPicParam, Tile, 0);
}

/******************************************************************************/
void AL_AVC_FillSliceParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam, AL_TDecPicParam* pPicParam, bool bConceal)
{
  const AL_TAvcPps* pPps = pSlice->pPPS;

  Rtos_Memset(pSliceParam, 0, offsetof(AL_TDecSliceParam, pEntryPointOffsets));

  pSliceParam->bIsRef = pSlice->nal_ref_idc != 0;

  pSliceParam->uCabacInitIdc = pSlice->cabac_init_idc;
  pSliceParam->bDirectSpatial = (bool)pSlice->direct_spatial_mv_pred_flag;

  pSliceParam->iCbQpOffset = pPps->chroma_qp_index_offset;
  pSliceParam->iCrQpOffset = pPps->second_chroma_qp_index_offset;
  pSliceParam->iSliceQq = pPps->pic_init_qp_minus26 + pSlice->slice_qp_delta + 26;

  if(!bConceal)
    pSliceParam->eSliceType = (AL_ESliceType)pSlice->slice_type;

  pSliceParam->iTcOffsetDiv2 = pSlice->slice_alpha_c0_offset_div2;
  pSliceParam->iBetaOffsetDiv2 = pSlice->slice_beta_offset_div2;
  int32_t const DEBLOCKING_FILTER_DISABLE = 0x1;
  pSliceParam->bDisableLoopFilter = (pSlice->disable_deblocking_filter_idc & DEBLOCKING_FILTER_DISABLE);
  int32_t const DEBLOCKING_FILTER_SLICE = 0x2;
  pSliceParam->bAcrossSliceLoopFilter = !(pSlice->disable_deblocking_filter_idc & DEBLOCKING_FILTER_SLICE);
  pSliceParam->uSliceId = pCtx->tCurrentFrameCtx.uNumSlice;

  if(pSliceParam->eSliceType == AL_SLICE_CONCEAL)
  {
    pSliceParam->uSliceFirstLcu = pCtx->tCurrentFrameCtx.uNumSlice;
    pSliceParam->uFirstLcuSliceSegment = pCtx->tCurrentFrameCtx.uNumSlice;
    pSliceParam->uFirstLcuSlice = pCtx->tCurrentFrameCtx.uNumSlice;
  }
  else
  {
    pSliceParam->uSliceFirstLcu = pSlice->first_mb_in_slice;
    pSliceParam->uFirstLcuSliceSegment = pSlice->first_mb_in_slice;
    pSliceParam->uFirstLcuSlice = pSlice->first_mb_in_slice;
  }

  pSliceParam->uNumRefIdxL0Minus1 = pSlice->num_ref_idx_l0_active_minus1;
  pSliceParam->uNumRefIdxL1Minus1 = pSlice->num_ref_idx_l1_active_minus1;

  pSliceParam->uSliceNumLcu = DecPicParam_GetNumLcuInFrame(pPicParam);
  pSliceParam->uSliceHeaderLength = pSlice->slice_header_length;

  if(pSliceParam->eSliceType == AL_SLICE_P)
  {
    pSliceParam->bWeightedPred = pPps->weighted_pred_flag;
    pSliceParam->bWeightedBiPred = 0;
  }
  else if(pSliceParam->eSliceType == AL_SLICE_B)
  {
    switch(pSlice->pPPS->weighted_bipred_idc)
    {
    case 0: // WP_DEFAULT
      pSliceParam->bWeightedPred = 0;
      pSliceParam->bWeightedBiPred = 0;
      break;

    case 1: // WP_EXPLICIT
      pSliceParam->bWeightedPred = 1;
      pSliceParam->bWeightedBiPred = 0;
      break;

    case 2: // WP_IMPLICIT
      pSliceParam->bWeightedPred = 0;
      pSliceParam->bWeightedBiPred = 1;
      break;
    }
  }

  pSliceParam->bDependentSlice = false;

}

/******************************************************************************/
void AL_HEVC_FillPictParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam)
{
  AL_THevcSps* pSps = pSlice->pSPS;
  const AL_THevcPps* pPps = pSlice->pPPS;

  pPicParam->uMaxTransfoDepthIntra = pSps->max_transform_hierarchy_depth_intra;
  pPicParam->uMaxTransfoDepthInter = pSps->max_transform_hierarchy_depth_inter;
  pPicParam->uNumTileCols = pPps->num_tile_columns_minus1 + 1;
  pPicParam->uNumTileRows = pPps->num_tile_rows_minus1 + 1;
  pPicParam->uLog2MaxTuSkipSize = pPps->log2_transform_skip_block_size_minus2 + 2;
  pPicParam->uLog2MinTuSize = pSps->log2_min_transform_block_size_minus2 + 2;
  pPicParam->uLog2MaxTuSize = pPicParam->uLog2MinTuSize + pSps->log2_diff_max_min_transform_block_size;
  pPicParam->uLog2MinPcmSize = pSps->pcm_enabled_flag ? pSps->log2_min_pcm_luma_coding_block_size_minus3 + 3 : 3;
  pPicParam->uLog2MaxPcmSize = pSps->pcm_enabled_flag ? pPicParam->uLog2MinPcmSize + pSps->log2_diff_max_min_pcm_luma_coding_block_size : 3;
  pPicParam->uLog2MinCuSize = pSps->Log2MinCbSize;
  pPicParam->uLog2MaxCuSize = pSps->Log2CtbSize;
  pPicParam->eCodec = AL_CODEC_HEVC;
  AL_SET_DEC_OPT(pPicParam, Tile, pPps->tiles_enabled_flag);

  AL_TDimension const tDim = { pSps->pic_width_in_luma_samples, pSps->pic_height_in_luma_samples };
  DecPicParam_SetPicDim(pPicParam, tDim);

  pPicParam->uPcmBitDepthY = pSps->pcm_enabled_flag ? pSps->pcm_sample_bit_depth_luma_minus1 + 1 : 0;
  pPicParam->uPcmBitDepthC = pSps->pcm_enabled_flag ? pSps->pcm_sample_bit_depth_chroma_minus1 + 1 : 0;

  pPicParam->uDeltaQpCuDepth = pPps->cu_qp_delta_enabled_flag ? pPps->diff_cu_qp_delta_depth : 0;
  AL_SET_DEC_OPT(pPicParam, CabacBypassAlign, pSps->cabac_bypass_alignment_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, RiceAdapt, pSps->persistent_rice_adaptation_enabled_flag);
  pPicParam->eEntMode = AL_MODE_CABAC;
  AL_SET_DEC_OPT(pPicParam, ExplicitRdpcmFlag, pSps->explicit_rdpcm_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, ImplicitRdpcmFlag, pSps->implicit_rdpcm_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, ExtPrecisionFlag, pSps->extended_precision_processing_flag);
  AL_SET_DEC_OPT(pPicParam, TransfoSkipCtx, pSps->transform_skip_context_enabled_flag);
  pPicParam->eChromaMode = pSps->ChromaArrayType;
  pPicParam->uBitDepthLuma = pSps->bit_depth_luma_minus8 + 8;
  pPicParam->uBitDepthChroma = pSps->bit_depth_chroma_minus8 + 8;
  AL_SET_DEC_OPT(pPicParam, CuQPDeltaFlag, pPps->cu_qp_delta_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, SignHiding, pPps->sign_data_hiding_flag);
  AL_SET_DEC_OPT(pPicParam, IntraPCM, pSps->pcm_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, AMP, pSps->amp_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, LossLess, pPps->transquant_bypass_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, SkipTransfo, pPps->transform_skip_enabled_flag);

  pPicParam->uLog2SaoOffsetScaleLuma = pPps->log2_sao_offset_scale_luma;
  pPicParam->uLog2SaoOffsetScaleChroma = pPps->log2_sao_offset_scale_chroma;

  pPicParam->uQpOffLstSize = pPps->chroma_qp_offset_list_enabled_flag ? pPps->chroma_qp_offset_list_len_minus1 : 0;
  AL_SET_DEC_OPT(pPicParam, WaveFront, pPps->entropy_coding_sync_enabled_flag);

  pPicParam->uChromaQpOffsetDepth = pPps->chroma_qp_offset_list_enabled_flag ? pPps->diff_cu_chroma_qp_offset_depth : 0;

  AL_SET_DEC_OPT(pPicParam, IntraSmoothDisable, pSps->intra_smoothing_disabled_flag);
  AL_SET_DEC_OPT(pPicParam, EnableSclLst, pSps->scaling_list_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, LoadSclLst, pSps->scaling_list_enabled_flag);

  AL_SET_DEC_OPT(pPicParam, XTileLoopFilter, pPps->loop_filter_across_tiles_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, DisPCMLoopFilter, pSps->pcm_loop_filter_disabled_flag);
  AL_SET_DEC_OPT(pPicParam, StrongIntraSmooth, pSps->strong_intra_smoothing_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, ConstrainedIntraPred, pPps->constrained_intra_pred_flag);

  pPicParam->uParallelMerge = pPps->log2_parallel_merge_level_minus2 + 2;
  pPicParam->iPicCbQpOffset = pPps->pps_cb_qp_offset;
  pPicParam->iPicCrQpOffset = pPps->pps_cr_qp_offset;
  AL_SET_DEC_OPT(pPicParam, TransfoSkipRot, pSps->transform_skip_rotation_enabled_flag);
  AL_SET_DEC_OPT(pPicParam, HighPrecOffset, pSps->high_precision_offsets_enabled_flag);

  for(int32_t i = 0; i < 6; ++i)
  {
    pPicParam->pCbQpOffsets[i] = pPps->cb_qp_offset_list[i];
    pPicParam->pCrQpOffsets[i] = pPps->cr_qp_offset_list[i];
  }

  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
  pPicParam->iCurrentPoc = pDpb->iCurFramePOC;

  pPicParam->ePicStruct = (AL_EPicStruct)pCtx->aup.hevcAup.ePicStruct;

  if(pPps->tiles_enabled_flag)
  {
    for(int32_t i = 0; i <= pPps->num_tile_columns_minus1; ++i)
      pPicParam->pTileColWidths[i] = pPps->pTileColWidths[i];

    for(int32_t i = 0; i <= pPps->num_tile_rows_minus1; ++i)
      pPicParam->pTileRowHeights[i] = pPps->pTileRowHeights[i];
  }

}

/******************************************************************************/
static bool IsSLNR(uint8_t uNut)
{
  return uNut == AL_HEVC_NUT_TRAIL_N ||
         uNut == AL_HEVC_NUT_TSA_N ||
         uNut == AL_HEVC_NUT_STSA_N ||
         uNut == AL_HEVC_NUT_RADL_N ||
         uNut == AL_HEVC_NUT_RASL_N ||
         uNut == AL_HEVC_NUT_RSV_VCL_N10 ||
         uNut == AL_HEVC_NUT_RSV_VCL_N12 ||
         uNut == AL_HEVC_NUT_RSV_VCL_N14;
}

/******************************************************************************/
void AL_HEVC_FillSliceParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  /* to speed up memset we don't reset pEntryPointOffsets array */
  Rtos_Memset(pSliceParam, 0, offsetof(AL_TDecSliceParam, pEntryPointOffsets));

  // fast access
  const AL_THevcPps* pPps = pSlice->pPPS;

  pSliceParam->bIsRef = !IsSLNR(pSlice->nal_unit_type);

  pSliceParam->uMaxMergeCand = 5 - pSlice->five_minus_max_num_merge_cand;
  pSliceParam->bColocFromL0 = pSlice->collocated_from_l0_flag;
  pSliceParam->bMvdL1ZeroFlag = pSlice->mvd_l1_zero_flag;
  pSliceParam->bTemporalMvp = (bool)pSlice->slice_temporal_mvp_enable_flag;

  pSliceParam->iCbQpOffset = pSlice->slice_cb_qp_offset;
  pSliceParam->iCrQpOffset = pSlice->slice_cr_qp_offset;
  pSliceParam->iSliceQq = 26 + pPps->init_qp_minus26 + pSlice->slice_qp_delta;
  pSliceParam->uCabacInitIdc = pSlice->cabac_init_flag;

  pSliceParam->eSliceType = (AL_ESliceType)pSlice->slice_type;
  pSliceParam->bDependentSlice = (bool)pSlice->dependent_slice_segment_flag;

  pSliceParam->iTcOffsetDiv2 = pSlice->slice_tc_offset_div2;
  pSliceParam->iBetaOffsetDiv2 = pSlice->slice_beta_offset_div2;
  pSliceParam->bSaoFilterLuma = (bool)pSlice->slice_sao_luma_flag;
  pSliceParam->bSaoFilterChroma = (bool)pSlice->slice_sao_chroma_flag;
  pSliceParam->bDisableLoopFilter = (bool)pSlice->slice_deblocking_filter_disabled_flag;
  pSliceParam->bAcrossSliceLoopFilter = (bool)pSlice->slice_loop_filter_across_slices_enabled_flag;
  pSliceParam->bCuChromaQpOffset = (bool)pSlice->cu_chroma_qp_offset_enabled_flag;
  pSliceParam->uSliceId = pCtx->tCurrentFrameCtx.uNumSlice;

  pSliceParam->uNumRefIdxL0Minus1 = pSlice->num_ref_idx_l0_active_minus1;
  pSliceParam->uNumRefIdxL1Minus1 = pSlice->num_ref_idx_l1_active_minus1;

  pSliceParam->uSliceHeaderLength = pSlice->slice_header_length;

  pSliceParam->uNumEntryPoint = pSlice->num_entry_point_offsets;

  if(pSliceParam->eSliceType == AL_SLICE_CONCEAL)
  {
    // search prev slice
    uint16_t uSliceID = pCtx->tCurrentFrameCtx.uNumSlice;
    AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

    if(pPrevSP)
    {
      pSliceParam->uFirstLcuSliceSegment = pPrevSP->uNextSliceSegment;
      pSliceParam->uFirstLcuSlice = pPrevSP->uNextSliceSegment;
    }
    else
    {
      pSliceParam->uFirstLcuSliceSegment = pCtx->tCurrentFrameCtx.uNumSlice;
      pSliceParam->uFirstLcuSlice = pCtx->tCurrentFrameCtx.uNumSlice;
    }
  }
  else
  {
    pSliceParam->uFirstLcuSliceSegment = pSlice->slice_segment_address;
    pSliceParam->uFirstLcuSlice = pSlice->slice_segment_address;
  }
  pSliceParam->uFirstLcuTileId = pCtx->tCurrentFrameCtx.uCurTileID;

  pSliceParam->bWeightedPred = pPps->weighted_pred_flag;
  pSliceParam->bWeightedBiPred = pPps->weighted_bipred_flag;

  pSliceParam->pEntryPointOffsets[0] = 0;

  for(int32_t i = 1; i <= pSlice->num_entry_point_offsets; ++i)
    pSliceParam->pEntryPointOffsets[i] = pSlice->entry_point_offset_minus1[i] + 1;
}

/*!@}*/
