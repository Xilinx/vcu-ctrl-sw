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

