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

#include "Utils.h"

#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferPixMapMeta.h"

#include "lib_assert/al_assert.h"

/*****************************************************************************/
int AL_GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode)
{
  switch(eFrameBufferStorageMode)
  {
  case AL_FB_RASTER: return 1;
  case AL_FB_TILE_32x4:
  case AL_FB_TILE_64x4: return 4;
  default:
  {
    AL_Assert(false);
    return 0;
  }
  }
}

/******************************************************************************/
static inline int GetWidthRound(AL_EFbStorageMode eStorageMode)
{
  switch(eStorageMode)
  {
  case AL_FB_RASTER: return 1;
  case AL_FB_TILE_64x4: return 64;
  case AL_FB_TILE_32x4: return 32;
  default:
  {
    AL_Assert(false);
    return 0;
  }
  }
}

/******************************************************************************/
int32_t ComputeRndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode, int iBurstAlignment)
{
  int32_t iVal = 0;
  int const iRndWidth = RoundUp(iWidth, GetWidthRound(eFrameBufferStorageMode));
  switch(eFrameBufferStorageMode)
  {
  case AL_FB_RASTER:
  {
    if(uBitDepth == 8)
      iVal = iRndWidth;
    else
    {
      iVal = (iRndWidth + 2) / 3 * 4;
    }
    break;
  }
  case AL_FB_TILE_32x4:
  case AL_FB_TILE_64x4:
  {
    uBitDepth = (uBitDepth + 1) & 0xFE; // Prevent 9 and 11 bitdepth -> 10/12
    iVal = iRndWidth * AL_GetNumLinesInPitch(eFrameBufferStorageMode) * uBitDepth / 8;
    break;
  }
  default:
    AL_Assert(false);
  }

  AL_Assert(iBurstAlignment > 0);
  AL_Assert((iBurstAlignment % HW_IP_BURST_ALIGNMENT) == 0);
  return RoundUp(iVal, iBurstAlignment);
}

/****************************************************************************/
int AL_GetChromaPitch(TFourCC tFourCC, int iLumaPitch)
{
  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
    return -1;

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return 0;

  int iNumPlanes = tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR ? 2 : 1;
  int iChromaPitch = iLumaPitch;

  if(tPicFormat.eChromaMode != AL_CHROMA_4_4_4)
  {
    int iRound = tPicFormat.uBitDepth > 8 ? 4 : 2;
    iChromaPitch = RoundUp(iLumaPitch, iRound) / 2;
  }

  return iChromaPitch * iNumPlanes;
}

/****************************************************************************/
int AL_GetChromaWidth(TFourCC tFourCC, int iLumaWidth)
{
  AL_TPicFormat tPicFormat;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
    return -1;

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO)
    return 0;

  int iNumPlanes = tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR ? 2 : 1;
  int iHrzScale = tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;
  return ((iLumaWidth + iHrzScale - 1) / iHrzScale) * iNumPlanes;
}

/****************************************************************************/
int AL_GetChromaHeight(TFourCC tFourCC, int iLumaHeight)
{
  AL_EChromaMode eChromaMode = AL_GetChromaMode(tFourCC);

  if(eChromaMode == AL_CHROMA_MAX_ENUM)
    return -1;

  if(eChromaMode == AL_CHROMA_MONO)
    return 0;

  return eChromaMode == AL_CHROMA_4_2_0 ? (iLumaHeight + 1) / 2 : iLumaHeight;
}

/****************************************************************************/
int AL_CLEAN_BUFFERS = 0;

void AL_CleanupMemory(void* pDst, size_t uSize)
{
  if(AL_CLEAN_BUFFERS)
  {
    Rtos_Memset(pDst, 0, uSize);
#ifdef COMPILE_FOR_MCU
    Rtos_FlushCacheMemory(pDst, uSize);
#endif
  }
}

