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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "DefaultDecoder.h"
#include "NalUnitParser.h"

#include "lib_common/Error.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Utils.h"
#include "lib_common/BufferSrcMeta.h"

#include "lib_common/AvcLevelsLimit.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_decode/I_DecChannel.h"


#define AVC_NAL_HDR_SIZE 4
#define HEVC_NAL_HDR_SIZE 5

/*****************************************************************************/
static bool isAVC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_AVC;
}

/******************************************************************************/
static bool IsAtLeastOneStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) || (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsAllStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) && (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsStreamChromaSet(AL_EChromaMode eChroma)
{
  return eChroma != CHROMA_MAX_ENUM;
}

/******************************************************************************/
static bool IsStreamBitDepthSet(int iBitDepth)
{
  return iBitDepth > 0;
}

/******************************************************************************/
static bool IsStreamLevelSet(int iLevel)
{
  return iLevel > 0;
}

/******************************************************************************/
static bool IsStreamProfileSet(int iProfileIdc)
{
  return iProfileIdc > 0;
}

static bool IsStreamSequenceModeSet(AL_ESequenceMode eSequenceMode)
{
  return eSequenceMode != AL_SM_MAX_ENUM;
}

/******************************************************************************/
static bool IsAllStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAllStreamDimSet(tStreamSettings.tDim) && IsStreamChromaSet(tStreamSettings.eChroma) && IsStreamBitDepthSet(tStreamSettings.iBitDepth) && IsStreamLevelSet(tStreamSettings.iLevel) && IsStreamProfileSet(tStreamSettings.iProfileIdc) && IsStreamSequenceModeSet(tStreamSettings.eSequenceMode);
}

static bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAtLeastOneStreamDimSet(tStreamSettings.tDim) || IsStreamChromaSet(tStreamSettings.eChroma) || IsStreamBitDepthSet(tStreamSettings.iBitDepth) || IsStreamLevelSet(tStreamSettings.iLevel) || IsStreamProfileSet(tStreamSettings.iProfileIdc) || IsStreamSequenceModeSet(tStreamSettings.eSequenceMode);
}

/* Buffer size must be aligned with hardware requests, which are 2048 or 4096 bytes for dec1 units (for old or new decoder respectively). */
static int const bitstreamRequestSize = 4096;

/*****************************************************************************/
static int GetCircularBufferSize(bool isAvc, int iStack, AL_TStreamSettings tStreamSettings)
{
  int const zMaxCPBSize = (isAvc ? 120 : 50) * 1024 * 1024; /* CPB default worst case */
  int const zWorstCaseNalSize = 2 << (isAvc ? 24 : 23); /* Single frame worst case */

  int circularBufferSize = 0;

  if(IsAllStreamSettingsSet(tStreamSettings))
  {
    /* Circular buffer always should be able to hold one frame, therefore compute the worst case and use it as a lower bound.  */
    int const zMaxNalSize = AL_GetMaxNalSize(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth); /* Worst case: (5/3)*PCM + Worst case slice Headers */
    int const zRealworstcaseNalSize = AL_GetMitigatedMaxNalSize(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth); /* Reasonnable: PCM + Slice Headers */
    circularBufferSize = UnsignedMax(zMaxNalSize, iStack * zRealworstcaseNalSize);
  }
  else
  {
    circularBufferSize = zWorstCaseNalSize * iStack;
  }

  /* Get minimum between absolute CPB worst case and computed value. */
  int const bufferSize = UnsignedMin(circularBufferSize, zMaxCPBSize);

  /* Round up for hardware. */
  return RoundUp(bufferSize, bitstreamRequestSize);
}

/*****************************************************************************/
static AL_TDecCtx* AL_sGetContext(AL_TDefaultDecoder* pDec)
{
  return &(pDec->ctx);
}

/*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, TMemDesc* pMD, uint32_t uSize, char const* name)
{
  if(!MemDesc_AllocNamed(pMD, pCtx->pAllocator, uSize, name))
  {
    pCtx->error = AL_ERR_NO_MEMORY;
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
static void AL_sDecoder_CallDecode(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
  assert(pDecodedFrame);

  pCtx->decodeCB.func(pDecodedFrame, pCtx->decodeCB.userParam);
}

/*****************************************************************************/
static void AL_sDecoder_CallDisplay(AL_TDecCtx* pCtx)
{
  while(1)
  {
    AL_TInfoDecode pInfo = { 0 };
    AL_TBuffer* pFrameToDisplay = AL_PictMngr_GetDisplayBuffer(&pCtx->PictMngr, &pInfo);

    if(!pFrameToDisplay)
      break;

    assert(AL_Buffer_GetData(pFrameToDisplay));

    pCtx->displayCB.func(pFrameToDisplay, &pInfo, pCtx->displayCB.userParam);
    AL_PictMngr_SignalCallbackDisplayIsDone(&pCtx->PictMngr, pFrameToDisplay);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_sDecoder_CallDecode(pCtx, iFrameID);

  AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  int iFrameID = pStatus->uFrmID;
  AL_PictMngr_UpdateDisplayBufferCRC(&pCtx->PictMngr, iFrameID, pStatus->uCRC);
  AL_PictMngr_EndDecoding(&pCtx->PictMngr, iFrameID);
  int iOffset = pCtx->iNumFrmBlk2 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->uFrameIDRefList[iOffset], pCtx->uMvIDRefList[iOffset]);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = pCtx->iStreamOffset[pCtx->iNumFrmBlk2 % pCtx->iStackSize];
  ++pCtx->iNumFrmBlk2;

  if(pStatus->bHanged)
    printf("***** /!\\ Timeout - resetting the decoder /!\\ *****\n");

  Rtos_ReleaseMutex(pCtx->DecMutex);

  AL_BufferFeeder_Signal(pCtx->Feeder);
  AL_sDecoder_CallBacks(pCtx, iFrameID);

  Rtos_GetMutex(pCtx->DecMutex);
  int iMotionVectorID = pStatus->uMvID;
  AL_PictMngr_UnlockID(&pCtx->PictMngr, iFrameID, iMotionVectorID);
  Rtos_ReleaseMutex(pCtx->DecMutex);

  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*************************************************************************//*!
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current start code searching status
*****************************************************************************/
static void AL_Decoder_EndScd(void* pUserParam, AL_TScStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  pCtx->ScdStatus.uNumBytes = pStatus->uNumBytes;
  pCtx->ScdStatus.uNumSC = pStatus->uNumSC;

  Rtos_SetEvent(pCtx->ScDetectionComplete);
}

/***************************************************************************/
/*                           Lib functions                                 */
/***************************************************************************/

/*****************************************************************************/
static void InitInternalBuffers(AL_TDecCtx* pCtx)
{
  MemDesc_Init(&pCtx->BufNoAE.tMD);
  MemDesc_Init(&pCtx->BufSCD.tMD);
  MemDesc_Init(&pCtx->SCTable.tMD);

  for(int i = 0; i < MAX_STACK_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PoolSclLst[i].tMD);
    MemDesc_Init(&pCtx->PoolCompData[i].tMD);
    MemDesc_Init(&pCtx->PoolCompMap[i].tMD);
    MemDesc_Init(&pCtx->PoolSP[i].tMD);
    MemDesc_Init(&pCtx->PoolWP[i].tMD);
    MemDesc_Init(&pCtx->PoolListRefAddr[i].tMD);
  }

  for(int i = 0; i < MAX_DPB_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
  }

}

/*****************************************************************************/
static void DeinitBuffers(AL_TDecCtx* pCtx)
{
  for(int i = 0; i < MAX_DPB_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
  }

  for(int i = 0; i < MAX_STACK_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PoolCompData[i].tMD);
    AL_Decoder_Free(&pCtx->PoolCompMap[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolWP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolListRefAddr[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSclLst[i].tMD);
  }

  AL_Decoder_Free(&pCtx->BufSCD.tMD);
  AL_Decoder_Free(&pCtx->SCTable.tMD);
  AL_Decoder_Free(&pCtx->BufNoAE.tMD);
}

/*****************************************************************************/
static void ReleaseFramePictureUnused(AL_TDecCtx* pCtx)
{
  while(true)
  {
    AL_TBuffer* pFrameToRelease = AL_PictMngr_GetUnusedDisplayBuffer(&pCtx->PictMngr);

    if(!pFrameToRelease)
      break;

    assert(AL_Buffer_GetData(pFrameToRelease));

    pCtx->displayCB.func(pFrameToRelease, NULL, pCtx->displayCB.userParam);
    AL_PictMngr_SignalCallbackReleaseIsDone(&pCtx->PictMngr, pFrameToRelease);
  }
}

/*****************************************************************************/
static void DeinitPictureManager(AL_TDecCtx* pCtx)
{
  ReleaseFramePictureUnused(pCtx);
  AL_PictMngr_Deinit(&pCtx->PictMngr);
}

/*****************************************************************************/
void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  assert(pCtx);

  AL_PictMngr_DecommitPool(&pCtx->PictMngr);

  if(pCtx->Feeder)
    AL_BufferFeeder_Destroy(pCtx->Feeder);

  if(pCtx->eosBuffer)
    AL_Buffer_Unref(pCtx->eosBuffer);

  AL_IDecChannel_Destroy(pCtx->pDecChannel);
  DeinitPictureManager(pCtx);
  MemDesc_Free(&pCtx->circularBuf.tMD);
  Rtos_Free(pCtx->BufNoAE.tMD.pVirtualAddr);
  DeinitBuffers(pCtx);

  Rtos_DeleteSemaphore(pCtx->Sem);
  Rtos_DeleteEvent(pCtx->ScDetectionComplete);
  Rtos_DeleteMutex(pCtx->DecMutex);

  Rtos_Free(pDec);
}

/*****************************************************************************/
void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm, bool bForceCleanBuffers)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  pCtx->bConceal = bConceal;
  pCtx->bUseBoard = bUseBoard;

  pCtx->iTraceFirstFrame = iFrmID;
  pCtx->iTraceLastFrame = iFrmID + iNumFrm;

  if(iNumFrm > 0 || bForceCleanBuffers)
    AL_CLEAN_BUFFERS = 1;
}

/*****************************************************************************/
static bool isAud(AL_ECodec codec, int nut)
{
  if(isAVC(codec))
    return nut == AL_AVC_NUT_AUD;
  else
    return nut == AL_HEVC_NUT_AUD;
}

/*****************************************************************************/
static bool isSuffixSei(AL_ECodec codec, int nut)
{
  if(isAVC(codec))
    return nut == AL_AVC_NUT_SUFFIX_SEI;
  else
    return nut == AL_HEVC_NUT_SUFFIX_SEI;
}

static bool checkSeiUUID(uint8_t* pBufs, AL_TNal* pNal, AL_ECodec codec)
{
  uint32_t const uTotalUUIDSize = isAVC(codec) ? 23 : 24;

  if(pNal->uSize != uTotalUUIDSize)
    return false;

  int const iStart = isAVC(codec) ? 6 : 7;

  for(int i = 0; i < 16; i++)
  {
    if(SEI_SUFFIX_USER_DATA_UNREGISTERED_UUID[i] != pBufs[pNal->tStartCode.uPosition + iStart + i])
      return false;
  }

  return true;
}

/*****************************************************************************/
static bool enoughStartCode(int iNumStartCode)
{
  return iNumStartCode > 1;
}

static bool isStartCode(uint8_t* pBuf, uint32_t uSize, uint32_t uPos)
{
  return (pBuf[uPos % uSize] == 0x00) &&
         (pBuf[(uPos + 1) % uSize] == 0x00) &&
         (pBuf[(uPos + 2) % uSize] == 0x01);
}

static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int const iNalHdrSize = isAVC(eCodec) ? AVC_NAL_HDR_SIZE : HEVC_NAL_HDR_SIZE;
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

static bool isVcl(AL_ECodec eCodec, AL_ENut eNUT)
{
  return isAVC(eCodec) ? AL_AVC_IsVcl(eNUT) : AL_HEVC_IsVcl(eNUT);
}

/* should only be used when the position is right after the nal header */
static bool isFirstSlice(uint8_t* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return pBuf[uPos] & 0x80;
}

/*****************************************************************************/
static bool SearchNextDecodingUnit(AL_TDecCtx* pCtx, TCircBuffer* pStream, int* pLastStartCodeInDecodingUnit, int* iLastVclNalInDecodingUnit)
{
  if(!enoughStartCode(pCtx->uNumSC))
    return false;

  int const iNalCount = (int)pCtx->uNumSC;
  AL_ECodec const eCodec = pCtx->chanParam.eCodec;
  int const iIsNotLastSlice = -1;
  AL_TNal* pTable = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
  uint8_t* pBuf = pStream->tMD.pVirtualAddr;
  uint32_t uSize = pStream->tMD.uSize;

  bool bVCLNalSeen = false;
  int iNalFound = 0;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_TNal* pNal = &pTable[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(iNal > 0)
      iNalFound++;

    if(!isVcl(eCodec, eNUT))
    {
      if(isAud(eCodec, eNUT))
      {
        if(bVCLNalSeen)
        {
          iNalFound--;
          *pLastStartCodeInDecodingUnit = iNalFound;
          return true;
        }
      }

      if(isSuffixSei(eCodec, eNUT) && checkSeiUUID(pBuf, pNal, eCodec))
      {
        if(bVCLNalSeen)
        {
          *pLastStartCodeInDecodingUnit = iNalFound;
          return true;
        }
      }
    }

    if(isVcl(eCodec, eNUT))
    {
      int iNalHdrSize = isAVC(eCodec) ? AVC_NAL_HDR_SIZE : HEVC_NAL_HDR_SIZE;

      // We want to check the first byte avec NAL_HDR_SIZE
      if((int)pNal->uSize > iNalHdrSize)
      {
        uint32_t uPos = pNal->tStartCode.uPosition;
        assert(isStartCode(pBuf, uSize, uPos));
        uPos = skipNalHeader(uPos, eCodec, uSize);
        bool bIsFirstSlice = isFirstSlice(pBuf, uPos);

        if(bVCLNalSeen && bIsFirstSlice)
        {
          iNalFound--;
          *pLastStartCodeInDecodingUnit = iNalFound;
          return true;
        }
      }

      if(pCtx->chanParam.eDecUnit == AL_VCL_NAL_UNIT)
      {
        if(bVCLNalSeen)
        {
          iNalFound--;
          *pLastStartCodeInDecodingUnit = iNalFound;
          *iLastVclNalInDecodingUnit = iIsNotLastSlice;
          return true;
        }
      }

      bVCLNalSeen = true;
      *iLastVclNalInDecodingUnit = iNal;
    }
  }

  return false;
}

/*****************************************************************************/
static void DecodeOneAVCNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(ScTable.tStartCode.uNUT >= AL_AVC_NUT_ERR)
    return;

  AL_AVC_DecodeOneNAL(&pCtx->aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
}

/*****************************************************************************/
static void DecodeOneHEVCNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(ScTable.tStartCode.uNUT >= AL_HEVC_NUT_ERR)
    return;

  AL_HEVC_DecodeOneNAL(&pCtx->aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
}

/*****************************************************************************/
static void DecodeOneNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(*pNumSlice > 0 && *pNumSlice > pCtx->chanParam.iMaxSlices)
    return;

  if(isAVC(pCtx->chanParam.eCodec))
    DecodeOneAVCNAL(pCtx, ScTable, pNumSlice, bIsLastVclNal);
  else
    DecodeOneHEVCNAL(pCtx, ScTable, pNumSlice, bIsLastVclNal);
}

/*****************************************************************************/
static void GenerateScdIpTraces(AL_TDecCtx* pCtx, AL_TScParam ScP, AL_TScBufferAddrs ScdBuffer, TCircBuffer StreamSCD, TMemDesc scBuffer)
{
  (void)pCtx;
  (void)ScP;
  (void)ScdBuffer;
  (void)StreamSCD;
  (void)scBuffer;
}

/*****************************************************************************/
static bool canStoreMoreStartCodes(AL_TDecCtx* pCtx)
{
  return (pCtx->uNumSC + 1) * sizeof(AL_TNal) <= pCtx->SCTable.tMD.uSize - pCtx->BufSCD.tMD.uSize;
}

/*****************************************************************************/
static void ResetStartCodes(AL_TDecCtx* pCtx)
{
  pCtx->uNumSC = 0;
}

/*****************************************************************************/
static size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  else
    return uSize + uSecondPos - uFirstPos;
}

/*****************************************************************************/
static bool RefillStartCodes(AL_TDecCtx* pCtx, TCircBuffer* pScStreamView)
{
  AL_TScParam ScP = { 0 };
  AL_TScBufferAddrs ScdBuffer = { 0 };
  TMemDesc scBuffer = pCtx->BufSCD.tMD;

  ScP.MaxSize = scBuffer.uSize >> 3;
  ScP.AVC = isAVC(pCtx->chanParam.eCodec);

  if(pScStreamView->iAvailSize <= 4)
    return false;

  ScdBuffer.pBufOut = scBuffer.uPhysicalAddr;
  ScdBuffer.pStream = pScStreamView->tMD.uPhysicalAddr;
  ScdBuffer.uMaxSize = pScStreamView->tMD.uSize;
  ScdBuffer.uOffset = pScStreamView->iOffset;
  ScdBuffer.uAvailSize = pScStreamView->iAvailSize;

  AL_CleanupMemory(scBuffer.pVirtualAddr, scBuffer.uSize);

  AL_CB_EndStartCode callback = { AL_Decoder_EndScd, pCtx };
  AL_IDecChannel_SearchSC(pCtx->pDecChannel, &ScP, &ScdBuffer, callback);
  Rtos_WaitEvent(pCtx->ScDetectionComplete, AL_WAIT_FOREVER);

  GenerateScdIpTraces(pCtx, ScP, ScdBuffer, *pScStreamView, scBuffer);
  pScStreamView->iOffset = (pScStreamView->iOffset + pCtx->ScdStatus.uNumBytes) % pScStreamView->tMD.uSize;
  pScStreamView->iAvailSize -= pCtx->ScdStatus.uNumBytes;

  if(pCtx->uNumSC && pCtx->ScdStatus.uNumSC)
  {
    AL_TNal* dst = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
    AL_TStartCode const* src = (AL_TStartCode const*)scBuffer.pVirtualAddr;
    dst[pCtx->uNumSC - 1].uSize = DeltaPosition(dst[pCtx->uNumSC - 1].tStartCode.uPosition, src[0].uPosition, ScdBuffer.uMaxSize);
  }

  for(int i = 0; i < pCtx->ScdStatus.uNumSC; i++)
  {
    AL_TNal* dst = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
    AL_TStartCode const* src = (AL_TStartCode const*)scBuffer.pVirtualAddr;
    dst[pCtx->uNumSC].tStartCode = src[i];

    if(i + 1 == pCtx->ScdStatus.uNumSC)
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, pScStreamView->iOffset, ScdBuffer.uMaxSize);
    else
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, src[i + 1].uPosition, ScdBuffer.uMaxSize);
    pCtx->uNumSC++;
  }

  return pCtx->ScdStatus.uNumSC > 0;
}

/*****************************************************************************/
static int FindNextDecodingUnit(AL_TDecCtx* pCtx, TCircBuffer* pScStreamView, int* iLastVclNalInAU)
{
  int iLastStartCodeIdx = 0;

  while(!SearchNextDecodingUnit(pCtx, pScStreamView, &iLastStartCodeIdx, iLastVclNalInAU))
  {
    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      ResetStartCodes(pCtx);
    }

    if(!RefillStartCodes(pCtx, pScStreamView))
      return 0;
  }

  return iLastStartCodeIdx + 1;
}

/*****************************************************************************/
static UNIT_ERROR DecodeOneUnit(AL_TDecCtx* pCtx, TCircBuffer* pScStreamView, int iNalCount, int iLastVclNalInAU)
{
  AL_TNal* nals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  /* copy start code buffer stream information into decoder stream buffer */
  pCtx->Stream.tMD = pScStreamView->tMD;

  int iNumSlice = 0;
  bool bIsEndOfFrame = false;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_TNal CurrentNal = nals[iNal];
    AL_TStartCode CurrentStartCode = CurrentNal.tStartCode;
    AL_TStartCode NextStartCode;

    if(iNal + 1 < pCtx->uNumSC)
    {
      NextStartCode = nals[iNal + 1].tStartCode;
    }
    else /* if we didn't wait for the next start code to arrive to decode the current NAL */
    {
      /* If there isn't a next start code, we take the end of the data processed
       * by the start code detector */
      NextStartCode.uPosition = pScStreamView->iOffset;
    }

    pCtx->Stream.iOffset = CurrentStartCode.uPosition;
    pCtx->Stream.iAvailSize = DeltaPosition(CurrentStartCode.uPosition, NextStartCode.uPosition, pScStreamView->tMD.uSize);

    bool bIsLastVclNal = (iNal == iLastVclNalInAU);

    if(bIsLastVclNal)
      bIsEndOfFrame = true;
    DecodeOneNAL(pCtx, CurrentNal, &iNumSlice, bIsLastVclNal);

    if(pCtx->eChanState == CHAN_INVALID)
      return ERR_UNIT_INVALID_CHANNEL;

    if(bIsLastVclNal)
    {
      pCtx->bFirstSliceInFrameIsValid = false;
      pCtx->bBeginFrameIsValid = false;
    }
  }

  pCtx->uNumSC -= iNalCount;
  Rtos_Memmove(nals, nals + iNalCount, pCtx->uNumSC * sizeof(AL_TNal));

  return bIsEndOfFrame ? SUCCESS_ACCESS_UNIT : SUCCESS_NAL_UNIT;
}


/*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_TryDecodeOneUnit(AL_TDecoder* pAbsDec, TCircBuffer* pScStreamView)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);


  int iLastVclNalInAU = -1;
  int iNalCount = FindNextDecodingUnit(pCtx, pScStreamView, &iLastVclNalInAU);

  if(iNalCount == 0)
    return ERR_UNIT_NOT_FOUND;

  return DecodeOneUnit(pCtx, pScStreamView, iNalCount, iLastVclNalInAU);
}

/*****************************************************************************/
bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  return AL_BufferFeeder_PushBuffer(pCtx->Feeder, pBuf, uSize, false);
}

/*****************************************************************************/
void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_BufferFeeder_Flush(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_WaitFrameSent(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
    Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseFrames(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
    Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  ResetStartCodes(pCtx);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = 0;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  AL_BufferFeeder_Reset(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);


  AL_Default_Decoder_WaitFrameSent(pAbsDec);

  AL_PictMngr_Flush(&pCtx->PictMngr);

  AL_Default_Decoder_FlushInput(pAbsDec);

  // Send eos & get last frames in dpb if any
  AL_sDecoder_CallDisplay(pCtx);

  AL_Default_Decoder_ReleaseFrames(pAbsDec);

  pCtx->displayCB.func(NULL, NULL, pCtx->displayCB.userParam);

  AL_PictMngr_Terminate(&pCtx->PictMngr);
}

static void CheckDisplayBufferCanBeUsed(AL_TDecCtx* pCtx, AL_TBuffer* pBuf)
{
  assert(pBuf);
  AL_TSrcMetaData* pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  assert(pMeta);

  /* decoder only output semiplanar */
  assert(pMeta->tPitches.iLuma == pMeta->tPitches.iChroma);

  AL_TStreamSettings* pSettings = &pCtx->tStreamSettings;

  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);

  assert(pMeta->tPitches.iLuma >= (int)AL_Decoder_GetMinPitch(pSettings->tDim.iWidth, pSettings->iBitDepth, eDisplayStorageMode));

  assert(pMeta->tPitches.iLuma % 64 == 0);
}

/*****************************************************************************/
void AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  CheckDisplayBufferCanBeUsed(pCtx, pDecPict);

  AL_PictMngr_PutDisplayBuffer(&pCtx->PictMngr, pDecPict);
  AL_BufferFeeder_Signal(pCtx->Feeder);
}

/*****************************************************************************/
int AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return pCtx->tStreamSettings.iBitDepth;
}

/*****************************************************************************/
static int Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int iCurOffset;
  Rtos_GetMutex(pCtx->DecMutex);
  iCurOffset = pCtx->iCurOffset;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return iCurOffset;
}

/*****************************************************************************/
int AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  return Secure_GetStrOffset(pCtx);
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_GetLastError(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_GetMutex(pCtx->DecMutex);
  AL_ERR ret = pCtx->error;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return ret;
}

/*****************************************************************************/
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  assert(!pCtx->bIsBuffersAllocated);
  assert(IsAllStreamSettingsSet(pCtx->tStreamSettings));

  AL_TStreamSettings tStreamSettings = pCtx->tStreamSettings;

  if(pCtx->tStreamSettings.eSequenceMode == AL_SM_MAX_ENUM)
  {
    pCtx->error = AL_ERR_REQUEST_MALFORMED;
    return false;
  }

  int const iSPSMaxSlices = isAVC(pCtx->chanParam.eCodec) ? Avc_GetMaxNumberOfSlices(122, 52, 1, 60, INT32_MAX) : 600; // TODO FIX
  int const iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int const iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int const iSizeCompData = isAVC(pCtx->chanParam.eCodec) ? AL_GetAllocSize_AvcCompData(tStreamSettings.tDim, tStreamSettings.eChroma) : AL_GetAllocSize_HevcCompData(tStreamSettings.tDim, tStreamSettings.eChroma);
  int const iSizeCompMap = AL_GetAllocSize_DecCompMap(tStreamSettings.tDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  int const iDpbMaxBuf = isAVC(pCtx->chanParam.eCodec) ? AL_AVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight) : AL_HEVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight);
  int const iRecBuf = isAVC(pCtx->chanParam.eCodec) ? REC_BUF : 0;
  int const iConcealBuf = CONCEAL_BUF;
  int const iMaxBuf = iDpbMaxBuf + pCtx->iStackSize + iRecBuf + iConcealBuf;
  int const iSizeMV = isAVC(pCtx->chanParam.eCodec) ? AL_GetAllocSize_AvcMV(tStreamSettings.tDim) : AL_GetAllocSize_HevcMV(tStreamSettings.tDim);
  int const iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  int const iDpbRef = iDpbMaxBuf;
  AL_EFbStorageMode const eStorageMode = pCtx->chanParam.eFBStorageMode;

  bool bEnableRasterOutput = pCtx->chanParam.eBufferOutputMode != AL_OUTPUT_INTERNAL;

  AL_PictMngr_Init(&pCtx->PictMngr, pCtx->pAllocator, iMaxBuf, iSizeMV, iDpbRef, pCtx->eDpbMode, eStorageMode, tStreamSettings.iBitDepth, bEnableRasterOutput);


  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);

  int iSizeYuv = AL_GetAllocSize_Frame(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);
  AL_TCropInfo const tCropInfo = { false, 0, 0, 0, 0 }; // XXX

  pCtx->resolutionFoundCB.func(iMaxBuf, iSizeYuv, &tStreamSettings, &tCropInfo, pCtx->resolutionFoundCB.userParam);

  pCtx->bIsBuffersAllocated = true;

  return true;
  fail_alloc:
  pCtx->error = AL_ERR_NO_MEMORY;
  return false;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferHEVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x28, 0, 0x80
  }; // simulate a new frame
  return AL_Buffer_WrapData((uint8_t*)EOSNal, sizeof EOSNal, &AL_Buffer_Destroy);
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferAVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x01, 0x80
  }; // simulate a new AU
  return AL_Buffer_WrapData((uint8_t*)EOSNal, sizeof EOSNal, &AL_Buffer_Destroy);
}

/*****************************************************************************/
static bool CheckStreamSettings(AL_TStreamSettings tStreamSettings)
{
  if(IsAtLeastOneStreamSettingsSet(tStreamSettings))
  {
    if(!IsAllStreamSettingsSet(tStreamSettings))
      return false;
  }
  return true;
}

/*****************************************************************************/
static bool isSubframe(AL_EDecUnit eDecUnit)
{
  return eDecUnit == AL_VCL_NAL_UNIT;
}

/*****************************************************************************/
static bool CheckAVCSettings(AL_TDecSettings const* pSettings)
{
  assert(pSettings->eCodec == AL_CODEC_AVC);

  if(pSettings->bParallelWPP)
    return false;

  if((pSettings->tStream.eSequenceMode != AL_SM_UNKNOWN) && (pSettings->tStream.eSequenceMode != AL_SM_PROGRESSIVE) && (pSettings->tStream.eSequenceMode != AL_SM_MAX_ENUM))
    return false;

  return true;
}

/*****************************************************************************/
static bool CheckSettings(AL_TDecSettings const* pSettings)
{
  if(!CheckStreamSettings(pSettings->tStream))
    return false;

  int const iStack = pSettings->iStackSize;

  if((iStack < 1) || (iStack > MAX_STACK_SIZE))
    return false;

  if((pSettings->uDDRWidth != 16) && (pSettings->uDDRWidth != 32) && (pSettings->uDDRWidth != 64))
    return false;

  if(isSubframe(pSettings->eDecUnit) && pSettings->bParallelWPP)
    return false;

  if(pSettings->eCodec == AL_CODEC_AVC)
  {
    if(!CheckAVCSettings(pSettings))
      return false;
  }

  return true;
}

/*****************************************************************************/
static void AssignSettings(AL_TDecCtx* const pCtx, AL_TDecSettings const* const pSettings)
{
  pCtx->iStackSize = pSettings->iStackSize;
  pCtx->bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->eDpbMode = pSettings->eDpbMode;
  pCtx->tStreamSettings = pSettings->tStream;

  AL_TDecChanParam* pChan = &pCtx->chanParam;
  pChan->uMaxLatency = pSettings->iStackSize;
  pChan->uNumCore = pSettings->uNumCore;
  pChan->uClkRatio = pSettings->uClkRatio;
  pChan->uFrameRate = pSettings->uFrameRate;
  pChan->bDisableCache = pSettings->bDisableCache;
  pChan->bFrameBufferCompression = pSettings->bFrameBufferCompression;
  pChan->eFBStorageMode = pSettings->eFBStorageMode;
  pChan->uDDRWidth = pSettings->uDDRWidth == 16 ? 0 : pSettings->uDDRWidth == 32 ? 1 : 2;
  pChan->bParallelWPP = pSettings->bParallelWPP;
  pChan->bLowLat = pSettings->bLowLat;
  pChan->eCodec = pSettings->eCodec;
  pChan->eDecUnit = pSettings->eDecUnit;

  pChan->eBufferOutputMode = pSettings->eBufferOutputMode;

}

/*****************************************************************************/
static bool CheckCallBacks(AL_TDecCallBacks* pCallbacks)
{
  if(pCallbacks->endDecodingCB.func == NULL)
    return false;

  if(pCallbacks->displayCB.func == NULL)
    return false;

  if(pCallbacks->resolutionFoundCB.func == NULL)
    return false;

  return true;
}

/*****************************************************************************/
static void AssignCallBacks(AL_TDecCtx* const pCtx, AL_TDecCallBacks* pCB)
{
  pCtx->decodeCB = pCB->endDecodingCB;
  pCtx->displayCB = pCB->displayCB;
  pCtx->resolutionFoundCB = pCB->resolutionFoundCB;
  pCtx->parsedSeiCB = pCB->parsedSeiCB;
}

static void errorHandler(void* pUserParam)
{
  AL_TDefaultDecoder* const pDec = (AL_TDefaultDecoder*)pUserParam;
  AL_TDecCtx* const pCtx = &pDec->ctx;
  pCtx->decodeCB.func(NULL, pCtx->decodeCB.userParam);
}

bool AL_Default_Decoder_AllocPool(AL_TDecCtx* pCtx, int iWPSize, int iSPSize, int iCompDataSize, int iCompMapSize)
{
#define SAFE_POOL_ALLOC(pCtx, pMD, iSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, iSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < pCtx->iStackSize; ++i)
  {
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolWP[i].tMD, iWPSize, "wp");
    AL_CleanupMemory(pCtx->PoolWP[i].tMD.pVirtualAddr, pCtx->PoolWP[i].tMD.uSize);
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolSP[i].tMD, iSPSize, "sp");
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompData[i].tMD, iCompDataSize, "comp data");
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompMap[i].tMD, iCompMapSize, "comp map");
  }

  return true;
}

bool AL_Default_Decoder_AllocMv(AL_TDecCtx* pCtx, int iMVSize, int iPOCSize, int iNum)
{
#define SAFE_MV_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < iNum; ++i)
  {
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD, iMVSize, "mv");
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD, iPOCSize, "poc");
  }

  return true;
}

/*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->error = eError;
  Rtos_ReleaseMutex(pCtx->DecMutex);
}


/*****************************************************************************/
static AL_TDecoderVtable const AL_Default_Decoder_Vtable =
{
  &AL_Default_Decoder_Destroy,
  &AL_Default_Decoder_SetParam,
  &AL_Default_Decoder_PushBuffer,
  &AL_Default_Decoder_Flush,
  &AL_Default_Decoder_PutDecPict,
  &AL_Default_Decoder_GetMaxBD,
  &AL_Default_Decoder_GetLastError,
  &AL_Default_Decoder_PreallocateBuffers,

  // only for the feeders
  &AL_Default_Decoder_TryDecodeOneUnit,
  &AL_Default_Decoder_InternalFlush,
  &AL_Default_Decoder_GetStrOffset,
  &AL_Default_Decoder_FlushInput,
};

/*****************************************************************************/
static void InitAUP(AL_TDecCtx* pCtx)
{
  if(isAVC(pCtx->chanParam.eCodec))
    AL_AVC_InitAUP(&pCtx->aup.avcAup);
  else
    AL_HEVC_InitAUP(&pCtx->aup.hevcAup);
}

/*****************************************************************************/
AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  *hDec = NULL;

  if(!CheckSettings(pSettings))
    return AL_ERR_REQUEST_MALFORMED;

  if(!CheckCallBacks(pCB))
    return AL_ERR_REQUEST_MALFORMED;

  AL_TDefaultDecoder* const pDec = (AL_TDefaultDecoder*)Rtos_Malloc(sizeof(AL_TDefaultDecoder));
  AL_ERR errorCode = AL_ERROR;

  if(!pDec)
    return AL_ERR_NO_MEMORY;

  Rtos_Memset(pDec, 0, sizeof(*pDec));

  pDec->vtable = &AL_Default_Decoder_Vtable;
  AL_TDecCtx* const pCtx = &pDec->ctx;

  pCtx->pDecChannel = pDecChannel;
  pCtx->pAllocator = pAllocator;

  InitInternalBuffers(pCtx);

  AssignSettings(pCtx, pSettings);
  AssignCallBacks(pCtx, pCB);

  pCtx->Sem = Rtos_CreateSemaphore(pCtx->iStackSize);
  pCtx->ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->DecMutex = Rtos_CreateMutex();


  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, false, false, 0, 0, false);

  // initialize decoder context
  pCtx->bFirstIsValid = false;
  pCtx->PictMngr.bFirstInit = false;
  pCtx->uNoRaslOutputFlag = 1;
  pCtx->bIsFirstPicture = true;
  pCtx->bLastIsEOS = false;
  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
  pCtx->bIsFirstSPSChecked = false;
  pCtx->bIsBuffersAllocated = false;

  AL_Conceal_Init(&pCtx->tConceal);

  // initialize decoder counters
  pCtx->uCurPocLsb = 0xFFFFFFFF;
  pCtx->uToggle = 0;
  pCtx->iNumFrmBlk1 = 0;
  pCtx->iNumFrmBlk2 = 0;
  pCtx->iCurOffset = 0;
  pCtx->iTraceCounter = 0;
  pCtx->eChanState = CHAN_UNINITIALIZED;

  // initialize tile information
  pCtx->uCurTileID = 0;
  pCtx->bTileSupToSlice = false;

  // initialize slice toggle information
  pCtx->uCurID = 0;

  InitAUP(pCtx);

#define SAFE_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
    { \
      errorCode = AL_ERR_NO_MEMORY; \
      goto cleanup; \
    } \
  } while(0)

  pCtx->uNumSC = 0;

  // Alloc Start Code Detector buffer
  SAFE_ALLOC(pCtx, &pCtx->BufSCD.tMD, SCD_SIZE, "scd");
  AL_CleanupMemory(pCtx->BufSCD.tMD.pVirtualAddr, pCtx->BufSCD.tMD.uSize);

  SAFE_ALLOC(pCtx, &pCtx->SCTable.tMD, pCtx->iStackSize * MAX_NAL_UNIT * sizeof(AL_TNal), "sctable");
  AL_CleanupMemory(pCtx->SCTable.tMD.pVirtualAddr, pCtx->SCTable.tMD.uSize);

  // Alloc Decoder buffers
  for(int i = 0; i < pCtx->iStackSize; ++i)
  {
    SAFE_ALLOC(pCtx, &pCtx->PoolListRefAddr[i].tMD, REF_LIST_SIZE, "reflist");
    SAFE_ALLOC(pCtx, &pCtx->PoolSclLst[i].tMD, SCLST_SIZE_DEC, "scllst");
    AL_CleanupMemory(pCtx->PoolSclLst[i].tMD.pVirtualAddr, pCtx->PoolSclLst[i].tMD.uSize);

    Rtos_Memset(&pCtx->PoolPP[i], 0, sizeof(pCtx->PoolPP[0]));
    Rtos_Memset(&pCtx->PoolPB[i], 0, sizeof(pCtx->PoolPB[0]));
    AL_SET_DEC_OPT(&pCtx->PoolPP[i], IntraOnly, 1);
  }


  // Alloc Decoder Deanti-emulated buffer for high level syntax parsing
  pCtx->BufNoAE.tMD.pVirtualAddr = Rtos_Malloc(NON_VCL_NAL_SIZE);
  pCtx->BufNoAE.tMD.uSize = NON_VCL_NAL_SIZE;

  if(!pCtx->BufNoAE.tMD.pVirtualAddr)
    goto cleanup;

  pCtx->eosBuffer = isAVC(pCtx->chanParam.eCodec) ? AllocEosBufferAVC() : AllocEosBufferHEVC();

  if(!pCtx->eosBuffer)
    goto cleanup;

  AL_Buffer_Ref(pCtx->eosBuffer);

  int const iBufferStreamSize = GetCircularBufferSize(isAVC(pCtx->chanParam.eCodec), pCtx->iStackSize, pCtx->tStreamSettings);
  int const iInputFifoSize = 256;
  AL_CB_Error errorCallback =
  {
    errorHandler,
    (void*)pDec
  };

  if(!MemDesc_AllocNamed(&pCtx->circularBuf.tMD, pAllocator, iBufferStreamSize, "circular stream"))
    goto cleanup;

  pCtx->Feeder = AL_BufferFeeder_Create((AL_HDecoder)pDec, &pCtx->circularBuf, iInputFifoSize, &errorCallback);

  if(!pCtx->Feeder)
    goto cleanup;

  pCtx->Feeder->eosBuffer = pCtx->eosBuffer;
  pCtx->error = AL_SUCCESS;

  *hDec = (AL_TDecoder*)pDec;

  return AL_SUCCESS;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pDec);
  return errorCode;
}

AL_EFbStorageMode AL_Default_Decoder_GetDisplayStorageMode(AL_TDecCtx* pCtx, bool* pEnableCompression)
{
  AL_EFbStorageMode eDisplayStorageMode = pCtx->chanParam.eFBStorageMode;
  *pEnableCompression = pCtx->chanParam.bFrameBufferCompression;


  return eDisplayStorageMode;
}

/*@}*/

