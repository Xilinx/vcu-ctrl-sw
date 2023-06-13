// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include <string>
#include <iomanip>

#include "lib_app/SinkCrcDump.h"
#include "lib_app/Sink.h"
#include "lib_app/utils.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/DisplayInfoMeta.h"
}

class StreamCrcDump : public IFrameSink
{
public:
  StreamCrcDump(std::string& path)
  {
    if(!path.empty())
    {
      /*if(path == "stdout")
        m_pCrcOut = &std::cout;
      else
      {
        OpenOutput(m_CrcFile, path);
        m_pCrcOut = &m_CrcFile;
      }*/
      OpenOutput(m_CrcFile, path, false);
      m_CrcFile << std::hex << std::uppercase;
    }
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    /*if(pBuf == EndOfStream)
    {
      auto const sCrc = m_Crc;
      * m_pCrcOut << sCrc << std::endl;
      return;
    }*/

    AL_TDisplayInfoMetaData* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DISPLAY_INFO));

    if(pMeta)
    {
      if(m_CrcFile.is_open())
        m_CrcFile << std::setfill('0') << std::setw(8) << (int)pMeta->uCrc << std::endl;
    }
  }

private:
  std::ofstream m_CrcFile;
  // std::ostream* m_pCrcOut;
};

std::unique_ptr<IFrameSink> createStreamCrcDump(std::string path)
{
  return std::unique_ptr<IFrameSink>(new StreamCrcDump(path));
}
