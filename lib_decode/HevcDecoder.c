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
#include "lib_common/Error.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/HevcParser.h"
#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Hevc_PictMngr.h"
#include "lib_parsing/SliceHdrParsing.h"

#include "FrameParam.h"
#include "I_DecoderCtx.h"
#include "DefaultDecoder.h"
#include "SliceDataParsing.h"
#include "NalUnitParserPrivate.h"
#include "NalDecoder.h"


/******************************************************************************/
static AL_TCropInfo getCropInfo(AL_THevcSps const* pSPS)
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
static uint8_t getMaxRextBitDepth(AL_TProfilevel pf)
{
  if(pf.general_max_8bit_constraint_flag)
    return 8;

  if(pf.general_max_10bit_constraint_flag)
    return 10;

  if(pf.general_max_12bit_constraint_flag)
    return 12;
  return 16;
}

/*************************************************************************/
static int getMaxBitDepth(AL_TProfilevel pf)
{
  if((pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt)) || pf.general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt)])
    return getMaxRextBitDepth(pf);

  if((pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)) || pf.general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN)])
    return 8;

  if((pf.general_profile_idc == AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)) || pf.general_profile_compatibility_flag[AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_MAIN_STILL)])
    return 8;

  return 10;
}

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
static int calculatePOC(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr* pSlice, uint8_t uNoRasOutputFlag)
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
static bool isFirstSPSCompatibleWithStreamSettings(AL_THevcSps const* pSPS, AL_TStreamSettings const* pStreamSettings)
{
  const int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_and_level);

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSMaxBitDepth))
    return false;

  const int iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;

  if((pStreamSettings->iLevel > 0) && (pStreamSettings->iLevel < iSPSLevel))
    return false;

  const AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if((pStreamSettings->eChroma != CHROMA_MAX_ENUM) && (pStreamSettings->eChroma < eSPSChromaMode))
    return false;

  const AL_TCropInfo tSPSCropInfo = getCropInfo(pSPS);
  const int iSPSCropWidth = tSPSCropInfo.uCropOffsetLeft + tSPSCropInfo.uCropOffsetRight;
  const AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  if((pStreamSettings->tDim.iWidth > 0) && (pStreamSettings->tDim.iWidth < (tSPSDim.iWidth - iSPSCropWidth)))
    return false;

  const int iSPSCropHeight = tSPSCropInfo.uCropOffsetTop + tSPSCropInfo.uCropOffsetBottom;

  if((pStreamSettings->tDim.iHeight > 0) && (pStreamSettings->tDim.iHeight < (tSPSDim.iHeight - iSPSCropHeight)))
    return false;

  return true;
}

int HEVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack);

/*****************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  const AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };
  const int iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;
  const int iSPSMaxSlices = getMaxSlicesCount(iSPSLevel);
  const int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  const int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  const AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;
  const int iSizeCompData = AL_GetAllocSize_HevcCompData(tSPSDim, eSPSChromaMode);
  const int iSizeCompMap = AL_GetAllocSize_CompMap(tSPSDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  const int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(iSPSLevel, tSPSDim.iWidth, tSPSDim.iHeight, pCtx->m_eDpbMode);
  const int iMaxBuf = HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->m_iStackSize);

  const int iSizeMV = AL_GetAllocSize_HevcMV(tSPSDim);
  const int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  const int iDpbRef = Min(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  const AL_EFbStorageMode eStorageMode = pCtx->m_chanParam.eFBStorageMode;
  AL_PictMngr_Init(&pCtx->m_PictMngr, iMaxBuf, iSizeMV, iDpbRef, pCtx->m_eDpbMode, eStorageMode);

  if(pCtx->m_resolutionFoundCB.func)
  {
    const int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_and_level);
    const bool useFBC = pCtx->m_chanParam.bFrameBufferCompression;
    const int iSizeYuv = AL_GetAllocSize_Frame(tSPSDim, eSPSChromaMode, iSPSMaxBitDepth, useFBC, eStorageMode);
    const AL_TCropInfo tCropInfo = getCropInfo(pSPS);

    AL_TStreamSettings tSettings;
    tSettings.tDim = tSPSDim;
    tSettings.eChroma = eSPSChromaMode;
    tSettings.iBitDepth = iSPSMaxBitDepth;
    tSettings.iLevel = iSPSLevel;
    tSettings.iProfileIdc = pSPS->profile_and_level.general_profile_idc;

    pCtx->m_resolutionFoundCB.func(iMaxBuf, iSizeYuv, &tSettings, &tCropInfo, pCtx->m_resolutionFoundCB.userParam);
  }

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, AL_ERR_NO_MEMORY);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  const AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };
  AL_TDecChanParam* pChan = &pCtx->m_chanParam;
  pChan->iWidth = tSPSDim.iWidth;
  pChan->iHeight = tSPSDim.iHeight;
  const int iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;
  const int iSPSMaxSlices = getMaxSlicesCount(iSPSLevel);
  pChan->iMaxSlices = iSPSMaxSlices;

  if(!pCtx->m_bForceFrameRate && pSPS->vui_parameters_present_flag && pSPS->vui_param.vui_timing_info_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale;
    pChan->uClkRatio = pSPS->vui_param.vui_num_units_in_tick;
  }

  AL_CB_EndFrameDecoding endFrameDecodingCallback = { AL_Default_Decoder_EndDecoding, pCtx };
  AL_ERR eError = AL_IDecChannel_Configure(pCtx->m_pDecChannel, &pCtx->m_chanParam, endFrameDecodingCallback);

  if(eError != AL_SUCCESS)
  {
    AL_Default_Decoder_SetError(pCtx, eError);
    pCtx->m_eChanState = CHAN_INVALID;
    return false;
  }


  pCtx->m_eChanState = CHAN_CONFIGURED;

  return true;
}

/*****************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice)
{
  AL_THevcAup* aup = &pCtx->m_aup.hevcAup;

  if(!pCtx->m_bIsFirstSPSChecked)
  {
    if(!isFirstSPSCompatibleWithStreamSettings(pSlice->m_pSPS, &pCtx->m_tStreamSettings))
    {
      pSlice->m_pPPS = &aup->m_pPPS[pCtx->m_tConceal.m_iLastPPSId];
      pSlice->m_pSPS = pSlice->m_pPPS->m_pSPS;
      return false;
    }

    pCtx->m_bIsFirstSPSChecked = true;

    if(!pCtx->m_bIsBuffersAllocated)
    {
      if(!allocateBuffers(pCtx, pSlice->m_pSPS))
        return false;

      pCtx->m_bIsBuffersAllocated = true;
    }

    if(!initChannel(pCtx, pSlice->m_pSPS))
      return false;
  }

  int ppsid = pSlice->slice_pic_parameter_set_id;
  int spsid = aup->m_pPPS[ppsid].pps_seq_parameter_set_id;

  aup->m_pActiveSPS = &aup->m_pSPS[spsid];

  const AL_TDimension tDim = { pSlice->m_pSPS->pic_width_in_luma_samples, pSlice->m_pSPS->pic_height_in_luma_samples };
  const int iLevel = pSlice->m_pSPS->profile_and_level.general_level_idc / 3;
  const int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(iLevel, tDim.iWidth, tDim.iHeight, pCtx->m_eDpbMode);
  const int iDpbRef = Min(pSlice->m_pSPS->sps_max_dec_pic_buffering_minus1[pSlice->m_pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  AL_PictMngr_UpdateDPBInfo(&pCtx->m_PictMngr, iDpbRef);

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
      pCtx->m_PictMngr.m_iCurFramePOC = calculatePOC(&pCtx->m_PictMngr, pSlice, pCtx->m_uNoRaslOutputFlag);

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

/*****************************************************************************/
static void copyScalingList(AL_THevcPps* pPPS, AL_TScl* pSCL)
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
static void processScalingList(AL_THevcAup* pAUP, AL_THevcSliceHdr* pSlice, AL_TScl* pScl)
{
  int ppsid = pSlice->slice_pic_parameter_set_id;

  AL_CleanupMemory(pScl, sizeof(*pScl));

  // Save ScalingList
  if(pAUP->m_pPPS[ppsid].m_pSPS->scaling_list_enabled_flag && pSlice->first_slice_segment_in_pic_flag)
    copyScalingList(&pAUP->m_pPPS[ppsid], pScl);
}

/*****************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bSliceHdrValid)
{
  pSP->eSliceType = pSlice->slice_type = SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT);

  AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP, true);

  AL_SetConcealParameters(pCtx, pSP);

  if(bSliceHdrValid)
    pSP->FirstLcuSliceSegment = pSP->FirstLcuSlice = pSP->FirstLCU = pSlice->slice_segment_address;

  AL_SET_DEC_OPT(pPP, IntraOnly, 0);
}

/*****************************************************************************/
static void createConcealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  concealSlice(pCtx, pPP, pSP, pSlice, false);
  pSP->FirstLcuSliceSegment = 0;
  pSP->FirstLcuSlice = 0;
  pSP->FirstLCU = 0;
  pSP->NextSliceSegment = pSlice->slice_segment_address;
  pSP->NumLCU = pSlice->slice_segment_address;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, AL_TDecPicParam* pPP, uint8_t pic_output_flag)
{
  AL_HEVC_PictMngr_UpdateRecInfo(&pCtx->m_PictMngr, pSlice->m_pSPS, pPP, pCtx->m_chanParam.eFBStorageMode);
  AL_HEVC_PictMngr_EndFrame(&pCtx->m_PictMngr, pSlice->slice_pic_order_cnt_lsb, eNUT, pSlice, pic_output_flag);

  if(pCtx->m_chanParam.eDecUnit == AL_AU_UNIT) /* launch HW for each frame(default mode)*/
    AL_LaunchFrameDecoding(pCtx);
  else
    AL_LaunchSliceDecoding(pCtx, true);
  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_THevcSliceHdr* pSlice = &pCtx->m_HevcSliceHdr[pCtx->m_uCurID];
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice - 1]);

  pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice - 1;
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = pCtx->m_iStreamOffset[(pCtx->m_iNumFrmBlk1 + pCtx->m_iStackSize - 1) % pCtx->m_iStackSize];

  if(pCtx->m_chanParam.eDecUnit == AL_VCL_NAL_UNIT) /* launch HW for each vcl nal in sub_frame latency*/
    --pCtx->m_PictMngr.m_uNumSlice;

  endFrame(pCtx, pSlice->nal_unit_type, pSlice, pPP, pSlice->pic_output_flag);
}

/*****************************************************************************/
static bool decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  bool* bBeginFrameIsValid = &pCtx->m_bBeginFrameIsValid;
  bool* bFirstIsValid = &pCtx->m_bFirstIsValid;
  bool* bFirstSliceInFrameIsValid = &pCtx->m_bFirstSliceInFrameIsValid;

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
  uint8_t uToggleID = (~pCtx->m_uCurID) & 0x01;
  AL_THevcSliceHdr* pSlice = &pCtx->m_HevcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(*pSlice));
  AL_TConceal* pConceal = &pCtx->m_tConceal;
  AL_THevcAup* pAUP = &pIAUP->hevcAup;
  bool isValid = AL_HEVC_ParseSliceHeader(pSlice, &pCtx->m_HevcSliceHdr[pCtx->m_uCurID], &rp, pConceal, pAUP->m_pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(isValid)
    bSliceBelongsToSameFrame = (pSlice->first_slice_segment_in_pic_flag || (pSlice->slice_pic_order_cnt_lsb == pCtx->m_uCurPocLsb));

  AL_TDecPicBuffers* pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];

  if(!bSliceBelongsToSameFrame && *bFirstIsValid && *bFirstSliceInFrameIsValid)
  {
    finishPreviousFrame(pCtx);

    pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];
    pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
    *bFirstSliceInFrameIsValid = false;
    *bBeginFrameIsValid = false;
  }

  if(isValid)
  {
    pCtx->m_uCurPocLsb = pSlice->slice_pic_order_cnt_lsb;
    isValid = initSlice(pCtx, pSlice);
  }

  if(!isValid)
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

  if(isValid)
  {
    uint8_t ppsid = pSlice->slice_pic_parameter_set_id;
    uint8_t spsid = pAUP->m_pPPS[ppsid].pps_seq_parameter_set_id;

    if(!pCtx->m_VideoConfiguration.bInit)
      AL_HEVC_UpdateVideoConfiguration(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    isValid = AL_HEVC_IsVideoConfigurationCompatible(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    if(!isValid)
      pAUP->m_pSPS[spsid].bConceal = true;
  }

  if(pSlice->first_slice_segment_in_pic_flag && *bFirstSliceInFrameIsValid)
    isValid = false;

  if(isValid && pSlice->first_slice_segment_in_pic_flag)
    *bFirstSliceInFrameIsValid = true;

  if(isValid && pSlice->slice_type != SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  pCtx->m_uCurID = (pCtx->m_uCurID + 1) & 1;

  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice]);

  if(bIsLastAUNal)
    pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice;

  pBufs->tStream.tMD = pCtx->m_Stream.tMD;

  if(!pSlice->m_pPPS)
    pSlice->m_pPPS = &pAUP->m_pPPS[pConceal->m_iLastPPSId];

  if(!pSlice->m_pSPS)
    pSlice->m_pSPS = pAUP->m_pActiveSPS;

  if(*bFirstSliceInFrameIsValid)
  {
    if(pSlice->first_slice_segment_in_pic_flag && !(*bBeginFrameIsValid))
    {
      bool bClearRef = (bIsRAP && pCtx->m_uNoRaslOutputFlag) ? true : false; // IRAP picture with NoRaslOutputFlag = 1
      bool bNoOutputPrior = (AL_HEVC_IsCRA(eNUT) || ((AL_HEVC_IsIDR(eNUT) || AL_HEVC_IsBLA(eNUT)) && pSlice->no_output_of_prior_pics_flag)) ? true : false;

      AL_HEVC_PictMngr_ClearDPB(&pCtx->m_PictMngr, pSlice->m_pSPS, bClearRef, bNoOutputPrior);
    }
  }
  else if(pSlice->slice_segment_address)
  {
    createConcealSlice(pCtx, pPP, pSP, pSlice);

    pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[++pCtx->m_PictMngr.m_uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(pSlice->slice_type != SLICE_I && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->m_PictMngr))
    isValid = false;

  if(!(*bBeginFrameIsValid) && pSlice->m_pSPS)
  {
    AL_TDimension const tDim = { pSlice->m_pSPS->pic_width_in_luma_samples, pSlice->m_pSPS->pic_height_in_luma_samples };
    AL_InitFrameBuffers(pCtx, pBufs, tDim, pPP);
    *bBeginFrameIsValid = true;
  }

  bool bLastSlice = *iNumSlice >= pCtx->m_chanParam.iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    isValid = false;

  AL_TScl ScalingList;

  if(isValid)
  {
    /*check if the first Access Unit is a random access point*/
    if(!(*bFirstIsValid))
    {
      if(bIsRAP)
      {
        *bFirstIsValid = true;
        pCtx->m_uMaxBD = getMaxBitDepth(pSlice->m_pSPS->profile_and_level);
      }
      else
      {
        pCtx->m_bIsFirstPicture = false;
        return false;
      }
    }

    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(!pCtx->m_PictMngr.m_uNumSlice)
    {
      AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
    }
    AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP, false);

    if(!AL_HEVC_PictMngr_BuildPictureList(&pCtx->m_PictMngr, pSlice, &pCtx->m_ListRef))
    {
      concealSlice(pCtx, pPP, pSP, pSlice, true);
    }
    else
    {
      AL_HEVC_FillSlicePicIdRegister(pSlice, pCtx, pPP, pSP);
      pConceal->m_bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSP);
    }
  }
  else if((bIsLastAUNal || pSlice->first_slice_segment_in_pic_flag || bLastSlice) && (*bFirstIsValid) && (*bFirstSliceInFrameIsValid)) /* conceal the current slice data */
  {
    concealSlice(pCtx, pPP, pSP, pSlice, true);

    if(bLastSlice)
      pSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
  }
  else // skip slice
  {
    if(bIsLastAUNal)
    {
      if(*bBeginFrameIsValid)
        AL_CancelFrameBuffers(pCtx);

      UpdateContextAtEndOfFrame(pCtx);
    }

    return SkipNal();
  }

  // Launch slice decoding
  AL_HEVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pCtx->m_PictMngr.m_uNumSlice;
  ++(*iNumSlice);

  if(bIsLastAUNal || bLastSlice)
  {
    uint8_t pic_output_flag = (AL_HEVC_IsRASL(eNUT) && pCtx->m_uNoRaslOutputFlag) ? 0 : pSlice->pic_output_flag;
    endFrame(pCtx, eNUT, pSlice, pPP, pic_output_flag);
    return true;
  }
  else if(pCtx->m_chanParam.eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, bIsLastAUNal);

  return false;
}

/*****************************************************************************/
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

static AL_PARSE_RESULT parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  AL_THevcAup* aup = &IAup->hevcAup;
  uint8_t LastPicId;
  AL_HEVC_ParsePPS(IAup, rp, &LastPicId);

  if(!aup->m_pPPS[LastPicId].bConceal && pCtx->m_tConceal.m_iLastPPSId <= LastPicId)
    pCtx->m_tConceal.m_iLastPPSId = LastPicId;

  return AL_OK;
}

/*****************************************************************************/
bool AL_HEVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  AL_NonVclNuts nuts =
  {
    0,
    AL_HEVC_NUT_SPS,
    AL_HEVC_NUT_PPS,
    AL_HEVC_NUT_VPS,
    AL_HEVC_NUT_EOS,
  };

  AL_NalParser parser =
  {
    AL_HEVC_ParseSPS,
    parsePPSandUpdateConcealment,
    ParseVPS,
    NULL,
    decodeSliceData,
    isSliceData,
  };

  return AL_DecodeOneNal(nuts, parser, pAUP, pCtx, eNUT, bIsLastAUNal, iNumSlice);
}

/*****************************************************************************/
void AL_HEVC_InitAUP(AL_THevcAup* pAUP)
{
  for(int i = 0; i < AL_HEVC_MAX_PPS; ++i)
    pAUP->m_pPPS[i].bConceal = true;

  for(int i = 0; i < AL_HEVC_MAX_SPS; ++i)
    pAUP->m_pSPS[i].bConceal = true;

  pAUP->m_pActiveSPS = NULL;
}

