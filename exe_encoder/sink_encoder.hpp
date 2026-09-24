// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include <cassert>
#include "lib_app/timing.hpp"
#include "lib_app/convert.hpp"
#include "IEncoderSink.hpp"
#include "QPGenerator.hpp"
#include "EncCmdMngr.hpp"
#include "CfgParser.hpp"
#include "CommandsSender.hpp"
#include "sink_ratectrl_meta.hpp"

#include "HDRParser.hpp"

#include "TwoPassMngr.hpp"

extern "C"
{
#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_common/BufferLookAheadMeta.h"
}

#include <string>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <map>
#include <functional>
#include <algorithm>

#include "RCPlugin.hpp"
#define NUM_PASS_OUTPUT 1

#define MAX_NUM_REC_OUTPUT (MAX_ENC_NUM_LAYER > NUM_PASS_OUTPUT ? MAX_ENC_NUM_LAYER : NUM_PASS_OUTPUT)
#define MAX_NUM_BITSTREAM_OUTPUT NUM_PASS_OUTPUT

static std::string PictTypeToString(AL_ESliceType type)
{
  std::map<AL_ESliceType, std::string> m =
  {
    { AL_SLICE_B, "B" },
    { AL_SLICE_P, "P" },
    { AL_SLICE_I, "I" },
    { AL_SLICE_GOLDEN, "Golden" },
    { AL_SLICE_CONCEAL, "Conceal" },
    { AL_SLICE_SKIP, "Skip" },
    { AL_SLICE_REPEAT, "Repeat" },
  };

  return m.at(type);
}

static AL_ERR PreprocessQP(AL_TBuffer* pQpBuf, AL_EGenerateQpMode eMode, const AL_TEncChanParam& tChParam, const std::string& sQPTablesFolder, AL_64S iFrameCountSent)
{
  auto iQPTableDepth = 0;

  int32_t minMaxQPSize = (int)(sizeof(tChParam.tRCParam.iMaxQP) / sizeof(tChParam.tRCParam.iMaxQP[0]));

  return GenerateQPBuffer(eMode, tChParam.tRCParam.iInitialQP,
                          *std::max_element(tChParam.tRCParam.iMinQP, tChParam.tRCParam.iMinQP + minMaxQPSize),
                          *std::min_element(tChParam.tRCParam.iMaxQP, tChParam.tRCParam.iMaxQP + minMaxQPSize),
                          AL_GetWidthInLCU(tChParam), AL_GetHeightInLCU(tChParam),
                          tChParam.eProfile, tChParam.uLog2MaxCuSize, iQPTableDepth, sQPTablesFolder,
                          iFrameCountSent, pQpBuf);
}

class QPBuffers
{
public:
  struct QPLayerInfo
  {
    BufPool* bufPool;
    std::string sQPTablesFolder;
    std::string sRoiFileName;
  };

  QPBuffers() {};

  void Configure(AL_TEncSettings const* pSettings, AL_EGenerateQpMode mode)
  {
    isExternQpTable = AL_IS_QP_TABLE_REQUIRED(pSettings->eQpTableMode);
    this->pSettings = pSettings;
    this->mode = mode;
  }

  ~QPBuffers(void)
  {

    for(auto roiCtx = mQPLayerRoiCtxs.begin(); roiCtx != mQPLayerRoiCtxs.end(); roiCtx++)
      AL_RoiMngr_Destroy(roiCtx->second);

  }

  void AddBufPool(QPLayerInfo& qpLayerInfo, int32_t iLayerID)
  {
    initLayer(qpLayerInfo, iLayerID);
  }

  AL_TBuffer* getBuffer(AL_64S frameNum)
  {
    return getBufferP(frameNum, 0);
  }

  void releaseBuffer(AL_TBuffer* buffer)
  {
    if(!isExternQpTable || !buffer)
      return;
    AL_Buffer_Unref(buffer);
  }

private:
  void initLayer(QPLayerInfo& qpLayerInfo, int32_t iLayerID)
  {
    mQPLayerInfos[iLayerID] = qpLayerInfo;
    auto& tChParam = pSettings->tChParam[iLayerID];
    mQPLayerRoiCtxs[iLayerID] = AL_RoiMngr_Create(tChParam.uEncWidth, tChParam.uEncHeight, tChParam.eProfile, tChParam.uLog2MaxCuSize, AL_ROI_QUALITY_MEDIUM, AL_ROI_QUALITY_ORDER);
  }

  AL_TBuffer* getBufferP(AL_64S frameNum, int32_t iLayerID)
  {
    if(!isExternQpTable || mQPLayerInfos.find(iLayerID) == mQPLayerInfos.end())
      return nullptr;

    auto& layerInfo = mQPLayerInfos[iLayerID];
    auto& tLayerChParam = pSettings->tChParam[iLayerID];

    AL_TBuffer* pQpBuf = layerInfo.bufPool->GetBuffer();

    if(!pQpBuf)
      throw std::runtime_error("Invalid QP buffer");

    std::string sErrorMsg = "Error loading external QP tables.";

    if(AL_GENERATE_ROI_QP == (AL_EGenerateQpMode)(mode & AL_GENERATE_QP_TABLE_MASK))
    {
      auto iQPTableDepth = 0;

      AL_ERR bRetROI = GenerateROIBuffer(mQPLayerRoiCtxs[iLayerID], layerInfo.sRoiFileName,
                                         AL_GetWidthInLCU(tLayerChParam), AL_GetHeightInLCU(tLayerChParam),
                                         tLayerChParam.eProfile, tLayerChParam.uLog2MaxCuSize, iQPTableDepth,
                                         frameNum, AL_Buffer_GetData(pQpBuf) + EP2_BUF_QP_BY_MB.Offset);

      if(!AL_IS_SUCCESS_CODE(bRetROI))
      {
        releaseBuffer(pQpBuf);
        switch(bRetROI)
        {
        case AL_ERR_CANNOT_OPEN_FILE:
          sErrorMsg = "Error loading ROI file.";
          throw std::runtime_error(sErrorMsg);
        default:
          break;
        }
      }
    }
    else
    {
      AL_ERR bRetQP = PreprocessQP(pQpBuf, mode, tLayerChParam, layerInfo.sQPTablesFolder, frameNum);

      auto releaseQPBuf = [&](std::string sErrorMsg, bool bThrow)
                          {
                            (void)sErrorMsg;
                            releaseBuffer(pQpBuf);
                            pQpBuf = NULL;

                            if(bThrow)
                              throw std::runtime_error(sErrorMsg);
                          };
      switch(bRetQP)
      {
      case AL_SUCCESS:
        break;
      case AL_ERR_QPLOAD_DATA:
        releaseQPBuf(AL_Codec_ErrorToString(bRetQP), true);
        break;
      case AL_ERR_QPLOAD_NOT_ENOUGH_DATA:
        releaseQPBuf(AL_Codec_ErrorToString(bRetQP), true);
        break;
      case AL_ERR_CANNOT_OPEN_FILE:
      {
        bool bThrow = false;
        bThrow = true;
        releaseQPBuf("Cannot open QP file.", bThrow);
        break;
      }
      default:
        break;
      }
    }

    return pQpBuf;
  }

private:
  bool isExternQpTable;
  AL_TEncSettings const* pSettings;
  AL_EGenerateQpMode mode;
  std::map<int, QPLayerInfo> mQPLayerInfos;
  std::map<int, AL_TRoiMngrCtx*> mQPLayerRoiCtxs;
};

struct safe_ifstream
{
  std::ifstream fp {};
  safe_ifstream(std::string const& filename, bool binary)
  {
    /* support no file at all */
    if(filename.empty())
      return;
    OpenInput(fp, filename, binary);
  }
};

struct EncoderSink : IEncoderSink
{

  explicit EncoderSink(ConfigFile const& cfg, AL_IEncScheduler* pScheduler, AL_TAllocator* pAllocator) :
    CmdFile(cfg.sCmdFileName, false),
    EncCmd(CmdFile.fp, cfg.RunInfo.iScnChgLookAhead, cfg.Settings.tChParam[0].tGopParam.uFreqLT),
    m_cfg(cfg),
    twoPassMngr(cfg.sTwoPassFileName, cfg.Settings.TwoPass, cfg.Settings.bEnableFirstPassSceneChangeDetection, cfg.Settings.tChParam[0].tGopParam.uGopLength,
                cfg.Settings.tChParam[0].tRCParam.uCPBSize / 90, cfg.Settings.tChParam[0].tRCParam.uInitialRemDelay / 90, cfg.MainInput.FileInfo.FrameRate),
    pAllocator{pAllocator},
    pSettings{&cfg.Settings},
    tLastEncodedDim{cfg.Settings.tChParam[0].uSrcWidth, cfg.Settings.tChParam[0].uSrcHeight}
  {
    AL_CB_EndEncoding onEncoding = { &EncoderSink::EndEncoding, this };

    qpBuffers.Configure(&cfg.Settings, cfg.RunInfo.eGenerateQpMode);

    AL_ERR errorCode = AL_Encoder_Create(&hEnc, pScheduler, this->pAllocator, &cfg.Settings, onEncoding);

    if(AL_IS_ERROR_CODE(errorCode))
      throw codec_error(AL_Codec_ErrorToString(errorCode), errorCode);

    if(AL_IS_WARNING_CODE(errorCode))
      LogWarning("%s\n", AL_Codec_ErrorToString(errorCode));

    commandsSender.reset(new CommandsSender(hEnc));

    for(int32_t i = 0; i < MAX_NUM_REC_OUTPUT; ++i)
      RecOutput[i].reset(new NullFrameSink);

    for(int32_t i = 0; i < MAX_NUM_BITSTREAM_OUTPUT; i++)
      BitstreamOutput[i].reset(new NullFrameSink);

    for(int32_t i = 0; i < MAX_ENC_NUM_LAYER; i++)
      m_input_picCount[i] = 0;

    m_pictureType = cfg.RunInfo.printPictureType ? AL_SLICE_MAX_ENUM : -1;

    if(!cfg.sHDRFileName.empty())
    {
      hdrParser.reset(new HDRParser(cfg.sHDRFileName));
      ReadHDR(0);
    }

    iPendingStreamCnt = 1;

  }

  void ReadHDR(int32_t iHDRIdx)
  {
    AL_THDRSEIs tHDRSEIs;

    if(!hdrParser->ReadHDRSEIs(tHDRSEIs, iHDRIdx))
      throw std::runtime_error("Failed to parse HDR File.");

    AL_Encoder_SetHDRSEIs(hEnc, &tHDRSEIs);
  }

  ~EncoderSink(void) override
  {
    LogInfo("%d pictures encoded. Average FrameRate = %.4f Fps\n",
            m_input_picCount[0], (m_input_picCount[0] * 1000.0) / (m_EndTime - m_StartTime));

    AL_Encoder_Destroy(hEnc);
  }

  void SetChangeSourceCallback(ChangeSourceCallback changeSourceCB) override
  {
    m_changeSourceCB = changeSourceCB;
  }

  void AddQpBufPool(QPBuffers::QPLayerInfo qpInf, int32_t iLayerID)
  {
    qpBuffers.AddBufPool(qpInf, iLayerID);
  }

  std::function<void(void)> m_done;

  void PreprocessFrame() override
  {
    commandsSender->Reset();
    EncCmd.Process(commandsSender.get(), m_input_picCount[0]);

    int32_t iIdx;

    if(commandsSender->HasInputChanged(iIdx))
      RequestSourceChange(iIdx, 0);

    if(commandsSender->HasHDRChanged(iIdx))
      ReadHDR(iIdx);
  }

  void ProcessFrame(AL_TBuffer* Src) override
  {
    if(m_input_picCount[0] == 0)
      m_StartTime = GetPerfTime();

    if(!Src)
    {
      LogVerbose("Flushing...\n\n");

      if(!AL_Encoder_Process(hEnc, nullptr, nullptr))
        CheckErrorAndThrow();
      return;
    }

    DisplayFrameStatus(m_input_picCount[0]);

    CheckSourceResolutionChanged(Src);

    if(twoPassMngr.iPass)
    {
      auto pPictureMetaTP = AL_TwoPassMngr_CreateAndAttachTwoPassMetaData(Src);

      if(twoPassMngr.iPass == 2)
        twoPassMngr.GetFrame(pPictureMetaTP);
    }

    AL_TBuffer* QpBuf = qpBuffers.getBuffer(m_input_picCount[0]);

    std::shared_ptr<AL_TBuffer> QpBufShared(QpBuf, [&](AL_TBuffer* pBuf) { qpBuffers.releaseBuffer(pBuf); });

    if(pSettings->hRcPluginDmaContext != NULL)
      RCPlugin_SetNextFrameQP(pSettings, this->pAllocator);

    if(!AL_Encoder_Process(hEnc, Src, QpBuf))
      CheckErrorAndThrow();

    m_input_picCount[0]++;
  }

  AL_ERR GetLastError(void) override
  {
    return m_EncoderLastError;
  }

  std::unique_ptr<IFrameSink> RecOutput[MAX_NUM_REC_OUTPUT];
  std::unique_ptr<IFrameSink> BitstreamOutput[MAX_NUM_BITSTREAM_OUTPUT];
  AL_HEncoder hEnc;
  bool shouldAddDummySei = false;

private:
  AL_64S iPendingStreamCnt;
  AL_64S m_input_picCount[MAX_ENC_NUM_LAYER] {};
  AL_64S m_pictureType = -1;
  AL_64U m_StartTime = 0;
  AL_64U m_EndTime = 0;
  safe_ifstream CmdFile;
  CEncCmdMngr EncCmd;
  ConfigFile const& m_cfg;
  TwoPassMngr twoPassMngr;
  QPBuffers qpBuffers;
  std::unique_ptr<CommandsSender> commandsSender;
  std::unique_ptr<HDRParser> hdrParser;

  AL_TAllocator* pAllocator;
  AL_TEncSettings const* pSettings;

  ChangeSourceCallback m_changeSourceCB;
  AL_TDimension tLastEncodedDim;
  AL_ERR m_EncoderLastError = AL_SUCCESS;

  void CheckErrorAndThrow(void)
  {
    AL_ERR eErr = AL_Encoder_GetLastError(hEnc);
    throw std::runtime_error(AL_IS_ERROR_CODE(eErr) ? AL_Codec_ErrorToString(eErr) : "Failed");
  }

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
    auto pThis = (EncoderSink*)userParam;

    if(isStreamReleased(pStream, pSrc) || isSourceReleased(pStream, pSrc))
      return;

    if(pThis->twoPassMngr.iPass == 1)
    {
      if(!pSrc)
        pThis->twoPassMngr.Flush();
      else
      {
        auto pPictureMetaTP = (AL_TLookAheadMetaData*)AL_Buffer_GetMetaData(pSrc, AL_META_TYPE_LOOKAHEAD);
        pThis->twoPassMngr.AddFrame(pPictureMetaTP);
      }
    }

    pThis->processOutput(pStream);
  }

  void ComputeQualityMeasure(AL_TRateCtrlMetaData* pMeta)
  {
    if(!pMeta->bFilled)
      return;
  }

  void AddSei(AL_TBuffer* pStream, bool isPrefix, int32_t payloadType, uint8_t* payload, int32_t payloadSize, int32_t tempId)
  {
    int32_t seiSection = AL_Encoder_AddSei(hEnc, pStream, isPrefix, payloadType, payload, payloadSize, tempId);

    if(seiSection < 0)
      LogWarning("Failed to add dummy SEI (id:%d) \n", seiSection);
  }

  AL_ERR PreprocessOutput(AL_TBuffer* pStream)
  {
    AL_ERR eErr = AL_Encoder_GetLastError(hEnc);

    if(AL_IS_ERROR_CODE(eErr))
    {
      LogError("%s\n", AL_Codec_ErrorToString(eErr));
      m_EncoderLastError = eErr;
    }

    if(AL_IS_WARNING_CODE(eErr))
      LogWarning("%s\n", AL_Codec_ErrorToString(eErr));

    if(pStream && shouldAddDummySei)
    {
      constexpr int32_t payloadSize = 8 * 10;
      uint8_t payload[payloadSize];

      for(int32_t i = 0; i < payloadSize; ++i)
        payload[i] = i;

      AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
      AddSei(pStream, false, 15, payload, payloadSize, pStreamMeta->uTemporalID);
      AddSei(pStream, true, 18, payload, payloadSize, pStreamMeta->uTemporalID);
    }

    if(pStream == EndOfStream)
      iPendingStreamCnt--;
    else
    {
      int32_t iStreamId = 0;

      if(m_pictureType != -1)
      {
        auto const pMeta = (AL_TPictureMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_PICTURE);
        m_pictureType = pMeta->eType;
        LogInfo("Picture Type %s (%i) %s\n", PictTypeToString(pMeta->eType).c_str(), m_pictureType, pMeta->bSkipped ? "is skipped" : "");
      }

      AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_RATECTRL);

      if(pMeta && pMeta->bFilled)
      {
      }

      BitstreamOutput[iStreamId]->ProcessFrame(pStream);
    }

    return AL_SUCCESS;
  }

  void CloseOutputs(void)
  {
    m_EndTime = GetPerfTime();
    m_done();
  }

  void CheckAndAllocateConversionBuffer(TFourCC tConvFourCC, AL_TDimension const& tConvDim, std::shared_ptr<AL_TBuffer>& pConvYUV)
  {
    if(pConvYUV != nullptr)
    {
      AL_TDimension tCurrentConvDim = AL_PixMapBuffer_GetDimension(pConvYUV.get());

      if(tCurrentConvDim.iHeight >= tConvDim.iHeight && tCurrentConvDim.iWidth >= tConvDim.iWidth)
        return;
    }

    AL_TBuffer* pYuv = AllocateDefaultYuvIOBuffer(tConvDim, tConvFourCC);

    if(pYuv == nullptr)
      throw std::runtime_error("Couldn't allocate reconstruct conversion buffer");

    pConvYUV = std::shared_ptr<AL_TBuffer>(pYuv, &AL_Buffer_Destroy);

  }

  void RecToYuv(AL_TBuffer const* pRec, AL_TBuffer* pYuv, TFourCC tYuvFourCC)
  {
    TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pRec);
    tConvFourCCFunc pFunc = GetConvFourCCFunc(tRecFourCC, tYuvFourCC);

    AL_PixMapBuffer_SetDimension(pYuv, AL_PixMapBuffer_GetDimension(pRec));

    if(!pFunc)
      throw std::runtime_error("Can't find a conversion function suitable for format");

    if(AL_IsTiled(tRecFourCC) == false)
      throw std::runtime_error("FourCC must be in Tile mode");
    return pFunc(pRec, pYuv);
  }

  void processOutput(AL_TBuffer* pStream)
  {
    AL_ERR eErr;
    {
      eErr = PreprocessOutput(pStream);
    }

    if(AL_IS_ERROR_CODE(eErr))
    {
      LogError("%s\n", AL_Codec_ErrorToString(eErr));
      m_EncoderLastError = eErr;
    }

    if(AL_IS_WARNING_CODE(eErr))
      LogWarning("%s\n", AL_Codec_ErrorToString(eErr));

    bool bPushBufferBack = pStream != NULL;

    if(bPushBufferBack)
    {
      auto const bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream);

      if(!bRet)
        throw std::runtime_error("AL_Encoder_PutStreamBuffer must always succeed");
    }

    AL_TRecPic RecPic;

    while(AL_Encoder_GetRecPicture(hEnc, &RecPic))
    {
      auto buf = RecPic.pBuf;
      int32_t iRecId = 0;

      if(buf)
      {
        TFourCC tFileRecFourCC = m_cfg.RecFourCC;
        AL_Buffer_InvalidateMemory(buf);

        {
          if(AL_PixMapBuffer_GetFourCC(buf) != tFileRecFourCC)
          {
            std::shared_ptr<AL_TBuffer> bufPostConv;
            CheckAndAllocateConversionBuffer(tFileRecFourCC, AL_PixMapBuffer_GetDimension(buf), bufPostConv);
            RecToYuv(buf, bufPostConv.get(), tFileRecFourCC);
            RecOutput[iRecId]->ProcessFrame(bufPostConv.get());
          }
          else
            RecOutput[iRecId]->ProcessFrame(buf);
        }
      }
      AL_Encoder_ReleaseRecPicture(hEnc, &RecPic);
    }

    if(iPendingStreamCnt == 0)
      CloseOutputs();
  }

  void RequestSourceChange(int32_t iInputIdx, int32_t iLayerIdx)
  {
    if(m_changeSourceCB)
      m_changeSourceCB(iInputIdx, iLayerIdx);
  }

  void CheckSourceResolutionChanged(AL_TBuffer* pSrc)
  {
    (void)pSrc;
    AL_TDimension tNewDim = AL_PixMapBuffer_GetDimension(pSrc);
    bool bDimensionChanged = tNewDim.iWidth != tLastEncodedDim.iWidth || tNewDim.iHeight != tLastEncodedDim.iHeight;

    if(bDimensionChanged)
    {
      AL_Encoder_SetInputResolution(hEnc, tNewDim);
      tLastEncodedDim = tNewDim;
    }
  }
};
