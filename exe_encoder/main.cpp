// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <climits>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <dirent.h>
#endif

#if defined(_WIN32)
#include "extra/dirent/include/dirent.h"
#endif

#include "lib_app/BufPool.hpp"
#include "lib_app/FileUtils.hpp"
#include "lib_app/PixMapBufPool.hpp"
#include "lib_app/YuvIO.hpp"
#include "lib_app/console.hpp"
#include "lib_app/plateform.hpp"
#include "lib_app/utils.hpp"
#include "lib_app/CommonCmdParser.hpp"
#include "lib_app/CompFrameCommon.hpp"
#include "lib_app/UnCompFrameReader.hpp"
#include "lib_app/SinkFrame.hpp"

#include "CfgParser.hpp"
#include "CodecUtils.hpp"
#include "IpDevice.hpp"
#include "IEncoderSink.hpp"

extern "C" {
#include "lib_common/PicFormat.h"
#include "lib_common/BufferPictureMeta.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/Error.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Round.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_encode/lib_encode.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "resource.h"
}

#include "lib_app/AL_RasterConvert.hpp"

#include "sink_encoder.hpp"
#include "sink_yuv_md5.hpp"
#include "sink_ratectrl_meta.hpp"
#include "sink_lookahead.hpp"
#include "QPGenerator.hpp"
#include "lib_app/SinkStreamMd5.hpp"
#include "sink_bitrate.hpp"
#include "sink_bitstream_writer.hpp"
#include "sink_repeater.hpp"

#include "RCPlugin.hpp"

static int32_t g_numFrameToRepeat;
static int32_t g_StrideHeight = -1;
static int32_t g_Stride = -1;
static int32_t constexpr g_defaultMinBuffers = 2;
static bool g_MultiChunk = false;

using namespace std;

static std::string toStringPathsSet(std::vector<std::string> paths)
{
  std::string out;

  for(auto const& path : paths)
  {
    if(out.length() != 0)
      out += string(", ");
    out += path;
  }

  return out;
}

/*****************************************************************************/
AL_HANDLE alignedAlloc(AL_TAllocator* pAllocator, char const* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset)
{
  *uAllocatedSize = 0;
  *uAlignmentOffset = 0;

  uSize += uAlign;

  auto pBuf = AL_Allocator_AllocNamed(pAllocator, uSize, pBufName);

  if(pBuf == nullptr)
    return nullptr;

  *uAllocatedSize = uSize;
  AL_PADDR pAddr = AL_Allocator_GetPhysicalAddr(pAllocator, pBuf);
  *uAlignmentOffset = AL_PhysAddrRoundUp(pAddr, uAlign) - pAddr;

  return pBuf;
}

/*****************************************************************************/

#include "lib_app/BuildInfo.hpp"

#if !HAS_COMPIL_FLAGS
#define AL_COMPIL_FLAGS ""
#endif

void DisplayBuildInfo(void)
{
  BuildInfoDisplay displayBuildInfo {
    SCM_REV_SW, SCM_BRANCH, AL_CONFIGURE_COMMANDLINE, AL_COMPIL_FLAGS, DELIVERY_BUILD_NUMBER, DELIVERY_SCM_REV, DELIVERY_DATE
  };
  displayBuildInfo.displayFeatures = [=](void)
                                     {
                                     };

  displayBuildInfo();
}

void DisplayVersionInfo(void)
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
  cfg.bDisableBitstreamOutput = false;
  cfg.RecFourCC = FOURCC(NULL);
  AL_Settings_SetDefaults(&cfg.Settings);
  cfg.MainInput.FileInfo.FourCC = FOURCC(I420);
  cfg.MainInput.FileInfo.FrameRate = 0;
  cfg.MainInput.FileInfo.PictHeight = 0;
  cfg.MainInput.FileInfo.PictWidth = 0;
  cfg.RunInfo.encDevicePaths = {};
  cfg.RunInfo.eDeviceType = AL_EDeviceType::AL_DEVICE_TYPE_BOARD;
  cfg.RunInfo.eSchedulerType = AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU;
  cfg.RunInfo.bLoop = false;
  cfg.RunInfo.iMaxPict = INT64_MAX; // ALL
  cfg.RunInfo.iFirstPict = 0;
  cfg.RunInfo.iScnChgLookAhead = 3;
  cfg.RunInfo.ipCtrlMode = AL_EIpCtrlMode::AL_IPCTRL_MODE_STANDARD;
  cfg.RunInfo.uInputSleepInMilliseconds = 0;
  cfg.strict_mode = false;
  cfg.iForceStreamBufSize = 0;
}

#include "lib_app/CommandLineParser.hpp"

static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cout << "Usage: " << ExeName << " -cfg <configfile> [options]" << endl;
  cout << "Options:" << endl;

  opt.usage();

  cout << endl << "Examples:" << endl;
  cout << "  " << ExeName << " -cfg test/config/encode_simple.cfg -r rec.yuv -o output.hevc -i input.yuv" << endl;
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

  if(s == "CHROMA_4_4_4")
    return AL_CHROMA_4_4_4;

  throw runtime_error("Unknown chroma mode: \"" + s + "\"");
}

template<typename T>
function<T(string const &)> createCmdlineParsingFunc(CfgParser* pCfgParser, char const* name_, function<T(ConfigFile &)> extractWantedValue)
{
  string name = name_;
  auto lambda = [=](string const& value)
                {
                  ConfigFile cfg {};
                  cfg.strict_mode = true;
                  string toParse = name + string("=") + value;
                  pCfgParser->ParseConfig(toParse, cfg);
                  return extractWantedValue(cfg);
                };
  return lambda;
}

function<TFourCC(string const &)> createParseInputFourCC(CfgParser* pCfgParser)
{
  return createCmdlineParsingFunc<TFourCC>(pCfgParser, "[INPUT]\nFormat", [](ConfigFile& cfg) { return cfg.MainInput.FileInfo.FourCC; });
}

function<TFourCC(string const &)> createParseRecFourCC(CfgParser* pCfgParser)
{
  return createCmdlineParsingFunc<TFourCC>(pCfgParser, "[OUTPUT]\nFormat", [](ConfigFile& cfg) { return cfg.RecFourCC; });
}

function<AL_EProfile(string const &)> createParseProfile(CfgParser* pCfgParser)
{
  return createCmdlineParsingFunc<AL_EProfile>(pCfgParser, "[SETTINGS]\nProfile", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].eProfile; });
}

function<AL_ERateCtrlMode(string const &)> createParseRCMode(CfgParser* pCfgParser)
{
  return createCmdlineParsingFunc<AL_ERateCtrlMode>(pCfgParser, "[RATE_CONTROL]\nRateCtrlMode", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].tRCParam.eRCMode; });
}

function<AL_EGopCtrlMode(string const &)> createParseGopMode(CfgParser* pCfgParser)
{
  return createCmdlineParsingFunc<AL_EGopCtrlMode>(pCfgParser, "[GOP]\nGopCtrlMode", [](ConfigFile& cfg) { return cfg.Settings.tChParam[0].tGopParam.eMode; });
}

void introspect(ConfigFile& cfg)
{
  (void)cfg;
  throw runtime_error("introspection is not compiled in");
}

static bool g_helpCfg = false;
static bool g_helpCfgJson = false;
static bool g_showCfg = false;

/*****************************************************************************/
void ParseCommandLine(int32_t argc, char** argv, ConfigFile& cfg, CfgParser& cfgParser)
{
  bool DoNotAcceptCfg = false;
  bool help = false;
  bool helpJson = false;
  bool version = false;
  stringstream warning;
  auto opt = CommandLineParser([&](string word)
  {
    if(word == "-cfg" || word == "-cfg-permissive" || word == "--cfg" || word == "--cfg-permissive")
    {
      if(DoNotAcceptCfg)
        throw runtime_error("Configuration files should be specified first, use -h to get help");
    }
    else
      DoNotAcceptCfg = true;
  });

  opt.addFlag("--help-cfg", &g_helpCfg, "Show cfg help. Show the default value for each parameter. If this option is used with a cfg, show the end value of each parameter.");
  opt.addFlag("--help-cfg-json", &g_helpCfgJson, "Same as --help-cfg but in json format");
  opt.addFlag("--show-cfg", &g_showCfg, "Show the cfg variables value before launching the program");

  opt.addFlag("--help,-h", &help, "Show this help");
  opt.addFlag("--help-json", &helpJson, "Show this help (json)");
  opt.addFlag("--version", &version, "Show version");

  opt.addOption("--cfg,-cfg", [&](string)
  {
    auto const cfgPath = opt.popWord();
    cfg.strict_mode = true;
    cfgParser.ParseConfigFile(cfgPath, cfg, warning);
  }, "Specify configuration file", "string");

  opt.addOption("--cfg-permissive,-cfg-permissive", [&](string)
  {
    auto const cfgPath = opt.popWord();
    cfgParser.ParseConfigFile(cfgPath, cfg, warning);
  }, "Use it instead of -cfg. Errors in the configuration file will be ignored", "string");

  opt.addOption("--set", [&](string)
  {
    cfgParser.ParseConfig(opt.popWord(), cfg);
  }, "Use the same syntax as in the cfg to specify a parameter. For instance: \"--set [INPUT]Width=512\", \"--set [GOP]Gop.Length=30\", ..", "string");

  opt.addString("--input,-i", &cfg.MainInput.YUVFileName, "YUV input file");
  opt.addString("--map,-m", &cfg.MainInput.sMapFileName, "Map input file");
  opt.addString("--output,-o", &cfg.BitstreamFileName, "Compressed output file");
  opt.addFlag("--no-output", &cfg.bDisableBitstreamOutput, "Disable writing compressed output file");
  opt.addString("--output-rec,-r", &cfg.RecFileName, "Output reconstructed YUV file");
  opt.addString("--md5-rec", &cfg.RunInfo.sRecMd5Path, "Filename to the output MD5 of the reconstructed pictures");
  opt.addString("--md5-stream", &cfg.RunInfo.sStreamMd5Path, "Filename to the output MD5 of the bitstream");
  opt.addInt("--input-width", &cfg.MainInput.FileInfo.PictWidth, "Specifies YUV input width");
  opt.addInt("--input-height", &cfg.MainInput.FileInfo.PictHeight, "Specifies YUV input height");
  opt.addCustom("--input-format", &cfg.MainInput.FileInfo.FourCC, createParseInputFourCC(&cfgParser), "Specifies YUV input format (I420, IYUV, YV12, NV12, Y800, Y010, P0AL, I0AL ...)", "enum");
  opt.addCustom("--rec-format", &cfg.RecFourCC, createParseRecFourCC(&cfgParser), "Specifies output format", "enum");

  opt.addInt("--level", &cfg.Settings.tChParam[0].uLevel, "Specifies the level we want to encode with (10 to 62)");
  opt.addCustom("--profile", &cfg.Settings.tChParam[0].eProfile, createParseProfile(&cfgParser), string { "Specifies the profile we want to encode with (examples: " } +
                string { "HEVC_MAIN, " } +
                string { "AVC_MAIN, " } +
                string { "..)" }, "enum");
  opt.addOption("--chroma-mode", [&](string)
  {
    auto chromaMode = stringToChromaMode(opt.popWord());
    AL_SET_CHROMA_MODE(&cfg.Settings.tChParam[0].ePicFormat, chromaMode);
  }, string { "Specify chroma-mode (CHROMA_MONO, CHROMA_4_0_0" } +
                string { ", CHROMA_4_2_0" } +
                string { ", CHROMA_4_2_2" } +
                string { ")" }, "enum");

  opt.addInt("--out-bitdepth", &cfg.iOutputBitdepth, string { "Specifies bitdepth of output stream (8" } +
             string { ", 10" } +
             string { ")" });

  opt.addInt("--num-slices", &cfg.Settings.tChParam[0].uNumSlices, "Specifies the number of slices to use");
  opt.addFlag("--slicelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Enable subframe latency");
  opt.addFlag("--framelat", &cfg.Settings.tChParam[0].bSubframeLatency, "Disable subframe latency", false);

  opt.startSection("Rate Control && GOP");
  opt.addCustom("--ratectrl-mode", &cfg.Settings.tChParam[0].tRCParam.eRCMode, createParseRCMode(&cfgParser),
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
    cfgParser.ParseConfig("[RATE_CONTROL]\nFrameRate=" + opt.popWord(), cfg);
  }, "Specifies the frame rate used for encoding", "number");
  opt.addInt("--sliceQP", &cfg.Settings.tChParam[0].tRCParam.iInitialQP, "Specifies the initial slice QP");

  opt.addCustom("--gop-mode", &cfg.Settings.tChParam[0].tGopParam.eMode, createParseGopMode(&cfgParser), "Specifies gop control mode (DEFAULT_GOP, LOW_DELAY_P)", "enum");
  opt.addInt("--gop-length", &cfg.Settings.tChParam[0].tGopParam.uGopLength, "Specifies the GOP length, 1 means I slice only");
  opt.addInt("--gop-numB", &cfg.Settings.tChParam[0].tGopParam.uNumB, "Number of consecutive B frame (0 .. 4)");

  opt.startSection("Run");

  opt.addInt("--first-picture", &cfg.RunInfo.iFirstPict, "First picture encoded (skip those before)");
  opt.addInt("--max-picture", &cfg.RunInfo.iMaxPict, "Maximum number of pictures encoded (1,2 .. -1 for ALL)");
  opt.addFlag("--loop", &cfg.RunInfo.bLoop, "Loop at the end of the yuv file");

  bool dummyNextChan; // As the --next-channel is parsed elsewhere, this option is only used to add the description in the usage
  opt.addFlag("--next-chan", &dummyNextChan, "Start the configuration of a new encoding channel.");

  opt.addInt("--input-sleep", &cfg.RunInfo.uInputSleepInMilliseconds, "Minimum waiting time in milliseconds between each process frame (0 by default)");

  opt.startSection("Traces && Debug");

  opt.addInt("--stride-height", &g_StrideHeight, "Chroma offset (vertical stride)");
  opt.addInt("--stride", &g_Stride, "Luma stride");
  opt.addFlag("--multi-chunk", &g_MultiChunk, "Allocate source luma and chroma on different memory chunks");
  opt.addInt("--num-core", &cfg.Settings.tChParam[0].uNumCore, "Specifies the number of cores to use (resolution needs to be sufficient)");

  opt.addInt("--stream-buf-size", &cfg.iForceStreamBufSize, "Specify stream buffers size");
  opt.addFlag("--non-realtime", &cfg.Settings.tChParam[0].bNonRealtime, "Specifies that the channel is a non-realtime channel");
  opt.addFlag("--print-picture-type", &cfg.RunInfo.printPictureType, "Write picture type for each frame in the file", true);

  opt.addOption("--device", [&](string) {
    cfg.RunInfo.encDevicePaths.push_back(opt.popWord());
  }, std::string(std::string("Path of the driver device(s) file(s) used to talk with the IP. Default(s) are: ") + toStringPathsSet(ENCODER_DEVICES)));
  opt.startSection("Misc");

  opt.addOption("--color", [&](string)
  {
    SetEnableColor(true);
  }, "Enable the display of command line color (Default: Auto)");

  opt.addOption("--no-color", [&](string)
  {
    SetEnableColor(false);
  }, "Disable the display of command line color");

  opt.addFlag("--quiet,-q", &g_Verbosity, "Do not print anything", 0);
  opt.addInt("--verbosity", &g_Verbosity, "Choose the verbosity level (-q is equivalent to --verbosity 0)");

  opt.startDeprecatedSection();
  opt.addString("--md5", &cfg.RunInfo.sRecMd5Path, "Use --md5-rec instead");
  opt.addInt("--ip-bitdepth", &cfg.iOutputBitdepth, "Use --out-bitdepth instead");

  opt.addFlag("--diagnostic", &cfg.Settings.bDiagnostic, "Additional checks meant for debugging. Not to be used on real usecases, might slow down encoding.");

  bool bHasDeprecated = opt.parse(argc, argv);
  cfgParser.PostParsingConfiguration(cfg, warning);

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

  if(bHasDeprecated && g_Verbosity)
    opt.usageDeprecated();

  if(g_Verbosity)
    cerr << warning.str();
}

/*****************************************************************************/
static void SetHardwareResolutions(ConfigFile& cfg)
{
  int32_t iMaxSrcWidth = cfg.MainInput.FileInfo.PictWidth;
  int32_t iMaxSrcHeight = cfg.MainInput.FileInfo.PictHeight;

  for(auto const& input: cfg.DynamicInputs)
  {
    iMaxSrcWidth = max(input.FileInfo.PictWidth, iMaxSrcWidth);
    iMaxSrcHeight = max(input.FileInfo.PictHeight, iMaxSrcHeight);
  }

  cfg.Settings.tChParam[0].uSrcWidth = iMaxSrcWidth;
  cfg.Settings.tChParam[0].uSrcHeight = iMaxSrcHeight;

  cfg.Settings.tChParam[0].uEncWidth = cfg.Settings.tChParam[0].uSrcWidth;
  cfg.Settings.tChParam[0].uEncHeight = cfg.Settings.tChParam[0].uSrcHeight;

  if(cfg.Settings.tChParam[0].bEnableSrcCrop)
  {
    cfg.Settings.tChParam[0].uEncWidth = cfg.Settings.tChParam[0].uSrcCropWidth;
    cfg.Settings.tChParam[0].uEncHeight = cfg.Settings.tChParam[0].uSrcCropHeight;
  }

}

static void SetHardwareBitDepths(ConfigFile& cfg)
{
  if(cfg.iOutputBitdepth != -1)
    AL_SET_BITDEPTH(&cfg.Settings.tChParam[0].ePicFormat, cfg.iOutputBitdepth);

  cfg.Settings.tChParam[0].uSrcBitDepth = AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat);
}

void SetMoreDefaults(ConfigFile& cfg)
{
  auto& FileInfo = cfg.MainInput.FileInfo;
  auto& Settings = cfg.Settings;
  auto& RecFourCC = cfg.RecFourCC;
  auto& RunInfo = cfg.RunInfo;

  if(RunInfo.encDevicePaths.empty())
    RunInfo.encDevicePaths = ENCODER_DEVICES;

  if(FileInfo.FrameRate == 0)
    FileInfo.FrameRate = Settings.tChParam[0].tRCParam.uFrameRate;

  if(RecFourCC == FOURCC(NULL))
  {
    AL_TPicFormat tOutPicFormat;

    if(AL_GetPicFormat(FileInfo.FourCC, &tOutPicFormat))
    {
      if(tOutPicFormat.eComponentOrder != AL_COMPONENT_ORDER_RGB && tOutPicFormat.eComponentOrder != AL_COMPONENT_ORDER_BGR)
        tOutPicFormat.eChromaMode = AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat);
      tOutPicFormat.ePlaneMode = tOutPicFormat.eChromaMode == AL_CHROMA_MONO ? AL_PLANE_MODE_MONOPLANE : tOutPicFormat.ePlaneMode;
      tOutPicFormat.uBitDepth = AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat);
      RecFourCC = AL_GetFourCC(&tOutPicFormat);
    }
    else
    {
      RecFourCC = FileInfo.FourCC;
    }
  }

  AL_TPicFormat tRecPicFormat;

  if(AL_GetPicFormat(RecFourCC, &tRecPicFormat))
  {
    auto& RecFileName = cfg.RecFileName;

    if(!RecFileName.empty())
      if(tRecPicFormat.eStorageMode != AL_FB_RASTER && !tRecPicFormat.bCompressed)
        throw runtime_error("Reconstructed storage format can only be tiled if compressed.");
  }
}

AL_ESrcMode SrcFormatToSrcMode(AL_ESrcFormat eSrcFormat)
{
  switch(eSrcFormat)
  {
  case AL_SRC_FORMAT_RASTER:
    return AL_SRC_RASTER;
  default:
    throw runtime_error("Unsupported source format.");
  }
}

void AdjustSettingsAfterParsing(ConfigFile& cfg)
{
  SetHardwareResolutions(cfg);

  SetHardwareBitDepths(cfg);

  if(AL_IS_STILL_PROFILE(cfg.Settings.tChParam[0].eProfile))
    cfg.RunInfo.iMaxPict = 1;

  auto& Settings = cfg.Settings;
  auto& RecFileName = cfg.RecFileName;
  auto& RunInfo = cfg.RunInfo;

  AL_Settings_SetDefaultParam(&Settings);
  SetMoreDefaults(cfg);

  if(!RecFileName.empty() || !RunInfo.sRecMd5Path.empty())
  {

    Settings.tChParam[0].eEncOptions = (AL_EChEncOption)(Settings.tChParam[0].eEncOptions | AL_OPT_FORCE_REC);
  }

  AL_ESrcMode eSrcMode = SrcFormatToSrcMode(cfg.eSrcFormat);

  for(uint8_t uLayer = 0; uLayer < cfg.Settings.NumLayer; uLayer++)
    Settings.tChParam[uLayer].eSrcMode = eSrcMode;
}

/*****************************************************************************/
bool CheckQPTableFolder(ConfigFile& cfg)
{
  std::regex qp_file_per_frame_regex("QP(^|)(s|_[0-9]+)\\.hex");

  if(!FolderExists(cfg.MainInput.sQPTablesFolder))
    return false;

  return FileExists(cfg.MainInput.sQPTablesFolder, qp_file_per_frame_regex);
}

void ValidateConfig(ConfigFile& cfg)
{
  string const invalid_settings("Invalid settings, check the [SETTINGS] section of your configuration file or check your commandline (use -h to get help)");

  if(cfg.MainInput.YUVFileName.empty())
    throw runtime_error("No YUV input was given, specify it in the [INPUT] section of your configuration file or in your commandline (use -h to get help)");

  if(!cfg.MainInput.sQPTablesFolder.empty() && ((cfg.RunInfo.eGenerateQpMode & AL_GENERATE_QP_TABLE_MASK) != AL_GENERATE_LOAD_QP))
    throw runtime_error("QPTablesFolder can only be specified with Load QP control mode");

  SetConsoleColor(CC_RED);

  FILE* out = stdout;

  if(!g_Verbosity)
    out = nullptr;

  auto const MaxLayer = cfg.Settings.NumLayer - 1;

  for(int32_t i = 0; i < cfg.Settings.NumLayer; ++i)
  {
    auto const err = AL_Settings_CheckValidity(&cfg.Settings, &cfg.Settings.tChParam[i], out);

    if(err != 0)
    {
      stringstream ss;
      ss << "Found: " << err << " errors(s). " << invalid_settings;
      throw runtime_error(ss.str());
    }

    if((cfg.RunInfo.eGenerateQpMode & AL_GENERATE_QP_TABLE_MASK) == AL_GENERATE_LOAD_QP)
    {
      if(!CheckQPTableFolder(cfg))
        throw runtime_error("No QP File found");
    }

    auto const incoherencies = AL_Settings_CheckCoherency(&cfg.Settings, &cfg.Settings.tChParam[i], cfg.MainInput.FileInfo.FourCC, out);

    if(incoherencies < 0)
      throw runtime_error("Fatal coherency error in settings (layer[" + to_string(i) + "/" + to_string(MaxLayer) + "])");

  }

  if(cfg.Settings.TwoPass == 1)
    AL_TwoPassMngr_SetPass1Settings(cfg.Settings);

  SetConsoleColor(CC_DEFAULT);
}

/*****************************************************************************/
static shared_ptr<AL_TBuffer> AllocateConversionBuffer(int32_t iWidth, int32_t iHeight, TFourCC tFourCC)
{
  AL_TBuffer* pYuv = AllocateDefaultYuvIOBuffer(AL_TDimension { iWidth, iHeight }, tFourCC);

  if(pYuv == nullptr)
    return nullptr;
  return shared_ptr<AL_TBuffer>(pYuv, &AL_Buffer_Destroy);
}

bool ReadSourceFrameBuffer(AL_TBuffer* pBuffer, AL_TBuffer* conversionBuffer, unique_ptr<FrameReader> const& frameReader, AL_TDimension tUpdatedDim, IConvSrc* hConv)
{

  AL_PixMapBuffer_SetDimension(pBuffer, tUpdatedDim);

  if(hConv)
  {
    AL_PixMapBuffer_SetDimension(conversionBuffer, tUpdatedDim);

    if(!frameReader->ReadFrame(conversionBuffer))
      return false;
    hConv->ConvertSrcBuf(conversionBuffer, pBuffer);
  }
  else
    return frameReader->ReadFrame(pBuffer);

  return true;
}

shared_ptr<AL_TBuffer> ReadSourceFrame(BaseBufPool* pBufPool, AL_TBuffer* conversionBuffer, unique_ptr<FrameReader> const& frameReader, AL_TDimension tUpdatedDim, IConvSrc* hConv)
{
  shared_ptr<AL_TBuffer> sourceBuffer = pBufPool->GetSharedBuffer();

  if(sourceBuffer == nullptr)
    throw runtime_error("sourceBuffer must exist");

  if(!ReadSourceFrameBuffer(sourceBuffer.get(), conversionBuffer, frameReader, tUpdatedDim, hConv))
    return nullptr;
  return sourceBuffer;
}

AL_TPicFormat GetSrcPicFormat(AL_TEncChanParam const& tChParam)
{
  AL_ESrcMode eSrcMode = tChParam.eSrcMode;
  auto eChromaMode = AL_GET_CHROMA_MODE(tChParam.ePicFormat);

  return AL_EncGetSrcPicFormat(eChromaMode, tChParam.uSrcBitDepth, eSrcMode);
}

struct SrcConverterParams
{
  AL_TDimension tDim;
  TFourCC tFileFourCC;
  AL_TPicFormat tSrcPicFmt;
  AL_ESrcFormat eSrcFormat;
};

bool IsConversionNeeded(SrcConverterParams& tSrcConverterParams)
{

  const TFourCC tSrcFourCC = AL_GetFourCC(&tSrcConverterParams.tSrcPicFmt);

  if(tSrcConverterParams.tFileFourCC != tSrcFourCC)
  {
    if(AL_IsCompatible(tSrcConverterParams.tFileFourCC, tSrcFourCC))
      // Update PicFormat to avoid conversion
      AL_GetPicFormat(tSrcConverterParams.tFileFourCC, &tSrcConverterParams.tSrcPicFmt);
    else
      return true;
  }

  return false;
}

unique_ptr<IConvSrc> AllocateSrcConverter(SrcConverterParams const& tSrcConverterParams, shared_ptr<AL_TBuffer>& pFileReaderYuv)
{
  // ********** Allocate the YUV buffer to read in the file **********
  pFileReaderYuv = AllocateConversionBuffer(tSrcConverterParams.tDim.iWidth, tSrcConverterParams.tDim.iHeight, tSrcConverterParams.tFileFourCC);

  if(pFileReaderYuv == nullptr)
    throw runtime_error("Couldn't allocate source conversion buffer");

  // ************* Allocate the YUV converter *************
  TFrameInfo tSrcFrameInfo {
    tSrcConverterParams.tDim, tSrcConverterParams.tSrcPicFmt.uBitDepth, tSrcConverterParams.tSrcPicFmt.eChromaMode
  };
  (void)tSrcFrameInfo;

  switch(tSrcConverterParams.eSrcFormat)
  {
  case AL_SRC_FORMAT_RASTER:
    return make_unique<CYuvSrcConv>(tSrcFrameInfo);
  default:
    throw runtime_error("Unsupported source conversion.");
  }

  return nullptr;
}

static int32_t ComputeYPitch(int32_t iWidth, const AL_TPicFormat& tPicFormat)
{
  auto iPitch = AL_EncGetMinPitch(iWidth, &tPicFormat);

  if(g_Stride != -1)
  {
    if(g_Stride < iPitch)
      throw runtime_error("g_Stride(" + to_string(g_Stride) + ") must be higher or equal than iPitch(" + to_string(iPitch) + ")");
    iPitch = g_Stride;
  }
  return iPitch;
}

static bool isLastPict(AL_64S iPictCount, AL_64S iMaxPict)
{
  return (iPictCount >= iMaxPict) && (iMaxPict != -1);
}

static shared_ptr<AL_TBuffer> GetSrcFrame(AL_64S& iReadCount, AL_64S iPictCount, unique_ptr<FrameReader> const& frameReader, AL_TYUVFileInfo const& FileInfo, PixMapBufPool& SrcBufPool, AL_TBuffer* Yuv, AL_TEncChanParam const& tChParam, ConfigFile const& cfg, IConvSrc* pSrcConv)
{
  shared_ptr<AL_TBuffer> frame;

  if(!isLastPict(iPictCount, cfg.RunInfo.iMaxPict))
  {
    if(cfg.MainInput.FileInfo.FrameRate != tChParam.tRCParam.uFrameRate)
    {
      iReadCount += frameReader->GotoNextPicture(FileInfo.FrameRate, tChParam.tRCParam.uFrameRate, iPictCount, iReadCount);
    }

    auto tUpdatedDim = AL_TDimension {
      AL_GetSrcWidth(tChParam), AL_GetSrcHeight(tChParam)
    };
    frame = ReadSourceFrame(&SrcBufPool, Yuv, frameReader, tUpdatedDim, pSrcConv);

    iReadCount++;
  }
  return frame;
}

/*****************************************************************************/
static bool InitQpBufPool(BufPool& pool, AL_TEncSettings& Settings, AL_TEncChanParam& tChParam, int32_t frameBuffersCount, AL_TAllocator* pAllocator)
{
  (void)Settings;

  if(!AL_IS_QP_TABLE_REQUIRED(Settings.eQpTableMode))
    return true;

  AL_TDimension tDim {
    tChParam.uEncWidth, tChParam.uEncHeight
  };
  return pool.Init(pAllocator, frameBuffersCount, AL_GetAllocSizeEP2(tDim, static_cast<AL_ECodec>(AL_GET_CODEC(tChParam.eProfile)), tChParam.uLog2MaxCuSize), nullptr, "qp-ext");
}

/*****************************************************************************/
struct SrcBufChunk
{
  int32_t iChunkSize;
  std::vector<AL_TPlaneDescription> vPlaneDesc;
};

struct SrcBufDesc
{
  TFourCC tFourCC;
  std::vector<SrcBufChunk> vChunks;
};

static SrcBufDesc GetSrcBufDescription(AL_TDimension tDimension, uint8_t uBitDepth, AL_EChromaMode eCMode, AL_ESrcMode eSrcMode, AL_ECodec eCodec)
{
  (void)eCodec;

  AL_TPicFormat const tPicFormat = AL_EncGetSrcPicFormat(eCMode, uBitDepth, eSrcMode);

  SrcBufDesc srcBufDesc =
  {
    AL_GetFourCC(&tPicFormat), {}
  };

  int32_t iPitchY = ComputeYPitch(tDimension.iWidth, tPicFormat);

  int32_t iAlignValue = 8;

  int32_t iStrideHeight = g_StrideHeight != -1 ? g_StrideHeight : AL_RoundUp(tDimension.iHeight, iAlignValue);

  SrcBufChunk srcChunk {};

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int32_t iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat, usedPlanes);

  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int32_t iPitch = usedPlanes[iPlane] == AL_PLANE_Y || usedPlanes[iPlane] == AL_PLANE_YUV ? iPitchY : AL_GetChromaPitch(srcBufDesc.tFourCC, iPitchY);
    srcChunk.vPlaneDesc.push_back(AL_TPlaneDescription { usedPlanes[iPlane], srcChunk.iChunkSize, iPitch });
    srcChunk.iChunkSize += AL_GetAllocSizeSrc_PixPlane(&tPicFormat, iPitchY, iStrideHeight, usedPlanes[iPlane]);

    if(g_MultiChunk)
    {
      srcBufDesc.vChunks.push_back(srcChunk);
      srcChunk = {};
    }
  }

  if(!g_MultiChunk)
    srcBufDesc.vChunks.push_back(srcChunk);

  return srcBufDesc;
}

/*****************************************************************************/
static uint8_t GetNumBufForGop(AL_TEncSettings Settings)
{
  int32_t uNumFields = 1;

  if(AL_IS_INTERLACED(Settings.tChParam[0].eVideoMode))
    uNumFields = 2;
  int32_t uAdditionalBuf = 0;
  return uNumFields * Settings.tChParam[0].tGopParam.uNumB + uAdditionalBuf;
}

/*****************************************************************************/
static bool InitStreamBufPool(BufPool& pool, AL_TEncSettings& Settings, int32_t iLayerID, uint8_t uNumCore, int32_t iForcedStreamBufferSize, AL_TAllocator* pAllocator)
{
  (void)uNumCore;

  int32_t numStreams;

  AL_TDimension dim {
    Settings.tChParam[iLayerID].uEncWidth, Settings.tChParam[iLayerID].uEncHeight
  };
  uint64_t streamSize = iForcedStreamBufferSize;

  if(streamSize == 0)
  {
    streamSize = AL_GetMitigatedMaxNalSize(dim, AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat), AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat));

    bool bIsXAVCIntraCBG = AL_IS_XAVC_CBG(Settings.tChParam[0].eProfile) && AL_IS_INTRA_PROFILE(Settings.tChParam[0].eProfile);

    if(bIsXAVCIntraCBG)
      streamSize = AL_GetMaxNalSize(dim, AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat), AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat), Settings.tChParam[0].eProfile, Settings.tChParam[0].uLevel);
  }

  {
    static const int32_t smoothingStream = 2;
    numStreams = g_defaultMinBuffers + smoothingStream + GetNumBufForGop(Settings);
  }

  bool bHasLookAhead;
  bHasLookAhead = AL_TwoPassMngr_HasLookAhead(Settings);

  if(bHasLookAhead)
  {
    int32_t extraLookAheadStream = 1;

    // the look ahead needs one more stream buffer to work in AVC due to (potential) multi-core
    if(AL_IS_AVC(Settings.tChParam[0].eProfile))
      extraLookAheadStream += 1;
    numStreams += extraLookAheadStream;
  }

  if(Settings.tChParam[0].bSubframeLatency)
  {
    numStreams *= Settings.tChParam[0].uNumSlices;

    {
      /* Due to rounding, the slices don't have all the same height. Compute size of the biggest slice */
      uint64_t lcuSize = 1LL << Settings.tChParam[0].uLog2MaxCuSize;
      uint64_t rndHeight = AL_RoundUp(dim.iHeight, lcuSize);
      streamSize = streamSize * lcuSize * (1 + rndHeight / (Settings.tChParam[0].uNumSlices * lcuSize)) / rndHeight;

      /* we need space for the headers on each slice */
      streamSize += AL_ENC_MAX_HEADER_SIZE;
      /* stream size is required to be 32bytes aligned */
      streamSize = AL_RoundUp(streamSize, HW_IP_BURST_ALIGNMENT);
    }
  }

  if(streamSize > INT32_MAX)
    throw runtime_error("streamSize(" + to_string(streamSize) + ") must be lower or equal than INT32_MAX(" + to_string(INT32_MAX) + ")");

  auto pMetaData = (AL_TMetaData*)AL_StreamMetaData_Create(AL_MAX_SECTION);
  bool bSucceed = pool.Init(pAllocator, numStreams, streamSize, pMetaData, "stream");
  AL_MetaData_Destroy(pMetaData);

  return bSucceed;
}

/*****************************************************************************/
static void InitSrcBufPool(PixMapBufPool& SrcBufPool, AL_TAllocator* pAllocator, TFrameInfo& FrameInfo, AL_ESrcMode eSrcMode, int32_t frameBuffersCount, AL_ECodec eCodec)
{
  auto srcBufDesc = GetSrcBufDescription(FrameInfo.tDimension, FrameInfo.iBitDepth, FrameInfo.eCMode, eSrcMode, eCodec);

  SrcBufPool.SetFormat(FrameInfo.tDimension, srcBufDesc.tFourCC);

  for(auto& vChunk : srcBufDesc.vChunks)
    SrcBufPool.AddChunk(vChunk.iChunkSize, vChunk.vPlaneDesc);

  bool const ret = SrcBufPool.Init(pAllocator, frameBuffersCount, "input");

  if(!ret)
    throw std::runtime_error("src buf pool must succeed init");
}

/*****************************************************************************/

/*****************************************************************************/
struct LayerResources
{
  void Init(ConfigFile& cfg, AL_TEncoderInfo tEncInfo, int32_t iLayerID, CIpDevice* pDevices, int32_t chanId);

  void PushResources(ConfigFile& cfg, EncoderSink* enc
                     , EncoderLookAheadSink* encFirstPassLA
                     );

  void OpenEncoderInput(ConfigFile& cfg);

  void ChangeEncoderInput(ConfigFile& cfg, int32_t iInputIdx);

  bool SendInput(ConfigFile& cfg, IEncoderSink* pEncoderSink, void* pTraceHook);

  bool sendInputFileTo(unique_ptr<FrameReader>& frameReader, PixMapBufPool& SrcBufPool, AL_TBuffer* Yuv, ConfigFile const& cfg, AL_TYUVFileInfo& FileInfo, IConvSrc* pSrcConv, IEncoderSink* pEncoderSink, AL_64S& iPictCount, AL_64S& iReadCount);

  unique_ptr<FrameReader> InitializeFrameReader(ConfigFile& cfg, ifstream& YuvFile, string sYuvFileName, ifstream& MapFile, string sMapFileName, AL_TYUVFileInfo& FileInfo);

  BufPool StreamBufPool;
  BufPool QpBufPool;
  PixMapBufPool SrcBufPool;

  // Input/Output Format conversion
  ifstream YuvFile;
  ifstream MapFile;
  unique_ptr<FrameReader> frameReader;
  unique_ptr<IConvSrc> pSrcConv;
  shared_ptr<AL_TBuffer> SrcYuv;

  vector<uint8_t> RecYuvBuffer;
  unique_ptr<IFrameSink> frameWriter;

  AL_64S iPictCount = 0;
  AL_64S iReadCount = 0;

  int32_t iLayerID = 0;
  int32_t iInputIdx = 0;
  vector<TConfigYUVInput> layerInputs;
};

void LayerResources::Init(ConfigFile& cfg, AL_TEncoderInfo tEncInfo, int32_t iLayerID, CIpDevice* pDevices, int32_t chanId)
{
  AL_TEncSettings& Settings = cfg.Settings;
  auto const eSrcMode = Settings.tChParam[iLayerID].eSrcMode;

  (void)chanId;
  this->iLayerID = iLayerID;

  {
    layerInputs.push_back(cfg.MainInput);
    layerInputs.insert(layerInputs.end(), cfg.DynamicInputs.begin(), cfg.DynamicInputs.end());
  }

  AL_TAllocator* pAllocator = pDevices->GetAllocator();

  // --------------------------------------------------------------------------------
  // Stream Buffers
  // --------------------------------------------------------------------------------
  if(!InitStreamBufPool(StreamBufPool, Settings, iLayerID, tEncInfo.uNumCore, cfg.iForceStreamBufSize, pAllocator))
    throw std::runtime_error("Error creating stream buffer pool");

  AL_TDimension tDim {
    Settings.tChParam[iLayerID].uEncWidth, Settings.tChParam[iLayerID].uEncHeight
  };

  bool bUsePictureMeta = false;
  bUsePictureMeta |= cfg.RunInfo.printPictureType;

  bUsePictureMeta |= AL_TwoPassMngr_HasLookAhead(Settings);

  if(iLayerID == 0 && bUsePictureMeta)
  {
    auto pMeta = (AL_TMetaData*)AL_PictureMetaData_Create();

    if(pMeta == nullptr)
      throw std::runtime_error("Meta must be created");
    bool const bRet = StreamBufPool.AddMetaData(pMeta);

    if(!bRet)
      throw std::runtime_error("Meta must be added in stream pool");
    AL_MetaData_Destroy(pMeta);
  }

  if(cfg.RunInfo.rateCtrlStat != AL_RATECTRL_STAT_MODE_NONE)
  {
    auto pMeta = (AL_TMetaData*)AL_RateCtrlMetaData_CustomCreate(pAllocator, cfg.RunInfo.rateCtrlStat, tDim, Settings.tChParam[iLayerID].uLog2MaxCuSize, AL_GET_CODEC(Settings.tChParam[iLayerID].eProfile));

    if(pMeta == nullptr)
      throw std::runtime_error("Meta must be created");
    bool const bRet = StreamBufPool.AddMetaData(pMeta);

    if(!bRet)
      throw std::runtime_error("Meta must be added in stream pool");

    AL_MetaData_Destroy(pMeta);
  }
  // --------------------------------------------------------------------------------
  // Tuning Input Buffers
  // --------------------------------------------------------------------------------
  int32_t frameBuffersCount = g_defaultMinBuffers + GetNumBufForGop(Settings);
  {
    frameBuffersCount = g_defaultMinBuffers + GetNumBufForGop(Settings);

    if(AL_TwoPassMngr_HasLookAhead(Settings))
    {
      frameBuffersCount += Settings.LookAhead + (GetNumBufForGop(Settings) * 2);

      if(AL_IS_AVC(cfg.Settings.tChParam[0].eProfile))
        frameBuffersCount += 1;
    }

  }

  if(!InitQpBufPool(QpBufPool, Settings, Settings.tChParam[iLayerID], frameBuffersCount, pAllocator))
    throw std::runtime_error("Error creating QP buffer pool");

  // --------------------------------------------------------------------------------
  // Application Input/Output Format conversion
  // --------------------------------------------------------------------------------
  const AL_TPicFormat tSrcPicFmt = GetSrcPicFormat(Settings.tChParam[iLayerID]);
  SrcConverterParams tSrcConverterParams =
  {
    { AL_GetSrcWidth(Settings.tChParam[iLayerID]), AL_GetSrcHeight(Settings.tChParam[iLayerID]) },
    layerInputs[iInputIdx].FileInfo.FourCC,
    tSrcPicFmt,
    cfg.eSrcFormat,
  };

  if(IsConversionNeeded(tSrcConverterParams))
    pSrcConv = AllocateSrcConverter(tSrcConverterParams, SrcYuv);

  TFrameInfo tSrcFrameInfo {
    tSrcConverterParams.tDim, tSrcConverterParams.tSrcPicFmt.uBitDepth, tSrcConverterParams.tSrcPicFmt.eChromaMode
  };

  // --------------------------------------------------------------------------------
  // Source Buffers
  // --------------------------------------------------------------------------------
  int32_t srcBuffersCount = max(frameBuffersCount, g_numFrameToRepeat);

  InitSrcBufPool(SrcBufPool, pAllocator, tSrcFrameInfo, eSrcMode, srcBuffersCount, static_cast<AL_ECodec>(AL_GET_CODEC(Settings.tChParam[0].eProfile)));

  iPictCount = 0;
  iReadCount = 0;
}

void LayerResources::PushResources(ConfigFile& cfg, EncoderSink* enc
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
  {
    encFirstPassLA->AddQpBufPool(qpInf, iLayerID);
  }

  if(frameWriter)
    enc->RecOutput[iLayerID] = std::move(frameWriter);

  for(int32_t i = 0; i < (int)StreamBufPool.GetNumBuf(); ++i)
  {
    std::shared_ptr<AL_TBuffer> pStream = StreamBufPool.GetSharedBuffer(AL_EBufMode::AL_BUF_MODE_NONBLOCK);

    if(pStream == nullptr)
      throw runtime_error("pStream must exist");

    AL_HEncoder hEnc = enc->hEnc;

    bool bRet = true;

    if(iLayerID == 0)
    {
      int32_t iStreamNum = 1;

      // the look ahead needs one more stream buffer to work AVC due to (potential) multi-core
      if(AL_IS_AVC(cfg.Settings.tChParam[0].eProfile))
        iStreamNum += 1;

      if(AL_TwoPassMngr_HasLookAhead(cfg.Settings) && i < iStreamNum)
        hEnc = encFirstPassLA->hEnc;

      bRet = AL_Encoder_PutStreamBuffer(hEnc, pStream.get());
    }

    if(!bRet)
      throw std::runtime_error("bRet must be true");
  }
}

void LayerResources::OpenEncoderInput(ConfigFile& cfg)
{
  if(iInputIdx >= static_cast<int>(layerInputs.size()))
    throw std::runtime_error("Invalid source input index!");

  AL_TDimension tInputDim {
    layerInputs[iInputIdx].FileInfo.PictWidth, layerInputs[iInputIdx].FileInfo.PictHeight
  };
  bool bResChange = (tInputDim.iWidth != AL_GetSrcWidth(cfg.Settings.tChParam[iLayerID])) || (tInputDim.iHeight != AL_GetSrcHeight(cfg.Settings.tChParam[iLayerID]));

  if(bResChange)
  {
    /* No resize with dynamic resolution changes */
    cfg.Settings.tChParam[iLayerID].uEncWidth = cfg.Settings.tChParam[iLayerID].uSrcWidth = tInputDim.iWidth;
    cfg.Settings.tChParam[iLayerID].uEncHeight = cfg.Settings.tChParam[iLayerID].uSrcHeight = tInputDim.iHeight;
  }

  frameReader = InitializeFrameReader(cfg, YuvFile,
                                      layerInputs[iInputIdx].YUVFileName,
                                      MapFile,
                                      cfg.MainInput.sMapFileName,
                                      layerInputs[iInputIdx].FileInfo);

}

void LayerResources::ChangeEncoderInput(ConfigFile& cfg, int32_t iInputIdx)
{
  this->iInputIdx = iInputIdx;
  OpenEncoderInput(cfg);
}

bool LayerResources::SendInput(ConfigFile& cfg, IEncoderSink* pEncoderSink, void* pTraceHook)
{
  (void)pTraceHook;
  pEncoderSink->PreprocessFrame();

  return sendInputFileTo(frameReader, SrcBufPool, SrcYuv.get(), cfg, layerInputs[iInputIdx].FileInfo, pSrcConv.get(), pEncoderSink, iPictCount, iReadCount);
}

bool LayerResources::sendInputFileTo(unique_ptr<FrameReader>& frameReader, PixMapBufPool& SrcBufPool, AL_TBuffer* Yuv, ConfigFile const& cfg, AL_TYUVFileInfo& FileInfo, IConvSrc* pSrcConv, IEncoderSink* pEncoderSink, AL_64S& iPictCount, AL_64S& iReadCount)
{
  if(AL_IS_ERROR_CODE(pEncoderSink->GetLastError()))
  {
    pEncoderSink->ProcessFrame(nullptr);
    return false;
  }

  shared_ptr<AL_TBuffer> frame = GetSrcFrame(iReadCount, iPictCount, frameReader, FileInfo, SrcBufPool, Yuv, cfg.Settings.tChParam[0], cfg, pSrcConv);

  pEncoderSink->ProcessFrame(frame.get());

  if(!frame)
    return false;

  iPictCount++;
  return true;
}

unique_ptr<FrameReader> LayerResources::InitializeFrameReader(ConfigFile& cfg, ifstream& YuvFile, string sYuvFileName, ifstream& MapFile, string sMapFileName, AL_TYUVFileInfo& FileInfo)
{
  (void)(MapFile);

  unique_ptr<FrameReader> pFrameReader;
  bool bUseCompressedFormat = AL_IsCompressed(FileInfo.FourCC);
  bool bHasCompressionMapFile = !sMapFileName.empty();

  if(bUseCompressedFormat != bHasCompressionMapFile)
    throw runtime_error(std::string("Providing a map file is ") + std::string(bUseCompressedFormat ? "mandatory" : "forbidden") +
                        " when using " + std::string(bUseCompressedFormat ? "compressed" : "uncompressed") + " input.");

  YuvFile.close();
  OpenInput(YuvFile, sYuvFileName);

  if(!bUseCompressedFormat)
    pFrameReader = unique_ptr<FrameReader>(new UnCompFrameReader(YuvFile, FileInfo, cfg.RunInfo.bLoop));
  pFrameReader->SeekAbsolute(cfg.RunInfo.iFirstPict + iReadCount);

  return pFrameReader;
}

void SafeChannelMain(ConfigFile& cfg, CIpDevice* pIpDevice, CIpDeviceParam& param, int32_t chanId)
{
  (void)param;
  auto& Settings = cfg.Settings;
  auto& StreamFileName = cfg.BitstreamFileName;
  auto& RunInfo = cfg.RunInfo;
  vector<LayerResources> layerResources(cfg.Settings.NumLayer);

  /* null if not supported */
  void* pTraceHook {};
  unique_ptr<EncoderSink> enc;
  unique_ptr<EncoderLookAheadSink> encFirstPassLA;

  auto pAllocator = pIpDevice->GetAllocator();
  auto pScheduler = pIpDevice->GetScheduler();

  AL_EVENT hFinished = Rtos_CreateEvent(false);
  RCPlugin_Init(&cfg.Settings, &cfg.Settings.tChParam[0], pAllocator);

  auto OnScopeExit = scopeExit([&]() {
    Rtos_DeleteEvent(hFinished);
    AL_Allocator_Free(pAllocator, cfg.Settings.hRcPluginDmaContext);
  });

  // --------------------------------------------------------------------------------
  // Create Encoder

  enc.reset(new EncoderSink(cfg, pScheduler, pAllocator));

  IEncoderSink* pFirstEncoderSink = enc.get();

  if(AL_TwoPassMngr_HasLookAhead(cfg.Settings))
  {
    encFirstPassLA.reset(new EncoderLookAheadSink(pFirstEncoderSink, cfg, pScheduler, pAllocator));

    pFirstEncoderSink = encFirstPassLA.get();
  }

  // --------------------------------------------------------------------------------
  // Allocate/Push Layers resources
  AL_TEncoderInfo tEncInfo;
  AL_Encoder_GetInfo(enc->hEnc, &tEncInfo);

  for(size_t i = 0; i < layerResources.size(); i++)
  {
    auto multisinkRec = unique_ptr<MultiSink>(new MultiSink);
    layerResources[i].Init(cfg, tEncInfo, i, pIpDevice, chanId);
    layerResources[i].PushResources(cfg, enc.get()
                                    ,
                                    encFirstPassLA.get()
                                    );

    // Rec file creation
    string LayerRecFileName = cfg.RecFileName;

    if(!LayerRecFileName.empty())
    {
      {
        std::unique_ptr<IFrameSink> recOutput(createUnCompFrameSink(LayerRecFileName, AL_FB_RASTER));
        multisinkRec->addSink(recOutput);
      }
    }
    enc->RecOutput[i] = std::move(multisinkRec);
  }

  auto multisink = unique_ptr<MultiSink>(new MultiSink);

  if(!cfg.bDisableBitstreamOutput)
  {
    std::unique_ptr<IFrameSink> bitstreamOutput(createBitstreamWriter(StreamFileName, cfg));
    multisink->addSink(bitstreamOutput);
  }

  if(!RunInfo.sStreamMd5Path.empty())
  {
    std::unique_ptr<IFrameSink> md5Calculator(createStreamMd5Calculator(RunInfo.sStreamMd5Path));
    multisink->addSink(md5Calculator);
  }

  if(!RunInfo.bitrateFile.empty())
  {
    std::unique_ptr<IFrameSink> bitrateOutput(createBitrateWriter(RunInfo.bitrateFile, cfg));
    multisink->addSink(bitrateOutput);
  }

  if(RunInfo.rateCtrlStat != AL_RATECTRL_STAT_MODE_NONE && !RunInfo.rateCtrlMetaPath.empty())
  {
    std::unique_ptr<IFrameSink> rateCtrlMetaSink(createRateCtrlMetaSink(RunInfo.rateCtrlMetaPath, AL_GET_CODEC(cfg.Settings.tChParam[0].eProfile)));
    multisink->addSink(rateCtrlMetaSink);
  }

  enc->BitstreamOutput[0] = std::move(multisink);

  // --------------------------------------------------------------------------------
  // Set Callbacks

  enc->m_done = ([&]() {
    Rtos_SetEvent(hFinished);
  });

  if(!RunInfo.sRecMd5Path.empty())
  {
    for(int32_t iLayerID = 0; iLayerID < Settings.NumLayer; ++iLayerID)
    {
      auto layer_multisink = unique_ptr<MultiSink>(new MultiSink);
      layer_multisink->addSink(enc->RecOutput[iLayerID]);
      string LayerMd5FileName = RunInfo.sRecMd5Path;
      std::unique_ptr<IFrameSink> md5Calculator(createYuvMd5Calculator(LayerMd5FileName, cfg));
      layer_multisink->addSink(md5Calculator);
      enc->RecOutput[iLayerID] = std::move(layer_multisink);
    }
  }

  unique_ptr<RepeaterSink> prefetch;

  if(g_numFrameToRepeat > 0)
  {
    prefetch.reset(new RepeaterSink(pFirstEncoderSink, g_numFrameToRepeat, RunInfo.iMaxPict));
    pFirstEncoderSink = prefetch.get();
    RunInfo.iMaxPict = g_numFrameToRepeat;
  }

  pFirstEncoderSink->SetChangeSourceCallback(
    [&](int32_t iInputIdx, int32_t iLayerID) {
    return layerResources[iLayerID].ChangeEncoderInput(cfg, iInputIdx);
  });

  bool hasInputAndNoError = true;

  for(int32_t i = 0; i < Settings.NumLayer; ++i)
    layerResources[i].OpenEncoderInput(cfg);

  while(hasInputAndNoError)
  {
    AL_64U uBeforeTime = Rtos_GetTime();

    for(int32_t i = 0; i < Settings.NumLayer; ++i)
      hasInputAndNoError = layerResources[i].SendInput(cfg, pFirstEncoderSink, pTraceHook) && hasInputAndNoError;

    AL_64U uAfterTime = Rtos_GetTime();

    if((uAfterTime - uBeforeTime) < RunInfo.uInputSleepInMilliseconds)
      Rtos_Sleep(RunInfo.uInputSleepInMilliseconds - (uAfterTime - uBeforeTime));
  }

  Rtos_WaitEvent(hFinished, AL_WAIT_FOREVER);

  if(auto err = enc->GetLastError())
    throw codec_error(AL_Codec_ErrorToString(err), err);

}

/*****************************************************************************/
static CIpDeviceParam BuildIPDeviceParams(ConfigFile& cfg)
{
  auto& RunInfo = cfg.RunInfo;

  CIpDeviceParam param;
  param.eSchedulerType = RunInfo.eSchedulerType;
  param.eDeviceType = RunInfo.eDeviceType;

  param.pCfgFile = &cfg;
  param.eTrackDmaMode = RunInfo.eTrackDmaMode;

  return param;
}

static shared_ptr<CIpDevice> BuildIPDevice(CIpDeviceParam param, ConfigFile* pCfgChannels, uint32_t const uNumChannels)
{
  (void)pCfgChannels;
  (void)uNumChannels;

  auto pIPDevice = shared_ptr<CIpDevice>(new CIpDevice);

  if(!pIPDevice)
    throw runtime_error("Can't create IpDevice");

  pIPDevice->Configure(param);

  return pIPDevice;
}

/*****************************************************************************/
struct channel_runtime_error : public runtime_error
{
  explicit channel_runtime_error() : runtime_error("")
  {
  }
};

static void ChannelMain(ConfigFile& cfg, CIpDevice* pIpDevice, CIpDeviceParam& param, exception_ptr& exception, int32_t chanId)
{
  try
  {
    SafeChannelMain(cfg, pIpDevice, param, chanId);
    exception = nullptr;
    return;
  }
  catch(codec_error const& error)
  {
    cerr << endl << "Codec error: " << error.what() << endl;
    exception = current_exception();
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    exception = make_exception_ptr(channel_runtime_error());
  }
}

static int32_t constexpr MAX_CHANNELS = 64;

int32_t GetChannelsArgv(vector<char*>* argvChannels, int32_t argc, char* argv[])
{
  int32_t curChan = 0;

  for(int32_t i = 0; i < argc; ++i)
  {
    if(string(argv[i]) == "--next-chan")
    {
      ++curChan;

      if(curChan > MAX_CHANNELS)
        throw runtime_error("Too many channels");

      argvChannels[curChan].push_back(argv[0]);
      continue;
    }

    argvChannels[curChan].push_back(argv[i]);
  }

  return curChan;
}

/*****************************************************************************/
void SafeMain(int32_t argc, char* argv[])
{
  InitializePlateform();

  vector<char*> argvChannels[MAX_CHANNELS] {};

  int32_t const maxChan = GetChannelsArgv(argvChannels, argc, argv);
  auto const numChan = maxChan + 1;

  if(numChan > 1)
    cout << "channel number: " << numChan << endl;

  ConfigFile cfgChannels[MAX_CHANNELS] {};
  CfgParser cfgParserChannels[MAX_CHANNELS] {};
  exception_ptr errorChannels[MAX_CHANNELS] {};
  thread worker[MAX_CHANNELS];

  for(int32_t chan = 0; chan < numChan; ++chan)
  {
    ConfigFile& cfg = cfgChannels[chan];
    CfgParser& cfgParser = cfgParserChannels[chan];

    SetDefaults(cfg);
    ParseCommandLine(argvChannels[chan].size(), argvChannels[chan].data(), cfg, cfgParser);
    AdjustSettingsAfterParsing(cfg);

    if(g_helpCfg)
    {
      cfgParser.PrintConfigFileUsage(cfg);
      return;
    }

    if(g_helpCfgJson)
    {
      cfgParser.PrintConfigFileUsageJson(cfg);
      return;
    }

    DisplayVersionInfo();

    ValidateConfig(cfg);

    if(g_showCfg)
      cfgParser.PrintConfig(cfg);

  }

  AL_ELibEncoderArch eArch = AL_LIB_ENCODER_ARCH_HOST;

  auto& cfgFirstChan = cfgChannels[0];

  if(!AL_IS_SUCCESS_CODE(AL_Lib_Encoder_Init(eArch)))
    throw runtime_error("Can't setup encode library");

  auto param = BuildIPDeviceParams(cfgFirstChan);
  auto pIpDevice = BuildIPDevice(param, &cfgChannels[0], numChan);

  if(numChan > 1)
  {
    for(int32_t chan = 0; chan < numChan; ++chan)
    {
      cout << "[main] Launching channel " << chan << endl;
      worker[chan] = thread(&ChannelMain, ref(cfgChannels[chan]), pIpDevice.get(), ref(param), ref(errorChannels[chan]), chan);
    }

    for(int32_t chan = 0; chan < numChan; ++chan)
    {
      cout << "[main] Waiting for channel " << chan << endl;
      worker[chan].join();
      cout << "[main] channel " << chan << " ended" << endl;
    }

    for(int32_t chan = 0; chan < numChan; ++chan)
    {
      cout << "[main] Looking for errors in channel " << chan << endl;

      if(errorChannels[chan])
        rethrow_exception(errorChannels[chan]);
    }

    return;
  }

  exception_ptr pError;
  ChannelMain(cfgFirstChan, pIpDevice.get(), param, pError, 0);

  if(pError)
    rethrow_exception(pError);

  AL_Lib_Encoder_DeInit();
}

/******************************************************************************/
int main(int argc, char* argv[])
{
  try
  {
    SafeMain(argc, argv);
    return 0;
  }
  catch(codec_error const& error)
  {
    return error.GetCode();
  }
  catch(channel_runtime_error const &)
  {
    return 1;
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    return 1;
  }
}
