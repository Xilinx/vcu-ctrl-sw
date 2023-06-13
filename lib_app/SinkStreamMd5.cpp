// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include "lib_app/SinkStreamMd5.h"
#include "lib_app/MD5.h"
#include "lib_app/YuvIO.h"
#include "lib_app/convert.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
}

using namespace std;

class StreamMd5Calculator : public IFrameSink, Md5Calculator
{
public:
  StreamMd5Calculator(std::string& path) :
    Md5Calculator(path)
  {}

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    if(pBuf == EndOfStream)
    {
      Md5Output();
      return;
    }

    AL_TStreamMetaData* pMeta = reinterpret_cast<AL_TStreamMetaData*>(AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_STREAM));

    if(pMeta)
    {
      uint8_t* pStreamData = AL_Buffer_GetData(pBuf);

      for(int i = 0; i < pMeta->uNumSection; i++)
        m_MD5.Update(pStreamData + pMeta->pSections[i].uOffset, pMeta->pSections[i].uLength);
    }
  }
};

std::unique_ptr<IFrameSink> createStreamMd5Calculator(std::string path)
{
  return std::unique_ptr<IFrameSink>(new StreamMd5Calculator(path));
}
