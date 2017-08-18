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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include "DPB.h"
#include "assert.h"

#define DPB_GET_MUTEX(pDpb) Rtos_GetMutex(pDpb->m_Mutex)
#define DPB_RELEASE_MUTEX(pDpb) Rtos_ReleaseMutex(pDpb->m_Mutex)

/*****************************************************************************/
static void AL_DispFifo_sInit(AL_TDispFifo* pFifo)
{
  int i;

  for(i = 0; i < FRM_BUF_POOL_SIZE; ++i)
  {
    pFifo->pFrmIDs[i] = UndefID;
    pFifo->pFrmStatus[i] = AL_NOT_NEEDED_FOR_OUTPUT;
  }

  pFifo->uFirstFrm = 0;
  pFifo->uNumFrm = 0;
}

/*************************************************************************/
static void AL_DispFifo_sDeinit(AL_TDispFifo* pFifo)
{
  (void)pFifo;
}

/*****************************************************************************/
static void AL_Dpb_sResetNodeInfo(AL_TDpb* pDpb, AL_TDpbNode* pNode)
{
  if(pNode->uNodeID == pDpb->m_uNodeWaiting)
  {
    pDpb->m_uNodeWaiting = uEndOfList;
    pDpb->m_uFrmWaiting = UndefID;
    pDpb->m_uMvWaiting = UndefID;
    pDpb->m_bPicWaiting = false;
  }

  pNode->iFramePOC = 0xFFFFFFFF;
  pNode->slice_pic_order_cnt_lsb = 0xFFFFFFFF;
  pNode->uNodeID = uEndOfList;
  pNode->uPrevPOC = uEndOfList;
  pNode->uNextPOC = uEndOfList;
  pNode->uPrevPocLsb = uEndOfList;
  pNode->uNextPocLsb = uEndOfList;
  pNode->uPrevDecOrder = uEndOfList;
  pNode->uNextDecOrder = uEndOfList;
  pNode->uFrmID = UndefID;
  pNode->uMvID = UndefID;
  pNode->eMarking_flag = UNUSED_FOR_REF;
  pNode->bIsReset = true;
  pNode->pic_output_flag = 0;
  pNode->bIsDisplayed = false;
  pNode->uPicLatency = 0;

  pNode->iPic_num = 0x7FFF;
  pNode->iFrame_num_wrap = 0x7FFF;
  pNode->iLong_term_pic_num = 0x7FFF;
  pNode->iLong_term_frame_idx = 0x7FFF;
  pNode->iSlice_frame_num = 0x7FFF;
  pNode->non_existing = 0;
  pNode->eNUT = AL_HEVC_NUT_ERR;
}

/*****************************************************************************/
static void AL_Dpb_sReleasePicID(AL_TDpb* pDpb, uint8_t uPicID)
{
  if(uPicID != UndefID)
  {
    pDpb->m_PicId2NodeId[uPicID] = UndefID;
    pDpb->m_FreePicIDs[pDpb->m_FreePicIdCnt++] = uPicID;
  }
}

/*****************************************************************************/
static void AL_Dpb_sAddToDisplayList(AL_TDpb* pDpb, uint8_t uNode)
{
  uint8_t uID;

  uID = pDpb->m_DispFifo.uFirstFrm + pDpb->m_DispFifo.uNumFrm++;

  pDpb->m_DispFifo.pFrmIDs[uID % FRM_BUF_POOL_SIZE] = pDpb->m_Nodes[uNode].uFrmID;
  pDpb->m_DispFifo.pPicLatency[pDpb->m_Nodes[uNode].uFrmID] = pDpb->m_Nodes[uNode].uPicLatency;

  pDpb->m_pfnOutputFrmBuf(pDpb->m_pPfnCtx, pDpb->m_Nodes[uNode].uFrmID);
}

/*****************************************************************************/
static void AL_Dpb_sSliding_window_marking(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint32_t uNumShortTerm = 0;
  uint32_t uNumLongTerm = 0;
  int16_t iMinFrameNumWrap = 0x7FFF;

  uint8_t uPic = pDpb->m_uHeadDecOrder;
  uint8_t uPosMin = 0;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].eMarking_flag == SHORT_TERM_REF)
      ++uNumShortTerm;
    else if(pNodes[uPic].eMarking_flag == LONG_TERM_REF)
      ++uNumLongTerm;

    if(pNodes[uPic].iFrame_num_wrap < iMinFrameNumWrap && pNodes[uPic].eMarking_flag == SHORT_TERM_REF)
    {
      iMinFrameNumWrap = pNodes[uPic].iFrame_num_wrap;
      uPosMin = uPic;
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }

  if((uNumShortTerm + uNumLongTerm) > pSlice->m_pSPS->max_num_ref_frames)
  {
    pNodes[uPosMin].eMarking_flag = UNUSED_FOR_REF;
    --pDpb->m_uCountRef;

    if(pNodes[uPosMin].non_existing)
      AL_Dpb_Remove(pDpb, uPosMin);
    else
    {
      AL_Dpb_sReleasePicID(pDpb, pDpb->m_Nodes[uPosMin].uPicID);
      pDpb->m_Nodes[uPosMin].uPicID = UndefID;
    }
  }
}

/*****************************************************************************/
static void AL_Dpb_sSetPicToUnused(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  pNodes[uNode].eMarking_flag = UNUSED_FOR_REF;
  --pDpb->m_uCountRef;
  AL_Dpb_sReleasePicID(pDpb, pNodes[uNode].uPicID);
  pNodes[uNode].uPicID = UndefID;
}

/*****************************************************************************/
static void AL_Dpb_sShortTermToUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uCurPos = pDpb->m_uCurRef;

  int16_t iPicNumX = pNodes[uCurPos].iPic_num - (pSlice->difference_of_pic_nums_minus1[iIdx] + 1);
  uint8_t uPic = pDpb->m_uHeadDecOrder;

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iPic_num == iPicNumX)
    {
      if(pNodes[uPic].eMarking_flag == SHORT_TERM_REF)
      {
        AL_Dpb_sSetPicToUnused(pDpb, uPic);
        break;
      }
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }
}

/*****************************************************************************/
/*8.2.5.4.2*/
static void AL_Dpb_sLongTermToUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  uint8_t uPic = pDpb->m_uHeadDecOrder;
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  int16_t iLong_pic_num = pSlice->long_term_pic_num[iIdx];

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iLong_term_pic_num == iLong_pic_num)
    {
      if(pNodes[uPic].eMarking_flag == LONG_TERM_REF)
      {
        AL_Dpb_sSetPicToUnused(pDpb, uPic);
      }
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }
}

/*****************************************************************************/
static void AL_Dpb_sSwitchLongTermPlace(AL_TDpb* pDpb, uint8_t iRef)
{
  uint8_t uNode = pDpb->m_uHeadDecOrder;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uPrev = pNodes[iRef].uPrevDecOrder;
  uint8_t uNext = pNodes[iRef].uNextDecOrder;

  while(uNode != uEndOfList)
  {
    if(pNodes[uNode].eMarking_flag == LONG_TERM_REF && uNode != iRef)
    {
      if(pNodes[uNode].iLong_term_pic_num > pNodes[iRef].iLong_term_pic_num)
      {
        uint8_t uPrevNode;

        // remove node from the ordered decoding list
        if(uPrev != uEndOfList)
          pNodes[uPrev].uNextDecOrder = uNext;

        if(uNext != uEndOfList)
          pNodes[uNext].uPrevDecOrder = uPrev;

        uPrevNode = pNodes[uNode].uPrevDecOrder;

        // insert node in the ordered decoding list
        if(uPrevNode != uEndOfList)
          pNodes[uPrevNode].uNextDecOrder = iRef;

        pNodes[iRef].uPrevDecOrder = uPrevNode;
        pNodes[uNode].uPrevDecOrder = iRef;
        pNodes[iRef].uNextDecOrder = uNode;

        if(uNode == pDpb->m_uHeadDecOrder)
          pDpb->m_uHeadDecOrder = iRef;
        break;
      }
    }
    uNode = pNodes[uNode].uNextDecOrder;
  }
}

/*****************************************************************************/
static void AL_Dpb_sLongTermFrameIdxToAShortTerm(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx_long_term_frame, uint8_t iIdx_diff_pic_num)
{
  uint8_t uPic = pDpb->m_uHeadDecOrder;
  uint8_t uPic_num = uEndOfList;
  uint8_t uPic_frame = uEndOfList;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uCurPos = pDpb->m_uCurRef;
  uint32_t iDiffPicNum = pSlice->difference_of_pic_nums_minus1[iIdx_diff_pic_num] + 1;

  int16_t iLongTermFrameIdx = pSlice->long_term_frame_idx[iIdx_long_term_frame];
  int32_t iPicNumX = pNodes[uCurPos].iPic_num - iDiffPicNum;

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iLong_term_frame_idx == iLongTermFrameIdx)
      uPic_frame = uPic;

    if(pNodes[uPic].iPic_num == iPicNumX)
      uPic_num = uPic;
    uPic = pNodes[uPic].uNextDecOrder;
  }

  if(uPic_frame != uEndOfList &&
     uPic_num != uEndOfList &&
     uPic_frame != uPic_num)
  {
    if(pNodes[uPic_frame].eMarking_flag != UNUSED_FOR_REF)
      AL_Dpb_sSetPicToUnused(pDpb, uPic_frame);
  }

  if(uPic_num != uEndOfList)
  {
    pNodes[uPic_num].eMarking_flag = LONG_TERM_REF;
    pNodes[uPic_num].iLong_term_frame_idx = iLongTermFrameIdx;
    pNodes[uPic_num].iLong_term_pic_num = iLongTermFrameIdx;
    AL_Dpb_sSwitchLongTermPlace(pDpb, uPic_num);
  }
}

/*****************************************************************************/
static void AL_Dpb_sDecodingMaxLongTermFrameIdx(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  uint8_t uPic = pDpb->m_uHeadDecOrder;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iLong_term_frame_idx < 0x7FFF &&
       pNodes[uPic].iLong_term_frame_idx > (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1) &&
       pNodes[uPic].eMarking_flag == LONG_TERM_REF)
    {
      AL_Dpb_sSetPicToUnused(pDpb, uPic);
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }

  pDpb->m_MaxLongTermFrameIdx = pSlice->max_long_term_frame_idx_plus1 ?
                                (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1) : 0x7FFF;
}

/*****************************************************************************/
static void AL_Dpb_sSetAllPicAsUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t iCurRef = pDpb->m_uCurRef;
  uint8_t uPic = pDpb->m_uHeadDecOrder;// pNodes[iCurRef].uPrevDecOrder;

  while(uPic != uEndOfList) /*remove all pictures from the DPB except the current picture*/
  {
    uint8_t uNext = pNodes[uPic].uNextDecOrder;

    if(uPic != iCurRef)
    {
      if(!pNodes[uPic].non_existing)
        AL_Dpb_Display(pDpb, uPic);
      AL_Dpb_Remove(pDpb, uPic);
    }
    uPic = uNext;
  }

  pNodes[iCurRef].iFramePOC = 0;
  pNodes[iCurRef].iSlice_frame_num = 0;
  pNodes[iCurRef].eNUT = AL_HEVC_NUT_ERR;
  pDpb->m_MaxLongTermFrameIdx = 0x7FFF;

  if(pSlice->nal_ref_idc)
    AL_Dpb_SetMMCO5(pDpb);

  AL_Dpb_BeginNewSeq(pDpb);
}

/*****************************************************************************/
static void AL_Dpb_sAssignLongTermFrameIdx(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  uint8_t uPic = pDpb->m_uHeadDecOrder;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uCurPos = pDpb->m_uCurRef;
  uint8_t iLongTermFrameIdx = pSlice->long_term_frame_idx[iIdx];

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iLong_term_frame_idx == iLongTermFrameIdx)
    {
      if(pNodes[uPic].eMarking_flag == LONG_TERM_REF)
      {
        AL_Dpb_sSetPicToUnused(pDpb, uPic);
      }
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }

  pNodes[uCurPos].eMarking_flag = LONG_TERM_REF;
  pNodes[uCurPos].iLong_term_frame_idx = iLongTermFrameIdx;
  pNodes[uCurPos].iLong_term_pic_num = iLongTermFrameIdx;
  AL_Dpb_sSwitchLongTermPlace(pDpb, uCurPos);
}

/*****************************************************************************/
static void AL_Dpb_sAdaptiveMemoryControlMarking(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint8_t uMmcoIdx = 0;
  uint8_t idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;

  do
  {
    switch(pSlice->memory_management_control_operation[uMmcoIdx])
    {
    case 1:
      AL_Dpb_sShortTermToUnused(pDpb, pSlice, idx1++);
      break;

    case 2:
      AL_Dpb_sLongTermToUnused(pDpb, pSlice, idx2++);
      break;

    case 3:
      AL_Dpb_sLongTermFrameIdxToAShortTerm(pDpb, pSlice, idx3++, idx1++);
      break;

    case 4:
      AL_Dpb_sDecodingMaxLongTermFrameIdx(pDpb, pSlice, idx4++);
      break;

    case 5:
      AL_Dpb_sSetAllPicAsUnused(pDpb, pSlice);
      break;

    case 6:
      AL_Dpb_sAssignLongTermFrameIdx(pDpb, pSlice, idx3++);
      break;

    default:
      break;
    }
  }
  while(pSlice->memory_management_control_operation[uMmcoIdx++] != 0);
}

/*****************************************************************************/
static int16_t AL_Dpb_sPicNumF(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t uNodeID)
{
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  int16_t iMaxFrameNum = 1 << (pSlice->m_pSPS->log2_max_frame_num_minus4 + 4);

  if(uNodeID == uEndOfList)
    return iMaxFrameNum;

  return (pNodes[uNodeID].eMarking_flag == SHORT_TERM_REF) ? pNodes[uNodeID].iPic_num : iMaxFrameNum;
}

/*****************************************************************************/
static int16_t AL_Dpb_sLongTermPicNumF(AL_TDpb* pDpb, uint8_t uNodeID)
{
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  int16_t iMaxLongTermFrameIdx = pDpb->m_MaxLongTermFrameIdx;

  if(uNodeID == uEndOfList) // undefined reference
    return 2 * (iMaxLongTermFrameIdx + 1);

  return (pNodes[uNodeID].eMarking_flag == LONG_TERM_REF) ? pNodes[uNodeID].iLong_term_pic_num : 2 * (iMaxLongTermFrameIdx + 1);
}

/*****************************************************************************/
static void AL_Dpb_sReleaseUnusedBuf(AL_TDpb* pDpb, bool bAll)
{
  int iNum = pDpb->m_iNumDltPic ? (bAll ? pDpb->m_iNumDltPic : 1) : 0;

  while(iNum--)
  {
    pDpb->m_pfnReleaseFrmBuf(pDpb->m_pPfnCtx, pDpb->m_pDltFrmIDLst[pDpb->m_iDltFrmLstHead++]);
    pDpb->m_pfnReleaseMvBuf(pDpb->m_pPfnCtx, pDpb->m_pDltMvIDLst[pDpb->m_iDltMvLstHead++]);

    pDpb->m_iDltFrmLstHead %= FRM_BUF_POOL_SIZE;
    pDpb->m_iDltMvLstHead %= MAX_DPB_SIZE;

    --pDpb->m_iNumDltPic;
  }
}

/*****************************************************************************/
static void AL_Dpb_sFillWaitingPicture(AL_TDpb* pDpb)
{
  uint8_t uPicID = pDpb->m_Nodes[pDpb->m_uNodeWaiting].non_existing ? uEndOfList : pDpb->m_FreePicIDs[--pDpb->m_FreePicIdCnt];

  pDpb->m_Nodes[pDpb->m_uNodeWaiting].uPicID = uPicID;

  if(uPicID != uEndOfList)
  {
    pDpb->m_PicId2NodeId[uPicID] = pDpb->m_uNodeWaiting;
    pDpb->m_PicId2FrmId[uPicID] = pDpb->m_uFrmWaiting;
    pDpb->m_PicId2MvId[uPicID] = pDpb->m_uMvWaiting;
  }

  pDpb->m_uNodeWaiting = uEndOfList;
  pDpb->m_uFrmWaiting = uEndOfList;
  pDpb->m_uMvWaiting = uEndOfList;
  pDpb->m_bPicWaiting = false;
}

/***************************************************************************/
/*                          D P B     f u n c t i o n s                    */
/***************************************************************************/

/*****************************************************************************/
void AL_Dpb_Init(AL_TDpb* pDpb, uint8_t uNumRef, uint8_t uNumInterBuf, void* pPfnCtx, PfnIncrementFrmBuf pfnIncrementFrmBuf, PfnReleaseFrmBuf pfnReleaseFrmBuf, PfnOutputFrmBuf pfnOutputFrmBuf, PfnIncrementMvBuf pfnIncrementMvBuf, PfnReleaseMvBuf pfnReleaseMvBuf)
{
  int i;

  for(i = 0; i < MAX_DPB_SIZE; i++)
    AL_Dpb_sResetNodeInfo(pDpb, &pDpb->m_Nodes[i]);

  for(i = 0; i < PIC_ID_POOL_SIZE; i++)
  {
    pDpb->m_FreePicIDs[i] = i;
    pDpb->m_PicId2NodeId[i] = uEndOfList;
    pDpb->m_PicId2FrmId[i] = uEndOfList;
    pDpb->m_PicId2MvId[i] = uEndOfList;
  }

  pDpb->m_FreePicIdCnt = PIC_ID_POOL_SIZE;

  pDpb->m_bNewSeq = true;
  pDpb->m_bPicWaiting = false;
  pDpb->m_uNodeWaiting = uEndOfList;
  pDpb->m_uFrmWaiting = uEndOfList;
  pDpb->m_uMvWaiting = uEndOfList;

  pDpb->m_uNumRef = uNumRef;
  pDpb->m_uNumInterBuf = uNumInterBuf;

  pDpb->m_uHeadDecOrder = uEndOfList;
  pDpb->m_uHeadPOC = uEndOfList;
  pDpb->m_uLastPOC = uEndOfList;

  pDpb->m_uCountRef = 0;
  pDpb->m_uCountPic = 0;
  pDpb->m_uCurRef = 0;
  pDpb->m_uNumOutputPic = 0;
  pDpb->m_iLastDisplayedPOC = 0x80000000;

  pDpb->m_iDltFrmLstHead = 0;
  pDpb->m_iDltFrmLstTail = 0;
  pDpb->m_iDltMvLstHead = 0;
  pDpb->m_iDltMvLstTail = 0;
  pDpb->m_iNumDltPic = 0;

  pDpb->m_Mutex = Rtos_CreateMutex(false);

  AL_DispFifo_sInit(&pDpb->m_DispFifo);

  pDpb->m_pPfnCtx = pPfnCtx;
  pDpb->m_pfnIncrementFrmBuf = pfnIncrementFrmBuf;
  pDpb->m_pfnReleaseFrmBuf = pfnReleaseFrmBuf;
  pDpb->m_pfnOutputFrmBuf = pfnOutputFrmBuf;
  pDpb->m_pfnIncrementMvBuf = pfnIncrementMvBuf;
  pDpb->m_pfnReleaseMvBuf = pfnReleaseMvBuf;
}

/*************************************************************************/
void AL_Dpb_Terminate(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);
  AL_Dpb_sReleaseUnusedBuf(pDpb, true);
  AL_DispFifo_sDeinit(&pDpb->m_DispFifo);
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_Deinit(AL_TDpb* pDpb)
{
  Rtos_DeleteMutex(pDpb->m_Mutex);
}

/*****************************************************************************/
uint8_t AL_Dpb_GetRefCount(AL_TDpb* pDpb)
{
  return pDpb->m_uCountRef;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetPicCount(AL_TDpb* pDpb)
{
  return pDpb->m_uCountPic;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetHeadPOC(AL_TDpb* pDpb)
{
  return pDpb->m_uHeadPOC;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetNextPOC(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].uNextPOC;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetOutputFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].pic_output_flag;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetNumOutputPict(AL_TDpb* pDpb)
{
  return pDpb->m_uNumOutputPic;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetMarkingFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].eMarking_flag;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetFifoLast(AL_TDpb* pDpb)
{
  return pDpb->m_DispFifo.uFirstFrm + pDpb->m_DispFifo.uNumFrm;
}

/*************************************************************************/
uint32_t AL_Dpb_GetPicLatency_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].uPicLatency;
}

/*************************************************************************/
uint32_t AL_Dpb_GetPicLatency_FromFifo(AL_TDpb* pDpb, uint8_t uFrmID)
{
  return pDpb->m_DispFifo.pPicLatency[uFrmID];
}

/*************************************************************************/
uint8_t AL_Dpb_GetPicID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].uPicID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetMvID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].uMvID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetFrmID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->m_Nodes[uNode].uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetFrmID_FromFifo(AL_TDpb* pDpb, uint8_t uID)
{
  return pDpb->m_DispFifo.pFrmIDs[uID % FRM_BUF_POOL_SIZE];
}

/*************************************************************************/
uint8_t AL_Dpb_GetLastPicID(AL_TDpb* pDpb)
{
  uint8_t uRetID;
  DPB_GET_MUTEX(pDpb);
  {
    uint8_t uNode = pDpb->m_uHeadPOC;

    if(uNode == uEndOfList)
      uRetID = uNode;
    else
    {
      while(pDpb->m_Nodes[uNode].uNextPOC != uEndOfList)
        uNode = pDpb->m_Nodes[uNode].uNextPOC;

      uRetID = pDpb->m_Nodes[uNode].uPicID;
    }
  }
  DPB_RELEASE_MUTEX(pDpb);
  return uRetID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetNumRef(AL_TDpb* pDpb)
{
  return pDpb->m_uNumRef;
}

/*************************************************************************/
uint8_t AL_Dpb_GetNumPic(AL_TDpb* pDpb)
{
  return pDpb->m_uNumRef + pDpb->m_uNumInterBuf;
}

/*************************************************************************/
void AL_Dpb_SetNumRef(AL_TDpb* pDpb, uint8_t uMaxRef)
{
  pDpb->m_uNumRef = uMaxRef;
}

/*************************************************************************/
void AL_Dpb_SetMarkingFlag(AL_TDpb* pDpb, uint8_t uNode, AL_EMarkingRef eMarkingFlag)
{
  pDpb->m_Nodes[uNode].eMarking_flag = eMarkingFlag;
}

/*************************************************************************/
void AL_Dpb_ResetOutputFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  pDpb->m_Nodes[uNode].pic_output_flag = 0;
  --pDpb->m_uNumOutputPic;
}

/*************************************************************************/
void AL_Dpb_IncrementPicLatency(AL_TDpb* pDpb, uint8_t uNode, int iCurFramePOC)
{
  if(pDpb->m_Nodes[uNode].iFramePOC > iCurFramePOC)
    ++pDpb->m_Nodes[uNode].uPicLatency;
}

/*************************************************************************/
void AL_Dpb_DecrementPicLatency(AL_TDpb* pDpb, uint8_t uNode)
{
  --pDpb->m_Nodes[uNode].uPicLatency;
}

/*************************************************************************/
void AL_Dpb_BeginNewSeq(AL_TDpb* pDpb)
{
  pDpb->m_bNewSeq = true;
  pDpb->m_iLastDisplayedPOC = 0x80000000;
}

/*************************************************************************/
bool AL_Dpb_NodeIsReset(AL_TDpb* pDpb, uint8_t uNode)
{
  return pDpb->m_Nodes[uNode].bIsReset;
}

/*************************************************************************/
bool AL_Dpb_LastHasMMCO5(AL_TDpb* pDpb)
{
  return pDpb->m_bLastHasMMCO5;
}

/*************************************************************************/
void AL_Dpb_SetMMCO5(AL_TDpb* pDpb)
{
  pDpb->m_bLastHasMMCO5 = true;
}

/*************************************************************************/
void AL_Dpb_ResetMMCO5(AL_TDpb* pDpb)
{
  pDpb->m_bLastHasMMCO5 = false;
}

/*************************************************************************/
uint8_t AL_Dpb_ConvertPicIDToNodeID(AL_TDpb const* pDpb, uint8_t uPicID)
{
  return pDpb->m_PicId2NodeId[uPicID];
}

/*************************************************************************/
uint8_t AL_Dpb_GetNextFreeNode(AL_TDpb* pDpb)
{
  uint8_t uNew = 0;

  DPB_GET_MUTEX(pDpb);

  while(pDpb->m_Nodes[uNew].eMarking_flag != UNUSED_FOR_REF || pDpb->m_Nodes[uNew].pic_output_flag)
    uNew = (uNew + 1) % MAX_DPB_SIZE;

  DPB_RELEASE_MUTEX(pDpb);

  return uNew;
}

/*****************************************************************************/
void AL_Dpb_FillList(AL_TDpb* pDpb, uint8_t uL0L1, TBufferListRef* pListRef, int* pPocList, uint32_t* pLongTermList)
{
  uint8_t i;

  DPB_GET_MUTEX(pDpb);

  for(i = 0; i < MAX_REF; ++i)
  {
    uint8_t uNodeID = (*pListRef)[uL0L1][i].uNodeID;

    if(uNodeID != uEndOfList && !pDpb->m_Nodes[uNodeID].non_existing)
    {
      uint8_t uPicID = pDpb->m_Nodes[uNodeID].uPicID;
      pPocList[uPicID] = pDpb->m_Nodes[uNodeID].iFramePOC;

      if(pDpb->m_Nodes[uNodeID].eMarking_flag == LONG_TERM_REF)
        *pLongTermList |= (1 << uPicID); // long term flag
      *pLongTermList |= ((uint32_t)1 << (16 + uPicID)); // available POC
    }
  }

  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
uint8_t AL_Dpb_SearchPocLsb(AL_TDpb* pDpb, uint32_t poc_lsb)
{
  uint8_t uParse;
  DPB_GET_MUTEX(pDpb);

  uParse = pDpb->m_uHeadPocLsb;

  while(uParse != uEndOfList)
  {
    if(pDpb->m_Nodes[uParse].slice_pic_order_cnt_lsb == poc_lsb && pDpb->m_Nodes[uParse].eMarking_flag != UNUSED_FOR_REF)
      break;
    else
      uParse = pDpb->m_Nodes[uParse].uNextPocLsb;
  }

  DPB_RELEASE_MUTEX(pDpb);
  return uParse;
}

/*****************************************************************************/
uint8_t AL_Dpb_SearchPOC(AL_TDpb* pDpb, int iPOC)
{
  uint8_t uParse;
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  DPB_GET_MUTEX(pDpb);

  uParse = pDpb->m_uHeadPOC;

  while(uParse != uEndOfList)
  {
    if(pNodes[uParse].iFramePOC == iPOC && pNodes[uParse].eMarking_flag != UNUSED_FOR_REF && !pNodes[uParse].non_existing)
      break;
    else
      uParse = pDpb->m_Nodes[uParse].uNextPOC;
  }

  DPB_RELEASE_MUTEX(pDpb);
  return uParse;
}

/*****************************************************************************/
void AL_Dpb_Display(AL_TDpb* pDpb, uint8_t uNode)
{
  uint8_t uParse;
  DPB_GET_MUTEX(pDpb);

  uParse = pDpb->m_uHeadPOC;

  // check if there is anterior picture to be displayed present in the DPB
  while(uParse != uNode)
  {
    uint8_t uNext = pDpb->m_Nodes[uParse].uNextPOC;

    if(pDpb->m_Nodes[uParse].pic_output_flag && (pDpb->m_Nodes[uParse].iFramePOC > pDpb->m_iLastDisplayedPOC || pDpb->m_bNewSeq))
      AL_Dpb_Display(pDpb, uParse);
    uParse = uNext;
  }

  // add current picture to display list
  if(pDpb->m_Nodes[uNode].pic_output_flag && (pDpb->m_Nodes[uNode].iFramePOC > pDpb->m_iLastDisplayedPOC || pDpb->m_bNewSeq))
  {
    if(!pDpb->m_Nodes[uNode].non_existing)
    {
      AL_Dpb_sAddToDisplayList(pDpb, uNode);
      pDpb->m_iLastDisplayedPOC = pDpb->m_Nodes[uNode].iFramePOC;
    }

    pDpb->m_Nodes[uNode].bIsDisplayed = true;
    pDpb->m_Nodes[uNode].pic_output_flag = 0;
    pDpb->m_bNewSeq = false;
    --pDpb->m_uNumOutputPic;
  }
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
uint8_t AL_Dpb_GetDisplayBuffer(AL_TDpb* pDpb)
{
  uint8_t uFrmID = UndefID;
  DPB_GET_MUTEX(pDpb);
  {
    uint8_t uEnd = AL_Dpb_GetFifoLast(pDpb) % FRM_BUF_POOL_SIZE;

    if(pDpb->m_DispFifo.uFirstFrm != uEnd || pDpb->m_DispFifo.uNumFrm == FRM_BUF_POOL_SIZE)
    {
      uFrmID = pDpb->m_DispFifo.pFrmIDs[pDpb->m_DispFifo.uFirstFrm];

      if(uFrmID != UndefID && pDpb->m_DispFifo.pFrmStatus[uFrmID] == AL_READY_FOR_OUTPUT)
      {
        pDpb->m_DispFifo.pFrmIDs[pDpb->m_DispFifo.uFirstFrm] = UndefID;
        pDpb->m_DispFifo.pFrmStatus[uFrmID] = AL_NOT_NEEDED_FOR_OUTPUT;
        pDpb->m_DispFifo.uFirstFrm = (pDpb->m_DispFifo.uFirstFrm + 1) % FRM_BUF_POOL_SIZE;
        --pDpb->m_DispFifo.uNumFrm;
      }
      else
        uFrmID = UndefID;
    }
  }
  DPB_RELEASE_MUTEX(pDpb);
  return uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_ReleaseDisplayBuffer(AL_TDpb* pDpb)
{
  uint8_t uFrmID;
  DPB_GET_MUTEX(pDpb);
  {
    uFrmID = pDpb->m_DispFifo.pFrmIDs[pDpb->m_DispFifo.uFirstFrm];

    if(uFrmID != UndefID)
    {
      pDpb->m_DispFifo.pFrmIDs[pDpb->m_DispFifo.uFirstFrm] = UndefID;
      pDpb->m_DispFifo.uFirstFrm = (pDpb->m_DispFifo.uFirstFrm + 1) % FRM_BUF_POOL_SIZE;
      pDpb->m_DispFifo.uNumFrm--;
      pDpb->m_DispFifo.pFrmStatus[uFrmID] = AL_NOT_NEEDED_FOR_OUTPUT;
      pDpb->m_pfnReleaseFrmBuf(pDpb->m_pPfnCtx, uFrmID);
    }
  }
  DPB_RELEASE_MUTEX(pDpb);
  return uFrmID;
}

/*************************************************************************/
void AL_Dpb_ClearOutput(AL_TDpb* pDpb)
{
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uNode;
  DPB_GET_MUTEX(pDpb);
  uNode = pDpb->m_uHeadPOC;

  while(uNode != uEndOfList)
  {
    pNodes[uNode].pic_output_flag = 0;
    uNode = pNodes[uNode].uNextPOC;
  }

  pDpb->m_uNumOutputPic = 0;
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_Flush(AL_TDpb* pDpb)
{
  uint8_t uHeadPOC;

  DPB_GET_MUTEX(pDpb);

  while(uEndOfList != (uHeadPOC = AL_Dpb_GetHeadPOC(pDpb)))
  {
    if(AL_Dpb_GetOutputFlag(pDpb, uHeadPOC))
      AL_Dpb_Display(pDpb, uHeadPOC);
    AL_Dpb_RemoveHead(pDpb);
  }

  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_HEVC_Cleanup(AL_TDpb* pDpb, uint32_t uMaxLatency, uint8_t MaxNumOutput)
{
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uNode;
  DPB_GET_MUTEX(pDpb);

  uNode = pDpb->m_uHeadPOC;

  while(uNode != uEndOfList)
  {
    if(AL_Dpb_GetOutputFlag(pDpb, uNode) && ((AL_Dpb_GetPicLatency_FromNode(pDpb, uNode) >= uMaxLatency) ||
                                             (AL_Dpb_GetNumOutputPict(pDpb) > MaxNumOutput)))
    {
      AL_Dpb_Display(pDpb, uNode);
    }

    if(pNodes[uNode].eMarking_flag == UNUSED_FOR_REF && !pNodes[uNode].pic_output_flag)
    {
      uint8_t uDelete = uNode;
      uNode = pNodes[uNode].uNextPOC;

      AL_Dpb_Remove(pDpb, uDelete);
    }
    else
      uNode = pNodes[uNode].uNextPOC;
  }

  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_AVC_Cleanup(AL_TDpb* pDpb)
{
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uNode;

  DPB_GET_MUTEX(pDpb);

  uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList)
  {
    uint8_t uNextNode = pNodes[uNode].uNextPOC;

    // Remove useless pictures
    if(AL_Dpb_GetMarkingFlag(pDpb, uNode) == UNUSED_FOR_REF && !AL_Dpb_GetOutputFlag(pDpb, uNode))
      AL_Dpb_Remove(pDpb, uNode);

    uNode = uNextNode;
  }

  uNode = AL_Dpb_GetHeadPOC(pDpb);

  while(uNode != uEndOfList && (AL_Dpb_GetRefCount(pDpb) > AL_Dpb_GetNumRef(pDpb) || AL_Dpb_GetPicCount(pDpb) > AL_Dpb_GetNumRef(pDpb)))
  {
    uint8_t uNextNode = pNodes[uNode].uNextPOC;

    // Remove useless pictures
    if(AL_Dpb_GetMarkingFlag(pDpb, uNode) == UNUSED_FOR_REF)
    {
      AL_Dpb_Display(pDpb, uNode);
      AL_Dpb_Remove(pDpb, uNode);
    }

    uNode = uNextNode;
  }

  // if waiting for pic ID picture
  if(pDpb->m_bPicWaiting && pDpb->m_FreePicIdCnt)
    AL_Dpb_sFillWaitingPicture(pDpb);

  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
uint8_t AL_Dpb_Remove(AL_TDpb* pDpb, uint8_t uNode)
{
  uint8_t uFrmID = UndefID;

  DPB_GET_MUTEX(pDpb);

  if(uNode != uEndOfList)
  {
    uint8_t uPrev = pDpb->m_Nodes[uNode].uPrevPOC;
    uint8_t uNext = pDpb->m_Nodes[uNode].uNextPOC;
    uint8_t uMvID = pDpb->m_Nodes[uNode].uMvID;

    uint8_t uNE = pDpb->m_Nodes[uNode].non_existing;
    uFrmID = pDpb->m_Nodes[uNode].uFrmID;

    // Remove from POC ordered linked list
    if(pDpb->m_uHeadPOC == uNode)
      pDpb->m_uHeadPOC = uNext;

    if(uPrev != uEndOfList)
      pDpb->m_Nodes[uPrev].uNextPOC = uNext;

    if(uNext != uEndOfList)
      pDpb->m_Nodes[uNext].uPrevPOC = uPrev;
    else
      pDpb->m_uLastPOC = uPrev;

    // Remove from poc lsb ordered linked list
    uPrev = pDpb->m_Nodes[uNode].uPrevPocLsb;
    uNext = pDpb->m_Nodes[uNode].uNextPocLsb;

    if(pDpb->m_uHeadPocLsb == uNode)
      pDpb->m_uHeadPocLsb = uNext;

    if(uPrev != uEndOfList)
      pDpb->m_Nodes[uPrev].uNextPocLsb = uNext;

    if(uNext != uEndOfList)
      pDpb->m_Nodes[uNext].uPrevPocLsb = uPrev;

    // Remove from Decoding ordered linked list
    uPrev = pDpb->m_Nodes[uNode].uPrevDecOrder;
    uNext = pDpb->m_Nodes[uNode].uNextDecOrder;

    if(pDpb->m_uHeadDecOrder == uNode)
      pDpb->m_uHeadDecOrder = uNext;

    if(uPrev != uEndOfList)
      pDpb->m_Nodes[uPrev].uNextDecOrder = uNext;

    if(uNext != uEndOfList)
      pDpb->m_Nodes[uNext].uPrevDecOrder = uPrev;

    // Release node
    AL_Dpb_sReleasePicID(pDpb, pDpb->m_Nodes[uNode].uPicID);

    // Update ref counter
    if(pDpb->m_Nodes[uNode].eMarking_flag != UNUSED_FOR_REF)
      --pDpb->m_uCountRef;
    --pDpb->m_uCountPic;

    // Reset node information
    AL_Dpb_sResetNodeInfo(pDpb, &pDpb->m_Nodes[uNode]);

    // assigned pic id to the awaiting picture
    if(pDpb->m_bPicWaiting && pDpb->m_FreePicIdCnt)
      AL_Dpb_sFillWaitingPicture(pDpb);

    if(!uNE)
    {
      pDpb->m_pDltFrmIDLst[pDpb->m_iDltFrmLstTail++] = uFrmID;
      pDpb->m_pDltMvIDLst[pDpb->m_iDltMvLstTail++] = uMvID;

      pDpb->m_iDltFrmLstTail %= FRM_BUF_POOL_SIZE;
      pDpb->m_iDltMvLstTail %= MAX_DPB_SIZE;

      ++pDpb->m_iNumDltPic;
    }
  }
  DPB_RELEASE_MUTEX(pDpb);

  return uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_RemoveHead(AL_TDpb* pDpb)
{
  // Remove header of the Decoding ordered linked list
  return AL_Dpb_Remove(pDpb, pDpb->m_uHeadPOC);
}

/*****************************************************************************/
void AL_Dpb_Insert(AL_TDpb* pDpb, int iFramePOC, uint32_t uPocLsb, uint8_t uNode, uint8_t uFrmID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT)
{
  uint8_t uPicID = uEndOfList;

  DPB_GET_MUTEX(pDpb);

  // Assign PicID
  if(!uNonExisting)
  {
    if(pDpb->m_FreePicIdCnt)
    {
      uPicID = pDpb->m_FreePicIDs[--pDpb->m_FreePicIdCnt];
      pDpb->m_PicId2NodeId[uPicID] = uNode;
      pDpb->m_PicId2FrmId[uPicID] = uFrmID;
      pDpb->m_PicId2MvId[uPicID] = uMvID;
    }
    else
    {
      pDpb->m_bPicWaiting = true;
      pDpb->m_uNodeWaiting = uNode;
      pDpb->m_uFrmWaiting = uFrmID;
      pDpb->m_uMvWaiting = uMvID;
    }
  }

  pDpb->m_uCurRef = uNode;

  // Assign frame buffer informations
  pDpb->m_Nodes[uNode].iFramePOC = iFramePOC;
  pDpb->m_Nodes[uNode].slice_pic_order_cnt_lsb = uPocLsb;
  pDpb->m_Nodes[uNode].uFrmID = uFrmID;
  pDpb->m_Nodes[uNode].uMvID = uMvID;
  pDpb->m_Nodes[uNode].uPicID = uPicID;
  pDpb->m_Nodes[uNode].eMarking_flag = eMarkingFlag;
  pDpb->m_Nodes[uNode].uNodeID = uNode;
  pDpb->m_Nodes[uNode].pic_output_flag = pic_output_flag;
  pDpb->m_Nodes[uNode].bIsReset = false;
  pDpb->m_Nodes[uNode].non_existing = uNonExisting;
  pDpb->m_Nodes[uNode].eNUT = eNUT;

  // Update frame status in display fifo list
  if(uFrmID != uEndOfList)
    pDpb->m_DispFifo.pFrmStatus[uFrmID] = pic_output_flag ? AL_NOT_READY_FOR_OUTPUT : AL_NOT_NEEDED_FOR_OUTPUT;

  // Insert it in the POC ordered linked list
  if(pDpb->m_uHeadPOC == uEndOfList)
  {
    pDpb->m_Nodes[uNode].uPrevPOC = uEndOfList;
    pDpb->m_Nodes[uNode].uNextPOC = uEndOfList;
    pDpb->m_uHeadPOC = uNode;
    pDpb->m_uLastPOC = uNode;

    pDpb->m_Nodes[uNode].uPrevPocLsb = uEndOfList;
    pDpb->m_Nodes[uNode].uNextPocLsb = uEndOfList;
    pDpb->m_uHeadPocLsb = uNode;

    pDpb->m_Nodes[uNode].uPrevDecOrder = uEndOfList;
    pDpb->m_Nodes[uNode].uNextDecOrder = uEndOfList;
    pDpb->m_uHeadDecOrder = uNode;
  }
  else
  {
    uint8_t uCurPOC = pDpb->m_uHeadPOC;
    uint8_t uCurPocLsb = pDpb->m_uHeadPocLsb;
    uint8_t uCurDecOrder = pDpb->m_uHeadDecOrder;

    while(1)
    {
      // compute poc ordered list
      if(pDpb->m_Nodes[uCurPOC].iFramePOC > iFramePOC)
      {
        uint8_t uPrev = pDpb->m_Nodes[uCurPOC].uPrevPOC;

        pDpb->m_Nodes[uNode].uPrevPOC = uPrev;
        pDpb->m_Nodes[uNode].uNextPOC = uCurPOC;
        pDpb->m_Nodes[uCurPOC].uPrevPOC = uNode;

        if(uPrev != uEndOfList)
          pDpb->m_Nodes[uPrev].uNextPOC = uNode;

        if(uCurPOC == pDpb->m_uHeadPOC)
          pDpb->m_uHeadPOC = uNode;
        break;
      }
      else if(pDpb->m_Nodes[uCurPOC].uNextPOC == uEndOfList)
      {
        pDpb->m_Nodes[uNode].uNextPOC = uEndOfList;
        pDpb->m_Nodes[uNode].uPrevPOC = uCurPOC;
        pDpb->m_Nodes[uCurPOC].uNextPOC = uNode;

        pDpb->m_uLastPOC = uNode;
        break;
      }
      else
        uCurPOC = pDpb->m_Nodes[uCurPOC].uNextPOC;
    }

    while(1)
    {
      // compute poc lsb ordered list
      if(pDpb->m_Nodes[uCurPocLsb].slice_pic_order_cnt_lsb > uPocLsb)
      {
        uint8_t uPrev = pDpb->m_Nodes[uCurPocLsb].uPrevPocLsb;
        pDpb->m_Nodes[uNode].uPrevPocLsb = uPrev;
        pDpb->m_Nodes[uNode].uNextPocLsb = uCurPocLsb;
        pDpb->m_Nodes[uCurPocLsb].uPrevPocLsb = uNode;

        if(uPrev != uEndOfList)
          pDpb->m_Nodes[uPrev].uNextPocLsb = uNode;

        if(uCurPocLsb == pDpb->m_uHeadPocLsb)
          pDpb->m_uHeadPocLsb = uNode;
        break;
      }
      else if(pDpb->m_Nodes[uCurPocLsb].uNextPocLsb == uEndOfList)
      {
        pDpb->m_Nodes[uNode].uNextPocLsb = uEndOfList;
        pDpb->m_Nodes[uNode].uPrevPocLsb = uCurPocLsb;
        pDpb->m_Nodes[uCurPocLsb].uNextPocLsb = uNode;
        break;
      }
      else
        uCurPocLsb = pDpb->m_Nodes[uCurPocLsb].uNextPocLsb;
    }

    // Decoding order linked list
    while(pDpb->m_Nodes[uCurDecOrder].uNextDecOrder != uEndOfList)
      uCurDecOrder = pDpb->m_Nodes[uCurDecOrder].uNextDecOrder;

    pDpb->m_Nodes[uNode].uPrevDecOrder = uCurDecOrder;
    pDpb->m_Nodes[uNode].uNextDecOrder = uEndOfList;
    pDpb->m_Nodes[uCurDecOrder].uNextDecOrder = uNode;
  }

  // Update List counters
  if(eMarkingFlag != UNUSED_FOR_REF)
    ++pDpb->m_uCountRef;
  ++pDpb->m_uCountPic;

  if(pic_output_flag)
    ++pDpb->m_uNumOutputPic;

  if(uFrmID != uEndOfList)
    pDpb->m_pfnIncrementFrmBuf(pDpb->m_pPfnCtx, uFrmID);

  if(uMvID != uEndOfList)
    pDpb->m_pfnIncrementMvBuf(pDpb->m_pPfnCtx, uMvID);
  DPB_RELEASE_MUTEX(pDpb);
}

/*****************************************************************************/
void AL_Dpb_EndDecoding(AL_TDpb* pDpb, int iFrmID)
{
  DPB_GET_MUTEX(pDpb);

  AL_Dpb_sReleaseUnusedBuf(pDpb, false);

  // Set Frame Availability
  if(pDpb->m_DispFifo.pFrmStatus[iFrmID] == AL_NOT_READY_FOR_OUTPUT)
    pDpb->m_DispFifo.pFrmStatus[iFrmID] = AL_READY_FOR_OUTPUT;

  DPB_RELEASE_MUTEX(pDpb);
}

/*****************************************************************************/
void AL_Dpb_PictNumberProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint8_t uPicID = pDpb->m_uHeadDecOrder;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint32_t uMaxFrameNum = 1 << (pSlice->m_pSPS->log2_max_frame_num_minus4 + 4);

  while(uPicID != uEndOfList)// loop over references pictures
  {
    if(pNodes[uPicID].eMarking_flag == UNUSED_FOR_REF)
    {
      uPicID = pNodes[uPicID].uNextDecOrder;
      continue;
    }

    if(pNodes[uPicID].eMarking_flag == SHORT_TERM_REF)
    {
      pNodes[uPicID].iFrame_num = pNodes[uPicID].iSlice_frame_num;

      if(pNodes[uPicID].iFrame_num > pSlice->frame_num)
        pNodes[uPicID].iFrame_num_wrap = pNodes[uPicID].iFrame_num - uMaxFrameNum;
      else
        pNodes[uPicID].iFrame_num_wrap = pNodes[uPicID].iFrame_num;
      pNodes[uPicID].iPic_num = pNodes[uPicID].iFrame_num_wrap;
    }
    else if(pNodes[uPicID].eMarking_flag == LONG_TERM_REF)
      pNodes[uPicID].iLong_term_pic_num = pNodes[uPicID].iLong_term_frame_idx;

    uPicID = pNodes[uPicID].uNextDecOrder;
  }
}

/******************************************************************************/
void AL_Dpb_MarkingProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint8_t op_idc;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;
  uint8_t uCurPos = pDpb->m_uCurRef;

  // fill node's parameters
  pNodes[uCurPos].iSlice_frame_num = pSlice->frame_num;
  pNodes[uCurPos].uNodeID = uCurPos;

  if(pSlice->nal_unit_type == 5) /*IDR picture case*/
  {
    if(!pSlice->long_term_reference_flag)
      pDpb->m_MaxLongTermFrameIdx = 0x7FFF;
    else
    {
      pNodes[uCurPos].iLong_term_frame_idx = 0;
      pDpb->m_MaxLongTermFrameIdx = 0;
    }
  }
  else /*non IDR picture case*/
  {
    AL_Dpb_PictNumberProcess(pDpb, pSlice);

    if(!pSlice->adaptive_ref_pic_marking_mode_flag)
      AL_Dpb_sSliding_window_marking(pDpb, pSlice);
    else
      AL_Dpb_sAdaptiveMemoryControlMarking(pDpb, pSlice);

    for(op_idc = 0; op_idc < 32; ++op_idc)
    {
      if(pSlice->memory_management_control_operation[op_idc] == 6 &&
         pNodes[uCurPos].eMarking_flag != LONG_TERM_REF)
      {
        pNodes[uCurPos].eMarking_flag = SHORT_TERM_REF;
        pNodes[uCurPos].iLong_term_frame_idx = 0x7FFF;
        break;
      }
    }
  }
}

/*****************************************************************************/
void AL_Dpb_InitPSlice_RefList(AL_TDpb* pDpb, TBufferRef* pRefList)
{
  int iPic, iRef;
  int iCnt_short = MAX_REF - 1;
  int iCnt_long = 0;

  uint8_t uNode = pDpb->m_uHeadDecOrder;

  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  AL_TDpbNode NodeShortTerm[16], NodeLongTerm[16];
  AL_TDpbNode* pNodeTemp;

  Rtos_Memset(&NodeShortTerm[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, 16 * sizeof(AL_TDpbNode));

  while(uNode != uEndOfList)
  {
    if(pNodes[uNode].eMarking_flag != UNUSED_FOR_REF)
    {
      pNodeTemp = (pNodes[uNode].eMarking_flag == SHORT_TERM_REF) ?
                  &NodeShortTerm[iCnt_short--] : &NodeLongTerm[iCnt_long++];
      *pNodeTemp = pNodes[uNode];
    }
    uNode = pNodes[uNode].uNextDecOrder;
  }

  // merge short term and long term reference in the RefList
  iPic = 0;
  iRef = iCnt_short + 1;
  iCnt_short = MAX_REF - iRef;

  while(iPic < iCnt_short)
    pRefList[iPic++].uNodeID = NodeShortTerm[iRef++].uNodeID;

  iRef = 0;

  while(iPic < (iCnt_short + iCnt_long))
    pRefList[iPic++].uNodeID = NodeLongTerm[iRef++].uNodeID;
}

/*****************************************************************************/
void AL_Dpb_InitBSlice_RefList(AL_TDpb* pDpb, int iCurFramePOC, TBufferListRef* pListRef)
{
  int iPic, iRef;
  int iCnt_short_great = 0;
  int iCnt_long = 0;
  int iCnt_short_less = MAX_REF - 1;

  uint8_t uShortNode = pDpb->m_uHeadPOC;
  uint8_t uLongNode = pDpb->m_uHeadDecOrder;
  // fast access
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  AL_TDpbNode NodeShortTermPOCGreat[16], NodeShortTermPOCLess[16], NodeLongTerm[16];
  AL_TDpbNode* pNodeShort;
  AL_TDpbNode NodeTemp;

  Rtos_Memset(&NodeShortTermPOCGreat[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeShortTermPOCLess[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, 16 * sizeof(AL_TDpbNode));

  while(uShortNode != uEndOfList)
  {
    if(pNodes[uShortNode].eMarking_flag == SHORT_TERM_REF) /*short term reference case*/
    {
      pNodeShort = (pNodes[uShortNode].iFramePOC < iCurFramePOC) ?
                   &NodeShortTermPOCLess[iCnt_short_less--] : &NodeShortTermPOCGreat[iCnt_short_great++];
      *pNodeShort = pNodes[uShortNode];
    }
    uShortNode = pNodes[uShortNode].uNextPOC;
  }

  while(uLongNode != uEndOfList) /*long_term_reference case*/
  {
    if(pNodes[uLongNode].eMarking_flag == LONG_TERM_REF)
      NodeLongTerm[iCnt_long++] = pNodes[uLongNode];
    uLongNode = pNodes[uLongNode].uNextDecOrder;
  }

  iPic = 0;
  iRef = iCnt_short_less + 1;
  iCnt_short_less = MAX_REF - iRef;

  // List 0
  while(iPic < iCnt_short_less)
    (*pListRef)[0][iPic++].uNodeID = NodeShortTermPOCLess[iRef++].uNodeID;

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great))
    (*pListRef)[0][iPic++].uNodeID = NodeShortTermPOCGreat[iRef++].uNodeID;

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great + iCnt_long))
    (*pListRef)[0][iPic++].uNodeID = NodeLongTerm[iRef++].uNodeID;

  iPic = 0;
  iRef = 0;

  // List 1
  while(iPic < iCnt_short_great)
    (*pListRef)[1][iPic++].uNodeID = NodeShortTermPOCGreat[iRef++].uNodeID;

  iRef = MAX_REF - iCnt_short_less;

  while(iPic < iCnt_short_less + iCnt_short_great)
    (*pListRef)[1][iPic++].uNodeID = NodeShortTermPOCLess[iRef++].uNodeID;

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great + iCnt_long))
    (*pListRef)[1][iPic++].uNodeID = NodeLongTerm[iRef++].uNodeID;

  if(iCnt_short_less + iCnt_short_great + iCnt_long > 1)
  {
    for(iPic = 0; iPic < iCnt_short_less + iCnt_short_great + iCnt_long; ++iPic)
    {
      if((*pListRef)[1][iPic].uNodeID != (*pListRef)[0][iPic].uNodeID)
        return;
    }
  }
  else
    return;
  NodeTemp = pNodes[(*pListRef)[1][0].uNodeID & 0x3F];
  (*pListRef)[1][0].uNodeID = (*pListRef)[1][1].uNodeID;
  (*pListRef)[1][1].uNodeID = NodeTemp.uNodeID;
}

/*****************************************************************************/
void AL_Dpb_ModifShortTerm(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, int iPicNumIdc, uint8_t uOffset, int iL0L1, uint8_t* pRefIdx, int* pPicNumPred, TBufferListRef* pListRef)
{
  // fast access
  int32_t iMaxFrameNum = 1 << (pSlice->m_pSPS->log2_max_frame_num_minus4 + 4);
  int iDiffPicNum = iL0L1 ? pSlice->abs_diff_pic_num_minus1_l1[uOffset] + 1 :
                    pSlice->abs_diff_pic_num_minus1_l0[uOffset] + 1;

  uint8_t uNumRef = iL0L1 ? pSlice->num_ref_idx_l1_active_minus1 + 1 :
                    pSlice->num_ref_idx_l0_active_minus1 + 1;

  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  int iPicNumNoWrap;
  int iPicNum;
  uint8_t uCpt;
  uint8_t unIdx;

  if(!iPicNumIdc)
    iPicNumNoWrap = ((*pPicNumPred - iDiffPicNum) < 0) ?
                    *pPicNumPred - iDiffPicNum + iMaxFrameNum : *pPicNumPred - iDiffPicNum + 0;
  else
    iPicNumNoWrap = ((*pPicNumPred + iDiffPicNum) >= iMaxFrameNum) ?
                    *pPicNumPred + iDiffPicNum - iMaxFrameNum : *pPicNumPred + iDiffPicNum + 0;

  *pPicNumPred = iPicNumNoWrap;

  /*warning : only frame slice*/
  iPicNum = (iPicNumNoWrap > pSlice->frame_num) ? iPicNumNoWrap - iMaxFrameNum : iPicNumNoWrap - 0;

  /*process reordering*/
  for(uCpt = uNumRef; uCpt > *pRefIdx; --uCpt)
    (*pListRef)[iL0L1][uCpt] = (*pListRef)[iL0L1][uCpt - 1];

  uCpt = pDpb->m_uHeadDecOrder;

  while(uCpt != uEndOfList)
  {
    if(pNodes[uCpt].iPic_num == iPicNum && pNodes[uCpt].eMarking_flag == SHORT_TERM_REF)
    {
      (*pListRef)[iL0L1][(*pRefIdx)++].uNodeID = pNodes[uCpt].uNodeID;

      unIdx = *pRefIdx;

      for(uCpt = *pRefIdx; uCpt <= uNumRef; ++uCpt)
      {
        if(AL_Dpb_sPicNumF(pDpb, pSlice, (*pListRef)[iL0L1][uCpt].uNodeID) != iPicNum)
          (*pListRef)[iL0L1][unIdx++] = (*pListRef)[iL0L1][uCpt];
      }

      break;
    }
    uCpt = pNodes[uCpt].uNextDecOrder;
  }
}

/*****************************************************************************/
void AL_Dpb_ModifLongTerm(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t uOffset, int iL0L1, uint8_t* pRefIdx, TBufferListRef* pListRef)
{
  // fast access
  uint8_t uNumRef = iL0L1 ?
                    pSlice->num_ref_idx_l1_active_minus1 + 1 : pSlice->num_ref_idx_l0_active_minus1 + 1;
  int16_t iLongTermPicNum = iL0L1 ?
                            pSlice->long_term_pic_num_l1[uOffset] : pSlice->long_term_pic_num_l0[uOffset];
  AL_TDpbNode* pNodes = pDpb->m_Nodes;

  uint8_t uCpt;
  uint8_t unIdx;

  for(uCpt = uNumRef; uCpt > *pRefIdx; --uCpt)
    (*pListRef)[iL0L1][uCpt] = (*pListRef)[iL0L1][uCpt - 1];

  uCpt = pDpb->m_uHeadDecOrder;

  while(uCpt != uEndOfList)
  {
    if(pNodes[uCpt].iLong_term_pic_num == iLongTermPicNum && pNodes[uCpt].eMarking_flag == LONG_TERM_REF)
    {
      (*pListRef)[iL0L1][(*pRefIdx)++].uNodeID = pNodes[uCpt].uNodeID;

      unIdx = *pRefIdx;

      for(uCpt = *pRefIdx; uCpt <= uNumRef; ++uCpt)
      {
        if(AL_Dpb_sLongTermPicNumF(pDpb, (*pListRef)[iL0L1][uCpt].uNodeID) != iLongTermPicNum)
          (*pListRef)[iL0L1][unIdx++] = (*pListRef)[iL0L1][uCpt];
      }

      break;
    }
    uCpt = pNodes[uCpt].uNextDecOrder;
  }
}

/*@}*/

