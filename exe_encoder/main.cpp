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

#include <limits.h>
#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <list>
#include <sstream>
#include <stdarg.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "lib_app/console.h"
#include "lib_app/utils.h"

#include "CodecUtils.h"
#include "sink.h"
#include "IpDevice.h"

#include "resource.h"

#include "lib_cfg/lib_cfg.h"

extern "C"
{
#include "lib_app/BufPool.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/versions.h"
#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_preprocess/PreprocessQP.h"
#include "lib_conv_yuv/lib_conv_yuv.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "sink_encoder.h"
#include "sink_bitstream_writer.h"
#include "sink_frame_writer.h"
#include "sink_md5.h"
#include "sink_repeater.h"

int g_numFrameToRepeat;

using namespace std;

/*****************************************************************************/
void DisplayVersionInfo()
{
  Message(CC_DEFAULT, "%s - %s v%d.%d.%d - %s\n", IP_ENCODER_COMPANY,
          IP_ENCODER_PRODUCT_NAME,
          IP_VERSION,
          IP_ENCODER_COPYRIGHT);

  Message(CC_YELLOW, "%s\n", IP_ENCODER_COMMENTS);
}

/*****************************************************************************/
void SetDefaults(ConfigFile& cfg)
{
  cfg.BitstreamFileName = "Stream.bin";
  cfg.RecFourCC = FOURCC(NULL);
  AL_Settings_SetDefaults(&cfg.Settings);
  cfg.FileInfo.FourCC = FOURCC(I420);
  cfg.FileInfo.FrameRate = 0;
  cfg.FileInfo.PictHeight = 0;
  cfg.FileInfo.PictWidth = 0;
  cfg.CfgTrace.sPath = ".";
  cfg.CfgTrace.eMode = AL_TRACE_NONE;
  cfg.CfgTrace.iFrame = 0;
  cfg.RunInfo.bUseBoard = true;
  cfg.RunInfo.iSchedulerType = SCHEDULER_TYPE_MCU;
  cfg.RunInfo.bLoop = false;
  cfg.RunInfo.iMaxPict = INT_MAX; // ALL
  cfg.RunInfo.iFirstPict = 0;
  cfg.RunInfo.iScnChgLookAhead = 3;
  cfg.RunInfo.ipCtrlMode = IPCTRL_MODE_STANDARD;

  cfg.strict_mode = false;
}

#include "lib_app/CommandLineParser.h"

static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cerr << "Usage: " << ExeName << " -cfg <configfile> [options]" << endl;
  cerr << "Options:" << endl;

  for(auto& name : opt.displayOrder)
  {
    auto& o = opt.options.at(name);
    cerr << "  " << o.desc << endl;
  }

  cerr << "Examples:" << endl;
  cerr << "  " << ExeName << " -cfg test/config/encode_simple.cfg -r rec.yuv -o output.hevc -i input.yuv" << endl;
}

static AL_EChromaMode stringToChromaMode(string s)
{
  if(s == "CHROMA_MONO")
    return CHROMA_MONO;

  if(s == "CHROMA_4_0_0")
    return CHROMA_4_0_0;

  if(s == "CHROMA_4_2_0")
    return CHROMA_4_2_0;

  if(s == "CHROMA_4_2_2")
    return CHROMA_4_2_2;
  return CHROMA_MAX_ENUM;
}

/*****************************************************************************/
void ParseCommandLine(int argc, char** argv, ConfigFile& cfg)
{
  bool DoNotAcceptCfg = false;
  bool help = false;
  auto opt = CommandLineParser([&](string word)
  {
    if(word == "-cfg" || word == "-cfg-permissive")
    {
      if(DoNotAcceptCfg)
        throw runtime_error("Configuration files should be specified first, use -h to get help");
    }
    else
      DoNotAcceptCfg = true;
  });

  opt.addOption("-cfg", [&]()
  {
    auto const cfgPath = opt.popWord();
    cfg.strict_mode = true;
    ParseConfigFile(cfgPath, cfg);
  }, "Specify configuration file");

  opt.addOption("-cfg-permissive", [&]()
  {
    auto const cfgPath = opt.popWord();
    ParseConfigFile(cfgPath, cfg);
  }, "Use it instead of -cfg. Errors in the configuration file will be ignored");

  opt.addFlag("--help,-h", &help, "Show this help");
  opt.addString("--input,-i", &cfg.YUVFileName, "YUV input file");
  opt.addString("--output,-o", &cfg.BitstreamFileName, "Compressed output file");
  opt.addString("--md5", &cfg.RunInfo.sMd5Path, "Path to the output MD5 textfile");
  opt.addString("--output-rec,-r", &cfg.RecFileName, "Output reconstructed YUV file");
  opt.addOption("--color", [&]() {
    SetEnableColor(true);
  }, "Enable color");

  opt.addFlag("--quiet,-q", &g_Verbosity, "do not output anything", 0);

  opt.addInt("--input-width", &cfg.FileInfo.PictWidth, "Specify YUV input width");
  opt.addInt("--input-height", &cfg.FileInfo.PictHeight, "Specify YUV input height");
  opt.addOption("--chroma-mode", [&]()
  {
    auto chromaMode = stringToChromaMode(opt.popWord());
    AL_SET_CHROMA_MODE(cfg.Settings.tChParam.ePicFormat, chromaMode);
  }, "Specify chroma-mode (CHROMA_MONO, CHROMA_4_0_0, CHROMA_4_2_0, CHROMA_4_2_2)");

  int ipbitdepth = -1;
  opt.addInt("--level", &cfg.Settings.tChParam.uLevel, "Specify the level we want to encode with (10 to 62)");
  opt.addCustom("--ip-bitdepth", &ipbitdepth, &GetCmdlineValue, "Specify bitdepth of ip input (8 : 10)");
  opt.addCustom("--input-format", &cfg.FileInfo.FourCC, &GetCmdlineFourCC, "Specify YUV input format (I420, IYUV, YV12, NV12, Y800, Y010, P010, I0AL ...)");
  opt.addCustom("--output-format", &cfg.RecFourCC, &GetCmdlineFourCC, "Specify output format");
  opt.addCustom("--ratectrl-mode", &cfg.Settings.tChParam.tRCParam.eRCMode, &GetCmdlineValue,
                "Specify rate control mode (CONST_QP, CBR, VBR"
                ", LOW_LATENCY"
                ")"
                );
  opt.addOption("--bitrate", [&]()
  {
    cfg.Settings.tChParam.tRCParam.uTargetBitRate = opt.popInt() * 1000;
  }, "Specify bitrate in Kbits/s");
  opt.addOption("--max-bitrate", [&]()
  {
    cfg.Settings.tChParam.tRCParam.uMaxBitRate = opt.popInt() * 1000;
  }, "Specify max bitrate in Kbits/s");
  opt.addOption("--framerate", [&]()
  {
    GetFpsCmdline(opt.popWord(),
                  cfg.Settings.tChParam.tRCParam.uFrameRate,
                  cfg.Settings.tChParam.tRCParam.uClkRatio);
  }, "Specify the frame rate used for encoding");
  opt.addInt("--sliceQP", &cfg.Settings.tChParam.tRCParam.iInitialQP, "Specify the initial slice QP");
  opt.addInt("--gop-length", &cfg.Settings.tChParam.tGopParam.uGopLength, "Specify the GOP length, 0 means I slice only");
  opt.addInt("--gop-numB", &cfg.Settings.tChParam.tGopParam.uNumB, "Number of consecutive B frame (0 .. 4)");
  opt.addInt("--max-picture", &cfg.RunInfo.iMaxPict, "Maximum number of pictures encoded (1,2 .. -1 for ALL)");
  opt.addInt("--num-slices", &cfg.Settings.tChParam.uNumSlices, "Specify the number of slices to use");
  opt.addInt("--num-core", &cfg.Settings.tChParam.uNumCore, "Specify the number of cores to use (resolution needs to be sufficient)");
  opt.addFlag("--loop", &cfg.RunInfo.bLoop, "loop at the end of the yuv file");

  opt.addInt("--prefetch", &g_numFrameToRepeat, "prefetch n frames and loop between these frames for max picture count");
  opt.parse(argc, argv);

  if(help)
  {
    Usage(opt, argv[0]);
    exit(0);
  }

  if(cfg.FileInfo.PictWidth > UINT16_MAX)
    throw runtime_error("Unsupported picture width value");

  if(cfg.FileInfo.PictHeight > UINT16_MAX)
    throw runtime_error("Unsupported picture height value");

  cfg.Settings.tChParam.uWidth = cfg.FileInfo.PictWidth;
  cfg.Settings.tChParam.uHeight = cfg.FileInfo.PictHeight;

  if(AL_IS_STILL_PROFILE(cfg.Settings.tChParam.eProfile))
    cfg.RunInfo.iMaxPict = 1;


  if(ipbitdepth != -1)
  {
    AL_SET_BITDEPTH(cfg.Settings.tChParam.ePicFormat, ipbitdepth);
  }
}

void ValidateConfig(ConfigFile& cfg)
{
  string invalid_settings("Invalid settings, check the [SETTINGS] section of your configuration file or check your commandline (use -h to get help)");

  if(cfg.YUVFileName.empty())
    throw runtime_error("No YUV input was given, specify it in the [INPUT] section of your configuration file or in your commandline (use -h to get help)");

  SetConsoleColor(CC_RED);

  auto const err = AL_Settings_CheckValidity(&cfg.Settings, stdout);

  if(err != 0)
  {
    stringstream ss;
    ss << err << " errors(s). " << invalid_settings;
    throw runtime_error(ss.str());
  }

  auto const incoherencies = AL_Settings_CheckCoherency(&cfg.Settings, cfg.FileInfo.FourCC, stdout);

  if(incoherencies == -1)
    throw runtime_error("Fatal coherency error in settings");
  SetConsoleColor(CC_DEFAULT);
}

void SetMoreDefaults(ConfigFile& cfg)
{
  auto& FileInfo = cfg.FileInfo;
  auto& Settings = cfg.Settings;
  auto& RecFourCC = cfg.RecFourCC;

  if(FileInfo.FrameRate == 0)
    FileInfo.FrameRate = Settings.tChParam.tRCParam.uFrameRate;

  if(RecFourCC == FOURCC(NULL))
  {
    if(AL_GET_CHROMA_MODE(Settings.tChParam.ePicFormat) == CHROMA_MONO)
    {
      if(AL_GET_BITDEPTH(Settings.tChParam.ePicFormat) == 8)
        RecFourCC = FOURCC(Y800);
      else
        RecFourCC = FOURCC(Y010);
    }
    else
      RecFourCC = FileInfo.FourCC;
  }
}

static AL_TOffsetYC GetOffsetYC(int iPitchY, int iHeight)
{
  AL_TOffsetYC tOffsetYC;
  tOffsetYC.iLuma = 0;
  tOffsetYC.iChroma = (int)(iPitchY * iHeight);
  return tOffsetYC;
}

/*****************************************************************************/
static
shared_ptr<AL_TBuffer> AllocateConversionBuffer(vector<uint8_t>& YuvBuffer, int iWidth, int iHeight, TFourCC tFourCC)
{
  uint32_t uSize = GetAllocSize_Yuv(iWidth, iHeight, tFourCC);
  YuvBuffer.resize(uSize);
  AL_TBuffer* Yuv = AL_Buffer_WrapData(YuvBuffer.data(), uSize, NULL);
  /* we want to read from /write to a file, so no alignement is necessary */
  auto iWidthInBytes = iWidth * GetPixelSize(AL_GetBitDepth(tFourCC));
  AL_TPitches tPitches {
    iWidthInBytes, AL_IsSemiPlanar(tFourCC) ? iWidthInBytes : iWidthInBytes / 2
  };

  AL_TOffsetYC tOffsetYC = GetOffsetYC(tPitches.iLuma, iHeight);
  AL_TMetaData* pMeta = (AL_TMetaData*)AL_SrcMetaData_Create(iWidth, iHeight, tPitches, tOffsetYC, tFourCC);

  if(!pMeta)
    throw runtime_error("Couldn't allocate conversion buffer");
  AL_Buffer_AddMetaData(Yuv, pMeta);

  return shared_ptr<AL_TBuffer>(Yuv, &AL_Buffer_Destroy);
}

shared_ptr<AL_TBuffer> ReadSourceFrame(AL_TBufPool* pBufPool, AL_TBuffer* conversionBuffer, ifstream& YuvFile, ConfigFile const& cfg, IConvSrc* hConv)
{
  shared_ptr<AL_TBuffer> sourceBuffer(AL_BufPool_GetBuffer(pBufPool, AL_BUF_MODE_BLOCK), &AL_Buffer_Unref);
  assert(sourceBuffer);

  if(!ReadOneFrameYuv(YuvFile, hConv ? conversionBuffer : sourceBuffer.get(), cfg.RunInfo.bLoop))
    return nullptr;

  if(hConv)
    hConv->ConvertSrcBuf(AL_GET_BITDEPTH(cfg.Settings.tChParam.ePicFormat), conversionBuffer, sourceBuffer.get());

  return sourceBuffer;
}

static int GetPitchYValue(int iWidth)
{
  return (iWidth + 31) & 0xFFFFFFE0; // 256 bits alignment
}

static void SetPitchYC(AL_TPitches& p, int iWidth, TFourCC tFourCC)
{
  (void)tFourCC;

  if(AL_Is10bitPacked(tFourCC))
  {
    p.iLuma = GetPitchYValue((iWidth + 2) / 3 * 4);
    p.iChroma = p.iLuma;
  }
  else
  {
    int iPixelSize = AL_GetBitDepth(tFourCC) > 8 ? sizeof(uint16_t) : sizeof(uint8_t);
    p.iLuma = GetPitchYValue(iWidth * iPixelSize);
    p.iChroma = AL_IsSemiPlanar(tFourCC) ? p.iLuma : p.iLuma / 2;
  }
}

bool isLastPict(int iPictCount, int iMaxPict)
{
  return (iPictCount >= iMaxPict) && (iMaxPict != -1);
}

int sendInputFileTo(string YUVFileName, BufPool& SrcBufPool, AL_TBuffer* Yuv, ConfigFile const& cfg, IConvSrc* pSrcConv, IFrameSink* sink)
{
  ifstream YuvFile;
  OpenInput(YuvFile, YUVFileName);

  GotoFirstPicture(cfg.FileInfo, YuvFile, cfg.RunInfo.iFirstPict);

  int iPictCount = 0;

  while(true)
  {
    shared_ptr<AL_TBuffer> frame;

    if(!isLastPict(iPictCount, cfg.RunInfo.iMaxPict))
      frame = ReadSourceFrame(&SrcBufPool, Yuv, YuvFile, cfg, pSrcConv);

    sink->ProcessFrame(frame.get());

    if(!frame)
      break;

    iPictCount++;
  }

  return iPictCount;
}

/*****************************************************************************/
int SafeMain(int argc, char** argv)
{
  ConfigFile cfg;
  SetDefaults(cfg);

  auto& FileInfo = cfg.FileInfo;
  auto& Settings = cfg.Settings;
  auto& StreamFileName = cfg.BitstreamFileName;
  auto& RecFileName = cfg.RecFileName;
  auto& RunInfo = cfg.RunInfo;

  ParseCommandLine(argc, argv, cfg);

  DisplayVersionInfo();

  AL_Settings_SetDefaultParam(&cfg.Settings);
  ValidateConfig(cfg);
  SetMoreDefaults(cfg);

  auto pIpDevice = CreateIpDevice(!RunInfo.bUseBoard, RunInfo.iSchedulerType, Settings, RunInfo.ipCtrlMode, RunInfo.trackDma, RunInfo.iVip, RunInfo.eVQDescr);

  if(!pIpDevice)
    throw runtime_error("Can't create IpDevice");

  auto hFinished = Rtos_CreateEvent(false);
  auto scopeMutex = scopeExit([&]() {
    Rtos_DeleteEvent(hFinished);
  });

  // --------------------------------------------------------------------------------
  // Create Encoder
  auto pAllocator = pIpDevice->m_pAllocator.get();
  auto pScheduler = pIpDevice->m_pScheduler;

  AL_TBufPoolConfig StreamBufPoolConfig;

  auto numStreams = 2 + 2 + Settings.tChParam.tGopParam.uNumB;
  auto streamSize = GetMaxNalSize(FileInfo.PictWidth, FileInfo.PictHeight, AL_GET_CHROMA_MODE(Settings.tChParam.ePicFormat));

  if(Settings.tChParam.bSubframeLatency)
  {
    numStreams *= Settings.tChParam.uNumSlices;
    streamSize /= Settings.tChParam.uNumSlices;
    /* we need space for the headers on each slice */
    streamSize += 4096 * 2;
    /* stream size is required to be 32bits aligned */
    streamSize = (streamSize + 31) & ~31;
  }

  StreamBufPoolConfig.uMinBuf = numStreams;
  StreamBufPoolConfig.uMaxBuf = numStreams;
  StreamBufPoolConfig.zBufSize = streamSize;
  StreamBufPoolConfig.pMetaData = (AL_TMetaData*)AL_StreamMetaData_Create(AL_MAX_SECTION);
  StreamBufPoolConfig.debugName = "stream";

  BufPool StreamBufPool(pAllocator, StreamBufPoolConfig);

  BufPool SrcBufPool;

  if(!RecFileName.empty() || !cfg.RunInfo.sMd5Path.empty())
    Settings.tChParam.eOptions = (AL_EChEncOption)(Settings.tChParam.eOptions | AL_OPT_FORCE_REC);


  int frameBuffersCount = 2 + cfg.Settings.tChParam.tGopParam.uNumB;
  AL_TBufPoolConfig QpBufPoolConfig = {};

  if(cfg.Settings.eQpCtrlMode & (MASK_QP_TABLE_EXT))
  {
    QpBufPoolConfig.uMinBuf = frameBuffersCount;
    QpBufPoolConfig.uMaxBuf = frameBuffersCount;
    QpBufPoolConfig.zBufSize = GetAllocSizeEP2(cfg.Settings.tChParam.uWidth, cfg.Settings.tChParam.uHeight, cfg.Settings.tChParam.uMaxCuSize);
    QpBufPoolConfig.pMetaData = NULL;
    QpBufPoolConfig.debugName = "qp-ext";
  }
  AL_TBufPool QpBufPool;
  AL_BufPool_Init(&QpBufPool, pAllocator, &QpBufPoolConfig);

  auto scopeQpBufPool = scopeExit([&]() {
    AL_BufPool_Deinit(&QpBufPool);
  });

  unique_ptr<EncoderSink> enc;

  enc.reset(new EncoderSink(cfg.sScnChgFileName, cfg.sLTFileName, cfg.sGMVFileName, cfg, pScheduler, pAllocator, QpBufPool));

  enc->BitstreamOutput = createBitstreamWriter(StreamFileName, cfg);
  enc->m_done = ([&]() {
    Rtos_SetEvent(hFinished);
  });

  IFrameSink* firstSink = enc.get();

  // Input/Output Format conversion
  shared_ptr<AL_TBuffer> SrcYuv;
  vector<uint8_t> YuvBuffer;

  bool shouldConvert = IsConversionNeeded(FileInfo.FourCC, AL_GET_CHROMA_MODE(Settings.tChParam.ePicFormat), AL_GET_BITDEPTH(Settings.tChParam.ePicFormat));

  if(shouldConvert)
    SrcYuv = AllocateConversionBuffer(YuvBuffer, FileInfo.PictWidth, FileInfo.PictHeight, FileInfo.FourCC);

  shared_ptr<AL_TBuffer> RecYuv;
  vector<uint8_t> RecYuvBuffer;

  if(!RecFileName.empty())
  {
    RecYuv = AllocateConversionBuffer(RecYuvBuffer, FileInfo.PictWidth, FileInfo.PictHeight, cfg.RecFourCC);
    enc->RecOutput = createFrameWriter(RecFileName, cfg, RecYuv.get());
  }

  if(!cfg.RunInfo.sMd5Path.empty())
  {
    auto multisink = unique_ptr<MultiSink>(new MultiSink);
    multisink->sinks.push_back(move(enc->RecOutput));
    multisink->sinks.push_back(createMd5Calculator(cfg.RunInfo.sMd5Path, cfg, RecYuv.get()));
    enc->RecOutput = move(multisink);
  }


  for(unsigned int i = 0; i < StreamBufPoolConfig.uMaxBuf; ++i)
  {
    AL_TBuffer* pStream = AL_BufPool_GetBuffer(&StreamBufPool, AL_BUF_MODE_NONBLOCK);
    assert(pStream);
    auto bRet = AL_Encoder_PutStreamBuffer(enc->hEnc, pStream);
    assert(bRet);
    AL_Buffer_Unref(pStream);
  }

  unique_ptr<RepeaterSink> prefetch;

  if(g_numFrameToRepeat > 0)
  {
    prefetch.reset(new RepeaterSink(g_numFrameToRepeat, cfg.RunInfo.iMaxPict));
    prefetch->next = enc.get();
    firstSink = prefetch.get();
    cfg.RunInfo.iMaxPict = g_numFrameToRepeat;
    frameBuffersCount = max(frameBuffersCount, g_numFrameToRepeat);
  }

  auto BitDepth = AL_GET_BITDEPTH(cfg.Settings.tChParam.ePicFormat);
  auto ChromaMode = AL_GET_CHROMA_MODE(cfg.Settings.tChParam.ePicFormat);
  TFourCC FourCC = AL_GetSrcFourCC(ChromaMode, BitDepth);
  AL_TPitches p;
  SetPitchYC(p, cfg.FileInfo.PictWidth, FourCC);
  AL_TOffsetYC tOffsetYC = GetOffsetYC(p.iLuma, cfg.FileInfo.PictHeight);

  /* source compression case */
  unique_ptr<IConvSrc> pSrcConv;
  switch(cfg.Settings.tChParam.eSrcConvMode)
  {
  case AL_NVX:
    pSrcConv.reset(new CNvxConv(cfg.FileInfo.PictWidth, cfg.FileInfo.PictHeight, BitDepth, ChromaMode));
    break;
  default:
    break;
  }

  AL_TBufPoolConfig poolConfig {};

  poolConfig.uMinBuf = frameBuffersCount;
  poolConfig.uMaxBuf = frameBuffersCount;
  poolConfig.zBufSize = pSrcConv->GetConvBufSize();
  poolConfig.pMetaData = (AL_TMetaData*)AL_SrcMetaData_Create(cfg.FileInfo.PictWidth, cfg.FileInfo.PictHeight, p, tOffsetYC, FourCC);
  poolConfig.debugName = "src";

  bool ret = AL_BufPool_Init(&SrcBufPool, pAllocator, &poolConfig);
  assert(ret);

  if(!shouldConvert)
    pSrcConv.reset(nullptr);

  sendInputFileTo(cfg.YUVFileName, SrcBufPool, SrcYuv.get(), cfg, pSrcConv.get(), firstSink);

  Rtos_WaitEvent(hFinished, AL_WAIT_FOREVER);

  return int(GetEncoderLastError());
}

/******************************************************************************/

int main(int argc, char** argv)
{
  try
  {
    SafeMain(argc, argv);
    return 0;
  }
  catch(codec_error const& error)
  {
    cerr << endl << "Codec error: " << error.what() << endl;
    return error.GetCode();
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    return 1;
  }
}

