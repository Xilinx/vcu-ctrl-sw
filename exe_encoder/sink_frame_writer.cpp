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

#include "sink_frame_writer.h"
#include "CodecUtils.h"
#include "YuvIO.h"
#include "lib_app/utils.h"
#include <cassert>


extern "C"
{
#include "lib_encode/lib_encoder.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common_enc/IpEncFourCC.h"
}
#include "lib_app/convert.h"

/****************************************************************************/
void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tFourCC)
{
  AL_TSrcMetaData* pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pRec, AL_META_TYPE_SOURCE);

  if(pRecMeta->tFourCC == FOURCC(Y800))
  {
    if(tFourCC == FOURCC(I420))
      Y800_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      Y800_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      Y800_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      Y800_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(Y800))
      AL_CopyYuv(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      Y800_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      Y800_To_I0AL(pRec, pYuv);
    // else if(tFourCC == FOURCC(Y010)) Y800_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(T60A))
  {
    if(tFourCC == FOURCC(Y800))
      T60A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      T60A_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      T60A_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      T60A_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      T60A_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      T60A_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      T60A_To_I0AL(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T60A_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(XV15))
      T60A_To_XV15(pRec, pYuv);
  }
  else if(pRecMeta->tFourCC == FOURCC(T62A))
  {
    if(tFourCC == FOURCC(Y800))
      T62A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T62A_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I422))
      T62A_To_I422(pRec, pYuv);
    else if(tFourCC == FOURCC(NV16))
      T62A_To_NV16(pRec, pYuv);
    else if(tFourCC == FOURCC(I2AL))
      T62A_To_I2AL(pRec, pYuv);
    else if(tFourCC == FOURCC(P210))
      T62A_To_P210(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T62A_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(XV20))
      T62A_To_XV20(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(T608))
  {
    if(tFourCC == FOURCC(Y800))
      T608_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T608_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      T608_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      T608_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      T608_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      T608_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      T608_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      T608_To_I0AL(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T608_To_Y010(pRec, pYuv);
  }
  else if(pRecMeta->tFourCC == FOURCC(T628))
  {
    if(tFourCC == FOURCC(Y800))
      T628_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T628_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I422))
      T628_To_I422(pRec, pYuv);
    else if(tFourCC == FOURCC(NV16))
      T628_To_NV16(pRec, pYuv);
    else if(tFourCC == FOURCC(I2AL))
      T628_To_I2AL(pRec, pYuv);
    else if(tFourCC == FOURCC(P210))
      T628_To_P210(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T628_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(T6m8))
  {
    if(tFourCC == FOURCC(Y800))
      T608_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T608_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      T6m8_To_I420(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(T6mA))
  {
    if(tFourCC == FOURCC(Y800))
      T60A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      T60A_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(XV10))
      T60A_To_XV10(pRec, pYuv);
    else
      assert(0);
  }
  else
    assert(0);
}

using namespace std;

class FrameWriter : public IFrameSink
{
public:
  FrameWriter(string RecFileName, ConfigFile& cfg_, AL_TBuffer* Yuv_, int iLayerID) : m_cfg(cfg_), m_Yuv(Yuv_)
  {
    (void)iLayerID; // if no fbc support
    OpenOutput(m_RecFile, RecFileName);
  }

  void ProcessFrame(AL_TBuffer* pBuf)
  {
    if(pBuf == EndOfStream)
    {
      m_RecFile.flush();
      return;
    }

    AL_TSrcMetaData* pMetaRec = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);
    AL_TSrcMetaData* pMetaYUV = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(m_Yuv, AL_META_TYPE_SOURCE);
    pMetaYUV->tDim = pMetaRec->tDim;

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

