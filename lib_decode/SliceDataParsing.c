// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "SliceDataParsing.h"
#include "I_DecoderCtx.h"
#include "NalUnitParser.h"

#include "lib_decode/lib_decode.h"
#include "lib_decode/I_DecScheduler.h"

#include "lib_common/PixMapBufferInternal.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/Utils.h"

#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/Avc_PictMngr.h"
#include "lib_decode/AvcHwBufInitialization.h"
#include "lib_parsing/Hevc_PictMngr.h"
#include "lib_decode/HevcHwBufInitialization.h"

/******************************************************************************/
static void setBufferHandle(const TBuffer* in, TBuffer* out)
{
  out->tMD.uPhysicalAddr = in->tMD.uPhysicalAddr;
  out->tMD.pVirtualAddr = in->tMD.pVirtualAddr;
  out->tMD.uSize = in->tMD.uSize;
}

/******************************************************************************/
static void AL_sGetToggleBuffers(const AL_TDecCtx* pCtx, AL_TDecBuffers* pPicBuffers)
{
  const int32_t toggle = pCtx->uToggle;
  setBufferHandle(&pCtx->PoolListRefAddr[toggle], &pPicBuffers->tListRef);
  setBufferHandle(&pCtx->PoolVirtRefAddr[toggle], &pPicBuffers->tListVirtRef);
  setBufferHandle(&pCtx->PoolCompData[toggle], &pPicBuffers->tCompData);
  setBufferHandle(&pCtx->PoolCompMap[toggle], &pPicBuffers->tCompMap);
  setBufferHandle(&pCtx->PoolSclLst[toggle], &pPicBuffers->tScl);
  setBufferHandle(&pCtx->PoolWP[toggle], &pPicBuffers->tWP);
}

/******************************************************************************/
static void pushCommandParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam, bool bIsLastAUNal)
{
  pSliceParam->bIsLastSlice = bIsLastAUNal;

  /* update circular buffer */
  pSliceParam->uStrAvailSize = pCtx->NalStream.iAvailSize;
  pSliceParam->uStrOffset = pCtx->NalStream.iOffset;
}

/******************************************************************************/
static void AL_sSaveCommandBlk2(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPicParam, AL_TDecBuffers* pPicBuffers)
{
  (void)pPicParam;

  AL_TBuffer* pRec = pCtx->pRecs.pFrame;

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pRec);
  AL_EPlaneMode ePlaneMode = AL_GetPlaneMode(tFourCC);
  AL_EChromaMode eChromaMode = AL_GetChromaMode(tFourCC);
  (void)eChromaMode;

  pPicBuffers->uBitdepth = AL_GetBitDepth(tFourCC);
  pPicBuffers->uPitchY = AL_PixMapBuffer_GetPlanePitch(pRec, AL_PLANE_Y);

  Rtos_Assert(pPicBuffers->uPitchY != 0);

  pPicBuffers->tRecY.tMD.uPhysicalAddr = AL_PixMapBuffer_GetPlanePhysicalAddress(pRec, AL_PLANE_Y);
  pPicBuffers->tRecY.tMD.pVirtualAddr = AL_PixMapBuffer_GetPlaneAddress(pRec, AL_PLANE_Y);
  Rtos_Assert(pPicBuffers->tRecY.tMD.uPhysicalAddr != 0 &&
              "Luma plane is not set (buffer does not fit requirement)");

  AL_EPlaneId eFirstCPlane = AL_PLANE_MODE_PLANAR == ePlaneMode ? AL_PLANE_U : AL_PLANE_UV;
  pPicBuffers->uPitchC = AL_PixMapBuffer_GetPlanePitch(pRec, eFirstCPlane);
  Rtos_Assert(pPicBuffers->uPitchC != 0 || ePlaneMode == AL_PLANE_MODE_INTERLEAVED || eChromaMode == AL_CHROMA_4_0_0);

  pPicBuffers->tRecC1.tMD.uPhysicalAddr = AL_PixMapBuffer_GetPlanePhysicalAddress(pRec, eFirstCPlane);
  pPicBuffers->tRecC1.tMD.pVirtualAddr = AL_PixMapBuffer_GetPlaneAddress(pRec, eFirstCPlane);
  Rtos_Assert((eChromaMode == AL_CHROMA_4_0_0 || pPicBuffers->tRecC1.tMD.uPhysicalAddr != 0) &&
              "First chroma plane is not set (buffer does not fit requirement)");

  uint32_t uOffset = AL_PixMapBuffer_GetPositionOffset(pRec, pCtx->tOutputPosition, AL_PLANE_Y);
  pPicBuffers->tRecY.tMD.uPhysicalAddr += uOffset;
  pPicBuffers->tRecY.tMD.pVirtualAddr += uOffset;
  uOffset = AL_PixMapBuffer_GetPositionOffset(pRec, pCtx->tOutputPosition, eFirstCPlane);
  pPicBuffers->tRecC1.tMD.uPhysicalAddr += uOffset;
  pPicBuffers->tRecC1.tMD.pVirtualAddr += uOffset;

}

/*****************************************************************************/
void AL_SaveNalStreamBlk1(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  (void)pSliceParam;
  pCtx->NalStream = *(pCtx->pStream);
}

/*****************************************************************************/
static void AL_FlushBuffers(AL_TDecCtx* pCtx)
{
  AL_TDecBuffers* pPictBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];

  Rtos_FlushCacheMemory(pPictBuffers->tListRef.tMD.pVirtualAddr, pPictBuffers->tListRef.tMD.uSize);

  bool bFlushScalingList = false;

  bFlushScalingList |= AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec);

  if(bFlushScalingList)
    Rtos_FlushCacheMemory(pPictBuffers->tScl.tMD.pVirtualAddr, pPictBuffers->tScl.tMD.uSize);

  bool bFlushPOC = false;

  bFlushPOC |= AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec);

  if(bFlushPOC)
    Rtos_FlushCacheMemory(pPictBuffers->tPoc.tMD.pVirtualAddr, pPictBuffers->tPoc.tMD.uSize);

  bool bFlushWP = !pCtx->bIntraOnlyProfile;

  bFlushWP &= AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec);

  if(bFlushWP)
    Rtos_FlushCacheMemory(pPictBuffers->tWP.tMD.pVirtualAddr, pPictBuffers->tWP.tMD.uSize);

  // Stream buffer was already flushed for start-code detection.
}

static void SetBufferHandleMetaData(AL_TDecCtx* pCtx, bool bConceal)
{
  if(!pCtx->pInputBuffer)
    return;

  if(pCtx->pInputBuffer == pCtx->eosBuffer)
    return;

  AL_TBuffer* pFrame = pCtx->pRecs.pFrame;

  AL_THandleMetaData* pMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_HANDLE);

  if(!pMeta)
  {
    pMeta = AL_HandleMetaData_Create(AL_MAX_SLICES_SUBFRAME, sizeof(AL_TDecMetaHandle));
    AL_Buffer_AddMetaData(pFrame, (AL_TMetaData*)pMeta);
  }

  AL_EDecHandleState eState = bConceal ? AL_DEC_HANDLE_STATE_CONCEALING : AL_DEC_HANDLE_STATE_PROCESSING;
  AL_TDecMetaHandle handle = { eState, pCtx->pInputBuffer };
  AL_HandleMetaData_AddHandle(pMeta, &handle);
}

/***************************************************************************/
/*                          P U B L I C   f u n c t i o n s                */
/***************************************************************************/

static void UpdateStreamOffset(AL_TDecCtx* pCtx)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = (pCtx->pStream->iOffset + pCtx->pStream->iAvailSize) % pCtx->pStream->tMD.uSize;
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

static void decodeOneSlice(AL_TDecCtx* pCtx, uint16_t uSliceID, AL_TDecBuffers* pPictBuffers)
{
  AL_TDecSliceParam* pSP_v = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID]);
  AL_PADDR pSP_p = (AL_PADDR)(uintptr_t)&(((AL_TDecSliceParam*)(uintptr_t)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.uPhysicalAddr)[uSliceID]);
  AL_TMemDesc SliceParam;
  SliceParam.pVirtualAddr = (AL_VADDR)pSP_v;
  SliceParam.uPhysicalAddr = pSP_p;
  // The HandleMetaData handle order should be the same as the slice order
  // as we add them each time we send one.
  pSP_v->uParsingId = uSliceID;
  AL_IDecScheduler_DecodeOneSlice(pCtx->pScheduler, pCtx->hChannel, &pCtx->tPoolPicParams[pCtx->uToggle], pPictBuffers, &SliceParam);
  pCtx->tConceal.iSliceDecodeIssued++;
}

/*****************************************************************************/

/*****************************************************************************/
void AL_LaunchSliceDecoding(AL_TDecCtx* pCtx, bool bIsLastAUNal, bool hasPreviousSlice)
{
  uint16_t uSliceID = pCtx->tCurrentFrameCtx.uNumSlice - 1;
  AL_TDecSliceParam* pPrevSP = NULL;

  UpdateStreamOffset(pCtx);

  if(hasPreviousSlice && uSliceID)
  {
    pPrevSP = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]);

    if(pPrevSP->eSliceType == AL_SLICE_CONCEAL && uSliceID == 1)
    {
      AL_FlushBuffers(pCtx);
      pCtx->PictBuffers = pCtx->tPoolPicBuffers[pCtx->uToggle];
      SetBufferHandleMetaData(pCtx, true);
    }
    decodeOneSlice(pCtx, uSliceID - 1, &pCtx->PictBuffers);
  }

  AL_FlushBuffers(pCtx);
  pCtx->PictBuffers = pCtx->tPoolPicBuffers[pCtx->uToggle];

  if((pCtx->tConceal.iSliceDecodeIssued > 0) && (!hasPreviousSlice))
    SetBufferHandleMetaData(pCtx, true); /* multiple slice decode in the same NAL, so need to control buffer release*/
  else
    SetBufferHandleMetaData(pCtx, false);

  if(!bIsLastAUNal)
    return;

  if(pPrevSP == NULL || !pPrevSP->bIsLastSlice)
    decodeOneSlice(pCtx, uSliceID, &pCtx->PictBuffers);

  pCtx->tCurrentFrameCtx.uCurTileID = 0;

  Rtos_GetMutex(pCtx->DecMutex);
  ++pCtx->iNumFrmBlk1;
  pCtx->uToggle = (pCtx->iNumFrmBlk1 % pCtx->iStackSize);
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

/*****************************************************************************/
void AL_LaunchFrameDecoding(AL_TDecCtx* pCtx)
{
  AL_FlushBuffers(pCtx);

  AL_TDecBuffers* pPictBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];
  SetBufferHandleMetaData(pCtx, false);

  UpdateStreamOffset(pCtx);
  AL_TDecSliceParam* pSliceParam = (AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr;
  pSliceParam->uParsingId = 0;

  // ToDo : #if AL_ENABLE_ARCH_WITHOUT_SCHEDULER
  if(pCtx->pScheduler)
  {
    AL_IDecScheduler_DecodeOneFrame(pCtx->pScheduler, pCtx->hChannel, &pCtx->tPoolPicParams[pCtx->uToggle], pPictBuffers, &pCtx->tPoolSliceParams[pCtx->uToggle].tMD);
  }

  pCtx->tCurrentFrameCtx.uCurTileID = 0;

  Rtos_GetMutex(pCtx->DecMutex);
  ++pCtx->iNumFrmBlk1;
  pCtx->uToggle = (pCtx->iNumFrmBlk1 % pCtx->iStackSize);
  Rtos_ReleaseMutex(pCtx->DecMutex);
}

/*****************************************************************************/
static void AL_InitRefBuffers(AL_TDecCtx* pCtx, AL_TDecBuffers* pPicBuffers)
{
  int32_t iOffset = pCtx->iNumFrmBlk1 % AL_DEC_SW_MAX_STACK_SIZE;
  pCtx->uNumRef[iOffset] = 0;

  if(AL_IS_ITU_CODEC(pCtx->pChanParam->eCodec))
  {
    AL_TIndex tNodeID = 0;
    AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

    while(tNodeID < AL_REF_MNGR_MAX_BUF_SIZE && pCtx->uNumRef[iOffset] < AL_MAX_REF)
    {
      AL_EMarkingRef eMarkingRef = AL_Dpb_GetMarkingFlag(pDpb, tNodeID);
      AL_TIndex tFrameId = AL_Dpb_GetFrmID_FromNode(pDpb, tNodeID);
      AL_TIndex tMvID = AL_Dpb_GetMvID_FromNode(pDpb, tNodeID);

      if(IS_NODE_VALID(tFrameId) && IS_NODE_VALID(tMvID) && (eMarkingRef != UNUSED_FOR_REF) && (eMarkingRef != NON_EXISTING_REF))
      {
        pCtx->tFrameIDRefList[iOffset][pCtx->uNumRef[iOffset]] = tFrameId;
        pCtx->tMvIDRefList[iOffset][pCtx->uNumRef[iOffset]] = tMvID;
        ++pCtx->uNumRef[iOffset];
      }

      ++tNodeID;
    }

    AL_PictMngr_LockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->tFrameIDRefList[iOffset], pCtx->tMvIDRefList[iOffset]);
  }

  // prepare buffers
  AL_sGetToggleBuffers(pCtx, pPicBuffers);
  {
    AL_CleanupMemory(pPicBuffers->tCompData.tMD.pVirtualAddr, pPicBuffers->tCompData.tMD.uSize);
    AL_CleanupMemory(pPicBuffers->tCompMap.tMD.pVirtualAddr, pPicBuffers->tCompMap.tMD.uSize);
  }
}

/*****************************************************************************/
bool AL_InitFrameBuffers(AL_TDecCtx* pCtx, AL_TDecBuffers* pPicBuffers, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eDecodedChromaMode, AL_TDecPicParam* pPicParam)
{
  Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);

  if(!AL_PictMngr_BeginFrame(&pCtx->PictMngr, bStartsNewCVS, tDim, eDecodedChromaMode))
  {
    pCtx->eChanState = CHAN_DESTROYING;
    Rtos_ReleaseSemaphore(pCtx->Sem);
    return false;
  }
  pPicParam->tBufIDs.tFrmID = AL_PictMngr_GetCurrentFrmID(&pCtx->PictMngr);
  pPicParam->tBufIDs.tMvID = AL_PictMngr_GetCurrentAnnexID(&pCtx->PictMngr);

  AL_InitRefBuffers(pCtx, pPicBuffers);

  pCtx->tCurrentFrameCtx.eBufStatus = DEC_FRAME_BUF_RESERVED;

  return true;
}

/*****************************************************************************/
void AL_CancelFrameBuffers(AL_TDecCtx* pCtx)
{
  AL_PictMngr_CancelFrame(&pCtx->PictMngr);

  int32_t iOffset = pCtx->iNumFrmBlk1 % AL_DEC_SW_MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->tFrameIDRefList[iOffset], pCtx->tMvIDRefList[iOffset]);
  UpdateContextAtEndOfFrame(pCtx);
  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
void AL_TerminateCurrentCommand(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPicParam, AL_TDecSliceParam* pSliceParam)
{
  AL_TDecBuffers* pPicBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];

  pSliceParam->uNextSliceSegment = DecPicParam_GetNumLcuInFrame(pPicParam);
  pSliceParam->bNextIsDependent = false;

  AL_sSaveCommandBlk2(pCtx, pPicParam, pPicBuffers);
  pushCommandParameters(pCtx, pSliceParam, true);
}

/*****************************************************************************/
void AL_SetConcealParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  pSliceParam->tConcealPicID = AL_PictMngr_GetLastPicID(&pCtx->PictMngr);
  pSliceParam->bValidConceal = IS_NODE_VALID(pSliceParam->tConcealPicID);
}

/*****************************************************************************/
void AL_TerminatePreviousCommand(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPicParam, AL_TDecSliceParam* pSliceParam, AL_TDecBuffers* pPicBuffers, bool bIsLastVclNalInAU, bool bNextIsDependent)
{
  AL_sSaveCommandBlk2(pCtx, pPicParam, pPicBuffers);

  if(pCtx->tCurrentFrameCtx.uNumSlice == 0)
    return;

  AL_TDecSliceParam* pPrevSP = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->tCurrentFrameCtx.uNumSlice - 1]);

  if(bIsLastVclNalInAU)
    pPrevSP->uNextSliceSegment = DecPicParam_GetNumLcuInFrame(pPicParam);
  else
    pPrevSP->uNextSliceSegment = pSliceParam->uFirstLcuSliceSegment;

  pPrevSP->bNextIsDependent = bNextIsDependent;

  if(!pCtx->tConceal.bValidFrame)
  {
    pPrevSP->eSliceType = AL_SLICE_CONCEAL;
    pPrevSP->uSliceFirstLcu = 0;
  }
  pushCommandParameters(pCtx, pPrevSP, bIsLastVclNalInAU);
}

/******************************************************************************/
static void FillRefPicIDs(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
  TBufferListRef* pListRef = &pCtx->ListRef;

  // Reg 9 ~ C
  for(uint8_t uRef = 0; uRef < AL_MAX_REF; ++uRef)
  {
    AL_TIndex tNodeIDL0 = (*pListRef)[0][uRef].tNodeID;
    AL_TIndex tNodeIDL1 = (*pListRef)[1][uRef].tNodeID;

    pSliceParam->pPicIdL0s[uRef] = (tNodeIDL0 == AL_BAD_INDEX) ? 0x00 : AL_Dpb_GetPicID_FromNode(pDpb, tNodeIDL0);
    pSliceParam->pPicIdL1s[uRef] = (tNodeIDL1 == AL_BAD_INDEX) ? 0x00 : AL_Dpb_GetPicID_FromNode(pDpb, tNodeIDL1);
  }
}

/******************************************************************************/
static void FillConcealValue(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

  if(AL_IS_VALID_INDEX(pDpb->tLastPOCNodeID))
  {
    pSliceParam->bValidConceal = true;
    pSliceParam->tColocPicID = pDpb->Nodes[pDpb->tLastPOCNodeID].tPicID;
  }
  else
    pSliceParam->bValidConceal = false;
}

/*****************************************************************************/
void AL_AVC_InitHWCommandBuffers(AL_TDecCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_TAvcSliceHdr const* pSlice, AL_TScalingList const* pSclLst, AL_EChromaMode eChromaMode, AL_TDecBuffers* pPicBuffers)
{
  if(pCtx->tCurrentFrameCtx.eBufStatus == DEC_FRAME_BUF_RESERVED)
  {
    AL_AVC_PictMngr_GetBuffers(&pCtx->PictMngr, pSliceParam, &pCtx->pRecs, pPicBuffers);
    AL_AVC_InitHWFrameBuffers(pSclLst, eChromaMode, pPicBuffers);
    pCtx->tCurrentFrameCtx.eBufStatus |= DEC_FRAME_BUF_FILLED;
  }

  AL_AVC_InitHWSliceBuffers(pCtx->tCurrentFrameCtx.uNumSlice, pSlice, pPicBuffers);
}

/******************************************************************************/
static void AL_AVC_FillSlicePicIDs(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
  TBufferListRef* pListRef = &pCtx->ListRef;

  FillRefPicIDs(pCtx, pSliceParam);

  // Reg 0x12
  pSliceParam->tColocPicID = 0;

  if(pSliceParam->eSliceType == AL_SLICE_B)
    pSliceParam->tColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[1][0].tNodeID);

  FillConcealValue(pCtx, pSliceParam);
}

/*****************************************************************************/
void AL_AVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScalingList* pSCL, AL_TDecPicParam* pPicParam, AL_TDecBuffers* pPicBuffers, AL_TDecSliceParam* pSliceParam, AL_TAvcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->tCurrentFrameCtx.uNumSlice;

  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPicParam->iFrmNum = pCtx->iNumFrmBlk1;
  pPicParam->uUserParam = pCtx->uToggle;

  AL_AVC_InitHWCommandBuffers(pCtx, pSliceParam, pSlice, (AL_TScalingList const*)pSCL, pPicParam->eChromaMode, pPicBuffers);

  AL_AVC_FillSlicePicIDs(pCtx, pSliceParam);

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, bIsLastVclNalInAU, false);
    return;
  }

  // copy collocated info
  if(pSliceParam->uFirstLcuSliceSegment && pSliceParam->eSliceType == AL_SLICE_I)
    pSliceParam->tColocPicID = pPrevSP->tColocPicID;

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, false, false);

  if(pSliceParam->uSliceFirstLcu)
    pSliceParam->uFirstLcuTileId = pSliceParam->bDependentSlice ? pPrevSP->uFirstLcuTileId : pCtx->tCurrentFrameCtx.uCurTileID;

  AL_SaveNalStreamBlk1(pCtx, pSliceParam);

  if(bIsLastVclNalInAU)
    AL_TerminateCurrentCommand(pCtx, pPicParam, pSliceParam);
}

/*****************************************************************************/
void AL_HEVC_InitHWCommandBuffers(AL_TDecCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_THevcSliceHdr const* pSlice, AL_TScalingList const* pSclLst, AL_TDecBuffers* pPicBuffers)
{
  if(pCtx->tCurrentFrameCtx.eBufStatus == DEC_FRAME_BUF_RESERVED)
  {
    AL_ItuPictMngr_GetBuffers(&pCtx->PictMngr, AL_CODEC_HEVC, pSliceParam, &pCtx->pRecs, pPicBuffers);

    if(pSlice->first_slice_segment_in_pic_flag)
      AL_HEVC_InitHWFrameBuffers(pSclLst, pPicBuffers);

    pCtx->tCurrentFrameCtx.eBufStatus |= DEC_FRAME_BUF_FILLED;
  }

  AL_HEVC_InitHWSliceBuffers(pCtx->tCurrentFrameCtx.uNumSlice, pSlice, pPicBuffers);
}

/******************************************************************************/
static void AL_HEVC_FillSlicePicIDs(AL_TDecCtx* pCtx, const AL_THevcSliceHdr* pSlice, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam)
{
  TBufferListRef* pListRef = &pCtx->ListRef;
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

  FillRefPicIDs(pCtx, pSliceParam);

  if(!pSliceParam->uFirstLcuSlice)
    pPicParam->tColocPicID = AL_BAD_INDEX;

  // Reg 0x12
  if((pSliceParam->eSliceType == AL_SLICE_B && pSlice->collocated_from_l0_flag) || pSliceParam->eSliceType == AL_SLICE_P)
    pPicParam->tColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[0][pSlice->collocated_ref_idx].tNodeID);
  else if(pSliceParam->eSliceType == AL_SLICE_B)
    pPicParam->tColocPicID = AL_Dpb_GetPicID_FromNode(pDpb, (*pListRef)[1][pSlice->collocated_ref_idx].tNodeID);

  FillConcealValue(pCtx, pSliceParam);
}

/*****************************************************************************/
void AL_HEVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScalingList* pSCL, AL_TDecPicParam* pPicParam, AL_TDecBuffers* pPicBuffers, AL_TDecSliceParam* pSliceParam, AL_THevcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid)
{
  // fast access
  uint16_t uSliceID = pCtx->tCurrentFrameCtx.uNumSlice;
  AL_TDecSliceParam* pPrevSP = uSliceID ? &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[uSliceID - 1]) : NULL;

  pPicParam->iFrmNum = pCtx->iNumFrmBlk1;
  pPicParam->uUserParam = pCtx->uToggle;

  AL_HEVC_InitHWCommandBuffers(pCtx, pSliceParam, pSlice, (AL_TScalingList const*)pSCL, pPicBuffers);

  AL_HEVC_FillSlicePicIDs(pCtx, pSlice, pPicParam, pSliceParam);

  if(pPrevSP && !bIsValid && bIsLastVclNalInAU)
  {
    // Here bNextIsDependent was used to automatically re-use the previous slice header.
    // But independent slices are forced to be run on the same parser, which is
    // conflicting with Dec1/Dec2 synchronization constraints in lowlat.
    bool bNextIsDependent = !(pPicParam->uNumTileRows > 1 && pCtx->pChanParam->bLowLat);
    AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, bIsLastVclNalInAU, bNextIsDependent);
    return;
  }

  // copy collocated info
  if(pSliceParam->uFirstLcuSliceSegment && pSliceParam->eSliceType == AL_SLICE_I)
    pSliceParam->tColocPicID = pPrevSP->tColocPicID;

  // stock command registers in memory
  AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, false, pSliceParam->bDependentSlice);

  if(pSliceParam->uFirstLcuSliceSegment)
  {
    pSliceParam->uFirstLcuSlice = pSliceParam->bDependentSlice ? pPrevSP->uFirstLcuSlice : pSliceParam->uFirstLcuSlice;
    pSliceParam->uFirstLcuTileId = pSliceParam->bDependentSlice ? pPrevSP->uFirstLcuTileId : pCtx->tCurrentFrameCtx.uCurTileID;
  }

  AL_SaveNalStreamBlk1(pCtx, pSliceParam);

  if(bIsLastVclNalInAU)
    AL_TerminateCurrentCommand(pCtx, pPicParam, pSliceParam);
}

/*!@}*/
