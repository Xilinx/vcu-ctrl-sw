/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "lib_conv_yuv/AL_RasterConvert.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "lib_app/convert.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
}

using namespace std;

/*****************************************************************************/
CRasterConv::CRasterConv(TFrameInfo const& FrameInfo) :
  m_FrameInfo(FrameInfo)
{
}

/*****************************************************************************/
static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
}

/*****************************************************************************/
static void NoConversionFound(TFourCC tInFourCC, TFourCC tOutFourCC)
{
  cout << "No conversion known from " << FourCCToString(tInFourCC) << " to " << FourCCToString(tOutFourCC) << endl;
  assert(0);
}

/*****************************************************************************/
static void convertToY010(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I0AL):
  case FOURCC(I2AL):
  case FOURCC(P010):
  case FOURCC(P210):
    return I0AL_To_Y010(pSrcIn, pSrcOut);
  case FOURCC(I420):
  case FOURCC(I422):
  case FOURCC(NV12):
  case FOURCC(NV16):
  case FOURCC(Y800):
    return Y800_To_Y010(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(Y010));
  }
}

/*****************************************************************************/
static void convertToY012(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I0CL):
  case FOURCC(I2CL):
    return I0CL_To_Y012(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(Y012));
  }
}

/*****************************************************************************/
static void convertToY800(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I0AL): return I0AL_To_Y800(pSrcIn, pSrcOut);
  case FOURCC(I420):
  case FOURCC(I422):
    return I420_To_Y800(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(Y800));
  }
}

/*****************************************************************************/
static void convertToNV12(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800): return Y800_To_NV12(pSrcIn, pSrcOut);
  case FOURCC(I420): return I420_To_NV12(pSrcIn, pSrcOut);
  case FOURCC(IYUV): return IYUV_To_NV12(pSrcIn, pSrcOut);
  case FOURCC(YV12): return YV12_To_NV12(pSrcIn, pSrcOut);
  case FOURCC(NV12): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  case FOURCC(P010): return P010_To_NV12(pSrcIn, pSrcOut);
  case FOURCC(I0AL): return I0AL_To_NV12(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(NV12));
  }
}

/*****************************************************************************/
static void convertToNV16(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422): return I422_To_NV16(pSrcIn, pSrcOut);
  case FOURCC(YV16): return I422_To_NV16(pSrcIn, pSrcOut);
  case FOURCC(NV16): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  case FOURCC(I2AL): return I2AL_To_NV16(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(NV16));
  }
}

/*****************************************************************************/
static void convertToNV24(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I444): return I444_To_NV24(pSrcIn, pSrcOut);
  case FOURCC(I4AL): return I4AL_To_NV24(pSrcIn, pSrcOut);
  case FOURCC(NV24): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(NV24));
  }
}

/*****************************************************************************/
static void convertToXV15(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800): return Y800_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(I420): return I420_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(IYUV): return IYUV_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(YV12): return YV12_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(NV12): return NV12_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(P010): return P010_To_XV15(pSrcIn, pSrcOut);
  case FOURCC(I0AL): return I0AL_To_XV15(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(XV15));
  }
}

/*****************************************************************************/
static void convertToXV20(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422): return I422_To_XV20(pSrcIn, pSrcOut);
  case FOURCC(YV16): return I422_To_XV20(pSrcIn, pSrcOut);
  case FOURCC(NV16): return NV16_To_XV20(pSrcIn, pSrcOut);
  case FOURCC(I2AL): return I2AL_To_XV20(pSrcIn, pSrcOut);
  case FOURCC(P210): return P210_To_XV20(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(XV20));
  }
}

/*****************************************************************************/
static void convertToXV10(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y010):
  case FOURCC(I0AL):
  case FOURCC(I2AL):
  case FOURCC(P010):
  case FOURCC(P210):
    return Y010_To_XV10(pSrcIn, pSrcOut);
  case FOURCC(Y800):
  case FOURCC(I420):
  case FOURCC(I422):
  case FOURCC(NV12):
  case FOURCC(NV16):
    return Y800_To_XV10(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(XV10));
  }
}

/*****************************************************************************/
static void convertToP010(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(Y800): return Y800_To_P010(pSrcIn, pSrcOut);
  case FOURCC(I420): return I420_To_P010(pSrcIn, pSrcOut);
  case FOURCC(IYUV): return IYUV_To_P010(pSrcIn, pSrcOut);
  case FOURCC(YV12): return YV12_To_P010(pSrcIn, pSrcOut);
  case FOURCC(NV12): return NV12_To_P010(pSrcIn, pSrcOut);
  case FOURCC(P010): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  case FOURCC(I0AL): return I0AL_To_P010(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(P010));
  }
}

/*****************************************************************************/
static void convertToP012(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(P012): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  case FOURCC(I0CL): return I0CL_To_P012(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(P012));
  }
}

/*****************************************************************************/
static void convertToP210(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I422): return I422_To_P210(pSrcIn, pSrcOut);
  case FOURCC(YV16): return I422_To_P210(pSrcIn, pSrcOut);
  case FOURCC(P210): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  case FOURCC(I2AL): return I2AL_To_P210(pSrcIn, pSrcOut);
  case FOURCC(NV16): return NV16_To_P210(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(P210));
  }
}

/*****************************************************************************/
static void convertToP212(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(I2CL): return I2CL_To_P212(pSrcIn, pSrcOut);
  case FOURCC(P212): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(P212));
  }
}

/*****************************************************************************/
static void convertToP410(AL_TBuffer const* pSrcIn, TFourCC inFourCC, AL_TBuffer* pSrcOut)
{
  switch(inFourCC)
  {
  case FOURCC(NV24): return NV24_To_P410(pSrcIn, pSrcOut);
  case FOURCC(I444): return I444_To_P410(pSrcIn, pSrcOut);
  case FOURCC(I4AL): return I4AL_To_P410(pSrcIn, pSrcOut);
  case FOURCC(P410): return CopyPixMapBuffer(pSrcIn, pSrcOut);
  default: return NoConversionFound(inFourCC, FOURCC(P410));
  }
}

/*****************************************************************************/
void CRasterConv::ConvertSrcBuf(uint8_t uBitDepth, AL_TBuffer const* pSrcIn, AL_TBuffer* pSrcOut)
{
  TFourCC tFourCCIn = AL_PixMapBuffer_GetFourCC(pSrcIn);
  auto const picFmt = AL_EncGetSrcPicFormat(m_FrameInfo.eCMode, uBitDepth, AL_FB_RASTER, false);
  TFourCC tSrcOutFourCC = AL_GetFourCC(picFmt);
  switch(tSrcOutFourCC)
  {
  case FOURCC(Y800): return convertToY800(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(Y010): return convertToY010(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(Y012): return convertToY012(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(NV12): return convertToNV12(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(NV16): return convertToNV16(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(NV24): return convertToNV24(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(XV10): return convertToXV10(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(XV15): return convertToXV15(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(XV20): return convertToXV20(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(P010): return convertToP010(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(P012): return convertToP012(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(P210): return convertToP210(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(P212): return convertToP212(pSrcIn, tFourCCIn, pSrcOut);
  case FOURCC(P410): return convertToP410(pSrcIn, tFourCCIn, pSrcOut);
  default: return NoConversionFound(tFourCCIn, tSrcOutFourCC);
  }
}
