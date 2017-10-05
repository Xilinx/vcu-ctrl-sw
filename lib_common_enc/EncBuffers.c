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
#include "lib_common_enc/EncBuffers.h"
#include "EncBuffersInternal.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "lib_common/Utils.h"
#include "lib_common_enc/EncSize.h"

uint32_t RndUpRXx(uint32_t uVal)
{
  return AL_RndUpPow2(uVal);
}

bool IsRXxFourCC(TFourCC tFourCC)
{
  return (tFourCC == FOURCC(RX0A)) ||
         (tFourCC == FOURCC(RX2A)) ||
         (tFourCC == FOURCC(RX10));
}


/****************************************************************************/
static uint32_t GetBlk64x64(uint16_t uWidth, uint16_t uHeight)
{
  uint16_t u64x64Width = (uWidth + 63) >> 6;
  uint16_t u64x64Height = (uHeight + 63) >> 6;

  return u64x64Width * u64x64Height;
}

/****************************************************************************/
static uint32_t GetBlk32x32(uint16_t uWidth, uint16_t uHeight)
{
  uint16_t u32x32Width = (uWidth + 31) >> 5;
  uint16_t u32x32Height = (uHeight + 31) >> 5;

  return u32x32Width * u32x32Height;
}

/****************************************************************************/
static uint32_t GetBlk16x16(uint16_t uWidth, uint16_t uHeight)
{
  uint16_t u16x16Width = (uWidth + 15) >> 4;
  uint16_t u16x16Height = (uHeight + 15) >> 4;

  return u16x16Width * u16x16Height;
}

/****************************************************************************/
uint32_t GetMaxLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uMaxCuSize)
{
  return ((uWidth + (1 << uMaxCuSize) - 1) >> uMaxCuSize)
         * ((uHeight + (1 << uMaxCuSize) - 1) >> uMaxCuSize);
}

/****************************************************************************/
uint32_t GetMaxVclNalSize(int iWidth, int iHeight, AL_EChromaMode eMode)
{
  uint32_t uMaxPCM = GetBlk64x64(iWidth, iHeight) * AL_PCM_SIZE[eMode][2];
  return uMaxPCM;
}

/****************************************************************************/
uint32_t GetMaxNalSize(int iWidth, int iHeight, AL_EChromaMode eMode)
{
  uint32_t uMaxPCM = GetMaxVclNalSize(iWidth, iHeight, eMode);

  uMaxPCM += 2048 + (((iHeight + 15) / 16) * AL_MAX_SLICE_HEADER_SIZE); // 4096 for AUD/SPS/PPS/SEI/slice header ...

  return RoundUp(uMaxPCM, 32);
}

/****************************************************************************/
uint32_t GetAllocSizeEP1()
{
  return (uint32_t)(EP1_BUF_LAMBDAS.Size + EP1_BUF_SCL_LST.Size);
}

/****************************************************************************/
uint32_t GetAllocSizeEP2(int iWidth, int iHeight, uint8_t uMaxCuSize)
{
  uint32_t uMaxLCUs = 0, uMaxSize = 0;
  switch(uMaxCuSize)
  {
  case 6: // VP9
    uMaxLCUs = GetBlk64x64(iWidth, iHeight);
    uMaxSize = uMaxLCUs + 16;
    break;

  case 5: // HEVC
    uMaxLCUs = GetBlk32x32(iWidth, iHeight);
    uMaxSize = uMaxLCUs;
    break;

  case 4: // AVC
    uMaxLCUs = GetBlk16x16(iWidth, iHeight);
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

int CalculatePitchValue(TFourCC tFourCC, int iWidth)
{
  int iPitch;
  (void)tFourCC;

  if(IsRXxFourCC(tFourCC))
  {
    iPitch = (iWidth + 2) / 3 * 4;
    iPitch = AL_RndUpPow2(iPitch);
  }
  else
  iPitch = RoundUp(iWidth, 32); // pitch aligned on 256 bits
  return iPitch;
}

int CalculatePixSize(TFourCC tFourCC)
{

  if(IsRXxFourCC(tFourCC))
    return sizeof(uint8_t);
  else
  return GetPixelSize(AL_GetBitDepth(tFourCC));
}

/****************************************************************************/
static uint32_t CalculateAllocSize(TFourCC tFourCC, uint32_t uPitch, int iHeight, int iSizePix)
{
  uint32_t uSize;

  uSize = uPitch * iHeight;
  uSize = ConsiderChromaForAllocSize(AL_GetChromaMode(tFourCC), uSize);

  return uSize * iSizePix;
}

/****************************************************************************/
uint32_t GetAllocSize_Yuv(int iWidth, int iHeight, TFourCC tFourCC)
{
  int iSizePix;
  uint32_t uPitch;

  uPitch = CalculatePitchValue(tFourCC, iWidth);
  iSizePix = CalculatePixSize(tFourCC);
  return CalculateAllocSize(tFourCC, uPitch, iHeight, iSizePix);
}

/****************************************************************************/
static uint32_t GetAllocSize_Tile(int iWidth, int iHeight, uint8_t uBitDepth, AL_EChromaMode eChromaMode, int iTileW, int iTileH)
{
  int iRndWidth = RoundUp(iWidth, iTileW);
  int iRndHeight = RoundUp(iHeight, iTileH);

  int iSizeLuma = iRndWidth * iRndHeight * GetPixelSize(uBitDepth);

  int iHrzScale = (eChromaMode == CHROMA_4_4_4) ? 1 : 2;
  int iVrtScale = (eChromaMode == CHROMA_4_2_0) ? 2 : 1;

  if(eChromaMode)
    return iSizeLuma + (iSizeLuma / (iHrzScale * iVrtScale) * 2);
  else
    return iSizeLuma;
}


/****************************************************************************/
uint32_t GetAllocSize_Src(int iWidth, int iHeight, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcConvMode eSrcFmt)
{
  switch(eSrcFmt)
  {
  case AL_NVX:
  {
    TFourCC tFourCC = AL_GetSrcFourCC(eChromaMode, uBitDepth);
    return GetAllocSize_Yuv(iWidth, iHeight, tFourCC);
  }

  case AL_TILE_32x8:
    return GetAllocSize_Tile(iWidth, iHeight, uBitDepth, eChromaMode, 32, 8);


  default:
    return 0;
  }
}

/****************************************************************************/
uint32_t GetAllocSize_SrcPitch(int iWidth, int iHeight, uint8_t uBitDepth, AL_EChromaMode eChromaMode, int iPitch)
{
  TFourCC tFourCC = AL_GetSrcFourCC(eChromaMode, uBitDepth);

  assert(iPitch >= iWidth);
  return CalculateAllocSize(tFourCC, iPitch, iHeight, CalculatePixSize(tFourCC));
}

/****************************************************************************/
uint32_t GetAllocSize_Ref(int iWidth, int iHeight, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bFbc)
{
  (void)bFbc;
  uint32_t uSize = 0;

  TFourCC tRecFourCC = AL_GetRecFourCC(eChromaMode, uBitDepth);
  const uint32_t uRoundedPixels = RoundUp(iWidth, 64) * RoundUp(iHeight, 64);

  if(tRecFourCC == FOURCC(AL0A))
    uSize = (uRoundedPixels * 3 * 10) / (2 * 8);
  else if(tRecFourCC == FOURCC(AL2A))
    uSize = (uRoundedPixels * 2 * 10) / 8;
  else if(tRecFourCC == FOURCC(AL08))
    uSize = (uRoundedPixels * 3) / 2;
  else if(tRecFourCC == FOURCC(AL28))
    uSize = (uRoundedPixels * 2);
  else if(tRecFourCC == FOURCC(ALm8))
    uSize = uRoundedPixels;
  else if(tRecFourCC == FOURCC(ALmA))
    uSize = (uRoundedPixels * 10) / 8;
  else
    assert(0);


  return uSize;
}

/****************************************************************************/
uint32_t GetAllocSize_RefPitch(int iWidth, int iHeight, uint8_t uBitDepth, AL_EChromaMode eChromaMode, int iPitch, bool bFbc)
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

/****************************************************************************/
uint32_t GetAllocSize_CompData(int iWidth, int iHeight)
{
  uint32_t uBlk16x16 = GetBlk16x16(iWidth, iHeight);
  return SIZE_COMPRESS_LCU * uBlk16x16;
}

/****************************************************************************/
uint32_t GetAllocSize_CompMap(int iWidth, int iHeight)
{
  uint32_t uBlk16x16 = GetBlk16x16(iWidth, iHeight);
  return RoundUp(SIZE_LCU_INFO * uBlk16x16, 32);
}

/*****************************************************************************/
uint32_t GetAllocSize_MV(int iWidth, int iHeight, uint8_t uLCUSize, AL_ECodec Codec)
{
  uint32_t uNumBlk = 0;
  int iMul = (Codec == AL_CODEC_HEVC) ? 1 :
             2;
  switch(uLCUSize)
  {
  case 4: uNumBlk = GetBlk16x16(iWidth, iHeight);
    break;
  case 5: uNumBlk = GetBlk32x32(iWidth, iHeight) << 2;
    break;
  case 6: uNumBlk = GetBlk64x64(iWidth, iHeight) << 4;
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

