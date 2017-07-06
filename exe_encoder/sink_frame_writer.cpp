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

#include "sink_frame_writer.h"
#include "CodecUtils.h"
#include "lib_cfg/lib_cfg.h"
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

  if(pRecMeta->tFourCC == FOURCC(P010))
  {
    if(tFourCC == FOURCC(I420))
      P010_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      P010_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      P010_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      P010_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(Y800))
      P010_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      AL_CopyYuv(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      P010_To_I0AL(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      P010_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(NV12))
  {
    if(tFourCC == FOURCC(I420))
      NV12_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      NV12_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      NV12_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      AL_CopyYuv(pRec, pYuv);
    else if(tFourCC == FOURCC(Y800))
      NV12_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      NV12_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      NV12_To_I0AL(pRec, pYuv);
    // else if(tFourCC == FOURCC(Y010)) NV12_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(Y800))
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
  else if(pRecMeta->tFourCC == FOURCC(AL0A))
  {
    if(tFourCC == FOURCC(Y800))
      AL0A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      AL0A_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      AL0A_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      AL0A_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      AL0A_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      AL0A_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      AL0A_To_I0AL(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL0A_To_Y010(pRec, pYuv);
  }
  else if(pRecMeta->tFourCC == FOURCC(AL2A))
  {
    if(tFourCC == FOURCC(Y800))
      AL2A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL2A_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I422))
      AL2A_To_I422(pRec, pYuv);
    else if(tFourCC == FOURCC(NV16))
      AL2A_To_NV16(pRec, pYuv);
    else if(tFourCC == FOURCC(I2AL))
      AL2A_To_I2AL(pRec, pYuv);
    else if(tFourCC == FOURCC(P210))
      AL2A_To_P210(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL2A_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(AL08))
  {
    if(tFourCC == FOURCC(Y800))
      AL08_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL08_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      AL08_To_I420(pRec, pYuv);
    else if(tFourCC == FOURCC(IYUV))
      AL08_To_IYUV(pRec, pYuv);
    else if(tFourCC == FOURCC(YV12))
      AL08_To_YV12(pRec, pYuv);
    else if(tFourCC == FOURCC(NV12))
      AL08_To_NV12(pRec, pYuv);
    else if(tFourCC == FOURCC(P010))
      AL08_To_P010(pRec, pYuv);
    else if(tFourCC == FOURCC(I0AL))
      AL08_To_I0AL(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL08_To_Y010(pRec, pYuv);
  }
  else if(pRecMeta->tFourCC == FOURCC(AL28))
  {
    if(tFourCC == FOURCC(Y800))
      AL28_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL28_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I422))
      AL28_To_I422(pRec, pYuv);
    else if(tFourCC == FOURCC(NV16))
      AL28_To_NV16(pRec, pYuv);
    else if(tFourCC == FOURCC(I2AL))
      AL28_To_I2AL(pRec, pYuv);
    else if(tFourCC == FOURCC(P210))
      AL28_To_P210(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL28_To_Y010(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(ALm8))
  {
    if(tFourCC == FOURCC(Y800))
      AL08_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL08_To_Y010(pRec, pYuv);
    else if(tFourCC == FOURCC(I420))
      ALm8_To_I420(pRec, pYuv);
    else
      assert(0);
  }
  else if(pRecMeta->tFourCC == FOURCC(ALmA))
  {
    if(tFourCC == FOURCC(Y800))
      AL0A_To_Y800(pRec, pYuv);
    else if(tFourCC == FOURCC(Y010))
      AL0A_To_Y010(pRec, pYuv);
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
  FrameWriter(string RecFileName, ConfigFile& cfg_, AL_TBuffer* Yuv_) : cfg(cfg_), Yuv(Yuv_)
  {
    OpenOutput(m_RecFile, RecFileName);
  }

  void ProcessFrame(AL_TBuffer* pBuf)
  {
    if(pBuf == EndOfStream)
    {
      m_RecFile.flush();
      return;
    }

    {
      RecToYuv(pBuf, Yuv, cfg.RecFourCC);
      WriteOneFrame(m_RecFile, Yuv, cfg.FileInfo.PictWidth, cfg.FileInfo.PictHeight);
    }
  }

private:
  ofstream m_RecFile;
  ConfigFile& cfg;
  AL_TBuffer* const Yuv;
};

unique_ptr<IFrameSink> createFrameWriter(string path, ConfigFile& cfg_, AL_TBuffer* Yuv_)
{
  return unique_ptr<IFrameSink>(new FrameWriter(path, cfg_, Yuv_));
}

