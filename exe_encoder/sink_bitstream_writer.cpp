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

#include "sink_bitstream_writer.h"
#include "lib_cfg/lib_cfg.h"
#include "lib_app/utils.h" // OpenOutput
#include "lib_app/InputFiles.h"
#include "CodecUtils.h" // WriteStream
#include <fstream>

extern "C"
{
#include "lib_encode/lib_encoder.h"
}
using namespace std;


struct BitstreamWriter : IFrameSink
{
  BitstreamWriter(string path, ConfigFile const& cfg_) : cfg(cfg_)
  {
    OpenOutput(m_file, path);

  }

  ~BitstreamWriter()
  {
    auto bitrate = (m_file.tellp() * cfg.Settings.tChParam.tRCParam.uFrameRate * 8.0) / (m_frameCount * cfg.Settings.tChParam.tRCParam.uClkRatio);
    Message(CC_DEFAULT, "\nAchieved bitrate = %.4f Kbps\n", bitrate);
  }

  void ProcessFrame(AL_TBuffer* pStream)
  {
    if(pStream == EndOfStream)
    {
      return;
    }

    m_frameCount += WriteStream(m_file, pStream);
  }

  int m_frameCount = 0;
  ofstream m_file;
  ConfigFile const cfg;
};

unique_ptr<IFrameSink> createBitstreamWriter(string path, ConfigFile const& cfg)
{
  return unique_ptr<IFrameSink>(new BitstreamWriter(path, cfg));
}

