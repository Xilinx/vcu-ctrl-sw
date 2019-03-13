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
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/Error.h"
#include "lib_common/FourCC.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/Defines_mcu.h"

#include "lib_parsing/AvcParser.h"
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
static AL_TDimension extractDimension(AL_TAvcSps const* pSPS)
{
  return (AL_TDimension) {
           (pSPS->pic_width_in_mbs_minus1 + 1) * 16, (pSPS->pic_height_in_map_units_minus1 + 1) * 16
  };
}

/******************************************************************************/
static AL_TCropInfo extractCropInfo(AL_TAvcSps const* pSPS)
{
  AL_TCropInfo tCropInfo = { false, 0, 0, 0, 0 };

  if(!pSPS->frame_cropping_flag)
    return tCropInfo;

  tCropInfo.bCropping = true;

  if(pSPS->chroma_format_idc == 1 || pSPS->chroma_format_idc == 2)
  {
    tCropInfo.uCropOffsetLeft += 2 * pSPS->frame_crop_left_offset;
    tCropInfo.uCropOffsetRight += 2 * pSPS->frame_crop_right_offset;
  }
  else
  {
    tCropInfo.uCropOffsetLeft += pSPS->frame_crop_left_offset;
    tCropInfo.uCropOffsetRight += pSPS->frame_crop_right_offset;
  }

  if(pSPS->chroma_format_idc == 1)
  {
    tCropInfo.uCropOffsetTop += 2 * pSPS->frame_crop_top_offset;
    tCropInfo.uCropOffsetBottom += 2 * pSPS->frame_crop_bottom_offset;
  }
  else
  {
    tCropInfo.uCropOffsetTop += pSPS->frame_crop_top_offset;
    tCropInfo.uCropOffsetBottom += pSPS->frame_crop_bottom_offset;
  }
  return tCropInfo;
}

/*************************************************************************/
static int getMaxBitDepth(int profile_idc)
{
  switch(profile_idc)
  {
  case AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE):
  case AL_GET_PROFILE_IDC(AL_PROFILE_AVC_MAIN):
  case AL_GET_PROFILE_IDC(AL_PROFILE_AVC_EXTENDED):
  case AL_GET_PROFILE_IDC(AL_PROFILE_AVC_HIGH): return 8;
  default: return 10;
  }
}

/*************************************************************************/
static int getMaxNumberOfSlices(AL_TStreamSettings const* pStreamSettings, AL_TAvcSps const* pSPS)
{
  int maxSlicesCountSupported = Avc_GetMaxNumberOfSlices(122, 52, 1, 60, INT32_MAX);
  return maxSlicesCountSupported; /* TODO : fix bad behaviour in firmware to decrease dynamically the number of slices */

  int macroblocksCountInPicture = (pStreamSettings->tDim.iWidth / 16) * (pStreamSettings->tDim.iHeight / 16);

  int numUnitsInTick = 1, timeScale = 1;

  if(pSPS->vui_parameters_present_flag)
  {
    numUnitsInTick = pSPS->vui_param.vui_num_units_in_tick;
    timeScale = pSPS->vui_param.vui_time_scale;
  }
  return Min(maxSlicesCountSupported, Avc_GetMaxNumberOfSlices(pStreamSettings->iProfileIdc, pStreamSettings->iLevel, numUnitsInTick, timeScale, macroblocksCountInPicture));
}

/*****************************************************************************/
static bool isSPSCompatibleWithStreamSettings(AL_TAvcSps const* pSPS, AL_TStreamSettings const* pStreamSettings)
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

  int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_idc);

  if(iSPSMaxBitDepth > HW_IP_BIT_DEPTH)
    return false;

  int iSPSLevel = pSPS->constraint_set3_flag ? 9 : pSPS->level_idc; /* We treat constraint set 3 as a level 9 */

  if((pStreamSettings->iLevel > 0) && (pStreamSettings->iLevel < iSPSLevel))
    return false;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if((pStreamSettings->eChroma != CHROMA_MAX_ENUM) && (pStreamSettings->eChroma < eSPSChromaMode))
    return false;

  AL_TCropInfo tSPSCropInfo = extractCropInfo(pSPS);

  int iSPSCropWidth = tSPSCropInfo.uCropOffsetLeft + tSPSCropInfo.uCropOffsetRight;
  AL_TDimension tSPSDim = extractDimension(pSPS);

  if((pStreamSettings->tDim.iWidth > 0) && (pStreamSettings->tDim.iWidth < (tSPSDim.iWidth - iSPSCropWidth)))
    return false;

  if((pStreamSettings->tDim.iWidth > 0) && (tSPSDim.iWidth < AL_CORE_AVC_MIN_WIDTH))
    return false;

  int iSPSCropHeight = tSPSCropInfo.uCropOffsetTop + tSPSCropInfo.uCropOffsetBottom;

  if((pStreamSettings->tDim.iHeight > 0) && (pStreamSettings->tDim.iHeight < (tSPSDim.iHeight - iSPSCropHeight)))
    return false;

  if(((pStreamSettings->eSequenceMode != AL_SM_MAX_ENUM) && pStreamSettings->eSequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != AL_SM_PROGRESSIVE))
    return false;

  return true;
}

/******************************************************************************/
int AVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack);

/******************************************************************************/
static AL_TStreamSettings extractStreamSettings(AL_TAvcSps const* pSPS)
{
  AL_TStreamSettings tStreamSettings;
  tStreamSettings.tDim = extractDimension(pSPS);
  tStreamSettings.eChroma = (AL_EChromaMode)pSPS->chroma_format_idc;
  tStreamSettings.iBitDepth = getMaxBitDepth(pSPS->profile_idc);
  tStreamSettings.iLevel = pSPS->constraint_set3_flag ? 9 : pSPS->level_idc;
  tStreamSettings.iProfileIdc = pSPS->profile_idc;
  tStreamSettings.eSequenceMode = AL_SM_PROGRESSIVE;
  return tStreamSettings;
}

/******************************************************************************/
static int getConcealedMaxDPBSize(int iLevel, int iWidth, int iHeight, int iSPSMaxRefFrames)
{
  int iDpbMaxBuf = AL_AVC_GetMaxDPBSize(iLevel, iWidth, iHeight);
  return Max(iDpbMaxBuf, iSPSMaxRefFrames);
}

/******************************************************************************/
static AL_ERR resolutionFound(AL_TDecCtx* pCtx, AL_TStreamSettings const* pSpsSettings, AL_TCropInfo const* pCropInfo, int iSPSMaxRefFrames)
{
  int iDpbMaxBuf = getConcealedMaxDPBSize(pSpsSettings->iLevel, pSpsSettings->tDim.iWidth, pSpsSettings->tDim.iHeight, iSPSMaxRefFrames);
  int iMaxBuf = AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->iStackSize);
  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);
  int iSizeYuv = AL_GetAllocSize_Frame(pSpsSettings->tDim, pSpsSettings->eChroma, pSpsSettings->iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);

  return pCtx->resolutionFoundCB.func(iMaxBuf, iSizeYuv, pSpsSettings, pCropInfo, pCtx->resolutionFoundCB.userParam);
}

/******************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS)
{
  pCtx->tStreamSettings = extractStreamSettings(pSPS);
  int iNumMBs = (pSPS->pic_width_in_mbs_minus1 + 1) * (pSPS->pic_height_in_map_units_minus1 + 1);
  assert(iNumMBs == ((pCtx->tStreamSettings.tDim.iWidth / 16) * (pCtx->tStreamSettings.tDim.iHeight / 16)));
  int iSPSMaxSlices = getMaxNumberOfSlices(&pCtx->tStreamSettings, pSPS);
  int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int iSizeCompData = AL_GetAllocSize_AvcCompData(pCtx->tStreamSettings.tDim, pCtx->tStreamSettings.eChroma);
  int iSizeCompMap = AL_GetAllocSize_DecCompMap(pCtx->tStreamSettings.tDim);
  AL_ERR error = AL_ERR_NO_MEMORY;

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  int iDpbMaxBuf = getConcealedMaxDPBSize(pCtx->tStreamSettings.iLevel, pCtx->tStreamSettings.tDim.iWidth, pCtx->tStreamSettings.tDim.iHeight, pSPS->max_num_ref_frames);
  int iMaxBuf = AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->iStackSize);
  int iSizeMV = AL_GetAllocSize_AvcMV(pCtx->tStreamSettings.tDim);
  int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  int iDpbRef = iDpbMaxBuf;
  int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_idc);
  bool bEnableRasterOutput = pCtx->chanParam.eBufferOutputMode != AL_OUTPUT_INTERNAL;

  AL_PictMngr_Init(&pCtx->PictMngr, pCtx->pAllocator, iMaxBuf, iSizeMV, iDpbRef, pCtx->eDpbMode, pCtx->chanParam.eFBStorageMode, iSPSMaxBitDepth, bEnableRasterOutput, pCtx->chanParam.bUseEarlyCallback);


  AL_TCropInfo tCropInfo = extractCropInfo(pSPS);
  error = resolutionFound(pCtx, &pCtx->tStreamSettings, &tCropInfo, pSPS->max_num_ref_frames);

  if(AL_IS_ERROR_CODE(error))
    goto fail_alloc;

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS)
{
  AL_TDecChanParam* pChan = &pCtx->chanParam;
  pChan->iWidth = pCtx->tStreamSettings.tDim.iWidth;
  pChan->iHeight = pCtx->tStreamSettings.tDim.iHeight;

  const int iSPSMaxSlices = getMaxNumberOfSlices(&pCtx->tStreamSettings, pSPS);
  pChan->iMaxSlices = iSPSMaxSlices;

  if(!pCtx->bForceFrameRate && pSPS->vui_parameters_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale / 2;
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
static int slicePpsId(AL_TAvcSliceHdr const* pSlice)
{
  return pSlice->pic_parameter_set_id;
}

/******************************************************************************/
static int sliceSpsId(AL_TAvcPps const* pPps, AL_TAvcSliceHdr const* pSlice)
{
  int const ppsid = slicePpsId(pSlice);
  return pPps[ppsid].seq_parameter_set_id;
}

/******************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_TAvcSliceHdr* pSlice)
{
  AL_TAvcAup* aup = &pCtx->aup.avcAup;

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

  return true;
}

/*****************************************************************************/
static void copyScalingList(AL_TAvcPps* pPPS, AL_TScl* pSCL)
{
  Rtos_Memcpy((*pSCL)[0].t4x4Y,
              !pPPS->UseDefaultScalingMatrix4x4Flag[0] ? pPPS->ScalingList4x4[0] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[0].t4x4Cb,
              !pPPS->UseDefaultScalingMatrix4x4Flag[1] ? pPPS->ScalingList4x4[1] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[0].t4x4Cr,
              !pPPS->UseDefaultScalingMatrix4x4Flag[2] ? pPPS->ScalingList4x4[2] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Y,
              !pPPS->UseDefaultScalingMatrix4x4Flag[3] ? pPPS->ScalingList4x4[3] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Cb,
              !pPPS->UseDefaultScalingMatrix4x4Flag[4] ? pPPS->ScalingList4x4[4] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Cr,
              !pPPS->UseDefaultScalingMatrix4x4Flag[5] ? pPPS->ScalingList4x4[5] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[0].t8x8Y,
              !pPPS->UseDefaultScalingMatrix8x8Flag[0] ? pPPS->ScalingList8x8[0] :
              AL_AVC_DefaultScalingLists8x8[0], 64);
  Rtos_Memcpy((*pSCL)[1].t8x8Y,
              !pPPS->UseDefaultScalingMatrix8x8Flag[1] ? pPPS->ScalingList8x8[1] :
              AL_AVC_DefaultScalingLists8x8[1], 64);
}

/******************************************************************************/
static void processScalingList(AL_TAvcAup* pAUP, AL_TAvcSliceHdr* pSlice, AL_TScl* pScl)
{
  int ppsid = pSlice->pic_parameter_set_id;

  AL_CleanupMemory(pScl, sizeof(*pScl));
  copyScalingList(&pAUP->pPPS[ppsid], pScl);
}

/******************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, AL_ENut eNUT)
{
  AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, true);

  pSP->eSliceType = SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT, pPP->FrmID);

  if(eNUT == AL_AVC_NUT_VCL_IDR)
  {
    pCtx->PictMngr.iCurFramePOC = 0;
    pSP->ValidConceal = false;
  }
  else
  {
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);
    AL_SetConcealParameters(pCtx, pSP);
  }
}

/*****************************************************************************/
static void createConcealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  concealSlice(pCtx, pPP, pSP, pSlice, false);
  pSP->FirstLcuSliceSegment = 0;
  pSP->FirstLcuSlice = 0;
  pSP->FirstLCU = 0;
  pSP->NextSliceSegment = pSlice->first_mb_in_slice;
  pSP->NumLCU = pSlice->first_mb_in_slice;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void updatePictManager(AL_TPictMngrCtx* pCtx, AL_ENut eNUT, AL_TDecPicParam* pPP, AL_TAvcSliceHdr* pSlice)
{
  bool bClearRef = AL_AVC_IsIDR(eNUT);
  AL_EMarkingRef eMarkingFlag = ((pSlice->nal_ref_idc > 0) || bClearRef) ? (pSlice->long_term_reference_flag ? LONG_TERM_REF : SHORT_TERM_REF) : UNUSED_FOR_REF;

  // update picture manager
  pCtx->iPrevFrameNum = pSlice->frame_num;
  pCtx->bLastIsIDR = bClearRef;

  AL_AVC_PictMngr_UpdateRecInfo(pCtx, pSlice->pSPS, pPP);
  AL_AVC_PictMngr_EndParsing(pCtx, bClearRef, eMarkingFlag);

  if(pSlice->nal_ref_idc)
  {
    AL_Dpb_MarkingProcess(&pCtx->DPB, pSlice);

    if(AL_Dpb_LastHasMMCO5(&pCtx->DPB))
    {
      pCtx->iPrevFrameNum = 0;
      pCtx->iCurFramePOC = 0;
    }
  }
  AL_AVC_PictMngr_CleanDPB(pCtx);
}

/******************************************************************************/
static void reallyEndFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TDecPicParam* pPP, AL_TAvcSliceHdr* pSlice, bool hasPreviousSlice)
{
  updatePictManager(&pCtx->PictMngr, eNUT, pPP, pSlice);

  if(pCtx->chanParam.eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);

  if(pCtx->chanParam.eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, true, hasPreviousSlice);

  UpdateContextAtEndOfFrame(pCtx);
}

static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TDecPicParam* pPP, AL_TAvcSliceHdr* pSlice)
{
  reallyEndFrame(pCtx, eNUT, pPP, pSlice, true);
}

static void endFrameConceal(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TDecPicParam* pPP, AL_TAvcSliceHdr* pSlice)
{
  reallyEndFrame(pCtx, eNUT, pPP, pSlice, false);
}

/*****************************************************************************/
static bool hasOngoingFrame(AL_TDecCtx* pCtx)
{
  return pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid;
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_TAvcSliceHdr* pSlice = &pCtx->AvcSliceHdr[pCtx->uCurID];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice - 1]);

  pPP->num_slice = pCtx->PictMngr.uNumSlice - 1;
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = pCtx->iStreamOffset[(pCtx->iNumFrmBlk1 + pCtx->iStackSize - 1) % pCtx->iStackSize];

  /* The slice is its own previous slice as we changed it in the last slice
   * This means we don't want to send a previous slice at all. */
  endFrameConceal(pCtx, pSlice->nal_unit_type, pPP, pSlice);

  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
}

/******************************************************************************/
static bool constructRefPicList(AL_TAvcSliceHdr* pSlice, AL_TDecCtx* pCtx, TBufferListRef* pListRef)
{
  AL_TPictMngrCtx* pPictMngrCtx = &pCtx->PictMngr;

  AL_Dpb_PictNumberProcess(&pPictMngrCtx->DPB, pSlice);

  AL_AVC_PictMngr_InitPictList(pPictMngrCtx, pSlice, pListRef);
  AL_AVC_PictMngr_ReorderPictList(pPictMngrCtx, pSlice, pListRef);

  for(int i = pSlice->num_ref_idx_l0_active_minus1 + 1; i < MAX_REF; ++i)
    (*pListRef)[0][i].uNodeID = 0xFF;

  for(int i = pSlice->num_ref_idx_l1_active_minus1 + 1; i < MAX_REF; ++i)
    (*pListRef)[1][i].uNodeID = 0xFF;

  int pNumRef[2] = { 0, 0 };

  for(int i = 0; i < MAX_REF; ++i)
  {
    if((*pListRef)[0][i].uNodeID != uEndOfList)
      pNumRef[0]++;

    if((*pListRef)[1][i].uNodeID != uEndOfList)
      pNumRef[1]++;
  }

  if((pSlice->slice_type != SLICE_I && pNumRef[0] < pSlice->num_ref_idx_l0_active_minus1 + 1) ||
     (pSlice->slice_type == SLICE_B && pNumRef[1] < pSlice->num_ref_idx_l1_active_minus1 + 1))
    return false;

  return true;
}

/*****************************************************************************/
static bool isRandomAccessPoint(AL_ENut eNUT)
{
  return eNUT == AL_AVC_NUT_VCL_IDR;
}

/*****************************************************************************/
static bool isValidSyncPoint(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_ESliceType ePicType, int iRecoveryCnt)
{
  if(isRandomAccessPoint(eNUT))
    return true;

  if(iRecoveryCnt)
    return true;

  if(pCtx->bUseIFramesAsSyncPoint && eNUT == AL_AVC_NUT_VCL_NON_IDR && ePicType == SLICE_I)
    return true;

  return false;
}

/*****************************************************************************/
static void decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  TCircBuffer* pBufStream = &pCtx->Stream;
  InitRbspParser(pBufStream, pCtx->BufNoAE.tMD.pVirtualAddr, true, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->uCurID) & 0x01;
  AL_TAvcSliceHdr* pSlice = &pCtx->AvcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(AL_TAvcSliceHdr));
  AL_TConceal* pConceal = &pCtx->tConceal;
  AL_TAvcAup* pAUP = &pIAUP->avcAup;
  bool isValid = AL_AVC_ParseSliceHeader(pSlice, &rp, pConceal, pAUP->pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(isValid)
    bSliceBelongsToSameFrame = (!pSlice->first_mb_in_slice || (pSlice->pic_order_cnt_lsb == pCtx->uCurPocLsb));

  bool* bFirstSliceInFrameIsValid = &pCtx->bFirstSliceInFrameIsValid;
  bool* bFirstIsValid = &pCtx->bFirstIsValid;
  bool* bBeginFrameIsValid = &pCtx->bBeginFrameIsValid;
  bool bCheckDynResChange = pCtx->bIsBuffersAllocated;
  AL_TDimension tLastDim = !pCtx->bIsFirstSPSChecked ? pCtx->tStreamSettings.tDim : extractDimension(pAUP->pActiveSPS);

  if(!bSliceBelongsToSameFrame && hasOngoingFrame(pCtx))
    finishPreviousFrame(pCtx);

  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];
  AL_TDecPicParam* pPP = &pCtx->PoolPP[pCtx->uToggle];

  if(isValid)
  {
    pCtx->uCurPocLsb = pSlice->pic_order_cnt_lsb;

    if(!initSlice(pCtx, pSlice))
    {
      UpdateContextAtEndOfFrame(pCtx);
      return;
    }
  }

  pCtx->uCurID = (pCtx->uCurID + 1) & 1;
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice]);

  if(!(*bFirstSliceInFrameIsValid) && pSlice->first_mb_in_slice)
  {
    if(!pSlice->pSPS)
      pSlice->pSPS = pAUP->pActiveSPS;
    createConcealSlice(pCtx, pPP, pSP, pSlice);

    pSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[++pCtx->PictMngr.uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(isValid)
  {
    int const spsid = sliceSpsId(pAUP->pPPS, pSlice);
    AL_TAvcSps* pSPS = &pAUP->pSPS[spsid];
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
      resolutionFound(pCtx, &spsSettings, &tCropInfo, pSPS->max_num_ref_frames);
    }
  }

  if(isValid && !pSlice->first_mb_in_slice)
    *bFirstSliceInFrameIsValid = true;

  if(isValid && pSlice->slice_type != SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  if(bIsLastAUNal)
    pPP->num_slice = pCtx->PictMngr.uNumSlice;

  pBufs->tStream.tMD = pCtx->Stream.tMD;

  if(!pSlice->pSPS)
    pSlice->pSPS = pAUP->pActiveSPS;

  // Compute Current POC
  if(isValid && (!pSlice->first_mb_in_slice || !bSliceBelongsToSameFrame))
    AL_AVC_PictMngr_SetCurrentPOC(&pCtx->PictMngr, pSlice);

  // compute check gaps in frameNum
  if(isValid)
    AL_AVC_PictMngr_Fill_Gap_In_FrameNum(&pCtx->PictMngr, pSlice);

  if(!(*bBeginFrameIsValid) && pSlice->pSPS)
  {
    AL_TDimension const tDim = extractDimension(pSlice->pSPS);

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
        *bBeginFrameIsValid = false;
        AL_CancelFrameBuffers(pCtx);
        return;
      }
      *bFirstIsValid = true;
    }

    // update Nal Unit size
    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(pCtx->PictMngr.uNumSlice == 0)
      AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
    AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, false);

    if(!constructRefPicList(pSlice, pCtx, &pCtx->ListRef) && !pAUP->iRecoveryCnt)
    {
      concealSlice(pCtx, pPP, pSP, pSlice, eNUT);
    }
    else
    {
      AL_AVC_FillSlicePicIdRegister(pCtx, pSP);
      pConceal->bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSP);
    }
  }
  else if((bIsLastAUNal || !pSlice->first_mb_in_slice || bLastSlice) && (*bFirstIsValid) && (*bFirstSliceInFrameIsValid)) /* conceal the current slice data */
  {
    concealSlice(pCtx, pPP, pSP, pSlice, eNUT);

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
  AL_AVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pCtx->PictMngr.uNumSlice;
  ++(*iNumSlice);

  if(pAUP->ePictureType == SLICE_I || pSlice->slice_type == SLICE_B)
    pAUP->ePictureType = (AL_ESliceType)pSlice->slice_type;

  if(bIsLastAUNal || bLastSlice)
  {
    endFrame(pCtx, eNUT, pPP, pSlice);
    pAUP->ePictureType = SLICE_I;
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
  case AL_AVC_NUT_VCL_NON_IDR:
  case AL_AVC_NUT_VCL_IDR:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
static AL_PARSE_RESULT parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  uint16_t PpsId;
  AL_PARSE_RESULT result = AL_AVC_ParsePPS(IAup, rp, &PpsId);

  AL_TAvcAup* aup = &IAup->avcAup;

  if(!aup->pPPS[PpsId].bConceal)
  {
    pCtx->tConceal.bHasPPS = true;

    if(pCtx->tConceal.iLastPPSId <= PpsId)
      pCtx->tConceal.iLastPPSId = PpsId;
  }

  return result;
}

/*****************************************************************************/
static bool isActiveSPSChanging(AL_TAvcSps* pNewSPS, AL_TAvcSps* pActiveSPS)
{
  // Only check resolution change - but any change is forgiven, according to the specification
  return pActiveSPS != NULL &&
         pNewSPS->seq_parameter_set_id == pActiveSPS->seq_parameter_set_id &&
         (pNewSPS->pic_width_in_mbs_minus1 != pActiveSPS->pic_width_in_mbs_minus1 ||
          pNewSPS->pic_height_in_map_units_minus1 != pActiveSPS->pic_height_in_map_units_minus1);
}

/*****************************************************************************/
static AL_PARSE_RESULT parseAndApplySPS(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_TDecCtx* pCtx)
{
  AL_TAvcSps tNewSPS;
  AL_PARSE_RESULT eParseResult = AL_AVC_ParseSPS(pRP, &tNewSPS);

  if(eParseResult == AL_OK)
  {
    if(hasOngoingFrame(pCtx) && isActiveSPSChanging(&tNewSPS, pIAup->avcAup.pActiveSPS))
    {
      // An active SPS should not be modified unless it is the end of the CVS (spec I.7.4.1.2).
      // So we consider we received the full frame.
      finishPreviousFrame(pCtx);
    }

    pIAup->avcAup.pSPS[tNewSPS.seq_parameter_set_id] = tNewSPS;
  }

  return eParseResult;
}

/*****************************************************************************/
void AL_AVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  AL_NonVclNuts nuts =
  {
    AL_AVC_NUT_PREFIX_SEI,
    AL_AVC_NUT_PREFIX_SEI, /* AVC doesn't distinguish between prefix and suffix */
    AL_AVC_NUT_SPS,
    AL_AVC_NUT_PPS,
    0,
    AL_AVC_NUT_EOS,
  };

  AL_NalParser parser =
  {
    parseAndApplySPS,
    parsePPSandUpdateConcealment,
    NULL,
    AL_AVC_ParseSEI,
    decodeSliceData,
    isSliceData,
  };
  AL_DecodeOneNal(nuts, parser, pAUP, pCtx, eNUT, bIsLastAUNal, iNumSlice);
}

/*****************************************************************************/
void AL_AVC_InitAUP(AL_TAvcAup* pAUP)
{
  for(int i = 0; i < AL_AVC_MAX_PPS; ++i)
    pAUP->pPPS[i].bConceal = true;

  for(int i = 0; i < AL_AVC_MAX_SPS; ++i)
    pAUP->pSPS[i].bConceal = true;

  pAUP->ePictureType = SLICE_I;
  pAUP->pActiveSPS = NULL;
}

/*****************************************************************************/
AL_ERR CreateAvcDecoder(AL_TDecoder** hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  return AL_CreateDefaultDecoder((AL_TDecoder**)hDec, pDecChannel, pAllocator, pSettings, pCB);
}

