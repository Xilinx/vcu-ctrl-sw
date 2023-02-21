// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <memory>

extern "C"
{
#include "lib_common/BufferAPI.h"
}

struct IFrameSink
{
  virtual ~IFrameSink() {};
  virtual void PreprocessFrame() {};
  virtual void ProcessFrame(AL_TBuffer* frame) = 0;
};

struct NullFrameSink : IFrameSink
{
  virtual void ProcessFrame(AL_TBuffer*) override {}
};

struct MultiSink : IFrameSink
{
  void ProcessFrame(AL_TBuffer* frame) override
  {
    for(auto& sink : sinks)
      sink->ProcessFrame(frame);
  }

  std::vector<std::unique_ptr<IFrameSink>> sinks;
};

AL_TBuffer* const EndOfStream = nullptr;

