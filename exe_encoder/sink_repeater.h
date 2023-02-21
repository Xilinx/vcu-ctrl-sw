// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

