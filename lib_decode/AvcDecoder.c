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
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/Error.h"
#include "lib_common/FourCC.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/RbspParser.h"

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
static AL_TCropInfo getCropInfo(AL_TAvcSps const* pSPS)
{
  AL_TCropInfo tCropInfo =
  {
    false, 0, 0, 0, 0
  };

  tCropInfo.bCropping = pSPS->frame_cropping_flag;

  if(tCropInfo.bCropping)
  {
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
static int getMaxNumberOfSlices(AL_TAvcSps const* pSPS)
{
  int maxSlicesCountSupported = Avc_GetMaxNumberOfSlices(122, 52, 1, 60, INT32_MAX);
  return maxSlicesCountSupported; /* TODO : fix bad behaviour in firmware to decrease dynamically the number of slices */

  int macroblocksCountInPicture = (pSPS->pic_width_in_mbs_minus1 + 1) * (pSPS->pic_height_in_map_units_minus1 + 1);

  int numUnitsInTick = 1, timeScale = 1;

  if(pSPS->vui_parameters_present_flag)
  {
    numUnitsInTick = pSPS->vui_param.vui_num_units_in_tick;
    timeScale = pSPS->vui_param.vui_time_scale;
  }
  return Min(maxSlicesCountSupported, Avc_GetMaxNumberOfSlices(pSPS->profile_idc, pSPS->level_idc, numUnitsInTick, timeScale, macroblocksCountInPicture));
}

/*****************************************************************************/
static bool isSPSCompatibleWithStreamSettings(AL_TAvcSps const* pSPS, AL_TStreamSettings const* pStreamSettings)
{
  const int iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSLumaBitDepth))
    return false;

  const int iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSChromaBitDepth))
    return false;

  const int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_idc);

  if((pStreamSettings->iBitDepth > 0) && (pStreamSettings->iBitDepth < iSPSMaxBitDepth))
    return false;

  const int iSPSLevel = pSPS->constraint_set3_flag ? 9 : pSPS->level_idc; /* We treat constraint set 3 as a level 9 */

  if((pStreamSettings->iLevel > 0) && (pStreamSettings->iLevel < iSPSLevel))
    return false;

  const AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if((pStreamSettings->eChroma != CHROMA_MAX_ENUM) && (pStreamSettings->eChroma < eSPSChromaMode))
    return false;

  const AL_TCropInfo tSPSCropInfo = getCropInfo(pSPS);
  const int iSPSCropWidth = tSPSCropInfo.uCropOffsetLeft + tSPSCropInfo.uCropOffsetRight;
  const AL_TDimension tSPSDim = { (pSPS->pic_width_in_mbs_minus1 + 1) * 16, (pSPS->pic_height_in_map_units_minus1 + 1) * 16 };

  if((pStreamSettings->tDim.iWidth > 0) && (pStreamSettings->tDim.iWidth < (tSPSDim.iWidth - iSPSCropWidth)))
    return false;

  const int iSPSCropHeight = tSPSCropInfo.uCropOffsetTop + tSPSCropInfo.uCropOffsetBottom;

  if((pStreamSettings->tDim.iHeight > 0) && (pStreamSettings->tDim.iHeight < (tSPSDim.iHeight - iSPSCropHeight)))
    return false;

  return true;
}

int AVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack);
/******************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS)
{
  const AL_TDimension tSPSDim = { (pSPS->pic_width_in_mbs_minus1 + 1) * 16, (pSPS->pic_height_in_map_units_minus1 + 1) * 16 };
  const int iNumMBs = (pSPS->pic_width_in_mbs_minus1 + 1) * (pSPS->pic_height_in_map_units_minus1 + 1);
  assert(iNumMBs == ((tSPSDim.iWidth / 16) * (tSPSDim.iHeight / 16)));
  const int iSPSLevel = pSPS->constraint_set3_flag ? 9 : pSPS->level_idc; /* We treat constraint set 3 as a level 9 */
  const int iSPSMaxSlices = getMaxNumberOfSlices(pSPS);
  const int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  const int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  const AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;
  const int iSizeCompData = AL_GetAllocSize_AvcCompData(tSPSDim, eSPSChromaMode);
  const int iSizeCompMap = AL_GetAllocSize_CompMap(tSPSDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  const int iDpbMaxBuf = AL_AVC_GetMaxDPBSize(iSPSLevel, tSPSDim.iWidth, tSPSDim.iHeight, pCtx->m_eDpbMode);

  const int iMaxBuf = AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->m_iStackSize);

  const int iSizeMV = AL_GetAllocSize_AvcMV(tSPSDim);
  const int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  const int iDpbRef = iDpbMaxBuf;
  const AL_EFbStorageMode eStorageMode = pCtx->m_chanParam.eFBStorageMode;
  AL_PictMngr_Init(&pCtx->m_PictMngr, iMaxBuf, iSizeMV, iDpbRef, pCtx->m_eDpbMode, eStorageMode);

  if(pCtx->m_resolutionFoundCB.func)
  {
    const int iSPSMaxBitDepth = getMaxBitDepth(pSPS->profile_idc);

    const bool useFBC = pCtx->m_chanParam.bFrameBufferCompression;
    const int iSizeYuv = AL_GetAllocSize_Frame(tSPSDim, eSPSChromaMode, iSPSMaxBitDepth, useFBC, eStorageMode);
    const AL_TCropInfo tCropInfo = getCropInfo(pSPS);

    pCtx->m_tStreamSettings.tDim = tSPSDim;
    pCtx->m_tStreamSettings.eChroma = eSPSChromaMode;
    pCtx->m_tStreamSettings.iBitDepth = iSPSMaxBitDepth;
    pCtx->m_tStreamSettings.iLevel = pSPS->constraint_set3_flag ? 9 : pSPS->level_idc;
    pCtx->m_tStreamSettings.iProfileIdc = pSPS->profile_idc;

    pCtx->m_resolutionFoundCB.func(iMaxBuf, iSizeYuv, &pCtx->m_tStreamSettings, &tCropInfo, pCtx->m_resolutionFoundCB.userParam);
  }

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, AL_ERR_NO_MEMORY);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS)
{
  const AL_TDimension tSPSDim = { (pSPS->pic_width_in_mbs_minus1 + 1) * 16, (pSPS->pic_height_in_map_units_minus1 + 1) * 16 };
  AL_TDecChanParam* pChan = &pCtx->m_chanParam;
  pChan->iWidth = tSPSDim.iWidth;
  pChan->iHeight = tSPSDim.iHeight;

  const int iSPSMaxSlices = getMaxNumberOfSlices(pSPS);
  pChan->iMaxSlices = iSPSMaxSlices;

  if(!pCtx->m_bForceFrameRate && pSPS->vui_parameters_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale / 2;
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
  AL_TAvcAup* aup = &pCtx->m_aup.avcAup;

  if(!pCtx->m_bIsFirstSPSChecked)
  {
    if(!isSPSCompatibleWithStreamSettings(pSlice->m_pSPS, &pCtx->m_tStreamSettings))
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

    }

    pCtx->m_bIsBuffersAllocated = true;

    if(!initChannel(pCtx, pSlice->m_pSPS))
      return false;
  }

  int const spsid = sliceSpsId(aup->m_pPPS, pSlice);
  aup->m_pActiveSPS = &aup->m_pSPS[spsid];

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
  copyScalingList(&pAUP->m_pPPS[ppsid], pScl);
}

/******************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, AL_ENut eNUT)
{
  AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, true);

  pSP->eSliceType = SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT);

  if(eNUT == 5)
  {
    pCtx->m_PictMngr.m_iCurFramePOC = 0;
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
  pCtx->m_iPrevFrameNum = pSlice->frame_num;
  pCtx->m_bLastIsIDR = bClearRef ? true : false;

  AL_AVC_PictMngr_UpdateRecInfo(pCtx, pSlice->m_pSPS, pPP);
  AL_AVC_PictMngr_EndParsing(pCtx, bClearRef, eMarkingFlag);

  if(pSlice->nal_ref_idc)
  {
    AL_Dpb_MarkingProcess(&pCtx->m_DPB, pSlice);

    if(AL_Dpb_LastHasMMCO5(&pCtx->m_DPB))
    {
      pCtx->m_iPrevFrameNum = 0;
      pCtx->m_iCurFramePOC = 0;
    }
  }
  AL_AVC_PictMngr_CleanDPB(pCtx);
}

/******************************************************************************/
static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice, AL_TDecPicParam* pPP)
{
  updatePictManager(&pCtx->m_PictMngr, eNUT, pPP, pSlice);

  if(pCtx->m_chanParam.eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);
  else
    AL_LaunchSliceDecoding(pCtx, true);
  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_TAvcSliceHdr* pSlice = &pCtx->m_AvcSliceHdr[pCtx->m_uCurID];
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice - 1]);

  pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice - 1;
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy stream offset from previous command
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = pCtx->m_iStreamOffset[(pCtx->m_iNumFrmBlk1 + pCtx->m_iStackSize - 1) % pCtx->m_iStackSize];

  if(pCtx->m_chanParam.eDecUnit == AL_VCL_NAL_UNIT) /* launch HW for each vcl nal in sub_frame latency*/
    --pCtx->m_PictMngr.m_uNumSlice;

  endFrame(pCtx, pSlice->nal_unit_type, pSlice, pPP);
}

/******************************************************************************/
static bool constructRefPicList(AL_TAvcSliceHdr* pSlice, AL_TDecCtx* pCtx, TBufferListRef* pListRef)
{
  AL_TPictMngrCtx* pPictMngrCtx = &pCtx->m_PictMngr;

  AL_Dpb_PictNumberProcess(&pPictMngrCtx->m_DPB, pSlice);

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
static bool decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  bool* bBeginFrameIsValid = &pCtx->m_bBeginFrameIsValid;
  bool* bFirstIsValid = &pCtx->m_bFirstIsValid;
  bool* bFirstSliceInFrameIsValid = &pCtx->m_bFirstSliceInFrameIsValid;

  // Slice header deanti-emulation
  AL_TRbspParser rp;
  TCircBuffer* pBufStream = &pCtx->m_Stream;
  InitRbspParser(pBufStream, pCtx->m_BufNoAE.tMD.pVirtualAddr, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->m_uCurID) & 0x01;
  AL_TAvcSliceHdr* pSlice = &pCtx->m_AvcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(AL_TAvcSliceHdr));
  AL_TConceal* pConceal = &pCtx->m_tConceal;
  AL_TAvcAup* pAUP = &pIAUP->avcAup;
  bool isValid = AL_AVC_ParseSliceHeader(pSlice, &rp, pConceal, pAUP->m_pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(isValid)
    bSliceBelongsToSameFrame = (!pSlice->first_mb_in_slice || (pSlice->pic_order_cnt_lsb == pCtx->m_uCurPocLsb));

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
    pCtx->m_uCurPocLsb = pSlice->pic_order_cnt_lsb;

    if(!initSlice(pCtx, pSlice))
      return false;
  }

  pCtx->m_uCurID = (pCtx->m_uCurID + 1) & 1;
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice]);

  if(!(*bFirstSliceInFrameIsValid) && pSlice->first_mb_in_slice)
  {
    if(!pSlice->m_pSPS)
      pSlice->m_pSPS = pAUP->m_pActiveSPS;
    createConcealSlice(pCtx, pPP, pSP, pSlice);

    pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[++pCtx->m_PictMngr.m_uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(isValid)
  {
    int const spsid = sliceSpsId(pAUP->m_pPPS, pSlice);
    isValid = isSPSCompatibleWithStreamSettings(&pAUP->m_pSPS[spsid], &pCtx->m_tStreamSettings);

    if(!isValid)
      pAUP->m_pSPS[spsid].bConceal = true;
  }

  if(isValid && !pSlice->first_mb_in_slice)
    *bFirstSliceInFrameIsValid = true;

  if(isValid && pSlice->slice_type != SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  if(bIsLastAUNal)
    pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice;

  pBufs->tStream.tMD = pCtx->m_Stream.tMD;

  if(!pSlice->m_pSPS)
    pSlice->m_pSPS = pAUP->m_pActiveSPS;

  // Compute Current POC
  if(isValid && !pSlice->first_mb_in_slice)
    isValid = AL_AVC_PictMngr_SetCurrentPOC(&pCtx->m_PictMngr, pSlice);

  // compute check gaps in frameNum
  if(isValid)
    AL_AVC_PictMngr_Fill_Gap_In_FrameNum(&pCtx->m_PictMngr, pSlice);

  if(!(*bBeginFrameIsValid) && pSlice->m_pSPS)
  {
    AL_TDimension const tDim = { (pSlice->m_pSPS->pic_width_in_mbs_minus1 + 1) * 16, (pSlice->m_pSPS->pic_height_in_map_units_minus1 + 1) * 16 };

    if(!AL_InitFrameBuffers(pCtx, pBufs, tDim, pPP))
      return false;
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
      bool bIsRAP = (eNUT == AL_AVC_NUT_VCL_IDR);

      if(bIsRAP)
      {
        *bFirstIsValid = true;
        pCtx->m_uMaxBD = getMaxBitDepth(pSlice->m_pSPS->profile_idc);
      }
      else
        return SkipNal();
    }

    // update Nal Unit size
    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(!pCtx->m_PictMngr.m_uNumSlice)
    {
      AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
    }
    AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, false);

    if(!constructRefPicList(pSlice, pCtx, &pCtx->m_ListRef))
    {
      concealSlice(pCtx, pPP, pSP, pSlice, eNUT);
    }
    else
    {
      AL_AVC_FillSlicePicIdRegister(pCtx, pSP);
      pConceal->m_bValidFrame = true;
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

    return SkipNal();
  }

  // Launch slice decoding
  AL_AVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pCtx->m_PictMngr.m_uNumSlice;
  ++(*iNumSlice);

  if(pAUP->ePictureType == SLICE_I || pSlice->slice_type == SLICE_B)
    pAUP->ePictureType = (AL_ESliceType)pSlice->slice_type;

  if(bIsLastAUNal || bLastSlice)
  {
    endFrame(pCtx, eNUT, pSlice, pPP);
    pAUP->ePictureType = SLICE_I;
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
  case AL_AVC_NUT_VCL_NON_IDR:
  case AL_AVC_NUT_VCL_IDR:
    return true;
  default:
    return false;
  }
}

static AL_PARSE_RESULT parsePPSandUpdateConcealment(AL_TAup* aup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  AL_PARSE_RESULT result = AL_AVC_ParsePPS(aup, rp);

  if(!aup->avcAup.m_pPPS->bConceal)
    pCtx->m_tConceal.m_bHasPPS = true;

  return result;
}

/*****************************************************************************/
bool AL_AVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice)
{
  AL_NonVclNuts nuts =
  {
    AL_AVC_NUT_PREFIX_SEI,
    AL_AVC_NUT_SPS,
    AL_AVC_NUT_PPS,
    0,
    AL_AVC_NUT_EOS,
  };

  AL_NalParser parser =
  {
    AL_AVC_ParseSPS,
    parsePPSandUpdateConcealment,
    NULL,
    AL_AVC_ParseSEI,
    decodeSliceData,
    isSliceData,
  };

  return AL_DecodeOneNal(nuts, parser, pAUP, pCtx, eNUT, bIsLastAUNal, iNumSlice);
}

/*****************************************************************************/
void AL_AVC_InitAUP(AL_TAvcAup* pAUP)
{
  for(int i = 0; i < AL_AVC_MAX_PPS; ++i)
    pAUP->m_pPPS[i].bConceal = true;

  for(int i = 0; i < AL_AVC_MAX_SPS; ++i)
    pAUP->m_pSPS[i].bConceal = true;

  pAUP->ePictureType = SLICE_I;
  pAUP->m_pActiveSPS = NULL;
}

