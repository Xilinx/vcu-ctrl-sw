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

#include "sink_frame_writer.h"

#include <stdexcept>

#include "lib_app/YuvIO.h"
#include "lib_app/convert.h"
#include "lib_app/utils.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common_enc/IpEncFourCC.h"
}

using namespace std;

/****************************************************************************/
void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tYuvFourCC)
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

class FrameWriter : public IFrameSink
{
public:
  FrameWriter(string RecFileName, ConfigFile& cfg_, int iLayerID) : m_cfg(cfg_)
  {
    (void)iLayerID; // if no fbc support
    OpenOutput(m_RecFile, RecFileName);
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    if(pBuf == EndOfStream)
    {
      m_RecFile.flush();
      return;
    }

    {
      if(AL_PixMapBuffer_GetFourCC(pBuf) != m_cfg.RecFourCC)
      {
        CheckAndAllocateConversionBuffer(pBuf);
        RecToYuv(pBuf, m_convYUV.get(), m_cfg.RecFourCC);
        WriteOneFrame(m_RecFile, m_convYUV.get());
      }
      else
        WriteOneFrame(m_RecFile, pBuf);
    }
  }

private:
  ofstream m_RecFile;
  ConfigFile& m_cfg;
  std::shared_ptr<AL_TBuffer> m_convYUV;

  void CheckAndAllocateConversionBuffer(AL_TBuffer* pBuf)
  {
    AL_TDimension tOutputDim = AL_PixMapBuffer_GetDimension(pBuf);

    if(m_convYUV != nullptr)
    {
      AL_TDimension tConvDim = AL_PixMapBuffer_GetDimension(m_convYUV.get());

      if(tConvDim.iHeight >= tOutputDim.iHeight && tConvDim.iWidth >= tOutputDim.iWidth)
        return;
    }

    AL_TBuffer* pYuv = AllocateDefaultYuvIOBuffer(tOutputDim, m_cfg.RecFourCC);

    if(pYuv == nullptr)
      throw runtime_error("Couldn't allocate reconstruct conversion buffer");

    m_convYUV = shared_ptr<AL_TBuffer>(pYuv, &AL_Buffer_Destroy);

  }
};

unique_ptr<IFrameSink> createFrameWriter(string path, ConfigFile& cfg_, int iLayerID_)
{

  if(cfg_.Settings.TwoPass == 1)
    return unique_ptr<IFrameSink>(new NullFrameSink);

  return unique_ptr<IFrameSink>(new FrameWriter(path, cfg_, iLayerID_));
}

