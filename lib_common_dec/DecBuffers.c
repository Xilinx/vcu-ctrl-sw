/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include <assert.h>
#include "lib_common_dec/DecBuffers.h"
#include "lib_common/Utils.h"
#include "lib_common/BufferPixMapMeta.h"

#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamBufferPrivate.h"

/*****************************************************************************/
int32_t RndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  int const iBurstAlignment = eFrameBufferStorageMode == AL_FB_RASTER ? 256 : HW_IP_BURST_ALIGNMENT;
  int const iRndWidth = RoundUp(iWidth, 64);
  return ComputeRndPitch(iRndWidth, uBitDepth, eFrameBufferStorageMode, iBurstAlignment);
}

/******************************************************************************/
int32_t RndHeight(int32_t iHeight)
{
  int const iAlignment = 64;
  return RoundUp(iHeight, iAlignment);
}

/****************************************************************************/
int AL_GetNumLCU(AL_TDimension tDim, uint8_t uLCUSize)
{
  switch(uLCUSize)
  {
  case 4: return GetBlk16x16(tDim);
  case 5: return GetBlk32x32(tDim);
  case 6: return GetBlk64x64(tDim);
  default: assert(0);
  }

  return 0;
}

/****************************************************************************/
int AL_GetAllocSize_HevcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int iBlk16x16 = GetBlk64x64(tDim) * 16;
  return HEVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int AL_GetAllocSize_AvcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int iBlk16x16 = GetBlk16x16(tDim);
  return AVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int AL_GetAllocSize_DecCompMap(AL_TDimension tDim)
{
  int iBlk16x16 = GetBlk16x16(tDim);
  return SIZE_LCU_INFO * iBlk16x16;
}

/*****************************************************************************/
int AL_GetAllocSize_HevcMV(AL_TDimension tDim)
{
  int iNumBlk = GetBlk64x64(tDim) * 16;
  return 4 * iNumBlk * sizeof(int32_t);
}

/*****************************************************************************/
int AL_GetAllocSize_AvcMV(AL_TDimension tDim)
{
  int iNumBlk = GetBlk16x16(tDim);
  return 16 * iNumBlk * sizeof(int32_t);
}

/****************************************************************************/
static uint32_t GetChromaAllocSize(AL_EChromaMode eChromaMode, uint32_t uAllocSizeY)
{
  switch(eChromaMode)
  {
  case AL_CHROMA_MONO:
    return 0;
  case AL_CHROMA_4_2_0:
    return uAllocSizeY >> 1;
  case AL_CHROMA_4_2_2:
    return uAllocSizeY;
  default:
    assert(0);
    break;
  }

  return 0;
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame_Y(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch)
{
  return iPitch * RndHeight(tDim.iHeight) / AL_GetNumLinesInPitch(eFbStorage);
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame_UV(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode)
{
  return GetChromaAllocSize(eChromaMode, AL_DecGetAllocSize_Frame_Y(eFbStorage, tDim, iPitch));
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame(AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  (void)bFbCompression;

  uint32_t uAllocSizeY = AL_DecGetAllocSize_Frame_Y(eFbStorageMode, tDim, iPitch);
  uint32_t uSize = uAllocSizeY + GetChromaAllocSize(eChromaMode, uAllocSizeY);

  return uSize;
}

/*****************************************************************************/
int AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  int iPitch = RndPitch(tDim.iWidth, uBitDepth, eFbStorageMode);
  return AL_DecGetAllocSize_Frame(tDim, iPitch, eChromaMode, bFbCompression, eFbStorageMode);
}

/*****************************************************************************/
AL_TMetaData* AL_CreateRecBufMetaData(AL_TDimension tDim, int iMinPitch, TFourCC tFourCC)
{
  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);
  AL_TPlane tPlaneY = { 0, 0, iMinPitch };
  int iOffsetC = AL_DecGetAllocSize_Frame_Y(eStorageMode, tDim, iMinPitch);
  AL_TPlane tPlaneUV = { 0, iOffsetC, iMinPitch };
  AL_TPixMapMetaData* pSrcMeta = AL_PixMapMetaData_Create(tDim, tPlaneY, tPlaneUV, tFourCC);

  return (AL_TMetaData*)pSrcMeta;
}

/*!@}*/

