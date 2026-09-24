// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "I_PictMngr.h"
#include "I_PictMngrCallbacks.h"

#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecOutputSettingsInternal.h"
#include "lib_common/Error.h"
#include "lib_common/Round.h"
#include "lib_common/Index.h"
#include "lib_common/PixMapBufferInternal.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common_dec/DecPicParam.h"
#include "lib_common/DisplayInfoMeta.h"
#include "lib_common/Utils.h"
#include "lib_decode/lib_decode.h"
#include "lib_rtos/lib_rtos.h"

#include "DPB.h"

/*************************************************************************/
static AL_TBuffer* sRecBuffers_GetDisplayBuffer(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;

  return tRecBuffers->pFrame;
}

/*************************************************************************/
static void sRecBuffers_Reset(AL_TRecBuffers* tRecBuffers)
{
  tRecBuffers->pFrame = NULL;
}

/*************************************************************************/
static void sRecBuffers_CleanUp(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;

  AL_Buffer_Cleanup(tRecBuffers->pFrame);

}

/*************************************************************************/
static void sRecBuffers_Release(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  AL_Buffer_Unref(tRecBuffers->pFrame);
}

/*************************************************************************/
static bool sRecBuffers_HasBuf(AL_TRecBuffers const* tRecBuffers, AL_TBuffer const* pBuf, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  bool bHasBuf = tRecBuffers->pFrame == pBuf;

  return bHasBuf;
}

/*************************************************************************/
static bool sRecBuffers_AreNotNull(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  bool bNotNull = tRecBuffers->pFrame;

  return bNotNull;
}

/*************************************************************************/
// This function is only a sanity check that confirms that the buffer pointers are set
static void sRecBuffers_CheckBuffersAreNotNull(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  bool bNotNull = sRecBuffers_AreNotNull(tRecBuffers, pFrmBufPool);
  (void)bNotNull;
  Rtos_Assert(bNotNull);
}

/*************************************************************************/
static int32_t sFrmBufPool_GetFrameIDFromBuf(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  for(int32_t i = 0; i < AL_REFMNGR_MAX_POOL_SIZE; i++)
  {
    if(sRecBuffers_HasBuf(&pFrmBufPool->vFrameData[i].tRecBuffers, pBuf, pFrmBufPool))
    {
      Rtos_ReleaseMutex(pFrmBufPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return -1;
}

/*************************************************************************/
static int32_t sFrmBufPool_GetFrameIDFromDisplay(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  for(int32_t i = 0; i < AL_REFMNGR_MAX_POOL_SIZE; i++)
  {
    if(sRecBuffers_GetDisplayBuffer(&pFrmBufPool->vFrameData[i].tRecBuffers, pFrmBufPool) == pDisplayBuf)
    {
      Rtos_ReleaseMutex(pFrmBufPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return -1;
}

/*************************************************************************/
static bool sFrmBufPool_HasAvailableBuffer(AL_TFrmBufPool const* pFrmBufPool)
{
  return !IntFifo_Empty(&pFrmBufPool->tAvailableFrameIDFifo);
}

/*************************************************************************/
// This function is only a sanity check that confirms that the buffer is not in the fifo
static void sFrmBufPool_CheckIsNotAvailable(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  bool bInFifo = false;

  if(!sFrmBufPool_HasAvailableBuffer(pFrmBufPool))
  {
    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return;
  }

  int32_t iLastIdx = (pFrmBufPool->tAvailableFrameIDFifo.tail + 1) % pFrmBufPool->tAvailableFrameIDFifo.total_elements;

  for(int32_t i = pFrmBufPool->tAvailableFrameIDFifo.head; i != iLastIdx; i = (i + 1) % pFrmBufPool->tAvailableFrameIDFifo.total_elements)
  {
    uint32_t tFrameID = pFrmBufPool->vAvailableFrameIDs[i];

    if(sRecBuffers_GetDisplayBuffer(&pFrmBufPool->vFrameData[tFrameID].tRecBuffers, pFrmBufPool) == pDisplayBuf)
    {
      bInFifo = true;
      break;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  (void)bInFifo;
  Rtos_Assert(!bInFifo);
}

/*************************************************************************/
static void sFrmBufPoolFifo_AddAvailableBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, AL_TRecBuffers const* pRecBuffers)
{
  Rtos_Memcpy(&pFrmBufPool->vFrameData[tFrameID].tRecBuffers, pRecBuffers, sizeof(AL_TRecBuffers));
  IntFifo_Queue(&pFrmBufPool->tAvailableFrameIDFifo, tFrameID);

  pFrmBufPool->vFrameData[tFrameID].iAccessCnt = 0;
  pFrmBufPool->vFrameData[tFrameID].bToBeDisplayed = false;
}

/*************************************************************************/
static bool sFrmBufPool_AddAvailableBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TRecBuffers const* pRecBuffers)
{
  sRecBuffers_CheckBuffersAreNotNull(pRecBuffers, pFrmBufPool);

  Rtos_GetMutex(pFrmBufPool->Mutex);

  AL_TIndex tFrameID = 0;

  while(tFrameID < AL_REFMNGR_MAX_POOL_SIZE && (sRecBuffers_AreNotNull(&pFrmBufPool->vFrameData[tFrameID].tRecBuffers, pFrmBufPool) ||
                                                (pFrmBufPool->vFrameData[tFrameID].iAccessCnt != -1) ||
                                                pFrmBufPool->vFrameData[tFrameID].bToBeDisplayed
                                                ))
    tFrameID++;

  if(tFrameID < AL_REFMNGR_MAX_POOL_SIZE)
  {
    sFrmBufPoolFifo_AddAvailableBuffer(pFrmBufPool, tFrameID, pRecBuffers);
    Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return tFrameID < AL_REFMNGR_MAX_POOL_SIZE;
}

/*************************************************************************/
static uint32_t sFrmBufPool_GetAvailableBuffer(AL_TFrmBufPool* pFrmBufPool)
{
  int32_t const iFrameID = IntFifo_Dequeue(&pFrmBufPool->tAvailableFrameIDFifo);

  if(iFrameID >= 0)
  {
    Rtos_Assert(pFrmBufPool->vFrameData[iFrameID].iAccessCnt == 0);
    pFrmBufPool->vFrameData[iFrameID].iAccessCnt = 1;
  }

  return iFrameID;
}

/*************************************************************************/
static int32_t sFrmBufPool_Pop(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetSemaphore(pFrmBufPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_GetMutex(pFrmBufPool->Mutex);

  if(pFrmBufPool->isDecommited)
  {
    Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return AL_BAD_INDEX;
  }

  int32_t const iFrameID = sFrmBufPool_GetAvailableBuffer(pFrmBufPool);

  if(iFrameID >= 0)
  {
    sRecBuffers_CheckBuffersAreNotNull(&pFrmBufPool->vFrameData[iFrameID].tRecBuffers, pFrmBufPool);
    Rtos_Assert(pFrmBufPool->vFrameData[iFrameID].bToBeDisplayed == false);

    sRecBuffers_CleanUp(&pFrmBufPool->vFrameData[iFrameID].tRecBuffers, pFrmBufPool);
    pFrmBufPool->vFrameData[iFrameID].tRecInfo.eError = AL_SUCCESS;
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return iFrameID;
}

/*************************************************************************/
static void sFrmBufPool_Decommit(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  pFrmBufPool->isDecommited = true;
  Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_RemoveID(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];
  sRecBuffers_CheckBuffersAreNotNull(&pFrame->tRecBuffers, pFrmBufPool);
  Rtos_Assert(pFrame->iAccessCnt == 0);

  sRecBuffers_Reset(&pFrame->tRecBuffers);
  pFrame->iAccessCnt = -1;
  pFrame->bToBeDisplayed = false;

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static AL_TRecBuffers* sFrmBufPool_GetBuffersFromID(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  sRecBuffers_CheckBuffersAreNotNull(&pFrmBufPool->vFrameData[tFrameID].tRecBuffers, pFrmBufPool);
  return &pFrmBufPool->vFrameData[tFrameID].tRecBuffers;
}

/*************************************************************************/
static AL_TRecBufferInfo const* sFrmBufPool_GetRecInfoFromID(AL_TFrmBufPool const* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  return &pFrmBufPool->vFrameData[tFrameID].tRecInfo;
}

/*************************************************************************/
static void sFrmBufPool_InitAvailableBuffers(AL_TFrmBufPool* pFrmBufPool)
{
  for(int32_t i = 0; i < AL_REFMNGR_MAX_POOL_SIZE; i++)
  {
    sRecBuffers_Reset(&pFrmBufPool->vFrameData[i].tRecBuffers);
    pFrmBufPool->vFrameData[i].iAccessCnt = -1;
    pFrmBufPool->vFrameData[i].bToBeDisplayed = false;
  }

  IntFifo_Init(&pFrmBufPool->tAvailableFrameIDFifo, pFrmBufPool->vAvailableFrameIDs, ARRAY_SIZE(pFrmBufPool->vAvailableFrameIDs));
}

/*************************************************************************/
static bool sFrmBufPool_Init(AL_TFrmBufPool* pFrmBufPool, AL_TAllocator* pAllocator, bool bHasPrimaryOutputFrame, bool bHasSecondOutputFrame)
{
  (void)pAllocator;
  (void)bHasPrimaryOutputFrame;
  (void)bHasSecondOutputFrame;

  pFrmBufPool->Mutex = Rtos_CreateMutex();

  if(!pFrmBufPool->Mutex)
    goto fail_alloc_mutex;

  pFrmBufPool->Semaphore = Rtos_CreateSemaphore(0);

  if(!pFrmBufPool->Semaphore)
    goto fail_alloc_sem_free;

  sFrmBufPool_InitAvailableBuffers(pFrmBufPool);

  return true;

  fail_alloc_sem_free:
  Rtos_DeleteMutex(pFrmBufPool->Mutex);
  fail_alloc_mutex:
  return false;
}

/*************************************************************************/
static void sFrmBufPool_EnsureRecBufferAreRemoved(AL_TFrmBufPool* pFrmBufPool)
{
  for(int32_t i = 0; i < AL_REFMNGR_MAX_POOL_SIZE; i++)
  {
    if(pFrmBufPool->vFrameData[i].iAccessCnt == 0)
    {
      AL_TRecBuffers tBuffers;
      Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBuffersFromID(pFrmBufPool, i), sizeof(tBuffers));
      sFrmBufPool_RemoveID(pFrmBufPool, i);
      sRecBuffers_Release(&tBuffers, pFrmBufPool);
    }

    Rtos_Assert(pFrmBufPool->vFrameData[i].iAccessCnt == -1);
  }
}

/*************************************************************************/
static void sFrmBufPool_Deinit(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_Assert(!sFrmBufPool_HasAvailableBuffer(pFrmBufPool));
  sFrmBufPool_EnsureRecBufferAreRemoved(pFrmBufPool);

  Rtos_DeleteSemaphore(pFrmBufPool->Semaphore);
  Rtos_DeleteMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_DecrementBufID(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, bool bForceDisplay)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];
  Rtos_Assert(pFrame->iAccessCnt >= 1);
  pFrame->iAccessCnt--;

  if(pFrame->iAccessCnt == 0 && !pFrame->bAlreadyDisplayed)
  {
    bool bToBeDisplayed = pFrame->bToBeDisplayed; // pFrame will be removed so save the param to be used in the condition below
    AL_TRecBuffers tBuffers;
    Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBuffersFromID(pFrmBufPool, tFrameID), sizeof(tBuffers));
    sFrmBufPool_RemoveID(pFrmBufPool, tFrameID);

    if((!bToBeDisplayed) || bForceDisplay)
    {
      sFrmBufPool_CheckIsNotAvailable(pFrmBufPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pFrmBufPool));
      sFrmBufPool_AddAvailableBuffer(pFrmBufPool, &tBuffers);
    }
    else
      sRecBuffers_Release(&tBuffers, pFrmBufPool);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_OutputBufID(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];
  Rtos_Assert(pFrame->bToBeDisplayed == false);
  pFrame->bToBeDisplayed = true;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_IncrementBufID(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];
  Rtos_Assert(pFrame->iAccessCnt >= 0);
  pFrame->iAccessCnt++;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*****************************************************************************/
static void sFrmBufPool_Terminate(AL_TFrmBufPool* pFrmBufPool)
{
  (void)pFrmBufPool; // Nothing to do
}

/*************************************************************************/
static void sPictMngr_DecrementFrmBuf(void* pUserParam, AL_TIndex tFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, tFrameID, pCtx->bForceDisplay);
}

/*************************************************************************/
static void sPictMngr_IncrementFrmBuf(void* pUserParam, AL_TIndex tFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_IncrementBufID(&pCtx->FrmBufPool, tFrameID);
}

/*************************************************************************/
static void sPictMngr_OutputFrmBuf(void* pUserParam, AL_TIndex tFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_OutputBufID(&pCtx->FrmBufPool, tFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementAnnexBuf(void* pUserParam, AL_TIndex tAnnexID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit && (pCtx->AnnexBufPool.uBufCnt > 0))
    AL_TBufPool_IncrementBufID(&pCtx->AnnexBufPool, tAnnexID);
}

/*************************************************************************/
static void sPictMngr_DecrementAnnexBuf(void* pUserParam, AL_TIndex tAnnexID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit && pCtx->bIsAnnexPoolSet)
    AL_TBufPool_DecrementBufID(&pCtx->AnnexBufPool, tAnnexID);
}

/*****************************************************************************/
static bool sPictMngrParam_CheckInitParameters(AL_TPictMngrParam const* pParam)
{
  if(pParam->eFbStorageMode >= AL_FB_MAX_ENUM)
    return false;

  return true;
}

/*****************************************************************************/
static size_t sPictMngr_GetAllocationSizeForAnnexBuffer(AL_TPictMngrCtx* pCtx, size_t const* vSizes, uint8_t uNumBuf)
{
  Rtos_Assert(uNumBuf <= AL_MAX_ANNEX_BUF);
  pCtx->zNumAnnexBuf = uNumBuf;

  size_t zSizeToAllocate = 0;

  for(int i = 0; i < uNumBuf; i++)
  {
    pCtx->vAnnexSubBufOffsets[i] = zSizeToAllocate;
    size_t zAlignedSize = AL_RoundUp(vSizes[i], HW_IP_BURST_ALIGNMENT);
    pCtx->vAnnexSubBufSizes[i] = zAlignedSize;
    zSizeToAllocate += zAlignedSize;
  }

  return zSizeToAllocate;
}

/*****************************************************************************/
static void sPictMngr_GetAnnexBuffers(AL_TPictMngrCtx const* pCtx, TBuffer const* pRawBuf, TBuffer* pAnnexBuf)
{
  for(size_t i = 0; i < pCtx->zNumAnnexBuf; i++)
  {
    TBuffer tSubBuf = *pRawBuf;
    tSubBuf.tMD.pVirtualAddr += pCtx->vAnnexSubBufOffsets[i];
    tSubBuf.tMD.uPhysicalAddr += pCtx->vAnnexSubBufOffsets[i];
    tSubBuf.tMD.uSize = pCtx->vAnnexSubBufSizes[i];
    pAnnexBuf[i] = tSubBuf;
  }
}

/*****************************************************************************/
bool AL_PictMngr_PreInit(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx)
    return false;

  pCtx->PreInitMutex = Rtos_CreateMutex();

  if(!pCtx->PreInitMutex)
    return false;

  pCtx->bBasicInit = false;
  pCtx->FrmBufPool.isDecommited = false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_BasicInit(AL_TPictMngrCtx* pCtx, AL_IReferenceManager* pRefMngr, AL_TPictMngrParam const* pParam, AL_TAllocator* pAllocator)
{
  if(!pCtx)
    return false;

  if(!sPictMngrParam_CheckInitParameters(pParam))
    return false;

  if(!pRefMngr)
    return false;

  pCtx->pRefMngr = pRefMngr;

  AL_TPictMngrCallbacks tCallbacks =
  {
    sPictMngr_IncrementFrmBuf,
    sPictMngr_DecrementFrmBuf,
    sPictMngr_OutputFrmBuf,
    sPictMngr_IncrementAnnexBuf,
    sPictMngr_DecrementAnnexBuf,
    pCtx,
  };

  if(!AL_IReferenceManager_Init(pCtx->pRefMngr, &tCallbacks))
    return false;

  if(pParam->uNumSubBuf > 0)
  {
    size_t tAnnexSize = sPictMngr_GetAllocationSizeForAnnexBuffer(pCtx, pParam->zSubBufSizes, pParam->uNumSubBuf);

    if(!AL_TBufPool_Init(&pCtx->AnnexBufPool, pParam->uNumAnnexBuf, tAnnexSize, pAllocator, "Annex", true))
      return false;

    pCtx->bIsAnnexPoolSet = true;
  }
  else
    pCtx->bIsAnnexPoolSet = false;

  pCtx->eFbStorageMode = pParam->eFbStorageMode;
  pCtx->eCodec = pParam->eCodec;
  pCtx->tFrameID = AL_BAD_INDEX;
  pCtx->tAnnexID = AL_BAD_INDEX;

  Rtos_GetMutex(pCtx->PreInitMutex);
  pCtx->bBasicInit = true;
  Rtos_ReleaseMutex(pCtx->PreInitMutex);
  pCtx->bForceDisplay = pParam->bForceDisplay;
  pCtx->bCompleteInit = false;

  pCtx->tOutputPosition = pParam->tOutputPosition;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_CompleteInit(AL_TPictMngrCtx* pCtx, AL_TAllocator* pAllocator, AL_TDecOutputSettings const* pDecOutputSettings)
{
  pCtx->tDecOutputSettings = *pDecOutputSettings;

  bool bEnableSecondOutput = false;

  bool bEnablePrimaryOutput = true;

  bool bSuccess = sFrmBufPool_Init(&pCtx->FrmBufPool, pAllocator, bEnablePrimaryOutput, bEnableSecondOutput);
  pCtx->bCompleteInit = bSuccess;
  return bSuccess;
}

/*****************************************************************************/
bool AL_PictMngr_IsInitComplete(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->bCompleteInit;
}

/*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bBasicInit)
    return;

  AL_IReferenceManager_Terminate(pCtx->pRefMngr);

  if(pCtx->bIsAnnexPoolSet)
    AL_TBufPool_Terminate(&pCtx->AnnexBufPool);

  if(pCtx->bCompleteInit)
    sFrmBufPool_Terminate(&pCtx->FrmBufPool);
}

/*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->bBasicInit)
  {
    AL_IReferenceManager_Deinit(pCtx->pRefMngr);

    if(pCtx->bIsAnnexPoolSet)
      AL_TBufPool_Deinit(&pCtx->AnnexBufPool);

    if(pCtx->bCompleteInit)
      sFrmBufPool_Deinit(&pCtx->FrmBufPool);
  }
  Rtos_DeleteMutex(pCtx->PreInitMutex);
}

/*****************************************************************************/
void AL_PictMngr_LockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, AL_TIndex* pRefFrameID, AL_TIndex* pRefAnnexId)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_IncrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_IncrementAnnexBuf(pCtx, pRefAnnexId[uRef]);
  }
}

/*****************************************************************************/
void AL_PictMngr_UnlockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, AL_TIndex* pRefFrameID, AL_TIndex* pRefAnnexId)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_DecrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_DecrementAnnexBuf(pCtx, pRefAnnexId[uRef]);
  }
}

/*****************************************************************************/
AL_TIndex AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->tFrameID;
}

/*****************************************************************************/
AL_TIndex AL_PictMngr_GetCurrentAnnexID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->tAnnexID;
}

/***************************************************************************/
static void sBuffer_ChangePictChromaMode(AL_TBuffer* pBuf, AL_EChromaMode eChromaMode)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TPicFormat tPicFmt;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFmt);
  (void)bSuccess;
  Rtos_Assert(bSuccess);

  if(eChromaMode == tPicFmt.eChromaMode)
    return;
  tPicFmt = AL_GetDecPicFormat(eChromaMode, tPicFmt.uBitDepth, tPicFmt.eStorageMode, tPicFmt.bCompressed, AL_PLANE_MODE_MAX_ENUM);
  tFourCC = AL_GetDecFourCC(&tPicFmt);
  AL_PixMapBuffer_SetFourCC(pBuf, tFourCC);
}

/***************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eDecodedChromaMode)
{
  Rtos_Assert(pCtx->bCompleteInit);

  (void)eDecodedChromaMode;
  pCtx->tFrameID = sFrmBufPool_Pop(&pCtx->FrmBufPool);

  if(!AL_IS_VALID_INDEX(pCtx->tFrameID))
    return false;

  if(pCtx->bIsAnnexPoolSet)
  {
    pCtx->tAnnexID = AL_TBufPool_GetFreeBufID(&pCtx->AnnexBufPool);
    Rtos_Assert(AL_IS_VALID_INDEX(pCtx->tAnnexID));
  }

  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, pCtx->tFrameID);

  AL_PixMapBuffer_SetDimension(pBuffers->pFrame, tDim);

  {
    sBuffer_ChangePictChromaMode(pBuffers->pFrame, eDecodedChromaMode);
  }

  pCtx->FrmBufPool.vFrameData[pCtx->tFrameID].tRecInfo.bStartsNewCVS = bStartsNewCVS;

  return true;
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackReleaseIsDone(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pReleasedFrame)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  int32_t const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pFrmBufPool, pReleasedFrame);
  Rtos_Assert(iFrameID >= 0);
  uint32_t tFrameID = iFrameID;

  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  Rtos_Assert(pFrmBufPool->vFrameData[tFrameID].bToBeDisplayed || pFrmBufPool->vFrameData[tFrameID].bAlreadyDisplayed);
  sFrmBufPool_CheckIsNotAvailable(pFrmBufPool, pReleasedFrame);

  pFrmBufPool->vFrameData[tFrameID].bToBeDisplayed = false;
  AL_TRecBuffers tBuffers;
  Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBuffersFromID(pFrmBufPool, tFrameID), sizeof(tBuffers));
  sFrmBufPool_RemoveID(pFrmBufPool, tFrameID);
  sRecBuffers_Release(&tBuffers, pFrmBufPool);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/***************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx)
{
  Rtos_Assert(pCtx->bCompleteInit);

  if(AL_IS_VALID_INDEX(pCtx->tAnnexID))
  {
    AL_TBufPool_DecrementBufID(&pCtx->AnnexBufPool, pCtx->tAnnexID);
    pCtx->tAnnexID = AL_BAD_INDEX;
  }

  if(AL_IS_VALID_INDEX(pCtx->tFrameID))
  {
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, pCtx->tFrameID, pCtx->bForceDisplay);
    pCtx->tFrameID = AL_BAD_INDEX;
  }
}

/*************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bBasicInit)
    return;

  AL_IReferenceManager_Flush(pCtx->pRefMngr);
}

/*****************************************************************************/
AL_TIndex AL_PictMngr_GetLastPicID(AL_TPictMngrCtx const* pCtx)
{
  return AL_IReferenceManager_GetLastPicID(pCtx->pRefMngr);
}

/*****************************************************************************/
AL_TIndex AL_PictMngr_GetLastRefPicID(AL_TPictMngrCtx const* pCtx)
{
  return AL_IReferenceManager_GetLastRefPicID(pCtx->pRefMngr);
}

/*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_TIndex tAnnexID, void* pParam)
{
  AL_IReferenceManager_Insert(pCtx->pRefMngr, tFrameID, tAnnexID, pParam);
}

/***************************************************************************/
static void sFrmBufPool_UpdateCRC(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, AL_64U uCRC)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  pFrmBufPool->vFrameData[tFrameID].tRecInfo.uCRC = uCRC;
}

/***************************************************************************/
static void sFrmBufPool_UpdatePixCount(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, uint32_t uPixCount)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  pFrmBufPool->vFrameData[tFrameID].tRecInfo.uPixCount = uPixCount;
}

/***************************************************************************/
static void sFrmBufPool_UpdateCrop(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, AL_TCropInfo const* pCrop)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE && pCrop);
  pFrmBufPool->vFrameData[tFrameID].tRecInfo.tCrop = *pCrop;
}

/***************************************************************************/
static void sFrmBufPool_UpdatePicStruct(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, AL_EPicStruct ePicStruct)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  pFrmBufPool->vFrameData[tFrameID].tRecInfo.ePicStruct = ePicStruct;
}

/***************************************************************************/
static void sFrmBufPool_UpdateError(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID, AL_ERR error)
{
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);
  pFrmBufPool->vFrameData[tFrameID].tRecInfo.eError = error;
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_64U uCRC)
{
  sFrmBufPool_UpdateCRC(&pCtx->FrmBufPool, tFrameID, uCRC);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferPixCount(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, uint32_t uPixCount)
{
  sFrmBufPool_UpdatePixCount(&pCtx->FrmBufPool, tFrameID, uPixCount);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCrop)
{
  sFrmBufPool_UpdateCrop(&pCtx->FrmBufPool, pCtx->tFrameID, pCrop);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct)
{
  sFrmBufPool_UpdatePicStruct(&pCtx->FrmBufPool, pCtx->tFrameID, ePicStruct);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferError(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_ERR eError)
{
  sFrmBufPool_UpdateError(&pCtx->FrmBufPool, tFrameID, eError);
}

/***************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID)
{
  AL_IReferenceManager_EndDecoding(pCtx->pRefMngr, tFrameID);
}

/***************************************************************************/
void AL_PictMngr_UnlockID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_TIndex tAnnexID)
{
  Rtos_Assert(pCtx->bCompleteInit);
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, tFrameID, pCtx->bForceDisplay);

  if(pCtx->bIsAnnexPoolSet)
    AL_TBufPool_DecrementBufID(&pCtx->AnnexBufPool, tAnnexID);
}

/***************************************************************************/
static void sPictMngr_GetInfoDecode(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_EFbStorageMode eFbStorageMode, bool bDisplayInfo)
{
  Rtos_Assert(pCtx->bCompleteInit);
  Rtos_Assert(pInfo);

  AL_TFrmBufPool* pFrmBufPool = &pCtx->FrmBufPool;

  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBuffersFromID(pFrmBufPool, tFrameID);
  sRecBuffers_CheckBuffersAreNotNull(pBuffers, pFrmBufPool);

  AL_TBuffer* pBuf = bDisplayInfo ? sRecBuffers_GetDisplayBuffer(pBuffers, pFrmBufPool) : pBuffers->pFrame;
  AL_TPixMapMetaData* pMetaSrc = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);
  Rtos_Assert(pMetaSrc);

  pInfo->tCrop = pFrmBufPool->vFrameData[tFrameID].tRecInfo.tCrop;
  pInfo->uBitDepthY = pInfo->uBitDepthC = AL_GetBitDepth(pMetaSrc->tFourCC);
  pInfo->uCRC = pFrmBufPool->vFrameData[tFrameID].tRecInfo.uCRC;
  pInfo->uPixCount = pFrmBufPool->vFrameData[tFrameID].tRecInfo.uPixCount;
  pInfo->tDim = AL_PixMapBuffer_GetDimension(pBuf);
  pInfo->eFbStorageMode = eFbStorageMode;
  pInfo->ePicStruct = pFrmBufPool->vFrameData[tFrameID].tRecInfo.ePicStruct;
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  Rtos_Assert(tFourCC != 0);
  pInfo->eChromaMode = AL_GetChromaMode(tFourCC);
  pInfo->eOutputID = AL_OUTPUT_MAIN;
  pInfo->tPos = pCtx->tOutputPosition;

  if(pStartsNewCVS)
    *pStartsNewCVS = pFrmBufPool->vFrameData[tFrameID].tRecInfo.bStartsNewCVS;
}

/***************************************************************************/
static AL_TBuffer* sPictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_TIndex tFrameID)
{
  Rtos_Assert(pCtx->bCompleteInit);

  if(!AL_IS_VALID_INDEX(tFrameID))
    return NULL;

  AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;

  if(pInfo)
    sPictMngr_GetInfoDecode(pCtx, tFrameID, pInfo, pStartsNewCVS, eOutputStorageMode, true);

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, tFrameID);
  AL_TBuffer* pDisplayBuffer = sRecBuffers_GetDisplayBuffer(pRecBuffers, &pCtx->FrmBufPool);

  if(pInfo)
  {
    AL_TDisplayInfoMetaData* pMeta = (AL_TDisplayInfoMetaData*)(AL_Buffer_GetMetaData(pDisplayBuffer, AL_META_TYPE_DISPLAY_INFO));

    if(pMeta)
    {
      pMeta->ePicStruct = pInfo->ePicStruct;
      pMeta->tCrop = pInfo->tCrop;
      pMeta->uCrc = pInfo->uCRC;
      pMeta->uPixCount = pInfo->uPixCount;
      pMeta->uStreamBitDepthY = pInfo->uBitDepthY;
      pMeta->uStreamBitDepthC = pInfo->uBitDepthC;
      pMeta->eOutputID = pInfo->eOutputID;

    }
  }

  return pDisplayBuffer;
}

/***************************************************************************/
AL_TBuffer* AL_PictMngr_ForceDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_TIndex tFrameID)
{
  Rtos_Assert(pCtx->bCompleteInit);
  Rtos_Assert(pCtx->bForceDisplay);

  AL_TFrmBufPool* pFrmBufPool = &pCtx->FrmBufPool;
  Rtos_GetMutex(pFrmBufPool->Mutex);
  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];
  pFrame->bAlreadyDisplayed = true;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return sPictMngr_GetDisplayBuffer(pCtx, pInfo, pStartsNewCVS, tFrameID);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS)
{
  if(!pCtx->bBasicInit)
    return NULL;

  AL_TIndex tDisplayID = AL_IReferenceManager_GetDisplayBuffer(pCtx->pRefMngr);
  return sPictMngr_GetDisplayBuffer(pCtx, pInfo, pStartsNewCVS, tDisplayID);
}

/*************************************************************************/
static void sFrmBufPool_ReleaseDisplayBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TIndex tFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(tFrameID < AL_REFMNGR_MAX_POOL_SIZE);

  AL_TPictMngrFrameData* pFrame = &pFrmBufPool->vFrameData[tFrameID];

  bool bPushBack = (pFrame->iAccessCnt == 0);

  if(pFrame->bAlreadyDisplayed)
    bPushBack = bPushBack && pFrame->bToBeDisplayed;

  if(!pFrame->bAlreadyDisplayed || bPushBack)
    pFrame->bToBeDisplayed = false;

  pFrame->bAlreadyDisplayed = false;

  if(bPushBack)
  {
    AL_TRecBuffers tBuffers;
    Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBuffersFromID(pFrmBufPool, tFrameID), sizeof(tBuffers));
    sFrmBufPool_RemoveID(pFrmBufPool, tFrameID);
    sFrmBufPool_CheckIsNotAvailable(pFrmBufPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pFrmBufPool));
    sFrmBufPool_AddAvailableBuffer(pFrmBufPool, &tBuffers);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID)
{
  Rtos_Assert(pCtx);

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, tFrameID);
  return sRecBuffers_GetDisplayBuffer(pRecBuffers, &pCtx->FrmBufPool);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID)
{
  Rtos_Assert(pCtx);
  return sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, tFrameID)->pFrame;
}

/*************************************************************************/
AL_TRecBufferInfo const* AL_PictMngr_GetRecInfoFromID(AL_TPictMngrCtx const* pCtx, AL_TIndex tFrameID)
{
  Rtos_Assert(pCtx);
  return sFrmBufPool_GetRecInfoFromID(&pCtx->FrmBufPool, tFrameID);
}

/*************************************************************************/
bool AL_PictMngr_GetFrameDecodingError(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pDisplayBuf, AL_ERR* pError)
{
  if(!pCtx->bCompleteInit)
    return false;

  if(pDisplayBuf == NULL)
    return false;

  int32_t iFrameID = sFrmBufPool_GetFrameIDFromBuf(&pCtx->FrmBufPool, pDisplayBuf);

  if(pError != NULL && iFrameID != -1)
  {
    *pError = pCtx->FrmBufPool.vFrameData[iFrameID].tRecInfo.eError;
    return true;
  }

  return false;
}

/*************************************************************************/
static bool sFrmBufPool_FillRecBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pDisplayBuf, AL_TRecBuffers* pRecBuffers)
{
  (void)pFrmBufPool;

  pRecBuffers->pFrame = pDisplayBuf;

  AL_Buffer_Ref(pDisplayBuf);

  return true;
}

/*************************************************************************/
static bool sFrmBufPool_PutDisplayBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  bool bSucceed = false;

  Rtos_Assert(pBuf);

  int32_t const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pFrmBufPool, pBuf);

  if(iFrameID == -1)
  {
    AL_TRecBuffers tRecBuffers;

    if(sFrmBufPool_FillRecBuffer(pFrmBufPool, pBuf, &tRecBuffers))
    {
      if(sFrmBufPool_AddAvailableBuffer(pFrmBufPool, &tRecBuffers))
        bSucceed = true;
      else
        sRecBuffers_Release(&tRecBuffers, pFrmBufPool);
    }
  }
  else
  {
    sFrmBufPool_ReleaseDisplayBuffer(pFrmBufPool, (AL_TIndex)iFrameID);
    bSucceed = true;
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);

  return bSucceed;
}

/*************************************************************************/
bool AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  Rtos_Assert(pCtx->bCompleteInit);
  return sFrmBufPool_PutDisplayBuffer(&pCtx->FrmBufPool, pBuf);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx)
{
  Rtos_Assert(pCtx->bBasicInit);
  AL_IReferenceManager_ReleaseDisplayBuffer(pCtx->pRefMngr);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame)
{
  Rtos_Assert(pCtx->bCompleteInit);
  sFrmBufPool_SignalCallbackReleaseIsDone(&pCtx->FrmBufPool, pReleasedFrame);
}

/*****************************************************************************/
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx)
{
  Rtos_GetMutex(pCtx->PreInitMutex);

  if(!pCtx->bBasicInit)
  {
    pCtx->FrmBufPool.isDecommited = true;
    Rtos_ReleaseMutex(pCtx->PreInitMutex);
    return;
  }
  Rtos_ReleaseMutex(pCtx->PreInitMutex);

  if(pCtx->bCompleteInit)
    sFrmBufPool_Decommit(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static AL_TBuffer* sFrmBufPool_FlushOneDisplayBuffer(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  int32_t const iFrameID = sFrmBufPool_GetAvailableBuffer(pFrmBufPool);

  if(iFrameID == -1)
  {
    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return NULL;
  }

  AL_TPictMngrFrameData* pRemovedBuf = &pFrmBufPool->vFrameData[iFrameID];
  Rtos_Assert(pRemovedBuf->bAlreadyDisplayed || !pRemovedBuf->bToBeDisplayed);

  pRemovedBuf->iAccessCnt = 0;

  if(!pRemovedBuf->bAlreadyDisplayed)
    pRemovedBuf->bToBeDisplayed = true;

  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBuffersFromID(pFrmBufPool, iFrameID);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);

  return sRecBuffers_GetDisplayBuffer(pBuffers, pFrmBufPool);
}

/*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bCompleteInit)
    return NULL;

  return sFrmBufPool_FlushOneDisplayBuffer(&pCtx->FrmBufPool);
}

/*****************************************************************************/
void AL_PictMngr_GetAnnexBuffers(AL_TPictMngrCtx const* pCtx, AL_TIndex tAnnexID, TBuffer* pAnnex)
{
  sPictMngr_GetAnnexBuffers(pCtx, &pCtx->AnnexBufPool.pBufs[tAnnexID], pAnnex);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, TBuffer* pAnnex, AL_TPictMngrRefBuffers* pRefBuffers)
{
  Rtos_Assert(pCtx->bCompleteInit);

  if(!AL_IS_VALID_INDEX(pCtx->tFrameID))
    return false;

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, pCtx->tFrameID);
  pRecs->pFrame = pRecBuffers->pFrame;

  if(pAnnex && AL_IS_VALID_INDEX(pCtx->tAnnexID))
    sPictMngr_GetAnnexBuffers(pCtx, &pCtx->AnnexBufPool.pBufs[pCtx->tAnnexID], pAnnex);

  if(!pRefBuffers)
    return true;

  AL_TIndex pFrameIds[AL_MAX_REF];
  AL_TIndex pAnnexIds[AL_MAX_REF];

  bool bValidConceal = pSliceParam ? pSliceParam->bValidConceal : false;
  AL_TIndex tConcealPicID = pSliceParam ? pSliceParam->tConcealPicID : 0;
  uint8_t uMaxRef = AL_IReferenceManager_GetReflistIds(pCtx->pRefMngr, pFrameIds, pAnnexIds, pRefBuffers->pConcealIds, bValidConceal, tConcealPicID);

  for(int32_t i = 0; i < uMaxRef; ++i)
  {
    {
      if(AL_IS_VALID_INDEX(pFrameIds[i]))
      {
        AL_TRecBuffers* pBufs = sFrmBufPool_GetBuffersFromID(&pCtx->FrmBufPool, pFrameIds[i]);
        pRefBuffers->pRefBufs[i] = pBufs->pFrame;
      }
      else
        pRefBuffers->pRefBufs[i] = pRecs->pFrame;

      if(AL_IS_VALID_INDEX(pAnnexIds[i]))
        sPictMngr_GetAnnexBuffers(pCtx, &pCtx->AnnexBufPool.pBufs[pAnnexIds[i]], pRefBuffers->pAnnexBufs[i]);
      else
      {
        for(size_t j = 0; j < pCtx->zNumAnnexBuf; j++)
          pRefBuffers->pAnnexBufs[i][j] = pAnnex[j];
      }
    }
  }

  return true;
}

/*************************************************************************/
void AL_PictMngr_GetAnnexBuffersFromReferenceID(AL_TPictMngrCtx const* pCtx, AL_TIndex tRefId, TBuffer* pAnnexBuffers)
{
  AL_TIndex tAnnexID = AL_IReferenceManager_GetAnnexIdFromReferenceId(pCtx->pRefMngr, tRefId);
  Rtos_Assert(AL_IS_VALID_INDEX(tAnnexID));

  TBuffer const* pRawAnnexBuf = &pCtx->AnnexBufPool.pBufs[tAnnexID];
  sPictMngr_GetAnnexBuffers(pCtx, pRawAnnexBuf, pAnnexBuffers);
}

/*************************************************************************/
void AL_PictMngr_UpdateReferenceManager(AL_TPictMngrCtx* pCtx)
{
  AL_IReferenceManager_Update(pCtx->pRefMngr);
}

/*!@}*/
