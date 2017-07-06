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
#include <stdio.h>

#include "lib_common/BufferSrcMeta.h"

#include "lib_common_dec/BufferDecodedPictureMeta.h"

#include "I_PictMngr.h"

/*************************************************************************/
static AL_TDecodedPictureMetaData* AL_sGetDecodedPictureMetaData(AL_TBuffer* pBuf)
{
  assert(pBuf);
  AL_TDecodedPictureMetaData* pMeta = (AL_TDecodedPictureMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DECODEDPICTURE);
  assert(pMeta);
  return pMeta;
}

/*************************************************************************/
static void AL_sFrmBufPool_Init(AL_TFrmBufPool* pPool, uint8_t iMaxBuf)
{
  int i;
  assert(iMaxBuf <= FRM_BUF_POOL_SIZE);

  for(i = 0; i < iMaxBuf; ++i)
  {
    pPool->pFreeIDs[i] = i;
    pPool->pFrmMeta[i] = AL_DecodedPictureMetaData_Create(i);
  }

  pPool->iBufCnt = 0;

  for(; i < FRM_BUF_POOL_SIZE; ++i)
  {
    pPool->pFreeIDs[i] = UndefID;
    pPool->pFrmMeta[i] = AL_DecodedPictureMetaData_Create(UndefID);
    pPool->pFrmBufs[i] = NULL;
  }

  pPool->iFreeCnt = 0;

  pPool->Mutex = Rtos_CreateMutex(false);
  pPool->SemaphoreFree = Rtos_CreateSemaphore(iMaxBuf);
  pPool->SemaphoreDPB = Rtos_CreateSemaphore(iMaxBuf);
}

/*************************************************************************/
static void AL_sFrmBufPool_Deinit(AL_TFrmBufPool* pPool)
{
  int i;

  for(i = 0; i < pPool->iBufCnt; ++i) // Wait until all buffer are freed
    Rtos_GetSemaphore(pPool->SemaphoreDPB, AL_WAIT_FOREVER);

  for(; i < FRM_BUF_POOL_SIZE; ++i)
    if(pPool->pFrmMeta[i])
      Rtos_Free(pPool->pFrmMeta[i]);

  Rtos_DeleteSemaphore(pPool->SemaphoreDPB);
  Rtos_DeleteSemaphore(pPool->SemaphoreFree);
  Rtos_DeleteMutex(pPool->Mutex);
}

/*************************************************************************/
static uint8_t AL_sFrmBufPool_GetFreeBufId(AL_TFrmBufPool* pPool)
{
  if(!Rtos_GetSemaphore(pPool->SemaphoreFree, AL_WAIT_FOREVER))
    return UndefID;

  Rtos_GetMutex(pPool->Mutex);
  uint8_t uFrmID = pPool->pFreeIDs[--pPool->iFreeCnt];
  assert(pPool->iFreeCnt >= 0);
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);
  pMeta->uAccessCntByDPB = 1;
  Rtos_ReleaseMutex(pPool->Mutex);

  return uFrmID;
}

/*************************************************************************/
static void AL_sFrmBufPool_FreeBufId(AL_TFrmBufPool* pPool, uint8_t uFrmID)
{
  assert(pPool);
  Rtos_GetMutex(pPool->Mutex);

  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);
  bool const bShouldBeReleasedFromDPB = (0 == pMeta->uAccessCntByDPB);
  bool const bShouldBeReleasedFromDisplay = (0 == pMeta->uAccessCntByDisplay);
  bool const bShouldBeFreedFromPool = (bShouldBeReleasedFromDisplay && bShouldBeReleasedFromDPB);

  if(bShouldBeFreedFromPool)
  {
    pPool->pFreeIDs[pPool->iFreeCnt++] = uFrmID;
    Rtos_ReleaseSemaphore(pPool->SemaphoreFree);
    pMeta->uPicLatency = 0;
  }
  Rtos_ReleaseMutex(pPool->Mutex);

  if(bShouldBeReleasedFromDPB)
    Rtos_ReleaseSemaphore(pPool->SemaphoreDPB);
}

/*************************************************************************/
static void AL_sFrmBufPool_ReleaseDisplayBufId(AL_TFrmBufPool* pPool, uint8_t uFrmID)
{
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);

  Rtos_GetMutex(pPool->Mutex);

  if(pMeta->uAccessCntByDisplay)
    --pMeta->uAccessCntByDisplay;
  Rtos_ReleaseMutex(pPool->Mutex);

  AL_sFrmBufPool_FreeBufId(pPool, uFrmID);
}

/*************************************************************************/
static void AL_sFrmBufPool_ReleaseDPBBufId(AL_TFrmBufPool* pPool, uint8_t uFrmID)
{
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);

  Rtos_GetMutex(pPool->Mutex);

  if(pMeta->uAccessCntByDPB)
    --pMeta->uAccessCntByDPB;
  Rtos_ReleaseMutex(pPool->Mutex);
  AL_sFrmBufPool_FreeBufId(pPool, uFrmID);
}

/*************************************************************************/
static uint8_t AL_sFrmBufPool_CopyDisplayBufId(AL_TFrmBufPool* pPool, uint8_t uFrmID)
{
  assert(uFrmID < FRM_BUF_POOL_SIZE);

  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);

  Rtos_GetMutex(pPool->Mutex);
  ++pMeta->uAccessCntByDisplay;
  Rtos_ReleaseMutex(pPool->Mutex);
  return uFrmID;
}

/*************************************************************************/
static uint8_t AL_sFrmBufPool_CopyDPBBufId(AL_TFrmBufPool* pPool, uint8_t uFrmID)
{
  assert(uFrmID < FRM_BUF_POOL_SIZE);

  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pPool->pFrmBufs[uFrmID]);

  Rtos_GetMutex(pPool->Mutex);
  ++pMeta->uAccessCntByDPB;
  Rtos_ReleaseMutex(pPool->Mutex);
  return uFrmID;
}

/*************************************************************************/
static void AL_sMvBufPool_Init(AL_TMvBufPool* pPool, int iMaxBuf)
{
  int i;

  assert(iMaxBuf <= MAX_DPB_SIZE);

  for(i = 0; i < iMaxBuf; ++i)
  {
    pPool->pFreeIDs[i] = i;
    pPool->uAccessCnt[i] = 0;
    AL_CleanupMemory(pPool->pMvBufs[i].tMD.pVirtualAddr, pPool->pMvBufs[i].tMD.uSize);
    AL_CleanupMemory(pPool->pPocBufs[i].tMD.pVirtualAddr, pPool->pPocBufs[i].tMD.uSize);
  }

  pPool->iBufCnt = iMaxBuf;

  for(; i < MAX_DPB_SIZE; ++i)
  {
    pPool->pFreeIDs[i] = UndefID;
    pPool->uAccessCnt[i] = 0;
  }

  pPool->iFreeCnt = iMaxBuf;

  pPool->Mutex = Rtos_CreateMutex(false);
  pPool->Semaphore = Rtos_CreateSemaphore(iMaxBuf);
}

/*************************************************************************/
static void AL_sMvBufPool_Deinit(AL_TMvBufPool* pPool)
{
  for(int i = 0; i < pPool->iBufCnt; ++i)
    Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_DeleteSemaphore(pPool->Semaphore);
  Rtos_DeleteMutex(pPool->Mutex);
}

/*************************************************************************/
static uint8_t AL_sMvBufPool_GetFreeBufId(AL_TMvBufPool* pPool)
{
  if(!Rtos_GetSemaphore(pPool->Semaphore, AL_WAIT_FOREVER))
    return UndefID;

  uint8_t uMvId;
  Rtos_GetMutex(pPool->Mutex);
  uMvId = pPool->pFreeIDs[--pPool->iFreeCnt];
  pPool->uAccessCnt[uMvId] = 1;
  Rtos_ReleaseMutex(pPool->Mutex);

  return uMvId;
}

/*************************************************************************/
static void AL_sMvBufPool_ReleaseBufId(AL_TMvBufPool* pPool, uint8_t uMvId)
{
  bool bFree = false;
  assert(uMvId < MAX_DPB_SIZE);

  Rtos_GetMutex(pPool->Mutex);

  if(pPool->uAccessCnt[uMvId])
  {
    bFree = (--pPool->uAccessCnt[uMvId] == 0);

    if(bFree)
      pPool->pFreeIDs[pPool->iFreeCnt++] = uMvId;
  }

  Rtos_ReleaseMutex(pPool->Mutex);

  if(bFree)
    Rtos_ReleaseSemaphore(pPool->Semaphore);
}

/*************************************************************************/
static void AL_sMvBufPool_CopyBufId(AL_TMvBufPool* pPool, uint8_t uMvId)
{
  Rtos_GetMutex(pPool->Mutex);
  assert(uMvId < MAX_DPB_SIZE);

  ++pPool->uAccessCnt[uMvId];
  Rtos_ReleaseMutex(pPool->Mutex);
}

/*************************************************************************/
static void AL_sPictMngr_ReleaseFrmBuf(AL_TPictMngrCtx* pCtx, uint8_t uFrmID)
{
  AL_sFrmBufPool_ReleaseDPBBufId(&pCtx->m_FrmBufPool, uFrmID);
}

/*************************************************************************/
static void AL_sPictMngr_IncrementFrmBuf(AL_TPictMngrCtx* pCtx, uint8_t uFrmID)
{
  AL_sFrmBufPool_CopyDPBBufId(&pCtx->m_FrmBufPool, uFrmID);
}

/*************************************************************************/
static void AL_sPictMngr_OutputFrmBuf(AL_TPictMngrCtx* pCtx, uint8_t uFrmID)
{
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pCtx->m_FrmBufPool.pFrmBufs[uFrmID]);
  AL_sFrmBufPool_CopyDisplayBufId(&pCtx->m_FrmBufPool, uFrmID);
  pMeta->uPicLatency = AL_Dpb_GetPicLatency_FromFifo(&pCtx->m_DPB, uFrmID);
}

/*************************************************************************/
static void AL_sPictMngr_IncrementMvBuf(AL_TPictMngrCtx* pCtx, uint8_t uMvID)
{
  AL_sMvBufPool_CopyBufId(&pCtx->m_MvBufPool, uMvID);
}

/*************************************************************************/
static void AL_sPictMngr_ReleaseMvBuf(AL_TPictMngrCtx* pCtx, uint8_t uMvID)
{
  AL_sMvBufPool_ReleaseBufId(&pCtx->m_MvBufPool, uMvID);
}

/***************************************************************************/
/*         P i c t u r e    M a n a g e r     f u n c t i o n s            */
/***************************************************************************/

/*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, bool bAvc, uint16_t uWidth, uint16_t uHeight, uint8_t uNumFrmBuf, uint8_t uNumMvBuf, uint8_t uNumRef, uint8_t uNumInterBuf)
{
  AL_sFrmBufPool_Init(&pCtx->m_FrmBufPool, uNumFrmBuf);
  AL_sMvBufPool_Init(&pCtx->m_MvBufPool, uNumMvBuf);
  AL_Dpb_Init(&pCtx->m_DPB, uNumRef, uNumInterBuf, pCtx,
              (PfnIncrementFrmBuf)AL_sPictMngr_IncrementFrmBuf,
              (PfnReleaseFrmBuf)AL_sPictMngr_ReleaseFrmBuf,
              (PfnOutputFrmBuf)AL_sPictMngr_OutputFrmBuf,
              (PfnIncrementMvBuf)AL_sPictMngr_IncrementMvBuf,
              (PfnReleaseMvBuf)AL_sPictMngr_ReleaseMvBuf);

  pCtx->m_uMaxFrmBuf = uNumFrmBuf;
  pCtx->m_uMaxMvBuf = uNumMvBuf;

  pCtx->m_uSizeMV = AL_GetAllocSize_MV(uWidth, uHeight, bAvc);
  pCtx->m_uSizePOC = POCBUFF_PL_SIZE;
  pCtx->m_iCurFramePOC = 0;

  pCtx->m_uPrevPocLSB = 0;
  pCtx->m_iPrevPocMSB = 0;

  pCtx->m_uRecID = UndefID;
  pCtx->m_uMvID = UndefID;

  pCtx->m_uNumSlice = 0;
  pCtx->m_iPrevFrameNum = -1;
  return true;
}

/*****************************************************************************/
uint32_t AL_PictMngr_GetReferenceSize(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode, uint8_t uBitDepth)
{
  uint32_t uPitch = RndPitch(uWidth, uBitDepth);
  uint32_t uSize = uPitch * RndHeight(uHeight);
  switch(eChromaMode)
  {
  case CHROMA_4_2_0:
    uSize += (uSize >> 1);
    break;

  case CHROMA_4_2_2:
    uSize += uSize;
    break;

  case CHROMA_4_4_4:
    uSize += (uSize << 1);
    break;

  default:
    break;
  }

  return uSize;
}

/*****************************************************************************/
uint32_t AL_PictMngr_GetLumaMapSize(uint16_t uWidth, uint16_t uHeight, AL_EFbStorageMode eFBStorageMode)
{
  if(eFBStorageMode == AL_FB_RASTER)
    return 0;
  assert(eFBStorageMode == AL_FB_TILE_32x4 || eFBStorageMode == AL_FB_TILE_64x4);
  uint32_t uTileWidth = eFBStorageMode == AL_FB_TILE_32x4 ? 32 : 64;
  uint32_t uTileHeight = 4;
  return (4096 / (2 * uTileWidth)) * ((uWidth + 4095) >> 12) * (uHeight / uTileHeight);
}

/*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->m_bFirstInit)
    AL_Dpb_Terminate(&pCtx->m_DPB);
}

/*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->m_bFirstInit)
  {
    AL_sMvBufPool_Deinit(&pCtx->m_MvBufPool);
    AL_sFrmBufPool_Deinit(&pCtx->m_FrmBufPool);
    AL_Dpb_Deinit(&pCtx->m_DPB);
  }
}

/*****************************************************************************/
void AL_PictMngr_LockRefMvId(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvId)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
    AL_sPictMngr_IncrementMvBuf(pCtx, pRefMvId[uRef]);
}

/*****************************************************************************/
void AL_PictMngr_UnlockRefMvId(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvId)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
    AL_sPictMngr_ReleaseMvBuf(pCtx, pRefMvId[uRef]);
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
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, int iWidth, int iHeight)
{
  AL_TSrcMetaData* pMeta;

  pCtx->m_uRecID = AL_sFrmBufPool_GetFreeBufId(&pCtx->m_FrmBufPool);

  if(pCtx->m_uRecID == UndefID)
    return false;

  pCtx->m_uMvID = AL_sMvBufPool_GetFreeBufId(&pCtx->m_MvBufPool);

  if(pCtx->m_uMvID == UndefID)
  {
    AL_sFrmBufPool_ReleaseDPBBufId(&pCtx->m_FrmBufPool, pCtx->m_uRecID);
    pCtx->m_uRecID = UndefID;
    return false;
  }

  pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pCtx->m_FrmBufPool.pFrmBufs[pCtx->m_uRecID], AL_META_TYPE_SOURCE);
  pMeta->iWidth = iWidth;
  pMeta->iHeight = iHeight;

  return true;
}

/***************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->m_uMvID != UndefID)
  {
    AL_sMvBufPool_ReleaseBufId(&pCtx->m_MvBufPool, pCtx->m_uMvID);
    pCtx->m_uMvID = UndefID;
  }

  if(pCtx->m_uRecID != UndefID)
  {
    AL_sFrmBufPool_ReleaseDPBBufId(&pCtx->m_FrmBufPool, pCtx->m_uRecID);
    pCtx->m_uRecID = UndefID;
  }
}

/*************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->m_bFirstInit)
  {
    AL_Dpb_Flush(&pCtx->m_DPB);
    AL_Dpb_BeginNewSeq(&pCtx->m_DPB);
  }
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
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, uint32_t uPocLsb, uint8_t uFrmID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT)
{
  uint8_t uNode = AL_Dpb_GetNextFreeNode(&pCtx->m_DPB);

  if(!AL_Dpb_NodeIsReset(&pCtx->m_DPB, uNode))
    AL_Dpb_Remove(&pCtx->m_DPB, uNode);

  if(uFrmID != uEndOfList)
  {
    AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pCtx->m_FrmBufPool.pFrmBufs[uFrmID]);
    pMeta->iFramePOC = iFramePOC;
  }
  AL_Dpb_Insert(&pCtx->m_DPB, iFramePOC, uPocLsb, uNode, uFrmID, uMvID, pic_output_flag, eMarkingFlag, uNonExisting, eNUT);
}

/***************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, uint8_t uFrmID, uint8_t uMvID, uint32_t uCRC)
{
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pCtx->m_FrmBufPool.pFrmBufs[uFrmID]);
  pMeta->uCrc = uCRC;
  AL_sFrmBufPool_ReleaseDPBBufId(&pCtx->m_FrmBufPool, uFrmID);
  AL_sMvBufPool_ReleaseBufId(&pCtx->m_MvBufPool, uMvID);
  AL_Dpb_EndDecoding(&pCtx->m_DPB, uFrmID);
}

static void getInfoDecode(AL_TInfoDecode* pInfo, AL_TBuffer* frame)
{
  AL_EChromaMode eChromaMode;

  assert(pInfo);
  assert(frame);

  AL_TDecodedPictureMetaData* pMetaDecoded = AL_sGetDecodedPictureMetaData(frame);
  AL_TSrcMetaData* pMetaSrc = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(frame, AL_META_TYPE_SOURCE);

  assert(pMetaDecoded);
  assert(pMetaSrc);

  pInfo->tCrop = pMetaDecoded->tCropInfo;
  pInfo->uBitDepthY = pMetaDecoded->uBitDepthY;
  pInfo->uBitDepthC = pMetaDecoded->uBitDepthC;
  pInfo->uCRC = pMetaDecoded->uCrc;
  pInfo->uWidth = pMetaSrc->iWidth;
  pInfo->uHeight = pMetaSrc->iHeight;
  eChromaMode = AL_GetChromaMode(pMetaSrc->tFourCC);
  pInfo->bChroma = (eChromaMode != CHROMA_MONO) ? true : false;
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo)
{
  if(!pCtx->m_bFirstInit)
    return NULL;

  uint8_t uFrmID = AL_Dpb_GetDisplayBuffer(&pCtx->m_DPB);

  if(uFrmID == UndefID)
    return NULL;

  if(pInfo)
    getInfoDecode(pInfo, pCtx->m_FrmBufPool.pFrmBufs[uFrmID]);

  return pCtx->m_FrmBufPool.pFrmBufs[uFrmID];
}

/*************************************************************************/
void AL_PictMngr_ReleaseDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  AL_TDecodedPictureMetaData* pMeta = AL_sGetDecodedPictureMetaData(pBuf);
  AL_sFrmBufPool_ReleaseDisplayBufId(&pCtx->m_FrmBufPool, pMeta->uFrmID);
}

static void RemovePreviousMetaData(AL_TBuffer* pBuf)
{
  AL_TMetaData* pMeta = AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DECODEDPICTURE);

  if(pMeta)
    AL_Buffer_RemoveMetaData(pBuf, pMeta);
}

static void ReplacePreviousMetaData(AL_TBuffer* pBuf, AL_TDecodedPictureMetaData* pMeta)
{
  RemovePreviousMetaData(pBuf);
  AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pMeta);
}

void AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  AL_Buffer_Ref(pBuf);
  ReplacePreviousMetaData(pBuf, pCtx->m_FrmBufPool.pFrmMeta[pCtx->m_FrmBufPool.iBufCnt]);
  pCtx->m_FrmBufPool.pFrmBufs[pCtx->m_FrmBufPool.iBufCnt] = pBuf;
  pCtx->m_FrmBufPool.iBufCnt++;
  pCtx->m_FrmBufPool.iFreeCnt++;

  AL_CleanupMemory(pBuf->pData, pBuf->zSize);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, TBufferListRef* pListRef, TBuffer* pListAddr, TBufferPOC** ppPOC, TBufferMV** ppMV, AL_TBuffer** ppRec, AL_EFbStorageMode eFBStorageMode)
{
  // fast access
  AL_ESliceType eType = (AL_ESliceType)pSP->eSliceType;

  // Rec buffer
  if(ppRec)
  {
    if(pCtx->m_uRecID == UndefID)
      return false;

    *ppRec = pCtx->m_FrmBufPool.pFrmBufs[pCtx->m_uRecID];
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
      int i;
      int32_t* pPocList = (int32_t*)((*ppPOC)->tMD.pVirtualAddr);
      uint32_t* pLongTermList = (uint32_t*)((*ppPOC)->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);

      if(!pSP->FirstLcuSliceSegment)
      {
        *pLongTermList = 0;

        for(i = 0; i < MAX_REF; ++i)
          pPocList[i] = 0xFFFFFFFF;
      }

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

    uint32_t uWidth = pPP->PicWidth << 3;
    uint32_t uHeight = pPP->PicHeight << 3;
    uint32_t uLumaSize = AL_PictMngr_GetReferenceSize(uWidth, uHeight, CHROMA_MONO, pPP->MaxBitDepth);
    uint32_t uBufferSize = AL_PictMngr_GetReferenceSize(uWidth, uHeight, pPP->ChromaMode, pPP->MaxBitDepth);
    uint32_t uYSizeMap = AL_PictMngr_GetLumaMapSize(uWidth, uHeight, eFBStorageMode);

    int i;

    for(i = 0; i < PIC_ID_POOL_SIZE; ++i)
    {
      bool bFindId = false;
      uint8_t uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->m_DPB, i);

      if(uNodeID == uEndOfList
         && pSP->eSliceType == SLICE_CONCEAL
         && pSP->ValidConceal
         && pCtx->m_DPB.m_uCountPic)
        uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->m_DPB, pSP->ConcealPicID);

      if(uNodeID != uEndOfList)
      {
        uint8_t uFrmID = AL_Dpb_GetFrmID_FromNode(&pCtx->m_DPB, uNodeID);
        uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->m_DPB, uNodeID);
        AL_TBuffer* pFrm = pCtx->m_FrmBufPool.pFrmBufs[uFrmID];

        pAddr[i] = AL_Allocator_GetPhysicalAddr(pFrm->pAllocator, pFrm->hBuf);
        pAddr[PIC_ID_POOL_SIZE + i] = pAddr[i] + uLumaSize;
        pColocMvList[i] = pCtx->m_MvBufPool.pMvBufs[uMvID].tMD.uPhysicalAddr;
        pColocPocList[i] = pCtx->m_MvBufPool.pPocBufs[uMvID].tMD.uPhysicalAddr;
        pFbcList[i] = pAddr[i] + uBufferSize;
        pFbcList[PIC_ID_POOL_SIZE + i] = pAddr[i] + uBufferSize + uYSizeMap;

        bFindId = true;
      }

      if(!bFindId)
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

