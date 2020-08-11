/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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
#include "lib_app/PixMapBufPool.h"
#include "lib_app/console.h"
#include "lib_app/utils.h"
#include "lib_app/plateform.h"

#include "CodecUtils.h"
#include "YuvIO.h"
#include "sink.h"
#include "IpDevice.h"

#include "resource.h"

#include "CfgParser.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferPictureMeta.h"
#include "lib_common/BufferLookAheadMeta.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Error.h"
#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "lib_conv_yuv/lib_conv_yuv.h"
#include "sink_encoder.h"
#include "sink_lookahead.h"
#include "sink_bitstream_writer.h"
#include "sink_bitrate.h"
#include "sink_frame_writer.h"
#include "sink_md5.h"
#include "sink_repeater.h"
#include "QPGenerator.h"

#include "RCPlugin.h"

static int g_numFrameToRepeat;
static int g_StrideHeight = -1;
static int g_Stride = -1;
static int constexpr g_defaultMinBuffers = 2;
static bool g_MultiChunk = false;

using namespace std;

/*****************************************************************************/
/* duplicated from Utils.h as we can't take these from inside the libraries */
static inline int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) / iRnd * iRnd;
}

static inline uint32_t UnsignedRoundUp(uint32_t uVal, int iRnd)
{
  return ((uVal + iRnd - 1) / iRnd) * iRnd;
}

AL_HANDLE AlignedAlloc(AL_TAllocator* pAllocator, const char* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset)
{
  AL_HANDLE pBuf = NULL;
  *uAllocatedSize = 0;
  *uAlignmentOffset = 0;

  uSize += uAlign;

  pBuf = AL_Allocator_AllocNamed(pAllocator, uSize, pBufName);

  if(NULL == pBuf)
    return NULL;

  *uAllocatedSize = uSize;
  AL_PADDR pAddr = AL_Allocator_GetPhysicalAddr(pAllocator, pBuf);
  *uAlignmentOffset = UnsignedRoundUp(pAddr, uAlign) - pAddr;

  return pBuf;
}

/*****************************************************************************/

#include "lib_app/BuildInfo.h"

#if !HAS_COMPIL_FLAGS
#define AL_COMPIL_FLAGS ""
#endif

void DisplayBuildInfo()
{
  BuildInfoDisplay displayBuildInfo {
    SCM_REV, SCM_BRANCH, AL_CONFIGURE_COMMANDLINE, AL_COMPIL_FLAGS, DELIVERY_BUILD_NUMBER, DELIVERY_SCM_REV, DELIVERY_DATE
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
  cfg.MainInput.FileInfo.FourCC = FOURCC(I420);
  cfg.MainInput.FileInfo.FrameRate = 0;
  cfg.MainInput.FileInfo.PictHeight = 0;
  cfg.MainInput.FileInfo.PictWidth = 0;
  cfg.RunInfo.iDeviceType = DEVICE_TYPE_BOARD;
  cfg.RunInfo.iSchedulerType = SCHEDULER_TYPE_MCU;
  cfg.RunInfo.bLoop = false;
  cfg.RunInfo.iMaxPict = INT_MAX; // ALL
  cfg.RunInfo.iFirstPict = 0;
  cfg.RunInfo.iScnChgLookAhead = 3;
  cfg.RunInfo.ipCtrlMode = IPCTRL_MODE_STANDARD;
  cfg.RunInfo.eVQDescr = 0;
  cfg.RunInfo.uInputSleepInMilliseconds = 0;
  cfg.strict_mode = false;
}

#include "lib_app/CommandLineParser.h"

static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cerr << "Usage: " << ExeName << " -cfg <configfile> [options]" << endl;
  cerr << "Options:" << endl;

  opt.usage();

  cerr << endl << "Examples:" << endl;
  cerr << "  " << ExeName << " -cfg test/config/encode_simple.cfg -r rec.yuv -o output.hevc -i input.yuv" << endl;
}

static AL_EChromaMode stringToChromaMode(string s)
{
  if(s == "CHROMA_MONO")
    return AL_CHROMA_MONO;

  if(s == "CHROMA_4_0_0")
    return AL_CHROMA_4_0_0;

  if(s == "CHROMA_4_2_0")
    return AL_CHROMA_4_2_0;

  if(s == "CHROMA_4_2_2")
    return AL_CHROMA_4_2_2;

  throw runtime_error("Unknown chroma mode: \"" + s + "\"");
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
  return createCmdlineParsingFunc<TFourCC>("[INPUT]\nFormat", [](ConfigFile& cfg) { return cfg.MainInput.FileInfo.FourCC; });
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

static void SetCodingResolution(ConfigFile& cfg)
{
  int iMaxSrcWidth = cfg.MainInput.FileInfo.PictWidth;
  int iMaxSrcHeight = cfg.MainInput.FileInfo.PictHeight;

  for(auto input = cfg.DynamicInputs.begin(); input != cfg.DynamicInputs.end(); input++)
  {
    iMaxSrcWidth = std::max(input->FileInfo.PictWidth, iMaxSrcWidth);
    iMaxSrcHeight = std::max(input->FileInfo.PictHeight, iMaxSrcHeight);
  }

  cfg.Settings.tChParam[0].uSrcWidth = iMaxSrcWidth;
  cfg.Settings.tChParam[0].uSrcHeight = iMaxSrcHeight;

  cfg.Settings.tChParam[0].uEncWidth = cfg.Settings.tChParam[0].uSrcWidth;
  cfg.Settings.tChParam[0].uEncHeight = cfg.Settings.tChParam[0].uSrcHeight;

}

bool g_helpCfg = false;
bool g_helpCfgJson = false;
bool g_showCfg = false;

/*****************************************************************************/
void ParseCommandLine(int argc, char** argv, ConfigFile& cfg)
{
  bool DoNotAcceptCfg = false;
  bool help = false;
  bool helpJson = false;
  bool version = false;
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

  opt.addFlag("--help-cfg", &g_helpCfg, "Show cfg help. Show the default value for each parameter. If this option is used with a cfg, show the end value of each parameter.");
  opt.addFlag("--help-cfg-json", &g_helpCfgJson, "Same as --help-cfg but in json format");
  opt.addFlag("--help,-h", &help, "Show this help");
  opt.addFlag("--help-json", &helpJson, "Show this help (json)");
  opt.addFlag("--show-cfg", &g_showCfg, "Show the cfg variables value before launching the program");
  opt.addFlag("--version", &version, "Show version");

  opt.addOption("-cfg", [&](string)
  {
    auto const cfgPath = opt.popWord();
    cfg.strict_mode = true;
    ParseConfigFile(cfgPath, cfg, warning);
  }, "Specify configuration file", "string");

  opt.addOption("-cfg-permissive", [&](string)
  {
    auto const cfgPath = opt.popWord();
    ParseConfigFile(cfgPath, cfg, warning);
  }, "Use it instead of -cfg. Errors in the configuration file will be ignored", "string");

  opt.addOption("--set", [&](string)
  {
    ParseConfig(opt.popWord(), cfg);
  }, "Use the same syntax as in the cfg to specify a parameter (Example: --set \"[INPUT] Width = 512\"", "string");

  opt.addString("--input,-i", &cfg.MainInput.YUVFileName, "YUV input file");
  opt.addString("--output,-o", &cfg.BitstreamFileName, "Compressed output file");
  opt.addString("--output-rec,-r", &cfg.RecFileName, "Output reconstructed YUV file");
  opt.addString("--md5", &cfg.RunInfo.sMd5Path, "Path to the output MD5 textfile");
  opt.addInt("--input-width", &cfg.MainInput.FileInfo.PictWidth, "Specifies YUV input width");
  opt.addInt("--input-height", &cfg.MainInput.FileInfo.PictHeight, "Specifies YUV input height");
  opt.addCustom("--input-format", &cfg.MainInput.FileInfo.FourCC, createParseInputFourCC(), "Specifies YUV input format (I420, IYUV, YV12, NV12, Y800, Y010, P010, I0AL ...)", "enum");
  opt.addCustom("--rec-format", &cfg.RecFourCC, createParseRecFourCC(), "Specifies output format", "enum");

  opt.addInt("--level", &cfg.Settings.tChParam[0].uLevel, "Specifies the level we want to encode with (10 to 62)");
  opt.addCustom("--profile", &cfg.Settings.tChParam[0].eProfile, createParseProfile(), "Specifies the profile we want to encode with (example: HEVC_MAIN, AVC_MAIN, ...)", "enum");
  opt.addOption("--chroma-mode", [&](string)
  {
    auto chromaMode = stringToChromaMode(opt.popWord());
    AL_SET_CHROMA_MODE(&cfg.Settings.tChParam[0].ePicFormat, chromaMode);
  }, "Specify chroma-mode (CHROMA_MONO, CHROMA_4_0_0, CHROMA_4_2_0, CHROMA_4_2_2)", "enum");

  int ipbitdepth = -1;
  opt.addInt("--ip-bitdepth", &ipbitdepth, "Specifies bitdepth of ip input (8 : 10)");
  opt.addInt("--num-slices", &cfg.Settings.tChParam[0].uNumSlices, "Specifies the number of slices to use");
  opt.addFlag("--slicelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Enable subframe latency");
  opt.addFlag("--framelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Disable subframe latency", false);

  opt.addInt("--lookahead", &cfg.Settings.LookAhead, "Set the twopass LookAhead size");
  opt.addInt("--pass", &cfg.Settings.TwoPass, "Specify which pass we are encoding");
  opt.addString("--pass-logfile", &cfg.sTwoPassFileName, "LogFile to transmit dual pass statistics");
  opt.addFlag("--first-pass-scd", &cfg.Settings.bEnableFirstPassSceneChangeDetection, "During first pass, the encoder encode faster by only enabling scene change detection");

  opt.startSection("Rate Control && GOP");
  opt.addCustom("--ratectrl-mode", &cfg.Settings.tChParam[0].tRCParam.eRCMode, createParseRCMode(),
                "Specifies rate control mode (CONST_QP, CBR, VBR"
                ", LOW_LATENCY"
                ")", "enum"
                );
  opt.addOption("--bitrate", [&](string)
  {
    cfg.Settings.tChParam[0].tRCParam.uTargetBitRate = opt.popInt() * 1000;
  }, "Specifies bitrate in Kbits/s", "number");
  opt.addOption("--max-bitrate", [&](string)
  {
    cfg.Settings.tChParam[0].tRCParam.uMaxBitRate = opt.popInt() * 1000;
  }, "Specifies max bitrate in Kbits/s", "number");
  opt.addOption("--framerate", [&](string)
  {
    ParseConfig("[RATE_CONTROL]\nFrameRate=" + opt.popWord(), cfg);
  }, "Specifies the frame rate used for encoding", "number");
  opt.addInt("--sliceQP", &cfg.Settings.tChParam[0].tRCParam.iInitialQP, "Specifies the initial slice QP");

  opt.addCustom("--gop-mode", &cfg.Settings.tChParam[0].tGopParam.eMode, createParseGopMode(), "Specifies gop control mode (DEFAULT_GOP, LOW_DELAY_P)", "enum");
  opt.addInt("--gop-length", &cfg.Settings.tChParam[0].tGopParam.uGopLength, "Specifies the GOP length, 1 means I slice only");
  opt.addInt("--gop-numB", &cfg.Settings.tChParam[0].tGopParam.uNumB, "Number of consecutive B frame (0 .. 4)");

  opt.endSection("Rate Control && GOP");

  opt.startSection("Run");

  opt.addInt("--first-picture", &cfg.RunInfo.iFirstPict, "First picture encoded (skip those before)");
  opt.addInt("--max-picture", &cfg.RunInfo.iMaxPict, "Maximum number of pictures encoded (1,2 .. -1 for ALL)");
  opt.addFlag("--loop", &cfg.RunInfo.bLoop, "Loop at the end of the yuv file");

  opt.addInt("--input-sleep", &cfg.RunInfo.uInputSleepInMilliseconds, "Minimum waiting time in milliseconds between each process frame (0 by default)");

  opt.endSection("Run");

  opt.startSection("Traces && Debug");

  opt.addInt("--stride-height", &g_StrideHeight, "Chroma offset (vertical stride)");
  opt.addInt("--stride", &g_Stride, "Luma stride");
  opt.addFlag("--multi-chunk", &g_MultiChunk, "Allocate source luma and chroma on different memory chunks");
  opt.addInt("--num-core", &cfg.Settings.tChParam[0].uNumCore, "Specifies the number of cores to use (resolution needs to be sufficient)");

  opt.addString("--log", &cfg.RunInfo.logsFile, "A file where log event will be dumped");
  opt.addInt("--prefetch", &g_numFrameToRepeat, "Prefetch n frames and loop between these frames for max picture count");
  opt.addFlag("--print-picture-type", &cfg.RunInfo.printPictureType, "Write picture type for each frame in the file", true);
  opt.addFlag("--print-ratectrl-stat", &cfg.RunInfo.printRateCtrlStat, "Write rate-control related statistics for each frame in the file. Only a subset of the statistics is written, more data and motion vectors are also available.", true);

  opt.endSection("Traces && Debug");

  opt.startSection("Misc");

  opt.addOption("--color", [&](string)
  {
    SetEnableColor(true);
  }, "Enable color (Default: Auto)");

  opt.addOption("--no-color", [&](string)
  {
    SetEnableColor(false);
  }, "Disable color");

  opt.addFlag("--quiet,-q", &g_Verbosity, "Do not print anything", 0);
  opt.addInt("--verbosity", &g_Verbosity, "Choose the verbosity level (-q is equivalent to --verbosity 0)");

  opt.endSection("Misc");

  opt.parse(argc, argv);

  if(help)
  {
    Usage(opt, argv[0]);
    exit(0);
  }

  if(helpJson)
  {
    opt.usageJson();
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

  SetCodingResolution(cfg);

  if(ipbitdepth != -1)
    AL_SET_BITDEPTH(&cfg.Settings.tChParam[0].ePicFormat, ipbitdepth);

  cfg.Settings.tChParam[0].uSrcBitDepth = AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat);

  if(AL_IS_STILL_PROFILE(cfg.Settings.tChParam[0].eProfile))
    cfg.RunInfo.iMaxPict = 1;
}

void ValidateConfig(ConfigFile& cfg)
{
  string invalid_settings("Invalid settings, check the [SETTINGS] section of your configuration file or check your commandline (use -h to get help)");

  if(cfg.MainInput.YUVFileName.empty())
    throw runtime_error("No YUV input was given, specify it in the [INPUT] section of your configuration file or in your commandline (use -h to get help)");

  if(!cfg.MainInput.sQPTablesFolder.empty() && ((cfg.RunInfo.eGenerateQpMode & AL_GENERATE_MASK_QP_TABLE) != AL_GENERATE_LOAD_QP))
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

    auto const incoherencies = AL_Settings_CheckCoherency(&cfg.Settings, &cfg.Settings.tChParam[i], cfg.MainInput.FileInfo.FourCC, out);

    if(incoherencies == -1)
      throw runtime_error("Fatal coherency error in settings");
  }

  AL_TwoPassMngr_SetGlobalSettings(cfg.Settings);

  if(cfg.Settings.TwoPass == 1)
    AL_TwoPassMngr_SetPass1Settings(cfg.Settings, NULL);

  SetConsoleColor(CC_DEFAULT);
}

void SetMoreDefaults(ConfigFile& cfg)
{
  auto& FileInfo = cfg.MainInput.FileInfo;
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
      tOutPicFormat.eChromaOrder = tOutPicFormat.eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : tOutPicFormat.eChromaOrder;
      tOutPicFormat.uBitDepth = AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat);
      RecFourCC = AL_GetFourCC(tOutPicFormat);
    }
    else
    {
      RecFourCC = FileInfo.FourCC;
    }
  }
}

/*****************************************************************************/
static shared_ptr<AL_TBuffer> AllocateConversionBuffer(int iWidth, int iHeight, TFourCC tFourCC)
{
  AL_TBuffer* pYuv = AllocateDefaultYuvIOBuffer(AL_TDimension { iWidth, iHeight }, tFourCC);

  if(pYuv == nullptr)
    return nullptr;
  return shared_ptr<AL_TBuffer>(pYuv, &AL_Buffer_Destroy);
}

shared_ptr<AL_TBuffer> ReadSourceFrame(BaseBufPool* pBufPool, AL_TBuffer* conversionBuffer, ifstream& YuvFile, AL_TEncChanParam const& tChParam, ConfigFile const& cfg, IConvSrc* hConv)
{
  shared_ptr<AL_TBuffer> sourceBuffer(pBufPool->GetBuffer(), &AL_Buffer_Unref);
  assert(sourceBuffer);

  AL_TDimension tUpdatedDim = AL_TDimension {
    AL_GetSrcWidth(tChParam), AL_GetSrcHeight(tChParam)
  };

  AL_PixMapBuffer_SetDimension(sourceBuffer.get(), tUpdatedDim);

  if(hConv)
  {
    AL_PixMapBuffer_SetDimension(conversionBuffer, tUpdatedDim);

    if(!ReadOneFrameYuv(YuvFile, conversionBuffer, cfg.RunInfo.bLoop))
      return nullptr;
    hConv->ConvertSrcBuf(tChParam.uSrcBitDepth, conversionBuffer, sourceBuffer.get());
  }
  else
  {
    if(!ReadOneFrameYuv(YuvFile, sourceBuffer.get(), cfg.RunInfo.bLoop))
      return nullptr;
  }

  return sourceBuffer;
}

bool ConvertSrcBuffer(AL_TEncChanParam& tChParam, TYUVFileInfo& FileInfo, shared_ptr<AL_TBuffer>& SrcYuv)
{
  auto const picFmt = AL_EncGetSrcPicFormat(AL_GET_CHROMA_MODE(tChParam.ePicFormat), tChParam.uSrcBitDepth, AL_GetSrcStorageMode(tChParam.eSrcMode),
                                            AL_IsSrcCompressed(tChParam.eSrcMode));
  bool shouldConvert = IsConversionNeeded(FileInfo.FourCC, picFmt);

  if(shouldConvert)
  {
    SrcYuv = AllocateConversionBuffer(AL_GetSrcWidth(tChParam), AL_GetSrcHeight(tChParam), FileInfo.FourCC);

    if(SrcYuv == nullptr)
      throw runtime_error("Couldn't allocate source conversion buffer");
  }

  return shouldConvert;
}

static int ComputeYPitch(int iWidth, TFourCC tFourCC)
{
  auto iPitch = AL_EncGetMinPitch(iWidth, AL_GetBitDepth(tFourCC), AL_GetStorageMode(tFourCC));

  if(g_Stride != -1)
  {
    assert(g_Stride >= iPitch);
    iPitch = g_Stride;
  }
  return iPitch;
}

static bool isLastPict(int iPictCount, int iMaxPict)
{
  return (iPictCount >= iMaxPict) && (iMaxPict != -1);
}

static void PrepareInput(ifstream& YuvFile, string& YUVFileName, TYUVFileInfo& FileInfo, int iFirstPict)
{
  YuvFile.close();
  OpenInput(YuvFile, YUVFileName);
  GotoFirstPicture(FileInfo, YuvFile, iFirstPict);
}

static void GetSrcFrame(shared_ptr<AL_TBuffer>& frame, int& iReadCount, int iPictCount, ifstream& YuvFile, const TYUVFileInfo& FileInfo, PixMapBufPool& SrcBufPool, AL_TBuffer* Yuv, AL_TEncChanParam const& tChParam, ConfigFile const& cfg, IConvSrc* pSrcConv)
{
  if(!isLastPict(iPictCount, cfg.RunInfo.iMaxPict))
  {
    if(cfg.MainInput.FileInfo.FrameRate != tChParam.tRCParam.uFrameRate)
      iReadCount += GotoNextPicture(FileInfo, YuvFile, tChParam.tRCParam.uFrameRate, iPictCount, iReadCount);

    frame = ReadSourceFrame(&SrcBufPool, Yuv, YuvFile, tChParam, cfg, pSrcConv);
    iReadCount++;
  }
}

static bool sendInputFileTo(ifstream& YuvFile, PixMapBufPool& SrcBufPool, AL_TBuffer* Yuv, ConfigFile const& cfg, TYUVFileInfo& FileInfo, IConvSrc* pSrcConv, IFrameSink* sink, int& iPictCount, int& iReadCount)
{
  if(AL_IS_ERROR_CODE(GetEncoderLastError()))
  {
    sink->ProcessFrame(nullptr);
    return false;
  }

  shared_ptr<AL_TBuffer> frame;
  GetSrcFrame(frame, iReadCount, iPictCount, YuvFile, FileInfo, SrcBufPool, Yuv, cfg.Settings.tChParam[0], cfg, pSrcConv);
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
function<AL_TIpCtrl* (AL_TIpCtrl*)> GetIpCtrlWrapper(TCfgRunInfo& RunInfo, void* pTraceHook)
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

  if(AL_IS_QP_TABLE_REQUIRED(Settings.eQpTableMode))
  {
    AL_TDimension tDim = { tChParam.uEncWidth, tChParam.uEncHeight };
    poolConfig = GetBufPoolConfig("qp-ext", NULL, AL_GetAllocSizeEP2(tDim, static_cast<AL_ECodec>(AL_GET_CODEC(tChParam.eProfile))), frameBuffersCount);
  }
  return poolConfig;
}

/*****************************************************************************/
static void ConfigureSrcBufPool(PixMapBufPool& SrcBufPool, TFrameInfo& FrameInfo, AL_ESrcMode eSrcMode)
{
  auto const tPicFormat = AL_EncGetSrcPicFormat(FrameInfo.eCMode, FrameInfo.iBitDepth, AL_GetSrcStorageMode(eSrcMode), AL_IsSrcCompressed(eSrcMode));
  TFourCC tFourCC = AL_GetFourCC(tPicFormat);

  SrcBufPool.SetFormat(FrameInfo.tDimension, tFourCC);

  int iPitchY = ComputeYPitch(FrameInfo.tDimension.iWidth, tFourCC);
  int iStrideHeight = g_StrideHeight != -1 ? g_StrideHeight : RoundUp(FrameInfo.tDimension.iHeight, 8);

  std::vector<AL_TPlaneDescription> vPlaneDesc;
  int iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat.eChromaOrder, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int iPitch = usedPlanes[iPlane] == AL_PLANE_Y ? iPitchY : AL_GetChromaPitch(tFourCC, iPitchY);
    vPlaneDesc.push_back(AL_TPlaneDescription { usedPlanes[iPlane], iOffset, iPitch });
    iOffset += AL_GetAllocSizeSrc_PixPlane(eSrcMode, iPitchY, iStrideHeight, FrameInfo.eCMode, usedPlanes[iPlane]);

    if(g_MultiChunk)
    {
      SrcBufPool.AddChunk(iOffset, vPlaneDesc);
      vPlaneDesc.clear();
      iOffset = 0;
    }
  }

  if(!g_MultiChunk)
    SrcBufPool.AddChunk(iOffset, vPlaneDesc);
}

/*****************************************************************************/
static uint8_t GetNumBufForGop(AL_TEncSettings Settings)
{
  int uNumFields = 1;
  return uNumFields * Settings.tChParam[0].tGopParam.uNumB;
}

/*****************************************************************************/
static AL_TBufPoolConfig GetStreamBufPoolConfig(AL_TEncSettings& Settings, int iLayerID)
{
  int smoothingStream = 2;
  auto numStreams = g_defaultMinBuffers + smoothingStream + GetNumBufForGop(Settings);
  AL_TDimension dim = { Settings.tChParam[iLayerID].uEncWidth, Settings.tChParam[iLayerID].uEncHeight };
  auto streamSize = AL_GetMitigatedMaxNalSize(dim, AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat), AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat));

  if(AL_TwoPassMngr_HasLookAhead(Settings))
  {
    int extraLookAheadStream = 1;

    // the look ahead needs one more stream buffer to work in AVC due to (potential) multi-core
    if(AL_IS_AVC(Settings.tChParam[0].eProfile))
      extraLookAheadStream += 1;
    numStreams += extraLookAheadStream;
  }

  if(Settings.tChParam[0].bSubframeLatency)
  {
    numStreams *= Settings.tChParam[0].uNumSlices;
    streamSize /= Settings.tChParam[0].uNumSlices;
    /* we need space for the headers on each slice */
    streamSize += AL_ENC_MAX_HEADER_SIZE;
    /* stream size is required to be 32bytes aligned */
    streamSize = RoundUp(streamSize, 32);
  }

  AL_TMetaData* pMetaData = (AL_TMetaData*)AL_StreamMetaData_Create(AL_MAX_SECTION);
  return GetBufPoolConfig("stream", pMetaData, streamSize, numStreams);
}

/*****************************************************************************/
static TFrameInfo GetFrameInfo(AL_TEncChanParam& tChParam)
{
  TFrameInfo tFrameInfo;

  tFrameInfo.tDimension = { AL_GetSrcWidth(tChParam), AL_GetSrcHeight(tChParam) };
  tFrameInfo.iBitDepth = tChParam.uSrcBitDepth;
  tFrameInfo.eCMode = AL_GET_CHROMA_MODE(tChParam.ePicFormat);

  return tFrameInfo;
}

/*****************************************************************************/
static void InitSrcBufPool(AL_TAllocator* pAllocator, bool shouldConvert, unique_ptr<IConvSrc>& pSrcConv, TFrameInfo& FrameInfo, AL_ESrcMode eSrcMode, int frameBuffersCount, PixMapBufPool& SrcBufPool)
{
  ConfigureSrcBufPool(SrcBufPool, FrameInfo, eSrcMode);
  bool ret = SrcBufPool.Init(pAllocator, frameBuffersCount, "input");
  assert(ret);

  if(!shouldConvert)
    pSrcConv.reset(nullptr);
}

/*****************************************************************************/

/*****************************************************************************/
struct LayerRessources
{
  ~LayerRessources()
  {
    Rtos_DeleteEvent(hFinished);
  }

  void Init(ConfigFile& cfg, int frameBuffersCount, int srcBuffersCount, int iLayerID, CIpDevice* pDevices);

  void PushRessources(ConfigFile& cfg, EncoderSink* enc
                      , EncoderLookAheadSink* encFirstPassLA
                      );

  void OpenInput(ConfigFile& cfg, AL_HEncoder hEnc);

  bool SendInput(ConfigFile& cfg, IFrameSink* firstSink);

  void ChangeInput(ConfigFile& cfg, int iInputIdx, AL_HEncoder hEnc);

  void WaitFinished()
  {
    Rtos_WaitEvent(hFinished, AL_WAIT_FOREVER);
  }

  AL_TBufPoolConfig StreamBufPoolConfig;
  BufPool StreamBufPool;

  BufPool QpBufPool;

  PixMapBufPool SrcBufPool;

  // Input/Output Format conversion
  shared_ptr<AL_TBuffer> SrcYuv;

  shared_ptr<AL_TBuffer> RecYuv;
  vector<uint8_t> RecYuvBuffer;

  unique_ptr<IFrameSink> frameWriter;

  ifstream YuvFile;
  unique_ptr<IConvSrc> pSrcConv;

  int iPictCount = 0;
  int iReadCount = 0;

  int iLayerID = 0;
  int iInputIdx = 0;
  std::vector<TConfigYUVInput> layerInputs;

  AL_EVENT hFinished = NULL;
};

void LayerRessources::Init(ConfigFile& cfg, int frameBuffersCount, int srcBuffersCount, int iLayerID, CIpDevice* pDevices)
{
  AL_TEncSettings& Settings = cfg.Settings;

  this->iLayerID = iLayerID;

  layerInputs.push_back(cfg.MainInput);
  layerInputs.insert(layerInputs.end(), cfg.DynamicInputs.begin(), cfg.DynamicInputs.end());

  hFinished = Rtos_CreateEvent(false);
  AL_TAllocator* pAllocator = pDevices->m_pAllocator.get();
  StreamBufPoolConfig = GetStreamBufPoolConfig(Settings, iLayerID);
  StreamBufPool.Init(pAllocator, StreamBufPoolConfig);

  bool bUsePictureMeta = false;
  bUsePictureMeta |= cfg.RunInfo.printPictureType;

  bUsePictureMeta |= AL_TwoPassMngr_HasLookAhead(Settings);

  if(iLayerID == 0 && bUsePictureMeta)
  {
    AL_TMetaData* pMeta = (AL_TMetaData*)AL_PictureMetaData_Create();
    assert(pMeta);
    bool bRet = StreamBufPool.AddMetaData(pMeta);
    assert(bRet);
    Rtos_Free(pMeta);
  }

  if(cfg.RunInfo.printRateCtrlStat)
  {
    AL_TDimension tDim = { Settings.tChParam[iLayerID].uEncWidth, Settings.tChParam[iLayerID].uEncHeight };
    AL_TMetaData* pMeta = (AL_TMetaData*)AL_RateCtrlMetaData_Create(pAllocator, tDim, Settings.tChParam[iLayerID].uMaxCuSize, AL_GET_CODEC(Settings.tChParam[iLayerID].eProfile));
    assert(pMeta);
    bool bRet = StreamBufPool.AddMetaData(pMeta);
    assert(bRet);
    Rtos_Free(pMeta);
  }

  AL_TBufPoolConfig poolConfig = GetQpBufPoolConfig(Settings, Settings.tChParam[iLayerID], frameBuffersCount);
  QpBufPool.Init(pAllocator, poolConfig);

  // Input/Output Format conversion
  bool shouldConvert = ConvertSrcBuffer(Settings.tChParam[iLayerID], layerInputs[iInputIdx].FileInfo, SrcYuv);

  string LayerRecFileName = cfg.RecFileName;

  if(!LayerRecFileName.empty())
  {
    RecYuv = AllocateConversionBuffer(Settings.tChParam[iLayerID].uEncWidth, Settings.tChParam[iLayerID].uEncHeight, cfg.RecFourCC);

    if(RecYuv == nullptr)
      throw runtime_error("Couldn't allocate reconstruct conversion buffer");
    frameWriter = createFrameWriter(LayerRecFileName, cfg, RecYuv.get(), iLayerID);
  }

  TFrameInfo FrameInfo = GetFrameInfo(Settings.tChParam[iLayerID]);
  auto const eSrcMode = Settings.tChParam[0].eSrcMode;

  /* source compression case */
  pSrcConv = CreateSrcConverter(FrameInfo, eSrcMode, Settings.tChParam[iLayerID]);

  InitSrcBufPool(pAllocator, shouldConvert, pSrcConv, FrameInfo, eSrcMode, srcBuffersCount, SrcBufPool);

  iPictCount = 0;
  iReadCount = 0;
}

void LayerRessources::PushRessources(ConfigFile& cfg, EncoderSink* enc
                                     , EncoderLookAheadSink* encFirstPassLA
                                     )
{
  (void)cfg;
  QPBuffers::QPLayerInfo qpInf
  {
    &QpBufPool,
    layerInputs[iInputIdx].sQPTablesFolder,
    layerInputs[iInputIdx].sRoiFileName
  };

  enc->AddQpBufPool(qpInf, iLayerID);

  if(AL_TwoPassMngr_HasLookAhead(cfg.Settings))
    encFirstPassLA->AddQpBufPool(qpInf, iLayerID);

  if(frameWriter)
    enc->LayerRecOutput[iLayerID] = std::move(frameWriter);

  for(int i = 0; i < (int)StreamBufPoolConfig.uNumBuf; ++i)
  {
    AL_TBuffer* pStream = StreamBufPool.GetBuffer(AL_BUF_MODE_NONBLOCK);
    assert(pStream);

    AL_HEncoder hEnc = enc->hEnc;

    bool bRet = true;

    if(iLayerID == 0)
    {
      int iStreamNum = 1;

      // the look ahead needs one more stream buffer to work AVC due to (potential) multi-core
      if(AL_IS_AVC(cfg.Settings.tChParam[0].eProfile))
        iStreamNum += 1;

      if(AL_TwoPassMngr_HasLookAhead(cfg.Settings) && i < iStreamNum)
        hEnc = encFirstPassLA->hEnc;

      bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream);
    }
    assert(bRet);
    AL_Buffer_Unref(pStream);
  }
}

void LayerRessources::OpenInput(ConfigFile& cfg, AL_HEncoder hEnc)
{
  ChangeInput(cfg, iInputIdx, hEnc);
}

bool LayerRessources::SendInput(ConfigFile& cfg, IFrameSink* firstSink)
{
  firstSink->PreprocessFrame();

  return sendInputFileTo(YuvFile, SrcBufPool, SrcYuv.get(), cfg, layerInputs[iInputIdx].FileInfo, pSrcConv.get(), firstSink, iPictCount, iReadCount);
}

void LayerRessources::ChangeInput(ConfigFile& cfg, int iInputIdx, AL_HEncoder hEnc)
{
  (void)hEnc;

  if(iInputIdx < static_cast<int>(layerInputs.size()))
  {
    this->iInputIdx = iInputIdx;
    AL_TDimension inputDim = { layerInputs[iInputIdx].FileInfo.PictWidth, layerInputs[iInputIdx].FileInfo.PictHeight };
    bool bResChange = (inputDim.iWidth != AL_GetSrcWidth(cfg.Settings.tChParam[iLayerID])) || (inputDim.iHeight != AL_GetSrcHeight(cfg.Settings.tChParam[iLayerID]));

    if(bResChange)
    {
      /* No resize with dynamic resolution changes */
      cfg.Settings.tChParam[iLayerID].uEncWidth = cfg.Settings.tChParam[iLayerID].uSrcWidth = inputDim.iWidth;
      cfg.Settings.tChParam[iLayerID].uEncHeight = cfg.Settings.tChParam[iLayerID].uSrcHeight = inputDim.iHeight;

      AL_Encoder_SetInputResolution(hEnc, inputDim);
    }
    PrepareInput(YuvFile, layerInputs[iInputIdx].YUVFileName, layerInputs[iInputIdx].FileInfo, cfg.RunInfo.iFirstPict + iReadCount);
  }
}

/*****************************************************************************/
void SafeMain(int argc, char** argv)
{
  InitializePlateform();

  ConfigFile cfg {};
  SetDefaults(cfg);

  auto& Settings = cfg.Settings;
  auto& StreamFileName = cfg.BitstreamFileName;
  auto& RecFileName = cfg.RecFileName;
  auto& RunInfo = cfg.RunInfo;

  ParseCommandLine(argc, argv, cfg);

  DisplayVersionInfo();

  AL_Settings_SetDefaultParam(&Settings);
  SetMoreDefaults(cfg);

  if(!RecFileName.empty() || !cfg.RunInfo.sMd5Path.empty())
    Settings.tChParam[0].eEncOptions = (AL_EChEncOption)(Settings.tChParam[0].eEncOptions | AL_OPT_FORCE_REC);

  if(g_helpCfg)
  {
    PrintConfigFileUsage(cfg);
    exit(0);
  }

  if(g_helpCfgJson)
  {
    PrintConfigFileUsageJson(cfg);
    exit(0);
  }

  ValidateConfig(cfg);

  if(g_showCfg)
  {
    PrintConfig(cfg);
  }

  /* null if not supported */
  void* pTraceHook = nullptr;

  function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl = GetIpCtrlWrapper(RunInfo, pTraceHook);

  CIpDeviceParam param;
  param.iSchedulerType = RunInfo.iSchedulerType;
  param.iDeviceType = RunInfo.iDeviceType;
  param.pCfgFile = &cfg;
  param.bTrackDma = RunInfo.trackDma;
  param.iVqDescr = RunInfo.eVQDescr;

  auto pIpDevice = CreateIpDevice(param, wrapIpCtrl);

  if(!pIpDevice)
    throw runtime_error("Can't create IpDevice");

  std::vector<LayerRessources> layerRessources(cfg.Settings.NumLayer);

  unique_ptr<EncoderSink> enc;
  unique_ptr<EncoderLookAheadSink> encFirstPassLA;

  auto pAllocator = pIpDevice->m_pAllocator.get();
  auto pScheduler = pIpDevice->m_pScheduler;

  RCPlugin_Init(&cfg.Settings, &cfg.Settings.tChParam[0], pAllocator);
  auto ReleaseRcPlugin = scopeExit([&]()
  {
    AL_Allocator_Free(pAllocator, cfg.Settings.hRcPluginDmaContext);
  });

  // --------------------------------------------------------------------------------
  // Allocate Layers resources
  int frameBuffersCount = g_defaultMinBuffers + GetNumBufForGop(Settings);

  // the LookAhead needs LookAheadSize source buffers to work
  if(AL_TwoPassMngr_HasLookAhead(Settings))
  {
    int LANum = Settings.LookAhead;

    frameBuffersCount += LANum + (GetNumBufForGop(Settings) * 2);
  }

  int srcBuffersCount = g_numFrameToRepeat == 0 ? frameBuffersCount : max(frameBuffersCount, g_numFrameToRepeat);

  for(size_t i = 0; i < layerRessources.size(); i++)
    layerRessources[i].Init(cfg, frameBuffersCount, srcBuffersCount, i, pIpDevice.get());

  // --------------------------------------------------------------------------------
  // Create Encoder
  enc.reset(new EncoderSink(cfg, pScheduler, pAllocator
                            ));

  IFrameSink* firstSink = enc.get();

  if(AL_TwoPassMngr_HasLookAhead(cfg.Settings))
  {
    encFirstPassLA.reset(new EncoderLookAheadSink(cfg, enc.get(), pScheduler, pAllocator
                                                  ));

    encFirstPassLA->next = firstSink;
    firstSink = encFirstPassLA.get();
  }

  // --------------------------------------------------------------------------------
  // Push created layer resources
  for(size_t i = 0; i < layerRessources.size(); i++)
  {
    layerRessources[i].PushRessources(cfg, enc.get()
                                      , encFirstPassLA.get()
                                      );
  }

  enc->BitstreamOutput = createBitstreamWriter(StreamFileName, cfg);

  if(!cfg.RunInfo.bitrateFile.empty())
    enc->BitrateOutput = createBitrateWriter(cfg.RunInfo.bitrateFile, cfg);

  // --------------------------------------------------------------------------------
  // Set Callbacks
  enc->m_InputChanged = ([&](int iInputIdx, int iLayerID) {
    layerRessources[iLayerID].ChangeInput(cfg, iInputIdx, enc->hEnc);
  });

  enc->m_done = ([&]() {
    Rtos_SetEvent(layerRessources[0].hFinished);
  });

  if(!cfg.RunInfo.sMd5Path.empty())
  {
    auto multisink = unique_ptr<MultiSink>(new MultiSink);
    multisink->sinks.push_back(move(enc->LayerRecOutput[0]));
    multisink->sinks.push_back(createMd5Calculator(cfg.RunInfo.sMd5Path, cfg, layerRessources[0].RecYuv.get()));
    enc->LayerRecOutput[0] = move(multisink);
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

  bool hasInputAndNoError = true;

  for(int i = 0; i < Settings.NumLayer; ++i)
    layerRessources[i].OpenInput(cfg, enc->hEnc);

  while(hasInputAndNoError)
  {
    AL_64U uBeforeTime = Rtos_GetTime();

    for(int i = 0; i < Settings.NumLayer; ++i)
      hasInputAndNoError = layerRessources[i].SendInput(cfg, firstSink) && hasInputAndNoError;

    AL_64U uAfterTime = Rtos_GetTime();

    if((uAfterTime - uBeforeTime) < cfg.RunInfo.uInputSleepInMilliseconds)
      Rtos_Sleep(cfg.RunInfo.uInputSleepInMilliseconds - (uAfterTime - uBeforeTime));
  }

  for(int i = 0; i < Settings.NumLayer; ++i)
    layerRessources[i].WaitFinished();

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

