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

#include <cassert>
#include <iostream>

#include "AL_NvxConvert.h"

extern "C"
{
#include "lib_common/BufferSrcMeta.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "lib_app/convert.h"

using namespace std;

/*****************************************************************************/
CNvxConv::CNvxConv(TFrameInfo const& FrameInfo) : m_FrameInfo(FrameInfo)
{
}

/*****************************************************************************/
unsigned int CNvxConv::GetSrcBufSize(int iPitch, int iStrideHeight)
{
  AL_TDimension tDim = { m_FrameInfo.iWidth, m_FrameInfo.iHeight };
  return AL_GetAllocSizeSrc(tDim, m_FrameInfo.eCMode, AL_SRC_NVX, iPitch, iStrideHeight);
}

#include <sstream>
#include <string>

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
};

static void convertToY010(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I0AL):
  case FOURCC(I2AL):
  case FOURCC(P010):
  case FOURCC(P210):
    I0AL_To_Y010(pSrcIn, pSrcOut);
    break;

  case FOURCC(I420):
  case FOURCC(I422):
  case FOURCC(NV12):
  case FOURCC(NV16):
  case FOURCC(Y800):
    Y800_To_Y010(pSrcIn, pSrcOut);
    break;

  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToY800(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I0AL):
    I0AL_To_Y800(pSrcIn, pSrcOut);
    break;
  case FOURCC(I420):
  case FOURCC(I422):
    I420_To_Y800(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToNV12(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800):
    Y800_To_NV12(pSrcIn, pSrcOut);
    break;
  case FOURCC(I420):
    I420_To_NV12(pSrcIn, pSrcOut);
    break;
  case FOURCC(IYUV):
    IYUV_To_NV12(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV12):
    YV12_To_NV12(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV12):
    AL_CopyYuv(pSrcIn, pSrcOut);
    break;
  case FOURCC(P010):
    P010_To_NV12(pSrcIn, pSrcOut);
    break;
  case FOURCC(I0AL):
    I0AL_To_NV12(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToNV16(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422):
    I422_To_NV16(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV16):
    I422_To_NV16(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV16):
    AL_CopyYuv(pSrcIn, pSrcOut);
    break;
  case FOURCC(I2AL):
    I2AL_To_NV16(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToXV15(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800):
    Y800_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(I420):
    I420_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(IYUV):
    IYUV_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV12):
    YV12_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV12):
    NV12_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(P010):
    P010_To_XV15(pSrcIn, pSrcOut);
    break;
  case FOURCC(I0AL):
    I0AL_To_XV15(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToXV20(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422):
    I422_To_XV20(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV16):
    I422_To_XV20(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV16):
    NV16_To_XV20(pSrcIn, pSrcOut);
    break;
  case FOURCC(I2AL):
    I2AL_To_XV20(pSrcIn, pSrcOut);
    break;
  case FOURCC(P210):
    P210_To_XV20(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToXV10(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y010):
  case FOURCC(I0AL):
  case FOURCC(I2AL):
  case FOURCC(P010):
  case FOURCC(P210):
    Y010_To_XV10(pSrcIn, pSrcOut);
    break;

  case FOURCC(Y800):
  case FOURCC(I420):
  case FOURCC(I422):
  case FOURCC(NV12):
  case FOURCC(NV16):
    Y800_To_XV10(pSrcIn, pSrcOut);
    break;

  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}


static void convertToP010(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800):
    Y800_To_P010(pSrcIn, pSrcOut);
    break;
  case FOURCC(I420):
    I420_To_P010(pSrcIn, pSrcOut);
    break;
  case FOURCC(IYUV):
    IYUV_To_P010(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV12):
    YV12_To_P010(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV12):
    NV12_To_P010(pSrcIn, pSrcOut);
    break;
  case FOURCC(P010):
    AL_CopyYuv(pSrcIn, pSrcOut);
    break;
  case FOURCC(I0AL):
    I0AL_To_P010(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

static void convertToP210(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422):
    I422_To_P210(pSrcIn, pSrcOut);
    break;
  case FOURCC(YV16):
    I422_To_P210(pSrcIn, pSrcOut);
    break;
  case FOURCC(P210):
    AL_CopyYuv(pSrcIn, pSrcOut);
    break;
  case FOURCC(I2AL):
    I2AL_To_P210(pSrcIn, pSrcOut);
    break;
  case FOURCC(NV16):
    NV16_To_P210(pSrcIn, pSrcOut);
    break;
  default:
    cout << "No conversion known from " << FourCCToString(inFourCC) << endl;
    assert(0);
  }
}

/*****************************************************************************/
void CNvxConv::ConvertSrcBuf(uint8_t uBitDepth, AL_TBuffer const* pSrcIn, AL_TBuffer* pSrcOut)
{
  AL_TSrcMetaData* pSrcInMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pSrcIn, AL_META_TYPE_SOURCE);
  auto const picFmt = AL_EncGetSrcPicFormat(m_FrameInfo.eCMode, uBitDepth, AL_FB_RASTER, false);
  TFourCC tSrcFourCC = AL_GetFourCC(picFmt);
  switch(tSrcFourCC)
  {
  case FOURCC(Y010):
    convertToY010(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(Y800):
    convertToY800(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(NV12):
    convertToNV12(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(NV16):
    convertToNV16(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(XV15):
    convertToXV15(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(XV20):
    convertToXV20(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(XV10):
    convertToXV10(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(P010):
    convertToP010(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  case FOURCC(P210):
    convertToP210(pSrcIn, pSrcInMeta->tFourCC, pSrcOut);
    break;

  default:
    cout << "No conversion known to " << FourCCToString(tSrcFourCC) << endl;
    assert(0);
  }
}

