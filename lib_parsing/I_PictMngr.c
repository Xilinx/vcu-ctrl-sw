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

#include <assert.h>
#include "lib_common/BufferSrcMeta.h"
#include "I_PictMngr.h"
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/IpDecFourCC.h"

/*************************************************************************/
static AL_TBuffer* sRecBuffers_GetDisplayBuffer(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool* pPool)
{
  (void)pPool;

  return tRecBuffers->pFrame;
}

/*************************************************************************/
static bool sRecBuffers_AreNull(AL_TRecBuffers* tRecBuffers)
{
  bool bNull = NULL == tRecBuffers->pFrame;


  return bNull;
}

/*************************************************************************/
static bool sRecBuffers_AreNotNull(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool* pPool)
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
static void sRecBuffers_CleanUp(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool* pPool)
{
  (void)pPool;
  AL_CleanupMemory(AL_Buffer_GetData(tRecBuffers->pFrame), tRecBuffers->pFrame->zSize);

}

/*************************************************************************/
static bool sRecBuffers_HasBuf(AL_TRecBuffers* tRecBuffers, AL_TBuffer* pBuf, AL_TFrmBufPool* pPool)
{
  (void)pPool;
  bool bHasBuf = tRecBuffers->pFrame == pBuf;


  return bHasBuf;
}

/*************************************************************************/
static int sFrmBufPool_GetFrameIDFromBuf(AL_TFrmBufPool* pPool, AL_TBuffer* pBuf)
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
static int sFrmBufPool_GetFrameIDFromDisplay(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayBuf)
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
static bool sFrmBufPoolFifo_IsInFifo(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayBuf)
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
static void sFrmBufPoolFifo_PushBack(AL_TFrmBufPool* pPool, AL_TRecBuffers tRecBuffers)
{
  assert(sRecBuffers_AreNotNull(&tRecBuffers, pPool));

  Rtos_GetMutex(pPool->Mutex);

  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(sRecBuffers_AreNull(&pPool->array[i].tRecBuffers) &&
       (pPool->array[i].iNext == -1) &&
       (pPool->array[i].iAccessCnt == -1) &&
       (pPool->array[i].bWillBeOutputed == false)
       )
    {
      AddBufferToFifo(pPool, i, tRecBuffers);
      Rtos_SetEvent(pPool->Event);
      Rtos_ReleaseMutex(pPool->Mutex);
      return;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  assert(0);
}

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

  assert(pPool->iFifoHead != -1);
  assert(sRecBuffers_AreNotNull(&pPool->array[pPool->iFifoHead].tRecBuffers, pPool));
  assert(pPool->array[pPool->iFifoHead].iAccessCnt == 0);
  assert(pPool->array[pPool->iFifoHead].bWillBeOutputed == false);

  sRecBuffers_CleanUp(&pPool->array[pPool->iFifoHead].tRecBuffers, pPool);
  pPool->array[pPool->iFifoHead].eError = AL_SUCCESS;

  int const iFrameID = RemoveBufferFromFifo(pPool);

  Rtos_ReleaseMutex(pPool->Mutex);
  return iFrameID;
}

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
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];

  assert(sRecBuffers_AreNotNull(&pFrame->tRecBuffers, pPool));
  assert(pFrame->iNext == -1);
  assert(pFrame->iAccessCnt == 0);

  sRecBuffers_Reset(&pFrame->tRecBuffers);
  pFrame->iAccessCnt = -1;
  pFrame->bWillBeOutputed = false;
  pFrame->iNext = -1;

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static AL_TRecBuffers sFrmBufPool_GetBufferFromID(AL_TFrmBufPool* pPool, int iFrameID)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  assert(sRecBuffers_AreNotNull(&pPool->array[iFrameID].tRecBuffers, pPool));
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
static bool sFrmBufPool_Init(AL_TFrmBufPool* pPool, bool bHasRasterFrame)
{
  (void)bHasRasterFrame;

  pPool->Mutex = Rtos_CreateMutex();

  if(!pPool->Mutex)
    goto fail_alloc_mutex;

  pPool->Event = Rtos_CreateEvent(0);
  pPool->iBufNumber = 0;
  pPool->isDecommited = false;

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
static void sFrmBufPool_DecrementBufID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  assert(pFrame->iAccessCnt >= 1);
  pFrame->iAccessCnt--;

  if(pFrame->iAccessCnt == 0)
  {
    if(pFrame->bWillBeOutputed)
    {
      AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
      sFrmBufPool_RemoveID(pPool, iFrameID);
      AL_Buffer_Unref(sRecBuffers_GetDisplayBuffer(&tBuffers, pPool));
    }
    else
    {
      AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
      sFrmBufPool_RemoveID(pPool, iFrameID);
      assert(sFrmBufPoolFifo_IsInFifo(pPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pPool)) == false);
      sFrmBufPoolFifo_PushBack(pPool, tBuffers);
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_OutputBufID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  assert(pFrame->bWillBeOutputed == false);
  pFrame->bWillBeOutputed = true;
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_IncrementBufID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];
  assert(pFrame->iAccessCnt >= 0);
  pFrame->iAccessCnt++;
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static bool sMvBufPool_Init(AL_TMvBufPool* pPool, int iMaxBuf)
{
  assert(iMaxBuf <= MAX_DPB_SIZE);

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
static int sMvBufPool_GetFreeBufID(AL_TMvBufPool* pPool)
{
  Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_GetMutex(pPool->Mutex);
  int iMvID = pPool->pFreeIDs[--pPool->iFreeCnt];
  assert(pPool->iAccessCnt[iMvID] == 0);
  pPool->iAccessCnt[iMvID] = 1;
  Rtos_ReleaseMutex(pPool->Mutex);

  return iMvID;
}

/*************************************************************************/
static void sMvBufPool_DecrementBufID(AL_TMvBufPool* pPool, uint8_t uMvID)
{
  assert(uMvID < MAX_DPB_SIZE);

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
  assert(iMvID < FRM_BUF_POOL_SIZE);
  Rtos_GetMutex(pPool->Mutex);
  Rtos_AtomicIncrement(&(pPool->iAccessCnt[iMvID]));
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void sPictMngr_DecrementFrmBuf(void* pUserParam, int iFrameID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementFrmBuf(void* pUserParam, int iFrameID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_IncrementBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_OutputFrmBuf(void* pUserParam, int iFrameID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_OutputBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_IncrementBufID(&pCtx->MvBufPool, uMvID);
}

/*************************************************************************/
static void sPictMngr_DecrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_DecrementBufID(&pCtx->MvBufPool, uMvID);
}

/***************************************************************************/
/*         P i c t u r e    M a n a g e r     f u n c t i o n s            */
/***************************************************************************/

/*****************************************************************************/
static bool CheckPictMngrInitParameter(int iNumMV, int iSizeMV, int iNumDPBRef, AL_EDpbMode eDPBMode, AL_EFbStorageMode eFbStorageMode, int iBitdepth)
{
  if(iSizeMV < 0)
    return false;

  if(iNumMV < 0)
    return false;

  if(iNumDPBRef < 0)
    return false;

  if(iNumMV < iNumDPBRef)
    return false;

  if(eDPBMode >= AL_DPB_MAX_ENUM)
    return false;

  if(eFbStorageMode >= AL_FB_MAX_ENUM)
    return false;

  if(iBitdepth < 0)
    return false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, AL_TAllocator* pAllocator, int iNumMV, int iSizeMV, int iNumDPBRef, AL_EDpbMode eDPBMode, AL_EFbStorageMode eFbStorageMode, int iBitdepth, bool bEnableRasterOutput)
{
  if(!pCtx)
    return false;

  if(!CheckPictMngrInitParameter(iNumMV, iSizeMV, iNumDPBRef, eDPBMode, eFbStorageMode, iBitdepth))
    return false;

  if(!sMvBufPool_Init(&pCtx->MvBufPool, iNumMV))
    return false;

  if(!sFrmBufPool_Init(&pCtx->FrmBufPool, bEnableRasterOutput))
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

  AL_Dpb_Init(&pCtx->DPB, iNumDPBRef, eDPBMode, tCallbacks);

  pCtx->eFbStorageMode = eFbStorageMode;
  pCtx->iBitdepth = iBitdepth;
  pCtx->uSizeMV = iSizeMV;
  pCtx->uSizePOC = POCBUFF_PL_SIZE;
  pCtx->iCurFramePOC = 0;

  pCtx->uPrevPocLSB = 0;
  pCtx->iPrevPocMSB = 0;

  pCtx->uRecID = UndefID;
  pCtx->uMvID = UndefID;

  pCtx->uNumSlice = 0;
  pCtx->iPrevFrameNum = -1;
  pCtx->bFirstInit = true;

  pCtx->pAllocator = pAllocator;

  return true;
}

/*****************************************************************************/
static void sFrmBufPool_Terminate(AL_TFrmBufPool* pPool)
{
  (void)pPool; // Nothing to do
}

/*****************************************************************************/
static void sMvBufPool_Terminate(AL_TMvBufPool* pPool)
{
  for(int i = 0; i < pPool->iBufCnt; ++i)
    Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);

  for(int i = 0; i < pPool->iBufCnt; ++i)
    Rtos_ReleaseSemaphore(pPool->Semaphore);
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
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx* pCtx)
{
  return pCtx->uRecID;
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx* pCtx)
{
  return pCtx->uMvID;
}

/*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx* pCtx)
{
  return pCtx->iCurFramePOC;
}

/***************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, AL_TDimension tDim)
{
  pCtx->uRecID = sFrmBufPoolFifo_Pop(&pCtx->FrmBufPool);

  if(pCtx->uRecID == UndefID)
    return false;

  pCtx->uMvID = sMvBufPool_GetFreeBufID(&pCtx->MvBufPool);
  assert(pCtx->uMvID != UndefID);

  AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uRecID);

  AL_TSrcMetaData* pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(tBuffers.pFrame, AL_META_TYPE_SOURCE);
  pMeta->tDim = tDim;


  return true;
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackDisplayIsDone(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayedFrame)
{
  Rtos_GetMutex(pPool->Mutex);
  int const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pDisplayedFrame);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackReleaseIsDone(AL_TFrmBufPool* pPool, AL_TBuffer* pReleasedFrame)
{
  Rtos_GetMutex(pPool->Mutex);
  int const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pReleasedFrame);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  assert(sFrmBufPoolFifo_IsInFifo(pPool, pReleasedFrame) == false);
  assert(pPool->array[iFrameID].bWillBeOutputed == true);
  pPool->array[iFrameID].bWillBeOutputed = false;
  sFrmBufPool_RemoveID(pPool, iFrameID);
  AL_Buffer_Unref(pReleasedFrame);
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
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, pCtx->uRecID);
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
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx* pCtx)
{
  return AL_Dpb_GetLastPicID(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, uint32_t uPocLsb, int iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT)
{
  uint8_t uNode = AL_Dpb_GetNextFreeNode(&pCtx->DPB);

  if(!AL_Dpb_NodeIsReset(&pCtx->DPB, uNode))
    AL_Dpb_Remove(&pCtx->DPB, uNode);

  AL_Dpb_Insert(&pCtx->DPB, iFramePOC, uPocLsb, uNode, iFrameID, uMvID, pic_output_flag, eMarkingFlag, uNonExisting, eNUT);
}

/***************************************************************************/
static void sFrmBufPool_UpdateCRC(AL_TFrmBufPool* pPool, int iFrameID, uint32_t uCRC)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].uCRC = uCRC;
}

/***************************************************************************/
static void sFrmBufPool_UpdateCrop(AL_TFrmBufPool* pPool, int iFrameID, AL_TCropInfo tCrop)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].tCrop = tCrop;
}

/***************************************************************************/
static void sFrmBufPool_UpdatePicStruct(AL_TFrmBufPool* pPool, int iFrameID, AL_EPicStruct ePicStruct)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].ePicStruct = ePicStruct;
}

/***************************************************************************/
static void sFrmBufPool_UpdateError(AL_TFrmBufPool* pPool, int iFrameID, AL_ERR error)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
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
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID);
  sMvBufPool_DecrementBufID(&pCtx->MvBufPool, iMotionVectorID);
}

/***************************************************************************/
static void sFrmBufPool_GetInfoDecode(AL_TFrmBufPool* pPool, int iFrameID, AL_TInfoDecode* pInfo, AL_EFbStorageMode eFbStorageMode, int iBitdepth, bool bDisplayInfo)
{
  assert(pInfo);

  AL_TRecBuffers tBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  assert(sRecBuffers_AreNotNull(&tBuffers, pPool));

  AL_TBuffer* pBuf = bDisplayInfo ? sRecBuffers_GetDisplayBuffer(&tBuffers, pPool) : tBuffers.pFrame;
  AL_TSrcMetaData* pMetaSrc = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
  assert(pMetaSrc);

  pInfo->tCrop = pPool->array[iFrameID].tCrop;
  pInfo->uBitDepthY = pInfo->uBitDepthC = iBitdepth;
  pInfo->uCRC = pPool->array[iFrameID].uCRC;
  pInfo->tDim = pMetaSrc->tDim;
  pInfo->eFbStorageMode = eFbStorageMode;
  pInfo->ePicStruct = pPool->array[iFrameID].ePicStruct;
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pMetaSrc->tFourCC);
  pInfo->bChroma = (eChromaMode != CHROMA_MONO);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo)
{
  if(!pCtx->bFirstInit)
    return NULL;

  int const iFrameID = AL_Dpb_GetDisplayBuffer(&pCtx->DPB);

  if(iFrameID == UndefID)
    return NULL;

  AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;


  if(pInfo)
    sFrmBufPool_GetInfoDecode(&pCtx->FrmBufPool, iFrameID, pInfo, eOutputStorageMode, pCtx->iBitdepth, true);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);

  return sRecBuffers_GetDisplayBuffer(&tRecBuffers, &pCtx->FrmBufPool);
}

/*************************************************************************/
static void sPictMngr_ReleaseDisplayBuffer(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);

  assert(pPool->array[iFrameID].bWillBeOutputed == true);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];

  pFrame->bWillBeOutputed = false;

  if(pFrame->iAccessCnt == 0)
  {
    AL_TRecBuffers pBuffers = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
    sFrmBufPool_RemoveID(pPool, iFrameID);
    assert(sFrmBufPoolFifo_IsInFifo(pPool, sRecBuffers_GetDisplayBuffer(&pBuffers, pPool)) == false);
    sFrmBufPoolFifo_PushBack(pPool, pBuffers);
  }

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, int iFrameID)
{
  assert(pCtx);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
  return sRecBuffers_GetDisplayBuffer(&tRecBuffers, &pCtx->FrmBufPool);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx* pCtx, int iFrameID)
{
  assert(pCtx);

  AL_TRecBuffers tRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
  return tRecBuffers.pFrame;
}

/*************************************************************************/
static AL_TBuffer* sFrmBufPool_GetRecBufferFromDisplayBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayBuf, int* iFrameID)
{
  AL_TBuffer* pRecBuf = NULL;

  Rtos_GetMutex(pPool->Mutex);

  assert(pDisplayBuf);

  *iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pDisplayBuf);

  if(*iFrameID != -1)
    pRecBuf = sFrmBufPool_GetBufferFromID(pPool, *iFrameID).pFrame;

  Rtos_ReleaseMutex(pPool->Mutex);

  return pRecBuf;
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pDisplayBuf, AL_TInfoDecode* pInfo)
{
  if(pDisplayBuf == NULL)
    return NULL;

  int iFrameID;
  AL_TBuffer* pRecBuf = sFrmBufPool_GetRecBufferFromDisplayBuffer(&pCtx->FrmBufPool, pDisplayBuf, &iFrameID);

  if(pInfo != NULL && pRecBuf != NULL)
  {
    AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;
    sFrmBufPool_GetInfoDecode(&pCtx->FrmBufPool, iFrameID, pInfo, eOutputStorageMode, pCtx->iBitdepth, false);
  }

  return pRecBuf;
}

/*************************************************************************/
bool AL_PictMngr_GetFrameEncodingError(AL_TPictMngrCtx* pCtx, AL_TBuffer* pDisplayBuf, AL_ERR* pError)
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
static AL_TRecBuffers sFrmBufPool_BuildRecBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pDisplayBuf)
{
  (void)pPool;

  AL_TRecBuffers tRecBuffers;
  tRecBuffers.pFrame = pDisplayBuf;


  return tRecBuffers;
}

/*************************************************************************/
static void sFrmBufPool_PutDisplayBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pPool->Mutex);

  assert(pBuf);

  int const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pPool, pBuf);

  if(iFrameID == -1)
  {
    AL_TRecBuffers tRecBuffers = sFrmBufPool_BuildRecBuffer(pPool, pBuf);
    AL_Buffer_Ref(pBuf);
    sFrmBufPoolFifo_PushBack(pPool, tRecBuffers);
  }
  else
    sPictMngr_ReleaseDisplayBuffer(pPool, iFrameID);

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
void AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  sFrmBufPool_PutDisplayBuffer(&pCtx->FrmBufPool, pBuf);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pDisplayedFrame)
{
  sFrmBufPool_SignalCallbackDisplayIsDone(&pCtx->FrmBufPool, pDisplayedFrame);
  AL_Dpb_ReleaseDisplayBuffer(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame)
{
  sFrmBufPool_SignalCallbackReleaseIsDone(&pCtx->FrmBufPool, pReleasedFrame);
}

void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bFirstInit)
    return;

  sFrmBufPoolFifo_Decommit(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static AL_TBuffer* sFrmBufPoolFifo_FlushOneDisplayBuffer(AL_TFrmBufPool* pPool)
{
  Rtos_GetMutex(pPool->Mutex);

  if((pPool->iFifoHead == -1) && (pPool->iFifoTail == -1))
  {
    Rtos_ReleaseMutex(pPool->Mutex);
    return NULL;
  }

  assert(pPool->iFifoHead != -1);
  assert(pPool->array[pPool->iFifoHead].iAccessCnt == 0);
  assert(pPool->array[pPool->iFifoHead].bWillBeOutputed == false);

  int const iFrameID = RemoveBufferFromFifo(pPool);
  pPool->array[iFrameID].iAccessCnt = 0;
  pPool->array[iFrameID].bWillBeOutputed = true;
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

static void FillPocAndLongtermLists(AL_TDpb* pDpb, TBufferPOC* pPoc, AL_TDecSliceParam* pSP, TBufferListRef* pListRef)
{
  int32_t* pPocList = (int32_t*)(pPoc->tMD.pVirtualAddr);
  uint32_t* pLongTermList = (uint32_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);

  if(!pSP->FirstLcuSliceSegment)
  {
    *pLongTermList = 0;

    for(int i = 0; i < MAX_REF; ++i)
      pPocList[i] = 0xFFFFFFFF;
  }

  AL_ESliceType eType = (AL_ESliceType)pSP->eSliceType;

  if(eType != SLICE_I)
    AL_Dpb_FillList(pDpb, 0, pListRef, pPocList, pLongTermList);

  if(eType == SLICE_B)
    AL_Dpb_FillList(pDpb, 1, pListRef, pPocList, pLongTermList);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, TBufferListRef* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, AL_TRecBuffers* pRecs)
{
  (void)pListVirtAddr; // only used for traces

  if(pCtx->uRecID == UndefID)
    return false;

  AL_TRecBuffers pRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uRecID);
  pRecs->pFrame = pRecBuffers.pFrame;

  AL_TSrcMetaData* pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pRecs->pFrame, AL_META_TYPE_SOURCE);
  assert(pRecMeta);
  int iPitch = pRecMeta->tPitches.iLuma;


  if(pMV && pPOC)
  {
    if(pCtx->uMvID == UndefID)
      return false;

    pMV->tMD = pCtx->MvBufPool.pMvBufs[pCtx->uMvID].tMD;
    pPOC->tMD = pCtx->MvBufPool.pPocBufs[pCtx->uMvID].tMD;

    if(pPOC)
      FillPocAndLongtermLists(&pCtx->DPB, pPOC, pSP, pListRef);
  }

  if(pListAddr && pListAddr->tMD.pVirtualAddr)
  {
    uint32_t* pAddr = (uint32_t*)pListAddr->tMD.pVirtualAddr;
    uint32_t* pColocMvList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + MVCOL_LIST_OFFSET);
    uint32_t* pColocPocList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + POCOL_LIST_OFFSET);
    uint32_t* pFbcList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + FBC_LIST_OFFSET);

    AL_TDimension tDim = { pPP->PicWidth * 8, pPP->PicHeight * 8 };
    int iLumaSize = AL_GetAllocSize_DecReference(tDim, iPitch, CHROMA_MONO, pCtx->eFbStorageMode);


    for(int i = 0; i < PIC_ID_POOL_SIZE; ++i)
    {
      bool bFindID = false;
      uint8_t uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, i);

      if(uNodeID == UndefID
         && pSP->eSliceType == SLICE_CONCEAL
         && pSP->ValidConceal
         && pCtx->DPB.uCountPic)
        uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, pSP->ConcealPicID);

      if(uNodeID != UndefID)
      {
        int iFrameID = AL_Dpb_GetFrmID_FromNode(&pCtx->DPB, uNodeID);
        uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->DPB, uNodeID);
        AL_TRecBuffers pBufs = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);

        pAddr[i] = AL_Allocator_GetPhysicalAddr(pBufs.pFrame->pAllocator, pBufs.pFrame->hBuf);
        pAddr[PIC_ID_POOL_SIZE + i] = pAddr[i] + iLumaSize;
        pColocMvList[i] = pCtx->MvBufPool.pMvBufs[uMvID].tMD.uPhysicalAddr;
        pColocPocList[i] = pCtx->MvBufPool.pPocBufs[uMvID].tMD.uPhysicalAddr;
        pFbcList[i] = 0;
        pFbcList[PIC_ID_POOL_SIZE + i] = 0;

        bFindID = true;
      }

      if(!bFindID)
      {
        // Use current Rec & MV Buffers to avoid non existing ref/coloc
        pAddr[i] = AL_Allocator_GetPhysicalAddr(pRecs->pFrame->pAllocator, pRecs->pFrame->hBuf);
        pAddr[PIC_ID_POOL_SIZE + i] = pAddr[i] + iLumaSize;
        pColocMvList[i] = pMV->tMD.uPhysicalAddr;
        pColocPocList[i] = pPOC->tMD.uPhysicalAddr;
        pFbcList[i] = 0;
        pFbcList[PIC_ID_POOL_SIZE + i] = 0;

      }
    }
  }
  return true;
}


/*@}*/

