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

#include "sink_bitstream_writer.h"
#include "lib_app/utils.h" // OpenOutput
#include "lib_app/InputFiles.h"
#include "CodecUtils.h" // WriteStream
#include <fstream>

extern "C"
{
#include "lib_encode/lib_encoder.h"
}
using namespace std;

void WriteContainerHeader(ofstream& fp, AL_TEncSettings const& Settings, TYUVFileInfo const& FileInfo, int numFrames);

struct BitstreamWriter : IFrameSink
{
  BitstreamWriter(string path, ConfigFile const& cfg_) : cfg(cfg_)
  {
    OpenOutput(m_file, path);

    WriteContainerHeader(m_file, cfg.Settings, cfg.FileInfo, -1);
  }

  void ProcessFrame(AL_TBuffer* pStream)
  {
    if(pStream == EndOfStream)
    {
      printBitrate();
      // update container header
      WriteContainerHeader(m_file, cfg.Settings, cfg.FileInfo, m_frameCount);
      return;
    }

    m_frameCount += WriteStream(m_file, pStream, &cfg.Settings.tChParam[0]);
  }


  void printBitrate()
  {
    auto const outputSizeInBits = m_file.tellp() * 8;
    auto const frameRate = (float)cfg.Settings.tChParam[0].tRCParam.uFrameRate / cfg.Settings.tChParam[0].tRCParam.uClkRatio;
    auto const durationInSeconds = m_frameCount / frameRate;
    auto bitrate = outputSizeInBits / durationInSeconds;
    Message(CC_DEFAULT, "\nAchieved bitrate = %.4f Kbps\n", (float)bitrate);
  }

  int m_frameCount = 0;
  ofstream m_file;
  ConfigFile const cfg;
};

unique_ptr<IFrameSink> createBitstreamWriter(string path, ConfigFile const& cfg)
{
#if AL_ENABLE_TWOPASS

  if(cfg.Settings.TwoPass == 1)
    return unique_ptr<IFrameSink>(new NullFrameSink);
#endif

  return unique_ptr<IFrameSink>(new BitstreamWriter(path, cfg));
}

