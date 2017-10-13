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
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common/Error.h"
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


/*************************************************************************//*!
   \brief Copies the final scaling list from pPPS to pSCL
*****************************************************************************/
static void AL_AVC_sCopyScalingList(AL_TAvcPps* pPPS, AL_TScl* pSCL)
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
static bool AL_AVC_sConstructRefPicList(AL_TAvcSliceHdr* pSlice, AL_TDecCtx* pCtx, TBufferListRef* pListRef)
{
  uint8_t iRef;
  uint8_t i, pNumRef[2] =
  {
    0, 0
  };
  AL_TPictMngrCtx* pPictMngrCtx = &pCtx->m_PictMngr;

  AL_Dpb_PictNumberProcess(&pPictMngrCtx->m_DPB, pSlice);

  AL_AVC_PictMngr_InitPictList(pPictMngrCtx, pSlice, pListRef);
  AL_AVC_PictMngr_ReorderPictList(pPictMngrCtx, pSlice, pListRef);

  // Clip reference list
  iRef = pSlice->num_ref_idx_l0_active_minus1 + 1;

  for(; iRef < MAX_REF; ++iRef)
    (*pListRef)[0][iRef].uNodeID = 0xFF;

  iRef = pSlice->num_ref_idx_l1_active_minus1 + 1;

  for(; iRef < MAX_REF; ++iRef)
    (*pListRef)[1][iRef].uNodeID = 0xFF;

  for(i = 0; i < 16; ++i)
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

/******************************************************************************/
static void ConcealAvcSlice(TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, AL_ENut eNUT)
{
  AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
  AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, true);

  pSP->eSliceType = SLICE_CONCEAL;
  pCtx->m_error = AL_WARN_CONCEAL_DETECT;

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

/******************************************************************************/
static void ProcessAvcScalingList(AL_TAvcAup* pAUP, AL_TAvcSliceHdr* pSlice, AL_TScl* pScl)
{
  int ppsid = pSlice->pic_parameter_set_id;

  AL_CleanupMemory(pScl, sizeof(*pScl));
  AL_AVC_sCopyScalingList(&pAUP->m_pPPS[ppsid], pScl);
}

/******************************************************************************/
static AL_TCropInfo AL_AVC_sGetCropInfo(const AL_TAvcSps* pSPS)
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
uint8_t AL_AVC_GetMaxBitDepth(const AL_TAvcSps* pSPS)
{
  assert(pSPS);
  switch(pSPS->profile_idc)
  {
  case 66:
  case 77:
  case 88:
  case 100: return 8;
  default: return 10;
  }
}

#include "limits.h"
/*************************************************************************/
static int getMaxNumberOfSlices(const AL_TAvcSps* pSps)
{
  int maxSlicesCountSupported = Avc_GetMaxNumberOfSlices(122, 52, 1, 60, INT_MAX);
  return maxSlicesCountSupported; /* TODO : fix bad behaviour in firmware to decrease dynamically the number of slices */

  int macroblocksCountInPicture = (pSps->pic_width_in_mbs_minus1 + 1) * (pSps->pic_height_in_map_units_minus1 + 1);

  int numUnitsInTick = 1, timeScale = 1;

  if(pSps->vui_parameters_present_flag)
  {
    numUnitsInTick = pSps->vui_param.vui_num_units_in_tick;
    timeScale = pSps->vui_param.vui_time_scale;
  }
  return Min(maxSlicesCountSupported, Avc_GetMaxNumberOfSlices(pSps->profile_idc, pSps->level_idc, numUnitsInTick, timeScale, macroblocksCountInPicture));
}

/******************************************************************************/
static bool AL_AVC_sInitDecoder(AL_TAvcSliceHdr* pSlice, AL_TDecCtx* pCtx)
{
  const AL_TAvcSps* pSps = pSlice->m_pSPS;
  const AL_TCropInfo tCropInfo = AL_AVC_sGetCropInfo(pSps);

  // fast access
  const uint8_t uMaxBitDepth = AL_AVC_GetMaxBitDepth(pSps);
  const AL_TDimension tDim = { (pSps->pic_width_in_mbs_minus1 + 1) << 4, (pSps->pic_height_in_map_units_minus1 + 1) << 4 };

  AL_TDecChanParam* pChanParam = &pCtx->m_chanParam;
  pChanParam->iWidth = tDim.iWidth;
  pChanParam->iHeight = tDim.iHeight;
  const AL_EChromaMode eChromaMode = (AL_EChromaMode)pSps->chroma_format_idc;

  if(eChromaMode >= CHROMA_MAX_ENUM)
    return false;

  pChanParam->iMaxSlices = getMaxNumberOfSlices(pSps);

  uint32_t uSizeWP = pChanParam->iMaxSlices * WP_SLICE_SIZE;
  uint32_t uSizeSP = pChanParam->iMaxSlices * sizeof(AL_TDecSliceParam);

  AL_ERR errorCode = AL_ERROR;
#define SAFE_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
    { \
      errorCode = AL_ERR_NO_MEMORY; \
      goto fail; \
    } \
  } while(0)

  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    SAFE_ALLOC(pCtx, &pCtx->m_PoolWP[i].tMD, uSizeWP, "wp");
    AL_CleanupMemory(pCtx->m_PoolWP[i].tMD.pVirtualAddr, pCtx->m_PoolWP[i].tMD.uSize);
    SAFE_ALLOC(pCtx, &pCtx->m_PoolSP[i].tMD, uSizeSP, "sp");
  }

  if(!pCtx->m_bForceFrameRate && pSps->vui_parameters_present_flag)
  {
    pChanParam->uFrameRate = pSps->vui_param.vui_time_scale / 2;
    pChanParam->uClkRatio = pSps->vui_param.vui_num_units_in_tick;
  }

  pChanParam->bParallelWPP = false;

  AL_TIDecChannelCallbacks CBs = { 0 };
  CBs.endFrameDecodingCB.func = &AL_Decoder_EndDecoding;
  CBs.endFrameDecodingCB.userParam = pCtx;

  errorCode = AL_IDecChannel_Configure(pCtx->m_pDecChannel, &pCtx->m_chanParam, &CBs);

  if(errorCode != AL_SUCCESS)
  {
    pCtx->m_eChanState = CHAN_INVALID;
    pCtx->m_error = errorCode;
    return false;
  }
  pCtx->m_eChanState = CHAN_CONFIGURED;


  // Alloc respect to resolution to limit memory footprint
  const uint32_t uSizeCompData = AL_GetAllocSize_AvcCompData(tDim.iWidth, tDim.iHeight, eChromaMode);
  const uint32_t uSizeCompMap = AL_GetAllocSize_CompMap(tDim.iWidth, tDim.iHeight);

  for(int iComp = 0; iComp < pCtx->m_iStackSize; ++iComp)
  {
    SAFE_ALLOC(pCtx, &pCtx->m_PoolCompData[iComp].tMD, uSizeCompData, "comp data");
    SAFE_ALLOC(pCtx, &pCtx->m_PoolCompMap[iComp].tMD, uSizeCompMap, "comp map");
  }

  const uint32_t uNumMBs = (pSps->pic_width_in_mbs_minus1 + 1) * (pSps->pic_height_in_map_units_minus1 + 1);
  assert(uNumMBs == ((tDim.iWidth / 16u) * (tDim.iHeight / 16u)));
  const int iLevel = pSps->constraint_set3_flag ? 9 : pSps->level_idc; /* We treat constraint set 3 as a level 9 */
  const int iDpbBuf = AL_AVC_GetMaxDPBSize(iLevel, tDim.iWidth, tDim.iHeight, pCtx->m_eDpbMode);
  const int iMaxBuf = iDpbBuf + pCtx->m_iStackSize + 1; // 1 Buffer for concealment

  const uint32_t uSizeMV = AL_GetAllocSize_MV(tDim.iWidth, tDim.iHeight, pChanParam->eCodec == AL_CODEC_AVC);
  const uint32_t uSizePOC = POCBUFF_PL_SIZE;

  for(int i = 0; i < iMaxBuf; ++i)
  {
    SAFE_ALLOC(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pMvBufs[i].tMD, uSizeMV, "mv");
    SAFE_ALLOC(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pPocBufs[i].tMD, uSizePOC, "poc");
  }

  AL_PictMngr_Init(&pCtx->m_PictMngr, pChanParam->eCodec == AL_CODEC_AVC, tDim.iWidth, tDim.iHeight, iMaxBuf, iDpbBuf, pCtx->m_eDpbMode);
  pCtx->m_PictMngr.m_bFirstInit = true;

  if(pCtx->m_resolutionFoundCB.func)
  {
    const int iSizeYuv = AL_GetAllocSize_Frame(tDim, eChromaMode, uMaxBitDepth, pChanParam->bFrameBufferCompression, pChanParam->eFBStorageMode);
    pCtx->m_resolutionFoundCB.func(iMaxBuf, iSizeYuv, tDim.iWidth, tDim.iHeight, tCropInfo, AL_GetDecFourCC(eChromaMode, uMaxBitDepth), pCtx->m_resolutionFoundCB.userParam);
  }

  /* verify the number of yuv buffers */
  for(int i = 0; i < iMaxBuf; ++i)
    assert(pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[i]);

  return true;

  fail:
  pCtx->m_eChanState = CHAN_INVALID;
  pCtx->m_error = errorCode;

  return false;
}

/******************************************************************************/
static bool InitAvcSlice(TDecCtx* pCtx, AL_TAvcSliceHdr* pSlice)
{
  if(!pCtx->m_PictMngr.m_bFirstInit)
  {
    if(!AL_AVC_sInitDecoder(pSlice, pCtx))
    {
      pSlice->m_pPPS = &pCtx->m_AvcAup.m_pPPS[pCtx->m_tConceal.m_iLastPPSId];
      pSlice->m_pSPS = pSlice->m_pPPS->m_pSPS;
      return false;
    }
  }
  int ppsid = pSlice->pic_parameter_set_id;
  int spsid = pCtx->m_AvcAup.m_pPPS[ppsid].seq_parameter_set_id;

  pCtx->m_AvcAup.m_pActiveSPS = &pCtx->m_AvcAup.m_pSPS[spsid];

  return true;
}

/*************************************************************************//*!
   \brief This function initializes a AVC Access Unit instance
   \param[in] pAUP Pointer to the Access Unit object we want to initialize
*****************************************************************************/
void AL_AVC_InitAUP(AL_TAvcAup* pAUP)
{
  int i;

  for(i = 0; i < AL_AVC_MAX_PPS; ++i)
    pAUP->m_pPPS[i].bConceal = true;

  for(i = 0; i < AL_AVC_MAX_SPS; ++i)
    pAUP->m_pSPS[i].bConceal = true;

  pAUP->ePictureType = SLICE_I;
  pAUP->m_pActiveSPS = 0;
}

/*****************************************************************************/
static void UpdateAvcPictMngr(AL_TPictMngrCtx* pCtx, AL_ENut eNUT, AL_TDecPicParam* pPP, AL_TAvcSliceHdr* pSlice)
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

/*****************************************************************************/
static void CreateConcealAvcSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, AL_TDecPicBuffers* pBufs)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  ConcealAvcSlice(pCtx, pPP, pSP, pSlice, false);
  AL_InitIntermediateBuffers(pCtx, pBufs);
  pSP->FirstLcuSliceSegment = 0;
  pSP->FirstLcuSlice = 0;
  pSP->FirstLCU = 0;
  pSP->NextSliceSegment = pSlice->first_mb_in_slice;
  pSP->NumLCU = pSlice->first_mb_in_slice;

  pSlice->slice_type = uCurSliceType;
}

/******************************************************************************/
static void EndAvcFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice, AL_TDecPicParam* pPP)
{
  UpdateAvcPictMngr(&pCtx->m_PictMngr, eNUT, pPP, pSlice);

  if(pCtx->m_eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);
  else
    AL_LaunchSliceDecoding(pCtx, true);
  UpdateContextAtEndOfFrame(pCtx);
}

/*****************************************************************************/
static void FinishAvcPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_TAvcSliceHdr* pSlice = &pCtx->m_AvcSliceHdr[pCtx->m_uCurID];
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice - 1]);

  pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice - 1;
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, true, true);

  // copy Stream Offset from previous command
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = pCtx->m_iStreamOffset[(pCtx->m_iNumFrmBlk1 + pCtx->m_iStackSize - 1) % pCtx->m_iStackSize];

  if(pCtx->m_eDecUnit == AL_VCL_NAL_UNIT) /* launch HW for each vcl nal in sub_frame latency*/
    --pCtx->m_PictMngr.m_uNumSlice;

  EndAvcFrame(pCtx, pSlice->nal_unit_type, pSlice, pPP);
}

/*****************************************************************************/
static bool decodeSliceData(AL_TAvcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bFirstSliceInFrameIsValid, int* iNumSlice, bool* bBeginFrameIsValid)
{
  uint8_t uToggleID = (~pCtx->m_uCurID) & 0x01;
  AL_TDecPicParam* pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
  AL_TDecPicBuffers* pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];

  bool bIsRAP = (eNUT == AL_AVC_NUT_VCL_IDR);

  TCircBuffer* pBufStream = &pCtx->m_Stream;
  // Slice header deanti-emulation
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pCtx->m_BufNoAE.tMD.pVirtualAddr, &rp);

  // Parse Slice Header
  AL_TAvcSliceHdr* pSlice = &pCtx->m_AvcSliceHdr[uToggleID];
  Rtos_Memset(pSlice, 0, sizeof(AL_TAvcSliceHdr));
  AL_TConceal* pConceal = &pCtx->m_tConceal;
  bool bRet = AL_AVC_ParseSliceHeader(pSlice, &rp, pConceal, pAUP->m_pPPS);
  bool bSliceBelongsToSameFrame = true;

  if(bRet)
    bSliceBelongsToSameFrame = (!pSlice->first_mb_in_slice || (pSlice->pic_order_cnt_lsb == pCtx->m_uCurPocLsb));

  if(!bSliceBelongsToSameFrame && *bFirstIsValid && *bFirstSliceInFrameIsValid)
  {
    FinishAvcPreviousFrame(pCtx);
    pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];
    pPP = &pCtx->m_PoolPP[pCtx->m_uToggle];
    *bFirstSliceInFrameIsValid = false;
    *bBeginFrameIsValid = false;
  }

  if(bRet)
  {
    pCtx->m_uCurPocLsb = pSlice->pic_order_cnt_lsb;

    if(!InitAvcSlice(pCtx, pSlice))
      return false;
  }

  pCtx->m_uCurID = (pCtx->m_uCurID + 1) & 1;
  AL_TDecSliceParam* pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice]);

  if(!(*bFirstSliceInFrameIsValid) && pSlice->first_mb_in_slice)
  {
    if(!pSlice->m_pSPS)
      pSlice->m_pSPS = pAUP->m_pActiveSPS;
    CreateConcealAvcSlice(pCtx, pPP, pSP, pSlice, pBufs);

    pSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[++pCtx->m_PictMngr.m_uNumSlice]);
    *bFirstSliceInFrameIsValid = true;
  }

  if(bRet)
  {
    uint8_t ppsid = pSlice->pic_parameter_set_id;
    uint8_t spsid = pAUP->m_pPPS[ppsid].seq_parameter_set_id;

    if(!pCtx->m_VideoConfiguration.bInit)
      AL_AVC_UpdateVideoConfiguration(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    bRet = AL_AVC_IsVideoConfigurationCompatible(&pCtx->m_VideoConfiguration, &pAUP->m_pSPS[spsid]);

    if(!bRet)
      pAUP->m_pSPS[spsid].bConceal = true;
  }

  if(bRet && !pSlice->first_mb_in_slice)
    *bFirstSliceInFrameIsValid = true;

  if(bRet && pSlice->slice_type != SLICE_I)
    AL_SET_DEC_OPT(pPP, IntraOnly, 0);

  if(bIsLastAUNal)
    pPP->num_slice = pCtx->m_PictMngr.m_uNumSlice;

  pBufs->tStream.tMD = pCtx->m_Stream.tMD;

  if(!pSlice->m_pSPS)
    pSlice->m_pSPS = pAUP->m_pActiveSPS;

  // Compute Current POC
  if(bRet && !pSlice->first_mb_in_slice)
    bRet = AL_AVC_PictMngr_SetCurrentPOC(&pCtx->m_PictMngr, pSlice);

  // compute check gaps in frameNum
  if(bRet)
    AL_AVC_PictMngr_Fill_Gap_In_FrameNum(&pCtx->m_PictMngr, pSlice);

  if(!(*bBeginFrameIsValid) && pSlice->m_pSPS)
  {
    AL_InitFrameBuffers(pCtx, (pSlice->m_pSPS->pic_width_in_mbs_minus1 + 1) << 4, (pSlice->m_pSPS->pic_height_in_map_units_minus1 + 1) << 4, pPP);
    *bBeginFrameIsValid = true;
  }

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
        pCtx->m_uMaxBD = AL_AVC_GetMaxBitDepth(pSlice->m_pSPS);
      }
      else
        return SkipNal();
    }

    // update Nal Unit size
    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    ProcessAvcScalingList(pAUP, pSlice, &ScalingList);

    if(!pCtx->m_PictMngr.m_uNumSlice)
    {
      AL_AVC_FillPictParameters(pSlice, pCtx, pPP);
      AL_InitIntermediateBuffers(pCtx, pBufs);
    }
    AL_AVC_FillSliceParameters(pSlice, pCtx, pSP, pPP, false);

    if(!AL_AVC_sConstructRefPicList(pSlice, pCtx, &pCtx->m_ListRef))
    {
      ConcealAvcSlice(pCtx, pPP, pSP, pSlice, eNUT);
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
    ConcealAvcSlice(pCtx, pPP, pSP, pSlice, eNUT);

    if(!pCtx->m_PictMngr.m_uNumSlice)
      AL_InitIntermediateBuffers(pCtx, pBufs);

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
  AL_AVC_PrepareCommand(pCtx, &ScalingList, pPP, pBufs, pSP, pSlice, bIsLastAUNal || bLastSlice, bRet);

  ++pCtx->m_PictMngr.m_uNumSlice;
  ++(*iNumSlice);

  if(pAUP->ePictureType == SLICE_I || pSlice->slice_type == SLICE_B)
    pAUP->ePictureType = (AL_ESliceType)pSlice->slice_type;

  if(bIsLastAUNal || bLastSlice)
  {
    EndAvcFrame(pCtx, eNUT, pSlice, pPP);
    pAUP->ePictureType = SLICE_I;
    return true;
  }
  else if(pCtx->m_eDecUnit == AL_VCL_NAL_UNIT)
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

/*****************************************************************************/
bool AL_AVC_DecodeOneNAL(AL_TAvcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bFirstSliceInFrameIsValid, int* iNumSlice, bool* bBeginFrameIsValid)
{
  if(isSliceData(eNUT))
    return decodeSliceData(pAUP, pCtx, eNUT, bIsLastAUNal, bFirstIsValid, bFirstSliceInFrameIsValid, iNumSlice, bBeginFrameIsValid);
  switch(eNUT)
  {
  // Parser SEI
  case AL_AVC_NUT_SEI:
  {
    AL_TRbspParser rp = getParserOnNonVclNal(pCtx);
    AL_TAvcSei sei;
    AL_AVC_ParseSEI(&sei, &rp, pAUP->m_pSPS, &pAUP->m_pActiveSPS);
    break;
  }

  // Parser SPS
  case AL_AVC_NUT_SPS:
  {
    AL_TRbspParser rp = getParserOnNonVclNal(pCtx);
    AL_PARSE_RESULT Ret = AL_AVC_ParseSPS(pAUP->m_pSPS, &rp);

    if(Ret == AL_UNSUPPORTED)
      pCtx->m_error = AL_ERR_NOT_SUPPORTED;
    break;
  }

  // Parser PPS
  case AL_AVC_NUT_PPS:
  {
    AL_TRbspParser rp = getParserOnNonVclNal(pCtx);
    AL_PARSE_RESULT Ret = AL_AVC_ParsePPS(pAUP->m_pPPS, &rp, pAUP->m_pSPS);

    if(Ret == AL_UNSUPPORTED)
      pCtx->m_error = AL_ERR_NOT_SUPPORTED;

    if(!pAUP->m_pPPS->bConceal)
      pCtx->m_tConceal.m_bHasPPS = true;
    break;
  }

  // other NAL unit we discard
  default:
    break;
  }

  return false;
}

