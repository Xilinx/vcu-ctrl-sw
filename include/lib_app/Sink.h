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
  virtual ~IFrameSink() = default;
  virtual void PreprocessFrame() {};
  virtual void ProcessFrame(AL_TBuffer* frame) = 0;
};

struct NullFrameSink final : IFrameSink
{
  ~NullFrameSink() = default;
  void ProcessFrame(AL_TBuffer*) override {}
};

struct MultiSink final : IFrameSink
{
  ~MultiSink() = default;
  void ProcessFrame(AL_TBuffer* frame) override
  {
    for(auto const& sink : sinks)
      sink->ProcessFrame(frame);
  }

  void addSink(std::unique_ptr<IFrameSink>& newWriter)
  {
    sinks.push_back(std::move(newWriter));
  }

private:
  std::vector<std::unique_ptr<IFrameSink>> sinks;
};

AL_TBuffer* const EndOfStream = nullptr;

