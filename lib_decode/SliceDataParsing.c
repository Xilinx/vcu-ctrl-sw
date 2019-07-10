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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include <assert.h>

#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferBufHandleMeta.h"

#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecHwScalingList.h"
#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Hevc_PictMngr.h"

#include "lib_decode/I_DecChannel.h"
#include "SliceDataParsing.h"
#include "I_DecoderCtx.h"
#include "FrameParam.h"


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
static void AL_sSaveCommandBlk2(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs)
{
  (void)pPP;

  int const iMaxBitDepth = pCtx->tStreamSettings.iBitDepth;
  AL_TBuffer* pRec = pCtx->pRecs.pFrame;
  AL_TSrcMetaData* pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pRec, AL_META_TYPE_SOURCE);
  assert(pRecMeta);
  uint16_t uPitch = pRecMeta->tPlanes[AL_PLANE_Y].iPitch;

  uint32_t const u10BitsFlag = (iMaxBitDepth == 8) ? 0x00000000 : 0x80000000;
  pBufs->uPitch = uPitch | u10BitsFlag;

  /* put addresses */
  AL_TAllocator* pRecAllocator = pRec->pAllocator;
  AL_HANDLE hRecHandle = pRec->hBuf;
  uint8_t* pRecData = AL_Buffer_GetData(pRec);

  pBufs->tRecY.tMD.uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pRecAllocator, hRecHandle);
  pBufs->tRecY.tMD.pVirtualAddr = pRecData;

  int const iOffsetC = pRecMeta->tPlanes[AL_PLANE_UV].iOffset;
  pBufs->tRecC.tMD.uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pRecAllocator, hRecHandle) + iOffsetC;
  pBufs->tRecC.tMD.pVirtualAddr = pRecData + iOffsetC;



  pBufs->tPoc.tMD.uPhysicalAddr = pCtx->POC.tMD.uPhysicalAddr;
  pBufs->tPoc.tMD.pVirtualAddr = pCtx->POC.tMD.pVirtualAddr;

  pBufs->tMV.tMD.uPhysicalAddr = pCtx->MV.tMD.uPhysicalAddr;
  pBufs->tMV.tMD.pVirtualAddr = pCtx->MV.tMD.pVirtualAddr;
}

/*****************************************************************************/
static void AL_sSaveNalStreamBlk1(AL_TDecCtx* pCtx)
{
  pCtx->NalStream = pCtx->Stream;
}

/*****************************************************************************/
static AL_TDecPicBufferAddrs AL_SetBufferAddrs(AL_TDecCtx* pCtx)
{
  AL_TDecPicBuffers* pPictBuffers = &pCtx->PoolPB[pCtx->uToggle];
  AL_TDecPicBufferAddrs BufAddrs;

  BufAddrs.pCompData = pPictBuffers->tCompData.tMD.uPhysicalAddr;
  BufAddrs.pCompMap = pPictBuffers->tCompMap.tMD.uPhysicalAddr;
  BufAddrs.pListRef = pPictBuffers->tListRef.tMD.uPhysicalAddr;
  BufAddrs.pMV = pPictBuffers->tMV.tMD.uPhysicalAddr;
  BufAddrs.pPoc = pPictBuffers->tPoc.tMD.uPhysicalAddr;
  BufAddrs.pRecY = pPictBuffers->tRecY.tMD.uPhysicalAddr;
  BufAddrs.pRecC = pPictBuffers->tRecC.tMD.uPhysicalAddr;
  BufAddrs.pRecFbcMapY = pCtx->chanParam.bFrameBufferCompression ? pPictBuffers->tRecFbcMapY.tMD.uPhysicalAddr : 0;
  BufAddrs.pRecFbcMapC = pCtx->chanParam.bFrameBufferCompression ? pPictBuffers->tRecFbcMapC.tMD.uPhysicalAddr : 0;
  BufAddrs.pScl = pPictBuffers->tScl.tMD.uPhysicalAddr;
  BufAddrs.pWP = pPictBuffers->tWP.tMD.uPhysicalAddr;
  BufAddrs.pStream = pPictBuffers->tStream.tMD.uPhysicalAddr;

  assert(pPictBuffers->tStream.tMD.uSize > 0);
  BufAddrs.uStreamSize = pPictBuffers->tStream.tMD.uSize;
  BufAddrs.uPitch = pPictBuffers->uPitch;


  return BufAddrs;
}

static void SetBufferHandleMetaData(AL_TDecCtx* pCtx)
{
  if(!pCtx->pInputBuffer)
    return;

  AL_TBufHandleMetaData* pMeta = (AL_TBufHandleMetaData*)AL_Buffer_GetMetaData(pCtx->pRecs.pFrame, AL_META_TYPE_BUFHANDLE);

  if(!pMeta)
  {
    pMeta = AL_BufHandleMetaData_Create(AL_MAX_SLICES_SUBFRAME);
    AL_Buffer_AddMetaData(pCtx->pRecs.pFrame, (AL_TMetaData*)pMeta);
  }
  AL_BufHandleMetaData_AddHandle(pMeta, pCtx->pInputBuffer);
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
  AL_IDecChannel_DecodeOneSlice(pCtx->pDecChannel, &pCtx->PoolPP[pCtx->uToggle], pBufAddrs, &SliceParam);
}

/*****************************************************************************/
void AL_LaunchSliceDecoding(AL_TDecCtx* pCtx, bool bIsLastAUNal, bool hasPreviousSlice)
{
  uint16_t uSliceID = pCtx->PictMngr.uNumSlice - 1;
  AL_TDecSliceParam* pPrevSP = NULL;


  UpdateStreamOffset(pCtx);

  if(hasPreviousSlice && uSliceID)
  {
    pPrevSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]);
    decodeOneSlice(pCtx, uSliceID - 1, &pCtx->BufAddrs);
  }

  pCtx->BufAddrs = AL_SetBufferAddrs(pCtx);
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
  AL_TDecPicBufferAddrs BufAddrs = AL_SetBufferAddrs(pCtx);


  UpdateStreamOffset(pCtx);
  SetBufferHandleMetaData(pCtx);

  AL_IDecChannel_DecodeOneFrame(pCtx->pDecChannel, &pCtx->PoolPP[pCtx->uToggle], &BufAddrs, &pCtx->PoolSP[pCtx->uToggle].tMD);

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
  AL_CleanupMemory(pBufs->tCompData.tMD.pVirtualAddr, pBufs->tCompData.tMD.uSize);
  AL_CleanupMemory(pBufs->tCompMap.tMD.pVirtualAddr, pBufs->tCompMap.tMD.uSize);
}

/*****************************************************************************/
bool AL_InitFrameBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs, AL_TDimension tDim, AL_TDecPicParam* pPP)
{
  Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);

  if(!AL_PictMngr_BeginFrame(&pCtx->PictMngr, tDim))
  {
    pCtx->eChanState = CHAN_DESTROYING;
    Rtos_ReleaseSemaphore(pCtx->Sem);
    return false;
  }
  pPP->FrmID = AL_PictMngr_GetCurrentFrmID(&pCtx->PictMngr);
  pPP->MvID = AL_PictMngr_GetCurrentMvID(&pCtx->PictMngr);

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
static void AL_TerminateCurrentCommand(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP)
{
  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];

  pSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
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
void AL_TerminatePreviousCommand(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, bool bIsLastVclNalInAU, bool bNextIsDependent)
{
  AL_TDecPicBuffers* pBufs = &pCtx->PoolPB[pCtx->uToggle];
  AL_sSaveCommandBlk2(pCtx, pPP, pBufs);

  if(pCtx->PictMngr.uNumSlice == 0)
    return;

  AL_TDecSliceParam* pPrevSP = &(((AL_TDecSliceParam*)pCtx->PoolSP[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->PictMngr.uNumSlice - 1]);

  if(bIsLastVclNalInAU)
    pPrevSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
  else
    pPrevSP->NextSliceSegment = pSP->FirstLcuSliceSegment;

  pPrevSP->NextIsDependent = bNextIsDependent;

  if(!pCtx->tConceal.bValidFrame)
  {
    pPrevSP->eSliceType = AL_SLICE_CONCEAL;
    pPrevSP->FirstLCU = 0;
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
    AL_AVC_WriteDecHwScalingList((AL_TScl const*)pSCL, pBufs->tScl.tMD.pVirtualAddr);
  AL_AVC_PictMngr_GetBuffers(&pCtx->PictMngr, pSP, pSlice, &pCtx->ListRef, &pBufs->tListVirtRef, &pBufs->tListRef, &pCtx->POC, &pCtx->MV, &pBufs->tWP, &pCtx->pRecs);

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, true);

  if(pSP->FirstLCU)
    pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->uCurTileID;

  AL_sSaveNalStreamBlk1(pCtx);

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
  AL_HEVC_PictMngr_GetBuffers(&pCtx->PictMngr, pSP, pSlice, &pCtx->ListRef, &pBufs->tListVirtRef, &pBufs->tListRef, &pCtx->POC, &pCtx->MV, &pBufs->tWP, &pCtx->pRecs);

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, pSP->DependentSlice);

  if(pSP->FirstLcuSliceSegment)
  {
    pSP->FirstLcuSlice = pSP->DependentSlice ? pPrevSP->FirstLcuSlice : pSP->FirstLcuSlice;
    pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->uCurTileID;
  }

  AL_sSaveNalStreamBlk1(pCtx);

  if(bIsLastVclNalInAU)
    AL_TerminateCurrentCommand(pCtx, pPP, pSP);
}

/*@}*/

