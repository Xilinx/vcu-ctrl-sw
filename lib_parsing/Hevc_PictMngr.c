// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "Hevc_PictMngr.h"
#include "lib_common/HevcUtils.h"
#include "lib_common/Index.h"
#include "lib_common/Arithmetics.h"
#include "lib_parsing/HevcParser.h"

/*****************************************************************************/
void AL_HEVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCropInfo, AL_EPicStruct ePicStruct)
{
  AL_PictMngr_UpdateDisplayBufferCrop(pCtx, pCropInfo);
  AL_PictMngr_UpdateDisplayBufferPicStruct(pCtx, ePicStruct);
}

/*************************************************************************/
void AL_HEVC_PictMngr_ClearDPB(AL_TPictMngrCtx* pCtx, AL_THevcSps const* pSPS, bool bClearRef, bool bNoOutputPrior)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;

  // pre decoding output process
  if(bClearRef)
  {
    if(bNoOutputPrior)
      AL_Dpb_ClearOutput(pDpb);
    AL_PictMngr_Flush(pCtx);
  }

  AL_Dpb_HEVC_Cleanup(pDpb, pSPS->SpsMaxLatency, pSPS->sps_max_num_reorder_pics[pSPS->sps_max_sub_layers_minus1]);
  AL_TIndex tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  while(IS_NODE_VALID(tNodeID) && AL_Dpb_GetPicCount(pDpb) >= (pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1))
  {
    if(AL_Dpb_GetOutputFlag(pDpb, tNodeID))
      AL_Dpb_Display(pDpb, tNodeID);

    if(AL_Dpb_GetMarkingFlag(pDpb, tNodeID) == UNUSED_FOR_REF && (!AL_Dpb_GetOutputFlag(pDpb, tNodeID) || AL_HEVC_IsSLNR(pDpb->Nodes[tNodeID].eNUT)))
    {
      AL_TIndex tDeleteNodeID = tNodeID;
      tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);

      AL_Dpb_Remove(pDpb, tDeleteNodeID);
    }
    else
      tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
  }

  // Compute DPB fullness
  uint8_t max_dec_pict_buffering = pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] + 1;
  // The number of ref shall be less than max_dec_pict_buffering, but this condition is reverse here for concealment
  max_dec_pict_buffering = Max(max_dec_pict_buffering, AL_Dpb_GetRefCount(pDpb) + 1);
  // clip to Max supported Ref
  max_dec_pict_buffering = Min(max_dec_pict_buffering, AL_Dpb_GetNumRef(pDpb) + 1);

  // Remove Unused for reference if DBP is Full
  tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  while(IS_NODE_VALID(tNodeID) && AL_Dpb_GetPicCount(pDpb) >= max_dec_pict_buffering)
  {
    if(AL_Dpb_GetOutputFlag(pDpb, tNodeID))
      AL_Dpb_Display(pDpb, tNodeID);

    if(AL_Dpb_GetMarkingFlag(pDpb, tNodeID) == UNUSED_FOR_REF && pDpb->Nodes[tNodeID].iFramePOC < pDpb->iLastDisplayedPOC)
    {
      AL_TIndex tDeleteNodeID = tNodeID;
      tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);

      AL_Dpb_Remove(pDpb, tDeleteNodeID);
    }
    else
      tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
  }

  // Remove oldest POC if DBP is Full
  if(!IS_NODE_VALID(tNodeID) && AL_Dpb_GetPicCount(pDpb) >= max_dec_pict_buffering)
  {
    AL_TIndex tDeleteNodeID = AL_Dpb_GetHeadPOC(pDpb);
    AL_TIndex tCurNodeID = tDeleteNodeID;

    while(IS_NODE_VALID(tCurNodeID))
    {
      if(pDpb->Nodes[tCurNodeID].iFramePOC < pDpb->Nodes[tDeleteNodeID].iFramePOC)
        tDeleteNodeID = tCurNodeID;
      tCurNodeID = AL_Dpb_GetNextPOC(pDpb, tCurNodeID);
    }

    AL_Dpb_Remove(pDpb, tDeleteNodeID);
  }
}

/*************************************************************************/
static bool IsShortOrLongTermRef(AL_EMarkingRef eMarking)
{
  return (eMarking == SHORT_TERM_REF) || (eMarking == LONG_TERM_REF);
}

/*************************************************************************/
bool AL_HEVC_Dpb_HasPictInDPB(AL_TDpb const* pDpb)
{
  AL_TIndex tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  while(IS_NODE_VALID(tNodeID))
  {
    if(IsShortOrLongTermRef(AL_Dpb_GetMarkingFlag(pDpb, tNodeID)))
      return true;
    tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
  }

  return false;
}

/*************************************************************************/
void AL_HEVC_Dpb_RemoveHeadFrame(AL_TDpb* pDpb)
{
  if(AL_Dpb_GetPicCount(pDpb) >= AL_Dpb_GetNumRef(pDpb))
    AL_Dpb_RemoveHead(pDpb);
}

/*************************************************************************/
void AL_HEVC_PictMngr_EndFrame(AL_TPictMngrCtx* pCtx, uint32_t uPocLsb, AL_ENut eNUT, AL_THevcSliceHdr const* pSlice, bool bPicOutputFlag)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;

  AL_HEVC_Dpb_RemoveHeadFrame(pDpb);

  // post decoding output process
  AL_TIndex tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  if(bPicOutputFlag)
  {
    while(IS_NODE_VALID(tNodeID))
    {
      AL_Dpb_IncrementPicLatency(pDpb, tNodeID);
      tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
    }
  }

  AL_TDpbInsertParam tParam;
  tParam.iFramePOC = pDpb->iCurFramePOC;
  tParam.ePicStruct = AL_PS_FRM;
  tParam.iPocLsb = uPocLsb;
  tParam.bPicOutputFlag = bPicOutputFlag;
  tParam.eMarkingFlag = SHORT_TERM_REF;
  tParam.bNonExisting = 0;
  tParam.eNUT = eNUT;
  tParam.bSubpicFlag = 0;

  AL_PictMngr_Insert(pCtx, pCtx->tFrameID, pCtx->tAnnexID, &tParam);
  AL_Dpb_HEVC_Cleanup(pDpb, pSlice->pSPS->SpsMaxLatency, pSlice->pSPS->sps_max_num_reorder_pics[pSlice->pSPS->sps_max_sub_layers_minus1]);
}

/*****************************************************************************
   \brief Prepares the reference picture set for the current slice reference picture list construction
   \param[in]  pDpb       Pointer to a DPB context object
   \param[in]  pSlice     Pointer to the slice header of the current slice
*****************************************************************************/
void AL_HEVC_Dpb_InitRefPictSet(AL_TDpb* pDpb, AL_THevcSliceHdr const* pSlice)
{
  uint8_t CurrDeltaPocMsbPresentFlag[16] = { 0 };
  uint8_t FollDeltaPocMsbPresentFlag[16] = { 0 };

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
        pDpb->HevcRef.PocStCurrBefore[j++] = pDpb->iCurFramePOC + pSPS->DeltaPocS0[StRpsIdx][i];
      else
        pDpb->HevcRef.PocStFoll[k++] = pDpb->iCurFramePOC + pSPS->DeltaPocS0[StRpsIdx][i];
    }

    for(i = 0, j = 0; i < pSPS->NumPositivePics[StRpsIdx]; ++i)
    {
      if(pSPS->UsedByCurrPicS1[StRpsIdx][i])
        pDpb->HevcRef.PocStCurrAfter[j++] = pDpb->iCurFramePOC + pSPS->DeltaPocS1[StRpsIdx][i];
      else
        pDpb->HevcRef.PocStFoll[k++] = pDpb->iCurFramePOC + pSPS->DeltaPocS1[StRpsIdx][i];
    }

    // compute long term reference picture variables
    for(i = 0, j = 0, k = 0; i < pSlice->num_long_term_sps + pSlice->num_long_term_pics; ++i)
    {
      uint32_t uPocLt = pSlice->PocLsbLt[i];

      if(pSlice->delta_poc_msb_present_flag[i])
        uPocLt += pDpb->iCurFramePOC - (pSlice->DeltaPocMSBCycleLt[i] * pSPS->MaxPicOrderCntLsb) - pSlice->slice_pic_order_cnt_lsb;

      if(pSlice->UsedByCurrPicLt[i])
      {
        pDpb->HevcRef.PocLtCurr[j] = uPocLt;
        CurrDeltaPocMsbPresentFlag[j++] = pSlice->delta_poc_msb_present_flag[i];
      }
      else
      {
        pDpb->HevcRef.PocLtFoll[k] = uPocLt;
        FollDeltaPocMsbPresentFlag[k++] = pSlice->delta_poc_msb_present_flag[i];
      }
    }
  }

  // Compute long term reference pictures
  for(int32_t i = 0; i < pSlice->NumPocLtCurr; ++i)
  {
    AL_TIndex tNodeID;

    if(!CurrDeltaPocMsbPresentFlag[i])
      tNodeID = AL_Dpb_SearchPocLsb(pDpb, pDpb->HevcRef.PocLtCurr[i]);
    else
      tNodeID = AL_Dpb_SearchPOC(pDpb, pDpb->HevcRef.PocLtCurr[i]);
    pDpb->HevcRef.RefPicSetLtCurr[i] = tNodeID;
  }

  for(int32_t i = 0; i < pSlice->NumPocLtFoll; ++i)
  {
    AL_TIndex tNodeID;

    if(!FollDeltaPocMsbPresentFlag[i])
      tNodeID = AL_Dpb_SearchPocLsb(pDpb, pDpb->HevcRef.PocLtFoll[i]);
    else
      tNodeID = AL_Dpb_SearchPOC(pDpb, pDpb->HevcRef.PocLtFoll[i]);
    pDpb->HevcRef.RefPicSetLtFoll[i] = tNodeID;
  }

  // Compute short term reference pictures
  for(int32_t i = 0; i < pSlice->NumPocStCurrBefore; ++i)
    pDpb->HevcRef.RefPicSetStCurrBefore[i] = AL_Dpb_SearchPOC(pDpb, pDpb->HevcRef.PocStCurrBefore[i]);

  for(int32_t i = 0; i < pSlice->NumPocStCurrAfter; ++i)
    pDpb->HevcRef.RefPicSetStCurrAfter[i] = AL_Dpb_SearchPOC(pDpb, pDpb->HevcRef.PocStCurrAfter[i]);

  for(int32_t i = 0; i < pSlice->NumPocStFoll; ++i)
    pDpb->HevcRef.RefPicSetStFoll[i] = AL_Dpb_SearchPOC(pDpb, pDpb->HevcRef.PocStFoll[i]);

  int32_t iNumRefAfterUpdate = pSlice->NumPocLtCurr
                               + pSlice->NumPocLtFoll
                               + pSlice->NumPocStCurrBefore
                               + pSlice->NumPocStCurrAfter
                               + pSlice->NumPocStFoll;

  // Error Concealment : do not change anything if there is no reference after RPS update
  if(pSlice->slice_type != AL_SLICE_I && iNumRefAfterUpdate == 0)
    return;

  // reset picture marking on all the picture in the dbp
  AL_TIndex tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  while(IS_NODE_VALID(tNodeID))
  {
    AL_Dpb_SetMarkingFlag(pDpb, tNodeID, UNUSED_FOR_REF);
    tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
  }

  // mark long term reference pictures
  for(int32_t i = 0; i < pSlice->NumPocLtCurr; ++i)
  {
    tNodeID = pDpb->HevcRef.RefPicSetLtCurr[i];

    if(IS_NODE_VALID(tNodeID))
      AL_Dpb_SetMarkingFlag(pDpb, tNodeID, LONG_TERM_REF);
  }

  for(int32_t i = 0; i < pSlice->NumPocLtFoll; ++i)
  {
    tNodeID = pDpb->HevcRef.RefPicSetLtFoll[i];

    if(IS_NODE_VALID(tNodeID))
      AL_Dpb_SetMarkingFlag(pDpb, tNodeID, LONG_TERM_REF);
  }

  // mark short term reference pictures
  for(int32_t i = 0; i < pSlice->NumPocStCurrBefore; ++i)
  {
    tNodeID = pDpb->HevcRef.RefPicSetStCurrBefore[i];

    if(IS_NODE_VALID(tNodeID))
      AL_Dpb_SetMarkingFlag(pDpb, tNodeID, SHORT_TERM_REF);
  }

  for(int32_t i = 0; i < pSlice->NumPocStCurrAfter; ++i)
  {
    tNodeID = pDpb->HevcRef.RefPicSetStCurrAfter[i];

    if(IS_NODE_VALID(tNodeID))
      AL_Dpb_SetMarkingFlag(pDpb, tNodeID, SHORT_TERM_REF);
  }

  for(int32_t i = 0; i < pSlice->NumPocStFoll; ++i)
  {
    tNodeID = pDpb->HevcRef.RefPicSetStFoll[i];

    if(IS_NODE_VALID(tNodeID))
      AL_Dpb_SetMarkingFlag(pDpb, tNodeID, SHORT_TERM_REF);
  }
}

/*****************************************************************************
   \brief Builds the reference picture list of the current slice
   \param[in]  pDpb     Pointer to a Dpb context object
   \param[in]  pSlice   Pointer to the slice header of the current slice
   \param[out] pListRef Pointer to the current reference list
*****************************************************************************/
bool AL_HEVC_Dpb_BuildPictureList(AL_TDpb* pDpb, AL_THevcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  uint8_t uRef;
  uint8_t pNumRef[2] =
  {
    0, 0
  };
  uint8_t NumPicTotalCurr = pSlice->NumPicTotalCurr;

  // reset reference picture list
  for(uRef = 0; uRef < AL_MAX_REF; ++uRef)
  {
    (*pListRef)[0][uRef].tNodeID = AL_BAD_INDEX;
    (*pListRef)[1][uRef].tNodeID = AL_BAD_INDEX;
  }

  if(pSlice->slice_type != AL_SLICE_I)
  {
    AL_TIndex tNodeList[16] = { 0 };
    uint8_t NumRpsCurrTempList = (NumPicTotalCurr > pSlice->num_ref_idx_l0_active_minus1 + 1) ? NumPicTotalCurr : pSlice->num_ref_idx_l0_active_minus1 + 1;
    // slice P
    uRef = 0;

    if(pSlice->NumPocStCurrBefore || pSlice->NumPocStCurrAfter || pSlice->NumPocLtCurr
       )
    {
      while(uRef < NumRpsCurrTempList)
      {
        for(uint8_t i = 0; i < pSlice->NumPocStCurrBefore && uRef < NumRpsCurrTempList; ++uRef, ++i)
          tNodeList[uRef] = pDpb->HevcRef.RefPicSetStCurrBefore[i];

        for(uint8_t i = 0; i < pSlice->NumPocStCurrAfter && uRef < NumRpsCurrTempList; ++uRef, ++i)
          tNodeList[uRef] = pDpb->HevcRef.RefPicSetStCurrAfter[i];

        for(uint8_t i = 0; i < pSlice->NumPocLtCurr && uRef < NumRpsCurrTempList; ++uRef, ++i)
          tNodeList[uRef] = pDpb->HevcRef.RefPicSetLtCurr[i];

      }

      for(uRef = 0; uRef <= pSlice->num_ref_idx_l0_active_minus1; ++uRef)
      {
        AL_TIndex tNodeID = pSlice->ref_pic_modif.ref_pic_list_modification_flag_l0 ? tNodeList[pSlice->ref_pic_modif.list_entry_l0[uRef]] :
                            tNodeList[uRef];

        if((!IS_NODE_VALID(tNodeID)) || (!AL_IS_VALID_INDEX(pDpb->Nodes[tNodeID].tFrameID)))
          tNodeID = AL_Dpb_GetHeadPOC(pDpb);

        if((!IS_NODE_VALID(tNodeID)) || (!AL_IS_VALID_INDEX(pDpb->Nodes[tNodeID].tFrameID)))
          return false;

        (*pListRef)[0][uRef].tNodeID = tNodeID;
      }
    }

    // slice B
    if(pSlice->slice_type == AL_SLICE_B)
    {
      NumRpsCurrTempList = (NumPicTotalCurr > pSlice->num_ref_idx_l1_active_minus1 + 1) ? NumPicTotalCurr : pSlice->num_ref_idx_l1_active_minus1 + 1;
      uRef = 0;

      if(pSlice->NumPocStCurrAfter || pSlice->NumPocStCurrBefore || pSlice->NumPocLtCurr
         )
      {
        while(uRef < NumRpsCurrTempList)
        {
          for(uint8_t i = 0; i < pSlice->NumPocStCurrAfter && uRef < NumRpsCurrTempList; ++uRef, ++i)
            tNodeList[uRef] = pDpb->HevcRef.RefPicSetStCurrAfter[i];

          for(uint8_t i = 0; i < pSlice->NumPocStCurrBefore && uRef < NumRpsCurrTempList; ++uRef, ++i)
            tNodeList[uRef] = pDpb->HevcRef.RefPicSetStCurrBefore[i];

          for(uint8_t i = 0; i < pSlice->NumPocLtCurr && uRef < NumRpsCurrTempList; ++uRef, ++i)
            tNodeList[uRef] = pDpb->HevcRef.RefPicSetLtCurr[i];

        }

        for(uRef = 0; uRef <= pSlice->num_ref_idx_l1_active_minus1; ++uRef)
        {
          AL_TIndex tNodeID = pSlice->ref_pic_modif.ref_pic_list_modification_flag_l1 ? tNodeList[pSlice->ref_pic_modif.list_entry_l1[uRef]] :
                              tNodeList[uRef];

          if((!IS_NODE_VALID(tNodeID)) || (!AL_IS_VALID_INDEX(pDpb->Nodes[tNodeID].tFrameID)))
            tNodeID = AL_Dpb_GetHeadPOC(pDpb);

          if((!IS_NODE_VALID(tNodeID)) || (!AL_IS_VALID_INDEX(pDpb->Nodes[tNodeID].tFrameID)))
            return false;

          (*pListRef)[1][uRef].tNodeID = tNodeID;
        }
      }
    }
  }

  for(uint8_t i = 0; i < 16; ++i)
  {
    if(IS_NODE_VALID((*pListRef)[0][i].tNodeID))
      pNumRef[0]++;

    if(IS_NODE_VALID((*pListRef)[1][i].tNodeID))
      pNumRef[1]++;
  }

  if((pSlice->slice_type != AL_SLICE_I && pNumRef[0] < pSlice->num_ref_idx_l0_active_minus1 + 1) ||
     (pSlice->slice_type == AL_SLICE_B && pNumRef[1] < pSlice->num_ref_idx_l1_active_minus1 + 1))
    return false;

  return true;
}

/*!@}*/
