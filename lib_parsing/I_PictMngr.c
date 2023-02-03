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

#include "I_PictMngr.h"

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common/Error.h"
#include "lib_common/PixMapBufferInternal.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_assert/al_assert.h"
#include "lib_decode/lib_decode.h"

/*************************************************************************/
static AL_TBuffer* sRecBuffers_GetDisplayBuffer(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pPool)
{
  (void)pPool;

  return tRecBuffers->pFrame;
}

/*************************************************************************/
static bool sRecBuffers_AreNull(AL_TRecBuffers const* tRecBuffers)
{
  bool bNull = NULL == tRecBuffers->pFrame;

  return bNull;
}

/*************************************************************************/
static bool sRecBuffers_AreNotNull(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pPool)
{
  (void)pPool;
  bool bNotNull = tRecBuffers->pFrame;

  return bNotNull;
}

/*************************************************************************/
static void sRecBuffers_Reset(AL_TRecBuffers* tRecBuffers)
{
  tRecBuffers->pFrame = NULL;
}

/*************************************************************************/
static void sRecBuffers_CleanUp(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pPool)
{
  (void)pPool;

  AL_Buffer_Cleanup(tRecBuffers->pFrame);

}

/*************************************************************************/
static void sRecBuffers_Release(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pPool)
{
  (void)pPool;
  AL_Buffer_Unref(tRecBuffers->pFrame);
}

/*************************************************************************/
static bool sRecBuffers_HasBuf(AL_TRecBuffers const* tRecBuffers, AL_TBuffer const* pBuf, AL_TFrmBufPool const* pPool)
{
  (void)pPool;
  bool bHasBuf = tRecBuffers->pFrame == pBuf;

  return bHasBuf;
}

/*************************************************************************/
static int sFrmBufPool_GetFrameIDFromBuf(AL_TFrmBufPool const* pPool, AL_TBuffer const* pBuf)
{
  Rtos_GetMutex(pPool->Mutex);

  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(sRecBuffers_HasBuf(&pPool->array[i].tRecBuffers, pBuf, pPool))
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return -1;
}

/*************************************************************************/
static int sFrmBufPool_GetFrameIDFromDisplay(AL_TFrmBufPool const* pPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pPool->Mutex);

  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(sRecBuffers_GetDisplayBuffer(&pPool->array[i].tRecBuffers, pPool) == pDisplayBuf)
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return -1;
}

/*************************************************************************/
static bool sFrmBufPoolFifo_IsInFifo(AL_TFrmBufPool const* pPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pPool->Mutex);

  for(int iCur = pPool->iFifoHead; iCur != -1; iCur = pPool->array[iCur].iNext)
  {
    if(sRecBuffers_GetDisplayBuffer(&pPool->array[iCur].tRecBuffers, pPool) == pDisplayBuf)
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return true;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return false;
}

/*************************************************************************/
static void AddBufferToFifo(AL_TFrmBufPool* pPool, int iFrameID, AL_TRecBuffers tRecBuffers)
{
  pPool->array[iFrameID].tRecBuffers = tRecBuffers;

  if(pPool->iFifoTail == -1 && pPool->iFifoHead == -1)
    pPool->iFifoHead = iFrameID;
  else
    pPool->array[pPool->iFifoTail].iNext = iFrameID;

  pPool->iFifoTail = iFrameID;

  pPool->array[iFrameID].iAccessCnt = 0;
  pPool->array[iFrameID].bWillBeOutputed = false;

  ++pPool->iBufNumber;
}

/*************************************************************************/
static bool sFrmBufPoolFifo_PushBack(AL_TFrmBufPool* pPool, AL_TRecBuffers tRecBuffers)
{
  bool const bRecNotNull = sRecBuffers_AreNotNull(&tRecBuffers, pPool);
  (void)bRecNotNull;
  AL_Assert(bRecNotNull);

  Rtos_GetMutex(pPool->Mutex);

  int iPoolIdx = 0;

  while(iPoolIdx < FRM_BUF_POOL_SIZE && (!sRecBuffers_AreNull(&pPool->array[iPoolIdx].tRecBuffers) ||
                                         (pPool->array[iPoolIdx].iNext != -1) ||
                                         (pPool->array[iPoolIdx].iAccessCnt != -1) ||
                                         pPool->array[iPoolIdx].bWillBeOutputed
                                         ))
    iPoolIdx++;

  if(iPoolIdx < FRM_BUF_POOL_SIZE)
  {
    AddBufferToFifo(pPool, iPoolIdx, tRecBuffers);
    Rtos_SetEvent(pPool->Event);
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return iPoolIdx < FRM_BUF_POOL_SIZE;
}

/*************************************************************************/
static int RemoveBufferFromFifo(AL_TFrmBufPool* pPool)
{
  int const iFrameID = pPool->iFifoHead;
  --pPool->iBufNumber;

  pPool->iFifoHead = pPool->array[pPool->iFifoHead].iNext;

  pPool->array[iFrameID].iNext = -1;
  pPool->array[iFrameID].iAccessCnt = 1;

  if(pPool->iFifoHead == -1)
    pPool->iFifoTail = pPool->iFifoHead;
  return iFrameID;
}

/*************************************************************************/
static int sFrmBufPoolFifo_Pop(AL_TFrmBufPool* pPool)
{
  Rtos_GetMutex(pPool->Mutex);

  while(true)
  {
    if(pPool->iBufNumber > 0)
      break;

    if(pPool->isDecommited)
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return UndefID;
    }

    Rtos_ReleaseMutex(pPool->Mutex);
    Rtos_WaitEvent(pPool->Event, AL_WAIT_FOREVER);
    Rtos_GetMutex(pPool->Mutex);
  }

  AL_Assert(pPool->iFifoHead != -1);
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pPool->array[pPool->iFifoHead].tRecBuffers, pPool);
  (void)bRecNotNull;
  AL_Assert(bRecNotNull);
  AL_Assert(pPool->array[pPool->iFifoHead].iAccessCnt == 0);
  AL_Assert(pPool->array[pPool->iFifoHead].bWillBeOutputed == false);

  sRecBuffers_CleanUp(&pPool->array[pPool->iFifoHead].tRecBuffers, pPool);
  pPool->array[pPool->iFifoHead].eError = AL_SUCCESS;

  int const iFrameID = RemoveBufferFromFifo(pPool);

  Rtos_ReleaseMutex(pPool->Mutex);
  return iFrameID;
}

/*************************************************************************/
static void sFrmBufPoolFifo_Decommit(AL_TFrmBufPool* pPool)
{
  Rtos_GetMutex(pPool->Mutex);
  pPool->isDecommited = true;
  Rtos_SetEvent(pPool->Event);
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_RemoveID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pFrame->tRecBuffers, pPool);
  (void)bRecNotNull;
  AL_Assert(bRecNotNull);
  AL_Assert(pFrame->iNext == -1);
  AL_Assert(pFrame->iAccessCnt == 0);

  sRecBuffers_Reset(&pFrame->tRecBuffers);
  pFrame->iAccessCnt = -1;
  pFrame->bWillBeOutputed = false;
  pFrame->iNext = -1;

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static AL_TRecBuffers sFrmBufPool_GetBufferFromID(AL_TFrmBufPool const* pPool, int iFrameID)
{
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pPool->array[iFrameID].tRecBuffers, pPool);
  (void)bRecNotNull;
  AL_Assert(bRecNotNull);
  return pPool->array[iFrameID].tRecBuffers;
}

/*************************************************************************/
static void sFrmBufPoolFifo_Init(AL_TFrmBufPool* pPool)
{
  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    sRecBuffers_Reset(&pPool->array[i].tRecBuffers);
    pPool->array[i].iNext = -1;
    pPool->array[i].iAccessCnt = -1;
    pPool->array[i].bWillBeOutputed = false;
  }

  pPool->iFifoHead = -1;
  pPool->iFifoTail = -1;
}

/*************************************************************************/
static bool sFrmBufPool_Init(AL_TFrmBufPool* pPool, AL_TAllocator* pAllocator, bool bHasSecondOutputFrame)
{
  (void)pAllocator;
  (void)bHasSecondOutputFrame;

  pPool->Mutex = Rtos_CreateMutex();

  if(!pPool->Mutex)
    goto fail_alloc_mutex;

  pPool->Event = Rtos_CreateEvent(0);
  pPool->iBufNumber = 0;

  if(!pPool->Event)
    goto fail_alloc_sem_free;

  sFrmBufPoolFifo_Init(pPool);

  return true;
  fail_alloc_sem_free:
  Rtos_DeleteMutex(pPool->Mutex);
  fail_alloc_mutex:
  return false;
}

/*************************************************************************/
static void sFrmBufPool_Deinit(AL_TFrmBufPool* pPool)
{
  Rtos_DeleteEvent(pPool->Event);
  Rtos_DeleteMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_DecrementBufID(AL_TFrmBufPool* pPool, int iFrameID, bool bForceOutput)
{
  Rtos_GetMutex(pPool->Mutex);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  AL_Assert(pFrame->iAccessCnt >= 1);
  pFrame->iAccessCnt--;

  if(pFrame->iAccessCnt == 0 && !pFrame->bOutEarly)
  {
    bool bWillBeOutputed = pFrame->bWillBeOutputed;
    AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
    sFrmBufPool_RemoveID(pPool, iFrameID);

    if((!bWillBeOutputed) || bForceOutput)
    {
      bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pPool)) == false;
      (void)bNotInFifo;
      AL_Assert(bNotInFifo);
      sFrmBufPoolFifo_PushBack(pPool, tBuffers);
    }
    else
      sRecBuffers_Release(&tBuffers, pPool);
  }

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_OutputBufID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  AL_Assert(pFrame->bWillBeOutputed == false);
  pFrame->bWillBeOutputed = true;
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_IncrementBufID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  AL_Assert(pFrame->iAccessCnt >= 0);
  pFrame->iAccessCnt++;
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*****************************************************************************/
static void sFrmBufPool_Terminate(AL_TFrmBufPool* pPool)
{
  (void)pPool; // Nothing to do
}

/*************************************************************************/
static bool sMvBufPool_Init(AL_TMvBufPool* pPool, int iMaxBuf)
{
  AL_Assert(iMaxBuf <= MAX_DPB_SIZE);

  for(int i = 0; i < iMaxBuf; ++i)
  {
    pPool->pFreeIDs[i] = i;
    pPool->iAccessCnt[i] = 0;
    AL_CleanupMemory(pPool->pMvBufs[i].tMD.pVirtualAddr, pPool->pMvBufs[i].tMD.uSize);
    AL_CleanupMemory(pPool->pPocBufs[i].tMD.pVirtualAddr, pPool->pPocBufs[i].tMD.uSize);
  }

  pPool->iBufCnt = iMaxBuf;

  for(int i = iMaxBuf; i < MAX_DPB_SIZE; ++i)
  {
    pPool->pFreeIDs[i] = UndefID;
    pPool->iAccessCnt[i] = 0;
  }

  pPool->iFreeCnt = iMaxBuf;
  pPool->Mutex = Rtos_CreateMutex();

  if(!pPool->Mutex)
    goto fail_alloc_mutex;

  pPool->Semaphore = Rtos_CreateSemaphore(iMaxBuf);

  if(!pPool->Semaphore)
    goto fail_alloc_sem;

  return true;
  fail_alloc_sem:
  Rtos_DeleteMutex(pPool->Mutex);
  fail_alloc_mutex:
  return false;
}

/*************************************************************************/
static void sMvBufPool_Deinit(AL_TMvBufPool* pPool)
{
  Rtos_DeleteSemaphore(pPool->Semaphore);
  Rtos_DeleteMutex(pPool->Mutex);
}

/*************************************************************************/
static uint8_t sMvBufPool_GetFreeBufID(AL_TMvBufPool* pPool)
{
  Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_GetMutex(pPool->Mutex);
  uint8_t uMvID = pPool->pFreeIDs[--pPool->iFreeCnt];
  AL_Assert(pPool->iAccessCnt[uMvID] == 0);
  pPool->iAccessCnt[uMvID] = 1;

  if(AL_CLEAN_BUFFERS)
    Rtos_Memset(pPool->pMvBufs[uMvID].tMD.pVirtualAddr, 0, pPool->pMvBufs[uMvID].tMD.uSize);

  Rtos_ReleaseMutex(pPool->Mutex);

  return uMvID;
}

/*************************************************************************/
static void sMvBufPool_DecrementBufID(AL_TMvBufPool* pPool, uint8_t uMvID)
{
  AL_Assert(uMvID < MAX_DPB_SIZE);

  Rtos_GetMutex(pPool->Mutex);

  bool bFree = false;

  if(pPool->iAccessCnt[uMvID])
  {
    bFree = (--pPool->iAccessCnt[uMvID] == 0);

    if(bFree)
      pPool->pFreeIDs[pPool->iFreeCnt++] = uMvID;
  }

  Rtos_ReleaseMutex(pPool->Mutex);

  if(bFree)
    Rtos_ReleaseSemaphore(pPool->Semaphore);
}

/*************************************************************************/
static void sMvBufPool_IncrementBufID(AL_TMvBufPool* pPool, int iMvID)
{
  AL_Assert(iMvID < FRM_BUF_POOL_SIZE);
  Rtos_GetMutex(pPool->Mutex);
  Rtos_AtomicIncrement(&(pPool->iAccessCnt[iMvID]));
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*****************************************************************************/
static void sMvBufPool_Terminate(AL_TMvBufPool* pPool)
{
  for(int i = 0; i < pPool->iBufCnt; ++i)
    Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);

  for(int i = 0; i < pPool->iBufCnt; ++i)
    Rtos_ReleaseSemaphore(pPool->Semaphore);
}

/*************************************************************************/
static void sPictMngr_DecrementFrmBuf(void* pUserParam, int iFrameID)
{
  AL_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID, pCtx->bForceOutput);
}

/*************************************************************************/
static void sPictMngr_IncrementFrmBuf(void* pUserParam, int iFrameID)
{
  AL_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_IncrementBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_OutputFrmBuf(void* pUserParam, int iFrameID)
{
  AL_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_OutputBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  AL_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_IncrementBufID(&pCtx->MvBufPool, uMvID);
}

/*************************************************************************/
static void sPictMngr_DecrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  AL_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_DecrementBufID(&pCtx->MvBufPool, uMvID);
}

/*****************************************************************************/
static bool CheckPictMngrInitParameter(AL_TPictMngrParam* pParam)
{
  if(pParam->iSizeMV < 0)
    return false;

  if(pParam->iNumMV < 0)
    return false;

  if(pParam->iNumDPBRef < 0)
    return false;

  if(pParam->iNumMV < pParam->iNumDPBRef)
    return false;

  if(pParam->eDPBMode >= AL_DPB_MAX_ENUM)
    return false;

  if(pParam->eFbStorageMode >= AL_FB_MAX_ENUM)
    return false;

  if(pParam->iBitdepth < 0)
    return false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_PreInit(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx)
    return false;

  pCtx->FirstInitMutex = Rtos_CreateMutex();

  if(!pCtx->FirstInitMutex)
    return false;

  pCtx->bFirstInit = false;
  pCtx->FrmBufPool.isDecommited = false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, AL_TPictMngrParam* pParam)
{
  if(!pCtx)
    return false;

  if(!CheckPictMngrInitParameter(pParam))
    return false;

  if(!sMvBufPool_Init(&pCtx->MvBufPool, pParam->iNumMV))
    return false;

  bool bEnableSecondOutput = false;
  AL_TAllocator* pAllocator = NULL;

  if(!sFrmBufPool_Init(&pCtx->FrmBufPool, pAllocator, bEnableSecondOutput))
    return false;

  AL_TDpbCallback tCallbacks =
  {
    sPictMngr_IncrementFrmBuf,
    sPictMngr_DecrementFrmBuf,
    sPictMngr_OutputFrmBuf,
    sPictMngr_IncrementMvBuf,
    sPictMngr_DecrementMvBuf,
    pCtx,
  };

  AL_Dpb_Init(&pCtx->DPB, pParam->iNumDPBRef, pParam->eDPBMode, tCallbacks);

  pCtx->eFbStorageMode = pParam->eFbStorageMode;
  pCtx->iBitdepth = pParam->iBitdepth;
  pCtx->uSizeMV = pParam->iSizeMV;
  pCtx->uSizePOC = POCBUFF_PL_SIZE;
  pCtx->iCurFramePOC = 0;

  pCtx->uPrevPocLSB = 0;
  pCtx->iPrevPocMSB = 0;

  pCtx->uRecID = UndefID;
  pCtx->uMvID = UndefID;

  pCtx->uNumSlice = 0;
  pCtx->iPrevFrameNum = -1;
  Rtos_GetMutex(pCtx->FirstInitMutex);
  pCtx->bFirstInit = true;
  Rtos_ReleaseMutex(pCtx->FirstInitMutex);
  pCtx->bForceOutput = pParam->bForceOutput;

  pCtx->tOutputPosition = pParam->tOutputPosition;

  return true;
}

/*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bFirstInit)
    return;

  AL_Dpb_Terminate(&pCtx->DPB);

  sFrmBufPool_Terminate(&pCtx->FrmBufPool);
  sMvBufPool_Terminate(&pCtx->MvBufPool);
}

/*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx)
{
  Rtos_DeleteMutex(pCtx->FirstInitMutex);

  if(!pCtx->bFirstInit)
    return;

  sMvBufPool_Deinit(&pCtx->MvBufPool);
  sFrmBufPool_Deinit(&pCtx->FrmBufPool);
  AL_Dpb_Deinit(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_LockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_IncrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_IncrementMvBuf(pCtx, pRefMvID[uRef]);
  }
}

/*****************************************************************************/
void AL_PictMngr_UnlockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_DecrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_DecrementMvBuf(pCtx, pRefMvID[uRef]);
  }
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->uRecID;
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->uMvID;
}

/*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->iCurFramePOC;
}

/***************************************************************************/
static void ChangePictChromaMode(AL_TBuffer* pBuf, AL_EChromaMode eChromaMode)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TPicFormat tPicFmt;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFmt);
  (void)bSuccess;
  AL_Assert(bSuccess);
  tPicFmt = AL_GetDecPicFormat(eChromaMode, tPicFmt.uBitDepth, tPicFmt.eStorageMode, tPicFmt.bCompressed);
  tFourCC = AL_GetDecFourCC(tPicFmt);
  AL_PixMapBuffer_SetFourCC(pBuf, tFourCC);
}

/***************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  pCtx->uRecID = sFrmBufPoolFifo_Pop(&pCtx->FrmBufPool);

  if(pCtx->uRecID == UndefID)
    return false;

  pCtx->uMvID = sMvBufPool_GetFreeBufID(&pCtx->MvBufPool);
  AL_Assert(pCtx->uMvID != UndefID);

  AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uRecID);

  AL_PixMapBuffer_SetDimension(tBuffers.pFrame, tDim);
  ChangePictChromaMode(tBuffers.pFrame, eChromaMode);

  pCtx->FrmBufPool.array[pCtx->uRecID].bStartsNewCVS = bStartsNewCVS;

  return true;
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackReleaseIsDone(AL_TFrmBufPool* pPool, AL_TBuffer* pReleasedFrame)
{
  Rtos_GetMutex(pPool->Mutex);
  int const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pReleasedFrame);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pPool, pReleasedFrame) == false;
  (void)bNotInFifo;
  AL_Assert(bNotInFifo);
  AL_Assert(pPool->array[iFrameID].bWillBeOutputed || pPool->array[iFrameID].bOutEarly);
  pPool->array[iFrameID].bWillBeOutputed = false;
  AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  sFrmBufPool_RemoveID(pPool, iFrameID);
  sRecBuffers_Release(&tBuffers, pPool);
  Rtos_ReleaseMutex(pPool->Mutex);
}

/***************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx)
{

  if(pCtx->uMvID != UndefID)
  {
    sMvBufPool_DecrementBufID(&pCtx->MvBufPool, pCtx->uMvID);
    pCtx->uMvID = UndefID;
  }

  if(pCtx->uRecID != UndefID)
  {
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, pCtx->uRecID, pCtx->bForceOutput);
    pCtx->uRecID = UndefID;
  }
}

/*************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bFirstInit)
    return;

  AL_Dpb_Flush(&pCtx->DPB);
  AL_Dpb_BeginNewSeq(&pCtx->DPB);
}

/*************************************************************************/
void AL_PictMngr_UpdateDPBInfo(AL_TPictMngrCtx* pCtx, uint8_t uMaxRef)
{
  AL_Dpb_SetNumRef(&pCtx->DPB, uMaxRef);
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx const* pCtx)
{
  return AL_Dpb_GetLastPicID(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, AL_EPicStruct ePicStruct, uint32_t uPocLsb, int iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT, uint8_t uSubpicFlag)
{
  uint8_t uNode = AL_Dpb_GetNextFreeNode(&pCtx->DPB);

  if(!AL_Dpb_NodeIsReset(&pCtx->DPB, uNode))
    AL_Dpb_Remove(&pCtx->DPB, uNode);

  AL_Dpb_Insert(&pCtx->DPB, iFramePOC, ePicStruct, uPocLsb, uNode, iFrameID, uMvID, pic_output_flag, eMarkingFlag, uNonExisting, eNUT, uSubpicFlag);
}

/***************************************************************************/
static void sFrmBufPool_UpdateCRC(AL_TFrmBufPool* pPool, int iFrameID, uint32_t uCRC)
{
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].uCRC = uCRC;
}

/***************************************************************************/
static void sFrmBufPool_UpdateCrop(AL_TFrmBufPool* pPool, int iFrameID, AL_TCropInfo tCrop)
{
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].tCrop = tCrop;
}

/***************************************************************************/
static void sFrmBufPool_UpdatePicStruct(AL_TFrmBufPool* pPool, int iFrameID, AL_EPicStruct ePicStruct)
{
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].ePicStruct = ePicStruct;
}

/***************************************************************************/
static void sFrmBufPool_UpdateError(AL_TFrmBufPool* pPool, int iFrameID, AL_ERR error)
{
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].eError = error;
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, int iFrameID, uint32_t uCRC)
{
  sFrmBufPool_UpdateCRC(&pCtx->FrmBufPool, iFrameID, uCRC);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TCropInfo tCrop)
{
  sFrmBufPool_UpdateCrop(&pCtx->FrmBufPool, iFrameID, tCrop);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferPicStruct(AL_TPictMngrCtx* pCtx, int iFrameID, AL_EPicStruct ePicStruct)
{
  sFrmBufPool_UpdatePicStruct(&pCtx->FrmBufPool, iFrameID, ePicStruct);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferError(AL_TPictMngrCtx* pCtx, int iFrameID, AL_ERR eError)
{
  sFrmBufPool_UpdateError(&pCtx->FrmBufPool, iFrameID, eError);
}

/***************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, int iFrameID)
{
  AL_Dpb_EndDecoding(&pCtx->DPB, iFrameID);
}

/***************************************************************************/
void AL_PictMngr_UnlockID(AL_TPictMngrCtx* pCtx, int iFrameID, int iMotionVectorID)
{
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID, pCtx->bForceOutput);
  sMvBufPool_DecrementBufID(&pCtx->MvBufPool, iMotionVectorID);
}

/***************************************************************************/
static void sFrmBufPool_GetInfoDecode(AL_TPictMngrCtx const* pCtx, int iFrameID, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_EFbStorageMode eFbStorageMode, int iBitdepth, bool bDisplayInfo)
{
  (void)iBitdepth;
  AL_Assert(pInfo);

  AL_TFrmBufPool const* pPool = &pCtx->FrmBufPool;

  AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  bool const bRecNotNull = sRecBuffers_AreNotNull(&tBuffers, pPool);
  (void)bRecNotNull;
  AL_Assert(bRecNotNull);

  AL_TBuffer* pBuf = bDisplayInfo ? sRecBuffers_GetDisplayBuffer(&tBuffers, pPool) : tBuffers.pFrame;
  AL_TPixMapMetaData* pMetaSrc = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);
  AL_Assert(pMetaSrc);

  pInfo->tCrop = pPool->array[iFrameID].tCrop;
  pInfo->uBitDepthY = pInfo->uBitDepthC = AL_GetBitDepth(pMetaSrc->tFourCC);
  pInfo->uCRC = pPool->array[iFrameID].uCRC;
  pInfo->tDim = AL_PixMapBuffer_GetDimension(pBuf);
  pInfo->eFbStorageMode = eFbStorageMode;
  pInfo->ePicStruct = pPool->array[iFrameID].ePicStruct;
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_Assert(tFourCC != 0);
  pInfo->eChromaMode = AL_GetChromaMode(tFourCC);
  pInfo->tPos = pCtx->tOutputPosition;

  if(pStartsNewCVS)
    *pStartsNewCVS = pPool->array[iFrameID].bStartsNewCVS;
}

/***************************************************************************/
static AL_TBuffer* AL_PictMngr_GetDisplayBuffer2(AL_TPictMngrCtx const* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, int iFrameID)
{
  if(iFrameID == UndefID)
    return NULL;

  AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;

  if(pInfo)
    sFrmBufPool_GetInfoDecode(pCtx, iFrameID, pInfo, pStartsNewCVS, eOutputStorageMode, pCtx->iBitdepth, true);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);

  return sRecBuffers_GetDisplayBuffer(&tRecBuffers, &pCtx->FrmBufPool);
}

/***************************************************************************/
AL_TBuffer* AL_PictMngr_ForceDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, int iFrameID)
{
  AL_Assert(pCtx->bForceOutput);
  AL_TFrmBufPool* pPool = &pCtx->FrmBufPool;
  Rtos_GetMutex(pPool->Mutex);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  pFrame->bOutEarly = true;
  Rtos_ReleaseMutex(pPool->Mutex);
  return AL_PictMngr_GetDisplayBuffer2(pCtx, pInfo, pStartsNewCVS, iFrameID);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx const* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS)
{
  if(!pCtx->bFirstInit)
    return NULL;

  return AL_PictMngr_GetDisplayBuffer2(pCtx, pInfo, pStartsNewCVS, AL_Dpb_GetDisplayBuffer(&pCtx->DPB));
}

/*************************************************************************/
static void sPictMngr_ReleaseDisplayBuffer(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  AL_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);

  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];

  if(pFrame->bOutEarly)
  {
    pFrame->bOutEarly = false;

    if((pFrame->iAccessCnt == 0) && pFrame->bWillBeOutputed)
    {
      pFrame->bWillBeOutputed = false;
      AL_TRecBuffers pBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
      sFrmBufPool_RemoveID(pPool, iFrameID);
      bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pPool, sRecBuffers_GetDisplayBuffer(&pBuffers, pPool)) == false;
      (void)bNotInFifo;
      AL_Assert(bNotInFifo);
      sFrmBufPoolFifo_PushBack(pPool, pBuffers);
    }

    Rtos_ReleaseMutex(pPool->Mutex);
    return;
  }

  AL_Assert(pFrame->bWillBeOutputed == true);

  pFrame->bWillBeOutputed = false;

  if(pFrame->iAccessCnt == 0)
  {
    AL_TRecBuffers pBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
    sFrmBufPool_RemoveID(pPool, iFrameID);
    bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pPool, sRecBuffers_GetDisplayBuffer(&pBuffers, pPool)) == false;
    (void)bNotInFifo;
    AL_Assert(bNotInFifo);
    sFrmBufPoolFifo_PushBack(pPool, pBuffers);
  }

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx const* pCtx, int iFrameID)
{
  AL_Assert(pCtx);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
  return sRecBuffers_GetDisplayBuffer(&tRecBuffers, &pCtx->FrmBufPool);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx const* pCtx, int iFrameID)
{
  AL_Assert(pCtx);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
  return tRecBuffers.pFrame;
}

/*************************************************************************/
static AL_TBuffer* sFrmBufPool_GetRecBufferFromDisplayBuffer(AL_TFrmBufPool const* pPool, AL_TBuffer const* pDisplayBuf, int* iFrameID)
{
  AL_TBuffer* pRecBuf = NULL;

  Rtos_GetMutex(pPool->Mutex);

  AL_Assert(pDisplayBuf);

  *iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pDisplayBuf);

  if(*iFrameID != -1)
    pRecBuf = sFrmBufPool_GetBufferFromID(pPool, *iFrameID).pFrame;

  Rtos_ReleaseMutex(pPool->Mutex);

  return pRecBuf;
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromDisplayBuffer(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pDisplayBuf, AL_TInfoDecode* pInfo)
{
  if(pDisplayBuf == NULL)
    return NULL;

  int iFrameID;
  AL_TBuffer* pRecBuf = sFrmBufPool_GetRecBufferFromDisplayBuffer(&pCtx->FrmBufPool, pDisplayBuf, &iFrameID);

  if(pInfo != NULL && pRecBuf != NULL)
  {
    AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;
    sFrmBufPool_GetInfoDecode(pCtx, iFrameID, pInfo, NULL, eOutputStorageMode, pCtx->iBitdepth, false);
  }

  return pRecBuf;
}

/*************************************************************************/
bool AL_PictMngr_GetFrameEncodingError(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pDisplayBuf, AL_ERR* pError)
{
  if(pDisplayBuf == NULL)
    return false;

  int iFrameID = sFrmBufPool_GetFrameIDFromBuf(&pCtx->FrmBufPool, pDisplayBuf);

  if(pError != NULL && iFrameID != -1)
  {
    *pError = pCtx->FrmBufPool.array[iFrameID].eError;
    return true;
  }

  return false;
}

/*************************************************************************/
static bool sFrmBufPool_FillRecBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayBuf, AL_TRecBuffers* pRecBuffers)
{
  (void)pPool;

  pRecBuffers->pFrame = pDisplayBuf;

  AL_Buffer_Ref(pDisplayBuf);

  return true;
}

/*************************************************************************/
static bool sFrmBufPool_PutDisplayBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pPool->Mutex);
  bool bSucceed = false;

  AL_Assert(pBuf);

  int const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pBuf);

  if(iFrameID == -1)
  {
    AL_TRecBuffers tRecBuffers;

    if(sFrmBufPool_FillRecBuffer(pPool, pBuf, &tRecBuffers))
    {
      if(sFrmBufPoolFifo_PushBack(pPool, tRecBuffers))
        bSucceed = true;
      else
        sRecBuffers_Release(&tRecBuffers, pPool);
    }
  }
  else
  {
    sPictMngr_ReleaseDisplayBuffer(pPool, iFrameID);
    bSucceed = true;
  }

  Rtos_ReleaseMutex(pPool->Mutex);

  return bSucceed;
}

/*************************************************************************/
bool AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  return sFrmBufPool_PutDisplayBuffer(&pCtx->FrmBufPool, pBuf);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx)
{
  AL_Dpb_ReleaseDisplayBuffer(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame)
{
  sFrmBufPool_SignalCallbackReleaseIsDone(&pCtx->FrmBufPool, pReleasedFrame);
}

/*****************************************************************************/
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx)
{
  Rtos_GetMutex(pCtx->FirstInitMutex);

  if(!pCtx->bFirstInit)
  {
    pCtx->FrmBufPool.isDecommited = true;
    Rtos_ReleaseMutex(pCtx->FirstInitMutex);
    return;
  }
  Rtos_ReleaseMutex(pCtx->FirstInitMutex);

  sFrmBufPoolFifo_Decommit(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static AL_TBuffer* sFrmBufPoolFifo_FlushOneDisplayBuffer(AL_TFrmBufPool* pPool)
{
  Rtos_GetMutex(pPool->Mutex);

  int iFifoHead = pPool->iFifoHead;

  if((iFifoHead == -1) && (pPool->iFifoTail == -1))
  {
    Rtos_ReleaseMutex(pPool->Mutex);
    return NULL;
  }

  AL_Assert(iFifoHead != -1);
  AL_TFrameFifo* headBuf = &pPool->array[iFifoHead];
  AL_Assert(headBuf->iAccessCnt == 0);

  if(!headBuf->bOutEarly)
    AL_Assert(headBuf->bWillBeOutputed == false);

  int const iFrameID = RemoveBufferFromFifo(pPool);
  AL_TFrameFifo* removedBuf = &pPool->array[iFrameID];
  removedBuf->iAccessCnt = 0;

  if(!removedBuf->bOutEarly)
    removedBuf->bWillBeOutputed = true;
  AL_TRecBuffers pBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  Rtos_ReleaseMutex(pPool->Mutex);

  return sRecBuffers_GetDisplayBuffer(&pBuffers, pPool);
}

/*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bFirstInit)
    return NULL;

  return sFrmBufPoolFifo_FlushOneDisplayBuffer(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static void FillPocAndLongtermLists(AL_TDpb const* pDpb, TBufferPOC* pPoc, AL_TDecSliceParam const* pSP, TBufferListRef const* pListRef)
{
  int32_t* pPocList = (int32_t*)(pPoc->tMD.pVirtualAddr);
  uint32_t* pLongTermList = (uint32_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);
  uint32_t* pSubpicList = (uint32_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_SUBPIC_OFFSET);

  if(!pSP->FirstLcuSliceSegment)
  {
    *pLongTermList = 0;
    *pSubpicList = 0;

    for(int i = 0; i < MAX_REF; ++i)
      pPocList[i] = 0xFFFFFFFF;
  }

  AL_ESliceType eType = (AL_ESliceType)pSP->eSliceType;

  if(eType != AL_SLICE_I)
    AL_Dpb_FillList(pDpb, 0, pListRef, pPocList, pLongTermList, pSubpicList);

  if(eType == AL_SLICE_B)
    AL_Dpb_FillList(pDpb, 1, pListRef, pPocList, pLongTermList, pSubpicList);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx const* pCtx, AL_TDecSliceParam const* pSP, TBufferListRef const* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, AL_TRecBuffers* pRecs)
{
  (void)pListVirtAddr; // only used for traces

  if(pCtx->uRecID == UndefID)
    return false;

  AL_TRecBuffers pRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uRecID);
  pRecs->pFrame = pRecBuffers.pFrame;

  if(pMV && pPOC)
  {
    if(pCtx->uMvID == UndefID)
      return false;

    pMV->tMD = pCtx->MvBufPool.pMvBufs[pCtx->uMvID].tMD;
    pPOC->tMD = pCtx->MvBufPool.pPocBufs[pCtx->uMvID].tMD;

    FillPocAndLongtermLists(&pCtx->DPB, pPOC, (AL_TDecSliceParam const*)pSP, (TBufferListRef const*)pListRef);
  }

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pRecs->pFrame);
  AL_EChromaOrder eChromaOrder = AL_GetChromaOrder(tFourCC);
  AL_EPlaneId eFirstCPlane = eChromaOrder == AL_C_ORDER_U_V ? AL_PLANE_U : AL_PLANE_UV;

  if(pListAddr && pListAddr->tMD.pVirtualAddr)
  {
    TRefListOffsets tRefListOffsets;
    AL_GetRefListOffsets(&tRefListOffsets, AL_CODEC_INVALID, eChromaOrder, sizeof(AL_PADDR));

    AL_PADDR* pAddr = (AL_PADDR*)pListAddr->tMD.pVirtualAddr;
    AL_PADDR* pColocMvList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uColocMVOffset);
    AL_PADDR* pColocPocList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uColocPocOffset);
    AL_PADDR* pFbcList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uMapOffset);

    for(int i = 0; i < PIC_ID_POOL_SIZE; ++i)
    {
      uint8_t uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, i);

      if(uNodeID == UndefID)
      {
        if(pSP->ValidConceal && pCtx->DPB.uCountPic)
          uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, pSP->ConcealPicID);
      }

      AL_TBuffer* pRefBuf = NULL;
      TBufferMV const* pRefMV = NULL;
      TBufferPOC const* pRefPOC = NULL;

      if(uNodeID != UndefID)
      {
        int iFrameID = AL_Dpb_GetFrmID_FromNode(&pCtx->DPB, uNodeID);
        uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->DPB, uNodeID);
        AL_TRecBuffers pBufs = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
        pRefBuf = pBufs.pFrame;
        pRefMV = &pCtx->MvBufPool.pMvBufs[uMvID];
        pRefPOC = &pCtx->MvBufPool.pPocBufs[uMvID];
      }
      else
      {
        // Use current Rec & MV Buffers to avoid non existing ref/coloc
        pRefBuf = pRecs->pFrame;
        pRefMV = pMV;
        pRefPOC = pPOC;
      }

      pAddr[i] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, AL_PLANE_Y);
      pAddr[PIC_ID_POOL_SIZE + i] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, eFirstCPlane);
      pAddr[i] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, pCtx->tOutputPosition, AL_PLANE_Y);
      pAddr[PIC_ID_POOL_SIZE + i] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, pCtx->tOutputPosition, eFirstCPlane);

      pFbcList[i] = 0;
      pFbcList[PIC_ID_POOL_SIZE + i] = 0;

      pColocMvList[i] = pRefMV->tMD.uPhysicalAddr;
      pColocPocList[i] = pRefPOC->tMD.uPhysicalAddr;

    }
  }
  return true;
}

/*@}*/

