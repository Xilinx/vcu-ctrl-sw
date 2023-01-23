/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include "I_DecoderCtx.h"

#include "lib_common/Utils.h"
#include "lib_common/BufferSeiMeta.h"

#include "lib_common_dec/RbspParser.h"

/*************************************************************************//*!
   \brief This function returns a pointer to the Nal data cleaned of the AE bytes
   \param[in]  pBufNoAE  Pointer to the buffer receiving the nal data without the antiemulated bytes
   \param[in]  pStream   Pointer to the circular stream buffer
   \param[in]  uLength   Maximum bytes's number to be parsed
*****************************************************************************/
static uint32_t AL_sCount_AntiEmulBytes(TCircBuffer* pStream, uint32_t uLength)
{
  uint8_t uSCDetect = 0;
  uint32_t uNumAE = 0;
  uint32_t uZeroBytesCount = 0;

  uint8_t* pBuf = pStream->tMD.pVirtualAddr;

  uint32_t uSize = pStream->tMD.uSize;
  uint32_t uOffset = pStream->iOffset;
  uint32_t uEnd = uOffset + uLength;

  // Replaces in m_pBuffer all sequences such as 0x00 0x00 0x03 0xZZ with 0x00 0x00 0xZZ (0x03 removal)
  // iff 0xZZ == 0x00 or 0x01 or 0x02 or 0x03.
  for(uint32_t uRead = uOffset; uRead < uEnd; ++uRead)
  {
    const uint8_t read = pBuf[uRead % uSize];

    if((uZeroBytesCount == 2) && (read == 0x03))
    {
      uZeroBytesCount = 0;
      ++uEnd;
      ++uNumAE;
      continue;
    }

    if((uZeroBytesCount >= 2) && (read == 0x01))
    {
      ++uSCDetect;

      if(uSCDetect == 2)
        return uNumAE;
    }

    if(read == 0x00)
      ++uZeroBytesCount;
    else
      uZeroBytesCount = 0;
  }

  return uNumAE;
}

/*****************************************************************************/
uint32_t GetNonVclSize(TCircBuffer* pBufStream)
{
  int iNumZeros = 0;
  int iNumNALFound = 0;
  uint8_t* pParseBuf = pBufStream->tMD.pVirtualAddr;
  uint32_t uMaxSize = pBufStream->tMD.uSize;
  uint32_t uLengthNAL = 0;
  int iOffset = pBufStream->iOffset;
  int iAvailSize = pBufStream->iAvailSize;

  for(int i = iOffset; i < iOffset + iAvailSize; ++i)
  {
    uint8_t uRead = pParseBuf[i % uMaxSize];

    if(iNumZeros >= 2 && uRead == 0x01)
    {
      if(++iNumNALFound == 2)
      {
        uLengthNAL -= iNumZeros;
        break;
      }
      else
        iNumZeros = 0;
    }

    if(uRead == 0x00)
      ++iNumZeros;
    else
      iNumZeros = 0;

    ++uLengthNAL;
  }

  return uLengthNAL;
}

/*****************************************************************************/
static void InitNonVclBuf(AL_TDecCtx* pCtx)
{
  // The anti emulation works on chunks of ANTI_EMUL_GRANULARITY size so we need
  // the size to be aligned to that granularity.
  uint32_t uLengthNAL = RoundUp(GetNonVclSize(&pCtx->Stream), ANTI_EMUL_GRANULARITY);

  if(uLengthNAL > pCtx->BufNoAE.tMD.uSize) /* should occurs only on long SEI message */
  {
    Rtos_Free(pCtx->BufNoAE.tMD.pVirtualAddr);
    pCtx->BufNoAE.tMD.pVirtualAddr = (uint8_t*)Rtos_Malloc(uLengthNAL);
    AL_Assert(pCtx->BufNoAE.tMD.pVirtualAddr);
    pCtx->BufNoAE.tMD.uSize = uLengthNAL;
  }

  Rtos_Memset(pCtx->BufNoAE.tMD.pVirtualAddr, 0, pCtx->BufNoAE.tMD.uSize);
}

/*****************************************************************************/
static uint32_t GetSliceHdrSize(AL_TRbspParser* pRP, TCircBuffer* pBufStream)
{
  uint32_t uLengthNAL = (offset(pRP) + 7) >> 3;
  int iNumAE = AL_sCount_AntiEmulBytes(pBufStream, uLengthNAL);
  return uLengthNAL + iNumAE;
}

/*****************************************************************************/
void UpdateContextAtEndOfFrame(AL_TDecCtx* pCtx)
{
  pCtx->bIsFirstPicture = false;

  pCtx->tConceal.iFirstLCU = -1;
  pCtx->tConceal.bValidFrame = false;
  pCtx->PictMngr.uNumSlice = 0;

  Rtos_Memset(&pCtx->PoolPP[pCtx->uToggle], 0, sizeof(AL_TDecPicParam));
  Rtos_Memset(&pCtx->PoolPB[pCtx->uToggle], 0, sizeof(AL_TDecPicBuffers));
  AL_SET_DEC_OPT(&pCtx->PoolPP[pCtx->uToggle], IntraOnly, 1);
}

/*****************************************************************************/
void UpdateCircBuffer(AL_TRbspParser* pRP, TCircBuffer* pBufStream, int* pSliceHdrLength)
{
  uint32_t uLengthNAL = GetSliceHdrSize(pRP, pBufStream);

  // remap stream offset
  if(pRP->iTotalBitIndex % 8)
    *pSliceHdrLength = 16 + (pRP->iTotalBitIndex % 8);
  else
    *pSliceHdrLength = 24;
  uLengthNAL -= (*pSliceHdrLength + 7) >> 3;

  // Update Circular buffer information
  pBufStream->iAvailSize -= uLengthNAL;
  pBufStream->iOffset += uLengthNAL;
  pBufStream->iOffset %= pBufStream->tMD.uSize;
}

/*****************************************************************************/
bool SkipNal(void)
{
  return false;
}

/*****************************************************************************/
AL_TRbspParser getParserOnNonVclNal(AL_TDecCtx* pCtx, uint8_t* pBufNoAE)
{
  TCircBuffer* pBufStream = &pCtx->Stream;
  AL_TRbspParser rp;
  InitRbspParser(pBufStream, pBufNoAE, true, &rp);
  return rp;
}

/*****************************************************************************/
AL_TRbspParser getParserOnNonVclNalInternalBuf(AL_TDecCtx* pCtx)
{
  InitNonVclBuf(pCtx);
  return getParserOnNonVclNal(pCtx, pCtx->BufNoAE.tMD.pVirtualAddr);
}

