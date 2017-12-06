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

#include <assert.h>

#include "lib_common/BufferSrcMeta.h"

#include "I_PictMngr.h"

/*************************************************************************/
static int sFrmBufPool_GetFrameID(AL_TFrmBufPool* pPool, AL_TBuffer* pFrameBuffer)
{
  Rtos_GetMutex(pPool->Mutex);

  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(pPool->array[i].pFrameBuffer == pFrameBuffer)
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return -1;
}

/*************************************************************************/
static bool sFrmBufPoolFifo_IsInFifo(AL_TFrmBufPool* pPool, AL_TBuffer* pFrameBuffer)
{
  Rtos_GetMutex(pPool->Mutex);

  for(int iCur = pPool->iFifoHead; iCur != -1; iCur = pPool->array[iCur].iNext)
  {
    if(pPool->array[iCur].pFrameBuffer == pFrameBuffer)
    {
      Rtos_ReleaseMutex(pPool->Mutex);
      return true;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  return false;
}

/*************************************************************************/
static void sFrmBufPoolFifo_PushBack(AL_TFrmBufPool* pPool, AL_TBuffer* pFrameBuffer)
{
  assert(pFrameBuffer);
  Rtos_GetMutex(pPool->Mutex);
  AL_CleanupMemory(AL_Buffer_GetData(pFrameBuffer), pFrameBuffer->zSize);

  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if((pPool->array[i].pFrameBuffer == NULL) &&
       (pPool->array[i].iNext == -1) &&
       (pPool->array[i].iAccessCnt == -1) &&
       (pPool->array[i].bWillBeOutputed == false)
       )
    {
      pPool->array[i].pFrameBuffer = pFrameBuffer;

      if(pPool->iFifoTail == -1 && pPool->iFifoHead == -1)
        pPool->iFifoHead = i;
      else
        pPool->array[pPool->iFifoTail].iNext = i;

      pPool->iFifoTail = i;

      pPool->array[i].iAccessCnt = 0;
      pPool->array[i].bWillBeOutputed = false;

      Rtos_ReleaseMutex(pPool->Mutex);
      Rtos_ReleaseSemaphore(pPool->Semaphore);
      return;
    }
  }

  Rtos_ReleaseMutex(pPool->Mutex);
  assert(0);
}

/*************************************************************************/
static int sFrmBufPoolFifo_Pop(AL_TFrmBufPool* pPool)
{
  Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);
  Rtos_GetMutex(pPool->Mutex);
  assert(pPool->iFifoHead != -1);
  assert(pPool->array[pPool->iFifoHead].pFrameBuffer != NULL);
  assert(pPool->array[pPool->iFifoHead].iAccessCnt == 0);
  assert(pPool->array[pPool->iFifoHead].bWillBeOutputed == false);
  int const iFrameID = pPool->iFifoHead;

  pPool->iFifoHead = pPool->array[pPool->iFifoHead].iNext;

  pPool->array[iFrameID].iNext = -1;
  pPool->array[iFrameID].iAccessCnt = 1;

  if(pPool->iFifoHead == -1)
    pPool->iFifoTail = pPool->iFifoHead;

  Rtos_ReleaseMutex(pPool->Mutex);
  return iFrameID;
}

/*************************************************************************/
static void sFrmBufPool_RemoveID(AL_TFrmBufPool* pPool, int iFrameID)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pPool->array[iFrameID];

  assert(pFrame->pFrameBuffer != NULL);
  assert(pFrame->iNext == -1);
  assert(pFrame->iAccessCnt == 0);

  pFrame->pFrameBuffer = NULL;
  pFrame->iAccessCnt = -1;
  pFrame->bWillBeOutputed = false;
  pFrame->iNext = -1;

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static AL_TBuffer* sFrmBufPool_GetBufferFromID(AL_TFrmBufPool* pPool, int iFrameID)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  assert(pPool->array[iFrameID].pFrameBuffer != NULL);
  return pPool->array[iFrameID].pFrameBuffer;
}

/*************************************************************************/
static void sFrmBufPoolFifo_Init(AL_TFrmBufPool* pPool)
{
  for(int i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    pPool->array[i].pFrameBuffer = NULL;
    pPool->array[i].iNext = -1;
    pPool->array[i].iAccessCnt = -1;
    pPool->array[i].bWillBeOutputed = false;
  }

  pPool->iFifoHead = -1;
  pPool->iFifoTail = -1;
}

/*************************************************************************/
static bool sFrmBufPool_Init(AL_TFrmBufPool* pPool)
{
  pPool->Mutex = Rtos_CreateMutex();

  if(!pPool->Mutex)
    goto fail_alloc_mutex;

  pPool->Semaphore = Rtos_CreateSemaphore(0);

  if(!pPool->Semaphore)
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
  Rtos_DeleteSemaphore(pPool->Semaphore);
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

  if(pFrame->iAccessCnt == 0 && !pFrame->bWillBeOutputed)
  {
    AL_TBuffer* pBuffer = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
    sFrmBufPool_RemoveID(pPool, iFrameID);
    assert(sFrmBufPoolFifo_IsInFifo(pPool, pBuffer) == false);
    sFrmBufPoolFifo_PushBack(pPool, pBuffer);
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
  sFrmBufPool_DecrementBufID(&pCtx->m_FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementFrmBuf(void* pUserParam, int iFrameID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_IncrementBufID(&pCtx->m_FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_OutputFrmBuf(void* pUserParam, int iFrameID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sFrmBufPool_OutputBufID(&pCtx->m_FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_IncrementBufID(&pCtx->m_MvBufPool, uMvID);
}

/*************************************************************************/
static void sPictMngr_DecrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;
  sMvBufPool_DecrementBufID(&pCtx->m_MvBufPool, uMvID);
}

/***************************************************************************/
/*         P i c t u r e    M a n a g e r     f u n c t i o n s            */
/***************************************************************************/

/*****************************************************************************/
static bool CheckPictMngrInitParameter(int iNumMV, int iSizeMV, int iNumDPBRef, AL_EDpbMode eDPBMode, AL_EFbStorageMode eFbStorageMode)
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

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, int iNumMV, int iSizeMV, int iNumDPBRef, AL_EDpbMode eDPBMode, AL_EFbStorageMode eFbStorageMode)
{
  if(!pCtx)
    return false;

  if(!CheckPictMngrInitParameter(iNumMV, iSizeMV, iNumDPBRef, eDPBMode, eFbStorageMode))
    return false;

  if(!sMvBufPool_Init(&pCtx->m_MvBufPool, iNumMV))
    return false;

  if(!sFrmBufPool_Init(&pCtx->m_FrmBufPool))
    return false;

  AL_TPictureManagerCallbacks tCallbacks =
  {
    sPictMngr_IncrementFrmBuf,
    sPictMngr_DecrementFrmBuf,
    sPictMngr_OutputFrmBuf,
    sPictMngr_IncrementMvBuf,
    sPictMngr_DecrementMvBuf,
    pCtx,
  };

  AL_Dpb_Init(&pCtx->m_DPB, iNumDPBRef, eDPBMode, tCallbacks);

  pCtx->m_eFbStorageMode = eFbStorageMode;
  pCtx->m_uSizeMV = iSizeMV;
  pCtx->m_uSizePOC = POCBUFF_PL_SIZE;
  pCtx->m_iCurFramePOC = 0;

  pCtx->m_uPrevPocLSB = 0;
  pCtx->m_iPrevPocMSB = 0;

  pCtx->m_uRecID = UndefID;
  pCtx->m_uMvID = UndefID;

  pCtx->m_uNumSlice = 0;
  pCtx->m_iPrevFrameNum = -1;
  pCtx->m_bFirstInit = true;
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
  if(!pCtx->m_bFirstInit)
    return;

  AL_Dpb_Terminate(&pCtx->m_DPB);

  sFrmBufPool_Terminate(&pCtx->m_FrmBufPool);
  sMvBufPool_Terminate(&pCtx->m_MvBufPool);
}

/*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->m_bFirstInit)
    return;

  sMvBufPool_Deinit(&pCtx->m_MvBufPool);
  sFrmBufPool_Deinit(&pCtx->m_FrmBufPool);
  AL_Dpb_Deinit(&pCtx->m_DPB);
}

/*****************************************************************************/
void AL_PictMngr_LockRefMvID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
    sPictMngr_IncrementMvBuf(pCtx, pRefMvID[uRef]);
}

/*****************************************************************************/
void AL_PictMngr_UnlockRefMvID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
    sPictMngr_DecrementMvBuf(pCtx, pRefMvID[uRef]);
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx* pCtx)
{
  return pCtx->m_uRecID;
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx* pCtx)
{
  return pCtx->m_uMvID;
}

/*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx* pCtx)
{
  return pCtx->m_iCurFramePOC;
}

/***************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, AL_TDimension tDim)
{
  pCtx->m_uRecID = sFrmBufPoolFifo_Pop(&pCtx->m_FrmBufPool);
  assert(pCtx->m_uRecID != UndefID);

  pCtx->m_uMvID = sMvBufPool_GetFreeBufID(&pCtx->m_MvBufPool);
  assert(pCtx->m_uMvID != UndefID);

  AL_TSrcMetaData* pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(sFrmBufPool_GetBufferFromID(&pCtx->m_FrmBufPool, pCtx->m_uRecID), AL_META_TYPE_SOURCE);
  pMeta->tDim = tDim;

  return true;
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackDisplayIsDone(AL_TFrmBufPool* pPool, AL_TBuffer* pBuffer)
{
  Rtos_GetMutex(pPool->Mutex);
  int const iFrameID = sFrmBufPool_GetFrameID(pPool, pBuffer);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackReleaseIsDone(AL_TFrmBufPool* pPool, AL_TBuffer* pBuffer)
{
  Rtos_GetMutex(pPool->Mutex);
  int const iFrameID = sFrmBufPool_GetFrameID(pPool, pBuffer);
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  assert(sFrmBufPoolFifo_IsInFifo(pPool, pBuffer) == false);
  assert(pPool->array[iFrameID].bWillBeOutputed == true);
  pPool->array[iFrameID].bWillBeOutputed = false;
  sFrmBufPool_RemoveID(pPool, iFrameID);
  AL_Buffer_Unref(pBuffer);

  Rtos_ReleaseMutex(pPool->Mutex);
}

/***************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->m_uMvID != UndefID)
  {
    sMvBufPool_DecrementBufID(&pCtx->m_MvBufPool, pCtx->m_uMvID);
    pCtx->m_uMvID = UndefID;
  }

  if(pCtx->m_uRecID != UndefID)
  {
    sFrmBufPool_DecrementBufID(&pCtx->m_FrmBufPool, pCtx->m_uRecID);
    pCtx->m_uRecID = UndefID;
  }
}

/*************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->m_bFirstInit)
    return;

  AL_Dpb_Flush(&pCtx->m_DPB);
  AL_Dpb_BeginNewSeq(&pCtx->m_DPB);
}

/*************************************************************************/
void AL_PictMngr_UpdateDPBInfo(AL_TPictMngrCtx* pCtx, uint8_t uMaxRef)
{
  AL_Dpb_SetNumRef(&pCtx->m_DPB, uMaxRef);
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx* pCtx)
{
  return AL_Dpb_GetLastPicID(&pCtx->m_DPB);
}

/*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, uint32_t uPocLsb, int iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT)
{
  uint8_t uNode = AL_Dpb_GetNextFreeNode(&pCtx->m_DPB);

  if(!AL_Dpb_NodeIsReset(&pCtx->m_DPB, uNode))
    AL_Dpb_Remove(&pCtx->m_DPB, uNode);

  AL_Dpb_Insert(&pCtx->m_DPB, iFramePOC, uPocLsb, uNode, iFrameID, uMvID, pic_output_flag, eMarkingFlag, uNonExisting, eNUT);
}

/***************************************************************************/
static void sFrmBufPool_UpdateCRC(AL_TFrmBufPool* pPool, int iFrameID, uint32_t uCRC)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].uCRC = uCRC;
}

/***************************************************************************/
static void sFrmBufPool_UpdateBitDepth(AL_TFrmBufPool* pPool, int iFrameID, AL_TBitDepth tBitDepth)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].tBitDepth = tBitDepth;
}

/***************************************************************************/
void sFrmBufPool_UpdateCrop(AL_TFrmBufPool* pPool, int iFrameID, AL_TCropInfo tCrop)
{
  assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pPool->array[iFrameID].tCrop = tCrop;
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferBitDepth(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TBitDepth tBitDepth)
{
  sFrmBufPool_UpdateBitDepth(&pCtx->m_FrmBufPool, iFrameID, tBitDepth);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, int iFrameID, uint32_t uCRC)
{
  sFrmBufPool_UpdateCRC(&pCtx->m_FrmBufPool, iFrameID, uCRC);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TCropInfo tCrop)
{
  sFrmBufPool_UpdateCrop(&pCtx->m_FrmBufPool, iFrameID, tCrop);
}

/***************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, int iFrameID, uint8_t uMvID)
{
  sFrmBufPool_DecrementBufID(&pCtx->m_FrmBufPool, iFrameID);
  sMvBufPool_DecrementBufID(&pCtx->m_MvBufPool, uMvID);
  AL_Dpb_EndDecoding(&pCtx->m_DPB, iFrameID);
}

/***************************************************************************/
static void sFrmBufPool_GetInfoDecode(AL_TFrmBufPool* pPool, int iFrameID, AL_TInfoDecode* pInfo, AL_EFbStorageMode eFbStorageMode)
{
  assert(pInfo);

  AL_TBuffer* frame = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  assert(frame);

  AL_TSrcMetaData* pMetaSrc = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(frame, AL_META_TYPE_SOURCE);
  assert(pMetaSrc);

  pInfo->tCrop = pPool->array[iFrameID].tCrop;
  pInfo->uBitDepthY = pPool->array[iFrameID].tBitDepth.iLuma;
  pInfo->uBitDepthC = pPool->array[iFrameID].tBitDepth.iChroma;
  pInfo->uCRC = pPool->array[iFrameID].uCRC;
  pInfo->tDim = pMetaSrc->tDim;
  pInfo->eFbStorageMode = eFbStorageMode;
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pMetaSrc->tFourCC);
  pInfo->bChroma = (eChromaMode != CHROMA_MONO);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo)
{
  if(!pCtx->m_bFirstInit)
    return NULL;

  int const iFrameID = AL_Dpb_GetDisplayBuffer(&pCtx->m_DPB);

  if(iFrameID == UndefID)
    return NULL;

  if(pInfo)
    sFrmBufPool_GetInfoDecode(&pCtx->m_FrmBufPool, iFrameID, pInfo, pCtx->m_eFbStorageMode);

  return sFrmBufPool_GetBufferFromID(&pCtx->m_FrmBufPool, iFrameID);
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
    AL_TBuffer* pBuffer = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
    sFrmBufPool_RemoveID(pPool, iFrameID);
    assert(sFrmBufPoolFifo_IsInFifo(pPool, pBuffer) == false);
    sFrmBufPoolFifo_PushBack(pPool, pBuffer);
  }

  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, int iFrameID)
{
  assert(pCtx);
  return sFrmBufPool_GetBufferFromID(&pCtx->m_FrmBufPool, iFrameID);
}

/*************************************************************************/
void sFrmBufPool_PutDisplayBuffer(AL_TFrmBufPool* pPool, AL_TBuffer* pBuffer)
{
  Rtos_GetMutex(pPool->Mutex);

  assert(pBuffer);

  int const iFrameID = sFrmBufPool_GetFrameID(pPool, pBuffer);

  if(iFrameID == -1)
  {
    AL_Buffer_Ref(pBuffer);
    sFrmBufPoolFifo_PushBack(pPool, pBuffer);
    Rtos_ReleaseMutex(pPool->Mutex);
    return;
  }

  sPictMngr_ReleaseDisplayBuffer(pPool, iFrameID);
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
void AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  sFrmBufPool_PutDisplayBuffer(&pCtx->m_FrmBufPool, pBuf);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuffer)
{
  sFrmBufPool_SignalCallbackDisplayIsDone(&pCtx->m_FrmBufPool, pBuffer);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuffer)
{
  sFrmBufPool_SignalCallbackReleaseIsDone(&pCtx->m_FrmBufPool, pBuffer);
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

  Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);
  assert(pPool->iFifoHead != -1);
  assert(pPool->array[pPool->iFifoHead].iAccessCnt == 0);
  assert(pPool->array[pPool->iFifoHead].bWillBeOutputed == false);
  int const iFrameID = pPool->iFifoHead;

  pPool->iFifoHead = pPool->array[pPool->iFifoHead].iNext;

  pPool->array[iFrameID].iNext = -1;

  pPool->array[iFrameID].bWillBeOutputed = true;

  if(pPool->iFifoHead == -1)
    pPool->iFifoTail = pPool->iFifoHead;

  AL_TBuffer* pBuffer = sFrmBufPool_GetBufferFromID(pPool, iFrameID);
  Rtos_ReleaseMutex(pPool->Mutex);
  return pBuffer;
}

/*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->m_bFirstInit)
    return NULL;

  return sFrmBufPoolFifo_FlushOneDisplayBuffer(&pCtx->m_FrmBufPool);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, TBufferListRef* pListRef, TBuffer* pListAddr, TBufferPOC** ppPOC, TBufferMV** ppMV, AL_TBuffer** ppRec, AL_EFbStorageMode eFBStorageMode)
{
  // Rec buffer
  if(ppRec)
  {
    if(pCtx->m_uRecID == UndefID)
      return false;

    *ppRec = sFrmBufPool_GetBufferFromID(&pCtx->m_FrmBufPool, pCtx->m_uRecID);
  }

  // MV buffer
  if(ppMV && ppPOC)
  {
    if(pCtx->m_uMvID == UndefID)
      return false;

    // Re-map memory descriptor
    pCtx->m_MV.tMD.pVirtualAddr = pCtx->m_MvBufPool.pMvBufs[pCtx->m_uMvID].tMD.pVirtualAddr;
    pCtx->m_MV.tMD.uPhysicalAddr = pCtx->m_MvBufPool.pMvBufs[pCtx->m_uMvID].tMD.uPhysicalAddr;
    pCtx->m_MV.tMD.uSize = pCtx->m_uSizeMV;
    *ppMV = &pCtx->m_MV;

    pCtx->m_POC.tMD.pVirtualAddr = pCtx->m_MvBufPool.pPocBufs[pCtx->m_uMvID].tMD.pVirtualAddr;
    pCtx->m_POC.tMD.uPhysicalAddr = pCtx->m_MvBufPool.pPocBufs[pCtx->m_uMvID].tMD.uPhysicalAddr;
    pCtx->m_POC.tMD.uSize = pCtx->m_uSizePOC;
    *ppPOC = &pCtx->m_POC;

    if(*ppPOC)
    {
      int32_t* pPocList = (int32_t*)((*ppPOC)->tMD.pVirtualAddr);
      uint32_t* pLongTermList = (uint32_t*)((*ppPOC)->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);

      if(!pSP->FirstLcuSliceSegment)
      {
        *pLongTermList = 0;

        for(int i = 0; i < MAX_REF; ++i)
          pPocList[i] = 0xFFFFFFFF;
      }

      AL_ESliceType eType = (AL_ESliceType)pSP->eSliceType;

      if(eType != SLICE_I)
        AL_Dpb_FillList(&pCtx->m_DPB, 0, pListRef, pPocList, pLongTermList);

      if(eType == SLICE_B)
        AL_Dpb_FillList(&pCtx->m_DPB, 1, pListRef, pPocList, pLongTermList);
    }
  }

  if(pListAddr && pListAddr->tMD.pVirtualAddr)
  {
    uint32_t* pAddr = (uint32_t*)pListAddr->tMD.pVirtualAddr;
    uint32_t* pColocMvList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + MVCOL_LIST_OFFSET);
    uint32_t* pColocPocList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + POCOL_LIST_OFFSET);
    uint32_t* pFbcList = (uint32_t*)(pListAddr->tMD.pVirtualAddr + FBC_LIST_OFFSET);

    AL_TDimension tDim = { pPP->PicWidth * 8, pPP->PicHeight * 8 };
    int iLumaSize = AL_GetAllocSize_DecReference(tDim, CHROMA_MONO, pPP->MaxBitDepth, eFBStorageMode);

    for(int i = 0; i < PIC_ID_POOL_SIZE; ++i)
    {
      bool bFindID = false;
      uint8_t uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->m_DPB, i);

      if(uNodeID == UndefID
         && pSP->eSliceType == SLICE_CONCEAL
         && pSP->ValidConceal
         && pCtx->m_DPB.m_uCountPic)
        uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->m_DPB, pSP->ConcealPicID);

      if(uNodeID != UndefID)
      {
        int iFrameID = AL_Dpb_GetFrmID_FromNode(&pCtx->m_DPB, uNodeID);
        uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->m_DPB, uNodeID);
        AL_TBuffer* pFrm = sFrmBufPool_GetBufferFromID(&pCtx->m_FrmBufPool, iFrameID);

        pAddr[i] = AL_Allocator_GetPhysicalAddr(pFrm->pAllocator, pFrm->hBuf);
        pAddr[PIC_ID_POOL_SIZE + i] = pAddr[i] + iLumaSize;
        pColocMvList[i] = pCtx->m_MvBufPool.pMvBufs[uMvID].tMD.uPhysicalAddr;
        pColocPocList[i] = pCtx->m_MvBufPool.pPocBufs[uMvID].tMD.uPhysicalAddr;
        pFbcList[i] = 0;
        pFbcList[PIC_ID_POOL_SIZE + i] = 0;

        bFindID = true;
      }

      if(!bFindID)
      {
        pAddr[i] = 0;
        pAddr[PIC_ID_POOL_SIZE + i] = 0;
        pColocMvList[i] = 0;
        pColocPocList[i] = 0;
        pFbcList[i] = 0;
        pFbcList[PIC_ID_POOL_SIZE + i] = 0;

      }
    }
  }
  return true;
}

/*@}*/

