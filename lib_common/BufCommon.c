// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Utils.h"

#include "include/config.h"
#include "include/lib_common/PicFormat.h"
#include "include/lib_rtos/assert.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common/BufferPixMapMeta.h"
#include "lib_common/Round.h"

/*****************************************************************************/
int32_t AL_GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode, AL_EChromaMode eChromaMode, uint8_t uBitDepth)
{
  (void)eChromaMode;
  (void)uBitDepth;

  if(eFrameBufferStorageMode == AL_FB_RASTER)
  {
    return 1;
  }

  int32_t iVal = 4;

  return iVal;
}

/******************************************************************************/
static inline int32_t GetWidthRound(AL_EFbStorageMode eStorageMode, uint8_t uBitDepth, AL_EChromaMode eChromaMode)
{
  (void)uBitDepth;
  (void)eChromaMode;
  switch(eStorageMode)
  {
  case AL_FB_RASTER: return 1;
  case AL_FB_TILE_64x4: return 64;
  case AL_FB_TILE_32x4: return 32;
  default:
  {
    if(IsTile(eStorageMode))
    {
      return GetTileWidth(eStorageMode, uBitDepth);
    }

    Rtos_Assert(false);
    return 0;
  }
  }
}

/******************************************************************************/
int32_t AL_GetLumaPixPlanePitch(int32_t iWidth, AL_TPicFormat const* pPicFormat, int32_t iPitchAlignment)
{
  int32_t iVal = 0;
  int32_t const iRndWidth = AL_RoundUp(iWidth, GetWidthRound(pPicFormat->eStorageMode, pPicFormat->uBitDepth, pPicFormat->eChromaMode));

  if(IsTile(pPicFormat->eStorageMode))
  {
    uint8_t uBitDepth = (pPicFormat->uBitDepth + 1) & 0xFE; // Prevent 9 and 11 bitdepth -> 10/12

    iVal = iRndWidth * AL_GetNumLinesInPitch(pPicFormat->eStorageMode, pPicFormat->eChromaMode, uBitDepth) * uBitDepth / 8;

  }
  else
  {
    if(pPicFormat->ePlaneMode == AL_PLANE_MODE_INTERLEAVED && pPicFormat->eChromaMode != AL_CHROMA_4_0_0)
    {
      int32_t iPixSize = sizeof(uint32_t);
      int32_t iHorizontalScale = pPicFormat->eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;

      bool bHasAlpha = pPicFormat->eAlphaMode == AL_ALPHA_MODE_BEFORE || pPicFormat->eAlphaMode == AL_ALPHA_MODE_AFTER;

      /* This checks mainly AYUV and Y410 formats*/
      if((bHasAlpha && pPicFormat->uBitDepth == 8)
         || (pPicFormat->eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED && pPicFormat->uBitDepth == 10))
        iPixSize = sizeof(uint32_t);

      /* This checks mainly Y416 format*/
      if(pPicFormat->eSamplePackMode == AL_SAMPLE_PACK_MODE_BYTE && (pPicFormat->uBitDepth == 12 || pPicFormat->uBitDepth == 10))
        iPixSize = sizeof(AL_64U);

      iVal = iRndWidth * iPixSize / iHorizontalScale;
    }
    else
    {
      if(pPicFormat->uBitDepth == 8)
        iVal = iRndWidth;
      else
      {
        iVal = iRndWidth * 2;

        if(pPicFormat->eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED_XV)
          iVal = (iRndWidth + 2) / 3 * 4;
      }
    }
  }

  Rtos_Assert(iPitchAlignment > 0);
  return AL_RoundUp(iVal, iPitchAlignment);
}

/****************************************************************************/
int32_t AL_GetChromaPixPlanePitch(AL_TPicFormat const* pPicFormat, int32_t iLumaPitch)
{
  if(pPicFormat->eChromaMode == AL_CHROMA_MONO || pPicFormat->ePlaneMode == AL_PLANE_MODE_INTERLEAVED)
    return 0;

  int32_t iNumPlanes = pPicFormat->ePlaneMode == AL_PLANE_MODE_SEMIPLANAR ? 2 : 1;
  int32_t iChromaPitch = iLumaPitch;

  if(pPicFormat->eChromaMode != AL_CHROMA_4_4_4)
  {
    int32_t iRound = pPicFormat->uBitDepth > 8 ? 4 : 2;
    iChromaPitch = AL_RoundUp(iLumaPitch, iRound) / 2;
  }

  return iChromaPitch * iNumPlanes;
}

/****************************************************************************/
static uint32_t GetChromaAllocSize(AL_EChromaMode eChromaMode, uint32_t uAllocSizeY)
{
  switch(eChromaMode)
  {
  case AL_CHROMA_MONO: return 0;
  case AL_CHROMA_4_2_0: return uAllocSizeY / 2;
  case AL_CHROMA_4_2_2: return uAllocSizeY;
  case AL_CHROMA_4_4_4: return uAllocSizeY * 2;
  default: Rtos_Assert(false);
  }

  return 0;
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_Frame_PixPlane(AL_TPicFormat const* pPicFormat, AL_TPitch tPitchDim, AL_EPlaneId ePlaneId)
{
  if(!AL_Plane_Exists(pPicFormat->ePlaneMode, false, ePlaneId))
    return 0;

  int32_t iSize = tPitchDim.iWidth * tPitchDim.iHeight;

  if(ePlaneId == AL_PLANE_UV)
    iSize = GetChromaAllocSize(pPicFormat->eChromaMode, iSize);

  return iSize;
}

/****************************************************************************/

uint32_t AL_GetAllocSize_Frame_Full(AL_TDimension tDim, AL_TPicFormat const* pPicFormat, AL_TPitch tPitchDim, int32_t iRoundVal)
{
  (void)tDim;
  (void)iRoundVal;
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  uint32_t uSize = 0;
  int32_t iNbPlanes = AL_Plane_GetBufferPlanes(*pPicFormat, usedPlanes);

  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    uSize += AL_GetAllocSize_Frame_PixPlane(pPicFormat, tPitchDim, usedPlanes[iPlane]);
  }

  return uSize;
}

/****************************************************************************/
int32_t AL_GetChromaPitch(TFourCC tFourCC, int32_t iLumaPitch)
{
  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
    return -1;

  return AL_GetChromaPixPlanePitch(&tPicFormat, iLumaPitch);
}

/****************************************************************************/
int32_t AL_GetChromaWidth(TFourCC tFourCC, int32_t iLumaWidth)
{
  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
    return -1;

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return 0;

  int32_t iNumPlanes = tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR ? 2 : 1;
  int32_t iHrzScale = tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;
  return ((iLumaWidth + iHrzScale - 1) / iHrzScale) * iNumPlanes;
}

/****************************************************************************/
int32_t AL_GetChromaHeight(TFourCC tFourCC, int32_t iLumaHeight)
{
  AL_EChromaMode eChromaMode = AL_GetChromaMode(tFourCC);

  if(eChromaMode == AL_CHROMA_MAX_ENUM)
    return -1;

  if(eChromaMode == AL_CHROMA_MONO)
    return 0;

  return eChromaMode == AL_CHROMA_4_2_0 ? (iLumaHeight + 1) / 2 : iLumaHeight;
}

/****************************************************************************/
int32_t AL_CLEAN_BUFFERS = 0;

void AL_CleanupMemory(void* pDst, size_t uSize)
{
  if(!pDst)
    return;

  if(AL_CLEAN_BUFFERS)
  {
    Rtos_Memset(pDst, 0, uSize);
#ifdef COMPILE_FOR_MCU
    Rtos_FlushCacheMemory(pDst, uSize);
#endif
  }
}
