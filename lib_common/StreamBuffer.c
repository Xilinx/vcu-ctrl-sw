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

#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamBufferPrivate.h"

/****************************************************************************/
int GetBlk64x64(AL_TDimension tDim)
{
  int i64x64Width = (tDim.iWidth + 63) >> 6;
  int i64x64Height = (tDim.iHeight + 63) >> 6;

  return i64x64Width * i64x64Height;
}

/****************************************************************************/
int GetBlk32x32(AL_TDimension tDim)
{
  int i32x32Width = (tDim.iWidth + 31) >> 5;
  int i32x32Height = (tDim.iHeight + 31) >> 5;

  return i32x32Width * i32x32Height;
}

/****************************************************************************/
int GetBlk16x16(AL_TDimension tDim)
{
  int i16x16Width = (tDim.iWidth + 15) >> 4;
  int i16x16Height = (tDim.iHeight + 15) >> 4;

  return i16x16Width * i16x16Height;
}

/****************************************************************************/
int GetMaxVclNalSize(AL_TDimension tDim, AL_EChromaMode eMode)
{
  return GetBlk64x64(tDim) * AL_PCM_SIZE[eMode][2];
}

/****************************************************************************/
int AL_GetMaxNalSize(AL_TDimension tDim, AL_EChromaMode eMode)
{
  int iMaxPCM = GetMaxVclNalSize(tDim, eMode);

  iMaxPCM += 2048 + (((tDim.iHeight + 15) / 16) * AL_MAX_SLICE_HEADER_SIZE);

  return RoundUp(iMaxPCM, 32);
}

