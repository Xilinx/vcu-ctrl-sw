// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <stdexcept>
#include <string>

#include "CmdParser.hpp"
#include "exe_decoder/CodecUtils.hpp"
#include "lib_app/CommandLineParser.hpp"
#include "lib_app/CommonCmdParser.hpp"

extern "C"
{
#include "lib_common/AllocatorTracker.h"
#include "lib_common/BufCommon.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Round.h"
#include "lib_rtos/assert.h"
#include "lib_rtos/utils.h"
#include "lib_common/AvcLevels.h"
#include "lib_common/HevcLevels.h"
}

using namespace std;

/******************************************************************************/
static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cout << "Usage: " << ExeName << " -in <bitstream_file> -out <yuv_file> [options]" << endl;
  cout << "Options:" << endl;

  opt.usage();

  cout << endl << "Examples:" << endl;
  cout << "  " << ExeName << " -avc  -in bitstream.264 -out decoded.yuv -bd 8 " << endl;
  cout << "  " << ExeName << " -hevc -in bitstream.265 -out decoded.yuv -bd 10" << endl;
  cout << endl;
}

/******************************************************************************/
static AL_TDecSettings GetDefaultDecSettings(void)
{
  AL_TDecSettings settings {};
  AL_DecSettings_SetDefaults(&settings);
  return settings;
}

/******************************************************************************/
Config::Config(void)
{
  tDecSettings = GetDefaultDecSettings();

}

/******************************************************************************/
static EDecErrorLevel ParseExitOn(const string& toParse)
{
  string toParseLower = toParse;
  for_each(toParseLower.begin(), toParseLower.end(), [](char& c) { c = ::tolower(c); });

  if(toParseLower == "w" || toParseLower == "warning")
    return DEC_WARNING;
  else if(toParseLower == "e" || toParseLower == "error")
    return DEC_ERROR;
  else
    throw runtime_error("wrong exit condition");
}

/******************************************************************************/
static TFourCC ParseFourCCFormat(const string& sOutputFormat)
{
  uint32_t uFourCC = 0;

  if(sOutputFormat.size() >= 1)
    uFourCC = ((uint32_t)sOutputFormat[0]);

  if(sOutputFormat.size() >= 2)
    uFourCC |= ((uint32_t)sOutputFormat[1]) << 8;

  if(sOutputFormat.size() >= 3)
    uFourCC |= ((uint32_t)sOutputFormat[2]) << 16;

  if(sOutputFormat.size() >= 4)
    uFourCC |= ((uint32_t)sOutputFormat[3]) << 24;

  return (TFourCC)uFourCC;
}

/******************************************************************************/
static int32_t ParseOutputBD(string& sOutputBitDepth)
{
  if(sOutputBitDepth == string("first"))
    return OUTPUT_BD_FIRST;
  else if(sOutputBitDepth == string("alloc"))
    return OUTPUT_BD_ALLOC;
  else if(sOutputBitDepth == string("stream"))
    return OUTPUT_BD_STREAM;
  else
  {
    int32_t iBitDepth;
    stringstream ss(sOutputBitDepth);
    ss >> iBitDepth;

    bool bIsBitDepthValid = (iBitDepth == 8) || (iBitDepth == 10) || (iBitDepth == 12);

    if(ss.fail() || !bIsBitDepthValid)
      throw runtime_error("Unsupported output bitdepth");
    return iBitDepth;
  }
}

/******************************************************************************/
static AL_EProfile ParseProfile(string const& sProf)
{
  static const map<string, AL_EProfile> PROFILES =
  {
    { "HEVC_MONO10", AL_PROFILE_HEVC_MONO10 },
    { "HEVC_MONO", AL_PROFILE_HEVC_MONO },
    { "HEVC_MAIN_444_STILL", AL_PROFILE_HEVC_MAIN_444_STILL },
    { "HEVC_MAIN_444_10_INTRA", AL_PROFILE_HEVC_MAIN_444_10_INTRA },
    { "HEVC_MAIN_444_INTRA", AL_PROFILE_HEVC_MAIN_444_INTRA },
    { "HEVC_MAIN_444_10", AL_PROFILE_HEVC_MAIN_444_10 },
    { "HEVC_MAIN_444", AL_PROFILE_HEVC_MAIN_444 },
    { "HEVC_MAIN_422_10_INTRA", AL_PROFILE_HEVC_MAIN_422_10_INTRA },
    { "HEVC_MAIN_422_10", AL_PROFILE_HEVC_MAIN_422_10 },
    { "HEVC_MAIN_422_12", AL_PROFILE_HEVC_MAIN_422_12 },
    { "HEVC_MAIN_444_12", AL_PROFILE_HEVC_MAIN_444_12 },
    { "HEVC_MAIN_422", AL_PROFILE_HEVC_MAIN_422 },
    { "HEVC_MAIN_INTRA", AL_PROFILE_HEVC_MAIN_INTRA },
    { "HEVC_MAIN_STILL", AL_PROFILE_HEVC_MAIN_STILL },
    { "HEVC_MAIN10_INTRA", AL_PROFILE_HEVC_MAIN10_INTRA },
    { "HEVC_MAIN10", AL_PROFILE_HEVC_MAIN10 },
    { "HEVC_MAIN12", AL_PROFILE_HEVC_MAIN12 },
    { "HEVC_MAIN", AL_PROFILE_HEVC_MAIN },
    /* Baseline is mapped to Constrained_Baseline */
    { "AVC_BASELINE", AL_PROFILE_AVC_C_BASELINE },
    { "AVC_C_BASELINE", AL_PROFILE_AVC_C_BASELINE },
    { "AVC_MAIN", AL_PROFILE_AVC_MAIN },
    { "AVC_HIGH10_INTRA", AL_PROFILE_AVC_HIGH10_INTRA },
    { "AVC_HIGH10", AL_PROFILE_AVC_HIGH10 },
    { "AVC_HIGH_422_INTRA", AL_PROFILE_AVC_HIGH_422_INTRA },
    { "AVC_HIGH_422", AL_PROFILE_AVC_HIGH_422 },
    { "AVC_HIGH_INTRA", AL_PROFILE_AVC_HIGH_INTRA },
    { "AVC_HIGH", AL_PROFILE_AVC_HIGH },
    { "AVC_C_HIGH", AL_PROFILE_AVC_C_HIGH },
    { "AVC_PROG_HIGH", AL_PROFILE_AVC_PROG_HIGH },
    { "AVC_CAVLC_444_INTRA", AL_PROFILE_AVC_CAVLC_444_INTRA },
    { "AVC_HIGH_444_INTRA", AL_PROFILE_AVC_HIGH_444_INTRA },
    { "AVC_HIGH_444_PRED", AL_PROFILE_AVC_HIGH_444_PRED },
    { "XAVC_HIGH10_INTRA_CBG", AL_PROFILE_XAVC_HIGH10_INTRA_CBG },
    { "XAVC_HIGH10_INTRA_VBR", AL_PROFILE_XAVC_HIGH10_INTRA_VBR },
    { "XAVC_HIGH_422_INTRA_CBG", AL_PROFILE_XAVC_HIGH_422_INTRA_CBG },
    { "XAVC_HIGH_422_INTRA_VBR", AL_PROFILE_XAVC_HIGH_422_INTRA_VBR },
    { "XAVC_LONG_GOP_MAIN_MP4", AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4 },
    { "XAVC_LONG_GOP_HIGH_MP4", AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4 },
    { "XAVC_LONG_GOP_HIGH_MXF", AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF },
    { "XAVC_LONG_GOP_HIGH_422_MXF", AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF },
  };

  map<string, AL_EProfile>::const_iterator it = PROFILES.find(sProf);

  if(it == PROFILES.end())
    return AL_PROFILE_UNKNOWN;

  return it->second;
}

/******************************************************************************/
static void GetExpectedSeparator(stringstream& ss, char expectedSep)
{
  char sep;
  ss >> sep;

  if(sep != expectedSep)
    throw runtime_error("wrong prealloc arguments separator");
}

/******************************************************************************/
static void ParsePreAllocArgs(AL_TStreamSettings* settings, AL_ECodec codec, string const& toParse)
{
  stringstream ss(toParse);
  ss.unsetf(ios::oct);
  ss.unsetf(ios::hex);
  ss >> settings->tDim.iWidth;
  GetExpectedSeparator(ss, 'x');
  ss >> settings->tDim.iHeight;
  GetExpectedSeparator(ss, ':');
  char vm[6] {};
  ss >> vm[0];
  ss >> vm[1];
  ss >> vm[2];
  ss >> vm[3];
  ss >> vm[4];
  GetExpectedSeparator(ss, ':');
  char chroma[4] {};
  ss >> chroma[0];
  ss >> chroma[1];
  ss >> chroma[2];
  GetExpectedSeparator(ss, ':');
  ss >> settings->iBitDepth;
  GetExpectedSeparator(ss, ':');

  if(ss.peek() >= '0' && ss.peek() <= '9')
  {
    int32_t iProfileIdc;
    ss >> iProfileIdc;
    settings->eProfile = AL_MAKE_PROFILE(codec, iProfileIdc, 0);
  }
  else
  {
    string const& sArgs = ss.str();
    string const& sProf = sArgs.substr(ss.tellg(), sArgs.find_first_of(':', ss.tellg()) - ss.tellg());
    settings->eProfile = ParseProfile(sProf);

    if(AL_GET_CODEC(settings->eProfile) != codec)
      throw runtime_error("The profile does not match the codec");

    ss.ignore(sProf.length());
  }

  GetExpectedSeparator(ss, ':');
  ss >> settings->iLevel;

  settings->iMaxRef = 0;

  if(ss.tellg() != streampos(-1))
  {
    GetExpectedSeparator(ss, ':');
    ss >> settings->iMaxRef;
  }

  bool bIsBitDepthValid = (settings->iBitDepth == 8) || (settings->iBitDepth == 10) || (settings->iBitDepth == 12);

  if(!bIsBitDepthValid)
    throw runtime_error("Unsupported bitdepth");

  if(codec == AL_CODEC_AVC)
  {
    if(!AL_AVC_IsLevel(settings->iLevel))
      throw runtime_error("The level does not match the codec");
  }

  if(codec == AL_CODEC_HEVC)
  {
    if(!AL_HEVC_IsLevel(settings->iLevel))
      throw runtime_error("The level does not match the codec");
  }

  if(string(chroma) == "400")
    settings->eChroma = AL_CHROMA_4_0_0;
  else if(string(chroma) == "420")
    settings->eChroma = AL_CHROMA_4_2_0;
  else if(string(chroma) == "422")
    settings->eChroma = AL_CHROMA_4_2_2;
  else if(string(chroma) == "444")
    settings->eChroma = AL_CHROMA_4_4_4;
  else
    throw runtime_error("wrong prealloc chroma format");

  if(string(vm) == "unkwn")
    settings->eSequenceMode = AL_SM_UNKNOWN;
  else if(string(vm) == "progr")
    settings->eSequenceMode = AL_SM_PROGRESSIVE;
  else if(string(vm) == "inter")
    settings->eSequenceMode = AL_SM_INTERLACED;
  else
    throw runtime_error("wrong prealloc video format");

  if((ss.tellg() != streampos(-1)))
    throw runtime_error("wrong prealloc arguments format");
}

/******************************************************************************/
static bool IsPrimaryOutputFormatAllowed(AL_EFbStorageMode mode, AL_ECodec codec)
{
  (void)codec;
  bool bAllowed = AL_FB_RASTER == mode || AL_FB_TILE_32x4 == mode || AL_FB_TILE_64x4 == mode;

  return bAllowed;
}

/******************************************************************************/
static void ProcessOutputArgs(Config& config, const string& sRasterOut)
{
  (void)sRasterOut;

  if(!IsPrimaryOutputFormatAllowed(config.tDecSettings.eFBStorageMode, config.tDecSettings.eCodec))
    throw runtime_error("Primary output format is not allowed !");

  if(!config.bEnableYUVOutput)
  {
    config.sMainOut = "";
  }
  else if(config.sMainOut.empty())
    config.sMainOut = "dec.yuv";
}

/******************************************************************************/
template<int32_t Offset>
static int32_t IntWithOffset(const string& word)
{
  try
  {
    return stoi(word.c_str()) + Offset;
  }
  catch(const exception &)
  {
    throw runtime_error("Error when converting string to int: unable to parse the word '" + word + "'");
  }
}

/******************************************************************************/
template<typename TCouple, char Separator>
static TCouple CoupleWithSeparator(const string& str)
{
  TCouple Couple;
  struct t_couple
  {
    uint32_t first;
    uint32_t second;
  }* pCouple = reinterpret_cast<t_couple*>(&Couple);

  Rtos_StaticAssert(sizeof(TCouple) == sizeof(t_couple), "Invalid structure size");

  size_t sep = str.find_first_of(Separator);

  try
  {
    pCouple->first = stoi(str.substr(0, sep).c_str());
    pCouple->second = stoi(str.substr(sep + 1).c_str());
  }
  catch(const exception &)
  {
    throw runtime_error("Error when converting string to couple of int: unable to parse the word '" + str + "'");
  }

  return Couple;
}

/******************************************************************************/
static string toStringPathsSet(set<string> paths)
{
  string out;

  for(auto path : paths)
  {
    if(out.length() != 0)
      out += string(", ");
    out += path;
  }

  return out;
}

/******************************************************************************/
DISABLE_VAR_TRACKING_ASSIGNMENT
Config ParseCommandLine(int32_t argc, char* argv[])
{
  Config config {};

  int32_t fps = 0;
  bool version = false;
  bool helpJson = false;

  string sRasterOut;
  string sOutputBitDepth;
  string sOutputFormat;
  set<string> const sDecDefaultDevicePath(DECODER_DEVICES);

  SetDefaultDecOutputSettings(&config.tUserOutputSettings);

  auto opt = CommandLineParser();

  opt.addFlag("--help,-h", &config.help, "Shows this help");
  opt.addFlag("--help-json", &helpJson, "Show this help (json)");
  opt.addFlag("--version", &version, "Show version");

  opt.addString("--input,--in,-in,--i,-i", &config.sIn, "Input bitstream");
  opt.addString("--output,--out,-out,--o,-o", &config.sMainOut, "Output YUV");
  opt.addFlag("--avc,-avc", &config.tDecSettings.eCodec,
              string("Specify the input bitstream codec (default: ") + string(AL_CodecToString(AL_DEC_DEFAULT_CODEC)) + string(")"),
              AL_CODEC_AVC);

  opt.addFlag("--hevc,-hevc", &config.tDecSettings.eCodec,
              string("Specify the input bitstream codec (default: ") + string(AL_CodecToString(AL_DEC_DEFAULT_CODEC)) + string(")"),
              AL_CODEC_HEVC);
  opt.addOption("--heic,-heic", [&](string)
  {
    config.tDecSettings.eCodec = AL_CODEC_HEVC;
    config.eContainer = EInputContainer::Heif;
  }, "Input is a HEIC still image");
  opt.addInt("--framerate,--fps,-fps", &fps, "force framerate");
  opt.addCustom("--clock,--clk,-clk", &config.tDecSettings.uClkRatio, &IntWithOffset<1000>, "Set clock ratio, (0 for 1000, 1 for 1001)", "number");
  opt.addString("--bitdepth,--bd,-bd", &sOutputBitDepth, "Output YUV bitdepth (8, 10, 12, alloc : force prealloc if present, if not fallback to first, stream: use current frame bitdepth, first: always use bitdepth of the first decoded frame)");
  opt.addString("--output-format", &sOutputFormat, "Output format FourCC (default: auto)");
  opt.addFlag("--sync-i-frames", &config.tDecSettings.bUseIFramesAsSyncPoint,
              "Allow decoder to sync on I frames if configurations' nals are presents",
              true);

  opt.addFlag("--wavefront-parallel-processing,--wpp,-wpp", &config.tDecSettings.bParallelWPP, "Wavefront parallelization processing activation");
  opt.addFlag("--low-latency-decoding,--lowlat,-lowlat", &config.tDecSettings.bLowLat, "Low latency decoding activation");
  opt.addOption("--slice-latency,--slicelat,-slicelat", [&](string)
  {
    config.tDecSettings.eDecUnit = AL_VCL_NAL_UNIT;
    config.tDecSettings.eDpbMode = AL_DPB_NO_REORDERING;
  }, "Specify decoder latency (default: Frame Latency)");

  opt.addFlag("--frame-latency,--framelat,-framelat", &config.tDecSettings.eDecUnit,
              "Specify decoder latency (default: Frame Latency)",
              AL_AU_UNIT);

  opt.addFlag("--no-reordering", &config.tDecSettings.eDpbMode,
              "Indicates to decoder that the stream doesn't contain B-frame & reference must be at best 1",
              AL_DPB_NO_REORDERING);

  opt.addOption("--fbuf-format,--ff,-ff", [&](string)
  {
    string sWordToParse = opt.popWord();
    config.tDecSettings.eFBStorageMode = ParseFrameBufferFormat(sWordToParse, config.tDecSettings.bFrameBufferCompression);

  }, "Specify the format of the decoded frame buffers (" + GetFrameBufferFormatOptDesc() + ")");

  opt.addFlag("--split-input", &config.tDecSettings.eInputMode,
              "Send stream by decoding unit",
              AL_DEC_SPLIT_INPUT);
  opt.addString("--split-from-sizes", &config.sSplitSizesFile, "Split stream according to frame sizes specified in file. One frame size by line. Requires preallocation.");
  opt.addString("--sei-file", &config.seiFile, "File in which the SEI decoded by the decoder will be dumped");

  opt.addString("--hdr-file", &config.hdrFile, "Parse and dump HDR data in the specified file");

  string preAllocArgs = "";
  opt.addString("--prealloc-args", &preAllocArgs, "Specify stream's parameters: 'widthxheight:video-mode:chroma-mode:bit-depth:profile:level' for example '1920x1080:progr:422:10:HEVC_MAIN:5'. video-mode values are: unkwn, progr or inter. Be careful cast is important.");
  opt.addCustom("--output-position", &config.tDecSettings.tOutputPosition, &CoupleWithSeparator<AL_TPosition, ','>, "Specify the position of the decoded frame in frame buffer");

  opt.addFlag("--decode-intraonly", &config.tDecSettings.tStream.bDecodeIntraOnly, "Decode Only I Frames");

  opt.startSection("Run");

  opt.addInt("--max-frames", &config.iMaxFrames, "Abort after max number of decoded frames (approximative abort)");
  opt.addInt("--loop,-loop", &config.iLoop, "Number of Decoding loop (optional)");
  opt.addInt("--timeout", &config.iTimeoutInSeconds, "Specify timeout in seconds");

  bool dummyNextChan; // As the --next-channel is parsed elsewhere, this option is only used to add the description in the usage
  opt.addFlag("--next-chan", &dummyNextChan, "Start the configuration of a new decoding channel. The options that are applied on all channels must be specified in the first channel.");

  opt.addCustom("--exit-on", &config.eExitCondition, ParseExitOn, "Specify early exit condition (e/error, w/warning)");

  opt.startSection("Trace && Debug");

  opt.addFlag("--multi-chunk", &config.bMultiChunk, "Allocate luma and chroma of decoded frames on different memory chunks");
  opt.addInt("--input-buffer-count,--nbuf,-nbuf", &config.uInputBufferNum, "Specify the number of input feeder buffer");
  opt.addInt("--input-buffer-size,--nsize,-nsize", &config.zInputBufferSize, "Specify the size (in bytes) of input feeder buffer");
  opt.addInt("--circular-buffer-size,-stream-buf-size", &config.tDecSettings.iStreamBufSize, "Specify the size (in bytes) of internal circular buffer size (0 = default)");

  opt.addString("--crc_ip,-crc_ip", &config.sCrc, "Output crc file");

  opt.addOption("--first-frame-to-trace,--t,-t", [&](string)
  {
    config.iTraceIdx = opt.popInt();
    config.iTraceNumber = max(1, config.iTraceNumber);
  }, "First frame to trace (optional)", "number");

  opt.addOption("--frame-to-trace-count,--num,-num", [&](string)
  {
    config.iTraceNumber = opt.popInt();
    config.iTraceIdx = max(0, config.iTraceIdx);
  }, "Number of frames to trace", "number");
  opt.addFlag("--output-trace-only", &config.bOutputTraceOnly, "Only trace output buffers (non input nor sram map)");

  opt.addFlag("--use-early-callback", &config.tDecSettings.bUseEarlyCallback, "Low latency phase 2. Call end decoding at decoding launch. This only makes sense with special support for hardware synchronization. Requires --lowlat. ");
  opt.addInt("--core,-core", &config.tDecSettings.uNumCore, "Number of decoder cores");
  opt.addFlag("--force-multiple-dec1-per-frame", &config.tDecSettings.bForceMultipleDec1PerFrame, "Force using multiple dec1 per frame (when slice/tile pattern permits)");
  opt.addFlag("--non-realtime", &config.tDecSettings.bNonRealtime, "Specifies that the channel is a non-realtime channel");
  opt.addInt("--ddrwidth,-ddrwidth", &config.tDecSettings.uDDRWidth, "Width of DDR requests (16, 32, 64) (default: 32)");
  opt.addFlag("--nocache,-nocache", &config.tDecSettings.bDisableCache, "Inactivate the cache");

  opt.addOption("--device", [&](string) {
    config.sDecDevicePath.insert(opt.popWord());
  }, string(string("Path of the driver device(s) file(s) used to talk with the IP. Default(s) are: ") + toStringPathsSet(sDecDefaultDevicePath)));
  opt.addFlag("--select-device-with-lowest-available-resources", &config.bSelectDeviceWithLowestAvailableResources, "Select the device with the lowest available resources. This flag is not applicable without multiple devices. It should be use with caution!");

  opt.addFlag("--noyuv,-noyuv", &config.bEnableYUVOutput,
              "Disable writing output YUV file",
              false);

  opt.addString("--md5", &config.md5File, "Filename to the output MD5 of the YUV file");

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

  opt.startDeprecatedSection();

  opt.addFlag("--lowref,-lowref", &config.tDecSettings.eDpbMode,
              "Use --no-reordering instead",
              AL_DPB_NO_REORDERING);

  opt.addUint("--conceal-max-fps", &config.tDecSettings.uConcealMaxFps, "Maximum fps to conceal invalid or corrupted stream header; 0 = no concealment");

  bool bHasDeprecated = opt.parse(argc, argv);

  if(config.help)
  {
    Usage(opt, argv[0]);
    return config;
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

  ProcessOutputArgs(config, sRasterOut);

  bool bMainOutputCompression = IsOutputStorageModeCompressed(config.tUserOutputSettings, config.tDecSettings.bFrameBufferCompression);

  if(bMainOutputCompression && config.bCertCRC)
    throw runtime_error("Certification CRC unavailable with fbc");

  if(!config.sDecDevicePath.empty() && config.bSelectDeviceWithLowestAvailableResources)
  {
    throw runtime_error("Cannot use both --device and --select-device-with-lowest-available-resources flags together");
  }

  if(config.sDecDevicePath.empty())
    config.sDecDevicePath = sDecDefaultDevicePath;

  if(fps > 0)
  {
    config.tDecSettings.uFrameRate = fps * 1000;
    config.tDecSettings.bForceFrameRate = true;
  }

  if(!config.sSplitSizesFile.empty())
    config.tDecSettings.eInputMode = AL_DEC_SPLIT_INPUT;

  if(!sOutputBitDepth.empty()
     )
  {
    config.iOutputBitDepth = ParseOutputBD(sOutputBitDepth);
  }

  if(!sOutputFormat.empty())
    config.tOutputFourCC = ParseFourCCFormat(sOutputFormat);

  {
    if(!preAllocArgs.empty())
    {
      ParsePreAllocArgs(&config.tDecSettings.tStream, config.tDecSettings.eCodec, preAllocArgs);

      /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
      /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
      /* And because we don't know the codec here, always use 64 as MB/LCU size. */
      int32_t iAlignValue = 8;

      if(config.tDecSettings.eCodec == AL_CODEC_AVC)
        iAlignValue = 16;

      config.tDecSettings.tStream.tDim.iWidth = AL_RoundUp(config.tDecSettings.tStream.tDim.iWidth, iAlignValue);
      config.tDecSettings.tStream.tDim.iHeight = AL_RoundUp(config.tDecSettings.tStream.tDim.iHeight, iAlignValue);

      config.bUsePreAlloc = true;
    }

    if(config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT && !config.bUsePreAlloc)
      throw runtime_error(" --split-input requires preallocation");

    if((config.tDecSettings.tOutputPosition.iX || config.tDecSettings.tOutputPosition.iY) && !config.bUsePreAlloc)
      throw runtime_error(" --output-position requires preallocation");
  }

  if(config.sIn.empty())
    throw runtime_error("No input file specified (use -h to get help)");

  if(config.eContainer != EInputContainer::Elementary && config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT)
    throw runtime_error("--avif/--heic do not support --split-input");

  return config;
}

RESTORE_VAR_TRACKING_ASSIGNMENT

bool IsOutputStorageModeCompressed(AL_TDecOutputSettings tUserOutputSettings, bool bMainOutputCompressed)
{
  (void)tUserOutputSettings;

  bool bOutputCompression = bMainOutputCompressed;

  return bOutputCompression;
}

/******************************************************************************/
AL_EFbStorageMode GetMainOutputStorageMode(AL_TDecOutputSettings tUserOutputSettings, AL_EFbStorageMode eOutstorageMode)
{
  (void)tUserOutputSettings;
  AL_EFbStorageMode eOutputStorageMode = eOutstorageMode;

  return eOutputStorageMode;
}
