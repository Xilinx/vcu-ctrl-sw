/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "EncBuffersInternal.h"

#include <string.h>
#include "lib_common/Utils.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamBufferPrivate.h"

#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "lib_common_enc/EncSize.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/
uint32_t AL_GetAllocSizeEP1()
{
  uint32_t uEP1Size = EP1_BUF_LAMBDAS.Size + EP1_BUF_SCL_LST.Size;

  return uEP1Size;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP2(AL_TDimension tDim, AL_ECodec eCodec)
{
  uint32_t uMaxSize = 0;
  switch(eCodec)
  {
  case AL_CODEC_HEVC:
  {
    int iMaxLCUs = GetBlk32x32(tDim);
#if AL_BLK16X16_QP_TABLE
    iMaxLCUs *= 8;
#endif
    uMaxSize = iMaxLCUs;
    break;
  }
  case AL_CODEC_AVC:
  {
    int iMaxLCUs = GetBlk16x16(tDim);
    uMaxSize = iMaxLCUs;
    break;
  }
  default:
    AL_Assert(0);
  }

  return (uint32_t)(EP2_BUF_QP_CTRL.Size + EP2_BUF_SEG_CTRL.Size) + RoundUp(uMaxSize, 128);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP3PerCore()
{
  return (uint32_t)(EP3_BUF_RC_TABLE1.Size + EP3_BUF_RC_TABLE2.Size + EP3_BUF_RC_CTX.Size + EP3_BUF_RC_LVL.Size);
  ;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP3()
{
  uint32_t uMaxSize = AL_GetAllocSizeEP3PerCore() * AL_ENC_NUM_CORES;
  return RoundUp(uMaxSize, 128);
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
  default: AL_Assert(0);
  }

  return 0;
}

/* Will be removed in 0.9 */
int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  return AL_EncGetMinPitch(iWidth, uBitDepth, eStorageMode);
}

int AL_EncGetMinPitch(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  return ComputeRndPitch(iWidth, uBitDepth, eStorageMode, HW_IP_BURST_ALIGNMENT);
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
bool AL_IsSrcCompressed(AL_ESrcMode eSrcMode)
{
  (void)eSrcMode;
  bool bCompressed = false;
  return bCompressed;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_PixPlane(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight, AL_EChromaMode eChromaMode, AL_EPlaneId ePlaneId)
{
  AL_EChromaOrder eChromaOrder = eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA :
                                 (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR);

  if(!AL_Plane_Exists(eChromaOrder, false, ePlaneId))
    return 0;

  AL_EFbStorageMode const eSrcStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  int iSize = iStrideHeight * iPitch / AL_GetNumLinesInPitch(eSrcStorageMode);

  if(ePlaneId == AL_PLANE_UV)
    iSize = GetChromaAllocSize(eChromaMode, iSize);

  return iSize;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_Y(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight)
{
  return AL_GetAllocSizeSrc_PixPlane(eSrcFmt, iPitch, iStrideHeight, AL_CHROMA_MONO, AL_PLANE_Y);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_UV(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight, AL_EChromaMode eChromaMode)
{
  return AL_GetAllocSizeSrc_PixPlane(eSrcFmt, iPitch, iStrideHeight, eChromaMode, AL_PLANE_UV);
}

/****************************************************************************/
/* Deprecated. Will be remove in 0.9 */
uint32_t AL_GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt)
{
  AL_EFbStorageMode const eSrcStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  int const iPitch = AL_EncGetMinPitch(tDim.iWidth, uBitDepth, eSrcStorageMode);
  return AL_GetAllocSizeSrc(tDim, eChromaMode, eSrcFmt, iPitch, tDim.iHeight);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc(AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight)
{
  (void)tDim;

  uint32_t uAllocSizeY = AL_GetAllocSizeSrc_PixPlane(eSrcFmt, iPitch, iStrideHeight, AL_CHROMA_MONO, AL_PLANE_Y);
  uint32_t uSize = uAllocSizeY + GetChromaAllocSize(eChromaMode, uAllocSizeY);

  return uSize;
}

/****************************************************************************/
static uint32_t GetRasterFrameSize(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode)
{
  uint32_t uSize = tDim.iWidth * tDim.iHeight;
  uint32_t uSizeDiv = 1;
  switch(eChromaMode)
  {
  case AL_CHROMA_MONO: break;
  case AL_CHROMA_4_2_0:
  {
    uSize *= 3;
    uSizeDiv *= 2;
    break;
  }
  case AL_CHROMA_4_2_2:
  {
    uSize *= 2;
    break;
  }
  case AL_CHROMA_4_4_4:
  {
    uSize *= 3;
    break;
  }
  default:
    AL_Assert(0);
  }

  if(uBitDepth > 8)
  {
    assert((uBitDepth <= 12) && "Support bitpdeth > 12");
    uSize *= uBitDepth;
    uSizeDiv *= 8;
  }

  return uSize / uSizeDiv;
}

/****************************************************************************/
static uint32_t GetAllocSize_Ref(AL_TDimension tRoundedDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, uint16_t uMVVRange, uint8_t uLCUSize, AL_EChEncOption eOptions)
{
  (void)eOptions, (void)uMVVRange, (void)uLCUSize;
  AL_TDimension tDim = tRoundedDim;

  uint32_t uSize = GetRasterFrameSize(tDim, uBitDepth, eChromaMode);

  return uSize;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, uint8_t uBitDepth, uint8_t uLCUSize, AL_EChromaMode eChromaMode, AL_EChEncOption eOptions, uint16_t uMVVRange)
{
  (void)uMVVRange, (void)uLCUSize;

  AL_TDimension RoundedDim;
  RoundedDim.iHeight = RoundUp(tDim.iHeight, 64);
  RoundedDim.iWidth = RoundUp(tDim.iWidth, 64);
  return GetAllocSize_Ref(RoundedDim, uBitDepth, eChromaMode, uMVVRange, uLCUSize, eOptions);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_CompData(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt)
{
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return AL_GetCompDataSize(uBlk16x16, uLog2MaxCuSize, uBitDepth, eChromaMode, bUseEnt);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncCompMap(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uNumCore, bool bUseEnt)
{
  (void)uLog2MaxCuSize, (void)uNumCore, (void)bUseEnt;
  uint32_t uBlk16x16 = GetBlk16x16(tDim);
  return RoundUp(SIZE_LCU_INFO * uBlk16x16, 32);
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_MV(AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec Codec)
{
  uint32_t uNumBlk = 0;
  int iMul = (Codec == AL_CODEC_HEVC) ? 1 :
             2;
  switch(uLog2MaxCuSize)
  {
  case 4: uNumBlk = GetBlk16x16(tDim);
    break;
  case 5: uNumBlk = GetBlk32x32(tDim) << 2;
    break;
  case 6: uNumBlk = GetBlk64x64(tDim) << 4;
    break;
  default: AL_Assert(0);
  }

  return MVBUFF_MV_OFFSET + ((uNumBlk * 4 * sizeof(uint32_t)) * iMul);
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_WPP(int iLCUPicHeight, int iNumSlices, uint8_t uNumCore)
{
  uint32_t uNumLinesPerCmd = (((iLCUPicHeight + iNumSlices - 1) / iNumSlices) + uNumCore - 1) / uNumCore;
  uint32_t uAlignedSize = RoundUp(uNumLinesPerCmd * sizeof(uint32_t), 128) * uNumCore * iNumSlices;
  return uAlignedSize;
}

uint32_t AL_GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uLog2MaxCuSize)
{
  int iWidthInLcu = (uWidth + ((1 << uLog2MaxCuSize) - 1)) >> uLog2MaxCuSize;
  int iHeightInLcu = (uHeight + ((1 << uLog2MaxCuSize) - 1)) >> uLog2MaxCuSize;
  uint32_t uSize = (uint32_t)Max(iWidthInLcu * iHeightInLcu * 32, iWidthInLcu * iHeightInLcu * sizeof(uint32_t) + uNumSlices * AL_ENC_NUM_CORES * 128);
  uint32_t uAlignedSize = RoundUp(uSize, 32);
  return uAlignedSize;
}

/*!@}*/

