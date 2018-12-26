/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#define DPB_GET_MUTEX(pDpb) Rtos_GetMutex(pDpb->Mutex)
#define DPB_RELEASE_MUTEX(pDpb) Rtos_ReleaseMutex(pDpb->Mutex)

static void DispFifo_Init(AL_TDispFifo* pFifo)
{
  for(int i = 0; i < FRM_BUF_POOL_SIZE; ++i)
  {
    pFifo->pFrmIDs[i] = UndefID;
    pFifo->pFrmStatus[i] = AL_NOT_NEEDED_FOR_OUTPUT;
  }

  pFifo->uFirstFrm = 0;
  pFifo->uNumFrm = 0;
}

static void DispFifo_Deinit(AL_TDispFifo* pFifo)
{
  (void)pFifo;
}

static bool DispFifo_IsEmpty(AL_TDispFifo* fifo)
{
  uint8_t uEnd = (fifo->uFirstFrm + fifo->uNumFrm) % FRM_BUF_POOL_SIZE;
  uint8_t uBegin = fifo->uFirstFrm;
  return uBegin == uEnd && fifo->uNumFrm != FRM_BUF_POOL_SIZE;
}

static void DispFifo_Queue(AL_TDispFifo* fifo, uint8_t uFrmID, uint32_t uPicLatency)
{
  uint8_t uID = fifo->uFirstFrm + fifo->uNumFrm++;

  fifo->pFrmIDs[uID % FRM_BUF_POOL_SIZE] = uFrmID;
  fifo->pPicLatency[uFrmID] = uPicLatency;
}

static uint8_t DispFifo_Peek(AL_TDispFifo* fifo)
{
  return fifo->pFrmIDs[fifo->uFirstFrm];
}

static uint8_t DispFifo_Dequeue(AL_TDispFifo* fifo)
{
  uint8_t uFrmID = fifo->pFrmIDs[fifo->uFirstFrm];

  if(uFrmID != UndefID)
  {
    fifo->pFrmIDs[fifo->uFirstFrm] = UndefID;
    fifo->uFirstFrm = (fifo->uFirstFrm + 1) % FRM_BUF_POOL_SIZE;
    fifo->uNumFrm--;
    fifo->pFrmStatus[uFrmID] = AL_NOT_NEEDED_FOR_OUTPUT;
  }
  return uFrmID;
}

static AL_EPicStatus DispFifo_GetStatus(AL_TDispFifo* fifo, uint8_t uFrmID)
{
  return fifo->pFrmStatus[uFrmID];
}

static void DispFifo_SetStatus(AL_TDispFifo* fifo, uint8_t uFrmID, AL_EPicStatus eStatus)
{
  fifo->pFrmStatus[uFrmID] = eStatus;
}

/*****************************************************************************/
static void AL_Dpb_sResetWaiting(AL_TDpb* pDpb)
{
  pDpb->uNodeWaiting = uEndOfList;
  pDpb->uFrmWaiting = UndefID;
  pDpb->uMvWaiting = UndefID;
  pDpb->bPicWaiting = false;
}

/*****************************************************************************/
static void AL_Dpb_sResetNodeInfo(AL_TDpb* pDpb, AL_TDpbNode* pNode)
{
  if(pNode->uNodeID == pDpb->uNodeWaiting)
    AL_Dpb_sResetWaiting(pDpb);

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

  pNode->iPic_num = INT32_MAX;
  pNode->iFrame_num_wrap = INT32_MAX;
  pNode->iLong_term_pic_num = INT32_MAX;
  pNode->iLong_term_frame_idx = INT32_MAX;
  pNode->iSlice_frame_num = INT32_MAX;
  pNode->non_existing = 0;
  pNode->eNUT = AL_HEVC_NUT_ERR;
}

/*****************************************************************************/
static void AL_Dpb_sReleasePicID(AL_TDpb* pDpb, uint8_t uPicID)
{
  if(uPicID != UndefID)
  {
    pDpb->PicId2NodeId[uPicID] = UndefID;
    pDpb->FreePicIDs[pDpb->FreePicIdCnt++] = uPicID;
  }
}

/*****************************************************************************/
static void AL_Dpb_sAddToDisplayList(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* node = &pDpb->Nodes[uNode];
  DispFifo_Queue(&pDpb->DispFifo, node->uFrmID, node->uPicLatency);

  AL_TDpbCallback* cb = &pDpb->tCallbacks;
  cb->pfnIncrementFrmBuf(cb->pUserParam, node->uFrmID);
  cb->pfnOutputFrmBuf(cb->pUserParam, node->uFrmID);
}

/*****************************************************************************/
static void AL_Dpb_sSlidingWindowMarking(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint32_t uNumShortTerm = 0;
  uint32_t uNumLongTerm = 0;
  int32_t iMinFrameNumWrap = INT32_MAX;
  uint8_t uPic = pDpb->uHeadDecOrder;
  uint8_t uPosMin = 0;
  AL_TDpbNode* pNodes = pDpb->Nodes;

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

  if((uNumShortTerm + uNumLongTerm) > UnsignedMin(pDpb->uNumRef, pSlice->pSPS->max_num_ref_frames))
  {
    pNodes[uPosMin].eMarking_flag = UNUSED_FOR_REF;
    --pDpb->uCountRef;

    if(pNodes[uPosMin].non_existing)
      AL_Dpb_Remove(pDpb, uPosMin);
    else
    {
      AL_Dpb_sReleasePicID(pDpb, pDpb->Nodes[uPosMin].uPicID);
      pDpb->Nodes[uPosMin].uPicID = UndefID;
    }
  }
}

/*****************************************************************************/
static void AL_Dpb_sSetPicToUnused(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;

  pNodes[uNode].eMarking_flag = UNUSED_FOR_REF;
  --pDpb->uCountRef;
  AL_Dpb_sReleasePicID(pDpb, pNodes[uNode].uPicID);
  pNodes[uNode].uPicID = UndefID;
}

/*****************************************************************************/
static void AL_Dpb_sShortTermToUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uCurPos = pDpb->uCurRef;
  uint8_t uPic = pDpb->uHeadDecOrder;
  int32_t iPicNumX = pNodes[uCurPos].iPic_num - (pSlice->difference_of_pic_nums_minus1[iIdx] + 1);

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
  uint8_t uPic = pDpb->uHeadDecOrder;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  int32_t iLong_pic_num = pSlice->long_term_pic_num[iIdx];

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
  uint8_t uNode = pDpb->uHeadDecOrder;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uPrev = pNodes[iRef].uPrevDecOrder;
  uint8_t uNext = pNodes[iRef].uNextDecOrder;

  while(uNode != uEndOfList)
  {
    if(pNodes[uNode].eMarking_flag == LONG_TERM_REF && uNode != iRef)
    {
      if(pNodes[uNode].iLong_term_pic_num > pNodes[iRef].iLong_term_pic_num)
      {
        // remove node from the ordered decoding list
        if(uPrev != uEndOfList)
          pNodes[uPrev].uNextDecOrder = uNext;

        if(uNext != uEndOfList)
          pNodes[uNext].uPrevDecOrder = uPrev;

        uint8_t uPrevNode = pNodes[uNode].uPrevDecOrder;

        // insert node in the ordered decoding list
        if(uPrevNode != uEndOfList)
          pNodes[uPrevNode].uNextDecOrder = iRef;

        pNodes[iRef].uPrevDecOrder = uPrevNode;
        pNodes[uNode].uPrevDecOrder = iRef;
        pNodes[iRef].uNextDecOrder = uNode;

        if(uNode == pDpb->uHeadDecOrder)
          pDpb->uHeadDecOrder = iRef;
        break;
      }
    }
    uNode = pNodes[uNode].uNextDecOrder;
  }
}

/*****************************************************************************/
static void AL_Dpb_sLongTermFrameIdxToAShortTerm(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx_long_term_frame, uint8_t iIdx_diff_pic_num)
{
  uint8_t uPic = pDpb->uHeadDecOrder;
  uint8_t uPic_num = uEndOfList;
  uint8_t uPic_frame = uEndOfList;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uCurPos = pDpb->uCurRef;
  uint32_t iDiffPicNum = pSlice->difference_of_pic_nums_minus1[iIdx_diff_pic_num] + 1;

  int32_t iLongTermFrameIdx = pSlice->long_term_frame_idx[iIdx_long_term_frame];
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
  uint8_t uPic = pDpb->uHeadDecOrder;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(uPic != uEndOfList)
  {
    if(pNodes[uPic].iLong_term_frame_idx < INT32_MAX &&
       pNodes[uPic].iLong_term_frame_idx > (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1) &&
       pNodes[uPic].eMarking_flag == LONG_TERM_REF)
    {
      AL_Dpb_sSetPicToUnused(pDpb, uPic);
    }
    uPic = pNodes[uPic].uNextDecOrder;
  }

  pDpb->MaxLongTermFrameIdx = pSlice->max_long_term_frame_idx_plus1[iIdx] ?
                              (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1) : INT32_MAX;
}

/*****************************************************************************/
static void AL_Dpb_sSetAllPicAsUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t iCurRef = pDpb->uCurRef;
  uint8_t uPic = pDpb->uHeadDecOrder;// pNodes[iCurRef].uPrevDecOrder;

  while(uPic != uEndOfList) /* remove all pictures from the DPB except the current picture */
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
  pDpb->MaxLongTermFrameIdx = INT32_MAX;

  if(pSlice->nal_ref_idc)
    AL_Dpb_SetMMCO5(pDpb);

  AL_Dpb_BeginNewSeq(pDpb);
}

/*****************************************************************************/
static void AL_Dpb_sAssignLongTermFrameIdx(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t iIdx)
{
  uint8_t uPic = pDpb->uHeadDecOrder;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uCurPos = pDpb->uCurRef;
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
static int32_t AL_Dpb_sPicNumF(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice, uint8_t uNodeID)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(uNodeID == uEndOfList)
    return iMaxFrameNum;

  return (pNodes[uNodeID].eMarking_flag == SHORT_TERM_REF) ? pNodes[uNodeID].iPic_num : iMaxFrameNum;
}

/*****************************************************************************/
static int32_t AL_Dpb_sLongTermPicNumF(AL_TDpb* pDpb, uint8_t uNodeID)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  int32_t iMaxLongTermFrameIdx = pDpb->MaxLongTermFrameIdx;

  if(uNodeID == uEndOfList) // undefined reference
    return 2 * (iMaxLongTermFrameIdx + 1);

  return (pNodes[uNodeID].eMarking_flag == LONG_TERM_REF) ? pNodes[uNodeID].iLong_term_pic_num : 2 * (iMaxLongTermFrameIdx + 1);
}

/*****************************************************************************/
static void AL_Dpb_sReleaseUnusedBuf(AL_TDpb* pDpb, bool bAll)
{
  int iNum = pDpb->iNumDltPic ? (bAll ? pDpb->iNumDltPic : 1) : 0;

  while(iNum--)
  {
    AL_TDpbCallback* cb = &pDpb->tCallbacks;
    cb->pfnDecrementFrmBuf(cb->pUserParam, pDpb->pDltFrmIDLst[pDpb->iDltFrmLstHead]);
    cb->pfnDecrementMvBuf(cb->pUserParam, pDpb->pDltMvIDLst[pDpb->iDltMvLstHead]);

    pDpb->iDltFrmLstHead = (pDpb->iDltFrmLstHead + 1) % FRM_BUF_POOL_SIZE;
    pDpb->iDltMvLstHead = (pDpb->iDltMvLstHead + 1) % MAX_DPB_SIZE;
    --pDpb->iNumDltPic;
  }
}

/*****************************************************************************/
static void AL_Dpb_sFillWaitingPicture(AL_TDpb* pDpb)
{
  uint8_t uPicID = pDpb->Nodes[pDpb->uNodeWaiting].non_existing ? uEndOfList : pDpb->FreePicIDs[--pDpb->FreePicIdCnt];

  pDpb->Nodes[pDpb->uNodeWaiting].uPicID = uPicID;

  if(uPicID != uEndOfList)
  {
    pDpb->PicId2NodeId[uPicID] = pDpb->uNodeWaiting;
    pDpb->PicId2FrmId[uPicID] = pDpb->uFrmWaiting;
    pDpb->PicId2MvId[uPicID] = pDpb->uMvWaiting;
  }

  pDpb->uNodeWaiting = uEndOfList;
  pDpb->uFrmWaiting = uEndOfList;
  pDpb->uMvWaiting = uEndOfList;
  pDpb->bPicWaiting = false;
}

/***************************************************************************/
/*                          D P B     f u n c t i o n s                    */
/***************************************************************************/

/*****************************************************************************/
void AL_Dpb_Init(AL_TDpb* pDpb, uint8_t uNumRef, AL_EDpbMode eMode, AL_TDpbCallback tCallbacks)
{
  for(int i = 0; i < PIC_ID_POOL_SIZE; i++)
  {
    pDpb->FreePicIDs[i] = i;
    pDpb->PicId2NodeId[i] = uEndOfList;
    pDpb->PicId2FrmId[i] = uEndOfList;
    pDpb->PicId2MvId[i] = uEndOfList;
  }

  pDpb->eMode = eMode;
  pDpb->FreePicIdCnt = PIC_ID_POOL_SIZE;

  pDpb->bNewSeq = true;

  AL_Dpb_sResetWaiting(pDpb);

  for(int i = 0; i < MAX_DPB_SIZE; i++)
  {
    pDpb->Nodes[i].uNodeID = UndefID;
    AL_Dpb_sResetNodeInfo(pDpb, &pDpb->Nodes[i]);
  }

  pDpb->uNumRef = uNumRef;

  pDpb->uHeadDecOrder = uEndOfList;
  pDpb->uHeadPOC = uEndOfList;
  pDpb->uLastPOC = uEndOfList;

  pDpb->uCountRef = 0;
  pDpb->uCountPic = 0;
  pDpb->uCurRef = 0;
  pDpb->uNumOutputPic = 0;
  pDpb->iLastDisplayedPOC = 0x80000000;

  pDpb->iDltFrmLstHead = 0;
  pDpb->iDltFrmLstTail = 0;
  pDpb->iDltMvLstHead = 0;
  pDpb->iDltMvLstTail = 0;
  pDpb->iNumDltPic = 0;

  pDpb->Mutex = Rtos_CreateMutex();

  DispFifo_Init(&pDpb->DispFifo);

  pDpb->tCallbacks = tCallbacks;
}

/*************************************************************************/
void AL_Dpb_Terminate(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);
  AL_Dpb_sReleaseUnusedBuf(pDpb, true);
  DispFifo_Deinit(&pDpb->DispFifo);
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_Deinit(AL_TDpb* pDpb)
{
  Rtos_DeleteMutex(pDpb->Mutex);
}

/*****************************************************************************/
uint8_t AL_Dpb_GetRefCount(AL_TDpb* pDpb)
{
  return pDpb->uCountRef;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetPicCount(AL_TDpb* pDpb)
{
  return pDpb->uCountPic;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetHeadPOC(AL_TDpb* pDpb)
{
  return pDpb->uHeadPOC;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetNextPOC(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].uNextPOC;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetOutputFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].pic_output_flag;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetNumOutputPict(AL_TDpb* pDpb)
{
  return pDpb->uNumOutputPic;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetMarkingFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].eMarking_flag;
}

/*************************************************************************/
uint32_t AL_Dpb_GetPicLatency_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].uPicLatency;
}

/*************************************************************************/
uint8_t AL_Dpb_GetPicID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  if(uNode == uEndOfList)
    return UndefID;

  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].uPicID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetMvID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  if(uNode == uEndOfList)
    return UndefID;

  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].uMvID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetFrmID_FromNode(AL_TDpb* pDpb, uint8_t uNode)
{
  if(uNode == uEndOfList)
    return UndefID;

  assert(uNode < MAX_DPB_SIZE);
  return pDpb->Nodes[uNode].uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetLastPicID(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uNode = pDpb->uHeadPOC;
  uint8_t uRetID;

  if(uNode == uEndOfList)
    uRetID = uNode;
  else
  {
    while(pDpb->Nodes[uNode].uNextPOC != uEndOfList)
      uNode = pDpb->Nodes[uNode].uNextPOC;

    uRetID = pDpb->Nodes[uNode].uPicID;
  }

  DPB_RELEASE_MUTEX(pDpb);

  return uRetID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetNumRef(AL_TDpb* pDpb)
{
  return pDpb->uNumRef;
}

/*************************************************************************/
void AL_Dpb_SetNumRef(AL_TDpb* pDpb, uint8_t uMaxRef)
{
  pDpb->uNumRef = uMaxRef;
}

/*************************************************************************/
void AL_Dpb_SetMarkingFlag(AL_TDpb* pDpb, uint8_t uNode, AL_EMarkingRef eMarkingFlag)
{
  pDpb->Nodes[uNode].eMarking_flag = eMarkingFlag;
}

/*************************************************************************/
void AL_Dpb_ResetOutputFlag(AL_TDpb* pDpb, uint8_t uNode)
{
  pDpb->Nodes[uNode].pic_output_flag = 0;
  --pDpb->uNumOutputPic;
}

/*************************************************************************/
void AL_Dpb_IncrementPicLatency(AL_TDpb* pDpb, uint8_t uNode, int iCurFramePOC)
{
  if(pDpb->Nodes[uNode].iFramePOC > iCurFramePOC)
    ++pDpb->Nodes[uNode].uPicLatency;
}

/*************************************************************************/
void AL_Dpb_DecrementPicLatency(AL_TDpb* pDpb, uint8_t uNode)
{
  --pDpb->Nodes[uNode].uPicLatency;
}

/*************************************************************************/
void AL_Dpb_BeginNewSeq(AL_TDpb* pDpb)
{
  pDpb->bNewSeq = true;
  pDpb->iLastDisplayedPOC = 0x80000000;
}

/*************************************************************************/
bool AL_Dpb_NodeIsReset(AL_TDpb* pDpb, uint8_t uNode)
{
  return pDpb->Nodes[uNode].bIsReset;
}

/*************************************************************************/
bool AL_Dpb_LastHasMMCO5(AL_TDpb* pDpb)
{
  return pDpb->bLastHasMMCO5;
}

/*************************************************************************/
void AL_Dpb_SetMMCO5(AL_TDpb* pDpb)
{
  pDpb->bLastHasMMCO5 = true;
}

/*************************************************************************/
void AL_Dpb_ResetMMCO5(AL_TDpb* pDpb)
{
  pDpb->bLastHasMMCO5 = false;
}

/*************************************************************************/
uint8_t AL_Dpb_ConvertPicIDToNodeID(AL_TDpb const* pDpb, uint8_t uPicID)
{
  return pDpb->PicId2NodeId[uPicID];
}

/*************************************************************************/
uint8_t AL_Dpb_GetNextFreeNode(AL_TDpb* pDpb)
{
  uint8_t uNew = 0;

  DPB_GET_MUTEX(pDpb);

  while(pDpb->Nodes[uNew].eMarking_flag != UNUSED_FOR_REF || pDpb->Nodes[uNew].pic_output_flag)
    uNew = (uNew + 1) % MAX_DPB_SIZE;

  DPB_RELEASE_MUTEX(pDpb);

  return uNew;
}

/*****************************************************************************/
void AL_Dpb_FillList(AL_TDpb* pDpb, uint8_t uL0L1, TBufferListRef const* pListRef, int* pPocList, uint32_t* pLongTermList)
{
  DPB_GET_MUTEX(pDpb);

  for(int i = 0; i < MAX_REF; ++i)
  {
    uint8_t uNodeID = (*pListRef)[uL0L1][i].uNodeID;

    if(uNodeID != uEndOfList && !pDpb->Nodes[uNodeID].non_existing)
    {
      uint8_t uPicID = pDpb->Nodes[uNodeID].uPicID;
      pPocList[uPicID] = pDpb->Nodes[uNodeID].iFramePOC;

      if(pDpb->Nodes[uNodeID].eMarking_flag == LONG_TERM_REF)
        *pLongTermList |= (1 << uPicID); // long term flag
      *pLongTermList |= ((uint32_t)1 << (16 + uPicID)); // available POC
    }
  }

  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
uint8_t AL_Dpb_SearchPocLsb(AL_TDpb* pDpb, uint32_t poc_lsb)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uParse = pDpb->uHeadPocLsb;

  while(uParse != uEndOfList)
  {
    if(pDpb->Nodes[uParse].slice_pic_order_cnt_lsb == poc_lsb && pDpb->Nodes[uParse].eMarking_flag != UNUSED_FOR_REF)
      break;
    else
      uParse = pDpb->Nodes[uParse].uNextPocLsb;
  }

  DPB_RELEASE_MUTEX(pDpb);
  return uParse;
}

/*****************************************************************************/
uint8_t AL_Dpb_SearchPOC(AL_TDpb* pDpb, int iPOC)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uParse = pDpb->uHeadPOC;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(uParse != uEndOfList)
  {
    if(pNodes[uParse].iFramePOC == iPOC && pNodes[uParse].eMarking_flag != UNUSED_FOR_REF && !pNodes[uParse].non_existing)
      break;
    else
      uParse = pDpb->Nodes[uParse].uNextPOC;
  }

  DPB_RELEASE_MUTEX(pDpb);
  return uParse;
}

/*****************************************************************************/
void AL_Dpb_Display(AL_TDpb* pDpb, uint8_t uNode)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uParse = pDpb->uHeadPOC;

  // check if there is anterior picture to be displayed present in the DPB
  while(uParse != uNode)
  {
    uint8_t uNext = pDpb->Nodes[uParse].uNextPOC;

    if(pDpb->Nodes[uParse].pic_output_flag)
      AL_Dpb_Display(pDpb, uParse);
    uParse = uNext;
  }

  // add current picture to display list
  if(pDpb->Nodes[uNode].pic_output_flag)
  {
    if(!pDpb->Nodes[uNode].non_existing)
    {
      AL_Dpb_sAddToDisplayList(pDpb, uNode);
      pDpb->iLastDisplayedPOC = pDpb->Nodes[uNode].iFramePOC;
    }

    pDpb->Nodes[uNode].bIsDisplayed = true;
    pDpb->Nodes[uNode].pic_output_flag = 0;
    pDpb->bNewSeq = false;
    --pDpb->uNumOutputPic;
  }
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
uint8_t AL_Dpb_GetDisplayBuffer(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uFrmID = UndefID;

  if(!DispFifo_IsEmpty(&pDpb->DispFifo))
  {
    uFrmID = DispFifo_Peek(&pDpb->DispFifo);

    if(DispFifo_GetStatus(&pDpb->DispFifo, uFrmID) != AL_READY_FOR_OUTPUT)
      uFrmID = UndefID;
  }

  DPB_RELEASE_MUTEX(pDpb);
  return uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_ReleaseDisplayBuffer(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);
  uint8_t uFrmID = DispFifo_Dequeue(&pDpb->DispFifo);
  AL_TDpbCallback* cb = &pDpb->tCallbacks;

  if(uFrmID != UndefID)
    cb->pfnDecrementFrmBuf(cb->pUserParam, uFrmID);
  DPB_RELEASE_MUTEX(pDpb);
  return uFrmID;
}

/*************************************************************************/
void AL_Dpb_ClearOutput(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);
  uint8_t uNode = pDpb->uHeadPOC;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(uNode != uEndOfList)
  {
    pNodes[uNode].pic_output_flag = 0;
    uNode = pNodes[uNode].uNextPOC;
  }

  pDpb->uNumOutputPic = 0;
  DPB_RELEASE_MUTEX(pDpb);
}

/*************************************************************************/
void AL_Dpb_Flush(AL_TDpb* pDpb)
{
  DPB_GET_MUTEX(pDpb);

  uint8_t uHeadPOC;

  while(uEndOfList != (uHeadPOC = pDpb->uHeadPOC))
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
  AL_TDpbNode* pNodes = pDpb->Nodes;
  DPB_GET_MUTEX(pDpb);

  uint8_t uNode = pDpb->uHeadPOC;

  while(uNode != uEndOfList)
  {
    bool bShouldBeDisplayed = AL_Dpb_GetOutputFlag(pDpb, uNode) && ((AL_Dpb_GetPicLatency_FromNode(pDpb, uNode) >= uMaxLatency) || (AL_Dpb_GetNumOutputPict(pDpb) > MaxNumOutput));

    if(bShouldBeDisplayed)
      AL_Dpb_Display(pDpb, uNode);

    if(pNodes[uNode].eMarking_flag == UNUSED_FOR_REF && !AL_Dpb_GetOutputFlag(pDpb, uNode))
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
  DPB_GET_MUTEX(pDpb);

  uint8_t uNode = pDpb->uHeadPOC;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(uNode != uEndOfList)
  {
    uint8_t uNextNode = pNodes[uNode].uNextPOC;

    // Remove useless pictures
    if(AL_Dpb_GetMarkingFlag(pDpb, uNode) == UNUSED_FOR_REF && !AL_Dpb_GetOutputFlag(pDpb, uNode))
      AL_Dpb_Remove(pDpb, uNode);

    uNode = uNextNode;
  }

  uNode = pDpb->uHeadPOC;

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
  if(pDpb->bPicWaiting && pDpb->FreePicIdCnt)
    AL_Dpb_sFillWaitingPicture(pDpb);

  DPB_RELEASE_MUTEX(pDpb);
}

// Remove from POC ordered linked list
static void RemoveFromPocList(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[uNode];
  uint8_t uPrev = pNode->uPrevPOC;
  uint8_t uNext = pNode->uNextPOC;

  if(pDpb->uHeadPOC == uNode)
    pDpb->uHeadPOC = uNext;

  if(uPrev != uEndOfList)
    pDpb->Nodes[uPrev].uNextPOC = uNext;

  if(uNext != uEndOfList)
    pDpb->Nodes[uNext].uPrevPOC = uPrev;
  else
    pDpb->uLastPOC = uPrev;
}

// Remove from poc lsb ordered linked list
static void RemoveFromPocLsbList(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[uNode];
  uint8_t uPrev = pNode->uPrevPocLsb;
  uint8_t uNext = pNode->uNextPocLsb;

  if(pDpb->uHeadPocLsb == uNode)
    pDpb->uHeadPocLsb = uNext;

  if(uPrev != uEndOfList)
    pDpb->Nodes[uPrev].uNextPocLsb = uNext;

  if(uNext != uEndOfList)
    pDpb->Nodes[uNext].uPrevPocLsb = uPrev;
}

// Remove from Decoding ordered linked list
static void RemoveFromDecOrderList(AL_TDpb* pDpb, uint8_t uNode)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[uNode];
  uint8_t uPrev = pNode->uPrevDecOrder;
  uint8_t uNext = pNode->uNextDecOrder;

  if(pDpb->uHeadDecOrder == uNode)
    pDpb->uHeadDecOrder = uNext;

  if(uPrev != uEndOfList)
    pDpb->Nodes[uPrev].uNextDecOrder = uNext;

  if(uNext != uEndOfList)
    pDpb->Nodes[uNext].uPrevDecOrder = uPrev;
}

/*************************************************************************/
uint8_t AL_Dpb_Remove(AL_TDpb* pDpb, uint8_t uNode)
{
  DPB_GET_MUTEX(pDpb);

  if(uNode == uEndOfList)
  {
    DPB_RELEASE_MUTEX(pDpb);
    return UndefID;
  }

  AL_TDpbNode* pNode = &pDpb->Nodes[uNode];

  uint8_t uMvID = pNode->uMvID;
  uint8_t uNonExisting = pNode->non_existing;
  uint8_t uFrmID = pNode->uFrmID;

  RemoveFromPocList(pDpb, uNode);
  RemoveFromPocLsbList(pDpb, uNode);
  RemoveFromDecOrderList(pDpb, uNode);

  // Release node
  AL_Dpb_sReleasePicID(pDpb, pNode->uPicID);

  // Update ref counter
  if(pNode->eMarking_flag != UNUSED_FOR_REF)
    --pDpb->uCountRef;
  --pDpb->uCountPic;

  // Reset node information
  AL_Dpb_sResetNodeInfo(pDpb, pNode);

  // assigned pic id to the awaiting picture
  if(pDpb->bPicWaiting && pDpb->FreePicIdCnt)
    AL_Dpb_sFillWaitingPicture(pDpb);

  if(!uNonExisting)
  {
    pDpb->pDltFrmIDLst[pDpb->iDltFrmLstTail] = uFrmID;
    pDpb->pDltMvIDLst[pDpb->iDltMvLstTail] = uMvID;

    pDpb->iDltFrmLstTail = (pDpb->iDltFrmLstTail + 1) % FRM_BUF_POOL_SIZE;
    pDpb->iDltMvLstTail = (pDpb->iDltMvLstTail + 1) % MAX_DPB_SIZE;

    ++pDpb->iNumDltPic;
  }

  DPB_RELEASE_MUTEX(pDpb);

  return uFrmID;
}

/*************************************************************************/
uint8_t AL_Dpb_RemoveHead(AL_TDpb* pDpb)
{
  // Remove header of the Decoding ordered linked list
  if(AL_Dpb_GetOutputFlag(pDpb, pDpb->uHeadPOC))
    AL_Dpb_Display(pDpb, pDpb->uHeadPOC);
  return AL_Dpb_Remove(pDpb, pDpb->uHeadPOC);
}

/*****************************************************************************/
void AL_Dpb_Insert(AL_TDpb* pDpb, int iFramePOC, uint32_t uPocLsb, uint8_t uNode, uint8_t uFrmID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT)
{
  uint8_t uPicID = uEndOfList;

  DPB_GET_MUTEX(pDpb);

  // Assign PicID
  if(!uNonExisting)
  {
    if(pDpb->FreePicIdCnt)
    {
      uPicID = pDpb->FreePicIDs[--pDpb->FreePicIdCnt];
      pDpb->PicId2NodeId[uPicID] = uNode;
      pDpb->PicId2FrmId[uPicID] = uFrmID;
      pDpb->PicId2MvId[uPicID] = uMvID;
    }
    else
    {
      pDpb->bPicWaiting = true;
      pDpb->uNodeWaiting = uNode;
      pDpb->uFrmWaiting = uFrmID;
      pDpb->uMvWaiting = uMvID;
    }
  }

  pDpb->uCurRef = uNode;
  AL_TDpbNode* pNode = &pDpb->Nodes[uNode];

  // Assign frame buffer informations
  pNode->iFramePOC = iFramePOC;
  pNode->slice_pic_order_cnt_lsb = uPocLsb;
  pNode->uFrmID = uFrmID;
  pNode->uMvID = uMvID;
  pNode->uPicID = uPicID;
  pNode->eMarking_flag = eMarkingFlag;
  pNode->uNodeID = uNode;
  pNode->pic_output_flag = pic_output_flag;
  pNode->bIsReset = false;
  pNode->non_existing = uNonExisting;
  pNode->eNUT = eNUT;

  // Update frame status in display fifo list
  if(uFrmID != uEndOfList)
    DispFifo_SetStatus(&pDpb->DispFifo, uFrmID, pic_output_flag ? AL_NOT_READY_FOR_OUTPUT : AL_NOT_NEEDED_FOR_OUTPUT);

  // Insert it in the POC ordered linked list
  if(pDpb->uHeadPOC == uEndOfList)
  {
    pNode->uPrevPOC = uEndOfList;
    pNode->uNextPOC = uEndOfList;
    pDpb->uHeadPOC = uNode;
    pDpb->uLastPOC = uNode;

    pNode->uPrevPocLsb = uEndOfList;
    pNode->uNextPocLsb = uEndOfList;
    pDpb->uHeadPocLsb = uNode;

    pNode->uPrevDecOrder = uEndOfList;
    pNode->uNextDecOrder = uEndOfList;
    pDpb->uHeadDecOrder = uNode;
  }
  else
  {
    uint8_t uCurPOC = pDpb->uHeadPOC;
    uint8_t uCurPocLsb = pDpb->uHeadPocLsb;
    uint8_t uCurDecOrder = pDpb->uHeadDecOrder;

    while(true)
    {
      // compute poc ordered list
      if(pDpb->Nodes[uCurPOC].iFramePOC > iFramePOC)
      {
        uint8_t uPrev = pDpb->Nodes[uCurPOC].uPrevPOC;

        pNode->uPrevPOC = uPrev;
        pNode->uNextPOC = uCurPOC;
        pDpb->Nodes[uCurPOC].uPrevPOC = uNode;

        if(uPrev != uEndOfList)
          pDpb->Nodes[uPrev].uNextPOC = uNode;

        if(uCurPOC == pDpb->uHeadPOC)
          pDpb->uHeadPOC = uNode;
        break;
      }
      else if(pDpb->Nodes[uCurPOC].uNextPOC == uEndOfList)
      {
        pNode->uNextPOC = uEndOfList;
        pNode->uPrevPOC = uCurPOC;
        pDpb->Nodes[uCurPOC].uNextPOC = uNode;

        pDpb->uLastPOC = uNode;
        break;
      }
      else
        uCurPOC = pDpb->Nodes[uCurPOC].uNextPOC;
    }

    while(true)
    {
      // compute poc lsb ordered list
      AL_TDpbNode* pNodeCurPocLsb = &pDpb->Nodes[uCurPocLsb];

      if(pNodeCurPocLsb->slice_pic_order_cnt_lsb > uPocLsb)
      {
        uint8_t uPrev = pNodeCurPocLsb->uPrevPocLsb;
        pNode->uPrevPocLsb = uPrev;
        pNode->uNextPocLsb = uCurPocLsb;
        pNodeCurPocLsb->uPrevPocLsb = uNode;

        if(uPrev != uEndOfList)
          pDpb->Nodes[uPrev].uNextPocLsb = uNode;

        if(uCurPocLsb == pDpb->uHeadPocLsb)
          pDpb->uHeadPocLsb = uNode;
        break;
      }
      else if(pNodeCurPocLsb->uNextPocLsb == uEndOfList)
      {
        pNode->uNextPocLsb = uEndOfList;
        pNode->uPrevPocLsb = uCurPocLsb;
        pNodeCurPocLsb->uNextPocLsb = uNode;
        break;
      }
      else
        uCurPocLsb = pNodeCurPocLsb->uNextPocLsb;
    }

    // Decoding order linked list
    while(pDpb->Nodes[uCurDecOrder].uNextDecOrder != uEndOfList)
      uCurDecOrder = pDpb->Nodes[uCurDecOrder].uNextDecOrder;

    pNode->uPrevDecOrder = uCurDecOrder;
    pNode->uNextDecOrder = uEndOfList;
    pDpb->Nodes[uCurDecOrder].uNextDecOrder = uNode;
  }

  // Update List counters
  if(eMarkingFlag != UNUSED_FOR_REF)
    ++pDpb->uCountRef;
  ++pDpb->uCountPic;

  if(pic_output_flag)
    ++pDpb->uNumOutputPic;

  AL_TDpbCallback* cb = &pDpb->tCallbacks;

  if(uFrmID != uEndOfList)
    cb->pfnIncrementFrmBuf(cb->pUserParam, uFrmID);

  if(uMvID != uEndOfList)
    cb->pfnIncrementMvBuf(cb->pUserParam, uMvID);
  DPB_RELEASE_MUTEX(pDpb);
}

/*****************************************************************************/
static bool AL_Dpb_sIsNoReordering(AL_TDpb* pDpb)
{
  return pDpb->eMode == AL_DPB_NO_REORDERING;
}

/*****************************************************************************/
static uint8_t Dpb_GetNodeFromFrmID(AL_TDpb* pDpb, int iFrameID)
{
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  uint8_t uNode = pDpb->uHeadDecOrder;

  while(uNode != uEndOfList)
  {
    if(pNodes[uNode].uFrmID == iFrameID)
      break;
    uNode = pNodes[uNode].uNextDecOrder;
  }

  return uNode;
}

/*****************************************************************************/
void AL_Dpb_EndDecoding(AL_TDpb* pDpb, int iFrmID)
{
  DPB_GET_MUTEX(pDpb);

  AL_Dpb_sReleaseUnusedBuf(pDpb, false);

  // Set Frame Availability
  if(DispFifo_GetStatus(&pDpb->DispFifo, iFrmID) == AL_NOT_READY_FOR_OUTPUT)
  {
    DispFifo_SetStatus(&pDpb->DispFifo, iFrmID, AL_READY_FOR_OUTPUT);

    if(AL_Dpb_sIsNoReordering(pDpb))
    {
      uint8_t uNode = Dpb_GetNodeFromFrmID(pDpb, iFrmID);
      bool isInDisplayList = (uNode == uEndOfList);

      if(!isInDisplayList && AL_Dpb_GetOutputFlag(pDpb, uNode))
        AL_Dpb_Display(pDpb, uNode);
    }
  }

  DPB_RELEASE_MUTEX(pDpb);
}

/*****************************************************************************/
void AL_Dpb_PictNumberProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr* pSlice)
{
  uint8_t uPicID = pDpb->uHeadDecOrder;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint32_t uMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

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
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uCurPos = pDpb->uCurRef;

  // fill node's parameters
  pNodes[uCurPos].iSlice_frame_num = pSlice->frame_num;
  pNodes[uCurPos].uNodeID = uCurPos;

  if(pSlice->nal_unit_type == 5) /*IDR picture case*/
  {
    if(!pSlice->long_term_reference_flag)
      pDpb->MaxLongTermFrameIdx = INT32_MAX;
    else
    {
      pNodes[uCurPos].iLong_term_frame_idx = 0;
      pDpb->MaxLongTermFrameIdx = 0;
    }
  }
  else /*non IDR picture case*/
  {
    AL_Dpb_PictNumberProcess(pDpb, pSlice);

    if(pSlice->adaptive_ref_pic_marking_mode_flag)
      AL_Dpb_sAdaptiveMemoryControlMarking(pDpb, pSlice);

    AL_Dpb_sSlidingWindowMarking(pDpb, pSlice);

    for(int op_idc = 0; op_idc < 32; ++op_idc)
    {
      if(pSlice->memory_management_control_operation[op_idc] == 6 &&
         pNodes[uCurPos].eMarking_flag != LONG_TERM_REF)
      {
        pNodes[uCurPos].eMarking_flag = SHORT_TERM_REF;
        pNodes[uCurPos].iLong_term_frame_idx = INT32_MAX;
        break;
      }
    }
  }
}

/*****************************************************************************/
void AL_Dpb_InitPSlice_RefList(AL_TDpb* pDpb, TBufferRef* pRefList)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TDpbNode NodeShortTerm[16], NodeLongTerm[16];
  AL_TDpbNode* pNodeTemp;

  Rtos_Memset(&NodeShortTerm[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, 16 * sizeof(AL_TDpbNode));

  int iCnt_short = MAX_REF - 1;
  int iCnt_long = 0;
  uint8_t uNode = pDpb->uHeadDecOrder;

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
  int iPic = 0;
  int iRef = iCnt_short + 1;
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
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TDpbNode NodeShortTermPOCGreat[16], NodeShortTermPOCLess[16], NodeLongTerm[16];
  AL_TDpbNode* pNodeShort;
  AL_TDpbNode NodeTemp;

  Rtos_Memset(&NodeShortTermPOCGreat[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeShortTermPOCLess[0], 0, 16 * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, 16 * sizeof(AL_TDpbNode));

  int iCnt_short_less = MAX_REF - 1;
  int iCnt_short_great = 0;
  uint8_t uShortNode = pDpb->uHeadPOC;

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

  int iCnt_long = 0;
  uint8_t uLongNode = pDpb->uHeadDecOrder;

  while(uLongNode != uEndOfList) /*long_term_reference case*/
  {
    if(pNodes[uLongNode].eMarking_flag == LONG_TERM_REF)
      NodeLongTerm[iCnt_long++] = pNodes[uLongNode];
    uLongNode = pNodes[uLongNode].uNextDecOrder;
  }

  int iPic = 0;
  int iRef = iCnt_short_less + 1;
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
  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);
  int iDiffPicNum = iL0L1 ? pSlice->abs_diff_pic_num_minus1_l1[uOffset] + 1 :
                    pSlice->abs_diff_pic_num_minus1_l0[uOffset] + 1;

  uint8_t uNumRef = iL0L1 ? pSlice->num_ref_idx_l1_active_minus1 + 1 :
                    pSlice->num_ref_idx_l0_active_minus1 + 1;

  AL_TDpbNode* pNodes = pDpb->Nodes;

  int iPicNumNoWrap;

  if(!iPicNumIdc)
    iPicNumNoWrap = ((*pPicNumPred - iDiffPicNum) < 0) ?
                    *pPicNumPred - iDiffPicNum + iMaxFrameNum : *pPicNumPred - iDiffPicNum + 0;
  else
    iPicNumNoWrap = ((*pPicNumPred + iDiffPicNum) >= iMaxFrameNum) ?
                    *pPicNumPred + iDiffPicNum - iMaxFrameNum : *pPicNumPred + iDiffPicNum + 0;

  *pPicNumPred = iPicNumNoWrap;

  /*warning : only frame slice*/
  int32_t iPicNum = (iPicNumNoWrap > pSlice->frame_num) ? iPicNumNoWrap - iMaxFrameNum : iPicNumNoWrap - 0;

  /*process reordering*/
  for(uint8_t u = uNumRef; u > *pRefIdx; --u)
    (*pListRef)[iL0L1][u] = (*pListRef)[iL0L1][u - 1];

  uint8_t uCpt = pDpb->uHeadDecOrder;

  while(uCpt != uEndOfList)
  {
    if(pNodes[uCpt].iPic_num == iPicNum && pNodes[uCpt].eMarking_flag == SHORT_TERM_REF)
    {
      (*pListRef)[iL0L1][(*pRefIdx)++].uNodeID = pNodes[uCpt].uNodeID;

      uint8_t unIdx = *pRefIdx;

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
  uint8_t uNumRef = iL0L1 ? pSlice->num_ref_idx_l1_active_minus1 + 1 : pSlice->num_ref_idx_l0_active_minus1 + 1;

  for(uint8_t u = uNumRef; u > *pRefIdx; --u)
    (*pListRef)[iL0L1][u] = (*pListRef)[iL0L1][u - 1];

  int32_t iLongTermPicNum = iL0L1 ? pSlice->long_term_pic_num_l1[uOffset] : pSlice->long_term_pic_num_l0[uOffset];
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint8_t uCpt = pDpb->uHeadDecOrder;

  while(uCpt != uEndOfList)
  {
    if(pNodes[uCpt].iLong_term_pic_num == iLongTermPicNum && pNodes[uCpt].eMarking_flag == LONG_TERM_REF)
    {
      (*pListRef)[iL0L1][(*pRefIdx)++].uNodeID = pNodes[uCpt].uNodeID;

      uint8_t unIdx = *pRefIdx;

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

