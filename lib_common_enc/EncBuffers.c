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

#include <string.h>
#include <assert.h>
#include "lib_common/Utils.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamBufferPrivate.h"

#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "lib_common_enc/EncSize.h"

#include "EncBuffersInternal.h"

#if USE_POWER_TWO_REF_PITCH
/****************************************************************************/
static int AL_RndUpPow2(int iVal)
{
  int iRnd = 1;

  while(iRnd < iVal)
    iRnd <<= 1;

  return iRnd;
}

#endif

/****************************************************************************/
uint32_t GetMaxLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uMaxCuSize)
{
  return ((uWidth + (1 << uMaxCuSize) - 1) >> uMaxCuSize)
         * ((uHeight + (1 << uMaxCuSize) - 1) >> uMaxCuSize);
}

/****************************************************************************/
uint32_t GetAllocSizeEP1()
{
  return (uint32_t)(EP1_BUF_LAMBDAS.Size + EP1_BUF_SCL_LST.Size);
}

/****************************************************************************/
uint32_t GetAllocSizeEP2(AL_TDimension tDim, uint8_t uMaxCuSize)
{
  uint32_t uMaxLCUs = 0, uMaxSize = 0;
  switch(uMaxCuSize)
  {
  case 6: // VP9
    uMaxLCUs = GetBlk64x64(tDim);
    uMaxSize = uMaxLCUs + 16;
    break;

  case 5: // HEVC
    uMaxLCUs = GetBlk32x32(tDim);
#if AL_BLK16X16_QP_TABLE
    uMaxSize = 8 * uMaxLCUs;
#else
    uMaxSize = uMaxLCUs;
#endif
    break;

  case 4: // AVC
    uMaxLCUs = GetBlk16x16(tDim);
    uMaxSize = uMaxLCUs;
    break;

  default:
    assert(0);
  }

  return (uint32_t)(EP2_BUF_QP_CTRL.Size + EP2_BUF_SEG_CTRL.Size) + RoundUp(uMaxSize, 128);
}

/****************************************************************************/
uint32_t GetAllocSizeEP3()
{
  uint32_t uHwRCSize = (uint32_t)(EP3_BUF_RC_TABLE1.Size + EP3_BUF_RC_TABLE2.Size + EP3_BUF_RC_CTX.Size + EP3_BUF_RC_LVL.Size);
  uint32_t uMaxSize = uHwRCSize * AL_ENC_NUM_CORES;

  return RoundUp(uMaxSize, 128);
}


static uint32_t ConsiderChromaForAllocSize(AL_EChromaMode eChromaMode, uint32_t uSize)
{
  switch(eChromaMode)
  {
  case CHROMA_MONO: break;
  case CHROMA_4_2_0: uSize += uSize >> 1;
    break;
  case CHROMA_4_2_2: uSize += uSize;
    break;
  case CHROMA_4_4_4:
  default: assert(0);
    break;
  }

  return uSize;
}

int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  int const iBurstAlignment = 32;
  return ComputeRndPitch(iWidth, uBitDepth, eStorageMode, iBurstAlignment);
}

/****************************************************************************/
uint32_t AL_GetAllocSize(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_EFbStorageMode eStorageMode)
{
  int const uPitch = AL_CalculatePitchValue(tDim.iWidth, uBitDepth, eStorageMode);
  int const uYSize = uPitch * tDim.iHeight / GetNumLinesInPitch(eStorageMode);
  return ConsiderChromaForAllocSize(eChromaMode, uYSize);
}


/****************************************************************************/
AL_EFbStorageMode AL_GetSrcStorageMode(AL_ESrcMode eSrcMode)
{
  switch(eSrcMode)
  {
  case AL_SRC_TILE_64x4:
  case AL_SRC_COMP_64x4:
    return AL_FB_TILE_64x4;
  case AL_SRC_TILE_32x4:
  case AL_SRC_COMP_32x4:
    return AL_FB_TILE_32x4;
  default:
    return AL_FB_RASTER;
  }
}

/****************************************************************************/
uint32_t GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt)
{

  AL_EFbStorageMode const eSrcStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  uint32_t uSize = AL_GetAllocSize(tDim, uBitDepth, eChromaMode, eSrcStorageMode);

  return uSize;
}

/****************************************************************************/
static uint32_t GetAllocSize_Ref(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bFbc)
{
  (void)bFbc;
  uint32_t uSize = 0;

  AL_TPicFormat const recFmt = { eChromaMode, uBitDepth, AL_FB_TILE_64x4 };
  TFourCC tRecFourCC = AL_GetRecFourCC(recFmt);
  const uint32_t uRoundedPixels = RoundUp(tDim.iWidth, 64) * RoundUp(tDim.iHeight, 64);

  if(tRecFourCC == FOURCC(T60A))
    uSize = (uRoundedPixels * 3 * 10) / (2 * 8);
  else if(tRecFourCC == FOURCC(T62A))
    uSize = (uRoundedPixels * 2 * 10) / 8;
  else if(tRecFourCC == FOURCC(T608))
    uSize = (uRoundedPixels * 3) / 2;
  else if(tRecFourCC == FOURCC(T628))
    uSize = (uRoundedPixels * 2);
  else if(tRecFourCC == FOURCC(T6m8))
    uSize = uRoundedPixels;
  else if(tRecFourCC == FOURCC(T6mA))
    uSize = (uRoundedPixels * 10) / 8;
  else
    assert(0);


  return uSize;
}

#if USE_POWER_TWO_REF_PITCH
/****************************************************************************/
static uint32_t GetAllocSize_RefPitch(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, int iPitch, bool bFbc)
{
  (void)bFbc;
  int iSizePix = GetPixelSize(uBitDepth);

  uint32_t uRounding = 64;
  uint32_t uRndHeight = RoundUp(iHeight, uRounding);

  assert(iPitch >= RoundUp(iWidth, uRounding));

  uint32_t uSize = iPitch * uRndHeight;
  switch(eChromaMode)
  {
  case CHROMA_MONO:
    break;

  case CHROMA_4_2_0:
    uSize += uSize >> 1;
    break;

  case CHROMA_4_2_2:
    uSize += uSize;
    break;

  case CHROMA_4_4_4:
  default:
    assert(0);
    break;
  }

  uSize *= iSizePix;


  return uSize;
}

#endif

/****************************************************************************/
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_EChEncOption eEncOption)
{
  (void)eEncOption;
  bool bComp = false;

#if USE_POWER_TWO_REF_PITCH
  return GetAllocSize_RefPitch(tDim, uBitDepth, eChromaMode, AL_RndUpPow2(iWidth), bComp);
#else
  return GetAllocSize_Ref(tDim, uBitDepth, eChromaMode, bComp);
#endif
}

/****************************************************************************/
uint32_t GetAllocSize_CompData(AL_TDimension tDim)
{
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return GetCompLcuSize() * uBlk16x16;
}

/****************************************************************************/
uint32_t GetAllocSize_CompMap(AL_TDimension tDim)
{
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return RoundUp(SIZE_LCU_INFO * uBlk16x16, 32);
}

/*****************************************************************************/
uint32_t GetAllocSize_MV(AL_TDimension tDim, uint8_t uLCUSize, AL_ECodec Codec)
{
  uint32_t uNumBlk = 0;
  int iMul = (Codec == AL_CODEC_HEVC) ? 1 :
             2;
  switch(uLCUSize)
  {
  case 4: uNumBlk = GetBlk16x16(tDim);
    break;
  case 5: uNumBlk = GetBlk32x32(tDim) << 2;
    break;
  case 6: uNumBlk = GetBlk64x64(tDim) << 4;
    break;
  default: assert(0);
  }

  return MVBUFF_MV_OFFSET + ((uNumBlk * 4 * sizeof(uint32_t)) * iMul);
}

/*****************************************************************************/
uint32_t GetAllocSize_WPP(int iLCUHeight, int iNumSlices, uint8_t uNumCore)
{
  uint32_t uNumLinesPerCmd = (((iLCUHeight + iNumSlices - 1) / iNumSlices) + uNumCore - 1) / uNumCore;
  uint32_t uAlignedSize = RoundUp(uNumLinesPerCmd * sizeof(uint32_t), 128) * uNumCore * iNumSlices;
  return uAlignedSize;
}

uint32_t GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uMaxCuSize)
{
  int iWidthInLcu = (uWidth + ((1 << uMaxCuSize) - 1)) >> uMaxCuSize;
  int iHeightInLcu = (uHeight + ((1 << uMaxCuSize) - 1)) >> uMaxCuSize;
  uint32_t uSize = (uint32_t)Max(iWidthInLcu * iHeightInLcu * 32, iWidthInLcu * iHeightInLcu * sizeof(uint32_t) + uNumSlices * AL_ENC_NUM_CORES * 128);
  uint32_t uAlignedSize = RoundUp(uSize, 32);
  return uAlignedSize;
}

/*!@}*/

