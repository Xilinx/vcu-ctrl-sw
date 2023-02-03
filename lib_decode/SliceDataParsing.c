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

#include "SliceDataParsing.h"
#include "I_DecoderCtx.h"

#include "lib_decode/lib_decode.h"
#include "lib_decode/I_DecScheduler.h"

#include "lib_common/PixMapBufferInternal.h"
#include "lib_common/BufferHandleMeta.h"

#include "lib_common_dec/DecHwScalingList.h"
#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Hevc_PictMngr.h"

#include "lib_assert/al_assert.h"

/******************************************************************************/
static void setBufferHandle(const TBuffer* in, TBuffer* out)
{
  out->tMD.uPhysicalAddr = in->tMD.uPhysicalAddr;
  out->tMD.pVirtualAddr = in->tMD.pVirtualAddr;
  out->tMD.uSize = in->tMD.uSize;
}

/******************************************************************************/
static void AL_sGetToggleBuffers(const AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs)
{
  const int toggle = pCtx->uToggle;
  setBufferHandle(&pCtx->PoolListRefAddr[toggle], &pBufs->tListRef);
  setBufferHandle(&pCtx->PoolVirtRefAddr[toggle], &pBufs->tListVirtRef);
  setBufferHandle(&pCtx->PoolCompData[toggle], &pBufs->tCompData);
  setBufferHandle(&pCtx->PoolCompMap[toggle], &pBufs->tCompMap);
  setBufferHandle(&pCtx->PoolSclLst[toggle], &pBufs->tScl);
  setBufferHandle(&pCtx->PoolWP[toggle], &pBufs->tWP);
}

/******************************************************************************/
static void pushCommandParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP, bool bIsLastAUNal)
{
  pSP->bIsLastSlice = bIsLastAUNal;

  /* update circular buffer */
  pSP->uStrAvailSize = pCtx->NalStream.iAvailSize;
  pSP->uStrOffset = pCtx->NalStream.iOffset;
}

/******************************************************************************/
static void AL_sSaveCommandBlk2(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPP, AL_TDecPicBuffers* pBufs)
{
  (void)pPP;

  int const iMaxBitDepth = pCtx->tStreamSettings.iBitDepth;
  AL_TBuffer* pRec = pCtx->pRecs.pFrame;

  uint32_t uPitch = AL_PixMapBuffer_GetPlanePitch(pRec, AL_PLANE_Y);
  AL_Assert(uPitch != 0);

  // The first version supported only 8 or 10 bit with a flag at pos 31.
  // For backward compatibility, the bit 30 is used to set 12 bits output picture bitdepth
  uint32_t uPictureBitDepth = (iMaxBitDepth - 8);
  int iDec2RecBitDepthOffset = 28;
  iDec2RecBitDepthOffset = 30;
  switch(iMaxBitDepth)
  {
  case 8: uPictureBitDepth = 0x0;
    break;
  case 10: uPictureBitDepth = 0x2;
    break;
  case 12: uPictureBitDepth = 0x1;
    break;
  default: assert(false);
  }

  uPictureBitDepth <<= iDec2RecBitDepthOffset;
  pBufs->uPitch = uPitch | uPictureBitDepth;

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pRec);
  AL_EChromaOrder eChromaOrder = AL_GetChromaOrder(tFourCC);

  AL_EPlaneId eFirstCPlane = eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_U : AL_PLANE_UV;
  pBufs->tRecY.tMD.uPhysicalAddr = AL_PixMapBuffer_GetPlanePhysicalAddress(pRec, AL_PLANE_Y);
  pBufs->tRecY.tMD.pVirtualAddr = AL_PixMapBuffer_GetPlaneAddress(pRec, AL_PLANE_Y);
  pBufs->tRecC1.tMD.uPhysicalAddr = AL_PixMapBuffer_GetPlanePhysicalAddress(pRec, eFirstCPlane);
  pBufs->tRecC1.tMD.pVirtualAddr = AL_PixMapBuffer_GetPlaneAddress(pRec, eFirstCPlane);

  uint32_t uOffset = AL_PixMapBuffer_GetPositionOffset(pRec, pCtx->tOutputPosition, AL_PLANE_Y);
  pBufs->tRecY.tMD.uPhysicalAddr += uOffset;
  pBufs->tRecY.tMD.pVirtualAddr += uOffset;
  uOffset = AL_PixMapBuffer_GetPositionOffset(pRec, pCtx->tOutputPosition, eFirstCPlane);
  pBufs->tRecC1.tMD.uPhysicalAddr += uOffset;
  pBufs->tRecC1.tMD.pVirtualAddr += uOffset;

  pBufs->tPoc.tMD.uPhysicalAddr = pCtx->POC.tMD.uPhysicalAddr;
  pBufs->tPoc.tMD.pVirtualAddr = pCtx->POC.tMD.pVirtualAddr;
  pBufs->tPoc.tMD.uSize = pCtx->POC.tMD.uSize;

  pBufs->tMV.tMD.uPhysicalAddr = pCtx->MV.tMD.uPhysicalAddr;
  pBufs->tMV.tMD.pVirtualAddr = pCtx->MV.tMD.pVirtualAddr;
  pBufs->tMV.tMD.uSize = pCtx->MV.tMD.uSize;

}

/*****************************************************************************/
static void AL_sSaveNalStreamBlk1(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  (void)pSP;
  pCtx->NalStream = pCtx->Stream;
}

/*****************************************************************************/
static void AL_FlushBuffers(AL_TDecCtx* pCtx)
{
  AL_TDecPicBuffers* pPictBuffers = &pCtx->PoolPB[pCtx->uToggle];

  Rtos_FlushCacheMemory(pPictBuffers->tListRef.tMD.pVirtualAddr, pPictBuffers->tListRef.tMD.uSize);
  Rtos_FlushCacheMemory(pPictBuffers->tPoc.tMD.pVirtualAddr, pPictBuffers->tPoc.tMD.uSize);
  Rtos_FlushCacheMemory(pPictBuffers->tScl.tMD.pVirtualAddr, pPictBuffers->tScl.tMD.uSize);
  Rtos_FlushCacheMemory(pPictBuffers->tWP.tMD.pVirtualAddr, pPictBuffers->tWP.tMD.uSize);

  // Stream buffer was already flushed for start-code detection.
}

/*****************************************************************************/
static void AL_SetBufferAddrs(AL_TDecCtx* pCtx, AL_TDecPicBufferAddrs* pBufAddrs)
{
  AL_TDecPicBuffers* pPictBuffers = &pCtx->PoolPB[pCtx->uToggle];
  pBufAddrs->pCompData = pPictBuffers->tCompData.tMD.uPhysicalAddr;
  pBufAddrs->pCompMap = pPictBuffers->tCompMap.tMD.uPhysicalAddr;
  pBufAddrs->pListRef = pPictBuffers->tListRef.tMD.uPhysicalAddr;
  pBufAddrs->pMV = pPictBuffers->tMV.tMD.uPhysicalAddr;
  pBufAddrs->pPoc = pPictBuffers->tPoc.tMD.uPhysicalAddr;
  pBufAddrs->pRecY = pPictBuffers->tRecY.tMD.uPhysicalAddr;
  pBufAddrs->pRecC1 = pPictBuffers->tRecC1.tMD.uPhysicalAddr;
  pBufAddrs->pRecFbcMapY = pCtx->pChanParam->bFrameBufferCompression ? pPictBuffers->tRecFbcMapY.tMD.uPhysicalAddr : 0;
  pBufAddrs->pRecFbcMapC1 = pCtx->pChanParam->bFrameBufferCompression ? pPictBuffers->tRecFbcMapC1.tMD.uPhysicalAddr : 0;
  pBufAddrs->pScl = pPictBuffers->tScl.tMD.uPhysicalAddr;
  pBufAddrs->pWP = pPictBuffers->tWP.tMD.uPhysicalAddr;
  pBufAddrs->pStream = pPictBuffers->tStream.tMD.uPhysicalAddr;

  AL_Assert(pPictBuffers->tStream.tMD.uSize > 0);
  pBufAddrs->uStreamSize = pPictBuffers->tStream.tMD.uSize;
  pBufAddrs->uPitch = pPictBuffers->uPitch;

}

static void SetBufferHandleMetaData(AL_TDecCtx* pCtx)
{
  if(!pCtx->pInputBuffer)
    return;

  if(pCtx->pInputBuffer == pCtx->eosBuffer)
    return;

  AL_THandleMetaData* pMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pCtx->pRecs.pFrame, AL_META_TYPE_HANDLE);

  if(!pMeta)
  {
    pMeta = AL_HandleMetaData_Create(AL_MAX_SLICES_SUBFRAME, sizeof(AL_TDecMetaHandle));
    AL_Buffer_AddMetaData(pCtx->pRecs.pFrame, (AL_TMetaData*)pMeta);
  }

  AL_TDecMetaHandle handle = { AL_DEC_HANDLE_STATE_PROCESSING, pCtx->pInputBuffer };
  AL_HandleMetaData_AddHandle(pMeta, &handle);
}

/***************************************************************************/
/*                          P U B L I C   f u n c t i o n s                */
/***************************************************************************/

static void UpdateStreamOffset(AL_TDecCtx* pCtx)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = (pCtx->Stream.iOffset + pCtx->Stream.iAvailSize) % pCtx->Stream.tMD.uSize;
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

static void decodeOneSlice(AL_TDecCtx* pCtx, uint16_t uSliceID, AL_TDecPicBufferAddrs* pBufAddrs)
{
  AL_TDecSliceParam* pSP_v = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID]);
  AL_PADDR pSP_p = (AL_PADDR)(uintptr_t)&(((AL_TDecSliceParam*)(uintptr_t)pCtx->PoolSP[pCtx->uToggle].tMD.uPhysicalAddr)[uSliceID]);
  TMemDesc SliceParam;
  SliceParam.pVirtualAddr = (AL_VADDR)pSP_v;
  SliceParam.uPhysicalAddr = pSP_p;
  // The HandleMetaData handle order should be the same as the slice order
  // as we add them each time we send one.
  pSP_v->uParsingId = uSliceID;
  AL_IDecScheduler_DecodeOneSlice(pCtx->pScheduler, pCtx->hChannel, &pCtx->PoolPP[pCtx->uToggle], pBufAddrs, &SliceParam);
}

/*****************************************************************************/

/*****************************************************************************/
void AL_LaunchSliceDecoding(AL_TDecCtx* pCtx, bool bIsLastAUNal, bool hasPreviousSlice)
{
  uint16_t uSliceID = pCtx->PictMngr.uNumSlice - 1;
  AL_TDecSliceParam* pPrevSP = NULL;

  UpdateStreamOffset(pCtx);

  if(hasPreviousSlice && uSliceID)
  {
    pPrevSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]);

    if(pPrevSP->eSliceType == AL_SLICE_CONCEAL && uSliceID == 1)
    {
      AL_FlushBuffers(pCtx);
      AL_SetBufferAddrs(pCtx, &pCtx->BufAddrs);
      SetBufferHandleMetaData(pCtx);
    }
    decodeOneSlice(pCtx, uSliceID - 1, &pCtx->BufAddrs);
  }

  AL_FlushBuffers(pCtx);
  AL_SetBufferAddrs(pCtx, &pCtx->BufAddrs);
  SetBufferHandleMetaData(pCtx);

  if(!bIsLastAUNal)
    return;

  if(pPrevSP == NULL || !pPrevSP->bIsLastSlice)
    decodeOneSlice(pCtx, uSliceID, &pCtx->BufAddrs);

  pCtx->uCurTileID = 0;

  Rtos_GetMutex(pCtx->DecMutex);
  ++pCtx->iNumFrmBlk1;
  pCtx->uToggle = (pCtx->iNumFrmBlk1 % pCtx->iStackSize);
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

/*****************************************************************************/
void AL_LaunchFrameDecoding(AL_TDecCtx* pCtx)
{
  AL_FlushBuffers(pCtx);

  AL_TDecPicBufferAddrs BufAddrs;
  AL_SetBufferAddrs(pCtx, &BufAddrs);

  UpdateStreamOffset(pCtx);
  SetBufferHandleMetaData(pCtx);
  AL_TDecSliceParam* pSP = (AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr;
  pSP->uParsingId = 0;

  AL_IDecScheduler_DecodeOneFrame(pCtx->pScheduler, pCtx->hChannel, &pCtx->PoolPP[pCtx->uToggle], &BufAddrs, &pCtx->PoolSP[pCtx->uToggle].tMD);

  pCtx->uCurTileID = 0;

  Rtos_GetMutex(pCtx->DecMutex);
  ++pCtx->iNumFrmBlk1;
  pCtx->uToggle = (pCtx->iNumFrmBlk1 % pCtx->iStackSize);
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

/*****************************************************************************/
static void AL_InitRefBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs)
{
  int iOffset = pCtx->iNumFrmBlk1 % MAX_STACK_SIZE;
  pCtx->uNumRef[iOffset] = 0;

  uint8_t uNode = 0;

  while(uNode < MAX_DPB_SIZE && pCtx->uNumRef[iOffset] < MAX_REF)
  {
    AL_EMarkingRef eMarkingRef = AL_Dpb_GetMarkingFlag(&pCtx->PictMngr.DPB, uNode);
    uint8_t uFrameId = AL_Dpb_GetFrmID_FromNode(&pCtx->PictMngr.DPB, uNode);
    uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->PictMngr.DPB, uNode);

    if((uFrameId != uEndOfList) && (uMvID != uEndOfList) && (eMarkingRef != UNUSED_FOR_REF) && (eMarkingRef != NON_EXISTING_REF))
    {
      pCtx->uFrameIDRefList[iOffset][pCtx->uNumRef[iOffset]] = uFrameId;
      pCtx->uMvIDRefList[iOffset][pCtx->uNumRef[iOffset]] = uMvID;
      ++pCtx->uNumRef[iOffset];
    }

    ++uNode;
  }

  AL_PictMngr_LockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->uFrameIDRefList[iOffset], pCtx->uMvIDRefList[iOffset]);

  // prepare buffers
  AL_sGetToggleBuffers(pCtx, pBufs);
  {
    AL_CleanupMemory(pBufs->tCompData.tMD.pVirtualAddr, pBufs->tCompData.tMD.uSize);
    AL_CleanupMemory(pBufs->tCompMap.tMD.pVirtualAddr, pBufs->tCompMap.tMD.uSize);
  }
}

/*****************************************************************************/
bool AL_InitFrameBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_TDecPicParam* pPP)
{
  Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);

  if(!AL_PictMngr_BeginFrame(&pCtx->PictMngr, bStartsNewCVS, tDim, eChromaMode))
  {
    pCtx->eChanState = CHAN_DESTROYING;
    Rtos_ReleaseSemaphore(pCtx->Sem);
    return false;
  }
  pPP->tBufIDs.FrmID = AL_PictMngr_GetCurrentFrmID(&pCtx->PictMngr);
  pPP->tBufIDs.MvID = AL_PictMngr_GetCurrentMvID(&pCtx->PictMngr);

  AL_InitRefBuffers(pCtx, pBufs);
  return true;
}

/*****************************************************************************/
void AL_CancelFrameBuffers(AL_TDecCtx* pCtx)
{
  AL_PictMngr_CancelFrame(&pCtx->PictMngr);

  int iOffset = pCtx->iNumFrmBlk1 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->uFrameIDRefList[iOffset], pCtx->uMvIDRefList[iOffset]);
  UpdateContextAtEndOfFrame(pCtx);
  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
static void AL_TerminateCurrentCommand(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPP, AL_TDecSliceParam* pSP)
{
  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];

  pSP->NextSliceSegment = pPP->LcuPicWidth * pPP->LcuPicHeight;
  pSP->NextIsDependent = false;

  AL_sSaveCommandBlk2(pCtx, pPP, pBufs);
  pushCommandParameters(pCtx, pSP, true);
}

/*****************************************************************************/
void AL_SetConcealParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  pSP->ConcealPicID = AL_PictMngr_GetLastPicID(&pCtx->PictMngr);
  pSP->ValidConceal = (pSP->ConcealPicID == uEndOfList) ? false : true;
}

/*****************************************************************************/
void AL_TerminatePreviousCommand(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPP, AL_TDecSliceParam* pSP, bool bIsLastVclNalInAU, bool bNextIsDependent)
{
  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];
  AL_sSaveCommandBlk2(pCtx, pPP, pBufs);

  if(pCtx->PictMngr.uNumSlice == 0)
    return;

  AL_TDecSliceParam* pPrevSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice - 1]);

  if(bIsLastVclNalInAU)
    pPrevSP->NextSliceSegment = pPP->LcuPicWidth * pPP->LcuPicHeight;
  else
    pPrevSP->NextSliceSegment = pSP->FirstLcuSliceSegment;

  pPrevSP->NextIsDependent = bNextIsDependent;

  if(!pCtx->tConceal.bValidFrame)
  {
    pPrevSP->eSliceType = AL_SLICE_CONCEAL;
    pPrevSP->SliceFirstLCU = 0;
  }
  pushCommandParameters(pCtx, pPrevSP, bIsLastVclNalInAU);
}

/*****************************************************************************/
void AL_AVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->PictMngr.uNumSlice;

  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPP->iFrmNum = pCtx->iNumFrmBlk1;
  pPP->UserParam = pCtx->uToggle;

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    AL_TerminatePreviousCommand(pCtx, pPP, pSP, bIsLastVclNalInAU, true);
    return;
  }

  // copy collocated info
  if(pSP->FirstLcuSliceSegment && pSP->eSliceType == AL_SLICE_I)
    pSP->ColocPicID = pPrevSP->ColocPicID;

  if(!pSlice->first_mb_in_slice)
    AL_AVC_WriteDecHwScalingList((AL_TScl const*)pSCL, pPP->ChromaMode, pBufs->tScl.tMD.pVirtualAddr);
  AL_AVC_PictMngr_GetBuffers(&pCtx->PictMngr, pSP, pSlice, (TBufferListRef const*)&pCtx->ListRef, &pBufs->tListVirtRef, &pBufs->tListRef, &pCtx->POC, &pCtx->MV, &pBufs->tWP, &pCtx->pRecs);

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, true);

  if(pSP->SliceFirstLCU)
    pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->uCurTileID;

  AL_sSaveNalStreamBlk1(pCtx, pSP);

  if(bIsLastVclNalInAU)
    AL_TerminateCurrentCommand(pCtx, pPP, pSP);
}

/*****************************************************************************/
void AL_HEVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->PictMngr.uNumSlice;
  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPP->iFrmNum = pCtx->iNumFrmBlk1;
  pPP->UserParam = pCtx->uToggle;

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    AL_TerminatePreviousCommand(pCtx, pPP, pSP, bIsLastVclNalInAU, true);
    return;
  }

  // copy collocated info
  if(pSP->FirstLcuSliceSegment && pSP->eSliceType == AL_SLICE_I)
    pSP->ColocPicID = pPrevSP->ColocPicID;

  if(pSlice->first_slice_segment_in_pic_flag)
    AL_HEVC_WriteDecHwScalingList((const AL_TScl*)pSCL, pBufs->tScl.tMD.pVirtualAddr);
  AL_HEVC_PictMngr_GetBuffers(&pCtx->PictMngr, pSP, pSlice, (TBufferListRef const*)&pCtx->ListRef, &pBufs->tListVirtRef, &pBufs->tListRef, &pCtx->POC, &pCtx->MV, &pBufs->tWP, &pCtx->pRecs);

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, pSP->DependentSlice);

  if(pSP->FirstLcuSliceSegment)
  {
    pSP->FirstLcuSlice = pSP->DependentSlice ? pPrevSP->FirstLcuSlice : pSP->FirstLcuSlice;
    pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->uCurTileID;
  }

  AL_sSaveNalStreamBlk1(pCtx, pSP);

  if(bIsLastVclNalInAU)
    AL_TerminateCurrentCommand(pCtx, pPP, pSP);
}

/*@}*/

