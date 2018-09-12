/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <climits>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <list>
#include <sstream>
#include <cstdarg>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "lib_app/BufPool.h"
#include "lib_app/console.h"
#include "lib_app/utils.h"

#include "CodecUtils.h"
#include "sink.h"
#include "IpDevice.h"

#include "resource.h"

#include "CfgParser.h"

extern "C"
{
#include "lib_common/BufferSrcMeta.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferPictureMeta.h"
#if AL_ENABLE_TWOPASS
#include "lib_common/BufferLookAheadMeta.h"
#endif
#include "lib_common/StreamBuffer.h"
#include "lib_common/Utils.h"
#include "lib_common/versions.h"
#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "lib_conv_yuv/lib_conv_yuv.h"
#include "sink_encoder.h"
#if AL_ENABLE_TWOPASS
#include "sink_lookahead.h"
#endif
#include "sink_bitstream_writer.h"
#include "sink_frame_writer.h"
#include "sink_md5.h"
#include "sink_repeater.h"
#include "QPGenerator.h"


static int g_numFrameToRepeat;
static int g_StrideHeight = -1;
static int g_Stride = -1;

using namespace std;

/*****************************************************************************/

#include "lib_app/BuildInfo.h"

#if !HAS_COMPIL_FLAGS
#define AL_COMPIL_FLAGS ""
#endif

void DisplayBuildInfo()
{
  BuildInfoDisplay displayBuildInfo {
    SVN_REV, AL_CONFIGURE_COMMANDLINE, AL_COMPIL_FLAGS
  };
  displayBuildInfo.displayFeatures = [=]()
                                     {
                                     };
  displayBuildInfo();
}

void DisplayVersionInfo()
{
  DisplayVersionInfo(AL_ENCODER_COMPANY,
                     AL_ENCODER_PRODUCT_NAME,
                     AL_ENCODER_VERSION,
                     AL_ENCODER_COPYRIGHT,
                     AL_ENCODER_COMMENTS);
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
  cfg.RunInfo.bUseBoard = true;
  cfg.RunInfo.iSchedulerType = SCHEDULER_TYPE_MCU;
  cfg.RunInfo.bLoop = false;
  cfg.RunInfo.iMaxPict = INT_MAX; // ALL
  cfg.RunInfo.iFirstPict = 0;
  cfg.RunInfo.iScnChgLookAhead = 3;
  cfg.RunInfo.ipCtrlMode = IPCTRL_MODE_STANDARD;
  cfg.RunInfo.uInputSleepInMilliseconds = 0;
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


template<typename T>
function<T(string const &)> createCmdlineParsingFunc(char const* name_, function<T(ConfigFile &)> extractWantedValue)
{
  string name = name_;
  auto lambda = [=](string const& value)
                {
                  ConfigFile cfg {};
                  cfg.strict_mode = true;
                  string toParse = name + string("=") + value;
                  ParseConfig(toParse, cfg);
                  return extractWantedValue(cfg);
                };
  return lambda;
}

function<TFourCC(string const &)> createParseInputFourCC()
{
  return createCmdlineParsingFunc<TFourCC>("[INPUT]\nFormat", [](ConfigFile& cfg) { return cfg.FileInfo.FourCC; });
}

function<TFourCC(string const &)> createParseRecFourCC()
{
  return createCmdlineParsingFunc<TFourCC>("[OUTPUT]\nFormat", [](ConfigFile& cfg) { return cfg.RecFourCC; });
}

function<AL_EProfile(string const &)> createParseProfile()
{
  return createCmdlineParsingFunc<AL_EProfile>("[SETTINGS]\nProfile", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].eProfile; });
}

function<AL_ERateCtrlMode(string const &)> createParseRCMode()
{
  return createCmdlineParsingFunc<AL_ERateCtrlMode>("[RATE_CONTROL]\nRateCtrlMode", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].tRCParam.eRCMode; });
}

function<AL_EGopCtrlMode(string const &)> createParseGopMode()
{
  return createCmdlineParsingFunc<AL_EGopCtrlMode>("[GOP]\nGopCtrlMode", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].tGopParam.eMode; });
}



void introspect(ConfigFile& cfg)
{
  (void)cfg;
  throw runtime_error("introspection is not compiled in");
}

/*****************************************************************************/
void ParseCommandLine(int argc, char** argv, ConfigFile& cfg)
{
  bool DoNotAcceptCfg = false;
  bool help = false;
  bool help_cfg = false;
  bool version = false;
  bool dumpCfg = false;
  stringstream warning;
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
    ParseConfigFile(cfgPath, cfg, warning);
  }, "Specify configuration file");

  opt.addOption("-cfg-permissive", [&]()
  {
    auto const cfgPath = opt.popWord();
    ParseConfigFile(cfgPath, cfg, warning);
  }, "Use it instead of -cfg. Errors in the configuration file will be ignored");

  opt.addFlag("--help-cfg", &help_cfg, "Show cfg help");
  opt.addFlag("--help,-h", &help, "Show this help");
  opt.addFlag("--version", &version, "Show version");
  opt.addString("--input,-i", &cfg.YUVFileName, "YUV input file");

  opt.addString("--output,-o", &cfg.BitstreamFileName, "Compressed output file");
  opt.addString("--md5", &cfg.RunInfo.sMd5Path, "Path to the output MD5 textfile");
  opt.addString("--output-rec,-r", &cfg.RecFileName, "Output reconstructed YUV file");
  opt.addOption("--color", [&]() {
    SetEnableColor(true);
  }, "Enable color");
  opt.addInt("--input-sleep", &cfg.RunInfo.uInputSleepInMilliseconds, "Minimum waiting time in milliseconds between each process frame (0 by default)");

  opt.addFlag("--quiet,-q", &g_Verbosity, "Do not print anything", 0);

  opt.addInt("--input-width", &cfg.FileInfo.PictWidth, "Specifies YUV input width");
  opt.addInt("--input-height", &cfg.FileInfo.PictHeight, "Specifies YUV input height");
  opt.addOption("--chroma-mode", [&]()
  {
    auto chromaMode = stringToChromaMode(opt.popWord());
    AL_SET_CHROMA_MODE(cfg.Settings.tChParam[0].ePicFormat, chromaMode);
  }, "Specify chroma-mode (CHROMA_MONO, CHROMA_4_0_0, CHROMA_4_2_0, CHROMA_4_2_2)");

  int ipbitdepth = -1;
  opt.addInt("--level", &cfg.Settings.tChParam[0].uLevel, "Specifies the level we want to encode with (10 to 62)");
  opt.addCustom("--profile", &cfg.Settings.tChParam[0].eProfile, createParseProfile(), "Specifies the profile we want to encode with (example: HEVC_MAIN, AVC_MAIN, ...)");
  opt.addInt("--ip-bitdepth", &ipbitdepth, "Specifies bitdepth of ip input (8 : 10)");
  opt.addCustom("--input-format", &cfg.FileInfo.FourCC, createParseInputFourCC(), "Specifies YUV input format (I420, IYUV, YV12, NV12, Y800, Y010, P010, I0AL ...)");
  opt.addCustom("--rec-format", &cfg.RecFourCC, createParseRecFourCC(), "Specifies output format");
  opt.addCustom("--ratectrl-mode", &cfg.Settings.tChParam[0].tRCParam.eRCMode, createParseRCMode(),
                "Specifies rate control mode (CONST_QP, CBR, VBR"
                ", LOW_LATENCY"
                ")"
                );
  opt.addOption("--bitrate", [&]()
  {
    cfg.Settings.tChParam[0].tRCParam.uTargetBitRate = opt.popInt() * 1000;
  }, "Specifies bitrate in Kbits/s");
  opt.addOption("--max-bitrate", [&]()
  {
    cfg.Settings.tChParam[0].tRCParam.uMaxBitRate = opt.popInt() * 1000;
  }, "Specifies max bitrate in Kbits/s");
  opt.addOption("--framerate", [&]()
  {
    ParseConfig("[RATE_CONTROL]\nFrameRate=" + opt.popWord(), cfg);
  }, "Specifies the frame rate used for encoding");
  opt.addInt("--sliceQP", &cfg.Settings.tChParam[0].tRCParam.iInitialQP, "Specifies the initial slice QP");
  opt.addInt("--gop-length", &cfg.Settings.tChParam[0].tGopParam.uGopLength, "Specifies the GOP length, 0 means I slice only");
  opt.addInt("--gop-numB", &cfg.Settings.tChParam[0].tGopParam.uNumB, "Number of consecutive B frame (0 .. 4)");
  opt.addCustom("--gop-mode", &cfg.Settings.tChParam[0].tGopParam.eMode, createParseGopMode(), "Specifies gop control mode (DEFAULT_GOP, PYRAMIDAL_GOP)");
  opt.addInt("--first-picture", &cfg.RunInfo.iFirstPict, "First picture encoded (skip those before)");
  opt.addInt("--max-picture", &cfg.RunInfo.iMaxPict, "Maximum number of pictures encoded (1,2 .. -1 for ALL)");
  opt.addInt("--num-slices", &cfg.Settings.tChParam[0].uNumSlices, "Specifies the number of slices to use");
  opt.addInt("--num-core", &cfg.Settings.tChParam[0].uNumCore, "Specifies the number of cores to use (resolution needs to be sufficient)");
  opt.addString("--log", &cfg.RunInfo.logsFile, "A file where log event will be dumped");
  opt.addFlag("--loop", &cfg.RunInfo.bLoop, "Loop at the end of the yuv file");
  opt.addFlag("--slicelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Enable subframe latency");
  opt.addFlag("--framelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Disable subframe latency", false);

  opt.addInt("--prefetch", &g_numFrameToRepeat, "Prefetch n frames and loop between these frames for max picture count");
  opt.addFlag("--print-picture-type", &cfg.RunInfo.printPictureType, "Write picture type for each frame in the file", true);



#if AL_ENABLE_TWOPASS
  opt.addInt("--lookahead", &cfg.Settings.LookAhead, "Set the twopass LookAhead size");
  opt.addInt("--pass", &cfg.Settings.TwoPass, "Specify which pass we are encoding");
  opt.addString("--twopass-logfile", &cfg.sTwoPassFileName, "File for video statistics used in twopass");
#endif

  opt.addOption("--set", [&]()
  {
    ParseConfig(opt.popWord(), cfg);
  }, "Use the same syntax as in the cfg to specify a parameter (experimental)");
  opt.parse(argc, argv);

  if(help)
  {
    Usage(opt, argv[0]);
    exit(0);
  }

  if(help_cfg)
  {
    PrintConfigFileUsage();
    exit(0);
  }

  if(version)
  {
    DisplayVersionInfo();
    DisplayBuildInfo();
    exit(0);
  }


  if(g_Verbosity)
    cerr << warning.str();

  if(cfg.FileInfo.PictWidth > UINT16_MAX)
    throw runtime_error("Unsupported picture width value");

  if(cfg.FileInfo.PictHeight > UINT16_MAX)
    throw runtime_error("Unsupported picture height value");

  AL_SetSrcWidth(&cfg.Settings.tChParam[0], cfg.FileInfo.PictWidth);
  AL_SetSrcHeight(&cfg.Settings.tChParam[0], cfg.FileInfo.PictHeight);

  if(ipbitdepth != -1)
  {
    AL_SET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat, ipbitdepth);
  }

  cfg.Settings.tChParam[0].uSrcBitDepth = AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat);

  if(AL_IS_STILL_PROFILE(cfg.Settings.tChParam[0].eProfile))
    cfg.RunInfo.iMaxPict = 1;

}

void ValidateConfig(ConfigFile& cfg)
{
  string invalid_settings("Invalid settings, check the [SETTINGS] section of your configuration file or check your commandline (use -h to get help)");

  if(cfg.YUVFileName.empty())
    throw runtime_error("No YUV input was given, specify it in the [INPUT] section of your configuration file or in your commandline (use -h to get help)");

  if(!cfg.sQPTablesFolder.empty() && cfg.Settings.eQpCtrlMode != LOAD_QP)
    throw runtime_error("QPTablesFolder can only be specified with Load QP control mode");

  SetConsoleColor(CC_RED);

  FILE* out = stdout;

  if(!g_Verbosity)
    out = NULL;

  for(int i = 0; i < cfg.Settings.NumLayer; ++i)
  {
    auto const err = AL_Settings_CheckValidity(&cfg.Settings, &cfg.Settings.tChParam[i], out);

    if(err != 0)
    {
      stringstream ss;
      ss << err << " errors(s). " << invalid_settings;
      throw runtime_error(ss.str());
    }

    auto const incoherencies = AL_Settings_CheckCoherency(&cfg.Settings, &cfg.Settings.tChParam[i], cfg.FileInfo.FourCC, out);

    if(incoherencies == -1)
      throw runtime_error("Fatal coherency error in settings");
  }

#if AL_ENABLE_TWOPASS

  if(cfg.Settings.TwoPass == 1)
    AL_TwoPassMngr_SetPass1Settings(cfg.Settings);
#endif

  SetConsoleColor(CC_DEFAULT);
}

void SetMoreDefaults(ConfigFile& cfg)
{
  auto& FileInfo = cfg.FileInfo;
  auto& Settings = cfg.Settings;
  auto& RecFourCC = cfg.RecFourCC;

  if(FileInfo.FrameRate == 0)
    FileInfo.FrameRate = Settings.tChParam[0].tRCParam.uFrameRate;

  if(RecFourCC == FOURCC(NULL))
  {
    AL_TPicFormat tOutPicFormat;

    if(AL_GetPicFormat(FileInfo.FourCC, &tOutPicFormat))
    {
      tOutPicFormat.eChromaMode = AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat);
      tOutPicFormat.eChromaOrder = tOutPicFormat.eChromaMode == CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : tOutPicFormat.eChromaOrder;
      tOutPicFormat.uBitDepth = AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat);
      RecFourCC = AL_GetFourCC(tOutPicFormat);
    }
    else
    {
      RecFourCC = FileInfo.FourCC;
    }
  }
}

static AL_TOffsetYC GetOffsetYC(int iPitchY, int iHeight, TFourCC fourCC)
{
  AL_TOffsetYC tOffsetYC;
  tOffsetYC.iLuma = 0;
  auto const iNumLinesInPitch = AL_GetNumLinesInPitch(AL_GetStorageMode(fourCC));
  tOffsetYC.iChroma = (int)(iPitchY * iHeight / iNumLinesInPitch);
  return tOffsetYC;
}

/*****************************************************************************/
static
shared_ptr<AL_TBuffer> AllocateConversionBuffer(vector<uint8_t>& YuvBuffer, int iWidth, int iHeight, TFourCC tFourCC)
{
  /* we want to read from /write to a file, so no alignement is necessary */
  int const iWidthInBytes = GetIOLumaRowSize(tFourCC, static_cast<uint32_t>(iWidth));
  AL_TPitches tPitches {
    iWidthInBytes, AL_IsSemiPlanar(tFourCC) ? iWidthInBytes : iWidthInBytes / 2
  };
  uint32_t uSize = tPitches.iLuma * iHeight;
  switch(AL_GetChromaMode(tFourCC))
  {
  case CHROMA_4_2_0:
    uSize += uSize / 2;
    break;
  case CHROMA_4_2_2:
    uSize += uSize;
    break;
  default:
    break;
  }

  YuvBuffer.resize(uSize);
  AL_TBuffer* Yuv = AL_Buffer_WrapData(YuvBuffer.data(), uSize, NULL);

  AL_TOffsetYC tOffsetYC = GetOffsetYC(tPitches.iLuma, iHeight, tFourCC);
  AL_TDimension tDimension = { iWidth, iHeight };
  AL_TMetaData* pMeta = (AL_TMetaData*)AL_SrcMetaData_Create(tDimension, tPitches, tOffsetYC, tFourCC);

  if(!pMeta)
    throw runtime_error("Couldn't allocate conversion buffer");
  AL_Buffer_AddMetaData(Yuv, pMeta);

  return shared_ptr<AL_TBuffer>(Yuv, &AL_Buffer_Destroy);
}

shared_ptr<AL_TBuffer> ReadSourceFrame(BufPool* pBufPool, AL_TBuffer* conversionBuffer, ifstream& YuvFile, AL_TEncChanParam const& tChParam, ConfigFile const& cfg, IConvSrc* hConv)
{
  shared_ptr<AL_TBuffer> sourceBuffer(pBufPool->GetBuffer(), &AL_Buffer_Unref);
  assert(sourceBuffer);

  if(!ReadOneFrameYuv(YuvFile, hConv ? conversionBuffer : sourceBuffer.get(), cfg.RunInfo.bLoop))
    return nullptr;

  if(hConv)
    hConv->ConvertSrcBuf(tChParam.uSrcBitDepth, conversionBuffer, sourceBuffer.get());

  return sourceBuffer;
}

bool ConvertSrcBuffer(AL_TEncChanParam& tChParam, TYUVFileInfo& FileInfo, vector<uint8_t>& YuvBuffer, shared_ptr<AL_TBuffer>& SrcYuv)
{
  auto const picFmt = AL_EncGetSrcPicFormat(AL_GET_CHROMA_MODE(tChParam.ePicFormat), tChParam.uSrcBitDepth, AL_GetSrcStorageMode(tChParam.eSrcMode),
                                            AL_IsSrcCompressed(tChParam.eSrcMode));
  bool shouldConvert = IsConversionNeeded(FileInfo.FourCC, picFmt);

  if(shouldConvert)
    SrcYuv = AllocateConversionBuffer(YuvBuffer, FileInfo.PictWidth, FileInfo.PictHeight, FileInfo.FourCC);
  return shouldConvert;
}

static AL_TPitches SetPitchYC(int iWidth, TFourCC tFourCC)
{
  AL_TPitches p;
  p.iLuma = AL_EncGetMinPitch(iWidth, AL_GetBitDepth(tFourCC), AL_GetStorageMode(tFourCC));

  if(g_Stride != -1)
  {
    assert(g_Stride >= p.iLuma);
    p.iLuma = g_Stride;
  }

  p.iChroma = AL_IsSemiPlanar(tFourCC) ? p.iLuma : p.iLuma / 2;
  return p;
}

static bool isLastPict(int iPictCount, int iMaxPict)
{
  return (iPictCount >= iMaxPict) && (iMaxPict != -1);
}

static void PrepareInput(ifstream& YuvFile, string& YUVFileName, TYUVFileInfo& FileInfo, ConfigFile const& cfg)
{
  OpenInput(YuvFile, YUVFileName);
  GotoFirstPicture(FileInfo, YuvFile, cfg.RunInfo.iFirstPict);
}

static void GetSrcFrame(shared_ptr<AL_TBuffer>& frame, int& iReadCount, int iPictCount, ifstream& YuvFile, const TYUVFileInfo& FileInfo, BufPool& SrcBufPool, AL_TBuffer* Yuv, AL_TEncChanParam const& tChParam, ConfigFile const& cfg, IConvSrc* pSrcConv)
{
  if(!isLastPict(iPictCount, cfg.RunInfo.iMaxPict))
  {
    if(cfg.FileInfo.FrameRate != tChParam.tRCParam.uFrameRate)
      iReadCount += GotoNextPicture(FileInfo, YuvFile, tChParam.tRCParam.uFrameRate, iPictCount, iReadCount);

    frame = ReadSourceFrame(&SrcBufPool, Yuv, YuvFile, tChParam, cfg, pSrcConv);
    iReadCount++;
  }
}

static bool sendInputFileTo(ifstream& YuvFile, BufPool& SrcBufPool, AL_TBuffer* Yuv, ConfigFile const& cfg, IConvSrc* pSrcConv, IFrameSink* sink, int& iPictCount, int& iReadCount)
{
  shared_ptr<AL_TBuffer> frame;
  GetSrcFrame(frame, iReadCount, iPictCount, YuvFile, cfg.FileInfo, SrcBufPool, Yuv, cfg.Settings.tChParam[0], cfg, pSrcConv);
  sink->ProcessFrame(frame.get());

  if(!frame)
    return false;

  iPictCount++;
  return true;
}


unique_ptr<IConvSrc> CreateSrcConverter(TFrameInfo const& FrameInfo, AL_ESrcMode eSrcMode, AL_TEncChanParam& tChParam)
{
  (void)tChParam;
  switch(eSrcMode)
  {
  case AL_SRC_NVX:
    return make_unique<CNvxConv>(FrameInfo);
  default:
    throw runtime_error("Unsupported source conversion.");
  }
}


/*****************************************************************************/
function<AL_TIpCtrl* (AL_TIpCtrl*)> GetIpCtrlWrapper(TCfgRunInfo& RunInfo)
{
  function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl;
  switch(RunInfo.ipCtrlMode)
  {
  default:
    wrapIpCtrl = [](AL_TIpCtrl* ipCtrl) -> AL_TIpCtrl*
                 {
                   return ipCtrl;
                 };
    break;
  }

  return wrapIpCtrl;
}

/*****************************************************************************/
static AL_TBufPoolConfig GetBufPoolConfig(const char* debugName, AL_TMetaData* pMetaData, int iSize, int frameBuffersCount)
{
  AL_TBufPoolConfig poolConfig = {};

  poolConfig.uNumBuf = frameBuffersCount;
  poolConfig.zBufSize = iSize;
  poolConfig.pMetaData = pMetaData;
  poolConfig.debugName = debugName;
  return poolConfig;
}

/*****************************************************************************/
static AL_TBufPoolConfig GetQpBufPoolConfig(AL_TEncSettings& Settings, AL_TEncChanParam& tChParam, int frameBuffersCount)
{
  AL_TBufPoolConfig poolConfig = {};

  if(Settings.eQpCtrlMode & (MASK_QP_TABLE_EXT))
  {
    AL_TDimension tDim = { tChParam.uWidth, tChParam.uHeight };
    poolConfig = GetBufPoolConfig("qp-ext", NULL, AL_GetAllocSizeEP2(tDim, tChParam.uMaxCuSize), frameBuffersCount);
  }
  return poolConfig;
}

/*****************************************************************************/
static AL_TBufPoolConfig GetSrcBufPoolConfig(unique_ptr<IConvSrc>& pSrcConv, TFrameInfo& FrameInfo, AL_ESrcMode eSrcMode, int frameBuffersCount)
{
  auto const tPictFormat = AL_EncGetSrcPicFormat(FrameInfo.eCMode, FrameInfo.iBitDepth, AL_GetSrcStorageMode(eSrcMode), AL_IsSrcCompressed(eSrcMode));
  TFourCC FourCC = AL_GetFourCC(tPictFormat);
  AL_TPitches p = SetPitchYC(FrameInfo.iWidth, FourCC);
  int iStrideHeight = (FrameInfo.iHeight + 7) & ~7;

  if(g_StrideHeight != -1)
    iStrideHeight = g_StrideHeight;

  AL_TOffsetYC tOffsetYC = GetOffsetYC(p.iLuma, iStrideHeight, FourCC);
  AL_TMetaData* pMetaData = (AL_TMetaData*)AL_SrcMetaData_Create({ FrameInfo.iWidth, FrameInfo.iHeight }, p, tOffsetYC, FourCC);
  int iSrcSize = pSrcConv->GetSrcBufSize(p.iLuma, iStrideHeight);

  return GetBufPoolConfig("src", pMetaData, iSrcSize, frameBuffersCount);
}

/*****************************************************************************/
static AL_TBufPoolConfig GetStreamBufPoolConfig(AL_TEncSettings& Settings, TYUVFileInfo& FileInfo)
{
  auto numStreams = 2 + 2 + Settings.tChParam[0].tGopParam.uNumB;
  AL_TDimension dim = { FileInfo.PictWidth, FileInfo.PictHeight };
  auto streamSize = AL_GetMitigatedMaxNalSize(dim, AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat), AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat));

#if AL_ENABLE_TWOPASS

  // the LookAhead needs one stream buffer to work (2 in AVC multi-core)
  if(AL_TwoPassMngr_HasLookAhead(Settings))
    numStreams += (Settings.tChParam[0].eProfile & AL_PROFILE_AVC) ? 2 : 1;
#endif

  if(Settings.tChParam[0].bSubframeLatency)
  {
    numStreams *= Settings.tChParam[0].uNumSlices;
    streamSize /= Settings.tChParam[0].uNumSlices;
    /* we need space for the headers on each slice */
    streamSize += 4096 * 2;
    /* stream size is required to be 32bytes aligned */
    streamSize = (streamSize + 31) & ~31;
  }

  AL_TMetaData* pMetaData = (AL_TMetaData*)AL_StreamMetaData_Create(AL_MAX_SECTION);
  return GetBufPoolConfig("stream", pMetaData, streamSize, numStreams);
}


/*****************************************************************************/
static TFrameInfo GetFrameInfo(TYUVFileInfo& tFileInfo, AL_TEncChanParam& tChParam)
{
  TFrameInfo tFrameInfo;

  tFrameInfo.iWidth = tFileInfo.PictWidth;
  tFrameInfo.iHeight = tFileInfo.PictHeight;
  tFrameInfo.iBitDepth = tChParam.uSrcBitDepth;
  tFrameInfo.eCMode = AL_GET_CHROMA_MODE(tChParam.ePicFormat);

  return tFrameInfo;
}

/*****************************************************************************/
static void InitSrcBufPool(AL_TAllocator* pAllocator, bool shouldConvert, unique_ptr<IConvSrc>& pSrcConv, TFrameInfo& FrameInfo, AL_ESrcMode eSrcMode, int frameBuffersCount, BufPool& SrcBufPool)
{
  AL_TBufPoolConfig poolConfig = GetSrcBufPoolConfig(pSrcConv, FrameInfo, eSrcMode, frameBuffersCount);
  bool ret = SrcBufPool.Init(pAllocator, poolConfig);
  assert(ret);

  if(!shouldConvert)
    pSrcConv.reset(nullptr);
}

/*****************************************************************************/
void SafeMain(int argc, char** argv)
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

  AL_Settings_SetDefaultParam(&Settings);
  SetMoreDefaults(cfg);

  if(!RecFileName.empty() || !cfg.RunInfo.sMd5Path.empty())
    Settings.tChParam[0].eOptions = (AL_EChEncOption)(Settings.tChParam[0].eOptions | AL_OPT_FORCE_REC);



  ValidateConfig(cfg);



  function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl = GetIpCtrlWrapper(RunInfo);

  auto pIpDevice = CreateIpDevice(!RunInfo.bUseBoard, RunInfo.iSchedulerType, Settings, wrapIpCtrl, RunInfo.trackDma, RunInfo.eVQDescr);

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

  AL_TBufPoolConfig StreamBufPoolConfig = GetStreamBufPoolConfig(Settings, FileInfo);
  BufPool StreamBufPool(pAllocator, StreamBufPoolConfig);
  /* instantiation has to be before the Encoder instantiation to get the destroying order right */
  BufPool SrcBufPool;

  int frameBuffersCount = 2 + Settings.tChParam[0].tGopParam.uNumB;
#if AL_ENABLE_TWOPASS

  // the LookAhead needs LookAheadSize source buffers to work
  if(AL_TwoPassMngr_HasLookAhead(cfg.Settings))
    frameBuffersCount += cfg.Settings.LookAhead;
#endif
  auto QpBufPoolConfig = GetQpBufPoolConfig(Settings, Settings.tChParam[0], frameBuffersCount);
  BufPool QpBufPool(pAllocator, QpBufPoolConfig);


  unique_ptr<EncoderSink> enc;
  enc.reset(new EncoderSink(cfg, pScheduler, pAllocator, QpBufPool
                            ));


  enc->BitstreamOutput = createBitstreamWriter(StreamFileName, cfg);
  enc->m_done = ([&]() {
    Rtos_SetEvent(hFinished);
  });

  IFrameSink* firstSink = enc.get();

#if AL_ENABLE_TWOPASS
  unique_ptr<EncoderLookAheadSink> encFirstPassLA;

  if(AL_TwoPassMngr_HasLookAhead(cfg.Settings))
  {
    encFirstPassLA.reset(new EncoderLookAheadSink(cfg, pScheduler, pAllocator, QpBufPool
                                                  ));
    encFirstPassLA->next = firstSink;
    firstSink = encFirstPassLA.get();
  }
#endif

  // Input/Output Format conversion
  shared_ptr<AL_TBuffer> SrcYuv;
  vector<uint8_t> YuvBuffer;
  bool shouldConvert = ConvertSrcBuffer(Settings.tChParam[0], FileInfo, YuvBuffer, SrcYuv);


  shared_ptr<AL_TBuffer> RecYuv;
  vector<uint8_t> RecYuvBuffer;

  if(!RecFileName.empty())
  {
    RecYuv = AllocateConversionBuffer(RecYuvBuffer, Settings.tChParam[0].uWidth, Settings.tChParam[0].uHeight, cfg.RecFourCC);
    enc->RecOutput = createFrameWriter(RecFileName, cfg, RecYuv.get(), 0);
  }


  if(!cfg.RunInfo.sMd5Path.empty())
  {
    auto multisink = unique_ptr<MultiSink>(new MultiSink);
    multisink->sinks.push_back(move(enc->RecOutput));
    multisink->sinks.push_back(createMd5Calculator(cfg.RunInfo.sMd5Path, cfg, RecYuv.get()));
    enc->RecOutput = move(multisink);
  }


  for(unsigned int i = 0; i < StreamBufPoolConfig.uNumBuf; ++i)
  {
    AL_TBuffer* pStream = StreamBufPool.GetBuffer(AL_BUF_MODE_NONBLOCK);
    assert(pStream);

    if(cfg.RunInfo.printPictureType)
    {
      AL_TMetaData* pMeta = (AL_TMetaData*)AL_PictureMetaData_Create();
      assert(pMeta);
      auto const attached = AL_Buffer_AddMetaData(pStream, pMeta);
      assert(attached);
    }

    AL_HEncoder hEnc = enc->hEnc;

#if AL_ENABLE_TWOPASS

    // the Lookahead needs one stream buffer to work (2 in AVC multi-core)
    if(AL_TwoPassMngr_HasLookAhead(cfg.Settings) && i < ((Settings.tChParam[0].eProfile & AL_PROFILE_AVC) ? 2 : 1))
      hEnc = encFirstPassLA->hEnc;
#endif
    bool bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream);
    assert(bRet);
    AL_Buffer_Unref(pStream);
  }


  unique_ptr<RepeaterSink> prefetch;

  if(g_numFrameToRepeat > 0)
  {
    prefetch.reset(new RepeaterSink(g_numFrameToRepeat, cfg.RunInfo.iMaxPict));
    prefetch->next = firstSink;
    firstSink = prefetch.get();
    cfg.RunInfo.iMaxPict = g_numFrameToRepeat;
    frameBuffersCount = max(frameBuffersCount, g_numFrameToRepeat);
  }

  TFrameInfo FrameInfo = GetFrameInfo(cfg.FileInfo, Settings.tChParam[0]);
  auto const eSrcMode = Settings.tChParam[0].eSrcMode;

  /* source compression case */
  auto pSrcConv = CreateSrcConverter(FrameInfo, eSrcMode, Settings.tChParam[0]);

  InitSrcBufPool(pAllocator, shouldConvert, pSrcConv, FrameInfo, eSrcMode, frameBuffersCount, SrcBufPool);
  ifstream YuvFile;
  PrepareInput(YuvFile, cfg.YUVFileName, cfg.FileInfo, cfg);

  int iPictCount = 0;
  int iReadCount = 0;
  bool bRet = true;

  while(bRet)
  {
    AL_64U uBeforeTime = Rtos_GetTime();
    bRet = sendInputFileTo(YuvFile, SrcBufPool, SrcYuv.get(), cfg, pSrcConv.get(), firstSink, iPictCount, iReadCount);

    AL_64U uAfterTime = Rtos_GetTime();

    if((uAfterTime - uBeforeTime) < cfg.RunInfo.uInputSleepInMilliseconds)
      Rtos_Sleep(cfg.RunInfo.uInputSleepInMilliseconds - (uAfterTime - uBeforeTime));
  }

  Rtos_WaitEvent(hFinished, AL_WAIT_FOREVER);

  if(auto err = GetEncoderLastError())
    throw codec_error(EncoderErrorToString(err), err);
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

