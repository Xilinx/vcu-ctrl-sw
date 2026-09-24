// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <cinttypes>
#include <cstdio>
#include <string>

#include "lib_app/SinkCrcDump.hpp"
#include "lib_app/Sink.hpp"
#include "lib_app/utils.hpp"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/DisplayInfoMeta.h"
}

class StreamCrcDump : public IFrameSink
{
public:
  StreamCrcDump(std::string const& path)
  {
    if(!path.empty())
    {
      m_CrcFile = fopen(path.c_str(), "w");
    }
  }

  ~StreamCrcDump()
  {
    if(m_CrcFile)
    {
      fclose(m_CrcFile);
    }
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    auto* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DISPLAY_INFO));

    if(pMeta)
    {
      if(m_CrcFile)
      {
        fprintf(m_CrcFile, "%016" PRIX64 " - %08" PRIu32, pMeta->uCrc, pMeta->uPixCount);
        fprintf(m_CrcFile, "\n");
      }
    }
  }

private:
  FILE* m_CrcFile = nullptr;
};

IFrameSink* createStreamCrcDump(std::string const& path)
{
  return new StreamCrcDump(path);
}
