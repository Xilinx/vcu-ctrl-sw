/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "sink_bitstream_writer.h"
#include "lib_app/utils.h" // OpenOutput
#include "lib_app/InputFiles.h"
#include "CodecUtils.h" // WriteStream
#include <fstream>

extern "C"
{
}
using namespace std;

void WriteContainerHeader(ofstream& fp, AL_TEncSettings const& Settings, TYUVFileInfo const& FileInfo, int numFrames);

struct BitstreamWriter : IFrameSink
{
  BitstreamWriter(string path, ConfigFile const& cfg_) : cfg(cfg_)
  {
    OpenOutput(m_file, path);
    WriteContainerHeader(m_file, cfg.Settings, cfg.MainInput.FileInfo, -1);
  }

  void ProcessFrame(AL_TBuffer* pStream) override
  {
    if(pStream == EndOfStream)
    {
      printBitrate();
      // update container header
      WriteContainerHeader(m_file, cfg.Settings, cfg.MainInput.FileInfo, m_frameCount);
      return;
    }

    m_frameCount += WriteStream(m_file, pStream, &cfg.Settings, hdr_pos, m_iFrameSize);
  }

  void printBitrate()
  {
    auto const outputSizeInBits = m_file.tellp() * 8;
    auto const frameRate = (float)cfg.Settings.tChParam[0].tRCParam.uFrameRate / cfg.Settings.tChParam[0].tRCParam.uClkRatio;
    auto const durationInSeconds = m_frameCount / (frameRate * cfg.Settings.NumLayer);
    auto bitrate = outputSizeInBits / durationInSeconds;
    LogInfo("Achieved bitrate = %.4f Kbps\n", (float)bitrate);
  }

  int m_frameCount = 0;
  ofstream m_file;
  streampos hdr_pos;
  int m_iFrameSize = 0;
  ConfigFile const cfg;
};

unique_ptr<IFrameSink> createBitstreamWriter(string path, ConfigFile const& cfg)
{

  if(cfg.Settings.TwoPass == 1)
    return unique_ptr<IFrameSink>(new NullFrameSink);

  return make_unique<BitstreamWriter>(path, cfg);
}

