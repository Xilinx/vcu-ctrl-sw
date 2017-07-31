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

#include <string.h>
#include "assert.h"
#include "stdio.h"
#include "lib_common/Error.h"

#include "lib_parsing/I_PictMngr.h"

#include "DefaultDecoder.h"
#include "lib_decode/I_DecChannel.h"
#include "I_DecoderCtx.h"
#include "NalUnitParser.h"


#define AVC_NAL_HDR_SIZE 2
#define HEVC_NAL_HDR_SIZE 3

/*****************************************************************************/
static AL_TDecCtx* AL_sGetContext(AL_TDefaultDecoder* pDec)
{
  return &(pDec->ctx);
}

/*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, TMemDesc* pMD, uint32_t uSize)
{
  if(!MemDesc_Alloc(pMD, pCtx->m_pAllocator, uSize))
  {
    pCtx->m_error = ERR_NO_MORE_MEMORY;
    return false;
  }

  return true;
}

/*****************************************************************************/
static void AL_Decoder_Free(TMemDesc* pMD)
{
  MemDesc_Free(pMD);
}

/*****************************************************************************/
static void AL_sDecoder_CallDecode(AL_TDecCtx const* pCtx, uint8_t const uFrameID)
{
  AL_TBuffer* pDecodedFrame = pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[uFrameID];
  assert(pDecodedFrame);

  pCtx->m_decodeCB.func(pDecodedFrame, pCtx->m_decodeCB.userParam);
}

/*****************************************************************************/
static void AL_sDecoder_CallDisplay(AL_TDecCtx* pCtx)
{
  while(1)
  {
    AL_TInfoDecode tInfo = { 0 };
    AL_TBuffer* pDisplayedFrame = AL_PictMngr_GetDisplayBuffer(&pCtx->m_PictMngr, &tInfo);

    if(!pDisplayedFrame)
      break;

    assert(AL_Buffer_GetBufferData(pDisplayedFrame));

    pCtx->m_displayCB.func(pDisplayedFrame, tInfo, pCtx->m_displayCB.userParam);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, uint8_t const uFrameID)
{
  if(pCtx->m_decodeCB.func)
    AL_sDecoder_CallDecode(pCtx, uFrameID);

  if(pCtx->m_displayCB.func)
    AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  uint8_t const uFrameID = pStatus->uFrmID;
  uint8_t const uMotionVectorID = pStatus->uMvID;

  AL_PictMngr_EndDecoding(&pCtx->m_PictMngr, uFrameID, uMotionVectorID, pStatus->uCRC);
  int iOffset = pCtx->m_iNumFrmBlk2 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefMvId(&pCtx->m_PictMngr, pCtx->m_uNumRef[iOffset], pCtx->m_uMvIDRefList[iOffset]);

  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_iCurOffset = pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk2 % pCtx->m_iStackSize];
  ++pCtx->m_iNumFrmBlk2;

  if(pStatus->bHanged)
    printf("Timeout - resetting the decoder\n");

  Rtos_ReleaseMutex(pCtx->m_DecMutex);

  AL_BufferFeeder_Signal(pCtx->m_Feeder);
  AL_sDecoder_CallBacks(pCtx, uFrameID);

  Rtos_ReleaseSemaphore(pCtx->m_Sem);
}

/*************************************************************************//*!
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current start code searching status
*****************************************************************************/
static void AL_Decoder_EndScd(void* pUserParam, AL_TScStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  pCtx->m_ScdStatus.uNumBytes = pStatus->uNumBytes;
  pCtx->m_ScdStatus.uNumSC = pStatus->uNumSC;

  Rtos_SetEvent(pCtx->m_ScDetectionComplete);
}

/***************************************************************************/
/*                           Lib functions                                 */
/***************************************************************************/

/*****************************************************************************/
static void InitBuffers(AL_TDecCtx* pCtx)
{
  MemDesc_Init(&pCtx->m_BufNoAE.tMD);
  MemDesc_Init(&pCtx->m_BufSCD.tMD);
  MemDesc_Init(&pCtx->m_SCTable.tMD);

  for(int i = 0; i < MAX_STACK_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->m_PoolSclLst[i].tMD);
    MemDesc_Init(&pCtx->m_PoolCompData[i].tMD);
    MemDesc_Init(&pCtx->m_PoolCompMap[i].tMD);
    MemDesc_Init(&pCtx->m_PoolSP[i].tMD);
    MemDesc_Init(&pCtx->m_PoolWP[i].tMD);
    MemDesc_Init(&pCtx->m_PoolListRefAddr[i].tMD);
  }

  for(int i = 0; i < MAX_DPB_SIZE; ++i)
  {
    pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[i] = NULL;
    MemDesc_Init(&pCtx->m_PictMngr.m_MvBufPool.pMvBufs[i].tMD);
    MemDesc_Init(&pCtx->m_PictMngr.m_MvBufPool.pPocBufs[i].tMD);
  }
}

static void InitAUP(AL_TDecCtx* pCtx)
{
  if(pCtx->m_chanParam.eCodec == AL_CODEC_AVC)
    AL_AVC_InitAUP(&pCtx->m_AvcAup);
  else
    AL_HEVC_InitAUP(&pCtx->m_HevcAup);
}

void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  assert(pCtx);

  AL_BufferFeeder_Destroy(pCtx->m_Feeder);

  if(pCtx->m_eosBuffer)
    AL_Buffer_Unref(pCtx->m_eosBuffer);

  AL_PictMngr_Terminate(&pCtx->m_PictMngr);
  AL_PictMngr_Deinit(&pCtx->m_PictMngr);

  for(uint8_t u = 0; u < MAX_DPB_SIZE; ++u)
  {
    if(pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[u])
    {
      AL_Buffer_Unref(pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[u]);
      pCtx->m_PictMngr.m_FrmBufPool.pFrmBufs[u] = NULL;
    }

    AL_Decoder_Free(&pCtx->m_PictMngr.m_MvBufPool.pMvBufs[u].tMD);
    AL_Decoder_Free(&pCtx->m_PictMngr.m_MvBufPool.pPocBufs[u].tMD);
  }

  AL_IDecChannel_Destroy(pCtx->m_pDecChannel);

  AL_Decoder_Free(&pCtx->m_BufSCD.tMD);
  AL_Decoder_Free(&pCtx->m_SCTable.tMD);

  for(uint8_t u = 0; u < MAX_STACK_SIZE; ++u)
  {
    AL_Decoder_Free(&pCtx->m_PoolCompData[u].tMD);
    AL_Decoder_Free(&pCtx->m_PoolCompMap[u].tMD);
    AL_Decoder_Free(&pCtx->m_PoolSP[u].tMD);
    AL_Decoder_Free(&pCtx->m_PoolWP[u].tMD);
    AL_Decoder_Free(&pCtx->m_PoolListRefAddr[u].tMD);
    AL_Decoder_Free(&pCtx->m_PoolSclLst[u].tMD);
  }

  Rtos_Free(pCtx->m_BufNoAE.tMD.pVirtualAddr);

  Rtos_DeleteSemaphore(pCtx->m_Sem);
  Rtos_DeleteEvent(pCtx->m_ScDetectionComplete);
  Rtos_DeleteMutex(pCtx->m_DecMutex);

  Rtos_Free(pDec);
}

/*****************************************************************************/

void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  pCtx->m_bConceal = bConceal;
  pCtx->m_bUseBoard = bUseBoard;

  pCtx->m_iTraceFirstFrame = iFrmID;
  pCtx->m_iTraceLastFrame = iFrmID + iNumFrm;

  if(iNumFrm > 0)
    AL_CLEAN_BUFFERS = 1;
}

/*****************************************************************************/
static bool SearchNextDecodUnit(AL_TDecCtx* pCtx, TCircBuffer* pStream, int* pLastNalInAU, int* iLastVclNalInAU)
{
  int iNalCount = (int)pCtx->m_uNumSC - 1;
  int iNalHdrSize = pCtx->m_chanParam.eCodec == AL_CODEC_AVC ? AVC_NAL_HDR_SIZE : HEVC_NAL_HDR_SIZE;
  int iLastVclNal = -1;
  int iLastNonVclNal = -1;

  AL_TScTable* pTable = (AL_TScTable*)(pCtx->m_SCTable.tMD.pVirtualAddr);
  uint8_t* pBuf = pStream->tMD.pVirtualAddr;
  uint32_t uSize = pStream->tMD.uSize;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    uint32_t uPos = pTable[iNal].uPosition;
    uint8_t uNUT = pTable[iNal].uNUT;

    bool bIsVcl = pCtx->m_chanParam.eCodec == AL_CODEC_AVC ? AL_AVC_IsVcl((AL_ENut)uNUT) : AL_HEVC_IsVcl((AL_ENut)uNUT);

    if(bIsVcl)
    {
      bool IsFirstSlice = false;

      /* skip all 0x00 */
      while(pBuf[uPos % uSize] != 0x01)
        ++uPos;

      uPos = (uPos + iNalHdrSize) % uSize; /* skip 0x01 + nal header*/

      if(pBuf[uPos] & 0x80) /* first slice segment flag */
        IsFirstSlice = true;

      bool bFind = false;
      switch(pCtx->m_eDecUnit)
      {
      case AL_AU_UNIT:
        bFind = (iLastVclNal >= 0 && IsFirstSlice);
        break;

      case AL_VCL_NAL_UNIT:
        bFind = iLastVclNal >= 0;
        break;

      default:
        break;
      }

      if(bFind)
      {
        *pLastNalInAU = Max(iLastNonVclNal - 1, iLastVclNal);
        *iLastVclNalInAU = IsFirstSlice ? iLastVclNal : -1;
        return true;
      }

      if(iLastVclNal < 0 || !IsFirstSlice)
        iLastVclNal = iNal;
      iLastNonVclNal = -1;
    }
    else if(iLastNonVclNal < 0 && (pCtx->m_chanParam.eCodec == AL_CODEC_AVC || (uNUT != AL_HEVC_NUT_SUFFIX_SEI)))
      iLastNonVclNal = iNal;
  }

  return false;
}

/*****************************************************************************/
static bool DecodeOneNAL(AL_TDecCtx* pCtx, AL_TScTable ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(*pNumSlice > 0 && *pNumSlice > pCtx->m_chanParam.iMaxSlices)
    return true;

  if(pCtx->m_chanParam.eCodec == AL_CODEC_AVC)
  {
    if(ScTable.uNUT >= AL_AVC_NUT_ERR)
      return false;

    return AL_AVC_DecodeOneNAL(&pCtx->m_AvcAup, pCtx, ScTable.uNUT, bIsLastVclNal, &pCtx->m_bFirstIsValid, &pCtx->m_bFirstSliceInFrameIsValid, pNumSlice);
  }
  else
  {
    if(ScTable.uNUT >= AL_HEVC_NUT_ERR)
      return false;

    return AL_HEVC_DecodeOneNAL(&pCtx->m_HevcAup, pCtx, ScTable.uNUT, bIsLastVclNal, &pCtx->m_bFirstIsValid, &pCtx->m_bFirstSliceInFrameIsValid, pNumSlice, &pCtx->m_bBeginFrameIsValid);
  }
}

static void GenerateIpTraces(AL_TDecCtx* pCtx, AL_TScParam ScP, AL_TScBufferAddrs ScdBuffer, TCircBuffer StreamSCD, TMemDesc scBuffer)
{
  (void)pCtx;
  (void)ScP;
  (void)ScdBuffer;
  (void)StreamSCD;
  (void)scBuffer;
}

static bool canStoreMoreStartCodes(AL_TDecCtx* pCtx)
{
  return (pCtx->m_uNumSC + 1) * sizeof(AL_TScTable) <= pCtx->m_SCTable.tMD.uSize - pCtx->m_BufSCD.tMD.uSize;
}

static bool RefillStartCodes(AL_TDecCtx* pCtx, TCircBuffer* pBufStream)
{
  AL_TScParam ScP =
  {
    0
  };
  AL_TScBufferAddrs ScdBuffer =
  {
    0
  };
  TMemDesc scBuffer = pCtx->m_BufSCD.tMD;

  ScP.MaxSize = scBuffer.uSize >> 3;
  ScP.AVC = pCtx->m_chanParam.eCodec == AL_CODEC_AVC;

  if(pBufStream->uAvailSize <= 4)
    return false;

  ScdBuffer.pBufOut = scBuffer.uPhysicalAddr;
  ScdBuffer.pStream = pBufStream->tMD.uPhysicalAddr;
  ScdBuffer.uMaxSize = pBufStream->tMD.uSize;
  ScdBuffer.uOffset = pBufStream->uOffset;
  ScdBuffer.uAvailSize = pBufStream->uAvailSize;

  AL_CleanupMemory(scBuffer.pVirtualAddr, scBuffer.uSize);

  AL_CB_EndStartCode callback = { &AL_Decoder_EndScd, pCtx };
  AL_IDecChannel_SearchSC(pCtx->m_pDecChannel, &ScP, &ScdBuffer, callback);
  Rtos_WaitEvent(pCtx->m_ScDetectionComplete, AL_WAIT_FOREVER);

  GenerateIpTraces(pCtx, ScP, ScdBuffer, *pBufStream, scBuffer);
  pBufStream->uOffset = (pBufStream->uOffset + pCtx->m_ScdStatus.uNumBytes) % pBufStream->tMD.uSize;
  pBufStream->uAvailSize -= pCtx->m_ScdStatus.uNumBytes;

  Rtos_Memcpy(pCtx->m_SCTable.tMD.pVirtualAddr + pCtx->m_uNumSC * sizeof(AL_TScTable), scBuffer.pVirtualAddr, pCtx->m_ScdStatus.uNumSC * sizeof(AL_TScTable));

  pCtx->m_uNumSC += pCtx->m_ScdStatus.uNumSC;

  return pCtx->m_ScdStatus.uNumSC > 0;
}

static int FindNextDecodingUnit(AL_TDecCtx* pCtx, TCircBuffer* pBufStream, int* iLastVclNalInAU)
{
  int iLastNalIdx = 0;

  while(!SearchNextDecodUnit(pCtx, pBufStream, &iLastNalIdx, iLastVclNalInAU))
  {
    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      pCtx->m_uNumSC = 0;
    }

    if(!RefillStartCodes(pCtx, pBufStream))
      return 0;
  }

  return iLastNalIdx + 1;
}

static size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  else
    return uSize + uSecondPos - uFirstPos;
}

static bool DecodeOneUnit(AL_TDecCtx* pCtx, TCircBuffer* pBufStream, int iNalCount, int iLastVclNalInAU)
{
  AL_TScTable* nals = (AL_TScTable*)pCtx->m_SCTable.tMD.pVirtualAddr;

  if(pCtx->m_uNumSC <= iNalCount)
    nals[iNalCount].uPosition = (nals[0].uPosition + pBufStream->uAvailSize) % pBufStream->tMD.uSize;

  /* copy start code buffer stream information into decoder stream buffer */
  pCtx->m_Stream.tMD = pBufStream->tMD;

  /* keep stream offset */
  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk1 % pCtx->m_iStackSize] = nals[iNalCount].uPosition;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);

  int iNumSlice = 0;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    bool bIsLastVclNal = (iNal == iLastVclNalInAU);

    AL_TScTable CurNal = nals[iNal];
    AL_TScTable NextNal = nals[iNal + 1];

    pCtx->m_Stream.uOffset = CurNal.uPosition;
    pCtx->m_Stream.uAvailSize = DeltaPosition(CurNal.uPosition, NextNal.uPosition, pBufStream->tMD.uSize);

    DecodeOneNAL(pCtx, CurNal, &iNumSlice, bIsLastVclNal);

    if(pCtx->m_eChanState == CHAN_INVALID)
      return false;

    if(bIsLastVclNal)
    {
      pCtx->m_bFirstSliceInFrameIsValid = false;
      pCtx->m_bBeginFrameIsValid = false;
    }
  }

  pCtx->m_uNumSC -= iNalCount;
  Rtos_Memmove(nals, nals + iNalCount, pCtx->m_uNumSC * sizeof(AL_TScTable));

  return true;
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_TryDecodeOneAU(AL_TDecoder* pAbsDec, TCircBuffer* pBufStream)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  int iLastVclNalInAU = -1;
  int iNalCount = FindNextDecodingUnit(pCtx, pBufStream, &iLastVclNalInAU);

  if(iNalCount == 0)
    return ERR_NO_FRAME_DECODED; /* no AU found */

  if(!DecodeOneUnit(pCtx, pBufStream, iNalCount, iLastVclNalInAU))
    return ERR_INIT_FAILED;

  return AL_SUCCESS;
}

bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize, AL_EBufMode eMode)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  return AL_BufferFeeder_PushBuffer(pCtx->m_Feeder, pBuf, eMode, uSize);
}

void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_BufferFeeder_Flush(pCtx->m_Feeder);
}

void AL_Default_Decoder_ForceStop(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_BufferFeeder_ForceStop(pCtx->m_Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->m_iStackSize; ++iSem)
    Rtos_GetSemaphore(pCtx->m_Sem, AL_WAIT_FOREVER);

  AL_PictMngr_Flush(&pCtx->m_PictMngr);

  // Send eos & get last frames in dpb if any
  if(pCtx->m_displayCB.func)
  {
    AL_TInfoDecode tmp = { 0 };
    AL_sDecoder_CallDisplay(pCtx);
    pCtx->m_displayCB.func(NULL, tmp, pCtx->m_displayCB.userParam);
  }
}

/*****************************************************************************/
AL_TBuffer* AL_Default_Decoder_GetDecPict(AL_TDecoder* pAbsDec, AL_TInfoDecode* pInfo)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return AL_PictMngr_GetDisplayBuffer(&pCtx->m_PictMngr, pInfo);
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  if(pDecPict)
  {
    AL_PictMngr_ReleaseDisplayBuffer(&pCtx->m_PictMngr, pDecPict);
    AL_BufferFeeder_Signal(pCtx->m_Feeder);
  }
}

void AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  if(pDecPict)
  {
    AL_PictMngr_PutDisplayBuffer(&pCtx->m_PictMngr, pDecPict);
  }
}

/*************************************************************************/
int AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return pCtx->m_uMaxBD;
}

/*************************************************************************/
static int Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int iCurOffset;
  Rtos_GetMutex(pCtx->m_DecMutex);
  iCurOffset = pCtx->m_iCurOffset;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
  return iCurOffset;
}

/*************************************************************************/
int AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  return Secure_GetStrOffset(pCtx);
}

/*************************************************************************/
AL_ERR AL_Default_Decoder_GetLastError(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_GetMutex(pCtx->m_DecMutex);
  AL_ERR ret = pCtx->m_error;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
  return ret;
}

const AL_TDecoderVtable AL_Default_Decoder_Vtable =
{
  &AL_Default_Decoder_Destroy,
  &AL_Default_Decoder_SetParam,
  &AL_Default_Decoder_PushBuffer,
  &AL_Default_Decoder_Flush,
  &AL_Default_Decoder_ForceStop,
  &AL_Default_Decoder_GetDecPict,
  &AL_Default_Decoder_ReleaseDecPict,
  &AL_Default_Decoder_PutDecPict,
  &AL_Default_Decoder_GetMaxBD,
  &AL_Default_Decoder_GetLastError,

  // only for the feeders
  &AL_Default_Decoder_TryDecodeOneAU,
  &AL_Default_Decoder_InternalFlush,
  &AL_Default_Decoder_GetStrOffset,
};

static AL_TBuffer* AllocEosBufferHEVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x28, 0, 0x80, 0, 0, 1, 0, 0
  }; // simulate a new frame
  return AL_Buffer_WrapData((uint8_t*)EOSNal, sizeof EOSNal, &AL_Buffer_Destroy);
}

static AL_TBuffer* AllocEosBufferAVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x01, 0x80, 0, 0, 1, 0, 0
  }; // simulate a new AU
  return AL_Buffer_WrapData((uint8_t*)EOSNal, sizeof EOSNal, &AL_Buffer_Destroy);
}

static void AssignSettings(AL_TDecCtx* const pCtx, AL_TDecSettings* const pSettings)
{
  pCtx->m_iStackSize = Min(pSettings->iStackSize, MAX_STACK_SIZE);
  pCtx->m_bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->m_eDecUnit = pSettings->eDecUnit;
  pCtx->m_eDpbMode = pSettings->eDpbMode;

  AL_TDecChanParam* pChan = &pCtx->m_chanParam;
  pChan->uMaxLatency = Min(pSettings->iStackSize, MAX_STACK_SIZE);
  pChan->uNumCore = pSettings->uNumCore;
  pChan->uClkRatio = pSettings->uClkRatio;
  pChan->uFrameRate = pSettings->uFrameRate;
  pChan->bDisableCache = pSettings->bDisableCache;
  pChan->bFrameBufferCompression = pSettings->bFrameBufferCompression;
  pChan->eFBStorageMode = pSettings->eFBStorageMode;
  pChan->uDDRWidth = pSettings->uDDRWidth == 16 ? 0 : pSettings->uDDRWidth == 32 ? 1 : 2;
  pChan->bParallelWPP = pSettings->bParallelWPP;
  pChan->bLowLat = pSettings->bLowLat;
  pChan->eCodec = pSettings->bIsAvc ? AL_CODEC_AVC : AL_CODEC_HEVC;
}

static void AssignCallBacks(AL_TDecCtx* const pCtx, AL_TDecCallBacks* const pCB)
{
  pCtx->m_decodeCB = pCB->endDecodingCB;
  pCtx->m_displayCB = pCB->displayCB;
  pCtx->m_resolutionFoundCB = pCB->resolutionFoundCB;
}

AL_TDecoder* AL_CreateDefaultDecoder(AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  AL_TDefaultDecoder* const pDec = (AL_TDefaultDecoder*)Rtos_Malloc(sizeof(AL_TDefaultDecoder));

  if(!pDec)
    return NULL;

  pDec->vtable = &AL_Default_Decoder_Vtable;
  AL_TDecCtx* const pCtx = &pDec->ctx;

  pCtx->m_pDecChannel = pDecChannel;
  pCtx->m_pAllocator = pAllocator;

  // initialize internal buffers
  InitBuffers(pCtx);

  // initialize decoder user parameters
  AssignSettings(pCtx, pSettings);
  AssignCallBacks(pCtx, pCB);

  pCtx->m_Sem = Rtos_CreateSemaphore(pCtx->m_iStackSize);
  pCtx->m_ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->m_DecMutex = Rtos_CreateMutex(false);

  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, false, false, 0, 0);

  // initialize decoder context
  pCtx->m_bFirstIsValid = false;
  pCtx->m_PictMngr.m_bFirstInit = false;
  pCtx->m_uNoRaslOutputFlag = 1;
  pCtx->m_bIsFirstPicture = true;
  pCtx->m_bLastIsEOS = false;
  pCtx->m_bFirstSliceInFrameIsValid = false;
  pCtx->m_bBeginFrameIsValid = false;

  AL_Conceal_Init(&pCtx->m_tConceal);

  // initialize video configuation
  Rtos_Memset(&pCtx->m_VideoConfiguration, 0, sizeof(pCtx->m_VideoConfiguration));

  // initialize decoder counters
  pCtx->m_uCurPocLsb = 0xFFFFFFFF;
  pCtx->m_uToggle = 0;
  pCtx->m_iNumFrmBlk1 = 0;
  pCtx->m_iNumFrmBlk2 = 0;
  pCtx->m_iCurOffset = 0;
  pCtx->m_iTraceCounter = 0;
  pCtx->m_eChanState = CHAN_UNINITIALIZED;

  // initialize tile information
  pCtx->m_uCurTileID = 0;
  pCtx->m_bTileSupToSlice = false;

  // initialize slice toggle information
  pCtx->m_uCurID = 0;
  pCtx->m_error = AL_SUCCESS;

  InitAUP(pCtx);

#define SAFE_ALLOC(pCtx, pMD, uSize) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize)) goto cleanup; \
  } while(0)

  // Alloc Start Code Detector buffer
  SAFE_ALLOC(pCtx, &pCtx->m_BufSCD.tMD, SCD_SIZE);
  AL_CleanupMemory(pCtx->m_BufSCD.tMD.pVirtualAddr, pCtx->m_BufSCD.tMD.uSize);

  SAFE_ALLOC(pCtx, &pCtx->m_SCTable.tMD, pCtx->m_iStackSize * MAX_NAL_UNIT * sizeof(AL_TScTable));
  AL_CleanupMemory(pCtx->m_SCTable.tMD.pVirtualAddr, pCtx->m_SCTable.tMD.uSize);

  pCtx->m_uNumSC = 0;

  // Alloc Decoder buffers
  for(int i = 0; i < pCtx->m_iStackSize; ++i)
    SAFE_ALLOC(pCtx, &pCtx->m_PoolListRefAddr[i].tMD, REF_LIST_SIZE);


  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    SAFE_ALLOC(pCtx, &pCtx->m_PoolSclLst[i].tMD, SCLST_SIZE_DEC);
    AL_CleanupMemory(pCtx->m_PoolSclLst[i].tMD.pVirtualAddr, pCtx->m_PoolSclLst[i].tMD.uSize);
  }

  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    Rtos_Memset(&pCtx->m_PoolPP[i], 0, sizeof(pCtx->m_PoolPP[0]));
    Rtos_Memset(&pCtx->m_PoolPB[i], 0, sizeof(pCtx->m_PoolPB[0]));
    AL_SET_DEC_OPT(&pCtx->m_PoolPP[i], IntraOnly, 1);
  }

  // Alloc Decoder Deanti-emulated buffer for high level syntax parsing
  pCtx->m_BufNoAE.tMD.pVirtualAddr = Rtos_Malloc(NON_VCL_NAL_SIZE);
  pCtx->m_BufNoAE.tMD.uSize = NON_VCL_NAL_SIZE;

  if(!pCtx->m_BufNoAE.tMD.pVirtualAddr)
    goto cleanup;

  pCtx->m_eosBuffer = pCtx->m_chanParam.eCodec == AL_CODEC_AVC ? AllocEosBufferAVC() : AllocEosBufferHEVC();

  if(!pCtx->m_eosBuffer)
    goto cleanup;

  AL_Buffer_Ref(pCtx->m_eosBuffer);

  const uint32_t bufferStreamSize = 64 * 1024 * 1024; /* 8k X 4k uncompressed picture (10 bits) */
  const uint32_t inputFifoSize = 256;
  pCtx->m_Feeder = AL_BufferFeeder_Create((AL_HDecoder)pDec, pAllocator, bufferStreamSize, inputFifoSize);

  if(!pCtx->m_Feeder)
    goto cleanup;

  pCtx->m_Feeder->eosBuffer = pCtx->m_eosBuffer;

  return (AL_TDecoder*)pDec;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pCtx);
  return NULL;
}

/*@}*/

