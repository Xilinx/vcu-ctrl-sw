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

#include "Hevc_PictMngr.h"
#include "lib_common/HevcUtils.h"
#include "lib_parsing/HevcParser.h"

/*****************************************************************************/
static void AL_HEVC_sFillWPCoeff(AL_VADDR pDataWP, AL_THevcSliceHdr const* pSlice, uint8_t uL0L1)
{
  uint8_t uNumRefIdx = (uL0L1 ? pSlice->num_ref_idx_l1_active_minus1 : pSlice->num_ref_idx_l0_active_minus1) + 1;
  uint32_t* pWP = (uint32_t*)(pDataWP + uL0L1 * WP_ONE_SET_SIZE);

  AL_TWPCoeff const* pWpCoeff = &pSlice->pred_weight_table.tWpCoeff[uL0L1];

  for(uint8_t i = 0; i < uNumRefIdx; ++i)
  {
    pWP[0] = ((pWpCoeff->luma_offset[i] & 0x3FF)) |
             ((pWpCoeff->chroma_offset[i][0] & 0x3FF) << 10) |
             ((pWpCoeff->chroma_offset[i][1] & 0x3FF) << 20);

    pWP[1] = ((pWpCoeff->luma_delta_weight[i] & 0xFF)) |
             ((pWpCoeff->chroma_delta_weight[i][0] & 0xFF) << 8) |
             ((pWpCoeff->chroma_delta_weight[i][1] & 0xFF) << 16) |
             ((pSlice->pred_weight_table.luma_log2_weight_denom & 0x0F) << 24) |
             ((pSlice->pred_weight_table.chroma_log2_weight_denom & 0x0F) << 28);
    pWP += 2 * WP_ONE_SET_SIZE / 4;
  }
}

/*************************************************************************//*!
   \brief this function writes in the motion vector buffer the weighted pred coefficient
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Pointer to the slice header of the current slice
   \param[out] pWP      Pointer to the weighted pred tables buffer
*****************************************************************************/
static void AL_HEVC_sBuildWPCoeff(AL_TPictMngrCtx const* pCtx, AL_THevcSliceHdr const* pSlice, TBuffer* pWP)
{
  AL_VADDR pDataWP = pWP->tMD.pVirtualAddr + (pCtx->uNumSlice * WP_SLICE_SIZE);

  // weighted pred case
  if((pSlice->pPPS->weighted_bipred_flag && pSlice->slice_type == AL_SLICE_B) ||
     (pSlice->pPPS->weighted_pred_flag && pSlice->slice_type == AL_SLICE_P))
  {
    Rtos_Memset(pDataWP, 0, WP_SLICE_SIZE);

    AL_HEVC_sFillWPCoeff(pDataWP, pSlice, 0);

    if(pSlice->slice_type == AL_SLICE_B)
      AL_HEVC_sFillWPCoeff(pDataWP, pSlice, 1);
  }
}

/***************************************************************************/
/*      H E V C   P i c t u r e    M a n a g e r   f u n c t i o n s       */
/***************************************************************************/

/*****************************************************************************/
void AL_HEVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_THevcSps const* pSPS, AL_EPicStruct ePicStruct)
{
  AL_TCropInfo cropInfo = AL_HEVC_GetCropInfo(pSPS);
  AL_PictMngr_UpdateDisplayBufferCrop(pCtx, pCtx->uRecID, cropInfo);
  AL_PictMngr_UpdateDisplayBufferPicStruct(pCtx, pCtx->uRecID, ePicStruct);
}

/*****************************************************************************/
bool AL_HEVC_PictMngr_GetBuffers(AL_TPictMngrCtx const* pCtx, AL_TDecSliceParam const* pSP, AL_THevcSliceHdr const* pSlice, TBufferListRef const* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, TBuffer* pWP, AL_TRecBuffers* pRecs)
{
  if(!AL_PictMngr_GetBuffers(pCtx, pSP, pListRef, pListVirtAddr, pListAddr, pPOC, pMV, pRecs))
    return false;

  // Build Weighted Pred Table
  AL_HEVC_sBuildWPCoeff(pCtx, pSlice, pWP);

  return true;
}

/*************************************************************************/
void AL_HEVC_PictMngr_ClearDPB(AL_TPictMngrCtx* pCtx, AL_THevcSps const* pSPS, bool bClearRef, bool bNoOutputPrior)
{
  AL_TDpb* pDpb = &pCtx->DPB;

  // pre decoding output process
  if(bClearRef)
  {
    if(bNoOutputPrior)
      AL_Dpb_ClearOutput(pDpb);
    AL_PictMngr_Flush(pCtx);
  }

  AL_Dpb_HEVC_Cleanup(pDpb, pSPS->SpsMaxLatency, pSPS->sps_max_num_reorder_pics[pSPS->sps_max_sub_layers_minus1]);
  uint8_t uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList && AL_Dpb_GetPicCount(pDpb) >= (pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1))
  {
    if(AL_Dpb_GetOutputFlag(pDpb, uNode))
      AL_Dpb_Display(&pCtx->DPB, uNode);

    if(AL_Dpb_GetMarkingFlag(pDpb, uNode) == UNUSED_FOR_REF && (!AL_Dpb_GetOutputFlag(pDpb, uNode) || AL_HEVC_IsSLNR(pDpb->Nodes[uNode].eNUT)))
    {
      uint8_t uDelete = uNode;
      uNode = AL_Dpb_GetNextPOC(pDpb, uNode);

      AL_Dpb_Remove(pDpb, uDelete);
    }
    else
      uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
  }

  // Remove Unused for reference if DBP is Full
  uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList && AL_Dpb_GetPicCount(pDpb) >= (pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1))
  {
    if(AL_Dpb_GetOutputFlag(pDpb, uNode))
      AL_Dpb_Display(&pCtx->DPB, uNode);

    if(AL_Dpb_GetMarkingFlag(pDpb, uNode) == UNUSED_FOR_REF && pDpb->Nodes[uNode].iFramePOC < pDpb->iLastDisplayedPOC)
    {
      uint8_t uDelete = uNode;
      uNode = AL_Dpb_GetNextPOC(pDpb, uNode);

      AL_Dpb_Remove(pDpb, uDelete);
    }
    else
      uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
  }

  // Remove oldest POC if DBP is Full
  if(uNode == uEndOfList && AL_Dpb_GetPicCount(pDpb) >= (pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1))
  {
    uint8_t uDelete = AL_Dpb_GetHeadPOC(pDpb);
    uint8_t uCurNode = uDelete;

    while(uCurNode != uEndOfList)
    {
      if(pDpb->Nodes[uCurNode].iFramePOC < pDpb->Nodes[uDelete].iFramePOC)
        uDelete = uCurNode;
      uCurNode = AL_Dpb_GetNextPOC(pDpb, uCurNode);
    }

    AL_Dpb_Remove(pDpb, uDelete);
  }
}

/*************************************************************************/
static bool IsShortOrLongTermRef(AL_EMarkingRef eMarking)
{
  return (eMarking == SHORT_TERM_REF) || (eMarking == LONG_TERM_REF);
}

/*************************************************************************/
bool AL_HEVC_PictMngr_HasPictInDPB(AL_TPictMngrCtx const* pCtx)
{
  AL_TDpb const* pDpb = &pCtx->DPB;
  uint8_t uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList)
  {
    if(IsShortOrLongTermRef(AL_Dpb_GetMarkingFlag(pDpb, uNode)))
      return true;
    uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
  }

  return false;
}

/*************************************************************************/
void AL_HEVC_PictMngr_RemoveHeadFrame(AL_TPictMngrCtx* pCtx)
{
  AL_TDpb* pDpb = &pCtx->DPB;

  if(AL_Dpb_GetPicCount(pDpb) >= AL_Dpb_GetNumRef(pDpb))
    AL_Dpb_RemoveHead(pDpb);
}

/*************************************************************************/
void AL_HEVC_PictMngr_EndFrame(AL_TPictMngrCtx* pCtx, uint32_t uPocLsb, AL_ENut eNUT, AL_THevcSliceHdr const* pSlice, uint8_t pic_output_flag)
{
  AL_TDpb* pDpb = &pCtx->DPB;

  AL_HEVC_PictMngr_RemoveHeadFrame(pCtx);

  // post decoding output process
  uint8_t uNode = AL_Dpb_GetHeadPOC(pDpb);

  if(pic_output_flag)
  {
    while(uNode != uEndOfList)
    {
      AL_Dpb_IncrementPicLatency(pDpb, uNode, pCtx->iCurFramePOC);
      uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
    }
  }

  AL_PictMngr_Insert(pCtx, pCtx->iCurFramePOC, AL_PS_FRM, uPocLsb, pCtx->uRecID, pCtx->uMvID, pic_output_flag, SHORT_TERM_REF, 0, eNUT, 0);
  AL_Dpb_HEVC_Cleanup(pDpb, pSlice->pSPS->SpsMaxLatency, pSlice->pSPS->sps_max_num_reorder_pics[pSlice->pSPS->sps_max_sub_layers_minus1]);
}

/*************************************************************************//*!
   \brief Prepares the reference picture set for the current slice reference picture list construction
   \param[in]  pCtx       Pointer to a Picture manager context object
   \param[in]  pSlice     Pointer to the slice header of the current slice
*****************************************************************************/
void AL_HEVC_PictMngr_InitRefPictSet(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr const* pSlice)
{
  uint8_t CurrDeltaPocMsbPresentFlag[16] = { 0 };
  uint8_t FollDeltaPocMsbPresentFlag[16] = { 0 };
  AL_TDpb* pDpb = &pCtx->DPB;

  // Fill the five lists of picture order count values
  if(!AL_HEVC_IsIDR(pSlice->nal_unit_type))
  {
    uint8_t i, j, k;
    AL_THevcSps* pSPS = pSlice->pSPS;
    uint8_t StRpsIdx = pSlice->short_term_ref_pic_set_sps_flag ? pSlice->short_term_ref_pic_set_idx :
                       pSPS->num_short_term_ref_pic_sets;

    // compute short term reference picture variables
    for(i = 0, j = 0, k = 0; i < pSPS->NumNegativePics[StRpsIdx]; ++i)
    {
      if(pSPS->UsedByCurrPicS0[StRpsIdx][i])
        pCtx->HevcRef.PocStCurrBefore[j++] = pCtx->iCurFramePOC + pSPS->DeltaPocS0[StRpsIdx][i];
      else
        pCtx->HevcRef.PocStFoll[k++] = pCtx->iCurFramePOC + pSPS->DeltaPocS0[StRpsIdx][i];
    }

    for(i = 0, j = 0; i < pSPS->NumPositivePics[StRpsIdx]; ++i)
    {
      if(pSPS->UsedByCurrPicS1[StRpsIdx][i])
        pCtx->HevcRef.PocStCurrAfter[j++] = pCtx->iCurFramePOC + pSPS->DeltaPocS1[StRpsIdx][i];
      else
        pCtx->HevcRef.PocStFoll[k++] = pCtx->iCurFramePOC + pSPS->DeltaPocS1[StRpsIdx][i];
    }

    // compute long term reference picture variables
    for(i = 0, j = 0, k = 0; i < pSlice->num_long_term_sps + pSlice->num_long_term_pics; ++i)
    {
      uint32_t uPocLt = pSlice->PocLsbLt[i];

      if(pSlice->delta_poc_msb_present_flag[i])
        uPocLt += pCtx->iCurFramePOC - (pSlice->DeltaPocMSBCycleLt[i] * pSPS->MaxPicOrderCntLsb) - pSlice->slice_pic_order_cnt_lsb;

      if(pSlice->UsedByCurrPicLt[i])
      {
        pCtx->HevcRef.PocLtCurr[j] = uPocLt;
        CurrDeltaPocMsbPresentFlag[j++] = pSlice->delta_poc_msb_present_flag[i];
      }
      else
      {
        pCtx->HevcRef.PocLtFoll[k] = uPocLt;
        FollDeltaPocMsbPresentFlag[k++] = pSlice->delta_poc_msb_present_flag[i];
      }
    }
  }

  // Compute long term reference pictures
  for(int i = 0; i < pSlice->NumPocLtCurr; ++i)
  {
    uint8_t uPos;

    if(!CurrDeltaPocMsbPresentFlag[i])
      uPos = AL_Dpb_SearchPocLsb(&pCtx->DPB, pCtx->HevcRef.PocLtCurr[i]);
    else
      uPos = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->HevcRef.PocLtCurr[i]);
    pCtx->HevcRef.RefPicSetLtCurr[i] = uPos;
  }

  for(int i = 0; i < pSlice->NumPocLtFoll; ++i)
  {
    uint8_t uPos;

    if(!FollDeltaPocMsbPresentFlag[i])
      uPos = AL_Dpb_SearchPocLsb(&pCtx->DPB, pCtx->HevcRef.PocLtFoll[i]);
    else
      uPos = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->HevcRef.PocLtFoll[i]);
    pCtx->HevcRef.RefPicSetLtFoll[i] = uPos;
  }

  // Compute short term reference pictures
  for(int i = 0; i < pSlice->NumPocStCurrBefore; ++i)
    pCtx->HevcRef.RefPicSetStCurrBefore[i] = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->HevcRef.PocStCurrBefore[i]);

  for(int i = 0; i < pSlice->NumPocStCurrAfter; ++i)
    pCtx->HevcRef.RefPicSetStCurrAfter[i] = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->HevcRef.PocStCurrAfter[i]);

  for(int i = 0; i < pSlice->NumPocStFoll; ++i)
    pCtx->HevcRef.RefPicSetStFoll[i] = AL_Dpb_SearchPOC(&pCtx->DPB, pCtx->HevcRef.PocStFoll[i]);

  int iNumRefAfterUpdate = pSlice->NumPocLtCurr
                           + pSlice->NumPocLtFoll
                           + pSlice->NumPocStCurrBefore
                           + pSlice->NumPocStCurrAfter
                           + pSlice->NumPocStFoll;

  // Error Concealment : do not change anything if there is no reference after RPS update
  if(pSlice->slice_type != AL_SLICE_I && iNumRefAfterUpdate == 0)
    return;

  // reset picture marking on all the picture in the dbp
  uint8_t uNode = AL_Dpb_GetHeadPOC(&pCtx->DPB);

  while(uNode != uEndOfList)
  {
    AL_Dpb_SetMarkingFlag(pDpb, uNode, UNUSED_FOR_REF);
    uNode = AL_Dpb_GetNextPOC(pDpb, uNode);
  }

  // mark long term reference pictures
  for(int i = 0; i < pSlice->NumPocLtCurr; ++i)
  {
    uNode = pCtx->HevcRef.RefPicSetLtCurr[i];

    if(uNode != uEndOfList)
      AL_Dpb_SetMarkingFlag(pDpb, uNode, LONG_TERM_REF);
  }

  for(int i = 0; i < pSlice->NumPocLtFoll; ++i)
  {
    uNode = pCtx->HevcRef.RefPicSetLtFoll[i];

    if(uNode != uEndOfList)
      AL_Dpb_SetMarkingFlag(pDpb, uNode, LONG_TERM_REF);
  }

  // mark short term reference pictures
  for(int i = 0; i < pSlice->NumPocStCurrBefore; ++i)
  {
    uNode = pCtx->HevcRef.RefPicSetStCurrBefore[i];

    if(uNode != uEndOfList)
      AL_Dpb_SetMarkingFlag(pDpb, uNode, SHORT_TERM_REF);
  }

  for(int i = 0; i < pSlice->NumPocStCurrAfter; ++i)
  {
    uNode = pCtx->HevcRef.RefPicSetStCurrAfter[i];

    if(uNode != uEndOfList)
      AL_Dpb_SetMarkingFlag(pDpb, uNode, SHORT_TERM_REF);
  }

  for(int i = 0; i < pSlice->NumPocStFoll; ++i)
  {
    uNode = pCtx->HevcRef.RefPicSetStFoll[i];

    if(uNode != uEndOfList)
      AL_Dpb_SetMarkingFlag(pDpb, uNode, SHORT_TERM_REF);
  }
}

/*************************************************************************//*!
   \brief Builds the reference picture list of the current slice
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Pointer to the slice header of the current slice
   \param[out] pListRef Pointer to the current reference list
*****************************************************************************/
bool AL_HEVC_PictMngr_BuildPictureList(AL_TPictMngrCtx const* pCtx, AL_THevcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  uint8_t uRef;
  uint8_t pNumRef[2] =
  {
    0, 0
  };
  uint8_t NumPocTotalCurr = pSlice->NumPocTotalCurr;

  // reset reference picture list
  for(uRef = 0; uRef < MAX_REF; ++uRef)
  {
    (*pListRef)[0][uRef].uNodeID = uEndOfList;
    (*pListRef)[1][uRef].uNodeID = uEndOfList;
  }

  if(pSlice->slice_type != AL_SLICE_I)
  {
    uint8_t uNodeList[16];
    uint8_t NumRpsCurrTempList = (NumPocTotalCurr > pSlice->num_ref_idx_l0_active_minus1 + 1) ? NumPocTotalCurr : pSlice->num_ref_idx_l0_active_minus1 + 1;
    // slice P
    uRef = 0;

    if(pSlice->NumPocStCurrBefore || pSlice->NumPocStCurrAfter || pSlice->NumPocLtCurr)
    {
      while(uRef < NumRpsCurrTempList)
      {
        for(uint8_t i = 0; i < pSlice->NumPocStCurrBefore && uRef < NumRpsCurrTempList; ++uRef, ++i)
          uNodeList[uRef] = pCtx->HevcRef.RefPicSetStCurrBefore[i];

        for(uint8_t i = 0; i < pSlice->NumPocStCurrAfter && uRef < NumRpsCurrTempList; ++uRef, ++i)
          uNodeList[uRef] = pCtx->HevcRef.RefPicSetStCurrAfter[i];

        for(uint8_t i = 0; i < pSlice->NumPocLtCurr && uRef < NumRpsCurrTempList; ++uRef, ++i)
          uNodeList[uRef] = pCtx->HevcRef.RefPicSetLtCurr[i];
      }

      for(uRef = 0; uRef <= pSlice->num_ref_idx_l0_active_minus1; ++uRef)
      {
        uint8_t uNode = pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 ? uNodeList[pSlice->ref_pic_modif.list_entry_l0[uRef]] :
                        uNodeList[uRef];
        uNode = (uNode == uEndOfList) ? AL_Dpb_GetHeadPOC(&pCtx->DPB) : uNode;

        (*pListRef)[0][uRef].uNodeID = uNode;
        (*pListRef)[0][uRef].RefBuf = *(AL_PictMngr_GetRecBufferFromID(pCtx, pCtx->DPB.Nodes[uNode].uFrmID));
      }
    }

    // slice B
    if(pSlice->slice_type == AL_SLICE_B)
    {
      NumRpsCurrTempList = (NumPocTotalCurr > pSlice->num_ref_idx_l1_active_minus1 + 1) ? NumPocTotalCurr : pSlice->num_ref_idx_l1_active_minus1 + 1;
      uRef = 0;

      if(pSlice->NumPocStCurrAfter || pSlice->NumPocStCurrBefore || pSlice->NumPocLtCurr)
      {
        while(uRef < NumRpsCurrTempList)
        {
          for(uint8_t i = 0; i < pSlice->NumPocStCurrAfter && uRef < NumRpsCurrTempList; ++uRef, ++i)
            uNodeList[uRef] = pCtx->HevcRef.RefPicSetStCurrAfter[i];

          for(uint8_t i = 0; i < pSlice->NumPocStCurrBefore && uRef < NumRpsCurrTempList; ++uRef, ++i)
            uNodeList[uRef] = pCtx->HevcRef.RefPicSetStCurrBefore[i];

          for(uint8_t i = 0; i < pSlice->NumPocLtCurr && uRef < NumRpsCurrTempList; ++uRef, ++i)
            uNodeList[uRef] = pCtx->HevcRef.RefPicSetLtCurr[i];
        }

        for(uRef = 0; uRef <= pSlice->num_ref_idx_l1_active_minus1; ++uRef)
        {
          uint8_t uNode = pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1 ? uNodeList[pSlice->ref_pic_modif.list_entry_l1[uRef]] :
                          uNodeList[uRef];
          uNode = (uNode == uEndOfList) ? AL_Dpb_GetHeadPOC(&pCtx->DPB) : uNode;

          (*pListRef)[1][uRef].uNodeID = uNode;
          (*pListRef)[1][uRef].RefBuf = *(AL_PictMngr_GetRecBufferFromID(pCtx, pCtx->DPB.Nodes[uNode].uFrmID));
        }
      }
    }
  }

  for(uint8_t i = 0; i < 16; ++i)
  {
    if((*pListRef)[0][i].uNodeID != uEndOfList)
      pNumRef[0]++;

    if((*pListRef)[1][i].uNodeID != uEndOfList)
      pNumRef[1]++;
  }

  if((pSlice->slice_type != AL_SLICE_I && pNumRef[0] < pSlice->num_ref_idx_l0_active_minus1 + 1) ||
     (pSlice->slice_type == AL_SLICE_B && pNumRef[1] < pSlice->num_ref_idx_l1_active_minus1 + 1))
    return false;

  return true;
}

/*@}*/

