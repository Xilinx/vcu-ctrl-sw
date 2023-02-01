/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include "sink_bitrate.h"
#include "CodecUtils.h" // for GetImageStreamSize
#include <deque>
#include <string>
#include <fstream>
#include "lib_app/utils.h" // for OpenOutput

#include <iostream>

struct BitrateWriter : IFrameSink
{
  BitrateWriter(std::string path, float framerate_) : framerate(framerate_)
  {
    OpenOutput(m_file, path);
    imageSizes.push_back(ImageSize { 0, false });
  }

  float calculateBitrate(int numBits, int numFrame)
  {
    auto const durationInSeconds = numFrame / framerate * 1000;
    /* bitrate in Kbps */
    return numBits / durationInSeconds;
  }

  float calculateWindowBitrate()
  {
    int totalBits = 0;

    for(auto& numBits : window)
    {
      totalBits += numBits;
    }

    return calculateBitrate(totalBits, window.size());
  }

  void ProcessFrame(AL_TBuffer* pStream) override
  {
    if(pStream == EndOfStream)
    {
      m_file << "]" << std::endl;
      return;
    }

    GetImageStreamSize(pStream, imageSizes);
    auto it = imageSizes.begin();

    while(it != imageSizes.end())
    {
      if(it->finished)
      {
        ++numFrame;
        int numBits = it->size * 8;

        window.push_back(numBits);

        if(window.size() > windowSize)
          window.pop_front();

        totalBits += numBits;

        auto windowBitrate = calculateWindowBitrate();

        /* first frame */
        if(numFrame == 1)
        {
          m_file << "[";
          m_file << "{ \"line_type\" : \"header\", \"window size\" : " << windowSize << ", \"units\" : \"size in bytes, bitrate in Kbps\" }";
        }

        m_file << "," << std::endl;

        m_file << "{ \"frame\" : " << numFrame << ", \"size \" : " << it->size << ", \"window bitrate\" : " << windowBitrate << ", \"bitrate alone\" : " << calculateBitrate(numBits, 1) << ", \"total bitrate\" : " << calculateBitrate(totalBits, numFrame) << "}";

        it = imageSizes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  size_t windowSize = 30;
  std::deque<int> window {};
  std::deque<ImageSize> imageSizes {};
  int numFrame = 0;
  float framerate;
  int totalBits = 0;
  std::ofstream m_file;
};

std::unique_ptr<IFrameSink> createBitrateWriter(std::string path, ConfigFile const& cfg)
{
  auto const frameRate = (float)cfg.Settings.tChParam[0].tRCParam.uFrameRate / cfg.Settings.tChParam[0].tRCParam.uClkRatio * 1000;
  return std::unique_ptr<IFrameSink>(new BitrateWriter(path, frameRate));
}

