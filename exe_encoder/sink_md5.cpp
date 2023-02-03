/******************************************************************************
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

#include <fstream>
#include "sink_md5.h"
#include "lib_app/MD5.h"
#include "lib_app/YuvIO.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
}

void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tYuvFourCC);

class YuvMd5Calculator : public IFrameSink, Md5Calculator
{
public:
  YuvMd5Calculator(std::string& path, ConfigFile& cfg_) :
    Md5Calculator(path),
    fourcc(cfg_.RecFourCC)
  {
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    if(pBuf == EndOfStream)
    {
      Md5Output();
      return;
    }

    TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pBuf);

    if(tRecFourCC != fourcc && !AL_IsCompressed(tRecFourCC))
    {
      CheckAndAllocateConversionBuffer(pBuf);
      RecToYuv(pBuf, m_convYUV.get(), fourcc);
      pBuf = m_convYUV.get();
    }

    int iChunkCnt = AL_Buffer_GetChunkCount(pBuf);

    for(int i = 0; i < iChunkCnt; i++)
      m_MD5.Update(AL_Buffer_GetDataChunk(pBuf, i), AL_Buffer_GetSizeChunk(pBuf, i));
  }

private:
  std::shared_ptr<AL_TBuffer> m_convYUV;
  TFourCC const fourcc;

  void CheckAndAllocateConversionBuffer(AL_TBuffer* pBuf)
  {
    AL_TDimension tOutputDim = AL_PixMapBuffer_GetDimension(pBuf);

    if(m_convYUV != nullptr)
    {
      AL_TDimension tConvDim = AL_PixMapBuffer_GetDimension(m_convYUV.get());

      if(tConvDim.iHeight >= tOutputDim.iHeight && tConvDim.iWidth >= tOutputDim.iWidth)
        return;
    }

    AL_TBuffer* pYuv = AllocateDefaultYuvIOBuffer(tOutputDim, fourcc);

    if(pYuv == nullptr)
      throw std::runtime_error("Couldn't allocate reconstruct conversion buffer");

    m_convYUV = std::shared_ptr<AL_TBuffer>(pYuv, &AL_Buffer_Destroy);

  }
};

std::unique_ptr<IFrameSink> createYuvMd5Calculator(std::string path, ConfigFile& cfg_)
{
  return std::unique_ptr<IFrameSink>(new YuvMd5Calculator(path, cfg_));
}

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

