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

/****************************************************************************/
uint32_t GetMaxLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uMaxCuSize)
{
  return ((uWidth + (1 << uMaxCuSize) - 1) >> uMaxCuSize)
         * ((uHeight + (1 << uMaxCuSize) - 1) >> uMaxCuSize);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP1()
{
  uint32_t uEP1Size = EP1_BUF_LAMBDAS.Size + EP1_BUF_SCL_LST.Size;


  return uEP1Size;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP2(AL_TDimension tDim, uint8_t uMaxCuSize)
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
uint32_t AL_GetAllocSizeEP3()
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

/* Will be removed in 0.9 */
int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  return AL_EncGetMinPitch(iWidth, uBitDepth, eStorageMode);
}

int AL_EncGetMinPitch(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  int const iBurstAlignment = 32;
  return ComputeRndPitch(iWidth, uBitDepth, eStorageMode, iBurstAlignment);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrcNoFbc(AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_EFbStorageMode eStorageMode, int iPitch, int iChromaOffset)
{
  (void)tDim;
  (void)eStorageMode;
  (void)iPitch;
  return ConsiderChromaForAllocSize(eChromaMode, iChromaOffset);
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
/* Deprecated. Will be remove in 0.9 */
uint32_t AL_GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt)
{
  AL_EFbStorageMode const eSrcStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  int const iPitch = AL_EncGetMinPitch(tDim.iWidth, uBitDepth, eSrcStorageMode);
  return AL_GetAllocSizeSrc(tDim, uBitDepth, eChromaMode, eSrcFmt, iPitch, tDim.iHeight);
}

uint32_t AL_GetAllocSizeSrc(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight)
{
  (void)uBitDepth;

  AL_EFbStorageMode const eSrcStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  uint32_t uSize = AL_GetAllocSizeSrcNoFbc(tDim, eChromaMode, eSrcStorageMode, iPitch, iStrideHeight * iPitch / AL_GetNumLinesInPitch(eSrcStorageMode));

  return uSize;
}

/****************************************************************************/
static uint32_t GetAllocSize_Ref(AL_TDimension tRoundedDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bFbc)
{
  (void)bFbc;
  uint32_t uSize = tRoundedDim.iWidth * tRoundedDim.iHeight;
  uint32_t uSizeDiv = 1;
  switch(eChromaMode)
  {
  case CHROMA_MONO:
    break;
  case CHROMA_4_2_0:
    uSize *= 3;
    uSizeDiv *= 2;
    break;
  case CHROMA_4_2_2:
    uSize *= 2;
    break;
  default:
    assert(0);
  }

  if(uBitDepth > 8)
  {
    uSize *= 10;
    uSizeDiv *= 8;
  }

  uSize /= uSizeDiv;


  return uSize;
}

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
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_EChEncOption eEncOption)
{
  (void)eEncOption;
  bool bComp = false;

  AL_TDimension RoundedDim;
  RoundedDim.iHeight = RoundUp(tDim.iHeight, 64);
  RoundedDim.iWidth = RoundUp(tDim.iWidth, 64);
#if USE_POWER_TWO_REF_PITCH
  RoundedDim.iWidth = AL_RndUpPow2(tDim.iWidth);
#endif
  return GetAllocSize_Ref(RoundedDim, uBitDepth, eChromaMode, bComp);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_CompData(AL_TDimension tDim, uint8_t uLCUSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt)
{
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return GetCompLcuSize(uLCUSize, uBitDepth, eChromaMode, bUseEnt) * uBlk16x16;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncCompMap(AL_TDimension tDim, uint8_t uLCUSize, uint8_t uNumCore, bool bUseEnt)
{
  (void)uLCUSize, (void)uNumCore, (void)bUseEnt;
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return RoundUp(SIZE_LCU_INFO * uBlk16x16, 32);
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_MV(AL_TDimension tDim, uint8_t uLCUSize, AL_ECodec Codec)
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
uint32_t AL_GetAllocSize_WPP(int iLCUHeight, int iNumSlices, uint8_t uNumCore)
{
  uint32_t uNumLinesPerCmd = (((iLCUHeight + iNumSlices - 1) / iNumSlices) + uNumCore - 1) / uNumCore;
  uint32_t uAlignedSize = RoundUp(uNumLinesPerCmd * sizeof(uint32_t), 128) * uNumCore * iNumSlices;
  return uAlignedSize;
}

uint32_t AL_GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uMaxCuSize)
{
  int iWidthInLcu = (uWidth + ((1 << uMaxCuSize) - 1)) >> uMaxCuSize;
  int iHeightInLcu = (uHeight + ((1 << uMaxCuSize) - 1)) >> uMaxCuSize;
  uint32_t uSize = (uint32_t)Max(iWidthInLcu * iHeightInLcu * 32, iWidthInLcu * iHeightInLcu * sizeof(uint32_t) + uNumSlices * AL_ENC_NUM_CORES * 128);
  uint32_t uAlignedSize = RoundUp(uSize, 32);
  return uAlignedSize;
}


/*!@}*/

