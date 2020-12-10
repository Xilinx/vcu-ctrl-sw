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

#include "sink_frame_writer.h"

#include <cassert>

#include "CodecUtils.h"

#include "lib_app/YuvIO.h"
#include "lib_app/convert.h"
#include "lib_app/utils.h"

extern "C"
{
#include "lib_encode/lib_encoder.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common_enc/IpEncFourCC.h"
}

using namespace std;

/****************************************************************************/
void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tYuvFourCC)
{
  TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pRec);

  assert(AL_IsTiled(tRecFourCC));
  switch(tRecFourCC)
  {
  case FOURCC(T6m8):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T608_To_Y800(pRec, pYuv);
    case FOURCC(Y010): return T608_To_Y010(pRec, pYuv);
    case FOURCC(I420): return T6m8_To_I420(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }
  case FOURCC(T6mA):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T60A_To_Y800(pRec, pYuv);
    case FOURCC(Y010): return T60A_To_Y010(pRec, pYuv);
    case FOURCC(XV10): return T60A_To_XV10(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }
  case FOURCC(T608):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T608_To_Y800(pRec, pYuv);
    case FOURCC(Y010): return T608_To_Y010(pRec, pYuv);
    case FOURCC(I420): return T608_To_I420(pRec, pYuv);
    case FOURCC(IYUV): return T608_To_IYUV(pRec, pYuv);
    case FOURCC(YV12): return T608_To_YV12(pRec, pYuv);
    case FOURCC(NV12): return T608_To_NV12(pRec, pYuv);
    case FOURCC(P010): return T608_To_P010(pRec, pYuv);
    case FOURCC(I0AL): return T608_To_I0AL(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }
  case FOURCC(T628):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T628_To_Y800(pRec, pYuv);
    case FOURCC(Y010): return T628_To_Y010(pRec, pYuv);
    case FOURCC(I422): return T628_To_I422(pRec, pYuv);
    case FOURCC(NV16): return T628_To_NV16(pRec, pYuv);
    case FOURCC(I2AL): return T628_To_I2AL(pRec, pYuv);
    case FOURCC(P210): return T628_To_P210(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }
  case FOURCC(T648):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(I444): return T648_To_I444(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }
  case FOURCC(T60A):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T60A_To_Y800(pRec, pYuv);
    case FOURCC(I420): return T60A_To_I420(pRec, pYuv);
    case FOURCC(IYUV): return T60A_To_IYUV(pRec, pYuv);
    case FOURCC(YV12): return T60A_To_YV12(pRec, pYuv);
    case FOURCC(NV12): return T60A_To_NV12(pRec, pYuv);
    case FOURCC(P010): return T60A_To_P010(pRec, pYuv);
    case FOURCC(I0AL): return T60A_To_I0AL(pRec, pYuv);
    case FOURCC(Y010): return T60A_To_Y010(pRec, pYuv);
    case FOURCC(XV15): return T60A_To_XV15(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  case FOURCC(T62A):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T62A_To_Y800(pRec, pYuv);
    case FOURCC(Y010): return T62A_To_Y010(pRec, pYuv);
    case FOURCC(I422): return T62A_To_I422(pRec, pYuv);
    case FOURCC(NV16): return T62A_To_NV16(pRec, pYuv);
    case FOURCC(I2AL): return T62A_To_I2AL(pRec, pYuv);
    case FOURCC(P210): return T62A_To_P210(pRec, pYuv);
    case FOURCC(XV20): return T62A_To_XV20(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  case FOURCC(T64A):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(I4AL): return T64A_To_I4AL(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  case FOURCC(T60C):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T60C_To_Y800(pRec, pYuv);
    case FOURCC(I420): return T60C_To_I420(pRec, pYuv);
    case FOURCC(I0AL): return T60C_To_I0AL(pRec, pYuv);
    case FOURCC(I0CL): return T60C_To_I0CL(pRec, pYuv);
    case FOURCC(Y010): return T60C_To_Y010(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  case FOURCC(T62C):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(Y800): return T62C_To_Y800(pRec, pYuv);
    case FOURCC(Y012): return T62C_To_Y012(pRec, pYuv);
    case FOURCC(I422): return T62C_To_I422(pRec, pYuv);
    case FOURCC(I2CL): return T62C_To_I2CL(pRec, pYuv);
    case FOURCC(P212): return T62C_To_P212(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  case FOURCC(T64C):
  {
    switch(tYuvFourCC)
    {
    case FOURCC(I4CL): return T64C_To_I4CL(pRec, pYuv);
    default: assert(false && "Unknown tile conversion");
    }

    break;
  }

  default:
    assert(false && "Unknown tile format");
  }
}

class FrameWriter : public IFrameSink
{
public:
  FrameWriter(string RecFileName, ConfigFile& cfg_, AL_TBuffer* Yuv_, int iLayerID) : m_cfg(cfg_), m_Yuv(Yuv_)
  {
    (void)iLayerID; // if no fbc support
    OpenOutput(m_RecFile, RecFileName);
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    if(pBuf == EndOfStream)
    {
      m_RecFile.flush();
      return;
    }

    AL_PixMapBuffer_SetDimension(m_Yuv, AL_PixMapBuffer_GetDimension(pBuf));

    {
      RecToYuv(pBuf, m_Yuv, m_cfg.RecFourCC);
      WriteOneFrame(m_RecFile, m_Yuv);
    }
  }

private:
  ofstream m_RecFile;
  ConfigFile& m_cfg;
  AL_TBuffer* const m_Yuv;
};

unique_ptr<IFrameSink> createFrameWriter(string path, ConfigFile& cfg_, AL_TBuffer* Yuv_, int iLayerID_)
{

  if(cfg_.Settings.TwoPass == 1)
    return unique_ptr<IFrameSink>(new NullFrameSink);

  return unique_ptr<IFrameSink>(new FrameWriter(path, cfg_, Yuv_, iLayerID_));
}

