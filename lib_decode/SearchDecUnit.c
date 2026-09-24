// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "SearchDecUnit.h"
#include "lib_common/Nuts.h"
#include "lib_common/Utils.h" // ARRAY_SIZE
#include "lib_common/SeiInternal.h"

#include "lib_common/AvcUtils.h"
#include "lib_common/HevcUtils.h"
#include "lib_rtos/lib_rtos.h"

/*****************************************************************************/
void AL_SearchDecUnit_Init(AL_TDecUnitSearchCtx* pCtx, AL_ECodec eCodec, AL_EDecUnit eDecUnit, AL_TNal* pNals, int32_t iMaxNal)
{
  pCtx->eCodec = eCodec;
  pCtx->bSubFrameUnit = (eDecUnit == AL_VCL_NAL_UNIT);

  pCtx->pNals = pNals;
  pCtx->iMaxNal = iMaxNal;

  pCtx->bEOS = false;

  AL_SearchDecUnit_SetStream(pCtx, NULL, 0);
  AL_SearchDecUnit_ResetNals(pCtx);
}

/*****************************************************************************/
void AL_SearchDecUnit_SetStream(AL_TDecUnitSearchCtx* pCtx, uint8_t const* pStream, uint32_t uSize)
{
  pCtx->pStream = pStream;
  pCtx->uStreamBufSize = uSize;
}

/*****************************************************************************/
int32_t AL_SearchDecUnit_GetCurrentNalCount(AL_TDecUnitSearchCtx const* pCtx)
{
  return pCtx->iNalCount;
}

/*****************************************************************************/
uint32_t AL_SearchDecUnit_GetFreeNalCount(AL_TDecUnitSearchCtx const* pCtx)
{
  return pCtx->iMaxNal - pCtx->iNalCount;
}

/*****************************************************************************/
void AL_SearchDecUnit_AddNals(AL_TDecUnitSearchCtx* pCtx, AL_TStartCode const* pNalSC, int32_t iNumNalSC, uint32_t uLastByte, bool bEOS)
{
  AL_TNal* dst = pCtx->pNals;

  if(pCtx->iNalCount && iNumNalSC)
    dst[pCtx->iNalCount - 1].uSize = DeltaPosition(dst[pCtx->iNalCount - 1].tStartCode.uPosition, pNalSC[0].uPosition, pCtx->uStreamBufSize);

  for(int32_t i = 0; i < iNumNalSC; i++)
  {
    dst[pCtx->iNalCount].tStartCode = pNalSC[i];

    if(pCtx->eCodec == AL_CODEC_HEVC)
    {
      dst[pCtx->iNalCount].tStartCode.uReserved.multilayer.uLayerID = (pCtx->pStream[pNalSC[i].uPosition + 3] & 1) << 5;
      dst[pCtx->iNalCount].tStartCode.uReserved.multilayer.uLayerID |= (pCtx->pStream[pNalSC[i].uPosition + 4] & 0xf8) >> 3;
    }

    if(i + 1 == iNumNalSC)
      dst[pCtx->iNalCount].uSize = DeltaPosition(pNalSC[i].uPosition, uLastByte, pCtx->uStreamBufSize);
    else
      dst[pCtx->iNalCount].uSize = DeltaPosition(pNalSC[i].uPosition, pNalSC[i + 1].uPosition, pCtx->uStreamBufSize);

    pCtx->iNalCount++;
  }

  pCtx->bEOS = bEOS;
}

/*****************************************************************************/
static bool isVcl(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AL_AVC_IsVcl(eNut);
  case AL_CODEC_HEVC:
    return AL_HEVC_IsVcl(eNut);
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isAud(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_AUD;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_AUD;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isEosOrEob(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return (eNut == AL_AVC_NUT_EOS) || (eNut == AL_AVC_NUT_EOB);
  case AL_CODEC_HEVC:
    return (eNut == AL_HEVC_NUT_EOS) || (eNut == AL_HEVC_NUT_EOB);
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isFd(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_FD;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_FD;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isPrefixSei(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_PREFIX_SEI;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_PREFIX_SEI;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool checkSeiUUID(uint8_t const* pBufs, AL_TNal const* pNal, AL_ECodec eCodec, int32_t iTotalSize)
{
  (void)eCodec;
  int32_t iTotalUUIDSize = 26;

  if(eCodec == AL_CODEC_AVC)
    iTotalUUIDSize = 25;

  if((int)pNal->uSize != iTotalUUIDSize)
    return false;

  int32_t iStart = 7;

  if(eCodec == AL_CODEC_AVC)
    iStart = 6;

  for(int32_t i = 0; i < UUID_SIZE; i++)
  {
    int32_t iPosition = (pNal->tStartCode.uPosition + iStart + i) % iTotalSize;

    if(ALLEGRO_NUM_SLICES_SEI_UUID[i] != pBufs[iPosition])
      return false;
  }

  return true;
}

/*****************************************************************************/
static int32_t getNumSliceInSei(uint8_t const* pBufs, AL_TNal* pNal, AL_ECodec eCodec, int32_t iTotalSize)
{
  (void)eCodec;
  Rtos_Assert(checkSeiUUID(pBufs, pNal, eCodec, iTotalSize));
  int32_t iStart = 7;

  if(eCodec == AL_CODEC_AVC)
    iStart = 6;
  int32_t const iSize = ARRAY_SIZE(ALLEGRO_NUM_SLICES_SEI_UUID);
  int32_t iPosition = (pNal->tStartCode.uPosition + iStart + iSize) % iTotalSize;
  return pBufs[iPosition];
}

/*****************************************************************************/
static bool isFirstSliceStatusAvailable(int32_t iSize, int32_t iNalHdrSize)
{
  return iSize > iNalHdrSize;
}

/******************************************************************************/
static int32_t NalHeaderSize(AL_ECodec eCodec)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AL_AVC_NAL_HDR_SIZE;
  case AL_CODEC_HEVC:
    return AL_HEVC_NAL_HDR_SIZE;
  default:
    return -1;
  }
}

/* should only be used when the position is right after the nal header */
/*****************************************************************************/
static bool isFirstSlice(uint8_t const* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return (pBuf[uPos] & 0x80) != 0;
}

/*****************************************************************************/
static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int32_t iNalHdrSize = NalHeaderSize(eCodec);
  Rtos_Assert(iNalHdrSize);
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
#define IS_START_CODE(pBuf, uSize, uPos) ( \
    ((pBuf)[(uPos) % (uSize)] == 0x00) && \
    ((pBuf)[((uPos) + 1) % (uSize)] == 0x00) && \
    ((pBuf)[((uPos) + 2) % (uSize)] == 0x01))

/*****************************************************************************/
static bool isFirstSliceNAL(AL_TDecUnitSearchCtx* pCtx, AL_TNal* pNal)
{
  uint8_t const* pBuf = pCtx->pStream;
  uint32_t uPos = pNal->tStartCode.uPosition;
  uint32_t uSize = pCtx->uStreamBufSize;

  Rtos_Assert(IS_START_CODE(pBuf, uSize, uPos));

  uPos = skipNalHeader(uPos, pCtx->eCodec, uSize);
  return isFirstSlice(pBuf, uPos);
}

/*****************************************************************************/
bool AL_SearchDecUnit_GetNextUnit(AL_TDecUnitSearchCtx* pCtx, int32_t* pNalCount, int32_t* pLastVclNalInAccessUnit)
{
  *pNalCount = 0;
  (void)pLastVclNalInAccessUnit;

  if(pCtx->iNalCount == 0)
    return false;

  bool bVclNalSeen = false;
  int32_t iNalFound = 0;

  for(int32_t iNal = 0; iNal < pCtx->iNalCount; ++iNal)
  {
    AL_TNal* pNal = &pCtx->pNals[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(iNal > 0)
      iNalFound++;

    if(!isVcl(pCtx->eCodec, eNUT))
    {
      if(isAud(pCtx->eCodec, eNUT) || isEosOrEob(pCtx->eCodec, eNUT))
      {
        if(bVclNalSeen)
        {
          *pNalCount = iNalFound;
          return true;
        }
      }

      if(isPrefixSei(pCtx->eCodec, eNUT) && checkSeiUUID(pCtx->pStream, pNal, pCtx->eCodec, pCtx->uStreamBufSize))
      {
        pCtx->iNumSlicesRemaining = getNumSliceInSei(pCtx->pStream, pNal, pCtx->eCodec, pCtx->uStreamBufSize);
        Rtos_Assert(pCtx->iNumSlicesRemaining > 0);
      }

      if(isFd(pCtx->eCodec, eNUT) && pCtx->bSubFrameUnit)
      {
        bool bIsLastSlice = pCtx->iNumSlicesRemaining == 1;

        if(bIsLastSlice && bVclNalSeen)
        {
          *pNalCount = iNalFound + 1;
          pCtx->iNumSlicesRemaining = 0;
          return true;
        }
      }

    }

    if(isVcl(pCtx->eCodec, eNUT))
    {
      int32_t iNalHdrSize = NalHeaderSize(pCtx->eCodec);
      Rtos_Assert(iNalHdrSize > 0);

      if(isFirstSliceStatusAvailable(pNal->uSize, iNalHdrSize))
      {
        bool bIsFirstSlice = isFirstSliceNAL(pCtx, pNal);

        if(bVclNalSeen)
        {
          if(bIsFirstSlice)
          {
            *pNalCount = iNalFound;
            return true;
          }

          if(pCtx->bSubFrameUnit)
          {
            pCtx->iNumSlicesRemaining--;
            *pNalCount = iNalFound;
            int32_t const iIsNotLastSlice = -1;
            *pLastVclNalInAccessUnit = iIsNotLastSlice;
            return true;
          }
        }
      }

      bVclNalSeen = true;
      *pLastVclNalInAccessUnit = iNal;
    }
  }

  if(pCtx->bEOS)
  {
    if(bVclNalSeen)
    {
      *pNalCount = pCtx->iNalCount;
      return true;
    }

    AL_SearchDecUnit_ConsumeNals(pCtx, pCtx->iNalCount);
  }

  return false;
}

/*****************************************************************************/
int32_t AL_SearchDecUnit_GetLastVCL(AL_TDecUnitSearchCtx const* pCtx)
{
  int32_t iLastVclNalInAU = -1;

  for(int32_t iNal = pCtx->iNalCount - 1; iNal >= 0; --iNal)
  {
    AL_ENut eNUT = pCtx->pNals[iNal].tStartCode.uNUT;

    if(isVcl(pCtx->eCodec, eNUT))
    {
      iLastVclNalInAU = iNal;
      break;
    }
  }

  return iLastVclNalInAU;
}

/*****************************************************************************/
int32_t AL_SearchDecUnit_GetCurrentStreamOffset(AL_TDecUnitSearchCtx const* pCtx)
{
  return pCtx->iCurNalStreamOffset;
}

/*****************************************************************************/
void AL_SearchDecUnit_ConsumeNals(AL_TDecUnitSearchCtx* pCtx, int32_t iNumNal)
{
  if(iNumNal == 0)
    return;

  pCtx->iCurNalStreamOffset = (pCtx->pNals[iNumNal - 1].tStartCode.uPosition + pCtx->pNals[iNumNal - 1].uSize) % pCtx->uStreamBufSize;
  pCtx->iNalCount -= iNumNal;
  Rtos_Memmove(pCtx->pNals, pCtx->pNals + iNumNal, pCtx->iNalCount * sizeof(AL_TNal));
}

/*****************************************************************************/
void AL_SearchDecUnit_ResetNals(AL_TDecUnitSearchCtx* pCtx)
{
  pCtx->iNalCount = 0;
  pCtx->iCurNalStreamOffset = 0;
}
