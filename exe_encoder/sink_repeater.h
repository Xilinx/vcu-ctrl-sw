/******************************************************************************
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

#pragma once

#include <vector>

struct RepeaterSink : IFrameSink
{
  RepeaterSink(int bufferingCount, int maxPicCount) : m_bufferingCount{bufferingCount}, m_picCount{maxPicCount}
  {
  }

  ~RepeaterSink()
  {
    for(auto frame : frames)
      AL_Buffer_Unref(frame);
  }

  void startProcessForReal()
  {
    auto frame = frames.begin();

    while(m_picCount > 0)
    {
      next->PreprocessFrame();
      next->ProcessFrame(*frame);
      --m_picCount;
      ++frame;

      if(frame == frames.end())
        frame = frames.begin();
    }

    next->PreprocessFrame();
    next->ProcessFrame(nullptr);
  }

  void ProcessFrame(AL_TBuffer* frame) override
  {
    if(frame)
    {
      AL_Buffer_Ref(frame);
      frames.push_back(frame);
      --m_bufferingCount;
    }
    else
    {
      m_bufferingCount = 0;
    }

    if(m_bufferingCount > 0 || m_hasAlreadyStarted)
      return;

    if(!m_hasAlreadyStarted)
    {
      m_hasAlreadyStarted = true;
      startProcessForReal();
    }
  }

  IFrameSink* next;

private:
  int m_bufferingCount;
  int m_picCount;
  bool m_hasAlreadyStarted = false;

  std::vector<AL_TBuffer*> frames;
};

