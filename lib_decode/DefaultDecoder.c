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
#include <assert.h>
#include <stdio.h>

#include "DefaultDecoder.h"
#include "I_DecoderCtx.h"
#include "NalUnitParser.h"

#include "lib_common/Error.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Utils.h"

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

/******************************************************************************/
static bool IsAllStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAllStreamDimSet(tStreamSettings.tDim) && IsStreamChromaSet(tStreamSettings.eChroma) && IsStreamBitDepthSet(tStreamSettings.iBitDepth) && IsStreamLevelSet(tStreamSettings.iLevel) && IsStreamProfileSet(tStreamSettings.iProfileIdc);
}

static bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAtLeastOneStreamDimSet(tStreamSettings.tDim) || IsStreamChromaSet(tStreamSettings.eChroma) || IsStreamBitDepthSet(tStreamSettings.iBitDepth) || IsStreamLevelSet(tStreamSettings.iLevel) || IsStreamProfileSet(tStreamSettings.iProfileIdc);
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
  if(!MemDesc_AllocNamed(pMD, pCtx->m_pAllocator, uSize, name))
  {
    pCtx->m_error = AL_ERR_NO_MEMORY;
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
  AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->m_PictMngr, iFrameID);
  assert(pDecodedFrame);

  pCtx->m_decodeCB.func(pDecodedFrame, pCtx->m_decodeCB.userParam);
}

/*****************************************************************************/
static void AL_sDecoder_CallDisplay(AL_TDecCtx* pCtx)
{
  while(1)
  {
    AL_TInfoDecode pInfo = { 0 };
    AL_TBuffer* pFrameToDisplay = AL_PictMngr_GetDisplayBuffer(&pCtx->m_PictMngr, &pInfo);

    if(!pFrameToDisplay)
      break;

    assert(AL_Buffer_GetData(pFrameToDisplay));

    pCtx->m_displayCB.func(pFrameToDisplay, &pInfo, pCtx->m_displayCB.userParam);
    AL_PictMngr_SignalCallbackDisplayIsDone(&pCtx->m_PictMngr, pFrameToDisplay);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, int iFrameID)
{
  if(pCtx->m_decodeCB.func)
    AL_sDecoder_CallDecode(pCtx, iFrameID);

  if(pCtx->m_displayCB.func)
    AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  uint8_t const uFrameID = pStatus->uFrmID;
  uint8_t const uMotionVectorID = pStatus->uMvID;

  AL_PictMngr_UpdateDisplayBufferCRC(&pCtx->m_PictMngr, uFrameID, pStatus->uCRC);
  int iOffset = pCtx->m_iNumFrmBlk2 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefMvID(&pCtx->m_PictMngr, pCtx->m_uNumRef[iOffset], pCtx->m_uMvIDRefList[iOffset]);
  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_iCurOffset = pCtx->m_iStreamOffset[pCtx->m_iNumFrmBlk2 % pCtx->m_iStackSize];
  ++pCtx->m_iNumFrmBlk2;

  if(pStatus->bHanged)
    printf("***** /!\\ Timeout - resetting the decoder /!\\ *****\n");

  Rtos_ReleaseMutex(pCtx->m_DecMutex);

  AL_BufferFeeder_Signal(pCtx->m_Feeder);
  AL_sDecoder_CallBacks(pCtx, uFrameID);

  Rtos_GetMutex(pCtx->m_DecMutex);
  AL_PictMngr_EndDecoding(&pCtx->m_PictMngr, uFrameID, uMotionVectorID);
  Rtos_ReleaseMutex(pCtx->m_DecMutex);

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
    MemDesc_Init(&pCtx->m_PictMngr.m_MvBufPool.pMvBufs[i].tMD);
    MemDesc_Init(&pCtx->m_PictMngr.m_MvBufPool.pPocBufs[i].tMD);
  }
}

/*****************************************************************************/
static void DeinitBuffers(AL_TDecCtx* pCtx)
{
  for(int i = 0; i < MAX_DPB_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->m_PictMngr.m_MvBufPool.pPocBufs[i].tMD);
    AL_Decoder_Free(&pCtx->m_PictMngr.m_MvBufPool.pMvBufs[i].tMD);
  }

  for(int i = 0; i < MAX_STACK_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->m_PoolCompData[i].tMD);
    AL_Decoder_Free(&pCtx->m_PoolCompMap[i].tMD);
    AL_Decoder_Free(&pCtx->m_PoolSP[i].tMD);
    AL_Decoder_Free(&pCtx->m_PoolWP[i].tMD);
    AL_Decoder_Free(&pCtx->m_PoolListRefAddr[i].tMD);
    AL_Decoder_Free(&pCtx->m_PoolSclLst[i].tMD);
  }

  AL_Decoder_Free(&pCtx->m_BufSCD.tMD);
  AL_Decoder_Free(&pCtx->m_SCTable.tMD);
  AL_Decoder_Free(&pCtx->m_BufNoAE.tMD);
}

/*****************************************************************************/
static void ReleaseFramePictureUnused(AL_TDecCtx* pCtx)
{
  while(1)
  {
    AL_TBuffer* pFrameToRelease = AL_PictMngr_GetUnusedDisplayBuffer(&pCtx->m_PictMngr);

    if(!pFrameToRelease)
      break;

    assert(AL_Buffer_GetData(pFrameToRelease));

    pCtx->m_displayCB.func(pFrameToRelease, NULL, pCtx->m_displayCB.userParam);

    AL_PictMngr_SignalCallbackReleaseIsDone(&pCtx->m_PictMngr, pFrameToRelease);
  }
}

/*****************************************************************************/
static void DeinitPictureManager(AL_TDecCtx* pCtx)
{
  ReleaseFramePictureUnused(pCtx);
  AL_PictMngr_Deinit(&pCtx->m_PictMngr);
}

/*****************************************************************************/
void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  assert(pCtx);

  AL_PictMngr_DecommitPool(&pCtx->m_PictMngr);

  if(pCtx->m_Feeder)
    AL_BufferFeeder_Destroy(pCtx->m_Feeder);

  if(pCtx->m_eosBuffer)
    AL_Buffer_Unref(pCtx->m_eosBuffer);

  DeinitPictureManager(pCtx);

  AL_IDecChannel_Destroy(pCtx->m_pDecChannel);

  MemDesc_Free(&pCtx->circularBuf.tMD);

  Rtos_Free(pCtx->m_BufNoAE.tMD.pVirtualAddr);

  DeinitBuffers(pCtx);

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

/*****************************************************************************/
static bool isEndOfFrameDelimiter(AL_ECodec codec, int nut)
{
  return isAud(codec, nut) || isSuffixSei(codec, nut);
}

/*****************************************************************************/
static bool enoughStartCode(int iNumStartCode)
{
  return iNumStartCode > 1;
}

static bool checkSEI_UUID(uint8_t* pBufs, AL_TNal nal, AL_ECodec codec)
{
  uint32_t const uTotalUUIDSize = isAVC(codec) ? 23 : 24;

  if(nal.uSize != uTotalUUIDSize)
    return false;

  int const iStart = isAVC(codec) ? 6 : 7;

  for(int i = 0; i < 16; i++)
  {
    if(SEI_SUFFIX_USER_DATA_UNREGISTERED_UUID[i] != pBufs[nal.tStartCode.uPosition + iStart + i])
      return false;
  }

  return true;
}

/*****************************************************************************/
static bool SearchNextDecodingUnit(AL_TDecCtx* pCtx, TCircBuffer* pStream, int* pLastStartCodeInDecodingUnit, int* iLastVclNalInDecodingUnit)
{
  if(!enoughStartCode(pCtx->m_uNumSC))
    return false;

  int const iNalCount = (int)pCtx->m_uNumSC;
  AL_ECodec const eCodec = pCtx->m_chanParam.eCodec;
  int const notFound = -1;
  int iLastVclNal = notFound;
  int iLastNonVclNal = notFound;

  AL_TNal* pTable = (AL_TNal*)pCtx->m_SCTable.tMD.pVirtualAddr;
  uint8_t* pBuf = pStream->tMD.pVirtualAddr;
  uint32_t uSize = pStream->tMD.uSize;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_ENut eNUT = pTable[iNal].tStartCode.uNUT;

    // The NAL returned by the last start code of the SCD may not be complete
    if((iNal == iNalCount - 1) && !isAud(eCodec, eNUT) && !(isSuffixSei(eCodec, eNUT) && checkSEI_UUID(pBuf, pTable[iNal], eCodec)))
      return false;

    bool bIsVcl = isAVC(eCodec) ? AL_AVC_IsVcl(eNUT) : AL_HEVC_IsVcl(eNUT);

    if(bIsVcl)
    {
      // Start Code
      uint32_t uPos = pTable[iNal].tStartCode.uPosition;

      assert(pBuf[uPos % uSize] == 0x00);
      assert(pBuf[(uPos + 1) % uSize] == 0x00);
      assert(pBuf[(uPos + 2) % uSize] == 0x01);

      int const iNalHdrSize = isAVC(eCodec) ? AVC_NAL_HDR_SIZE : HEVC_NAL_HDR_SIZE;
      uPos = (uPos + iNalHdrSize) % uSize; // skip start code + nal header

      bool const IsFirstSlice = pBuf[uPos] & 0x80;
      bool bFind = false;
      switch(pCtx->m_chanParam.eDecUnit)
      {
      case AL_AU_UNIT:
      {
        bFind = (iLastVclNal != notFound && IsFirstSlice);
        break;
      }
      case AL_VCL_NAL_UNIT:
      {
        bFind = iLastVclNal != notFound;
        break;
      }
      default:
        assert(0);
      }

      if(bFind)
      {
        *pLastStartCodeInDecodingUnit = Max(iLastNonVclNal - 1, iLastVclNal);
        *iLastVclNalInDecodingUnit = IsFirstSlice ? iLastVclNal : notFound;
        return true;
      }

      if(iLastVclNal == notFound || !IsFirstSlice)
        iLastVclNal = iNal;
      iLastNonVclNal = notFound;
    }
    else
    {
      if(iLastNonVclNal == notFound && (isAVC(eCodec) || (eNUT != AL_HEVC_NUT_SUFFIX_SEI)))
        iLastNonVclNal = iNal;

      if(isEndOfFrameDelimiter(eCodec, eNUT))
      {
        if(iLastVclNal == notFound)
          continue;

        if(isSuffixSei(eCodec, eNUT) && !(checkSEI_UUID(pBuf, pTable[iNal], eCodec)))
          continue;

        *pLastStartCodeInDecodingUnit = iNal;

        if(isAud(eCodec, eNUT))
          *pLastStartCodeInDecodingUnit -= 1;

        *iLastVclNalInDecodingUnit = iLastVclNal;
        return true;
      }
    }
  }

  return false;
}

/*****************************************************************************/
static bool DecodeOneNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(*pNumSlice > 0 && *pNumSlice > pCtx->m_chanParam.iMaxSlices)
    return true;

  if(isAVC(pCtx->m_chanParam.eCodec))
  {
    if(ScTable.tStartCode.uNUT >= AL_AVC_NUT_ERR)
      return false;

    return AL_AVC_DecodeOneNAL(&pCtx->m_aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
  }
  else
  {
    if(ScTable.tStartCode.uNUT >= AL_HEVC_NUT_ERR)
      return false;

    return AL_HEVC_DecodeOneNAL(&pCtx->m_aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
  }
}

/*****************************************************************************/
static void GenerateIpTraces(AL_TDecCtx* pCtx, AL_TScParam ScP, AL_TScBufferAddrs ScdBuffer, TCircBuffer StreamSCD, TMemDesc scBuffer)
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
  return (pCtx->m_uNumSC + 1) * sizeof(AL_TNal) <= pCtx->m_SCTable.tMD.uSize - pCtx->m_BufSCD.tMD.uSize;
}

/*****************************************************************************/
static void ResetStartCodes(AL_TDecCtx* pCtx)
{
  pCtx->m_uNumSC = 0;
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
static bool RefillStartCodes(AL_TDecCtx* pCtx, TCircBuffer* pBufStream)
{
  AL_TScParam ScP = { 0 };
  AL_TScBufferAddrs ScdBuffer = { 0 };
  TMemDesc scBuffer = pCtx->m_BufSCD.tMD;

  ScP.MaxSize = scBuffer.uSize >> 3;
  ScP.AVC = isAVC(pCtx->m_chanParam.eCodec);

  if(pBufStream->uAvailSize <= 4)
    return false;

  ScdBuffer.pBufOut = scBuffer.uPhysicalAddr;
  ScdBuffer.pStream = pBufStream->tMD.uPhysicalAddr;
  ScdBuffer.uMaxSize = pBufStream->tMD.uSize;
  ScdBuffer.uOffset = pBufStream->uOffset;
  ScdBuffer.uAvailSize = pBufStream->uAvailSize;

  AL_CleanupMemory(scBuffer.pVirtualAddr, scBuffer.uSize);

  AL_CB_EndStartCode callback = { AL_Decoder_EndScd, pCtx };
  AL_IDecChannel_SearchSC(pCtx->m_pDecChannel, &ScP, &ScdBuffer, callback);
  Rtos_WaitEvent(pCtx->m_ScDetectionComplete, AL_WAIT_FOREVER);

  GenerateIpTraces(pCtx, ScP, ScdBuffer, *pBufStream, scBuffer);
  pBufStream->uOffset = (pBufStream->uOffset + pCtx->m_ScdStatus.uNumBytes) % pBufStream->tMD.uSize;
  pBufStream->uAvailSize -= pCtx->m_ScdStatus.uNumBytes;

  if(pCtx->m_uNumSC && pCtx->m_ScdStatus.uNumSC)
  {
    AL_TNal* dst = (AL_TNal*)pCtx->m_SCTable.tMD.pVirtualAddr;
    AL_TStartCode const* src = (AL_TStartCode const*)scBuffer.pVirtualAddr;
    dst[pCtx->m_uNumSC - 1].uSize = DeltaPosition(dst[pCtx->m_uNumSC - 1].tStartCode.uPosition, src[0].uPosition, scBuffer.uSize);
  }

  for(int i = 0; i < pCtx->m_ScdStatus.uNumSC; i++)
  {
    AL_TNal* dst = (AL_TNal*)pCtx->m_SCTable.tMD.pVirtualAddr;
    AL_TStartCode const* src = (AL_TStartCode const*)scBuffer.pVirtualAddr;
    dst[pCtx->m_uNumSC].tStartCode = src[i];

    if(i + 1 == pCtx->m_ScdStatus.uNumSC)
      dst[pCtx->m_uNumSC].uSize = DeltaPosition(src[i].uPosition, pBufStream->uOffset, scBuffer.uSize);
    else
      dst[pCtx->m_uNumSC].uSize = DeltaPosition(src[i].uPosition, src[i + 1].uPosition, scBuffer.uSize);
    pCtx->m_uNumSC++;
  }

  return pCtx->m_ScdStatus.uNumSC > 0;
}

/*****************************************************************************/
static int FindNextDecodingUnit(AL_TDecCtx* pCtx, TCircBuffer* pBufStream, int* iLastVclNalInAU)
{
  int iLastStartCodeIdx = 0;

  while(!SearchNextDecodingUnit(pCtx, pBufStream, &iLastStartCodeIdx, iLastVclNalInAU))
  {
    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      ResetStartCodes(pCtx);
    }

    if(!RefillStartCodes(pCtx, pBufStream))
      return 0;
  }

  return iLastStartCodeIdx + 1;
}

/*****************************************************************************/
static bool DecodeOneUnit(AL_TDecCtx* pCtx, TCircBuffer* pBufStream, int iNalCount, int iLastVclNalInAU, bool* pEndOfFrame)
{
  AL_TNal* nals = (AL_TNal*)pCtx->m_SCTable.tMD.pVirtualAddr;

  /* copy start code buffer stream information into decoder stream buffer */
  pCtx->m_Stream.tMD = pBufStream->tMD;

  int iNumSlice = 0;

  *pEndOfFrame = false;
  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    bool bIsLastVclNal = (iNal == iLastVclNalInAU);
    if(bIsLastVclNal)
      *pEndOfFrame = true;
    AL_TNal CurNal = nals[iNal];
    AL_TNal NextStartCode = nals[iNal + 1];

    pCtx->m_Stream.uOffset = CurNal.tStartCode.uPosition;
    pCtx->m_Stream.uAvailSize = DeltaPosition(CurNal.tStartCode.uPosition, NextStartCode.tStartCode.uPosition, pBufStream->tMD.uSize);

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
  Rtos_Memmove(nals, nals + iNalCount, pCtx->m_uNumSC * sizeof(AL_TNal));

  return true;
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_TryDecodeOneAU(AL_TDecoder* pAbsDec, TCircBuffer* pBufStream, bool* pEndOfFrame)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  int iLastVclNalInAU = -1;
  int iNalCount = FindNextDecodingUnit(pCtx, pBufStream, &iLastVclNalInAU);

  if(iNalCount == 0)
    return AL_ERR_NO_FRAME_DECODED; /* no AU found */

  if(!DecodeOneUnit(pCtx, pBufStream, iNalCount, iLastVclNalInAU, pEndOfFrame))
    return AL_ERR_INIT_FAILED;

  return AL_SUCCESS;
}

/*****************************************************************************/
bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize, AL_EBufMode eMode)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  return AL_BufferFeeder_PushBuffer(pCtx->m_Feeder, pBuf, eMode, uSize, false);
}

/*****************************************************************************/
void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_BufferFeeder_Flush(pCtx->m_Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_WaitFrameSent(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->m_iStackSize; ++iSem)
    Rtos_GetSemaphore(pCtx->m_Sem, AL_WAIT_FOREVER);
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseFrames(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->m_iStackSize; ++iSem)
    Rtos_ReleaseSemaphore(pCtx->m_Sem);
}

/*****************************************************************************/
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  ResetStartCodes(pCtx);
  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_iCurOffset = 0;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
  AL_BufferFeeder_Reset(pCtx->m_Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  AL_Default_Decoder_WaitFrameSent(pAbsDec);

  AL_PictMngr_Flush(&pCtx->m_PictMngr);

  AL_Default_Decoder_FlushInput(pAbsDec);

  // Send eos & get last frames in dpb if any
  if(pCtx->m_displayCB.func)
    AL_sDecoder_CallDisplay(pCtx);

  AL_Default_Decoder_ReleaseFrames(pAbsDec);

  if(pCtx->m_displayCB.func)
    pCtx->m_displayCB.func(NULL, NULL, pCtx->m_displayCB.userParam);

  AL_PictMngr_Terminate(&pCtx->m_PictMngr);
}

/*****************************************************************************/
void AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  assert(pDecPict);
  AL_PictMngr_PutDisplayBuffer(&pCtx->m_PictMngr, pDecPict);
  AL_BufferFeeder_Signal(pCtx->m_Feeder);
}

/*****************************************************************************/
int AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return pCtx->m_tStreamSettings.iBitDepth;
}

/*****************************************************************************/
static int Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int iCurOffset;
  Rtos_GetMutex(pCtx->m_DecMutex);
  iCurOffset = pCtx->m_iCurOffset;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
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
  Rtos_GetMutex(pCtx->m_DecMutex);
  AL_ERR ret = pCtx->m_error;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
  return ret;
}

/*****************************************************************************/
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec)
{
  AL_TDefaultDecoder* pDec = (AL_TDefaultDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  assert(!pCtx->m_bIsBuffersAllocated);
  assert(IsAllStreamSettingsSet(pCtx->m_tStreamSettings));

  AL_TStreamSettings tStreamSettings = pCtx->m_tStreamSettings;

  int const iSPSMaxSlices = isAVC(pCtx->m_chanParam.eCodec) ? Avc_GetMaxNumberOfSlices(122, 52, 1, 60, INT32_MAX) : 600; // TODO FIX
  int const iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int const iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int const iSizeCompData = isAVC(pCtx->m_chanParam.eCodec) ? AL_GetAllocSize_AvcCompData(tStreamSettings.tDim, tStreamSettings.eChroma) : AL_GetAllocSize_HevcCompData(tStreamSettings.tDim, tStreamSettings.eChroma);
  int const iSizeCompMap = AL_GetAllocSize_CompMap(tStreamSettings.tDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  int const iDpbMaxBuf = isAVC(pCtx->m_chanParam.eCodec) ? AL_AVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight, pCtx->m_eDpbMode) : AL_HEVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight, pCtx->m_eDpbMode);
  int const iRecBuf = isAVC(pCtx->m_chanParam.eCodec) ? REC_BUF : 0;
  int const iConcealBuf = CONCEAL_BUF;
  int const iMaxBuf = iDpbMaxBuf + pCtx->m_iStackSize + iRecBuf + iConcealBuf;
  int const iSizeMV = isAVC(pCtx->m_chanParam.eCodec) ? AL_GetAllocSize_AvcMV(tStreamSettings.tDim) : AL_GetAllocSize_HevcMV(tStreamSettings.tDim);
  int const iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  int const iDpbRef = iDpbMaxBuf;
  AL_EFbStorageMode const eStorageMode = pCtx->m_chanParam.eFBStorageMode;
  AL_PictMngr_Init(&pCtx->m_PictMngr, iMaxBuf, iSizeMV, iDpbRef, pCtx->m_eDpbMode, eStorageMode, tStreamSettings.iBitDepth);

  if(pCtx->m_resolutionFoundCB.func)
  {
    int const iSizeYuv = AL_GetAllocSize_Frame(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth, pCtx->m_chanParam.bFrameBufferCompression, pCtx->m_chanParam.eFBStorageMode);
    AL_TCropInfo const tCropInfo = { false, 0, 0, 0, 0 }; // XXX
    pCtx->m_resolutionFoundCB.func(iMaxBuf, iSizeYuv, &tStreamSettings, &tCropInfo, pCtx->m_resolutionFoundCB.userParam);
  }

  pCtx->m_bIsBuffersAllocated = true;

  return true;
  fail_alloc:
  pCtx->m_error = AL_ERR_NO_MEMORY;
  return false;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferHEVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x28, 0, 0x80, 0, 0, 1, 0, 0
  }; // simulate a new frame
  return AL_Buffer_WrapData((uint8_t*)EOSNal, sizeof EOSNal, &AL_Buffer_Destroy);
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferAVC()
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x01, 0x80, 0, 0, 1, 0, 0
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
static bool CheckSettings(AL_TDecSettings* const pSettings)
{
  if(!CheckStreamSettings(pSettings->tStream))
    return false;

  int const iStack = pSettings->iStackSize;

  if((iStack < 1) || (iStack > MAX_STACK_SIZE))
    return false;

  if(pSettings->bIsAvc && pSettings->bParallelWPP)
    return false;

  if(isSubframe(pSettings->eDecUnit) && pSettings->bParallelWPP)
    return false;

  return true;
}

/*****************************************************************************/
static void AssignSettings(AL_TDecCtx* const pCtx, AL_TDecSettings* const pSettings)
{
  pCtx->m_iStackSize = pSettings->iStackSize;
  pCtx->m_bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->m_eDpbMode = pSettings->eDpbMode;
  pCtx->m_tStreamSettings = pSettings->tStream;

  AL_TDecChanParam* pChan = &pCtx->m_chanParam;
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
  pChan->eCodec = pSettings->bIsAvc ? AL_CODEC_AVC : AL_CODEC_HEVC;
  pChan->eDecUnit = pSettings->eDecUnit;
}

/*****************************************************************************/
static void AssignCallBacks(AL_TDecCtx* const pCtx, AL_TDecCallBacks* const pCB)
{
  pCtx->m_decodeCB = pCB->endDecodingCB;
  pCtx->m_displayCB = pCB->displayCB;
  pCtx->m_resolutionFoundCB = pCB->resolutionFoundCB;
}

static void errorHandler(void* pUserParam)
{
  AL_TDefaultDecoder* const pDec = (AL_TDefaultDecoder*)pUserParam;
  AL_TDecCtx* const pCtx = &pDec->ctx;
  pCtx->m_decodeCB.func(NULL, pCtx->m_decodeCB.userParam);
}

bool AL_Default_Decoder_AllocPool(AL_TDecCtx* pCtx, int iWPSize, int iSPSize, int iCompDataSize, int iCompMapSize)
{
#define SAFE_POOL_ALLOC(pCtx, pMD, iSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, iSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    SAFE_POOL_ALLOC(pCtx, &pCtx->m_PoolWP[i].tMD, iWPSize, "wp");
    AL_CleanupMemory(pCtx->m_PoolWP[i].tMD.pVirtualAddr, pCtx->m_PoolWP[i].tMD.uSize);
    SAFE_POOL_ALLOC(pCtx, &pCtx->m_PoolSP[i].tMD, iSPSize, "sp");
    SAFE_POOL_ALLOC(pCtx, &pCtx->m_PoolCompData[i].tMD, iCompDataSize, "comp data");
    SAFE_POOL_ALLOC(pCtx, &pCtx->m_PoolCompMap[i].tMD, iCompMapSize, "comp map");
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
    SAFE_MV_ALLOC(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pMvBufs[i].tMD, iMVSize, "mv");
    SAFE_MV_ALLOC(pCtx, &pCtx->m_PictMngr.m_MvBufPool.pPocBufs[i].tMD, iPOCSize, "poc");
  }

  return true;
}

/*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError)
{
  Rtos_GetMutex(pCtx->m_DecMutex);
  pCtx->m_error = eError;
  Rtos_ReleaseMutex(pCtx->m_DecMutex);
}


/*****************************************************************************/
const AL_TDecoderVtable AL_Default_Decoder_Vtable =
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
  &AL_Default_Decoder_TryDecodeOneAU,
  &AL_Default_Decoder_InternalFlush,
  &AL_Default_Decoder_GetStrOffset,
  &AL_Default_Decoder_FlushInput,
};

/*****************************************************************************/
static void InitAUP(AL_TDecCtx* pCtx)
{
  if(isAVC(pCtx->m_chanParam.eCodec))
    AL_AVC_InitAUP(&pCtx->m_aup.avcAup);
  else
    AL_HEVC_InitAUP(&pCtx->m_aup.hevcAup);
}

/*****************************************************************************/
AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  AL_TDefaultDecoder* const pDec = (AL_TDefaultDecoder*)Rtos_Malloc(sizeof(AL_TDefaultDecoder));
  *hDec = NULL;
  AL_ERR errorCode = AL_ERROR;

  if(!pDec)
    return AL_ERR_NO_MEMORY;

  Rtos_Memset(pDec, 0, sizeof(*pDec));

  pDec->vtable = &AL_Default_Decoder_Vtable;
  AL_TDecCtx* const pCtx = &pDec->ctx;

  pCtx->m_pDecChannel = pDecChannel;
  pCtx->m_pAllocator = pAllocator;

  // initialize internal buffers
  InitBuffers(pCtx);

  if(!CheckSettings(pSettings))
  {
    errorCode = AL_ERR_INIT_FAILED;
    goto cleanup;
  }

  AssignSettings(pCtx, pSettings);
  AssignCallBacks(pCtx, pCB);

  pCtx->m_Sem = Rtos_CreateSemaphore(pCtx->m_iStackSize);
  pCtx->m_ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->m_DecMutex = Rtos_CreateMutex();

  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, false, false, 0, 0);

  // initialize decoder context
  pCtx->m_bFirstIsValid = false;
  pCtx->m_PictMngr.m_bFirstInit = false;
  pCtx->m_uNoRaslOutputFlag = 1;
  pCtx->m_bIsFirstPicture = true;
  pCtx->m_bLastIsEOS = false;
  pCtx->m_bFirstSliceInFrameIsValid = false;
  pCtx->m_bBeginFrameIsValid = false;
  pCtx->m_bIsFirstSPSChecked = false;
  pCtx->m_bIsBuffersAllocated = false;

  AL_Conceal_Init(&pCtx->m_tConceal);

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

  InitAUP(pCtx);

#define SAFE_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
    { \
      errorCode = AL_ERR_NO_MEMORY; \
      goto cleanup; \
    } \
  } while(0)

  // Alloc Start Code Detector buffer
  SAFE_ALLOC(pCtx, &pCtx->m_BufSCD.tMD, SCD_SIZE, "scd");
  AL_CleanupMemory(pCtx->m_BufSCD.tMD.pVirtualAddr, pCtx->m_BufSCD.tMD.uSize);

  SAFE_ALLOC(pCtx, &pCtx->m_SCTable.tMD, pCtx->m_iStackSize * MAX_NAL_UNIT * sizeof(AL_TNal), "sctable");
  AL_CleanupMemory(pCtx->m_SCTable.tMD.pVirtualAddr, pCtx->m_SCTable.tMD.uSize);

  pCtx->m_uNumSC = 0;

  // Alloc Decoder buffers
  for(int i = 0; i < pCtx->m_iStackSize; ++i)
    SAFE_ALLOC(pCtx, &pCtx->m_PoolListRefAddr[i].tMD, REF_LIST_SIZE, "reflist");


  for(int i = 0; i < pCtx->m_iStackSize; ++i)
  {
    SAFE_ALLOC(pCtx, &pCtx->m_PoolSclLst[i].tMD, SCLST_SIZE_DEC, "scllst");
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

  pCtx->m_eosBuffer = isAVC(pCtx->m_chanParam.eCodec) ? AllocEosBufferAVC() : AllocEosBufferHEVC();

  if(!pCtx->m_eosBuffer)
    goto cleanup;

  AL_Buffer_Ref(pCtx->m_eosBuffer);

  int const iBufferStreamSize = GetCircularBufferSize(isAVC(pCtx->m_chanParam.eCodec), pCtx->m_iStackSize, pCtx->m_tStreamSettings);
  int const iInputFifoSize = 256;
  AL_CB_Error errorCallback =
  {
    errorHandler,
    (void*)pDec
  };

  if(!MemDesc_AllocNamed(&pCtx->circularBuf.tMD, pAllocator, iBufferStreamSize, "circular stream"))
    goto cleanup;

  pCtx->m_Feeder = AL_BufferFeeder_Create((AL_HDecoder)pDec, &pCtx->circularBuf, iInputFifoSize, &errorCallback);

  if(!pCtx->m_Feeder)
    goto cleanup;

  pCtx->m_Feeder->eosBuffer = pCtx->m_eosBuffer;
  pCtx->m_error = AL_SUCCESS;

  *hDec = (AL_TDecoder*)pDec;

  return AL_SUCCESS;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pDec);
  return errorCode;
}

/*@}*/

