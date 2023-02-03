/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include "Avc_PictMngr.h"
#include "AvcParser.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferPixMapMeta.h"
#include "lib_common/AvcUtils.h"

/*****************************************************************************/
static void AL_sGetPocType0(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  int iPrevPocMSB = 0;
  uint32_t uPrevPocLSB = 0;
  uint32_t uMaxPocLSB = 1u << (pSlice->pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4);

  if(!AL_AVC_IsIDR(pSlice->nal_unit_type))
  {
    if(AL_Dpb_LastHasMMCO5(&pCtx->DPB))
    {
      /*warning : work in frame only*/
      uPrevPocLSB = pCtx->iTopFieldOrderCnt;
    }
    else
    {
      iPrevPocMSB = pCtx->iPrevPocMSB;
      uPrevPocLSB = pCtx->uPrevPocLSB;
    }
  }

  if((pSlice->pic_order_cnt_lsb < uPrevPocLSB) &&
     ((uPrevPocLSB - pSlice->pic_order_cnt_lsb) >= (uMaxPocLSB / 2)))
    iPrevPocMSB = iPrevPocMSB + uMaxPocLSB;

  else if((pSlice->pic_order_cnt_lsb > uPrevPocLSB) &&
          ((pSlice->pic_order_cnt_lsb - uPrevPocLSB) > (uMaxPocLSB / 2)))
    iPrevPocMSB = iPrevPocMSB - uMaxPocLSB;

  /*warning : work in frame only*/
  pCtx->iTopFieldOrderCnt = iPrevPocMSB + pSlice->pic_order_cnt_lsb;
  pCtx->iBotFieldOrderCnt = pCtx->iTopFieldOrderCnt + pSlice->delta_pic_order_cnt_bottom;

  if(pSlice->nal_ref_idc)
  {
    pCtx->uPrevPocLSB = pSlice->pic_order_cnt_lsb;
    pCtx->iPrevPocMSB = iPrevPocMSB;
  }
}

/*****************************************************************************/
static void AL_sGetPocType1(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  int iExpectedDeltaPerPicOrderCntCycle = 0;
  bool bIsIDR = AL_AVC_IsIDR(pSlice->nal_unit_type);

  for(int i = 0; i < pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle; ++i)
    iExpectedDeltaPerPicOrderCntCycle += pSlice->pSPS->offset_for_ref_frame[i];

  if(!bIsIDR)
  {
    if(AL_Dpb_LastHasMMCO5(&pCtx->DPB))
    {
      pCtx->iPrevFrameNumOffset = 0;
      pCtx->iPrevFrameNum = 0;
    }
  }

  int iFrameNumOffset;
  uint32_t uMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(bIsIDR)
    iFrameNumOffset = 0;
  else if(pCtx->iPrevFrameNum > pSlice->frame_num)
    iFrameNumOffset = pCtx->iPrevFrameNumOffset + uMaxFrameNum;
  else
    iFrameNumOffset = pCtx->iPrevFrameNumOffset;

  int iAbsFrameNum = 0;

  if(pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle)
    iAbsFrameNum = iFrameNumOffset + pSlice->frame_num;

  if(!pSlice->nal_ref_idc && iAbsFrameNum > 0)
    iAbsFrameNum -= 1;

  int iExpectedPicOrderCnt = 0;

  if(iAbsFrameNum > 0)
  {
    int iPicOrderCntCycleCnt = (iAbsFrameNum - 1) / pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle;
    int iFrameNumInPicOrderCntCycle = (iAbsFrameNum - 1) % pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle;
    iExpectedPicOrderCnt = iPicOrderCntCycleCnt * iExpectedDeltaPerPicOrderCntCycle;

    for(int i = 0; i <= iFrameNumInPicOrderCntCycle; ++i)
      iExpectedPicOrderCnt += pSlice->pSPS->offset_for_ref_frame[i];
  }

  if(!pSlice->nal_ref_idc)
    iExpectedPicOrderCnt += pSlice->pSPS->offset_for_non_ref_pic;

  pCtx->iPrevFrameNumOffset = iFrameNumOffset;
  /*warning : work only in frame mode*/
  pCtx->iTopFieldOrderCnt = iExpectedPicOrderCnt + pSlice->delta_pic_order_cnt[0];
  pCtx->iBotFieldOrderCnt = pCtx->iTopFieldOrderCnt + pSlice->pSPS->offset_for_top_to_bottom_field +
                            pSlice->delta_pic_order_cnt[1];
}

/*****************************************************************************/
static void AL_sGetPocType2(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  bool bIsIDR = AL_AVC_IsIDR(pSlice->nal_unit_type);

  if(!bIsIDR)
  {
    if(AL_Dpb_LastHasMMCO5(&pCtx->DPB))
    {
      pCtx->iPrevFrameNumOffset = 0;
      pCtx->iPrevFrameNum = 0;
    }
  }

  int iFrameNumOffset;
  uint32_t uMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(bIsIDR)
    iFrameNumOffset = 0;
  else if(pCtx->iPrevFrameNum > pSlice->frame_num)
    iFrameNumOffset = pCtx->iPrevFrameNumOffset + uMaxFrameNum;
  else
    iFrameNumOffset = pCtx->iPrevFrameNumOffset;

  int iTempPicOrderCnt;

  if(bIsIDR)
    iTempPicOrderCnt = 0;
  else if(!pSlice->nal_ref_idc)
    iTempPicOrderCnt = 2 * (iFrameNumOffset + pSlice->frame_num) - 1;
  else
    iTempPicOrderCnt = 2 * (iFrameNumOffset + pSlice->frame_num);

  pCtx->iPrevFrameNumOffset = iFrameNumOffset;
  /*warning : work only in frame mode*/
  pCtx->iTopFieldOrderCnt = iTempPicOrderCnt;
  pCtx->iBotFieldOrderCnt = iTempPicOrderCnt;
}

/*****************************************************************************/
static void AL_AVC_sFillWPCoeff(AL_VADDR pDataWP, AL_TAvcSliceHdr const* pSlice, uint8_t uL0L1)
{
  uint8_t uNumRefIdx = (uL0L1 ? pSlice->num_ref_idx_l1_active_minus1 : pSlice->num_ref_idx_l0_active_minus1) + 1;
  uint32_t* pWP = (uint32_t*)(pDataWP + (uL0L1 * WP_ONE_SET_SIZE));

  AL_TWPCoeff const* pWpCoeff = &pSlice->pred_weight_table.tWpCoeff[uL0L1];

  for(int i = 0; i < uNumRefIdx; ++i)
  {
    pWP[0] = ((pWpCoeff->luma_offset[i] & 0x3FF)) |
             ((pWpCoeff->chroma_offset[i][0] & 0x3FF) << 10) |
             ((pWpCoeff->chroma_offset[i][1] & 0x3FF) << 20);

    pWP[1] = ((pWpCoeff->luma_delta_weight[i] & 0xFF)) |
             ((pWpCoeff->chroma_delta_weight[i][0] & 0xFF) << 8) |
             ((pWpCoeff->chroma_delta_weight[i][1] & 0xFF) << 16) |
             ((pSlice->pred_weight_table.luma_log2_weight_denom & 0x07) << 24) |
             ((pWpCoeff->luma_weight_flag[i] & 0x01) << 27) |
             ((pSlice->pred_weight_table.chroma_log2_weight_denom & 0x07) << 28) |
             ((pWpCoeff->chroma_weight_flag[i] & 0x01u) << 31);

    pWP += 2 * WP_ONE_SET_SIZE / 4;
  }
}

/*****************************************************************************/
static void AL_sBuildWPCoeff(AL_TPictMngrCtx const* pCtx, AL_TAvcSliceHdr const* pSlice, TBuffer* pWP)
{
  AL_VADDR pDataWP = pWP->tMD.pVirtualAddr + (pCtx->uNumSlice * WP_SLICE_SIZE);
  Rtos_Memset(pDataWP, 0, WP_SLICE_SIZE);

  // explicit weighted_pred case
  if((pSlice->slice_type == AL_SLICE_P && pSlice->pPPS->weighted_pred_flag) ||
     (pSlice->slice_type == AL_SLICE_B && pSlice->pPPS->weighted_bipred_idc == 1))
  {
    AL_AVC_sFillWPCoeff(pDataWP, pSlice, 0);

    if(pSlice->slice_type == AL_SLICE_B)
      AL_AVC_sFillWPCoeff(pDataWP, pSlice, 1);
  }
}

/*****************************************************************************/
static int32_t AL_sCalculatePOC(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  switch(pSlice->pSPS->pic_order_cnt_type)
  {
  case 0:
    AL_sGetPocType0(pCtx, pSlice);
    break;

  case 1:
    AL_sGetPocType1(pCtx, pSlice);
    break;

  case 2:
    AL_sGetPocType2(pCtx, pSlice);
    break;

  default:
    return 0xBAADF00D;
  }

  return (pCtx->iTopFieldOrderCnt < pCtx->iBotFieldOrderCnt) ? pCtx->iTopFieldOrderCnt :
         pCtx->iBotFieldOrderCnt;
}

/*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPOC(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  int32_t iCurPoc = AL_sCalculatePOC(pCtx, pSlice);

  pCtx->iCurFramePOC = iCurPoc;
}

/*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct)
{
  pCtx->ePicStruct = ePicStruct;
}

/*****************************************************************************/
void AL_AVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TAvcSps const* pSPS, AL_EPicStruct ePicStruct)
{
  AL_TCropInfo cropInfo = AL_AVC_GetCropInfo(pSPS);
  AL_PictMngr_UpdateDisplayBufferCrop(pCtx, pCtx->uRecID, cropInfo);
  AL_PictMngr_UpdateDisplayBufferPicStruct(pCtx, pCtx->uRecID, ePicStruct);
}

/***************************************************************************/
void AL_AVC_PictMngr_EndParsing(AL_TPictMngrCtx* pCtx, bool bClearRef, AL_EMarkingRef eMarkingFlag)
{
  AL_TDpb* pDpb = &pCtx->DPB;

  if(bClearRef)
    AL_PictMngr_Flush(pCtx);

  // increment present pictures latency count
  uint8_t uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList)
  {
    AL_Dpb_IncrementPicLatency(pDpb, uNode, pCtx->iCurFramePOC);
    uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
  }

  uint8_t uDelete = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->iCurFramePOC);
  bool bIsPOCAlreadyInDPB = uDelete != uEndOfList;

  if(bIsPOCAlreadyInDPB)
  {
    if(AL_Dpb_GetOutputFlag(pDpb, uDelete))
      AL_Dpb_Display(pDpb, uDelete);

    AL_Dpb_Remove(pDpb, uDelete);
  }

  AL_PictMngr_Insert(pCtx, pCtx->iCurFramePOC, pCtx->ePicStruct, 0, pCtx->uRecID, pCtx->uMvID, 1, eMarkingFlag, 0, 0, 0);
  AL_Dpb_ResetMMCO5(&pCtx->DPB);
}

/***************************************************************************/
void AL_AVC_PictMngr_CleanDPB(AL_TPictMngrCtx* pCtx)
{
  AL_Dpb_AVC_Cleanup(&pCtx->DPB);
}

/***************************************************************************/
bool AL_AVC_PictMngr_GetBuffers(AL_TPictMngrCtx const* pCtx, AL_TDecSliceParam const* pSP, AL_TAvcSliceHdr const* pSlice, TBufferListRef const* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, TBuffer* pWP, AL_TRecBuffers* pRecs)
{
  if(!AL_PictMngr_GetBuffers(pCtx, pSP, pListRef, pListVirtAddr, pListAddr, pPOC, pMV, pRecs))
    return false;

  // Build Weighted Pred Table
  AL_sBuildWPCoeff(pCtx, pSlice, pWP);

  return true;
}

/*****************************************************************************/
void AL_AVC_PictMngr_Fill_Gap_In_FrameNum(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  uint32_t uMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(
    (!AL_AVC_IsIDR(pSlice->nal_unit_type))
    && (pSlice->frame_num != pCtx->iPrevFrameNum)
    && (pSlice->frame_num != (int)((pCtx->iPrevFrameNum + 1) % uMaxFrameNum))
    )
  {
    if(pSlice->pSPS->gaps_in_frame_num_value_allowed_flag)
    {
      uint32_t UnusedShortTermFrameNum = (pCtx->iPrevFrameNum + 1) % uMaxFrameNum;
      uint32_t CurrFrameNum = pSlice->frame_num;

      while(CurrFrameNum != UnusedShortTermFrameNum)
      {
        AL_TAvcSliceHdr pUnusedSlice = *pSlice;

        pUnusedSlice.frame_num = UnusedShortTermFrameNum;
        pUnusedSlice.adaptive_ref_pic_marking_mode_flag = 0;
        int iFramePOC = AL_sCalculatePOC(pCtx, &pUnusedSlice);

        AL_PictMngr_Insert(pCtx, iFramePOC, AL_PS_FRM, 0, uEndOfList, uEndOfList, 0, SHORT_TERM_REF, 1, 0, 0);
        AL_Dpb_MarkingProcess(&pCtx->DPB, &pUnusedSlice, pCtx->iCurFramePOC);
        AL_Dpb_AVC_Cleanup(&pCtx->DPB);
        pCtx->iPrevFrameNum = UnusedShortTermFrameNum;
        UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % uMaxFrameNum;
      }
    }
  }
}

/*****************************************************************************/
void AL_AVC_PictMngr_InitPictList(AL_TPictMngrCtx const* pCtx, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  for(uint8_t uRef = 0; uRef < MAX_REF; ++uRef)
  {
    (*pListRef)[0][uRef].uNodeID = uEndOfList;
    (*pListRef)[1][uRef].uNodeID = uEndOfList;
  }

  if(pSlice->slice_type == AL_SLICE_P || pSlice->slice_type == AL_SLICE_SP)
    AL_Dpb_InitPSlice_RefList(&pCtx->DPB, pCtx->ePicStruct, &(*pListRef)[0][0]);
  else if(pSlice->slice_type == AL_SLICE_B)
    AL_Dpb_InitBSlice_RefList(&pCtx->DPB, pCtx->iCurFramePOC, pCtx->ePicStruct, pListRef);
}

/*****************************************************************************/
void AL_AVC_PictMngr_ReorderPictList(AL_TPictMngrCtx const* pCtx, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  int iPicNumPred = pSlice->frame_num * (1 + pSlice->field_pic_flag) + pSlice->field_pic_flag;

  if(pSlice->ref_pic_list_reordering_flag_l0)
  {
    uint8_t uParse = 0;
    uint8_t uRefIdxL0 = 0;
    uint8_t uParseShort = 0;
    uint8_t uParseLong = 0;

    while(pSlice->reordering_of_pic_nums_idc_l0[uParse] != 3)
    {
      int iPicNumIdc = pSlice->reordering_of_pic_nums_idc_l0[uParse++];
      switch(iPicNumIdc)
      {
      case 0:
      case 1:
        AL_Dpb_ModifShortTerm(&pCtx->DPB, pSlice, iPicNumIdc, uParseShort++, 0, &uRefIdxL0, &iPicNumPred, pListRef);
        break;
      case 2:
        AL_Dpb_ModifLongTerm(&pCtx->DPB, pSlice, uParseLong++, 0, &uRefIdxL0, pListRef);
        break;
      default:
        break;
      }
    }
  }

  if(pSlice->ref_pic_list_reordering_flag_l1 && pSlice->slice_type == AL_SLICE_B)
  {
    uint8_t uParse = 0;
    uint8_t uRefIdxL1 = 0;
    uint8_t uParseShort = 0;
    uint8_t uParseLong = 0;

    iPicNumPred = pSlice->frame_num * (1 + pSlice->field_pic_flag) + pSlice->field_pic_flag;

    while(pSlice->reordering_of_pic_nums_idc_l1[uParse] != 3)
    {
      int iPicNumIdc = pSlice->reordering_of_pic_nums_idc_l1[uParse++];
      switch(iPicNumIdc)
      {
      case 0:
      case 1:
        AL_Dpb_ModifShortTerm(&pCtx->DPB, pSlice, iPicNumIdc, uParseShort++, 1, &uRefIdxL1, &iPicNumPred, pListRef);
        break;
      case 2:
        AL_Dpb_ModifLongTerm(&pCtx->DPB, pSlice, uParseLong++, 1, &uRefIdxL1, pListRef);
        break;
      default:
        break;
      }
    }
  }
}

/*****************************************************************************/
int AL_AVC_PictMngr_GetNumExistingRef(AL_TPictMngrCtx const* pCtx, TBufferListRef const* pListRef)
{
  return AL_Dpb_GetNumExistingRef(&pCtx->DPB, pListRef);
}

/*@}*/

