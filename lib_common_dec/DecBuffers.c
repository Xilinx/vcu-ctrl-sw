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
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include <assert.h>
#include "lib_common_dec/DecBuffers.h"
#include "lib_common/Utils.h"


/****************************************************************************/
static uint32_t GetBlk64x64(uint16_t uWidth, uint16_t uHeight)
{
  uint32_t u64x64Width = (uWidth + 63) >> 6;
  uint32_t u64x64Height = (uHeight + 63) >> 6;

  return u64x64Width * u64x64Height;
}

/****************************************************************************/
static uint32_t GetBlk32x32(uint16_t uWidth, uint16_t uHeight)
{
  uint32_t u32x32Width = (uWidth + 31) >> 5;
  uint32_t u32x32Height = (uHeight + 31) >> 5;

  return u32x32Width * u32x32Height;
}

/****************************************************************************/
static uint32_t GetBlk16x16(uint16_t uWidth, uint16_t uHeight)
{
  uint32_t u16x16Width = (uWidth + 15) >> 4;
  uint32_t u16x16Height = (uHeight + 15) >> 4;

  return u16x16Width * u16x16Height;
}

/****************************************************************************/
uint32_t RndUpPow2(uint32_t uVal)
{
  uint32_t uRnd = 1;

  while(uRnd < uVal)
    uRnd <<= 1;

  return uRnd;
}

/******************************************************************************/
uint32_t RndPitch(uint32_t uWidth, uint8_t uBitDepth)
{
  uint32_t uVal;

  if(AL_DEC_RASTER_3x10B_ON_32B)
    uVal = (uBitDepth == 8) ? uWidth : (uWidth + 2) / 3 * 4;
  else
    uVal = (uBitDepth == 8) ? uWidth : uWidth * 2;
  return (uVal + (AL_ALIGN_PITCH - 1)) & ~(AL_ALIGN_PITCH - 1);
}

/******************************************************************************/
uint32_t RndHeight(uint32_t uHeight)
{
  return (uHeight + (AL_ALIGN_HEIGHT - 1)) & ~(AL_ALIGN_HEIGHT - 1);
}

/****************************************************************************/
uint32_t AL_GetNumLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uLCUSize)
{
  switch(uLCUSize)
  {
  case 4: return GetBlk16x16(uWidth, uHeight);
  case 5: return GetBlk32x32(uWidth, uHeight);
  case 6: return GetBlk64x64(uWidth, uHeight);
  default: assert(0);
  }

  return 0;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_HevcCompData(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode)
{
  uint32_t uBlk16x16 = GetBlk64x64(uWidth, uHeight) * 16;
  return HEVC_LCU_CMP_SIZE[eChromaMode] * uBlk16x16;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_AvcCompData(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode)
{
  uint32_t uBlk16x16 = GetBlk16x16(uWidth, uHeight);
  return AVC_LCU_CMP_SIZE[eChromaMode] * uBlk16x16;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_CompMap(uint16_t uWidth, uint16_t uHeight)
{
  uint32_t uBlk16x16 = GetBlk16x16(uWidth, uHeight);
  return SIZE_LCU_INFO * uBlk16x16;
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_MV(uint16_t uWidth, uint16_t uHeight, bool bIsAvc)
{
  uint32_t uNumBlk = bIsAvc ? GetBlk16x16(uWidth, uHeight) : GetBlk64x64(uWidth, uHeight) << 4;

  return bIsAvc ? (16 * uNumBlk * sizeof(uint32_t)) : (4 * uNumBlk * sizeof(uint32_t));
}

/*****************************************************************************/
int AL_GetAllocSize_Frame(AL_TDimension tDimension, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFrameBufferStorageMode)
{
  (void)bFrameBufferCompression;
  (void)eFrameBufferStorageMode;
  int iTotalSize = AL_GetAllocSize_Reference(tDimension.iWidth, tDimension.iHeight, eChromaMode, uBitDepth);
  return iTotalSize;
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_Reference(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode, uint8_t uBitDepth)
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
uint32_t AL_GetAllocSize_LumaMap(uint16_t uWidth, uint16_t uHeight, AL_EFbStorageMode eFBStorageMode)
{
  if(eFBStorageMode == AL_FB_RASTER)
    return 0;
  assert(eFBStorageMode == AL_FB_TILE_32x4 || eFBStorageMode == AL_FB_TILE_64x4);
  uint32_t uTileWidth = eFBStorageMode == AL_FB_TILE_32x4 ? 32 : 64;
  uint32_t uTileHeight = 4;
  return (4096 / (2 * uTileWidth)) * ((uWidth + 4095) >> 12) * (uHeight / uTileHeight);
}

/*!@}*/

