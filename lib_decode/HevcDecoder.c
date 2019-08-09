/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/Defines_mcu.h"

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
static AL_TCropInfo extractCropInfo(AL_THevcSps const* pSPS)
{
  AL_TCropInfo tCropInfo = { false, 0, 0, 0, 0 };

  if(!pSPS->conformance_window_flag)
    return tCropInfo;

  tCropInfo.bCropping = true;

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
  uint32_t MaxPOCLsb = pSlice->pSPS->MaxPicOrderCntLsb;
  uint32_t MaxPOCLsb_div2 = MaxPOCLsb >> 1;

  if((!AL_HEVC_IsBLA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsCRA(pSlice->nal_unit_type) &&
      !AL_HEVC_IsIDR(pSlice->nal_unit_type)) || !uNoRasOutputFlag)
  {
    if((pSlice->slice_pic_order_cnt_lsb < pCtx->uPrevPocLSB) &&
       ((pCtx->uPrevPocLSB - pSlice->slice_pic_order_cnt_lsb) >= MaxPOCLsb_div2))
      POCMsb = pCtx->iPrevPocMSB + MaxPOCLsb;

    else if((pSlice->slice_pic_order_cnt_lsb > pCtx->uPrevPocLSB) &&
            ((pSlice->slice_pic_order_cnt_lsb - pCtx->uPrevPocLSB) > MaxPOCLsb_div2))
      POCMsb = pCtx->iPrevPocMSB - MaxPOCLsb;

    else
      POCMsb = pCtx->iPrevPocMSB;
  }

  if(!(pSlice->temporal_id_plus1 - 1) && !AL_HEVC_IsRASL_RADL_SLNR(pSlice->nal_unit_type))
  {
    pCtx->uPrevPocLSB = pSlice->slice_pic_order_cnt_lsb;
    pCtx->iPrevPocMSB = POCMsb;
  }

  return pSlice->slice_pic_order_cnt_lsb + POCMsb;
}

/*****************************************************************************/
static AL_ESequenceMode getSequenceMode(AL_THevcSps const* pSPS)
{
  AL_TProfilevel const* pProfileLevel = &pSPS->profile_and_level;

  if(pSPS->vui_parameters_present_flag)
  {
    AL_TVuiParam const* pVUI = &pSPS->vui_param;

    if(pVUI->field_seq_flag)
      return AL_SM_INTERLACED;
  }

  if(pSPS->profile_and_level.general_frame_only_constraint_flag)
    return AL_SM_PROGRESSIVE;

  if((pProfileLevel->general_progressive_source_flag == 0) && (pProfileLevel->general_interlaced_source_flag == 0))
    return AL_SM_UNKNOWN;

  if(pProfileLevel->general_progressive_source_flag && (pProfileLevel->general_interlaced_source_flag == 0))
    return AL_SM_PROGRESSIVE;

  if((pProfileLevel->general_progressive_source_flag == 0) && pProfileLevel->general_interlaced_source_flag)
    return AL_SM_INTERLACED;

  if(pProfileLevel->general_progressive_source_flag && pProfileLevel->general_interlaced_source_flag)
    return AL_SM_MAX_ENUM;

  return AL_SM_MAX_ENUM;
}

/******************************************************************************/
static AL_TStreamSettings extractStreamSettings(AL_THevcSps const* pSPS)
{
  AL_TStreamSettings tStreamSettings;
  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };
  tStreamSettings.tDim = tSPSDim;
  tStreamSettings.eChroma = (AL_EChromaMode)pSPS->chroma_format_idc;
  tStreamSettings.iBitDepth = getMaxBitDepth(pSPS->profile_and_level);
  tStreamSettings.iLevel = pSPS->profile_and_level.general_level_idc / 3;
  tStreamSettings.iProfileIdc = pSPS->profile_and_level.general_profile_idc;
  tStreamSettings.eSequenceMode = getSequenceMode(pSPS);
  assert(tStreamSettings.eSequenceMode != AL_SM_MAX_ENUM);
  return tStreamSettings;
}

/*****************************************************************************/
int HEVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack);

/*****************************************************************************/
static AL_ERR resolutionFound(AL_TDecCtx* pCtx, AL_TStreamSettings const* pSpsSettings, AL_TCropInfo const* pCropInfo)
{
  int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(pSpsSettings->iLevel, pSpsSettings->tDim.iWidth, pSpsSettings->tDim.iHeight);
  int iMaxBuf = HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->iStackSize);
  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);
  int iSizeYuv = AL_GetAllocSize_Frame(pSpsSettings->tDim, pSpsSettings->eChroma, pSpsSettings->iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);

  return pCtx->resolutionFoundCB.func(iMaxBuf, iSizeYuv, pSpsSettings, pCropInfo, pCtx->resolutionFoundCB.userParam);
}

/*****************************************************************************/
static bool isSPSCompatibleWithStreamSettings(AL_THevcSps const* pSPS, AL_TStreamSettings const* pStreamSettings)
{
  int iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;

  if(iSPSLumaBitDepth > HW_IP_BIT_DEPTH)
    return false;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSLumaBitDepth))
    return false;

  int iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;

  if(iSPSChromaBitDepth > HW_IP_BIT_DEPTH)
    return false;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSChromaBitDepth))
    return false;

  int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_and_level);

  if(iSPSMaxBitDepth > HW_IP_BIT_DEPTH)
    return false;

  int iSPSLevel = pSPS->profile_and_level.general_level_idc / 3;

  if((pStreamSettings->iLevel > 0) && (pStreamSettings->iLevel < iSPSLevel))
    return false;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if((pStreamSettings->eChroma != AL_CHROMA_MAX_ENUM) && (pStreamSettings->eChroma < eSPSChromaMode))
    return false;

  AL_TCropInfo tSPSCropInfo = extractCropInfo(pSPS);

  int iSPSCropWidth = tSPSCropInfo.uCropOffsetLeft + tSPSCropInfo.uCropOffsetRight;
  AL_TDimension tSPSDim = { pSPS->pic_width_in_luma_samples, pSPS->pic_height_in_luma_samples };

  if((pStreamSettings->tDim.iWidth > 0) && (pStreamSettings->tDim.iWidth < (tSPSDim.iWidth - iSPSCropWidth)))
    return false;

  if((pStreamSettings->tDim.iWidth > 0) && (tSPSDim.iWidth < AL_CORE_HEVC_MIN_WIDTH))
    return false;

  int iSPSCropHeight = tSPSCropInfo.uCropOffsetTop + tSPSCropInfo.uCropOffsetBottom;

  if((pStreamSettings->tDim.iHeight > 0) && (pStreamSettings->tDim.iHeight < (tSPSDim.iHeight - iSPSCropHeight)))
    return false;

  AL_ESequenceMode sequenceMode = getSequenceMode(pSPS);

  if(sequenceMode == AL_SM_MAX_ENUM)
    return false;

  if(sequenceMode != AL_SM_UNKNOWN)
  {
    if(((pStreamSettings->eSequenceMode != AL_SM_MAX_ENUM) && pStreamSettings->eSequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != sequenceMode))
      return false;
  }

  return true;
}

/*****************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  pCtx->tStreamSettings = extractStreamSettings(pSPS);
  int iSPSMaxSlices = getMaxSlicesCount(pCtx->tStreamSettings.iLevel);
  int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int iSizeCompData = AL_GetAllocSize_HevcCompData(pCtx->tStreamSettings.tDim, pCtx->tStreamSettings.eChroma);
  int iSizeCompMap = AL_GetAllocSize_DecCompMap(pCtx->tStreamSettings.tDim);
  AL_ERR error = AL_ERR_NO_MEMORY;

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(pCtx->tStreamSettings.iLevel, pCtx->tStreamSettings.tDim.iWidth, pCtx->tStreamSettings.tDim.iHeight);
  int iMaxBuf = HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->iStackSize);
  int iSizeMV = AL_GetAllocSize_HevcMV(pCtx->tStreamSettings.tDim);
  int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  int iDpbRef = Min(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  bool bEnableRasterOutput = pCtx->chanParam.eBufferOutputMode != AL_OUTPUT_INTERNAL;

  AL_PictMngr_Init(&pCtx->PictMngr, pCtx->pAllocator, iMaxBuf, iSizeMV, iDpbRef, pCtx->eDpbMode, pCtx->chanParam.eFBStorageMode, pCtx->tStreamSettings.iBitDepth, bEnableRasterOutput, pCtx->chanParam.bUseEarlyCallback);

  AL_TCropInfo tCropInfo = extractCropInfo(pSPS);
  error = resolutionFound(pCtx, &pCtx->tStreamSettings, &tCropInfo);

  if(AL_IS_ERROR_CODE(error))
    goto fail_alloc;

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_THevcSps const* pSPS)
{
  AL_TDecChanParam* pChan = &pCtx->chanParam;
  pChan->iWidth = pCtx->tStreamSettings.tDim.iWidth;
  pChan->iHeight = pCtx->tStreamSettings.tDim.iHeight;

  const int iSPSMaxSlices = getMaxSlicesCount(pCtx->tStreamSettings.iLevel);
  pChan->iMaxSlices = iSPSMaxSlices;

  if(!pCtx->bForceFrameRate && pSPS->vui_parameters_present_flag && pSPS->vui_param.vui_timing_info_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale;
    pChan->uClkRatio = pSPS->vui_param.vui_num_units_in_tick;
  }

  AL_CB_EndFrameDecoding endFrameDecodingCallback = { AL_Default_Decoder_EndDecoding, pCtx };
  AL_ERR eError = AL_IDecChannel_Configure(pCtx->pDecChannel, &pCtx->chanParam, endFrameDecodingCallback);

  if(AL_IS_ERROR_CODE(eError))
  {
    AL_Default_Decoder_SetError(pCtx, eError, -1);
    pCtx->eChanState = CHAN_INVALID;
    return false;
  }


  pCtx->eChanState = CHAN_CONFIGURED;

  return true;
}

/******************************************************************************/
static int slicePpsId(AL_THevcSliceHdr const* pSlice)
{
  return pSlice->slice_pic_parameter_set_id;
}

/******************************************************************************/
static int sliceSpsId(AL_THevcPps const* pPps, AL_THevcSliceHdr const* pSlice)
{
  int const ppsid = slicePpsId(pSlice);
  return pPps[ppsid].pps_seq_parameter_set_id;
}

/*****************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_THevcSliceHdr* pSlice)
{
  AL_THevcAup* aup = &pCtx->aup.hevcAup;

  if(!pCtx->bIsFirstSPSChecked)
  {
    if(!isSPSCompatibleWithStreamSettings(pSlice->pSPS, &pCtx->tStreamSettings))
    {
      pSlice->pPPS = &aup->pPPS[pCtx->tConceal.iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS, -1);
      return false;
    }

    pCtx->bIsFirstSPSChecked = true;

    if(!pCtx->bIsBuffersAllocated)
    {
      if(!allocateBuffers(pCtx, pSlice->pSPS))
        return false;
    }

    pCtx->bIsBuffersAllocated = true;

    if(!initChannel(pCtx, pSlice->pSPS))
      return false;
  }

  int const spsid = sliceSpsId(aup->pPPS, pSlice);

  aup->pActiveSPS = &aup->pSPS[spsid];

  const AL_TDimension tDim = { pSlice->pSPS->pic_width_in_luma_samples, pSlice->pSPS->pic_height_in_luma_samples };
  const int iLevel = pSlice->pSPS->profile_and_level.general_level_idc / 3;
  const int iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(iLevel, tDim.iWidth, tDim.iHeight);
  const int iDpbRef = Min(pSlice->pSPS->sps_max_dec_pic_buffering_minus1[pSlice->pSPS->sps_max_sub_layers_minus1] + 1, iDpbMaxBuf);
  AL_PictMngr_UpdateDPBInfo(&pCtx->PictMngr, iDpbRef);

  if(!pSlice->dependent_slice_segment_flag)
  {
    if(pSlice->slice_type != AL_SLICE_I && !aup->iRecoveryCnt && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
      return false;

    if(pSlice->IdrPicFlag)
    {
      pCtx->PictMngr.iCurFramePOC = 0;
      pCtx->PictMngr.uPrevPocLSB = 0;
      pCtx->PictMngr.iPrevPocMSB = 0;
    }
    else if(!pCtx->tConceal.bValidFrame)
      pCtx->PictMngr.iCurFramePOC = calculatePOC(&pCtx->PictMngr, pSlice, pCtx->uNoRaslOutputFlag);

    if(!pCtx->tConceal.bValidFrame)
    {
      AL_HEVC_PictMngr_InitRefPictSet(&pCtx->PictMngr, pSlice);

      /* at least one active reference on inter slice */
      if(pSlice->slice_type != AL_SLICE_I && !pSlice->NumPocTotalCurr && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
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
static bool isFirstSliceSegmentInPicture(AL_THevcSliceHdr* pSlice)
{
  return pSlice->first_slice_segment_in_pic_flag == 1;
}

/*****************************************************************************/
static void processScalingList(AL_THevcAup* pAUP, AL_THevcSliceHdr* pSlice, AL_TScl* pScl)
{
  int const ppsid = slicePpsId(pSlice);

  AL_CleanupMemory(pScl, sizeof(*pScl));

  // Save ScalingList
  if(pAUP->pPPS[ppsid].pSPS->scaling_list_enabled_flag && isFirstSliceSegmentInPicture(pSlice))
    copyScalingList(&pAUP->pPPS[ppsid], pScl);
}

/*****************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bSliceHdrValid)
{
  pSP->eSliceType = pSlice->slice_type = AL_SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT, pPP->FrmID);

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
static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, AL_TDecPicParam* pPP, uint8_t pic_output_flag, bool bHasPreviousSlice)
{
  AL_HEVC_PictMngr_UpdateRecInfo(&pCtx->PictMngr, pSlice->pSPS, pPP);
  AL_HEVC_PictMngr_EndFrame(&pCtx->PictMngr, pSlice->slice_pic_order_cnt_lsb, eNUT, pSlice, pic_output_flag);

  if(pCtx->chanParam.eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);

  if(pCtx->chanParam.eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, true, bHasPreviousSlice);

  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static bool hasOngoingFrame(AL_TDecCtx* pCtx)
{
  return pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid;
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[pCtx->uCurID];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice - 1]);

  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = pCtx->iStreamOffset[(pCtx->iNumFrmBlk1 + pCtx->iStackSize - 1) % pCtx->iStackSize];

  endFrame(pCtx, pSlice->nal_unit_type, pSlice, pPP, pSlice->pic_output_flag, false);

  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
}

/*****************************************************************************/
static bool isRandomAccessPoint(AL_ENut eNUT)
{
  return AL_HEVC_IsCRA(eNUT) || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL22) || (eNUT == AL_HEVC_NUT_RSV_IRAP_VCL23);
}

/*****************************************************************************/
static bool isValidSyncPoint(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_ESliceType ePicType, int iRecoveryCnt)
{
  if(isRandomAccessPoint(eNUT))
    return true;

  if(iRecoveryCnt)
    return true;

  if(pCtx->bUseIFramesAsSyncPoint && eNUT == AL_HEVC_NUT_TRAIL_R && ePicType == AL_SLICE_I)
    return true;

  return false;
}

/*****************************************************************************/
static void decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  // ignore RASL picture associated with an IRAP picture that has NoRaslOutputFlag = 1
  if(AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag)
    return;

  bool const bIsRAP = isRandomAccessPoint(eNUT);

  if(bIsRAP)
    pCtx->uNoRaslOutputFlag = (pCtx->bIsFirstPicture || pCtx->bLastIsEOS || AL_HEVC_IsBLA(eNUT) || AL_HEVC_IsIDR(eNUT)) ? 1 : 0;

  AL_THevcAup* pAUP = &pIAUP->hevcAup;
  bool bCheckDynResChange = pCtx->bIsBuffersAllocated;
  AL_TDimension tLastDim = pCtx->tStreamSettings.tDim;

  if(pCtx->bIsFirstSPSChecked)
  {
    tLastDim.iWidth = pAUP->pActiveSPS->pic_width_in_luma_samples;
    tLastDim.iHeight = pAUP->pActiveSPS->pic_height_in_luma_samples;
  }

  TCircBuffer* pBufStream = &pCtx->Stream;
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pCtx->BufNoAE.tMD.pVirtualAddr, true, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->uCurID) & 0x01;
  AL_THevcSliceHdr* pSlice = &pCtx->HevcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(*pSlice));
  AL_TConceal* pConceal = &pCtx->tConceal;
  bool isValid = AL_HEVC_ParseSliceHeader(pSlice, &pCtx->HevcSliceHdr[pCtx->uCurID], &rp, pConceal, pAUP->pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(isValid)
  {
    if(pSlice->slice_pic_order_cnt_lsb != pCtx->uCurPocLsb && !isFirstSliceSegmentInPicture(pSlice))
      bSliceBelongsToSameFrame = false;
    else if(pSlice->slice_segment_address <= pConceal->iFirstLCU && !pSlice->pPPS->tiles_enabled_flag && !pSlice->pPPS->entropy_coding_sync_enabled_flag)
      isValid = false;
  }

  if(isValid)
    pConceal->iFirstLCU = pSlice->slice_segment_address;

  bool* bFirstSliceInFrameIsValid = &pCtx->bFirstSliceInFrameIsValid;
  bool* bFirstIsValid = &pCtx->bFirstIsValid;
  bool* bBeginFrameIsValid = &pCtx->bBeginFrameIsValid;

  if(!bSliceBelongsToSameFrame && hasOngoingFrame(pCtx))
    finishPreviousFrame(pCtx);

  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];

  if(isValid)
  {
    pCtx->uCurPocLsb = pSlice->slice_pic_order_cnt_lsb;
    isValid = initSlice(pCtx, pSlice);
  }

  if(!isValid)
  {
    if(!*bFirstIsValid)
    {
      pCtx->bIsFirstPicture = false;
      UpdateContextAtEndOfFrame(pCtx);
      return;
    }
    AL_HEVC_PictMngr_RemoveHeadFrame(&pCtx->PictMngr);
  }

  if(isValid)
  {
    int const spsid = sliceSpsId(pAUP->pPPS, pSlice);
    AL_THevcSps* pSPS = &pAUP->pSPS[spsid];
    isValid = isSPSCompatibleWithStreamSettings(pSPS, &pCtx->tStreamSettings);
    AL_TStreamSettings spsSettings = extractStreamSettings(pSPS);

    if(!isValid)
    {
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS, pPP->FrmID);
      pSPS->bConceal = true;
    }
    else if(bCheckDynResChange && (spsSettings.tDim.iWidth != tLastDim.iWidth || spsSettings.tDim.iHeight != tLastDim.iHeight))
    {
      AL_TCropInfo tCropInfo = extractCropInfo(pSPS);
      resolutionFound(pCtx, &spsSettings, &tCropInfo);
    }
  }

  if(isFirstSliceSegmentInPicture(pSlice) && *bFirstSliceInFrameIsValid)
    isValid = false;

  if(isValid && isFirstSliceSegmentInPicture(pSlice))
    *bFirstSliceInFrameIsValid = true;

  if(isValid && pSlice->slice_type != AL_SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  pCtx->uCurID = (pCtx->uCurID + 1) & 1;

  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice]);

  pBufs->tStream.tMD = pCtx->Stream.tMD;

  if(!pSlice->pPPS)
    pSlice->pPPS = &pAUP->pPPS[pConceal->iLastPPSId];

  if(!pSlice->pSPS)
    pSlice->pSPS = pAUP->pActiveSPS;

  if(*bFirstSliceInFrameIsValid)
  {
    if(isFirstSliceSegmentInPicture(pSlice) && !(*bBeginFrameIsValid))
    {
      bool bClearRef = (bIsRAP && pCtx->uNoRaslOutputFlag); // IRAP picture with NoRaslOutputFlag = 1
      bool bNoOutputPrior = (AL_HEVC_IsCRA(eNUT) || ((AL_HEVC_IsIDR(eNUT) || AL_HEVC_IsBLA(eNUT)) && pSlice->no_output_of_prior_pics_flag));

      AL_HEVC_PictMngr_ClearDPB(&pCtx->PictMngr, pSlice->pSPS, bClearRef, bNoOutputPrior);
    }
  }

  if(pSlice->slice_type != AL_SLICE_I && !pIAUP->hevcAup.iRecoveryCnt && !AL_HEVC_PictMngr_HasPictInDPB(&pCtx->PictMngr))
    isValid = false;
  else if(!(*bFirstSliceInFrameIsValid) && pSlice->slice_segment_address)
  {
    createConcealSlice(pCtx, pPP, pSP, pSlice);

    pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[++pCtx->PictMngr.uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(!(*bBeginFrameIsValid) && pSlice->pSPS)
  {
    AL_TDimension const tDim = { pSlice->pSPS->pic_width_in_luma_samples, pSlice->pSPS->pic_height_in_luma_samples };

    if(!AL_InitFrameBuffers(pCtx, pBufs, tDim, pPP))
      return;
    *bBeginFrameIsValid = true;
  }

  bool bLastSlice = *iNumSlice >= pCtx->chanParam.iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    isValid = false;

  AL_TScl ScalingList = { 0 };

  if(isValid)
  {
    if(!(*bFirstIsValid))
    {
      if(!isValidSyncPoint(pCtx, eNUT, pSlice->slice_type, pAUP->iRecoveryCnt))
      {
        pCtx->bIsFirstPicture = false;
        *bBeginFrameIsValid = false;
        AL_CancelFrameBuffers(pCtx);
        return;
      }
      *bFirstIsValid = true;
    }

    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(pCtx->PictMngr.uNumSlice == 0)
      AL_HEVC_FillPictParameters(pSlice, pCtx, pPP);
    AL_HEVC_FillSliceParameters(pSlice, pCtx, pSP, false);

    if(!AL_HEVC_PictMngr_BuildPictureList(&pCtx->PictMngr, pSlice, &pCtx->ListRef) && !pAUP->iRecoveryCnt)
    {
      concealSlice(pCtx, pPP, pSP, pSlice, true);
    }
    else
    {
      AL_HEVC_FillSlicePicIdRegister(pSlice, pCtx, pPP, pSP);
      pConceal->bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSP);
    }
  }
  else if((bIsLastAUNal || isFirstSliceSegmentInPicture(pSlice) || bLastSlice) && (*bFirstIsValid) && (*bFirstSliceInFrameIsValid)) /* conceal the current slice data */
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

    return;
  }

  // Launch slice decoding
  AL_HEVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pCtx->PictMngr.uNumSlice;
  ++(*iNumSlice);

  if(bIsLastAUNal || bLastSlice)
  {
    uint8_t pic_output_flag = (AL_HEVC_IsRASL(eNUT) && pCtx->uNoRaslOutputFlag) ? 0 : pSlice->pic_output_flag;
    endFrame(pCtx, eNUT, pSlice, pPP, pic_output_flag, true);
    return;
  }

  if(pCtx->chanParam.eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, false, true);
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

/*****************************************************************************/
static AL_PARSE_RESULT parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  uint16_t PpsId;
  AL_HEVC_ParsePPS(IAup, rp, &PpsId);

  AL_THevcAup* aup = &IAup->hevcAup;

  if(!aup->pPPS[PpsId].bConceal)
  {
    pCtx->tConceal.bHasPPS = true;

    if(pCtx->tConceal.iLastPPSId <= PpsId)
      pCtx->tConceal.iLastPPSId = PpsId;
  }

  return AL_OK;
}

/*****************************************************************************/
static bool isActiveSPSChanging(AL_THevcSps* pNewSPS, AL_THevcSps* pActiveSPS)
{
  // Only check resolution change - but any change is forgiven, according to the specification
  return pActiveSPS != NULL &&
         pNewSPS->sps_seq_parameter_set_id == pActiveSPS->sps_seq_parameter_set_id &&
         (pNewSPS->pic_height_in_luma_samples != pActiveSPS->pic_height_in_luma_samples ||
          pNewSPS->pic_width_in_luma_samples != pActiveSPS->pic_width_in_luma_samples);
}

/*****************************************************************************/
static AL_PARSE_RESULT parseAndApplySPS(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_TDecCtx* pCtx)
{
  AL_THevcSps tNewSPS;
  AL_PARSE_RESULT eParseResult = AL_HEVC_ParseSPS(pRP, &tNewSPS);

  if(eParseResult == AL_OK)
  {
    if(hasOngoingFrame(pCtx) && isActiveSPSChanging(&tNewSPS, pIAup->hevcAup.pActiveSPS))
    {
      // An active SPS should not be modified unless it is the end of the CVS (spec 7.4.2.4).
      // So we consider we received the full frame.
      finishPreviousFrame(pCtx);
    }

    pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id] = tNewSPS;
  }
  else
    pIAup->hevcAup.pSPS[tNewSPS.sps_seq_parameter_set_id].bConceal = true;

  return eParseResult;
}

/*****************************************************************************/
void AL_HEVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  AL_NonVclNuts nuts =
  {
    AL_HEVC_NUT_PREFIX_SEI,
    AL_HEVC_NUT_SUFFIX_SEI,
    AL_HEVC_NUT_SPS,
    AL_HEVC_NUT_PPS,
    AL_HEVC_NUT_VPS,
    AL_HEVC_NUT_FD,
    AL_HEVC_NUT_EOS,
    AL_HEVC_NUT_EOB,
  };

  AL_NalParser parser =
  {
    parseAndApplySPS,
    parsePPSandUpdateConcealment,
    ParseVPS,
    AL_HEVC_ParseSEI,
    decodeSliceData,
    isSliceData,
    finishPreviousFrame,
  };
  AL_DecodeOneNal(nuts, parser, pAUP, pCtx, eNUT, bIsLastAUNal, iNumSlice);
}

/*****************************************************************************/
void AL_HEVC_InitAUP(AL_THevcAup* pAUP)
{
  for(int i = 0; i < AL_HEVC_MAX_PPS; ++i)
    pAUP->pPPS[i].bConceal = true;

  for(int i = 0; i < AL_HEVC_MAX_SPS; ++i)
    pAUP->pSPS[i].bConceal = true;

  pAUP->pActiveSPS = NULL;
}

/*****************************************************************************/
AL_ERR CreateHevcDecoder(AL_TDecoder** hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  return AL_CreateDefaultDecoder((AL_TDecoder**)hDec, pDecChannel, pAllocator, pSettings, pCB);
}

