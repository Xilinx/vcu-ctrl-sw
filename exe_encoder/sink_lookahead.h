/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
  EncoderLookAheadSink(ConfigFile const& cfg, TScheduler* pScheduler, AL_TAllocator* pAllocator
                       ) :
    CmdFile(cfg.sCmdFileName),
    EncCmd(CmdFile, cfg.RunInfo.iScnChgLookAhead, cfg.Settings.tChParam[0].tGopParam.uFreqLT),
    qpBuffers(cfg.Settings),
    lookAheadMngr(cfg.Settings.LookAhead, cfg.Settings.bEnableFirstPassSceneChangeDetection)
  {
    AL_CB_EndEncoding onEndEncoding = { &EncoderLookAheadSink::EndEncoding, this };

    ConfigFile cfgLA = cfg;

    AL_TwoPassMngr_SetPass1Settings(cfgLA.Settings);
    AL_Settings_CheckCoherency(&cfgLA.Settings, &cfgLA.Settings.tChParam[0], cfgLA.MainInput.FileInfo.FourCC, NULL);

    AL_ERR errorCode = AL_Encoder_Create(&hEnc, pScheduler, pAllocator, &cfgLA.Settings, onEndEncoding);

    if(errorCode)
      ThrowEncoderError(errorCode);

    commandsSender.reset(new CommandsSender(hEnc));
    m_pictureType = cfg.RunInfo.printPictureType ? AL_SLICE_MAX_ENUM : -1;

    bEnableFirstPassSceneChangeDetection = cfg.Settings.bEnableFirstPassSceneChangeDetection;
    EOSFinished = Rtos_CreateEvent(false);
    iNumLayer = cfg.Settings.NumLayer;
  }

  ~EncoderLookAheadSink()
  {
    AL_Encoder_Destroy(hEnc);
    Rtos_DeleteEvent(EOSFinished);
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
        bool ret = AL_Buffer_AddMetaData(Src, (AL_TMetaData*)pPictureMetaLA);
        assert(ret);
      }
      AL_LookAheadMetaData_Reset(pPictureMetaLA);

      QpBuf = qpBuffers.getBuffer(m_picCount);
    }

    std::shared_ptr<AL_TBuffer> QpBufShared(QpBuf, [&](AL_TBuffer* pBuf) { qpBuffers.releaseBuffer(pBuf); });

    if(!AL_Encoder_Process(hEnc, Src, QpBuf))
      throw std::runtime_error("Failed LA");

    if(Src)
      m_picCount++;
    else if(iNumLayer == 1)
    {
      // the main process waits for the LookAhead to end so he can flush the fifo
      Rtos_WaitEvent(EOSFinished, AL_WAIT_FOREVER);
      ProcessFifo(true);
    }
  }


  AL_HEncoder hEnc;
  IFrameSink* next;

private:
  int m_picCount = 0;
  int m_pictureType = -1;
  std::ifstream CmdFile;
  CEncCmdMngr EncCmd;
  QPBuffers qpBuffers;
  std::unique_ptr<CommandsSender> commandsSender;
  LookAheadMngr lookAheadMngr;
  bool bEnableFirstPassSceneChangeDetection;
  AL_EVENT EOSFinished;
  int iNumLayer;

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

    pThis->AddFifo((AL_TBuffer*)pSrc);

    pThis->processOutputLookAhead(pStream);
  }

  void processOutputLookAhead(AL_TBuffer* pStream)
  {
    AL_ERR eErr = AL_Encoder_GetLastError(hEnc);

    if(AL_IS_ERROR_CODE(eErr))
      ThrowEncoderError(eErr);

    if(AL_IS_WARNING_CODE(eErr))
      Message(CC_YELLOW, "%s\n", EncoderErrorToString(eErr));

    if(pStream)
    {
      auto bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream);
      assert(bRet);
    }

    TRecPic RecPic;

    while(AL_Encoder_GetRecPicture(hEnc, &RecPic))
      AL_Encoder_ReleaseRecPicture(hEnc, &RecPic);
  }

  bool AddFifo(AL_TBuffer* pSrc)
  {
    bool bRet = true;

    if(pSrc)
    {
      AL_Buffer_Ref(pSrc);
      lookAheadMngr.m_fifo.push_back(pSrc);
      ProcessFifo(false);
    }
    else
      Rtos_SetEvent(EOSFinished);
    return bRet;
  }

  void ProcessFifo(bool isEOS)
  {
    // Fifo is empty, we propagate the EndOfStream
    if(isEOS && lookAheadMngr.m_fifo.size() == 0)
    {
      next->PreprocessFrame();
      next->ProcessFrame(NULL);
    }
    // Fifo is full, or fifo must be emptied at EOS
    else if(isEOS || lookAheadMngr.m_fifo.size() == lookAheadMngr.uLookAheadSize)
    {
      lookAheadMngr.ProcessLookAheadParams();
      AL_TBuffer* pSrc = lookAheadMngr.m_fifo.front();
      lookAheadMngr.m_fifo.pop_front();

      next->PreprocessFrame();
      next->ProcessFrame(pSrc);
      AL_Buffer_Unref(pSrc);


      if(isEOS)
        ProcessFifo(isEOS);
    }
  }
};


