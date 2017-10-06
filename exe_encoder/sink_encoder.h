/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#pragma once

#include "lib_app/timing.h"

static void PreprocessQP(uint8_t* pQPs, const AL_TEncSettings& Settings, int iFrameCountSent)
{
  uint8_t* pSegs = NULL;
  bool bRet = PreprocessQP(Settings.eQpCtrlMode, Settings.tChParam.tRCParam.iInitialQP,
                           Settings.tChParam.tRCParam.iMinQP, Settings.tChParam.tRCParam.iMaxQP,
                           AL_GetWidthInLCU(Settings.tChParam), AL_GetHeightInLCU(Settings.tChParam),
                           Settings.tChParam.eProfile, iFrameCountSent, pQPs + EP2_BUF_QP_BY_MB.Offset, pSegs);
  assert(bRet);
}

class QPBuffers
{
public:
  QPBuffers(AL_TBufPool& bufpool, const AL_TEncSettings& settings) :
    bufpool(bufpool), isExternQpTable(settings.eQpCtrlMode & (MASK_QP_TABLE_EXT)), settings(settings)
  {
  }

  AL_TBuffer* getBuffer(int frameNum)
  {
    if(!isExternQpTable)
      return nullptr;

    AL_TBuffer* QpBuf = AL_BufPool_GetBuffer(&bufpool, AL_BUF_MODE_BLOCK);
    PreprocessQP(AL_Buffer_GetData(QpBuf), settings, frameNum);
    return QpBuf;
  }

  void releaseBuffer(AL_TBuffer* buffer)
  {
    if(!isExternQpTable || !buffer)
      return;
    AL_Buffer_Unref(buffer);
  }

private:
  AL_TBufPool& bufpool;
  bool isExternQpTable;
  const AL_TEncSettings& settings;
};

class SceneChange
{
public:
  SceneChange(string sScnChgFileName, int lookahead, int inputFrameRate, int outputFrameRate) :
    lookahead(lookahead),
    inputFrameRate(inputFrameRate),
    outputFrameRate(outputFrameRate)
  {
    if(!sScnChgFileName.empty())
    {
      OpenInput(ScnChgFile, sScnChgFileName, false);

      bScnChgFileIn = true;
      iNextScnChg = ReadNextFrame(ScnChgFile);
    }
  }

  void notify(AL_HEncoder hEnc, unsigned int iPictCount, int framesRead)
  {
    if(bScnChgFileIn)
    {
      int iAheadCount = ((iPictCount + lookahead) * inputFrameRate) / outputFrameRate;

      while((iNextScnChg != -1) && (framesRead < iNextScnChg) && (iAheadCount >= iNextScnChg))
      {
        AL_Encoder_NotifySceneChange(hEnc, lookahead);
        iNextScnChg = ReadNextFrame(ScnChgFile);
      }
    }
  }

private:
  ifstream ScnChgFile;
  bool bScnChgFileIn = false;
  int iNextScnChg = -1;
  int const lookahead;
  int const inputFrameRate;
  int const outputFrameRate;
};

class LongTermRef
{
public:
  LongTermRef(string sLTFileName, int longTermFreq) : longTermFreq(longTermFreq)
  {
    if(!sLTFileName.empty())
    {
      OpenInput(LTFile, sLTFileName, false);

      iNextLT = ReadNextFrame(LTFile);
    }
  }

  void notify(AL_HEncoder hEnc)
  {
    if(LTFile.is_open() && longTermFreq > 0)
    {
      if((iNextLT != -1) && (iCurFrame == iNextLT))
      {
        AL_Encoder_NotifyLongTerm(hEnc);
        iNextLT = ReadNextFrame(LTFile);
      }

      iCurFrame++;
    }
  }

private:
  ifstream LTFile;
  int iNextLT = -1;
  int iCurFrame = 0;
  int const longTermFreq;
};


static AL_ERR g_EncoderLastError = AL_SUCCESS;

AL_ERR GetEncoderLastError()
{
  return g_EncoderLastError;
}

static
void ThrowEncoderError(AL_ERR eErr)
{
  switch(eErr)
  {
  case AL_ERR_STREAM_OVERFLOW: Message(CC_RED, "Stream Error : Stream overflow\n");
    break;
  case AL_ERR_TOO_MANY_SLICES: Message(CC_RED, "Stream Error : Too many slices\n");
    break;
  case AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE: throw codec_error("Encoder failed : Channel creation failed, no channel available", eErr);
    break;
  case AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE: throw codec_error("Encoder failed : Channel creation failed, processing power of the available cores insufficient", eErr);
    break;
  case AL_ERR_CHAN_CREATION_NOT_ENOUGH_CORES: throw codec_error("Encoder failed : Channel creation failed, couldn't spread the load on enough cores", eErr);
    break;
  case AL_ERR_REQUEST_MALFORMED: throw codec_error("Encoder failed : Channel creation failed, request was malformed", eErr);
    break;
  case AL_ERR_NO_MEMORY: throw codec_error("Encoder failed : Memory shortage detected (dma, embedded memory or virtual memory shortage)", eErr);
    break;
#if ENABLE_WATCHDOG
  case AL_ERR_WATCHDOG_TIMEOUT: throw codec_error("Encoder failed : Watch dog timeout", eErr);
    break;
#endif
  case AL_WARN_LCU_OVERFLOW: Message(CC_RED, "Warning some LCU exceed the maximum allowed bits\n");
    break;
  case AL_SUCCESS: /* do nothing */ break;

  default: throw codec_error("Encoder failed : Unknown error !!", eErr);
    break;
  }

  if(eErr != AL_SUCCESS)
    g_EncoderLastError = eErr;
}

struct EncoderSink : IFrameSink
{
  EncoderSink(string sScnChgFileName, string sLTFileName, string sGMVFileName, ConfigFile const& cfg, TScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TBufPool& qpBufPool) :
    ScnChg(sScnChgFileName,
           cfg.RunInfo.iScnChgLookAhead,
           cfg.FileInfo.FrameRate,
           cfg.Settings.tChParam.tRCParam.uFrameRate),
    LT(sLTFileName, cfg.Settings.tChParam.tGopParam.uFreqLT),
    qpBuffers(qpBufPool, cfg.Settings)
  {
    (void)sGMVFileName;
    AL_CB_EndEncoding onEndEncoding = { &EncoderSink::EndEncoding, this };

    AL_ERR errorCode = AL_Encoder_Create(&hEnc, pScheduler, pAllocator, &cfg.Settings, onEndEncoding);

    if(errorCode)
      ThrowEncoderError(errorCode);

    BitstreamOutput.reset(new NullFrameSink);
    RecOutput.reset(new NullFrameSink);
  }

  ~EncoderSink()
  {
    Message(CC_DEFAULT, "\n\n%d pictures encoded. Average FrameRate = %.4f Fps\n",
            m_picCount, (m_picCount * 1000.0) / (m_EndTime - m_StartTime));

    AL_Encoder_Destroy(hEnc);
  }

  std::function<void(void)> m_done;

  void ProcessFrame(AL_TBuffer* Src) override
  {
    if(m_picCount == 0)
      m_StartTime = GetPerfTime();

    if(Src)
      DisplayFrameStatus(m_picCount);
    else
      Message(CC_DEFAULT, "Flushing...");

    AL_TBuffer* QpBuf = nullptr;

    if(Src)
    {
      ScnChg.notify(hEnc, m_picCount, m_picCount + 1);

      LT.notify(hEnc);
      QpBuf = qpBuffers.getBuffer(m_picCount);
    }

    if(!AL_Encoder_Process(hEnc, Src, QpBuf))
      throw runtime_error("Failed");

    qpBuffers.releaseBuffer(QpBuf);

    if(Src)
      m_picCount++;
  }

  unique_ptr<IFrameSink> RecOutput;
  unique_ptr<IFrameSink> BitstreamOutput;
  AL_HEncoder hEnc;

private:
  int m_picCount = 0;
  uint64_t m_StartTime = 0;
  uint64_t m_EndTime = 0;
  SceneChange ScnChg;
  LongTermRef LT;
  QPBuffers qpBuffers;

  static inline bool isEOS(AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    return !pStream && !pSrc;
  }

  static inline bool isEncodedFrame(AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    return pStream && pSrc;
  }

  static inline bool isStreamReleased(AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    return pStream && !pSrc;
  }

  static void EndEncoding(void* userParam, AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    auto pThis = (EncoderSink*)userParam;

    if(!isEncodedFrame(pStream, pSrc) && !isEOS(pStream, pSrc) && !isStreamReleased(pStream, pSrc))
      assert(0);

    if(isStreamReleased(pStream, pSrc))
      return;

    pThis->processOutput(pStream);
  }

  void processOutput(AL_TBuffer* pStream)
  {
    if(AL_ERR eErr = AL_Encoder_GetLastError(hEnc))
      ThrowEncoderError(eErr);

    BitstreamOutput->ProcessFrame(pStream);

    if(pStream)
    {
      auto bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream);
      assert(bRet);
    }

    TRecPic RecPic;

    while(AL_Encoder_GetRecPicture(hEnc, &RecPic))
    {
      auto buf = WrapBufferYuv(&RecPic.tBuf);
      RecOutput->ProcessFrame(buf);
      AL_Buffer_Destroy(buf);

      AL_Encoder_ReleaseRecPicture(hEnc, &RecPic);
    }

    if(!pStream)
    {
      RecOutput->ProcessFrame(EndOfStream);
      m_EndTime = GetPerfTime();
      m_done();
    }
  }

  static AL_TBuffer* WrapBufferYuv(TBufferYuv* frame)
  {
    AL_TBuffer* pBuf = AL_Buffer_WrapData(frame->tMD.pVirtualAddr, frame->tMD.uSize, NULL);
    AL_TPitches tPitches = { frame->iPitchY, frame->iPitchC };
    AL_TOffsetYC tOffsetYC = { 0, frame->iPitchY * frame->iHeight };
    AL_TSrcMetaData* pBufMeta = AL_SrcMetaData_Create(frame->iWidth, frame->iHeight, tPitches, tOffsetYC, frame->tFourCC);
    AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pBufMeta);
    return pBuf;
  }
};

