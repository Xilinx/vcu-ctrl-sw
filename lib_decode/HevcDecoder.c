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

#include <assert.h>

#include "lib_common/Utils.h"
#include "lib_common/HwScalingList.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common_dec/DecError.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/SeiParsing.h"
#include "lib_parsing/VpsParsing.h"
#include "lib_parsing/SpsParsing.h"
#include "lib_parsing/PpsParsing.h"
#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Hevc_PictMngr.h"
#include "lib_parsing/SliceHdrParsing.h"

#include "FrameParam.h"
#include "I_DecoderCtx.h"
#include "DefaultDecoder.h"
#include "SliceDataParsing.h"
#include "NalUnitParserPrivate.h"


/*****************************************************************************/
static int getMaxSlicesCount(int level)
{
  return 600; /* TODO : fix bad behaviour in firmware to decrease dynamically the number of slices */
  switch(level)
  {
  case 10:
  case 20:
    return 16;
  case 21:
    return 20;
  case 30:
    return 30;
  case 31:
    return 40;
  case 40:
  case 41:
    return 75;
  case 50:
  case 51:
  case 52:
    return 200;
  default:
    return 600;
  }
}

/*****************************************************************************/
static void AL_HEVC_sCopyScalingList(AL_THevcPps* pPPS, AL_TScl* pSCL)
{
  Rtos_Memcpy((*pSCL)[0].t4x4Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[0] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][0], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[1] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][1], 16);

  Rtos_Memcpy((*pSCL)[0].t4x4Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[2] ? AL_HEVC_DefaultScalingLists4x4[0] :
              pPPS->scaling_list_param.ScalingList[0][2], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[3] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][3], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[4] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][4], 16);

  Rtos_Memcpy((*pSCL)[1].t4x4Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[5] ? AL_HEVC_DefaultScalingLists4x4[1] :
              pPPS->scaling_list_param.ScalingList[0][5], 16);

  Rtos_Memcpy((*pSCL)[0].t8x8Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[6] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][0], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[7] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][1], 64);

  Rtos_Memcpy((*pSCL)[0].t8x8Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[8] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[1][2], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[9] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][3], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[10] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][4], 64);

  Rtos_Memcpy((*pSCL)[1].t8x8Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[11] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[1][5], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[12] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][0], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[13] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][1], 64);

  Rtos_Memcpy((*pSCL)[0].t16x16Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[14] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[2][2], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Y, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[15] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][3], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cb, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[16] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][4], 64);

  Rtos_Memcpy((*pSCL)[1].t16x16Cr, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[17] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[2][5], 64);

  Rtos_Memcpy((*pSCL)[0].t32x32, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[18] ? AL_HEVC_DefaultScalingLists8x8[0] :
              pPPS->scaling_list_param.ScalingList[3][0], 64);

  Rtos_Memcpy((*pSCL)[1].t32x32, pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[19] ? AL_HEVC_DefaultScalingLists8x8[1] :
              pPPS->scaling_list_param.ScalingList[3][3], 64);

  (*pSCL)[0].tDC[0] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[12] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][0];
  (*pSCL)[0].tDC[1] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[13] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][1];
  (*pSCL)[0].tDC[2] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[14] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][2];
  (*pSCL)[0].tDC[3] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[18] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[1][0];
  (*pSCL)[1].tDC[0] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[15] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][3];
  (*pSCL)[1].tDC[1] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[16] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][4];
  (*pSCL)[1].tDC[2] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[17] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[0][5];
  (*pSCL)[1].tDC[3] = pPPS->scaling_list_param.UseDefaultScalingMatrixFlag[19] ? 16 : pPPS->scaling_list_param.scaling_list_dc_coeff[1][3];
}

/*****************************************************************************/
static void ConcealHevcSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bSliceHdrValid)
{
  pSP->eSliceType = pSlice->slice_type = SLICE_CONCEAL;
  AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP, true);

  pSP->ConcealPicID = AL_PictMngr_GetLastPicID(&pCtx->m_PictMngr);
  pSP->ValidConceal = (pSP->ConcealPicID == uEndOfList) ? false : true;

  if(bSliceHdrValid)
    pSP->FirstLcuSliceSegment = pSP->FirstLcuSlice = pSP->FirstLCU = pSlice->slice_segment_address;

  AL_SET_DEC_OPT(pPP, IntraOnly, 0);
}

/*****************************************************************************/
static void CreateConcealHevcSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, AL_TDecPicBuffers* pBufs)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  ConcealHevcSlice(pCtx, pPP, pSP, pSlice, false);
  AL_InitIntermediateBuffers(pCtx, pBufs);
  pSP->FirstLcuSliceSegment = 0;
  pSP->FirstLcuSlice = 0;
  pSP->FirstLCU = 0;
  pSP->NextSliceSegment = pSlice->slice_segment_address;
  pSP->NumLCU = pSlice->slice_segment_address;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void EndHevcFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, AL_TDecPicParam* pPP, uint8_t pic_output_flag)
{
  AL_HEVC_PictMngr_UpdateRecInfo(&pCtx->m_PictMngr, pSlice->m_pSPS, pPP);
  AL_HEVC_PictMngr_EndFrame(&pCtx->m_PictMngr, pSlice->slice_pic_order_cnt_lsb, eNUT, pSlice, pic_output_flag);

  if(pCtx->m_eDecUnit == AL_AU_UNIT) /* launch HW for each frame(default mode)*/
    AL_LaunchFrameDecoding(pCtx);
  else
    AL_LaunchSliceDecoding(pCtx, true);
  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static void FinishHevcPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_THevcSliceHdr* pSlice = &pCtx->m_HevcSliceHdr[pCtx->m_uCurID];
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice - 1]);

  pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice - 1;
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = pCtx->m_iStreamOffset[(pCtx->m_iNumFrmBlk1 + pCtx->m_iStackSize - 1) % pCtx->m_iStackSize];

  if(pCtx->m_eDecUnit == AL_VCL_NAL_UNIT) /* launch HW for each vcl nal in sub_frame latency*/
    --pCtx->m_PictMngr.m_uNumSlice;

  EndHevcFrame(pCtx, pSlice->nal_unit_type, pSlice, pPP, pSlice->pic_output_flag);
}

/*****************************************************************************/
static void ProcessHevcScalingList(AL_THevcAup* pAUP, AL_THevcSliceHdr* pSlice, AL_TScl* pScl)
{
  int ppsid = pSlice->slice_pic_parameter_set_id;
  int spsid = pAUP->m_pPPS[ppsid].pps_seq_parameter_set_id;

  pAUP->m_pActiveSPS = &pAUP->m_pSPS[spsid];

  AL_CleanupMemory(pScl, sizeof(*pScl));

  // Save ScalingList
  if(pAUP->m_pPPS[ppsid].m_pSPS->scaling_list_enabled_flag && pSlice->first_slice_segment_in_pic_flag)
    AL_HEVC_sCopyScalingList(&pAUP->m_pPPS[ppsid], pScl);
}

/******************************************************************************/
static AL_TCropInfo AL_HEVC_sGetCropInfo(const AL_THevcSps* pSPS)
{
  AL_TCropInfo tCropInfo =
  {
    false, 0, 0, 0, 0
  };

  tCropInfo.bCropping = pSPS->conformance_window_flag;

  if(tCropInfo.bCropping)
  {
    if(pSPS->chroma_format_idc == 1 || pSPS->chroma_format_idc == 2)
    {
      tCropInfo.uCropOffsetLeft += 2 * pSPS->conf_win_left_offset;
      tCropInfo.uCropOffsetRight += 2 * pSPS->conf_win_right_offset;
    }
    else
    {
      tCropInfo.uCropOffsetLeft += pSPS->conf_win_left_offset;
      tCropInfo.uCropOffsetRight += pSPS->conf_win_right_offset;
    }

    if(pSPS->chroma_format_idc == 1)
    {
      tCropInfo.uCropOffsetTop += 2 * pSPS->conf_win_top_offset;
      tCropInfo.uCropOffsetBottom += 2 * pSPS->conf_win_bottom_offset;
    }
    else
    {
      tCropInfo.uCropOffsetTop += pSPS->conf_win_top_offset;
      tCropInfo.uCropOffsetBottom += pSPS->conf_win_bottom_offset;
    }
  }

  return tCropInfo;
}

/*************************************************************************/
static uint8_t AL_HEVC_GetMaxRextBitDepth(AL_TProfilevel const* const pProf)
{
  if(pProf->general_max_8bit_constraint_flag)
    return 8;

  if(pProf->general_max_10bit_constraint_flag)
    return 10;

  if(pProf->general_max_12bit_constraint_flag)
    return 12;
  return 16;
}

/*************************************************************************/
static uint8_t AL_HEVC_GetMaxBitDepth(AL_THevcSps const* pSPS)
{
  AL_TProfilevel const* const pProf = &pSPS->profile_and_level;

  if((pProf->general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt)) || pProf->general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt)])
    return AL_HEVC_GetMaxRextBitDepth(pProf);

  if((pProf->general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)) || pProf->general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)])
    return 8;

  if((pProf->general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)) || pProf->general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)])
    return 8;

  return 10;
}

/******************************************************************************/
static bool AL_HEVC_sInitDecoder(AL_THevcSliceHdr* pSlice, AL_TDecCtx* pCtx)
{
  const AL_THevcSps* pSps = pSlice->m_pSPS;
  const AL_TCropInfo tCropInfo = AL_HEVC_sGetCropInfo(pSps);

  // fast access
  const uint8_t uMaxBitDepth = AL_HEVC_GetMaxBitDepth(pSps);
  const uint16_t uWidth = pSps->pic_width_in_luma_samples;
  const uint16_t uHeight = pSps->pic_height_in_luma_samples;
  pCtx->m_chanParam.iWidth = uWidth;
  pCtx->m_chanParam.iHeight = uHeight;
  const AL_EChromaMode eChromaMode = (AL_EChromaMode)pSps->chroma_format_idc;

  if(eChromaMode > 3)
    return false;

  const uint32_t uSizeMV = AL_GetAllocSize_MV(uWidth, uHeight, pCtx->m_chanParam.eCodec == AL_CODEC_AVC);
  const uint32_t uSizePOC = POCBUFF_PL_SIZE;

  const int level = pSps->profile_and_level.general_level_idc / 3;
  pCtx->m_chanParam.iMaxSlices = getMaxSlicesCount(level);

  uint32_t uSizeWP = pCtx->m_chanParam.iMaxSlices * WP_SLICE_SIZE;
  uint32_t uSizeSP = pCtx->m_chanParam.iMaxSlices * sizeof(AL_TDecSliceParam);

  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    AL_Decoder_Alloc(pCtx, &pCtx->m_PoolWP[i].tMD, uSizeWP);
    AL_CleanupMemory(pCtx->m_PoolWP[i].tMD.pVirtualAddr, pCtx->m_PoolWP[i].tMD.uSize);
    AL_Decoder_Alloc(pCtx, &pCtx->m_PoolSP[i].tMD, uSizeSP);
  }

  // Create Channel
  if(!pCtx->m_bForceFrameRate && pSps->vui_parameters_present_flag && pSps->vui_param.vui_timing_info_present_flag)
  {
    pCtx->m_chanParam.uFrameRate = pSps->vui_param.vui_time_scale;
    pCtx->m_chanParam.uClkRatio = pSps->vui_param.vui_num_units_in_tick;
  }

  AL_TIDecChannelCallbacks CBs = { 0 };
  CBs.endFrameDecodingCB.func = AL_Decoder_EndDecoding;
  CBs.endFrameDecodingCB.userParam = pCtx;

  if(!AL_IDecChannel_Configure(pCtx->m_pDecChannel, &pCtx->m_chanParam, &CBs))
  {
    Rtos_GetMutex(pCtx->m_DecMutex);
    pCtx->m_eChanState = CHAN_INVALID;
    pCtx->m_error = ERR_CHANNEL_NOT_CREATED;
    Rtos_ReleaseMutex(pCtx->m_DecMutex);
    return false;
  }
  pCtx->m_eChanState = CHAN_CONFIGURED;


  // Alloc respect to resolution to limit memory footprint
  const uint32_t uSizeCompData = AL_GetAllocSize_HevcCompData(uWidth, uHeight, eChromaMode);
  const uint32_t uSizeCompMap = AL_GetAllocSize_CompMap(uWidth, uHeight);

  for(int iComp = 0; iComp < pCtx->m_iStackSize; ++iComp)
  {
    if(!AL_Decoder_Alloc(pCtx, &pCtx->m_PoolCompData[iComp].tMD, uSizeCompData))
      goto fail;

    if(!AL_Decoder_Alloc(pCtx, &pCtx->m_PoolCompMap[iComp].tMD, uSizeCompMap))
      goto fail;
  }

  const uint8_t uDpbMaxBuf = AL_HEVC_GetMaxDPBSize(pSps->profile_and_level.general_level_idc, uWidth * uHeight);
  const uint8_t uInterBuf = UnsignedMin(pCtx->m_iStackSize + 1, MAX_STACK);
  const uint8_t uMaxBuf = uDpbMaxBuf + uInterBuf;

  const bool isDpbLowRef = (pCtx->m_eDpbMode == AL_DPB_LOW_REF);
  const uint8_t uDpbMaxRef = isDpbLowRef ? 2 : UnsignedMin(pSps->sps_max_dec_pic_buffering_minus1[pSps->sps_max_sub_layers_minus1] + 1, uDpbMaxBuf);

  assert(pCtx->m_iNumBufHeldByNextComponent >= 1);
  const uint8_t uMaxFrmBuf = (uMaxBuf + pCtx->m_iNumBufHeldByNextComponent);
  const uint8_t uMaxMvBuf = uMaxBuf;

  for(uint8_t u = 0; u < uMaxMvBuf; ++u)
  {
    if(!AL_Decoder_Alloc(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pMvBufs[u].tMD, uSizeMV))
      goto fail;

    if(!AL_Decoder_Alloc(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pPocBufs[u].tMD, uSizePOC))
      goto fail;
  }

  AL_PictMngr_Init(&pCtx->m_PictMngr, pCtx->m_chanParam.eCodec == AL_CODEC_AVC, uWidth, uHeight, uMaxFrmBuf, uMaxMvBuf, uDpbMaxRef, uInterBuf);
  pCtx->m_PictMngr.m_bFirstInit = true;

  if(pCtx->m_resolutionFoundCB.func)
  {
    uint32_t uSizeYuv = AL_PictMngr_GetReferenceSize(uWidth, uHeight, eChromaMode, uMaxBitDepth);


    pCtx->m_resolutionFoundCB.func(uMaxFrmBuf, uSizeYuv, uWidth, uHeight, tCropInfo, AL_GetDecFourCC(eChromaMode, uMaxBitDepth), pCtx->m_resolutionFoundCB.userParam);
  }

  /* verify the number of yuv buffers */
  for(uint8_t uFrm = 0; uFrm < uMaxFrmBuf; ++uFrm)
    assert(pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[uFrm]);

  return true;

  fail:
  pCtx->m_eChanState = CHAN_INVALID;

  return false;
}

/*****************************************************************************/
static int AL_HEVC_sCalculatePOC(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr* pSlice, uint8_t uNoRasOutputFlag)
{
  int32_t POCMsb = 0;
  uint32_t MaxPOCLsb = pSlice->m_pSPS->MaxPicOrderCntLsb;
  uint32_t MaxPOCLsb_div2 = MaxPOCLsb >> 1;

  if((!AL_HEVC_IsBLA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsCRA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsIDR(pSlice->nal_unit_type)) || !uNoRasOutputFlag)
  {
    if((pSlice->slice_pic_order_cnt_lsb < pCtx->m_uPrevPocLSB) &&
       ((pCtx->m_uPrevPocLSB - pSlice->slice_pic_order_cnt_lsb) >= MaxPOCLsb_div2))
      POCMsb = pCtx->m_iPrevPocMSB + MaxPOCLsb;

    else if((pSlice->slice_pic_order_cnt_lsb > pCtx->m_uPrevPocLSB) &&
            ((pSlice->slice_pic_order_cnt_lsb - pCtx->m_uPrevPocLSB) > MaxPOCLsb_div2))
      POCMsb = pCtx->m_iPrevPocMSB - MaxPOCLsb;

    else
      POCMsb = pCtx->m_iPrevPocMSB;
  }

  if(!(pSlice->temporal_id_plus1 - 1) && !AL_HEVC_IsRASL_RADL_SLNR(pSlice->nal_unit_type))
  {
    pCtx->m_uPrevPocLSB = pSlice->slice_pic_order_cnt_lsb;
    pCtx->m_iPrevPocMSB = POCMsb;
  }

  return pSlice->slice_pic_order_cnt_lsb + POCMsb;
}

/*****************************************************************************/
static bool InitHevcSlice(TDecCtx* pCtx, AL_THevcSliceHdr* pSlice)
{
  if(!pCtx->m_PictMngr.m_bFirstInit)
  {
    if(!AL_HEVC_sInitDecoder(pSlice, pCtx))
    {
      pSlice->m_pPPS = &pCtx->m_HevcAup.m_pPPS[pCtx->m_tConceal.m_iLastPPSId];
      pSlice->m_pSPS = pSlice->m_pPPS->m_pSPS;
      return false;
    }
  }
  AL_PictMngr_UpdateDPBInfo(&pCtx->m_PictMngr, pSlice->m_pSPS->sps_max_dec_pic_buffering_minus1[pSlice->m_pSPS->sps_max_sub_layers_minus1] + 1);

  if(!pSlice->dependent_slice_segment_flag)
  {
    if(pSlice->slice_type != SLICE_I && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->m_PictMngr))
      return false;

    if(pSlice->IdrPicFlag)
    {
      pCtx->m_PictMngr.m_iCurFramePOC = 0;
      pCtx->m_PictMngr.m_uPrevPocLSB = 0;
      pCtx->m_PictMngr.m_iPrevPocMSB = 0;
    }
    else if(!pCtx->m_tConceal.m_bValidFrame)
      pCtx->m_PictMngr.m_iCurFramePOC = AL_HEVC_sCalculatePOC(&pCtx->m_PictMngr, pSlice, pCtx->m_uNoRaslOutputFlag);

    if(!pCtx->m_tConceal.m_bValidFrame)
    {
      AL_HEVC_PictMngr_InitRefPictSet(&pCtx->m_PictMngr, pSlice);

      /* at least one active reference on inter slice */
      if(pSlice->slice_type != SLICE_I && !pSlice->NumPocTotalCurr && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->m_PictMngr))
        return false;
    }
  }

  return true;
}

/*************************************************************************//*!
   \brief This function initializes a HEVC Access Unit instance
   \param[in] pAUP Pointer to the Access Unit object we want to initialize
*****************************************************************************/
void AL_HEVC_InitAUP(AL_THevcAup* pAUP)
{
  int i;
  pAUP->m_pActiveSPS = NULL;

  for(i = 0; i < AL_HEVC_MAX_PPS; ++i)
    pAUP->m_pPPS[i].bConceal = true;

  for(i = 0; i < AL_HEVC_MAX_SPS; ++i)
    pAUP->m_pSPS[i].bConceal = true;
}

static bool decodeSliceData(AL_THevcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bFirstSliceInFrameIsValid, int* iNumSlice, bool* bBeginFrameIsValid)
{
  uint8_t uToggleID = (~pCtx->m_uCurID) & 0x01;
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecPicBuffers* pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];

  // ignore RASL picture associated with an IRAP picture that has NoRaslOutputFlag = 1
  if(AL_HEVC_IsRASL(eNUT) && pCtx->m_uNoRaslOutputFlag)
    return SkipNal();

  bool bIsRAP = AL_HEVC_IsCRA(eNUT) || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT) || eNUT == AL_HEVC_NUT_RSV_IRAP_VCL22 || eNUT == AL_HEVC_NUT_RSV_IRAP_VCL23;// CRA, BLA IDR or RAP nal

  if(bIsRAP)
    pCtx->m_uNoRaslOutputFlag = (pCtx->m_bIsFirstPicture || pCtx->m_bLastIsEOS || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT)) ? 1 : 0;

  TCircBuffer* pBufStream = &pCtx->m_Stream;
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pCtx->m_BufNoAE.tMD.pVirtualAddr, &rp);

  // Parse Slice Header
  AL_THevcSliceHdr* pSlice = &pCtx->m_HevcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(*pSlice));
  AL_TConceal* pConceal = &pCtx->m_tConceal;
  bool bRet = AL_HEVC_ParseSliceHeader(pSlice, &pCtx->m_HevcSliceHdr[pCtx->m_uCurID], &rp, pConceal, pAUP->m_pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(bRet)
    bSliceBelongsToSameFrame = (pSlice->first_slice_segment_in_pic_flag || (pSlice->slice_pic_order_cnt_lsb == pCtx->m_uCurPocLsb));

  if(!bSliceBelongsToSameFrame && *bFirstIsValid)
  {
    FinishHevcPreviousFrame(pCtx);

    pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];
    pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
    *bFirstSliceInFrameIsValid = false;
  }

  if(bRet)
  {
    pCtx->m_uCurPocLsb = pSlice->slice_pic_order_cnt_lsb;
    bRet = InitHevcSlice(pCtx, pSlice);
  }

  if(!bRet)
  {
    if(*bFirstIsValid)
    {
      AL_HEVC_PictMngr_RemoveHeadFrame(&pCtx->m_PictMngr);
    }
    else
    {
      pCtx->m_bIsFirstPicture = false;
      return SkipNal();
    }
  }

  if(bRet)
  {
    uint8_t ppsid = pSlice->slice_pic_parameter_set_id;
    uint8_t spsid = pAUP->m_pPPS[ppsid].pps_seq_parameter_set_id;

    if(!pCtx->m_VideoConfiguration.bInit)
      AL_HEVC_UpdateVideoConfiguration(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    bRet = AL_HEVC_IsVideoConfigurationCompatible(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    if(!bRet)
      pAUP->m_pSPS[spsid].bConceal = true;
  }

  if(pSlice->first_slice_segment_in_pic_flag && *bFirstSliceInFrameIsValid)
    bRet = false;

  if(bRet && pSlice->first_slice_segment_in_pic_flag)
    *bFirstSliceInFrameIsValid = true;

  if(bRet && pSlice->slice_type != SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  pCtx->m_uCurID = (pCtx->m_uCurID + 1) & 1;

  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice]);

  if(bIsLastAUNal)
    pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice;

  pBufs->tStream.tMD = pCtx->m_Stream.tMD;

  if(*bFirstSliceInFrameIsValid)
  {
    if(pSlice->first_slice_segment_in_pic_flag && !(*bBeginFrameIsValid))
    {
      bool bClearRef = (bIsRAP && pCtx->m_uNoRaslOutputFlag) ? true : false; // IRAP picture with NoRaslOutputFlag = 1
      bool bNoOutputPrior = (AL_HEVC_IsCRA(eNUT) || ((AL_HEVC_IsIDR(eNUT) || AL_HEVC_IsBLA(eNUT)) && pSlice->no_output_of_prior_pics_flag)) ? true : false;

      AL_HEVC_PictMngr_ClearDPB(&pCtx->m_PictMngr, pSlice->m_pSPS, bClearRef, bNoOutputPrior);
      AL_InitFrameBuffers(pCtx, pSlice->m_pSPS->pic_width_in_luma_samples, pSlice->m_pSPS->pic_height_in_luma_samples, pPP);
      *bBeginFrameIsValid = true;
    }
  }
  else if(pSlice->slice_segment_address)
  {
    if(!pSlice->m_pSPS)
      pSlice->m_pSPS = pAUP->m_pActiveSPS;
    CreateConcealHevcSlice(pCtx, pPP, pSP, pSlice, pBufs);

    if(!bSliceBelongsToSameFrame)
      AL_InitFrameBuffers(pCtx, pSlice->m_pSPS->pic_width_in_luma_samples, pSlice->m_pSPS->pic_height_in_luma_samples, pPP);

    pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[++pCtx->m_PictMngr.m_uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(pSlice->slice_type != SLICE_I && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->m_PictMngr))
    bRet = false;

  bool bLastSlice = *iNumSlice >= pCtx->m_chanParam.iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    bRet = false;

  AL_TScl ScalingList;

  if(bRet)
  {
    /*check if the first Access Unit is a random access point*/
    if(!(*bFirstIsValid))
    {
      if(bIsRAP)
      {
        *bFirstIsValid = true;
        pCtx->m_uMaxBD = AL_HEVC_GetMaxBitDepth(pSlice->m_pSPS);
      }
      else
      {
        pCtx->m_bIsFirstPicture = false;
        return false;
      }
    }

    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    ProcessHevcScalingList(pAUP, pSlice, &ScalingList);

    if(!pCtx->m_PictMngr.m_uNumSlice)
    {
      AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
      AL_InitIntermediateBuffers(pCtx, pBufs);
    }
    AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP, false);

    if(!AL_HEVC_PictMngr_BuildPictureList(&pCtx->m_PictMngr, pSlice, &pCtx->m_ListRef))
      ConcealHevcSlice(pCtx, pPP, pSP, pSlice, true);
    else
    {
      AL_HEVC_FillSlicePicIdRegister(pSlice, pCtx, pPP, pSP);
      pConceal->m_bValidFrame = true;
      pSP->ConcealPicID = AL_PictMngr_GetLastPicID(&pCtx->m_PictMngr);
      pSP->ValidConceal = (pSP->ConcealPicID == uEndOfList) ? false : true;
    }
  }
  else if((bIsLastAUNal || pSlice->first_slice_segment_in_pic_flag || bLastSlice) && (*bFirstIsValid) && (*bFirstSliceInFrameIsValid)) /* conceal the current slice data */
  {
    ConcealHevcSlice(pCtx, pPP, pSP, pSlice, true);

    if(!pCtx->m_PictMngr.m_uNumSlice)
      AL_InitIntermediateBuffers(pCtx, pBufs);

    if(bLastSlice)
      pSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
  }
  else /* skip slice */
  {
    if(bIsLastAUNal)
    {
      if(*bBeginFrameIsValid)
        AL_PictMngr_CancelFrame(&pCtx->m_PictMngr);

      UpdateContextAtEndOfFrame(pCtx);
    }

    return SkipNal();
  }

  // Launch slice decoding
  AL_HEVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, bRet);

  ++pCtx->m_PictMngr.m_uNumSlice;
  ++(*iNumSlice);

  if(bIsLastAUNal || bLastSlice)
  {
    uint8_t pic_output_flag = (AL_HEVC_IsRASL(eNUT) && pCtx->m_uNoRaslOutputFlag) ? 0 : pSlice->pic_output_flag;
    EndHevcFrame(pCtx, eNUT, pSlice, pPP, pic_output_flag);
    return true;
  }
  else if(pCtx->m_eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, bIsLastAUNal);

  return false;
}

static bool isSliceData(AL_ENut nut)
{
  switch(nut)
  {
  case AL_HEVC_NUT_TRAIL_N:
  case AL_HEVC_NUT_TRAIL_R:
  case AL_HEVC_NUT_TSA_N:
  case AL_HEVC_NUT_TSA_R:
  case AL_HEVC_NUT_STSA_N:
  case AL_HEVC_NUT_STSA_R:
  case AL_HEVC_NUT_RADL_N:
  case AL_HEVC_NUT_RADL_R:
  case AL_HEVC_NUT_RASL_N:
  case AL_HEVC_NUT_RASL_R:
  case AL_HEVC_NUT_BLA_W_LP:
  case AL_HEVC_NUT_BLA_W_RADL:
  case AL_HEVC_NUT_BLA_N_LP:
  case AL_HEVC_NUT_IDR_W_RADL:
  case AL_HEVC_NUT_IDR_N_LP:
  case AL_HEVC_NUT_CRA:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
bool AL_HEVC_DecodeOneNAL(AL_THevcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bFirstSliceInFrameIsValid, int* iNumSlice, bool* bBeginFrameIsValid)
{
  if(isSliceData(eNUT))
    return decodeSliceData(pAUP, pCtx, eNUT, bIsLastAUNal, bFirstIsValid, bFirstSliceInFrameIsValid, iNumSlice, bBeginFrameIsValid);

  AL_TRbspParser rp = getParserOnNonVclNal(pCtx);
  switch(eNUT)
  {
  // Parser SEI
  case AL_HEVC_NUT_PREFIX_SEI:
  case AL_HEVC_NUT_SUFFIX_SEI:
  {
    break;
  }

  // Parser VPS
  case AL_HEVC_NUT_VPS:
  {
    ParseVPS(pAUP->m_pVPS, &rp);
    break;
  }

  // Parser SPS
  case AL_HEVC_NUT_SPS:
  {
    uint8_t SpsId;
    AL_HEVC_ParseSPS(pAUP->m_pSPS, &rp, pAUP->m_pVPS, &pCtx->m_VideoConfiguration, &SpsId);
    break;
  }

  // Parser PPS
  case AL_HEVC_NUT_PPS:
  {
    uint8_t LastPicId;
    AL_HEVC_ParsePPS(pAUP->m_pPPS, &rp, pAUP->m_pSPS, &LastPicId);

    if(!pAUP->m_pPPS[LastPicId].bConceal && pCtx->m_tConceal.m_iLastPPSId <= LastPicId)
      pCtx->m_tConceal.m_iLastPPSId = LastPicId;
    break;
  }

  // End of Sequence
  case AL_HEVC_NUT_EOS:
  {
    pCtx->m_bIsFirstPicture = true;
    pCtx->m_bLastIsEOS = true;
    break;
  }

  // no processing on other nal units
  default:
    break;
  }

  return false;
}

