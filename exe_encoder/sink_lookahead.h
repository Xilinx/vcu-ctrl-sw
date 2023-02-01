/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#pragma once

#include "sink_encoder.h"

#include <memory>
#include <stdexcept>

/*
** Special EncoderSink structure, used for encoding the first pass
** The encoding settings are adapted for the first pass
** LookAhead Metadata is created and filled during the encoding
** Frames are stocked in a fifo after the first pass
** More informations are filled in the metadata while in the fifo (scene change, complexity)
** The frames are then sent to the real EncoderSink for the second pass, with first pass info in the metadata
*/
struct EncoderLookAheadSink : IFrameSink
{
  EncoderLookAheadSink(ConfigFile const& cfg, EncoderSink* pBaseSink, AL_IEncScheduler* pScheduler, AL_TAllocator* pAllocator) :
    CmdFile(cfg.sCmdFileName),
    EncCmd(CmdFile, cfg.RunInfo.iScnChgLookAhead, cfg.Settings.tChParam[0].tGopParam.uFreqLT),
    lookAheadMngr(cfg.Settings.LookAhead, cfg.Settings.bEnableFirstPassSceneChangeDetection)
  {
    (void)pBaseSink;

    BitstreamOutput.reset(new NullFrameSink);
    RecOutput.reset(new NullFrameSink);

    AL_CB_EndEncoding onEndEncoding = { &EncoderLookAheadSink::EndEncoding, this };
    cfgLA = cfg;
    AL_TwoPassMngr_SetPass1Settings(cfgLA.Settings);

    AL_Settings_CheckCoherency(&cfgLA.Settings, &cfgLA.Settings.tChParam[0], cfgLA.MainInput.FileInfo.FourCC, NULL);

    qpBuffers.Configure(&cfgLA.Settings, cfgLA.RunInfo.eGenerateQpMode);

    AL_ERR errorCode = AL_Encoder_Create(&hEnc, pScheduler, pAllocator, &cfgLA.Settings, onEndEncoding);

    if(errorCode)
      throw codec_error(AL_Codec_ErrorToString(errorCode), errorCode);

    commandsSender.reset(new CommandsSender(hEnc));
    m_pictureType = cfg.RunInfo.printPictureType ? AL_SLICE_MAX_ENUM : -1;

    bEnableFirstPassSceneChangeDetection = false;
    bEnableFirstPassSceneChangeDetection = cfg.Settings.bEnableFirstPassSceneChangeDetection;
    EOSFinished = Rtos_CreateEvent(false);
    FifoFlushFinished = Rtos_CreateEvent(false);
    iNumLayer = cfg.Settings.NumLayer;

    iNumFrameEnded = 0;

    m_maxpicCount = cfg.RunInfo.iMaxPict;
  }

  ~EncoderLookAheadSink()
  {
    AL_Encoder_Destroy(hEnc);
    Rtos_DeleteEvent(EOSFinished);
    Rtos_DeleteEvent(FifoFlushFinished);
  }

  void AddQpBufPool(QPBuffers::QPLayerInfo qpInf, int iLayerID)
  {
    qpBuffers.AddBufPool(qpInf, iLayerID);
  }

  void PreprocessFrame() override
  {
    EncCmd.Process(commandsSender.get(), m_picCount);
  }

  void ProcessFrame(AL_TBuffer* Src) override
  {
    AL_TBuffer* QpBuf = nullptr;

    if(Src)
    {

      auto pPictureMetaLA = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(Src, AL_META_TYPE_LOOKAHEAD);

      if(!pPictureMetaLA)
      {
        pPictureMetaLA = AL_LookAheadMetaData_Create();

        if(AL_Buffer_AddMetaData(Src, (AL_TMetaData*)pPictureMetaLA) == false)
          throw std::runtime_error("Add metadata shouldn't fail!");
      }
      AL_LookAheadMetaData_Reset(pPictureMetaLA);

      QpBuf = qpBuffers.getBuffer(m_picCount);
    }

    std::shared_ptr<AL_TBuffer> QpBufShared(QpBuf, [&](AL_TBuffer* pBuf) { qpBuffers.releaseBuffer(pBuf); });

    if(m_picCount <= m_maxpicCount)
    {
      AL_TBuffer* pSrc = (m_picCount == m_maxpicCount) ? nullptr : Src;

      if(!AL_Encoder_Process(hEnc, pSrc, QpBuf))
        throw std::runtime_error("Failed LA");
    }

    if(Src)
      m_picCount++;
    else if(iNumLayer == 1)
    {
      // the main process waits for the LookAhead to end so he can flush the fifo
      Rtos_WaitEvent(EOSFinished, AL_WAIT_FOREVER);
      ProcessFifo(true, false);
    }
  }

  AL_HEncoder hEnc;
  IFrameSink* next;
  std::unique_ptr<IFrameSink> BitstreamOutput;
  std::unique_ptr<IFrameSink> RecOutput;

private:
  int m_picCount = 0;
  int m_maxpicCount = -1;
  int m_pictureType = -1;
  std::ifstream CmdFile;
  CEncCmdMngr EncCmd;
  ConfigFile cfgLA;
  QPBuffers qpBuffers;
  std::unique_ptr<CommandsSender> commandsSender;
  LookAheadMngr lookAheadMngr;
  bool bEnableFirstPassSceneChangeDetection;
  AL_EVENT EOSFinished;
  AL_EVENT FifoFlushFinished;
  int iNumLayer;
  int iNumFrameEnded;

  static inline bool isStreamReleased(AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    return pStream && !pSrc;
  }

  static inline bool isSourceReleased(AL_TBuffer* pStream, AL_TBuffer const* pSrc)
  {
    return !pStream && pSrc;
  }

  static void EndEncoding(void* userParam, AL_TBuffer* pStream, AL_TBuffer const* pSrc, int)
  {
    auto pThis = (EncoderLookAheadSink*)userParam;

    if(isStreamReleased(pStream, pSrc) || isSourceReleased(pStream, pSrc))
      return;

    pThis->AddFifo(const_cast<AL_TBuffer*>(pSrc), pStream);

    pThis->processOutputLookAhead(pStream);
  }

  void processOutputLookAhead(AL_TBuffer* pStream)
  {
    BitstreamOutput->ProcessFrame(pStream);
    AL_ERR eErr = AL_Encoder_GetLastError(hEnc);

    if(AL_IS_ERROR_CODE(eErr))
    {
      LogError("%s\n", AL_Codec_ErrorToString(eErr));
      g_EncoderLastError = eErr;
    }

    if(AL_IS_WARNING_CODE(eErr))
      LogWarning("%s\n", AL_Codec_ErrorToString(eErr));

    if(pStream)
    {
      if(AL_Encoder_PutStreamBuffer(hEnc, pStream) == false)
        throw std::runtime_error("PutStreamBuffer shouldn't fail");
    }

    AL_TRecPic RecPic;

    while(AL_Encoder_GetRecPicture(hEnc, &RecPic))
    {
      RecOutput->ProcessFrame(RecPic.pBuf);
      AL_Encoder_ReleaseRecPicture(hEnc, &RecPic);
    }
  }

  void AddFifo(AL_TBuffer* pSrc, AL_TBuffer* pStream)
  {
    if(!pSrc)
    {
      Rtos_SetEvent(EOSFinished);
      return;
    }

    if(!pStream)
      return;

    bool bIsRepeat = false;
    AL_TPictureMetaData* pPictureMeta = (AL_TPictureMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_PICTURE);

    if(pPictureMeta == nullptr)
      throw std::runtime_error("pPictureMeta buffer must exist");
    bIsRepeat = pPictureMeta->eType == AL_SLICE_REPEAT;

    if(bIsRepeat)
      return;

    AL_Buffer_Ref(pSrc);
    lookAheadMngr.m_fifo.push_back(pSrc);

    ProcessFifo(false, false);

    ++iNumFrameEnded;
  }

  AL_TBuffer* GetSrcBuffer()
  {
    AL_TBuffer* pSrc = lookAheadMngr.m_fifo.front();
    lookAheadMngr.m_fifo.pop_front();
    return pSrc;
  }

  void ProcessFifo(bool isEOS, bool NoFirstPass)
  {
    auto iLASize = lookAheadMngr.uLookAheadSize;

    // Fifo is empty, we propagate the EndOfStream
    if(isEOS && lookAheadMngr.m_fifo.size() == 0)
    {
      next->PreprocessFrame();
      next->ProcessFrame(NULL);
    }
    // Fifo is full, or fifo must be emptied at EOS
    else if((lookAheadMngr.m_fifo.size() != 0) && (isEOS || iNumFrameEnded == iLASize || NoFirstPass))
    {
      iNumFrameEnded--;
      lookAheadMngr.ProcessLookAheadParams();
      AL_TBuffer* pSrc = GetSrcBuffer();

      next->PreprocessFrame();
      next->ProcessFrame(pSrc);
      AL_Buffer_Unref(pSrc);

      if(isEOS)
        ProcessFifo(isEOS, NoFirstPass);
    }
  }
};

