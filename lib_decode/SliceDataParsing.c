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

#include <assert.h>

#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecHwScalingList.h"
#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Hevc_PictMngr.h"

#include "lib_decode/I_DecChannel.h"
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
  const int toggle = pCtx->m_uToggle;
  setBufferHandle(&pCtx->m_PoolListRefAddr[toggle], &pBufs->tListRef);
  setBufferHandle(&pCtx->m_PoolCompData[toggle], &pBufs->tCompData);
  setBufferHandle(&pCtx->m_PoolCompMap[toggle], &pBufs->tCompMap);
  setBufferHandle(&pCtx->m_PoolSclLst[toggle], &pBufs->tScl);
  setBufferHandle(&pCtx->m_PoolWP[toggle], &pBufs->tWP);
}

/******************************************************************************/
static void pushCommandParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP, bool bIsLastAUNal)
{
  pSP->bIsLastSlice = bIsLastAUNal;

  /* update circular buffer */
  pSP->uStrAvailSize = pCtx->m_NalStream.uAvailSize;
  pSP->uStrOffset = pCtx->m_NalStream.uOffset;
}

/******************************************************************************/
static void AL_sSaveCommandBlk2(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs)
{
  AL_TDimension const tDim = { pPP->PicWidth * 8, pPP->PicHeight * 8 };
  int const iMaxBitDepth = pPP->MaxBitDepth;
  AL_EFbStorageMode const eStorage = pCtx->m_chanParam.eFBStorageMode;
  uint16_t const uPitch = RndPitch(tDim.iWidth, iMaxBitDepth, eStorage);
  uint32_t const u10BitsFlag = (iMaxBitDepth == 8) ? 0x00000000 : 0x80000000;
  pBufs->uPitch = uPitch | u10BitsFlag;

  AL_TBuffer* pRec = pCtx->m_pRec;
  uint8_t* pRecData = AL_Buffer_GetData(pCtx->m_pRec);
  AL_TAllocator* pRecAllocator = pRec->pAllocator;
  AL_HANDLE hRecHandle = pRec->hBuf;

  /* put addresses */
  pBufs->tRecY.tMD.uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pRecAllocator, hRecHandle);
  pBufs->tRecY.tMD.pVirtualAddr = pRecData;

  int const iLumaSize = AL_GetAllocSize_DecReference(tDim, CHROMA_MONO, iMaxBitDepth, eStorage);
  pBufs->tRecC.tMD.uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pRecAllocator, hRecHandle) + iLumaSize;
  pBufs->tRecC.tMD.pVirtualAddr = pRecData + iLumaSize;

  pBufs->tPoc.tMD.uPhysicalAddr = pCtx->m_pPOC->tMD.uPhysicalAddr;
  pBufs->tPoc.tMD.pVirtualAddr = pCtx->m_pPOC->tMD.pVirtualAddr;

  pBufs->tMV.tMD.uPhysicalAddr = pCtx->m_pMV->tMD.uPhysicalAddr;
  pBufs->tMV.tMD.pVirtualAddr = pCtx->m_pMV->tMD.pVirtualAddr;

}

/*****************************************************************************/
static void AL_sSaveNalStreamBlk1(AL_TDecCtx* pCtx)
{
  pCtx->m_NalStream = pCtx->m_Stream;
}

/*****************************************************************************/
static AL_TDecPicBufferAddrs AL_SetBufferAddrs(AL_TDecCtx* pCtx)
{
  AL_TDecPicBuffers* pPictBuffers = &pCtx->m_PoolPB[pCtx->m_uToggle];
  AL_TDecPicBufferAddrs BufAddrs;

  BufAddrs.pCompData = pPictBuffers->tCompData.tMD.uPhysicalAddr;
  BufAddrs.pCompMap = pPictBuffers->tCompMap.tMD.uPhysicalAddr;
  BufAddrs.pListRef = pPictBuffers->tListRef.tMD.uPhysicalAddr;
  BufAddrs.pMV = pPictBuffers->tMV.tMD.uPhysicalAddr;
  BufAddrs.pPoc = pPictBuffers->tPoc.tMD.uPhysicalAddr;
  BufAddrs.pRecY = pPictBuffers->tRecY.tMD.uPhysicalAddr;
  BufAddrs.pRecC = pPictBuffers->tRecC.tMD.uPhysicalAddr;
  BufAddrs.pRecFbcMapY = pCtx->m_chanParam.bFrameBufferCompression ? pPictBuffers->tRecFbcMapY.tMD.uPhysicalAddr : 0;
  BufAddrs.pRecFbcMapC = pCtx->m_chanParam.bFrameBufferCompression ? pPictBuffers->tRecFbcMapC.tMD.uPhysicalAddr : 0;
  BufAddrs.pScl = pPictBuffers->tScl.tMD.uPhysicalAddr;
  BufAddrs.pWP = pPictBuffers->tWP.tMD.uPhysicalAddr;
  BufAddrs.pStream = pPictBuffers->tStream.tMD.uPhysicalAddr;

  assert(pPictBuffers->tStream.tMD.uSize > 0);
  BufAddrs.uStreamSize = pPictBuffers->tStream.tMD.uSize;
  BufAddrs.uPitch = pPictBuffers->uPitch;

  return BufAddrs;
}

/***************************************************************************/
/*                          P U B L I C   f u n c t i o n s                */
/***************************************************************************/

static void UpdateStreamOffset(AL_TDecCtx* pCtx)
{
  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = (pCtx->m_Stream.uOffset + pCtx->m_Stream.uAvailSize) % pCtx->m_Stream.tMD.uSize;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
}

/*****************************************************************************/
void AL_LaunchSliceDecoding(AL_TDecCtx* pCtx, bool bIsLastAUNal)
{
  AL_TDecPicBufferAddrs BufAddrs = AL_SetBufferAddrs(pCtx);

  uint16_t uSliceID = pCtx->m_PictMngr.m_uNumSlice - 1;


  UpdateStreamOffset(pCtx);

  if(uSliceID)
  {
    AL_TDecSliceParam* pPrevSP_v = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[uSliceID - 1]);
    AL_PADDR pPrevSP_p = (AL_PADDR)(uintptr_t)&(((AL_TDecSliceParam*)(uintptr_t)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.uPhysicalAddr)[uSliceID - 1]);
    TMemDesc pPrevSP;
    pPrevSP.pVirtualAddr = (AL_VADDR)pPrevSP_v;
    pPrevSP.uPhysicalAddr = pPrevSP_p;
    AL_IDecChannel_DecodeOneSlice(pCtx->m_pDecChannel, &pCtx->m_PoolPP[pCtx->m_uToggle], &BufAddrs, &pPrevSP);
  }

  if(bIsLastAUNal)
  {
    AL_TDecSliceParam* pSP_v = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[uSliceID]);
    AL_PADDR pSP_p = (AL_PADDR)(uintptr_t)&(((AL_TDecSliceParam*)(uintptr_t)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.uPhysicalAddr)[uSliceID]);
    TMemDesc pSP;
    pSP.pVirtualAddr = (AL_VADDR)pSP_v;
    pSP.uPhysicalAddr = pSP_p;
    AL_IDecChannel_DecodeOneSlice(pCtx->m_pDecChannel, &pCtx->m_PoolPP[pCtx->m_uToggle], &BufAddrs, &pSP);

    pCtx->m_uCurTileID = 0;

    Rtos_GetMutex(pCtx->m_DecMutex);
    ++pCtx->m_iNumFrmBlk1;
    pCtx->m_uToggle = (pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize);
    Rtos_ReleaseMutex(pCtx->m_DecMutex);
  }
}

/*****************************************************************************/
void AL_LaunchFrameDecoding(AL_TDecCtx* pCtx)
{
  AL_TDecPicBufferAddrs BufAddrs = AL_SetBufferAddrs(pCtx);


  UpdateStreamOffset(pCtx);

  AL_IDecChannel_DecodeOneFrame(pCtx->m_pDecChannel, &pCtx->m_PoolPP[pCtx->m_uToggle], &BufAddrs, &pCtx->m_PoolSP[pCtx->m_uToggle].tMD);

  pCtx->m_uCurTileID = 0;

  Rtos_GetMutex(pCtx->m_DecMutex);
  ++pCtx->m_iNumFrmBlk1;
  pCtx->m_uToggle = (pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize);
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
}

/*****************************************************************************/
static void AL_InitIntermediateBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs)
{
  int iOffset = pCtx->m_iNumFrmBlk1 % MAX_STACK_SIZE;
  pCtx->m_uNumRef[iOffset] = 0;

  for(uint8_t uNode = 0; uNode < MAX_REF; ++uNode)
  {
    AL_EMarkingRef eMarkingRef = AL_Dpb_GetMarkingFlag(&pCtx->m_PictMngr.m_DPB, uNode);
    uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->m_PictMngr.m_DPB, uNode);

    if(eMarkingRef != UNUSED_FOR_REF && eMarkingRef != NON_EXISTING_REF && uMvID != uEndOfList)
    {
      pCtx->m_uMvIDRefList[iOffset][pCtx->m_uNumRef[iOffset]] = uMvID;
      ++pCtx->m_uNumRef[iOffset];
    }
  }

  AL_PictMngr_LockRefMvID(&pCtx->m_PictMngr, pCtx->m_uNumRef[iOffset], pCtx->m_uMvIDRefList[iOffset]);

  // prepare buffers
  AL_sGetToggleBuffers(pCtx, pBufs);
  AL_CleanupMemory(pBufs->tCompData.tMD.pVirtualAddr, pBufs->tCompData.tMD.uSize);
  AL_CleanupMemory(pBufs->tCompMap.tMD.pVirtualAddr, pBufs->tCompMap.tMD.uSize);
}

/*****************************************************************************/
bool AL_InitFrameBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs, AL_TDimension tDim, AL_TDecPicParam* pPP)
{
  Rtos_GetSemaphore(pCtx->m_Sem, AL_WAIT_FOREVER);

  if(!AL_PictMngr_BeginFrame(&pCtx->m_PictMngr, tDim))
  {
    Rtos_ReleaseSemaphore(pCtx->m_Sem);
    return false;
  }
  pPP->FrmID = AL_PictMngr_GetCurrentFrmID(&pCtx->m_PictMngr);
  pPP->MvID = AL_PictMngr_GetCurrentMvID(&pCtx->m_PictMngr);
  AL_InitIntermediateBuffers(pCtx, pBufs);
  return true;
}

/*****************************************************************************/
void AL_CancelFrameBuffers(AL_TDecCtx* pCtx)
{
  AL_PictMngr_CancelFrame(&pCtx->m_PictMngr);

  int iOffset = pCtx->m_iNumFrmBlk1 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefMvID(&pCtx->m_PictMngr, pCtx->m_uNumRef[iOffset], pCtx->m_uMvIDRefList[iOffset]);
  Rtos_ReleaseSemaphore(pCtx->m_Sem);
}

/*****************************************************************************/
static void AL_TerminateCurrentCommand(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP)
{
  AL_TDecPicBuffers* pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];

  pSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
  pSP->NextIsDependent = false;

  AL_sSaveCommandBlk2(pCtx, pPP, pBufs);
  pushCommandParameters(pCtx, pSP, true);
}

/*****************************************************************************/
void AL_SetConcealParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP)
{
  pSP->ConcealPicID = AL_PictMngr_GetLastPicID(&pCtx->m_PictMngr);
  pSP->ValidConceal = (pSP->ConcealPicID == uEndOfList) ? false : true;
}

/*****************************************************************************/
void AL_TerminatePreviousCommand(AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, bool bIsLastVclNalInAU, bool bNextIsDependent)
{
  if(pCtx->m_PictMngr.m_uNumSlice)
  {
    AL_TDecSliceParam* pPrevSP = &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[pCtx->m_PictMngr.m_uNumSlice - 1]);
    AL_TDecPicBuffers* pBufs = &pCtx->m_PoolPB[pCtx->m_uToggle];

    AL_sSaveCommandBlk2(pCtx, pPP, pBufs);

    if(bIsLastVclNalInAU)
      pPrevSP->NextSliceSegment = pPP->LcuWidth * pPP->LcuHeight;
    else
      pPrevSP->NextSliceSegment = pSP->FirstLcuSliceSegment;

    pPrevSP->NextIsDependent = bNextIsDependent;

    if(!pCtx->m_tConceal.m_bValidFrame)
    {
      pPrevSP->eSliceType = SLICE_CONCEAL;
      pPrevSP->FirstLCU = 0;
    }
    pushCommandParameters(pCtx, pPrevSP, bIsLastVclNalInAU);
  }
}

/*****************************************************************************/
void AL_AVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->m_PictMngr.m_uNumSlice;

  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPP->iFrmNum = pCtx->m_iNumFrmBlk1;
  pPP->UserParam = pCtx->m_uToggle;

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    AL_TerminatePreviousCommand(pCtx, pPP, pSP, bIsLastVclNalInAU, true);
  }
  else
  {
    // copy collocated info
    if(pSP->FirstLcuSliceSegment && pSP->eSliceType == SLICE_I)
      pSP->ColocPicID = pPrevSP->ColocPicID;

    if(!pSlice->first_mb_in_slice)
      AL_AVC_WriteDecHwScalingList((AL_TScl const*)pSCL, pBufs->tScl.tMD.pVirtualAddr);
    AL_AVC_PictMngr_GetBuffers(&pCtx->m_PictMngr, pPP, pSP, pSlice, &pCtx->m_ListRef, &pBufs->tListRef, &pCtx->m_pPOC, &pCtx->m_pMV, &pBufs->tWP, &pCtx->m_pRec);

    // stock command registers in memory
    if(pSP->FirstLCU)
    {
      AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, true);
      pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->m_uCurTileID;
    }

    AL_sSaveNalStreamBlk1(pCtx);

    if(bIsLastVclNalInAU)
      AL_TerminateCurrentCommand(pCtx, pPP, pSP);
  }
}

/*****************************************************************************/
void AL_HEVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->m_PictMngr.m_uNumSlice;
  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->m_PoolSP[pCtx->m_uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPP->iFrmNum = pCtx->m_iNumFrmBlk1;
  pPP->UserParam = pCtx->m_uToggle;

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    AL_TerminatePreviousCommand(pCtx, pPP, pSP, bIsLastVclNalInAU, true);
  }
  else
  {
    // copy collocated info
    if(pSP->FirstLcuSliceSegment && pSP->eSliceType == SLICE_I)
      pSP->ColocPicID = pPrevSP->ColocPicID;

    if(pSlice->first_slice_segment_in_pic_flag)
      AL_HEVC_WriteDecHwScalingList((const AL_TScl*)pSCL, pBufs->tScl.tMD.pVirtualAddr);
    AL_HEVC_PictMngr_GetBuffers(&pCtx->m_PictMngr, pPP, pSP, pSlice, &pCtx->m_ListRef, &pBufs->tListRef, &pCtx->m_pPOC, &pCtx->m_pMV, &pBufs->tWP, &pCtx->m_pRec);

    // stock command registers in memory
    if(pSP->FirstLcuSliceSegment)
    {
      AL_TerminatePreviousCommand(pCtx, pPP, pSP, false, pSP->DependentSlice);

      pSP->FirstLcuSlice = pSP->DependentSlice ? pPrevSP->FirstLcuSlice : pSP->FirstLcuSlice;
      pSP->FirstLcuTileID = pSP->DependentSlice ? pPrevSP->FirstLcuTileID : pCtx->m_uCurTileID;
    }

    AL_sSaveNalStreamBlk1(pCtx);

    if(bIsLastVclNalInAU)
      AL_TerminateCurrentCommand(pCtx, pPP, pSP);
  }
}

/*@}*/

