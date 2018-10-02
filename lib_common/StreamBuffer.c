/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamBufferPrivate.h"
#include "lib_common/AvcLevelsLimit.h"

/****************************************************************************/
int GetBlk64x64(AL_TDimension tDim)
{
  int i64x64Width = (tDim.iWidth + 63) / 64;
  int i64x64Height = (tDim.iHeight + 63) / 64;

  return i64x64Width * i64x64Height;
}

/****************************************************************************/
int GetBlk32x32(AL_TDimension tDim)
{
  int i32x32Width = (tDim.iWidth + 31) / 32;
  int i32x32Height = (tDim.iHeight + 31) / 32;

  return i32x32Width * i32x32Height;
}

/****************************************************************************/
int GetBlk16x16(AL_TDimension tDim)
{
  int i16x16Width = (tDim.iWidth + 15) / 16;
  int i16x16Height = (tDim.iHeight + 15) / 16;

  return i16x16Width * i16x16Height;
}

/****************************************************************************/
static int GetPcmSizeWithFractionalCoefficient(AL_TDimension tDim, AL_EChromaMode eMode, int iBitDepth, int coeffNumerator, int coeffDenominator)
{
  const int bitdepthNumerator = iBitDepth;
  const int bitdepthDenominator = 8;

  /* Careful round up (at the 64x64 MB/LCU level) while calculating fraction. */
  const int iLcu64PcmSizeMultipliedByFraction = ((AL_PCM_SIZE[eMode][2] * bitdepthNumerator * coeffNumerator + (bitdepthDenominator * coeffDenominator - 1)) / (bitdepthDenominator * coeffDenominator));
  return GetBlk64x64(tDim) * iLcu64PcmSizeMultipliedByFraction;
}

/****************************************************************************/
int GetPcmVclNalSize(AL_TDimension tDim, AL_EChromaMode eMode, int iBitDepth)
{
  return GetPcmSizeWithFractionalCoefficient(tDim, eMode, iBitDepth, 1, 1);
}

/****************************************************************************/
int GetMaxVclNalSize(AL_TDimension tDim, AL_EChromaMode eMode, int iBitDepth)
{
  /* Spec. A.3.2, A.3.3: Number of bits in the macroblock is at most: 5 * RawCtuBits / 3. */
  return GetPcmSizeWithFractionalCoefficient(tDim, eMode, iBitDepth, 5, 3);
}

/****************************************************************************/
int AL_GetMaxNalSize(AL_ECodec eCodec, AL_TDimension tDim, AL_EChromaMode eMode, int iBitDepth, int iLevel, int iProfileIdc)
{
  (void)iProfileIdc;
  /* Actual worst case: 5/3*PCM + one slice per MB/LCU. */
  int iMaxPCM = eCodec == AL_CODEC_HEVC ? GetMaxVclNalSize(tDim, eMode, iBitDepth) : GetPcmVclNalSize(tDim, eMode, iBitDepth);
  int iNumSlices = iLevel > 52 ? 600 : 200;

  if(eCodec == AL_CODEC_AVC)
    iNumSlices = Avc_GetMaxNumberOfSlices(iProfileIdc, iLevel, 1, 60, INT32_MAX);

  iMaxPCM += 2048 + (iNumSlices * AL_MAX_SLICE_HEADER_SIZE);

  return RoundUp(iMaxPCM, 32);
}

/****************************************************************************/
int AL_GetMitigatedMaxNalSize(AL_TDimension tDim, AL_EChromaMode eMode, int iBitDepth)
{
  /* Mitigated worst case: PCM + one slice per row. */
  int iMaxPCM = GetPcmVclNalSize(tDim, eMode, iBitDepth);
  int iNumSlices = ((tDim.iHeight + 15) / 16);
  iMaxPCM += 2048 + (iNumSlices * AL_MAX_SLICE_HEADER_SIZE);

  return RoundUp(iMaxPCM, 32);
}

