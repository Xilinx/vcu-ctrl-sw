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

#include <fstream>
#include "lib_app/utils.h"
#include "sink_md5.h"
#include "MD5.h"
#include "CodecUtils.h"

extern "C"
{
#include "lib_common/BufferSrcMeta.h"
}

void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tFourCC);

class Md5Calculator : public IFrameSink
{
public:
  Md5Calculator(std::string path, ConfigFile& cfg_, AL_TBuffer* Yuv_) :
    Yuv(Yuv_),
    fourcc(cfg_.RecFourCC)
  {
    OpenOutput(m_Md5File, path);
  }

  void ProcessFrame(AL_TBuffer* pBuf)
  {
    if(pBuf == EndOfStream)
    {
      auto const sMD5 = m_MD5.GetMD5();
      m_Md5File << sMD5;
      return;
    }

    auto meta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

    if(meta->tFourCC != fourcc)
    {
      RecToYuv(pBuf, Yuv, fourcc);
      pBuf = Yuv;
    }

    m_MD5.Update(AL_Buffer_GetData(pBuf), pBuf->zSize);
  }


private:
  std::ofstream m_Md5File;
  CMD5 m_MD5;
  AL_TBuffer* const Yuv;
  TFourCC const fourcc;
};

std::unique_ptr<IFrameSink> createMd5Calculator(std::string path, ConfigFile& cfg_, AL_TBuffer* Yuv_)
{
  return std::unique_ptr<IFrameSink>(new Md5Calculator(path, cfg_, Yuv_));
}

