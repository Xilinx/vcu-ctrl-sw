// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include "sink_yuv_md5.h"
#include "lib_app/YuvIO.h"
#include "lib_app/convert.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
}

using namespace std;

/****************************************************************************/
static void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tYuvFourCC)
{
  TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pRec);
  tConvFourCCFunc pFunc = GetConvFourCCFunc(tRecFourCC, tYuvFourCC);

  AL_PixMapBuffer_SetDimension(pYuv, AL_PixMapBuffer_GetDimension(pRec));

  if(!pFunc)
    throw runtime_error("Can't find a conversion function suitable for format");

  if(AL_IsTiled(tRecFourCC) == false)
    throw runtime_error("FourCC must be in Tile mode");
  return pFunc(pRec, pYuv);
}

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

