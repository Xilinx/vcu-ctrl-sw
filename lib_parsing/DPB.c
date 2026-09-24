// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "DPB.h"
#include "lib_common/Round.h"
#include "lib_common/Index.h"
#include "lib_common/Arithmetics.h"

static void DispFifo_Init(AL_TDispFifo* pFifo)
{
  for(int32_t i = 0; i < AL_REFMNGR_MAX_POOL_SIZE; ++i)
  {
    pFifo->pFrmIDs[i] = AL_BAD_INDEX; // Conceptual problem. It should be AL_BAD_INDEX but recursive inclusion
    pFifo->pFrmStatus[i] = AL_NOT_NEEDED_FOR_OUTPUT;
  }

  pFifo->tFirstFrmID = 0;
  pFifo->uNumFrm = 0;
}

static void DispFifo_Deinit(AL_TDispFifo* pFifo)
{
  (void)pFifo;
}

static bool DispFifo_IsEmpty(AL_TDispFifo const* fifo)
{
  uint8_t uEnd = (fifo->tFirstFrmID + fifo->uNumFrm) % AL_REFMNGR_MAX_POOL_SIZE;
  uint8_t uBegin = fifo->tFirstFrmID;
  return uBegin == uEnd && fifo->uNumFrm != AL_REFMNGR_MAX_POOL_SIZE;
}

static void DispFifo_Queue(AL_TDispFifo* fifo, AL_TIndex tFrameID, uint32_t uPicLatency)
{
  uint8_t uID = fifo->tFirstFrmID + fifo->uNumFrm++;

  fifo->pFrmIDs[uID % AL_REFMNGR_MAX_POOL_SIZE] = tFrameID;
  fifo->pPicLatency[tFrameID] = uPicLatency;
}

static AL_TIndex DispFifo_Peek(AL_TDispFifo const* fifo)
{
  return fifo->pFrmIDs[fifo->tFirstFrmID];
}

static AL_TIndex DispFifo_Dequeue(AL_TDispFifo* fifo)
{
  AL_TIndex tFrameID = fifo->pFrmIDs[fifo->tFirstFrmID];

  if(AL_IS_VALID_INDEX(tFrameID))
  {
    fifo->pFrmIDs[fifo->tFirstFrmID] = AL_BAD_INDEX;
    fifo->tFirstFrmID = (fifo->tFirstFrmID + 1) % AL_REFMNGR_MAX_POOL_SIZE;
    fifo->uNumFrm--;
    fifo->pFrmStatus[tFrameID] = AL_NOT_NEEDED_FOR_OUTPUT;
  }
  return tFrameID;
}

static AL_EPicStatus DispFifo_GetStatus(AL_TDispFifo const* fifo, AL_TIndex tFrameID)
{
  return fifo->pFrmStatus[tFrameID];
}

static void DispFifo_SetStatus(AL_TDispFifo* fifo, AL_TIndex tFrameID, AL_EPicStatus eStatus)
{
  fifo->pFrmStatus[tFrameID] = eStatus;
}

/*****************************************************************************/
static void AL_Dpb_sResetWaiting(AL_TDpb* pDpb)
{
  pDpb->tNodeWaitingID = AL_BAD_INDEX;
  pDpb->tFrmWaitingID = AL_BAD_INDEX;
  pDpb->tMvWaitingID = AL_BAD_INDEX;
  pDpb->bPicWaiting = false;
}

/*****************************************************************************/
static void AL_Dpb_sResetNodeInfo(AL_TDpb* pDpb, AL_TDpbNode* pNode)
{
  if(pNode->tNodeID == pDpb->tNodeWaitingID)
    AL_Dpb_sResetWaiting(pDpb);

  pNode->iFramePOC = 0xFFFFFFFF;
  pNode->slice_pic_order_cnt_lsb = 0xFFFFFFFF;
  pNode->tNodeID = AL_BAD_INDEX;
  pNode->tPrevPOCNodeID = AL_BAD_INDEX;
  pNode->tNextPOCNodeID = AL_BAD_INDEX;
  pNode->tPrevPocLsbNodeID = AL_BAD_INDEX;
  pNode->tNextPocLsbNodeID = AL_BAD_INDEX;
  pNode->tPrevDecOrderNodeID = AL_BAD_INDEX;
  pNode->tNextDecOrderNodeID = AL_BAD_INDEX;
  pNode->tFrameID = AL_BAD_INDEX;
  pNode->tMvID = AL_BAD_INDEX;
  pNode->eMarkingFlag = UNUSED_FOR_REF;
  pNode->bIsReset = true;
  pNode->bPicOutputFlag = false;
  pNode->bIsDisplayed = false;
  pNode->uPicLatency = 0;

  pNode->iPicNum = INT32_MAX;
  pNode->iFrameNumberWrap = INT32_MAX;
  pNode->iLongTermPicNum = INT32_MAX;
  pNode->iLongTermFrameIdx = INT32_MAX;
  pNode->iSliceFrameNumber = INT32_MAX;
  pNode->bNonExisting = false;
  pNode->eNUT = AL_NUT_ERR;
}

/*****************************************************************************/
static void AL_Dpb_sReleasePicID(AL_TDpb* pDpb, AL_TIndex tPicID)
{
  if(AL_IS_VALID_INDEX(tPicID))
  {
    pDpb->PicId2NodeId[tPicID] = AL_BAD_INDEX;
    pDpb->FreePicIDs[pDpb->FreePicIdCnt++] = tPicID;
  }
}

/*****************************************************************************/
static void AL_Dpb_sAddToDisplayList(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode* node = &pDpb->Nodes[tNodeID];
  DispFifo_Queue(&pDpb->DispFifo, node->tFrameID, node->uPicLatency);

  AL_TPictMngrCallbacks* cb = &pDpb->tCallbacks;
  cb->pfnIncrementFrmBuf(cb->pUserParam, node->tFrameID);
  cb->pfnOutputFrmBuf(cb->pUserParam, node->tFrameID);
}

/*************************************************************************/
static void AL_Dpb_sBeginNewSeq(AL_TDpb* pDpb)
{
  pDpb->iLastDisplayedPOC = 0x80000000;
}

#include "lib_common/AvcUtils.h"
/*****************************************************************************/
static void AL_Dpb_sSlidingWindowMarking(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;
  AL_TIndex tMinPicId = 0;
  int32_t iMinFrameNumWrap = INT32_MAX;
  uint32_t uNumShortTerm = 0;
  uint32_t uNumLongTerm = 0;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(IS_NODE_VALID(tPicID))
  {
    if(pNodes[tPicID].eMarkingFlag == SHORT_TERM_REF)
      ++uNumShortTerm;
    else if(pNodes[tPicID].eMarkingFlag == LONG_TERM_REF)
      ++uNumLongTerm;

    if((pNodes[tPicID].iFrameNumberWrap < iMinFrameNumWrap) && (pNodes[tPicID].eMarkingFlag == SHORT_TERM_REF))
    {
      iMinFrameNumWrap = pNodes[tPicID].iFrameNumberWrap;
      tMinPicId = tPicID;
    }
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }

  if((uNumShortTerm + uNumLongTerm) > UnsignedMin(pDpb->uNumRef, pSlice->pSPS->max_num_ref_frames * (1 + pSlice->field_pic_flag)))
  {
    pNodes[tMinPicId].eMarkingFlag = UNUSED_FOR_REF;
    --pDpb->uCountRef;

    if(pNodes[tMinPicId].bNonExisting)
      AL_Dpb_Remove(pDpb, tMinPicId);
    else
    {
      AL_Dpb_sReleasePicID(pDpb, pDpb->Nodes[tMinPicId].tPicID);
      pDpb->Nodes[tMinPicId].tPicID = AL_BAD_INDEX;
    }
  }
}

/*****************************************************************************/
static void AL_Dpb_sSetPicToUnused(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;

  pNodes[tNodeID].eMarkingFlag = UNUSED_FOR_REF;
  --pDpb->uCountRef;
  AL_Dpb_sReleasePicID(pDpb, pNodes[tNodeID].tPicID);
  pNodes[tNodeID].tPicID = AL_BAD_INDEX;
}

/*****************************************************************************/
static void AL_Dpb_sShortTermToUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t iIdx)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TIndex tCurNodeID = pDpb->tCurRefNodeID;
  int32_t iPicNumX = pNodes[tCurNodeID].iPicNum - (pSlice->difference_of_pic_nums_minus1[iIdx] + 1);
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tPicID))
  {
    if(pNodes[tPicID].iPicNum == iPicNumX)
    {
      if(pNodes[tPicID].eMarkingFlag == SHORT_TERM_REF)
      {
        AL_Dpb_sSetPicToUnused(pDpb, tPicID);
        break;
      }
    }
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }
}

/*****************************************************************************/
/*8.2.5.4.2*/
static void AL_Dpb_sLongTermToUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t iIdx)
{
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  int32_t iLong_pic_num = pSlice->long_term_pic_num[iIdx];

  while(IS_NODE_VALID(tPicID))
  {
    if(pNodes[tPicID].iLongTermPicNum == iLong_pic_num)
    {
      if(pNodes[tPicID].eMarkingFlag == LONG_TERM_REF)
        AL_Dpb_sSetPicToUnused(pDpb, tPicID);
    }
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }
}

/*****************************************************************************/
static void AL_Dpb_sSwitchLongTermPlace(AL_TDpb* pDpb, AL_TIndex tRefID)
{
  AL_TIndex tNodeID = pDpb->tHeadDecOrderNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TIndex tPrevNodeID = pNodes[tRefID].tPrevDecOrderNodeID;
  AL_TIndex tNextNodeID = pNodes[tRefID].tNextDecOrderNodeID;

  while(IS_NODE_VALID(tNodeID))
  {
    if((pNodes[tNodeID].eMarkingFlag == LONG_TERM_REF) && (tNodeID != tRefID))
    {
      if(pNodes[tNodeID].iLongTermPicNum > pNodes[tRefID].iLongTermPicNum)
      {
        // remove node from the ordered decoding list
        if(IS_NODE_VALID(tPrevNodeID))
          pNodes[tPrevNodeID].tNextDecOrderNodeID = tNextNodeID;

        if(IS_NODE_VALID(tNextNodeID))
          pNodes[tNextNodeID].tPrevDecOrderNodeID = tPrevNodeID;

        AL_TIndex tPrevNode = pNodes[tNodeID].tPrevDecOrderNodeID;

        // insert node in the ordered decoding list
        if(IS_NODE_VALID(tPrevNode))
          pNodes[tPrevNode].tNextDecOrderNodeID = tRefID;

        pNodes[tRefID].tPrevDecOrderNodeID = tPrevNode;
        pNodes[tNodeID].tPrevDecOrderNodeID = tRefID;
        pNodes[tRefID].tNextDecOrderNodeID = tNodeID;

        if(tNodeID == pDpb->tHeadDecOrderNodeID)
          pDpb->tHeadDecOrderNodeID = tRefID;
        break;
      }
    }
    tNodeID = pNodes[tNodeID].tNextDecOrderNodeID;
  }
}

/*****************************************************************************/
static void AL_Dpb_sLongTermFrameIdxToAShortTerm(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t iIdx_long_term_frame, uint8_t iIdx_diff_pic_num)
{
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;
  AL_TIndex tNumPicID = AL_BAD_INDEX;
  AL_TIndex tFramePicID = AL_BAD_INDEX;
  AL_TIndex tCurRefNodeID = pDpb->tCurRefNodeID;
  uint32_t iDiffPicNum = pSlice->difference_of_pic_nums_minus1[iIdx_diff_pic_num] + 1;
  int32_t iLongTermFrameIdx = pSlice->long_term_frame_idx[iIdx_long_term_frame];
  AL_TDpbNode* pNodes = pDpb->Nodes;
  int32_t iPicNumX = pNodes[tCurRefNodeID].iPicNum - iDiffPicNum;

  while(IS_NODE_VALID(tPicID))
  {
    if(pNodes[tPicID].iLongTermFrameIdx == iLongTermFrameIdx)
      tFramePicID = tPicID;

    if(pNodes[tPicID].iPicNum == iPicNumX)
      tNumPicID = tPicID;
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }

  if(IS_NODE_VALID(tFramePicID) && IS_NODE_VALID(tNumPicID) && tFramePicID != tNumPicID)
  {
    if(pNodes[tFramePicID].eMarkingFlag != UNUSED_FOR_REF)
      AL_Dpb_sSetPicToUnused(pDpb, tFramePicID);
  }

  if(IS_NODE_VALID(tNumPicID))
  {
    pNodes[tNumPicID].eMarkingFlag = LONG_TERM_REF;
    pNodes[tNumPicID].iLongTermFrameIdx = iLongTermFrameIdx;
    pNodes[tNumPicID].iLongTermPicNum = iLongTermFrameIdx;
    AL_Dpb_sSwitchLongTermPlace(pDpb, tNumPicID);
  }
}

/*****************************************************************************/
static void AL_Dpb_sDecodingMaxLongTermFrameIdx(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t iIdx)
{
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(IS_NODE_VALID(tPicID))
  {
    if((pNodes[tPicID].iLongTermFrameIdx < INT32_MAX)
       && (pNodes[tPicID].iLongTermFrameIdx > (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1))
       && (pNodes[tPicID].eMarkingFlag == LONG_TERM_REF)
       )
      AL_Dpb_sSetPicToUnused(pDpb, tPicID);
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }

  pDpb->iMaxLongTermFrameIdx = pSlice->max_long_term_frame_idx_plus1[iIdx] ?
                               (pSlice->max_long_term_frame_idx_plus1[iIdx] - 1) : INT32_MAX;
}

/*****************************************************************************/
static void AL_Dpb_sSetAllPicAsUnused(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TIndex tCurRefNodeID = pDpb->tCurRefNodeID;
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;// pNodes[tCurRefNodeID].tPrevDecOrderNodeID;

  while(IS_NODE_VALID(tPicID)) /* remove all pictures from the DPB except the current picture */
  {
    AL_TIndex tNextNodeID = pNodes[tPicID].tNextDecOrderNodeID;

    if(tPicID != tCurRefNodeID)
    {
      if(!pNodes[tPicID].bNonExisting)
        AL_Dpb_Display(pDpb, tPicID);
      AL_Dpb_Remove(pDpb, tPicID);
    }
    tPicID = tNextNodeID;
  }

  pNodes[tCurRefNodeID].iFramePOC = 0;
  pNodes[tCurRefNodeID].iSliceFrameNumber = 0;
  pNodes[tCurRefNodeID].eNUT = AL_NUT_ERR;
  pDpb->iMaxLongTermFrameIdx = INT32_MAX;

  if(pSlice->nal_ref_idc)
    pDpb->bLastHasMMCO5 = true;

  AL_Dpb_sBeginNewSeq(pDpb);
}

/*****************************************************************************/
static void AL_Dpb_sAssignLongTermFrameIdx(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t iIdx, int32_t iCurFramePOC)
{
  AL_TIndex tPicID = pDpb->tHeadDecOrderNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TIndex tCurRefNodeID = pDpb->tCurRefNodeID;
  uint8_t iLongTermFrameIdx = pSlice->long_term_frame_idx[iIdx];
  (void)iCurFramePOC;

  while(IS_NODE_VALID(tPicID))
  {
    if(pNodes[tPicID].iLongTermFrameIdx == iLongTermFrameIdx)
    {
      if(pNodes[tPicID].eMarkingFlag == LONG_TERM_REF)
      {
        {
          AL_Dpb_sSetPicToUnused(pDpb, tPicID);
        }
      }
    }
    tPicID = pNodes[tPicID].tNextDecOrderNodeID;
  }

  pNodes[tCurRefNodeID].eMarkingFlag = LONG_TERM_REF;
  pNodes[tCurRefNodeID].iLongTermFrameIdx = iLongTermFrameIdx;
  pNodes[tCurRefNodeID].iLongTermPicNum = iLongTermFrameIdx;
  AL_Dpb_sSwitchLongTermPlace(pDpb, tCurRefNodeID);
}

/*****************************************************************************/
static void AL_Dpb_sAdaptiveMemoryControlMarking(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, int32_t iCurFramePOC)
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
      AL_Dpb_sAssignLongTermFrameIdx(pDpb, pSlice, idx3++, iCurFramePOC);
      break;

    default:
      break;
    }
  }
  while(pSlice->memory_management_control_operation[uMmcoIdx++] != 0);
}

/*****************************************************************************/
static int32_t AL_Dpb_sPicNumF(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, AL_TIndex tNodeID)
{
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(!IS_NODE_VALID(tNodeID))
    return iMaxFrameNum;

  return (pNodes[tNodeID].eMarkingFlag == SHORT_TERM_REF) ? pNodes[tNodeID].iPicNum : iMaxFrameNum;
}

/*****************************************************************************/
static int32_t AL_Dpb_sLongTermPicNumF(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  int32_t iMaxLongTermFrameIdx = pDpb->iMaxLongTermFrameIdx;

  if(!IS_NODE_VALID(tNodeID)) // undefined reference
    return 2 * (iMaxLongTermFrameIdx + 1);

  return (pNodes[tNodeID].eMarkingFlag == LONG_TERM_REF) ? pNodes[tNodeID].iLongTermPicNum : 2 * (iMaxLongTermFrameIdx + 1);
}

/*****************************************************************************/
static void AL_Dpb_sReleaseUnusedBuf(AL_TDpb* pDpb)
{
  if(pDpb->iNumDeletedPic == 0)
    return;

  int32_t iNum = pDpb->iNumDeletedPic;

  while(iNum--)
  {
    AL_TPictMngrCallbacks* cb = &pDpb->tCallbacks;
    cb->pfnDecrementFrmBuf(cb->pUserParam, pDpb->pDeletedFrmIDLst[pDpb->iDeletedFrmLstHead]);
    cb->pfnDecrementAnnexBuf(cb->pUserParam, pDpb->pDeletedMvIDLst[pDpb->iDeletedMvLstHead]);

    pDpb->iDeletedFrmLstHead = (pDpb->iDeletedFrmLstHead + 1) % AL_REFMNGR_MAX_POOL_SIZE;
    pDpb->iDeletedMvLstHead = (pDpb->iDeletedMvLstHead + 1) % AL_REF_MNGR_MAX_BUF_SIZE;
    --pDpb->iNumDeletedPic;
  }
}

/*****************************************************************************/
static void AL_Dpb_sFillWaitingPicture(AL_TDpb* pDpb)
{
  AL_TIndex tPicID = pDpb->Nodes[pDpb->tNodeWaitingID].bNonExisting ? AL_BAD_INDEX : pDpb->FreePicIDs[--pDpb->FreePicIdCnt];

  pDpb->Nodes[pDpb->tNodeWaitingID].tPicID = tPicID;

  if(IS_NODE_VALID(tPicID))
  {
    pDpb->PicId2NodeId[tPicID] = pDpb->tNodeWaitingID;
    pDpb->PicId2FrmId[tPicID] = pDpb->tFrmWaitingID;
    pDpb->PicId2MvId[tPicID] = pDpb->tMvWaitingID;
  }

  pDpb->tNodeWaitingID = AL_BAD_INDEX;
  pDpb->tFrmWaitingID = AL_BAD_INDEX;
  pDpb->tMvWaitingID = AL_BAD_INDEX;
  pDpb->bPicWaiting = false;
}

/***************************************************************************/
/*                          D P B     f u n c t i o n s                    */
/***************************************************************************/

/*****************************************************************************/
static bool sApiInit(AL_IReferenceManager* pICtx, AL_TPictMngrCallbacks* pCallbacks)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;

  pDpb->Mutex = Rtos_CreateMutex();

  if(NULL == pDpb->Mutex)
    return false;

  for(int32_t i = 0; i < AL_MAX_REF; i++)
  {
    pDpb->FreePicIDs[i] = i;
    pDpb->PicId2NodeId[i] = AL_BAD_INDEX;
    pDpb->PicId2FrmId[i] = AL_BAD_INDEX;
    pDpb->PicId2MvId[i] = AL_BAD_INDEX;
  }

  pDpb->FreePicIdCnt = AL_MAX_REF;

  AL_Dpb_sResetWaiting(pDpb);

  for(int32_t i = 0; i < AL_REF_MNGR_MAX_BUF_SIZE; i++)
  {
    pDpb->Nodes[i].tNodeID = AL_BAD_INDEX;
    AL_Dpb_sResetNodeInfo(pDpb, &pDpb->Nodes[i]);
  }

  pDpb->tHeadDecOrderNodeID = AL_BAD_INDEX;
  pDpb->tHeadPOCNodeID = AL_BAD_INDEX;
  pDpb->tLastPOCNodeID = AL_BAD_INDEX;
  pDpb->tHeadPocLsbNodeID = AL_BAD_INDEX;

  pDpb->uCountRef = 0;
  pDpb->uCountPic = 0;
  pDpb->tCurRefNodeID = 0;
  pDpb->uNumOutputPic = 0;
  pDpb->iLastDisplayedPOC = 0x80000000;

  pDpb->iDeletedFrmLstHead = 0;
  pDpb->iDeletedFrmLstTail = 0;
  pDpb->iDeletedMvLstHead = 0;
  pDpb->iDeletedMvLstTail = 0;
  pDpb->iNumDeletedPic = 0;

  pDpb->iCurFramePOC = 0;
  pDpb->iPrevPocLSB = 0;
  pDpb->iPrevPocMSB = 0;
  pDpb->iPrevPOC = 0;
  pDpb->iPrevFrameNum = -1;

  DispFifo_Init(&pDpb->DispFifo);

  pDpb->tCallbacks = *pCallbacks;
  return true;
}

/*************************************************************************/
void sApiTerminate(AL_IReferenceManager* pICtx)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);
  AL_Dpb_sReleaseUnusedBuf(pDpb);
  DispFifo_Deinit(&pDpb->DispFifo);
  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
static void sApiDeinit(AL_IReferenceManager* pICtx)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_DeleteMutex(pDpb->Mutex);
  Rtos_Free(pDpb);
}

/*****************************************************************************/
static bool sIsEmpty(AL_TDpb const* pDpb)
{
  return pDpb->uCountPic == 0;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetRefCount(AL_TDpb const* pDpb)
{
  return pDpb->uCountRef;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetPicCount(AL_TDpb const* pDpb)
{
  return pDpb->uCountPic;
}

/*****************************************************************************/
AL_TIndex AL_Dpb_GetHeadPOC(AL_TDpb const* pDpb)
{
  return pDpb->tHeadPOCNodeID;
}

/*****************************************************************************/
AL_TIndex AL_Dpb_GetNextPOC(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].tNextPOCNodeID;
}

/*****************************************************************************/
bool AL_Dpb_GetOutputFlag(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].bPicOutputFlag;
}

/*****************************************************************************/
uint8_t AL_Dpb_GetMarkingFlag(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].eMarkingFlag;
}

/*************************************************************************/
uint32_t sGetPicLatency_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].uPicLatency;
}

/*************************************************************************/
AL_TIndex AL_Dpb_GetPicID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{

  if(!IS_NODE_VALID(tNodeID))
    return AL_BAD_INDEX;

  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].tPicID;
}

/*************************************************************************/
AL_TIndex AL_Dpb_GetMvID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  if(!IS_NODE_VALID(tNodeID))
    return AL_BAD_INDEX;

  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].tMvID;
}

/*************************************************************************/
AL_TIndex AL_Dpb_GetFrmID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  if(!IS_NODE_VALID(tNodeID))
    return AL_BAD_INDEX;

  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].tFrameID;
}

/*************************************************************************/
AL_EPicStruct AL_Dpb_GetPicStruct_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  if(!IS_NODE_VALID(tNodeID))
    return AL_BAD_INDEX;

  Rtos_Assert(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE);
  return pDpb->Nodes[tNodeID].ePicStruct;
}

/*************************************************************************/
static AL_TIndex sApiGetLastPicID(AL_IReferenceManager const* pICtx)
{
  AL_TDpb const* pDpb = (AL_TDpb const*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;
  AL_TIndex tRetID = AL_BAD_INDEX;

  while(IS_NODE_VALID(tNodeID))
  {
    if(IS_NODE_VALID(pDpb->Nodes[tNodeID].tPicID))
      tRetID = pDpb->Nodes[tNodeID].tPicID;

    tNodeID = pDpb->Nodes[tNodeID].tNextPOCNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);

  return tRetID;
}

/*************************************************************************/
static AL_TIndex sApiGetLastRefPicID(AL_IReferenceManager const* pICtx)
{
  AL_TDpb const* pDpb = (AL_TDpb const*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;
  AL_TIndex tRetID = AL_BAD_INDEX;

  while(IS_NODE_VALID(tNodeID))
  {
    if(IS_NODE_VALID(pDpb->Nodes[tNodeID].tPicID) && pDpb->Nodes[tNodeID].eMarkingFlag < UNUSED_FOR_REF)
      tRetID = pDpb->Nodes[tNodeID].tPicID;

    tNodeID = pDpb->Nodes[tNodeID].tNextPOCNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);

  return tRetID;
}

/*************************************************************************/
uint8_t AL_Dpb_GetNumRef(AL_TDpb const* pDpb)
{
  return pDpb->uNumRef;
}

/*************************************************************************/
void AL_Dpb_SetNumRef(AL_TDpb* pDpb, uint8_t uMaxRef)
{
  pDpb->uNumRef = uMaxRef;
}

/*************************************************************************/
void AL_Dpb_SetMarkingFlag(AL_TDpb* pDpb, AL_TIndex tNodeID, AL_EMarkingRef eMarkingFlag)
{
  if(pDpb->Nodes[tNodeID].eMarkingFlag != UNUSED_FOR_REF && eMarkingFlag == UNUSED_FOR_REF)
    --pDpb->uCountRef;
  else if(pDpb->Nodes[tNodeID].eMarkingFlag == UNUSED_FOR_REF && eMarkingFlag != UNUSED_FOR_REF)
    ++pDpb->uCountRef;

  pDpb->Nodes[tNodeID].eMarkingFlag = eMarkingFlag;
}

/*************************************************************************/
void AL_Dpb_IncrementPicLatency(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  if(pDpb->Nodes[tNodeID].iFramePOC > pDpb->iCurFramePOC)
    ++pDpb->Nodes[tNodeID].uPicLatency;
}

/*************************************************************************/
bool AL_Dpb_sNodeIsReset(AL_TDpb const* pDpb, AL_TIndex tNodeID)
{
  return pDpb->Nodes[tNodeID].bIsReset;
}

/*************************************************************************/
bool AL_Dpb_LastHasMMCO5(AL_TDpb const* pDpb)
{
  return pDpb->bLastHasMMCO5;
}

/*************************************************************************/
void AL_Dpb_ResetMMCO5(AL_TDpb* pDpb)
{
  pDpb->bLastHasMMCO5 = false;
}

/*************************************************************************/
AL_TIndex AL_Dpb_ConvertPicIDToNodeID(AL_TDpb const* pDpb, AL_TIndex tPicID)
{
  return pDpb->PicId2NodeId[tPicID];
}

/*************************************************************************/
static AL_TIndex AL_Dpb_sGetNextFreeNode(AL_TDpb const* pDpb)
{
  AL_TIndex tNewID = 0;

  Rtos_GetMutex(pDpb->Mutex);

  while(tNewID < AL_REF_MNGR_MAX_BUF_SIZE)
  {
    if((pDpb->Nodes[tNewID].eMarkingFlag == UNUSED_FOR_REF) && !pDpb->Nodes[tNewID].bPicOutputFlag)
    {
      Rtos_ReleaseMutex(pDpb->Mutex);
      return tNewID;
    }
    ++tNewID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);

  return AL_BAD_INDEX;
}

/*****************************************************************************/
void AL_Dpb_sFillList(AL_TDpb const* pDpb, int32_t* pPocList, uint16_t* pLongTermList, uint16_t* pAvailableRefList, uint32_t* pSubpicList)
{
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;

  while(IS_NODE_VALID(tNodeID))
  {
    if(IS_NODE_VALID(pDpb->Nodes[tNodeID].tPicID) && !pDpb->Nodes[tNodeID].bNonExisting)
    {
      AL_TIndex tPicID = pDpb->Nodes[tNodeID].tPicID;

      if(IS_NODE_VALID(tPicID) && tPicID < AL_MAX_REF)
      {
        pPocList[tPicID] = pDpb->Nodes[tNodeID].iFramePOC;

        if(pDpb->Nodes[tNodeID].eMarkingFlag == LONG_TERM_REF)
          *pLongTermList |= (1 << tPicID); // long term flag

        if(pDpb->Nodes[tNodeID].eMarkingFlag != UNUSED_FOR_REF)
          *pAvailableRefList |= (1 << tPicID); // available ref POC
        *pSubpicList |= (pDpb->Nodes[tNodeID].bSubpicFlag << tPicID);
      }
    }

    tNodeID = pDpb->Nodes[tNodeID].tNextPOCNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*****************************************************************************/
void AL_Dpb_FillPocAndLongtermLists(AL_TDpb const* pDpb, TBufferPOC* pPoc, uint32_t uFirstLcuSliceSegment)
{
  int32_t* pPocList = (int32_t*)(pPoc->tMD.pVirtualAddr);

  if(pPocList == NULL)
    return;

  uint16_t* pLongTermList = (uint16_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);
  uint16_t* pAvailableRefList = (uint16_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_AVAILABLE_REF_OFFSET);
  uint32_t* pSubpicList = (uint32_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_SUBPIC_OFFSET);

  if(!uFirstLcuSliceSegment)
  {
    *pLongTermList = 0;
    *pAvailableRefList = 0;
    *pSubpicList = 0;

    for(int32_t i = 0; i < AL_MAX_REF; ++i)
      pPocList[i] = UINT32_MAX;
  }

  AL_Dpb_sFillList(pDpb, pPocList, pLongTermList, pAvailableRefList, pSubpicList);
}

/*************************************************************************/
AL_TIndex AL_Dpb_SearchPocLsb(AL_TDpb const* pDpb, int32_t poc_lsb)
{
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tParseNodeID = pDpb->tHeadPocLsbNodeID;

  while(IS_NODE_VALID(tParseNodeID))
  {
    if((pDpb->Nodes[tParseNodeID].slice_pic_order_cnt_lsb == poc_lsb) && (pDpb->Nodes[tParseNodeID].eMarkingFlag != UNUSED_FOR_REF))
      break;
    tParseNodeID = pDpb->Nodes[tParseNodeID].tNextPocLsbNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
  return tParseNodeID;
}

/*****************************************************************************/
AL_TIndex AL_Dpb_SearchPOC(AL_TDpb const* pDpb, int32_t iPOC)
{
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tParseNodeID = pDpb->tHeadPOCNodeID;
  AL_TDpbNode const* pNodes = pDpb->Nodes;

  while(IS_NODE_VALID(tParseNodeID))
  {
    if((pNodes[tParseNodeID].iFramePOC == iPOC) && (pNodes[tParseNodeID].eMarkingFlag != UNUSED_FOR_REF) && !pNodes[tParseNodeID].bNonExisting)
      break;
    tParseNodeID = pDpb->Nodes[tParseNodeID].tNextPOCNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
  return tParseNodeID;
}

/*****************************************************************************/
void AL_Dpb_Display(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tParseNodeID = pDpb->tHeadPOCNodeID;

  // check if there is anterior picture to be displayed present in the DPB
  while(tParseNodeID != tNodeID)
  {
    AL_TIndex tNextNodeID = pDpb->Nodes[tParseNodeID].tNextPOCNodeID;

    if(pDpb->Nodes[tParseNodeID].bPicOutputFlag)
      AL_Dpb_Display(pDpb, tParseNodeID);
    tParseNodeID = tNextNodeID;
  }

  // add current picture to display list
  if(pDpb->Nodes[tNodeID].bPicOutputFlag)
  {
    if(!pDpb->Nodes[tNodeID].bNonExisting)
    {
      AL_Dpb_sAddToDisplayList(pDpb, tNodeID);
      pDpb->iLastDisplayedPOC = pDpb->Nodes[tNodeID].iFramePOC;
    }

    pDpb->Nodes[tNodeID].bIsDisplayed = true;
    pDpb->Nodes[tNodeID].bPicOutputFlag = 0;
    --pDpb->uNumOutputPic;
  }
  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
static AL_TIndex sApiGetDisplayBuffer(AL_IReferenceManager* pICtx)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tFrameID = AL_BAD_INDEX;

  if(!DispFifo_IsEmpty(&pDpb->DispFifo))
  {
    tFrameID = DispFifo_Peek(&pDpb->DispFifo);

    if(DispFifo_GetStatus(&pDpb->DispFifo, tFrameID) != AL_READY_FOR_OUTPUT)
      tFrameID = AL_BAD_INDEX;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
  return tFrameID;
}

/*************************************************************************/
static AL_TIndex sApiReleaseDisplayBuffer(AL_IReferenceManager* pICtx)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);
  AL_TIndex tFrameID = DispFifo_Dequeue(&pDpb->DispFifo);
  AL_TPictMngrCallbacks* cb = &pDpb->tCallbacks;

  if(AL_IS_VALID_INDEX(tFrameID))
    cb->pfnDecrementFrmBuf(cb->pUserParam, tFrameID);
  Rtos_ReleaseMutex(pDpb->Mutex);
  return tFrameID;
}

/*************************************************************************/
void AL_Dpb_ClearOutput(AL_TDpb* pDpb)
{
  Rtos_GetMutex(pDpb->Mutex);
  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(IS_NODE_VALID(tNodeID))
  {
    pNodes[tNodeID].bPicOutputFlag = 0;
    tNodeID = pNodes[tNodeID].tNextPOCNodeID;
  }

  pDpb->uNumOutputPic = 0;
  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
static void sApiFlush(AL_IReferenceManager* pICtx)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);

  while(IS_NODE_VALID(pDpb->tHeadPOCNodeID))
    AL_Dpb_RemoveHead(pDpb);

  AL_Dpb_sBeginNewSeq(pDpb);

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
void AL_Dpb_HEVC_Cleanup(AL_TDpb* pDpb, uint32_t uMaxLatency, uint8_t MaxNumOutput)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;

  while(IS_NODE_VALID(tNodeID))
  {
    bool bShouldBeDisplayed = AL_Dpb_GetOutputFlag(pDpb, tNodeID) && ((sGetPicLatency_FromNode(pDpb, tNodeID) >= uMaxLatency) || (pDpb->uNumOutputPic > MaxNumOutput));

    if(bShouldBeDisplayed)
      AL_Dpb_Display(pDpb, tNodeID);

    if(pNodes[tNodeID].eMarkingFlag == UNUSED_FOR_REF && !AL_Dpb_GetOutputFlag(pDpb, tNodeID))
    {
      AL_TIndex tDeleteNodeID = tNodeID;
      tNodeID = pNodes[tNodeID].tNextPOCNodeID;

      AL_Dpb_Remove(pDpb, tDeleteNodeID);
    }
    else
      tNodeID = pNodes[tNodeID].tNextPOCNodeID;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
void AL_Dpb_AVC_Cleanup(AL_TDpb* pDpb)
{
  Rtos_GetMutex(pDpb->Mutex);

  AL_TIndex tNodeID = pDpb->tHeadPOCNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;

  while(IS_NODE_VALID(tNodeID))
  {
    AL_TIndex tNextNodeID = pNodes[tNodeID].tNextPOCNodeID;

    // Remove useless pictures
    if((AL_Dpb_GetMarkingFlag(pDpb, tNodeID) == UNUSED_FOR_REF) && !AL_Dpb_GetOutputFlag(pDpb, tNodeID))
      AL_Dpb_Remove(pDpb, tNodeID);

    tNodeID = tNextNodeID;
  }

  tNodeID = pDpb->tHeadPOCNodeID;

  while(IS_NODE_VALID(tNodeID) && (AL_Dpb_GetRefCount(pDpb) > AL_Dpb_GetNumRef(pDpb) || AL_Dpb_GetPicCount(pDpb) > AL_Dpb_GetNumRef(pDpb)))
  {
    AL_TIndex tNextNodeID = pNodes[tNodeID].tNextPOCNodeID;

    // Remove useless pictures
    if(AL_Dpb_GetMarkingFlag(pDpb, tNodeID) == UNUSED_FOR_REF)
    {
      AL_Dpb_Display(pDpb, tNodeID);
      AL_Dpb_Remove(pDpb, tNodeID);
    }

    tNodeID = tNextNodeID;
  }

  // if waiting for pic ID picture
  if(pDpb->bPicWaiting && (pDpb->FreePicIdCnt > 0))
    AL_Dpb_sFillWaitingPicture(pDpb);

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*************************************************************************/
// Remove from POC ordered linked list
static void RemoveFromPocList(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[tNodeID];
  AL_TIndex tPrevNodeID = pNode->tPrevPOCNodeID;
  AL_TIndex tNextNodeID = pNode->tNextPOCNodeID;

  if(pDpb->tHeadPOCNodeID == tNodeID)
    pDpb->tHeadPOCNodeID = tNextNodeID;

  if(IS_NODE_VALID(tPrevNodeID))
    pDpb->Nodes[tPrevNodeID].tNextPOCNodeID = tNextNodeID;

  if(IS_NODE_VALID(tNextNodeID))
    pDpb->Nodes[tNextNodeID].tPrevPOCNodeID = tPrevNodeID;
  else
    pDpb->tLastPOCNodeID = tPrevNodeID;
}

/*************************************************************************/
// Remove from poc lsb ordered linked list
static void RemoveFromPocLsbList(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[tNodeID];
  AL_TIndex tPrevNodeID = pNode->tPrevPocLsbNodeID;
  AL_TIndex tNextNodeID = pNode->tNextPocLsbNodeID;

  if(pDpb->tHeadPocLsbNodeID == tNodeID)
    pDpb->tHeadPocLsbNodeID = tNextNodeID;

  if(IS_NODE_VALID(tPrevNodeID))
    pDpb->Nodes[tPrevNodeID].tNextPocLsbNodeID = tNextNodeID;

  if(IS_NODE_VALID(tNextNodeID))
    pDpb->Nodes[tNextNodeID].tPrevPocLsbNodeID = tPrevNodeID;
}

/*************************************************************************/
// Remove from Decoding ordered linked list
static void RemoveFromDecOrderList(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  AL_TDpbNode* pNode = &pDpb->Nodes[tNodeID];
  AL_TIndex tPrevNodeID = pNode->tPrevDecOrderNodeID;
  AL_TIndex tNextNodeID = pNode->tNextDecOrderNodeID;

  if(pDpb->tHeadDecOrderNodeID == tNodeID)
    pDpb->tHeadDecOrderNodeID = tNextNodeID;

  if(IS_NODE_VALID(tPrevNodeID))
    pDpb->Nodes[tPrevNodeID].tNextDecOrderNodeID = tNextNodeID;

  if(IS_NODE_VALID(tNextNodeID))
    pDpb->Nodes[tNextNodeID].tPrevDecOrderNodeID = tPrevNodeID;
}

/*************************************************************************/
AL_TIndex AL_Dpb_Remove(AL_TDpb* pDpb, AL_TIndex tNodeID)
{
  Rtos_GetMutex(pDpb->Mutex);

  if(!IS_NODE_VALID(tNodeID))
  {
    Rtos_ReleaseMutex(pDpb->Mutex);
    return AL_BAD_INDEX;
  }

  AL_TDpbNode* pNode = &pDpb->Nodes[tNodeID];

  AL_TIndex tMvID = pNode->tMvID;
  bool bNonExisting = pNode->bNonExisting;
  AL_TIndex tFrameID = pNode->tFrameID;

  RemoveFromPocList(pDpb, tNodeID);
  RemoveFromPocLsbList(pDpb, tNodeID);
  RemoveFromDecOrderList(pDpb, tNodeID);

  // Release node
  AL_Dpb_sReleasePicID(pDpb, pNode->tPicID);

  // Update ref counter
  if(pNode->eMarkingFlag != UNUSED_FOR_REF)
    --pDpb->uCountRef;
  --pDpb->uCountPic;

  // Reset node information
  AL_Dpb_sResetNodeInfo(pDpb, pNode);

  // assigned pic id to the awaiting picture
  if(pDpb->bPicWaiting && (pDpb->FreePicIdCnt > 0))
    AL_Dpb_sFillWaitingPicture(pDpb);

  if(!bNonExisting)
  {
    pDpb->pDeletedFrmIDLst[pDpb->iDeletedFrmLstTail] = tFrameID;
    pDpb->pDeletedMvIDLst[pDpb->iDeletedMvLstTail] = tMvID;

    pDpb->iDeletedFrmLstTail = (pDpb->iDeletedFrmLstTail + 1) % AL_REFMNGR_MAX_POOL_SIZE;
    pDpb->iDeletedMvLstTail = (pDpb->iDeletedMvLstTail + 1) % AL_REF_MNGR_MAX_BUF_SIZE;

    ++pDpb->iNumDeletedPic;
  }

  Rtos_ReleaseMutex(pDpb->Mutex);

  return tFrameID;
}

/*************************************************************************/
AL_TIndex AL_Dpb_RemoveHead(AL_TDpb* pDpb)
{
  // Remove header of the Decoding ordered linked list
  if(AL_Dpb_GetOutputFlag(pDpb, pDpb->tHeadPOCNodeID))
    AL_Dpb_Display(pDpb, pDpb->tHeadPOCNodeID);
  return AL_Dpb_Remove(pDpb, pDpb->tHeadPOCNodeID);
}

/*****************************************************************************/
static void sApiInsert(AL_IReferenceManager* pICtx, AL_TIndex tFrameID, AL_TIndex tMvID, void* pIParam)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  AL_TDpbInsertParam* pParam = (AL_TDpbInsertParam*)pIParam;

  AL_TIndex tNodeID = AL_Dpb_sGetNextFreeNode(pDpb);

  if(!IS_NODE_VALID(tNodeID))
    return;

  if(!AL_Dpb_sNodeIsReset(pDpb, tNodeID))
    AL_Dpb_Remove(pDpb, tNodeID);

  AL_TIndex tPicID = AL_BAD_INDEX;

  Rtos_GetMutex(pDpb->Mutex);

  // Assign PicID
  if(!pParam->bNonExisting)
  {
    if(pDpb->FreePicIdCnt > 0)
    {
      tPicID = pDpb->FreePicIDs[--pDpb->FreePicIdCnt];
      pDpb->PicId2NodeId[tPicID] = tNodeID;
      pDpb->PicId2FrmId[tPicID] = tFrameID;
      pDpb->PicId2MvId[tPicID] = tMvID;
    }
    else
    {
      pDpb->bPicWaiting = true;
      pDpb->tNodeWaitingID = tNodeID;
      pDpb->tFrmWaitingID = tFrameID;
      pDpb->tMvWaitingID = tMvID;
    }
  }

  pDpb->tCurRefNodeID = tNodeID;
  AL_TDpbNode* pNode = &pDpb->Nodes[tNodeID];

  // Assign frame buffer information
  pNode->iFramePOC = pParam->iFramePOC;
  pNode->ePicStruct = pParam->ePicStruct;
  pNode->slice_pic_order_cnt_lsb = pParam->iPocLsb;
  pNode->tFrameID = tFrameID;
  pNode->tMvID = tMvID;
  pNode->tPicID = tPicID;
  pNode->eMarkingFlag = pParam->eMarkingFlag;
  pNode->tNodeID = tNodeID;
  pNode->bPicOutputFlag = pParam->bPicOutputFlag;
  pNode->bIsReset = false;
  pNode->bNonExisting = pParam->bNonExisting;
  pNode->eNUT = pParam->eNUT;
  pNode->bSubpicFlag = pParam->bSubpicFlag;

  // Update frame status in display fifo list
  if(IS_NODE_VALID(tFrameID))
    DispFifo_SetStatus(&pDpb->DispFifo, tFrameID, pParam->bPicOutputFlag ? AL_NOT_READY_FOR_OUTPUT : AL_NOT_NEEDED_FOR_OUTPUT);

  // Insert it in the POC ordered linked list
  if(!IS_NODE_VALID(pDpb->tHeadPOCNodeID))
  {
    pNode->tPrevPOCNodeID = AL_BAD_INDEX;
    pNode->tNextPOCNodeID = AL_BAD_INDEX;
    pDpb->tHeadPOCNodeID = tNodeID;
    pDpb->tLastPOCNodeID = tNodeID;

    pNode->tPrevPocLsbNodeID = AL_BAD_INDEX;
    pNode->tNextPocLsbNodeID = AL_BAD_INDEX;
    pDpb->tHeadPocLsbNodeID = tNodeID;

    pNode->tPrevDecOrderNodeID = AL_BAD_INDEX;
    pNode->tNextDecOrderNodeID = AL_BAD_INDEX;
    pDpb->tHeadDecOrderNodeID = tNodeID;
  }
  else
  {
    AL_TIndex tCurPOCNodeID = pDpb->tHeadPOCNodeID;
    AL_TIndex tCurPocLsbNodeID = pDpb->tHeadPocLsbNodeID;
    AL_TIndex tCurDecOrderNodeID = pDpb->tHeadDecOrderNodeID;

    while(true)
    {
      // compute poc ordered list
      if(pDpb->Nodes[tCurPOCNodeID].iFramePOC > pParam->iFramePOC)
      {
        AL_TIndex tPrevNodeID = pDpb->Nodes[tCurPOCNodeID].tPrevPOCNodeID;

        pNode->tPrevPOCNodeID = tPrevNodeID;
        pNode->tNextPOCNodeID = tCurPOCNodeID;
        pDpb->Nodes[tCurPOCNodeID].tPrevPOCNodeID = tNodeID;

        if(IS_NODE_VALID(tPrevNodeID))
          pDpb->Nodes[tPrevNodeID].tNextPOCNodeID = tNodeID;

        if(tCurPOCNodeID == pDpb->tHeadPOCNodeID)
          pDpb->tHeadPOCNodeID = tNodeID;
        break;
      }
      else if(!IS_NODE_VALID(pDpb->Nodes[tCurPOCNodeID].tNextPOCNodeID))
      {
        pNode->tNextPOCNodeID = AL_BAD_INDEX;
        pNode->tPrevPOCNodeID = tCurPOCNodeID;
        pDpb->Nodes[tCurPOCNodeID].tNextPOCNodeID = tNodeID;

        pDpb->tLastPOCNodeID = tNodeID;
        break;
      }
      else
        tCurPOCNodeID = pDpb->Nodes[tCurPOCNodeID].tNextPOCNodeID;
    }

    while(true)
    {
      // compute poc lsb ordered list
      AL_TDpbNode* pNodeCurPocLsb = &pDpb->Nodes[tCurPocLsbNodeID];

      if(pNodeCurPocLsb->slice_pic_order_cnt_lsb > pParam->iPocLsb)
      {
        AL_TIndex tPrevNodeID = pNodeCurPocLsb->tPrevPocLsbNodeID;
        pNode->tPrevPocLsbNodeID = tPrevNodeID;
        pNode->tNextPocLsbNodeID = tCurPocLsbNodeID;
        pNodeCurPocLsb->tPrevPocLsbNodeID = tNodeID;

        if(IS_NODE_VALID(tPrevNodeID))
          pDpb->Nodes[tPrevNodeID].tNextPocLsbNodeID = tNodeID;

        if(tCurPocLsbNodeID == pDpb->tHeadPocLsbNodeID)
          pDpb->tHeadPocLsbNodeID = tNodeID;
        break;
      }
      else if(!IS_NODE_VALID(pNodeCurPocLsb->tNextPocLsbNodeID))
      {
        pNode->tNextPocLsbNodeID = AL_BAD_INDEX;
        pNode->tPrevPocLsbNodeID = tCurPocLsbNodeID;
        pNodeCurPocLsb->tNextPocLsbNodeID = tNodeID;
        break;
      }
      else
        tCurPocLsbNodeID = pNodeCurPocLsb->tNextPocLsbNodeID;
    }

    // Decoding order linked list
    while(IS_NODE_VALID(pDpb->Nodes[tCurDecOrderNodeID].tNextDecOrderNodeID))
      tCurDecOrderNodeID = pDpb->Nodes[tCurDecOrderNodeID].tNextDecOrderNodeID;

    pNode->tPrevDecOrderNodeID = tCurDecOrderNodeID;
    pNode->tNextDecOrderNodeID = AL_BAD_INDEX;
    pDpb->Nodes[tCurDecOrderNodeID].tNextDecOrderNodeID = tNodeID;
  }

  // Update List counters
  if(pParam->eMarkingFlag != UNUSED_FOR_REF)
    ++pDpb->uCountRef;
  ++pDpb->uCountPic;

  if(pParam->bPicOutputFlag)
    ++pDpb->uNumOutputPic;

  AL_TPictMngrCallbacks* cb = &pDpb->tCallbacks;

  if(IS_NODE_VALID(tFrameID))
    cb->pfnIncrementFrmBuf(cb->pUserParam, tFrameID);

  if(IS_NODE_VALID(tMvID))
    cb->pfnIncrementAnnexBuf(cb->pUserParam, tMvID);
  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*****************************************************************************/
static void sApiUpdate(AL_IReferenceManager* pICtx)
{
  (void)pICtx;
}

/*****************************************************************************/
static bool AL_Dpb_sIsNoReordering(AL_TDpb const* pDpb)
{
  return pDpb->eMode == AL_DPB_NO_REORDERING;
}

/*****************************************************************************/
static AL_TIndex Dpb_GetNodeFromFrmID(AL_TDpb const* pDpb, AL_TIndex tFrameID)
{
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  AL_TIndex tNodeID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tNodeID))
  {
    if(pNodes[tNodeID].tFrameID == tFrameID)
      break;
    tNodeID = pNodes[tNodeID].tNextDecOrderNodeID;
  }

  return tNodeID;
}

/*****************************************************************************/
static void sApiEndDecoding(AL_IReferenceManager* pICtx, AL_TIndex tFrameID)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;
  Rtos_GetMutex(pDpb->Mutex);

  AL_Dpb_sReleaseUnusedBuf(pDpb);

  // Set Frame Availability
  if(DispFifo_GetStatus(&pDpb->DispFifo, tFrameID) == AL_NOT_READY_FOR_OUTPUT)
  {
    DispFifo_SetStatus(&pDpb->DispFifo, tFrameID, AL_READY_FOR_OUTPUT);

    if(AL_Dpb_sIsNoReordering(pDpb))
    {
      AL_TIndex tNodeID = Dpb_GetNodeFromFrmID(pDpb, tFrameID);
      bool isInDisplayList = (!IS_NODE_VALID(tNodeID));

      if(!isInDisplayList && AL_Dpb_GetOutputFlag(pDpb, tNodeID))
        AL_Dpb_Display(pDpb, tNodeID);
    }
  }

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*****************************************************************************/
void AL_Dpb_PictNumberProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  AL_TIndex tNodeID = pDpb->tHeadDecOrderNodeID;
  AL_TDpbNode* pNodes = pDpb->Nodes;
  uint32_t uMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  while(IS_NODE_VALID(tNodeID))// loop over references pictures
  {
    if(pNodes[tNodeID].eMarkingFlag == UNUSED_FOR_REF)
    {
      tNodeID = pNodes[tNodeID].tNextDecOrderNodeID;
      continue;
    }

    if(pNodes[tNodeID].eMarkingFlag == SHORT_TERM_REF)
    {
      pNodes[tNodeID].iFrameNumber = pNodes[tNodeID].iSliceFrameNumber;

      if(pNodes[tNodeID].iFrameNumber > pSlice->frame_num)
        pNodes[tNodeID].iFrameNumberWrap = pNodes[tNodeID].iFrameNumber - uMaxFrameNum;
      else
        pNodes[tNodeID].iFrameNumberWrap = pNodes[tNodeID].iFrameNumber;

      pNodes[tNodeID].iPicNum = pNodes[tNodeID].iFrameNumberWrap;

    }
    else if(pNodes[tNodeID].eMarkingFlag == LONG_TERM_REF)
      pNodes[tNodeID].iLongTermPicNum = pNodes[tNodeID].iLongTermFrameIdx;

    tNodeID = pNodes[tNodeID].tNextDecOrderNodeID;
  }
}

/******************************************************************************/
void AL_Dpb_MarkingProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, int32_t iCurFramePOC)
{
  AL_TDpbNode* pNodes = pDpb->Nodes;
  AL_TIndex tCurRefNodeID = pDpb->tCurRefNodeID;

  // fill node's parameters
  pNodes[tCurRefNodeID].iSliceFrameNumber = pSlice->frame_num;
  pNodes[tCurRefNodeID].tNodeID = tCurRefNodeID;

  if(AL_AVC_IsIDR(pSlice->nal_unit_type))
  {
    if(!pSlice->long_term_reference_flag)
    {
      pDpb->iMaxLongTermFrameIdx = INT32_MAX;
      return;
    }
    pNodes[tCurRefNodeID].iLongTermFrameIdx = 0;
    pDpb->iMaxLongTermFrameIdx = 0;
    return;
  }

  AL_Dpb_PictNumberProcess(pDpb, pSlice);

  if(pSlice->adaptive_ref_pic_marking_mode_flag)
    AL_Dpb_sAdaptiveMemoryControlMarking(pDpb, pSlice, iCurFramePOC);

  AL_Dpb_sSlidingWindowMarking(pDpb, pSlice);

  for(int32_t op_idc = 0; op_idc < 32; ++op_idc)
  {
    if((pSlice->memory_management_control_operation[op_idc] == 6) && (pNodes[tCurRefNodeID].eMarkingFlag != LONG_TERM_REF))
    {
      pNodes[tCurRefNodeID].eMarkingFlag = SHORT_TERM_REF;
      pNodes[tCurRefNodeID].iLongTermFrameIdx = INT32_MAX;
      break;
    }
  }
}

/*****************************************************************************/
void AL_Dpb_InitPSlice_RefList(AL_TDpb const* pDpb, AL_EPicStruct eCurrPicStruct, TBufferRef* pRefList)
{
  (void)eCurrPicStruct;
  Rtos_GetMutex(pDpb->Mutex);
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  AL_TDpbNode NodeShortTerm[AL_MAX_REF], NodeLongTerm[AL_MAX_REF];
  AL_TDpbNode NodeShortTermReordered[AL_MAX_REF], NodeLongTermReordered[AL_MAX_REF];
  AL_TDpbNode* pNodeTemp;

  Rtos_Memset(&NodeShortTerm[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));

  int32_t iCnt_short = AL_MAX_REF - 1;
  int32_t iCnt_long = 0;
  AL_TIndex tNodeID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tNodeID))
  {
    if(pNodes[tNodeID].eMarkingFlag != UNUSED_FOR_REF)
    {
      pNodeTemp = (pNodes[tNodeID].eMarkingFlag == SHORT_TERM_REF) ? &NodeShortTerm[iCnt_short--] : &NodeLongTerm[iCnt_long++];
      *pNodeTemp = pNodes[tNodeID];
    }
    tNodeID = pNodes[tNodeID].tNextDecOrderNodeID;
  }

  for(int32_t i = 0; i < AL_MAX_REF; i++)
  {
    NodeShortTermReordered[i] = NodeShortTerm[i];
    NodeLongTermReordered[i] = NodeLongTerm[i];
  }

  // merge short term and long term reference in the RefList
  int32_t iPic = 0;
  int32_t iRef = iCnt_short + 1;
  iCnt_short = AL_MAX_REF - iRef;

  while(iPic < iCnt_short)
    pRefList[iPic++].tNodeID = NodeShortTermReordered[iRef++].tNodeID;

  iRef = 0;

  while(iPic < (iCnt_short + iCnt_long))
    pRefList[iPic++].tNodeID = NodeLongTermReordered[iRef++].tNodeID;

  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*****************************************************************************/
void AL_Dpb_InitBSlice_RefList(AL_TDpb const* pDpb, int32_t iCurFramePOC, AL_EPicStruct eCurrPicStruct, TBufferListRef* pListRef)
{
  Rtos_GetMutex(pDpb->Mutex);
  AL_TDpbNode const* pNodes = pDpb->Nodes;
  AL_TDpbNode NodeShortTermPOCGreat[AL_MAX_REF], NodeShortTermPOCLess[AL_MAX_REF], NodeLongTerm[AL_MAX_REF];
  AL_TDpbNode NodeShortTermPOCL0[AL_MAX_REF], NodeShortTermPOCL1[AL_MAX_REF];
  AL_TDpbNode NodeShortTermPOCL0Reordered[AL_MAX_REF], NodeShortTermPOCL1Reordered[AL_MAX_REF], NodeLongTermReordered[AL_MAX_REF];
  AL_TDpbNode* pNodeShort;
  AL_TDpbNode NodeTemp;

  Rtos_Memset(&NodeShortTermPOCGreat[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeShortTermPOCLess[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeShortTermPOCL0[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeShortTermPOCL1[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));
  Rtos_Memset(&NodeLongTerm[0], 0, AL_MAX_REF * sizeof(AL_TDpbNode));

  int32_t iCnt_short_less = AL_MAX_REF - 1;
  int32_t iCnt_short_great = 0;
  AL_TIndex tShortNodeID = pDpb->tHeadPOCNodeID;

  while(IS_NODE_VALID(tShortNodeID))
  {
    if(pNodes[tShortNodeID].eMarkingFlag == SHORT_TERM_REF)
    {
      pNodeShort = (pNodes[tShortNodeID].iFramePOC < iCurFramePOC) ? &NodeShortTermPOCLess[iCnt_short_less--] : &NodeShortTermPOCGreat[iCnt_short_great++];
      *pNodeShort = pNodes[tShortNodeID];
    }
    tShortNodeID = pNodes[tShortNodeID].tNextPOCNodeID;
  }

  int32_t iCnt_long = 0;
  AL_TIndex tLongNodeID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tLongNodeID)) /*long_term_reference case*/
  {
    if(pNodes[tLongNodeID].eMarkingFlag == LONG_TERM_REF)
      NodeLongTerm[iCnt_long++] = pNodes[tLongNodeID];
    tLongNodeID = pNodes[tLongNodeID].tNextDecOrderNodeID;
  }

  // Create short-term list O
  int32_t iPic = 0;
  int32_t iRef = iCnt_short_less + 1;
  iCnt_short_less = AL_MAX_REF - iRef;

  while(iPic < iCnt_short_less)
    NodeShortTermPOCL0[iPic++] = NodeShortTermPOCLess[iRef++];

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great))
    NodeShortTermPOCL0[iPic++] = NodeShortTermPOCGreat[iRef++];

  // Create short-term list 1
  iPic = 0;
  iRef = 0;

  while(iPic < iCnt_short_great)
    NodeShortTermPOCL1[iPic++] = NodeShortTermPOCGreat[iRef++];

  iRef = AL_MAX_REF - iCnt_short_less;

  while(iPic < (iCnt_short_less + iCnt_short_great))
    NodeShortTermPOCL1[iPic++] = NodeShortTermPOCLess[iRef++];

  for(int32_t i = 0; i < AL_MAX_REF; i++)
  {
    NodeShortTermPOCL0Reordered[i] = NodeShortTermPOCL0[i];
    NodeShortTermPOCL1Reordered[i] = NodeShortTermPOCL1[i];
    NodeLongTermReordered[i] = NodeLongTerm[i];
  }

  (void)eCurrPicStruct;

  iPic = 0;
  iRef = 0;

  // List 0
  while(iPic < iCnt_short_less + iCnt_short_great)
    (*pListRef)[0][iPic++].tNodeID = NodeShortTermPOCL0Reordered[iRef++].tNodeID;

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great + iCnt_long))
    (*pListRef)[0][iPic++].tNodeID = NodeLongTermReordered[iRef++].tNodeID;

  iPic = 0;
  iRef = 0;

  // List 1
  while(iPic < iCnt_short_less + iCnt_short_great)
    (*pListRef)[1][iPic++].tNodeID = NodeShortTermPOCL1Reordered[iRef++].tNodeID;

  iRef = 0;

  while(iPic < (iCnt_short_less + iCnt_short_great + iCnt_long))
    (*pListRef)[1][iPic++].tNodeID = NodeLongTermReordered[iRef++].tNodeID;

  if(!(iCnt_short_less + iCnt_short_great + iCnt_long > 1))
  {
    Rtos_ReleaseMutex(pDpb->Mutex);
    return;
  }

  for(iPic = 0; iPic < iCnt_short_less + iCnt_short_great + iCnt_long; ++iPic)
  {
    if((*pListRef)[1][iPic].tNodeID != (*pListRef)[0][iPic].tNodeID)
    {
      Rtos_ReleaseMutex(pDpb->Mutex);
      return;
    }
  }

  NodeTemp = pNodes[(*pListRef)[1][0].tNodeID & 0x3F];
  (*pListRef)[1][0].tNodeID = (*pListRef)[1][1].tNodeID;
  (*pListRef)[1][1].tNodeID = NodeTemp.tNodeID;
  Rtos_ReleaseMutex(pDpb->Mutex);
}

/*****************************************************************************/
void AL_Dpb_ModifShortTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, int32_t iPicNumIdc, uint8_t uOffset, int32_t iL0L1, AL_TIndex* pRefIdx, int32_t* pPicNumPred, TBufferListRef* pListRef)
{
  int32_t iMaxFrameNum = (1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4)) * (1 + pSlice->field_pic_flag);
  int32_t iDiffPicNum = iL0L1 ? pSlice->abs_diff_pic_num_minus1_l1[uOffset] + 1 :
                        pSlice->abs_diff_pic_num_minus1_l0[uOffset] + 1;

  uint8_t uNumRef = iL0L1 ? pSlice->num_ref_idx_l1_active_minus1 + 1 :
                    pSlice->num_ref_idx_l0_active_minus1 + 1;

  AL_TDpbNode const* pNodes = pDpb->Nodes;

  int32_t iPicNumNoWrap;

  if(!iPicNumIdc)
    iPicNumNoWrap = ((*pPicNumPred - iDiffPicNum) < 0) ?
                    *pPicNumPred - iDiffPicNum + iMaxFrameNum : *pPicNumPred - iDiffPicNum + 0;
  else
    iPicNumNoWrap = ((*pPicNumPred + iDiffPicNum) >= iMaxFrameNum) ?
                    *pPicNumPred + iDiffPicNum - iMaxFrameNum : *pPicNumPred + iDiffPicNum + 0;

  *pPicNumPred = iPicNumNoWrap;

  int32_t iCurrPicNum = pSlice->frame_num * (1 + pSlice->field_pic_flag) + pSlice->field_pic_flag;
  int32_t iPicNum = (iPicNumNoWrap > iCurrPicNum) ? iPicNumNoWrap - iMaxFrameNum : iPicNumNoWrap - 0;

  /*process reordering*/
  for(uint8_t u = uNumRef; u > *pRefIdx; --u)
    (*pListRef)[iL0L1][u] = (*pListRef)[iL0L1][u - 1];

  AL_TIndex tCptNodeID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tCptNodeID))
  {
    if((pNodes[tCptNodeID].iPicNum == iPicNum) && (pNodes[tCptNodeID].eMarkingFlag == SHORT_TERM_REF))
    {
      (*pListRef)[iL0L1][(*pRefIdx)].tNodeID = pNodes[tCptNodeID].tNodeID;
      (*pRefIdx)++;

      uint8_t unIdx = *pRefIdx;

      for(tCptNodeID = *pRefIdx; tCptNodeID <= uNumRef; ++tCptNodeID)
      {
        if(AL_Dpb_sPicNumF(pDpb, pSlice, (*pListRef)[iL0L1][tCptNodeID].tNodeID) != iPicNum)
        {
          // avoid memcpy if source == destination
          if(unIdx != tCptNodeID)
            (*pListRef)[iL0L1][unIdx] = (*pListRef)[iL0L1][tCptNodeID];
          unIdx++;
        }
      }

      break;
    }
    tCptNodeID = pNodes[tCptNodeID].tNextDecOrderNodeID;
  }
}

/*****************************************************************************/
void AL_Dpb_ModifLongTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t uOffset, int32_t iL0L1, AL_TIndex* pRefIdx, TBufferListRef* pListRef)
{
  uint8_t uNumRef = iL0L1 ? pSlice->num_ref_idx_l1_active_minus1 + 1 : pSlice->num_ref_idx_l0_active_minus1 + 1;

  for(uint8_t u = uNumRef; u > *pRefIdx; --u)
    (*pListRef)[iL0L1][u] = (*pListRef)[iL0L1][u - 1];

  AL_64S const iLongTermPicNum = iL0L1 ? pSlice->long_term_pic_num_l1[uOffset] : pSlice->long_term_pic_num_l0[uOffset];
  AL_EPicStruct ePicStruct = AL_PS_FRM;

  AL_TDpbNode const* pNodes = pDpb->Nodes;
  AL_TIndex tCptNodeID = pDpb->tHeadDecOrderNodeID;

  while(IS_NODE_VALID(tCptNodeID))
  {
    AL_64S iRefLongTermPicNum = pNodes[tCptNodeID].iLongTermFrameIdx;

    if(ePicStruct != AL_PS_FRM)
      iRefLongTermPicNum = (pNodes[tCptNodeID].ePicStruct == ePicStruct) ? (2 * iRefLongTermPicNum + 1) : (2 * iRefLongTermPicNum);

    if((iRefLongTermPicNum == iLongTermPicNum) && (pNodes[tCptNodeID].eMarkingFlag == LONG_TERM_REF))
    {
      (*pListRef)[iL0L1][(*pRefIdx)++].tNodeID = pNodes[tCptNodeID].tNodeID;

      uint8_t unIdx = *pRefIdx;

      for(tCptNodeID = *pRefIdx; tCptNodeID <= uNumRef; ++tCptNodeID)
      {
        if(AL_Dpb_sLongTermPicNumF(pDpb, (*pListRef)[iL0L1][tCptNodeID].tNodeID) != iLongTermPicNum)
          (*pListRef)[iL0L1][unIdx++] = (*pListRef)[iL0L1][tCptNodeID];
      }

      break;
    }
    tCptNodeID = pNodes[tCptNodeID].tNextDecOrderNodeID;
  }
}

/*****************************************************************************/
bool AL_Dpb_HasExistingRef(AL_TDpb const* pDpb, TBufferListRef const* pListRef)
{
  for(int32_t l = 0; l < 2; ++l) // L0/L1
  {
    TBufferRef const* pRef = (*pListRef)[l];

    while(IS_NODE_VALID(pRef->tNodeID))
    {
      if(!pDpb->Nodes[pRef->tNodeID].bNonExisting)
        return true;
      ++pRef;
    }
  }

  return false;
}

/*****************************************************************************/
static uint8_t sApiGetReflistIds(AL_IReferenceManager* pICtx, AL_TIndex* pFrameIds, AL_TIndex* pAnnexIds, bool* pConcealIds, bool bConceal, AL_TIndex tConcealPicID)
{
  AL_TDpb* pDpb = (AL_TDpb*)pICtx;

  for(int32_t iPicID = 0; iPicID < AL_MAX_REF; ++iPicID)
  {
    AL_TIndex tNodeID = AL_Dpb_ConvertPicIDToNodeID(pDpb, iPicID);
    pConcealIds[iPicID] = false;

    if(!IS_NODE_VALID(tNodeID))
    {

      if(bConceal && !sIsEmpty(pDpb))
      {
        tNodeID = AL_Dpb_ConvertPicIDToNodeID(pDpb, tConcealPicID);
        pConcealIds[iPicID] = true;
      }
    }

    if(IS_NODE_VALID(tNodeID))
    {
      AL_TIndex tFrameID = AL_Dpb_GetFrmID_FromNode(pDpb, tNodeID);
      AL_TIndex tMvID = AL_Dpb_GetMvID_FromNode(pDpb, tNodeID);
      pFrameIds[iPicID] = tFrameID;
      pAnnexIds[iPicID] = tMvID;
    }
    else
    {
      pFrameIds[iPicID] = AL_BAD_INDEX;
      pAnnexIds[iPicID] = AL_BAD_INDEX;
      pConcealIds[iPicID] = true;
    }
  }

  return AL_MAX_REF;
}

/*************************************************************************/
AL_TIndex sApiGetAnnexIdFromReferenceId(AL_IReferenceManager const* pCtx, AL_TIndex tRefId)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx;

  AL_TIndex tNodeID = AL_Dpb_ConvertPicIDToNodeID(pDpb, tRefId);
  AL_TIndex tMvID = AL_Dpb_GetMvID_FromNode(pDpb, tNodeID);

  return tMvID;
}

/*************************************************************************/
static AL_IReferenceManagerVtable const vTable =
{
  sApiInit,
  sApiDeinit,

  sApiInsert,
  sApiUpdate,
  sApiEndDecoding,
  sApiGetDisplayBuffer,
  sApiReleaseDisplayBuffer,
  sApiTerminate,
  sApiFlush,

  sApiGetReflistIds,
  sApiGetLastPicID,
  sApiGetLastRefPicID,
  sApiGetAnnexIdFromReferenceId
};

/*************************************************************************/
AL_IReferenceManager* AL_Dpb_Create(AL_TDpbInitParam* pParams)
{
  AL_TDpb* pThis = Rtos_Malloc(sizeof(*pThis));

  if(!pThis)
    return NULL;

  pThis->vtable = &vTable;
  pThis->eMode = pParams->eMode;
  pThis->uNumRef = pParams->uNumRef;

  return (AL_IReferenceManager*)pThis;
}

/*!@}*/
