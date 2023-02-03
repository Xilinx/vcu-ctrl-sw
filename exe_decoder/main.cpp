/******************************************************************************
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

#include <cstdarg>
#include <cstdlib>
#include <climits>
#include <atomic>
#include <memory>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <mutex>
#include <map>
#include <thread>
#include <algorithm>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufCommon.h"
#include "lib_common/Error.h"
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferSeiMeta.h"
#include "lib_common_dec/HDRMeta.h"
}

#include "lib_app/BufPool.h"
#include "lib_app/PixMapBufPool.h"
#include "lib_app/console.h"
#include "lib_app/convert.h"
#include "lib_app/timing.h"
#include "lib_app/utils.h"
#include "lib_app/CommandLineParser.h"
#include "lib_app/plateform.h"
#include "lib_app/YuvIO.h"
#include "lib_app/MD5.h"

#include "Conversion.h"
#include "IpDevice.h"
#include "CodecUtils.h"
#include "crc.h"
#include "InputLoader.h"
#include "HDRWriter.h"

using namespace std;

struct codec_error : public runtime_error
{
  explicit codec_error(AL_ERR eErrCode) : runtime_error(AL_Codec_ErrorToString(eErrCode)), Code(eErrCode)
  {
  }

  const AL_ERR Code;
};

/* duplicated from Utils.h as we can't take these from inside the libraries */
static inline int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) / iRnd * iRnd;
}

static uint32_t constexpr uDefaultNumBuffersHeldByNextComponent = 1; /* We need at least 1 buffer to copy the output on a file */
static bool bCertCRC = false;
static bool g_MultiChunk = false;

AL_TDecSettings getDefaultDecSettings()
{
  AL_TDecSettings settings {};
  AL_DecSettings_SetDefaults(&settings);
  return settings;
}

static int const zDefaultInputBufferSize = 32 * 1024;
static const int OUTPUT_BD_FIRST = 0;
static const int OUTPUT_BD_ALLOC = -1;
static const int OUTPUT_BD_STREAM = -2;
static const int SEI_NOT_ASSOCIATED_WITH_FRAME = -1;

typedef enum
{
  DEC_WARNING,
  DEC_ERROR,
}EDecErrorLevel;

struct Config
{
  bool help = false;

  string sIn;
  string sMainOut = ""; // Output rec file
  string sCrc;

  AL_TDecSettings tDecSettings = getDefaultDecSettings();

  int iDeviceType = AL_DEVICE_TYPE_BOARD; // board
  AL_ESchedulerType iSchedulerType = AL_SCHEDULER_TYPE_MCU;
  int iOutputBitDepth = OUTPUT_BD_ALLOC;
  TFourCC tOutputFourCC = FOURCC(NULL);
  int iNumTrace = -1;
  int iNumberTrace = 0;
  bool bForceCleanBuffers = false;
  bool bEnableYUVOutput = true;
  unsigned int uInputBufferNum = 2;
  size_t zInputBufferSize = zDefaultInputBufferSize;
  AL_EIpCtrlMode ipCtrlMode = AL_IPCTRL_MODE_STANDARD;
  string logsFile = "";
  string md5File = "";
  string apbFile = "";
  bool trackDma = false;
  int hangers = 0;
  int iLoop = 1;
  int iTimeoutInSeconds = -1;
  int iMaxFrames = INT_MAX;
  string seiFile = "";
  string hdrFile = "";
  bool bUsePreAlloc = false;
  EDecErrorLevel eExitCondition = DEC_ERROR;
};

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

template<int Offset>
static int IntWithOffset(const string& word)
{
  return atoi(word.c_str()) + Offset;
}

template<typename TCouple, char Separator>
static TCouple CoupleWithSeparator(const string& str)
{
  TCouple Couple;
  struct t_couple
  {
    uint32_t first;
    uint32_t second;
  }* pCouple = reinterpret_cast<t_couple*>(&Couple);

  static_assert(sizeof(TCouple) == sizeof(t_couple), "Invalid structure size");

  size_t sep = str.find_first_of(Separator);
  pCouple->first = atoi(str.substr(0, sep).c_str());
  pCouple->second = atoi(str.substr(sep + 1).c_str());

  return Couple;
}

/******************************************************************************/
static AL_EFbStorageMode GetMainOutputStorageMode(const AL_TDecSettings& decSettings, bool& bOutputCompression, uint8_t uBitDepth)
{
  (void)uBitDepth;
  AL_EFbStorageMode eOutputStorageMode = decSettings.eFBStorageMode;
  bOutputCompression = decSettings.bFrameBufferCompression;

  return eOutputStorageMode;
}

/******************************************************************************/
static bool IsPrimaryOutputFormatAllowed(AL_EFbStorageMode mode)
{
  bool bAllowed = true;

  (void)mode;

  return bAllowed;
}

/******************************************************************************/
void processOutputArgs(Config& config, const string& sRasterOut)
{
  (void)sRasterOut;

  if(!IsPrimaryOutputFormatAllowed(config.tDecSettings.eFBStorageMode))
    throw runtime_error("Primary output format is not allowed !");

  if(!config.bEnableYUVOutput)
  {
    config.sMainOut = "";
  }
  else if(config.sMainOut.empty())
    config.sMainOut = "dec.yuv";
}

/******************************************************************************/
static AL_EFbStorageMode ParseFrameBufferFormat(const string& sBufFormat, bool& bBufComp)
{
  bBufComp = false;

  if(sBufFormat == "raster")
    return AL_FB_RASTER;

  throw runtime_error("Invalid buffer format");
}

/******************************************************************************/
static std::string GetFrameBufferFormatOptDesc(bool bSecondOutput = false)
{
  std::string sFBufFormatOptDesc = "raster";

  if(!bSecondOutput)
  {

  }

  return sFBufFormatOptDesc;
}

/******************************************************************************/
void parseOutputFormat(Config& config, const string& sOutputFormat)
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

  config.tOutputFourCC = (TFourCC)uFourCC;
}

/******************************************************************************/
void parseOutputBD(Config& config, string& sOutputBitDepth)
{
  if(sOutputBitDepth == string("first"))
    config.iOutputBitDepth = OUTPUT_BD_FIRST;
  else if(sOutputBitDepth == string("alloc"))
    config.iOutputBitDepth = OUTPUT_BD_ALLOC;
  else if(sOutputBitDepth == string("stream"))
    config.iOutputBitDepth = OUTPUT_BD_STREAM;
  else
  {
    stringstream ss(sOutputBitDepth);
    ss >> config.iOutputBitDepth;

    if(ss.fail())
      throw runtime_error("wrong output bitdepth");
  }
}

/******************************************************************************/
void getExpectedSeparator(stringstream& ss, char expectedSep)
{
  char sep;
  ss >> sep;

  if(sep != expectedSep)
    throw runtime_error("wrong prealloc arguments separator");
}

AL_EProfile parseProfile(string const& sProf)
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

void parsePreAllocArgs(AL_TStreamSettings* settings, AL_ECodec codec, string& toParse)
{
  stringstream ss(toParse);
  ss.unsetf(ios::dec);
  ss.unsetf(ios::hex);
  ss >> settings->tDim.iWidth;
  getExpectedSeparator(ss, 'x');
  ss >> settings->tDim.iHeight;
  getExpectedSeparator(ss, ':');
  char vm[6] {};
  ss >> vm[0];
  ss >> vm[1];
  ss >> vm[2];
  ss >> vm[3];
  ss >> vm[4];
  getExpectedSeparator(ss, ':');
  char chroma[4] {};
  ss >> chroma[0];
  ss >> chroma[1];
  ss >> chroma[2];
  getExpectedSeparator(ss, ':');
  ss >> settings->iBitDepth;
  getExpectedSeparator(ss, ':');

  if(ss.peek() >= '0' && ss.peek() <= '9')
  {
    int iProfileIdc;
    ss >> iProfileIdc;
    settings->eProfile = AL_MAKE_PROFILE(codec, iProfileIdc, 0);
  }
  else
  {
    string const& sArgs = ss.str();
    string const& sProf = sArgs.substr(ss.tellg(), sArgs.find_first_of(':', ss.tellg()) - ss.tellg());
    settings->eProfile = parseProfile(sProf);

    if(AL_GET_CODEC(settings->eProfile) != codec)
      throw runtime_error("The profile does not match the codec");

    ss.ignore(sProf.length());
  }

  getExpectedSeparator(ss, ':');
  ss >> settings->iLevel;
  switch(codec)
  {
  case AL_CODEC_AVC:
  case AL_CODEC_HEVC:

    if(settings->iLevel < 10 || settings->iLevel > 62)
      throw runtime_error("The level does not match the codec");
    break;
  case AL_CODEC_VVC:

    if(settings->iLevel < 10 || settings->iLevel > 63)
      throw runtime_error("The level does not match the codec");
    break;
  default:
    break;
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

  if(ss.fail() || (ss.tellg() != streampos(-1)))
    throw runtime_error("wrong prealloc arguments format");
}

static EDecErrorLevel parseExitOn(const string& toParse)
{
  string toParseLower = toParse;
  std::for_each(toParseLower.begin(), toParseLower.end(), [](char& c) { c = ::tolower(c); });

  if(toParseLower == "w" || toParseLower == "warning")
    return DEC_WARNING;
  else if(toParseLower == "e" || toParseLower == "error")
    return DEC_ERROR;
  else
    throw runtime_error("wrong exit condition");
}

/******************************************************************************/
std::string getHandledValuesList(const std::map<std::string, int> HandledValues)
{
  std::string sHandledValuesList;

  for(const auto& bdMode : HandledValues)
    sHandledValuesList += std::string("'") + bdMode.first + std::string("' ");

  return sHandledValuesList;
}

/******************************************************************************/
int parseStringToEnum(string s, const std::map<std::string, int> HandledValues, int defaultValue)
{
  if(s.empty())
  {
    return defaultValue;
  }

  auto chosenValue = HandledValues.find(s);

  if(chosenValue != HandledValues.end())
    return chosenValue->second;

  throw runtime_error(std::string("Wrong value. Allowed values are: ") + getHandledValuesList(HandledValues));
}

/******************************************************************************/
static Config ParseCommandLine(int argc, char* argv[])
{
  Config Config {};

  int fps = 0;
  bool version = false;
  bool helpJson = false;

  string sRasterOut;
  string sOutputBitDepth = "";
  string sOutputFormat = "";

  auto opt = CommandLineParser(ShouldShowAdvancedFeatures());

  opt.addFlag("--help,-h", &Config.help, "Shows this help");
  opt.addFlag("--help-json", &helpJson, "Show this help (json)");
  opt.addFlag("--version", &version, "Show version");

  opt.addString("-in,-i", &Config.sIn, "Input bitstream");
  opt.addString("-out,-o", &Config.sMainOut, "Output YUV");

  opt.addFlag("-avc", &Config.tDecSettings.eCodec,
              "Specify the input bitstream codec (default: HEVC)",
              AL_CODEC_AVC);

  opt.addFlag("-hevc", &Config.tDecSettings.eCodec,
              "Specify the input bitstream codec (default: HEVC)",
              AL_CODEC_HEVC);

  opt.addInt("-fps", &fps, "force framerate");
  opt.addCustom("-clk", &Config.tDecSettings.uClkRatio, &IntWithOffset<1000>, "Set clock ratio, (0 for 1000, 1 for 1001)", "number");
  opt.addString("-bd", &sOutputBitDepth, "Output YUV bitdepth (8, 10, 12, alloc (auto), stream, first)");
  opt.addString("--output-format", &sOutputFormat, "Output format FourCC (default: auto)");
  opt.addFlag("--sync-i-frames", &Config.tDecSettings.bUseIFramesAsSyncPoint,
              "Allow decoder to sync on I frames if configurations' nals are presents",
              true);

  opt.addFlag("-wpp", &Config.tDecSettings.bParallelWPP, "Wavefront parallelization processing activation");
  opt.addFlag("-lowlat", &Config.tDecSettings.bLowLat, "Low latency decoding activation");
  opt.addOption("-slicelat", [&](string)
  {
    Config.tDecSettings.eDecUnit = AL_VCL_NAL_UNIT;
    Config.tDecSettings.eDpbMode = AL_DPB_NO_REORDERING;
  }, "Specify decoder latency (default: Frame Latency)");

  opt.addFlag("-framelat", &Config.tDecSettings.eDecUnit,
              "Specify decoder latency (default: Frame Latency)",
              AL_AU_UNIT);

  opt.addFlag("--no-reordering", &Config.tDecSettings.eDpbMode,
              "Indicates to decoder that the stream doesn't contain B-frame & reference must be at best 1",
              AL_DPB_NO_REORDERING);

  opt.addOption("--fbuf-format,-ff", [&](string)
  {
    Config.tDecSettings.eFBStorageMode = ParseFrameBufferFormat(opt.popWord(), Config.tDecSettings.bFrameBufferCompression);
  }, "Specify the format of the decoded frame buffers (" + GetFrameBufferFormatOptDesc() + ")");

  opt.addFlag("--split-input", &Config.tDecSettings.eInputMode,
              "Send stream by decoding unit",
              AL_DEC_SPLIT_INPUT);

  opt.addString("--sei-file", &Config.seiFile, "File in which the SEI decoded by the decoder will be dumped");

  opt.addString("--hdr-file", &Config.hdrFile, "Parse and dump HDR data in the specified file");

  string preAllocArgs = "";
  opt.addString("--prealloc-args", &preAllocArgs, "Specify stream's parameters: 'widthxheight:video-mode:chroma-mode:bit-depth:profile:level' for example '1920x1080:progr:422:10:HEVC_MAIN:5'. video-mode values are: unkwn, progr or inter. Be careful cast is important.");
  opt.addCustom("--output-position", &Config.tDecSettings.tOutputPosition, &CoupleWithSeparator<AL_TPosition, ','>, "Specify the position of the decoded frame in frame buffer");

  opt.addFlag("--decode-intraonly", &Config.tDecSettings.tStream.bDecodeIntraOnly, "Decode Only I Frames");

  opt.startSection("Run");

  opt.addInt("--max-frames", &Config.iMaxFrames, "Abort after max number of decoded frames (approximative abort)");
  opt.addInt("-loop", &Config.iLoop, "Number of Decoding loop (optional)");
  opt.addInt("--timeout", &Config.iTimeoutInSeconds, "Specify timeout in seconds");

  bool dummyNextChan; // As the --next-channel is parsed elsewhere, this option is only used to add the description in the usage
  opt.addFlag("--next-chan", &dummyNextChan, "Start the configuration of a new decoding channel. The options that are applied on all channels must be specified in the first channel.");

  opt.addCustom("--exit-on", &Config.eExitCondition, parseExitOn, "Specifify early exit condition (e/error, w/warning)");

  opt.startSection("Trace && Debug");

  opt.addFlag("--multi-chunk", &g_MultiChunk, "Allocate luma and chroma of decoded frames on different memory chunks");
  opt.addInt("-nbuf", &Config.uInputBufferNum, "Specify the number of input feeder buffer");
  opt.addInt("-nsize", &Config.zInputBufferSize, "Specify the size (in bytes) of input feeder buffer");
  opt.addInt("-stream-buf-size", &Config.tDecSettings.iStreamBufSize, "Specify the size (in bytes) of internal circualr buffer size (0 = default)");

  opt.addString("-crc_ip", &Config.sCrc, "Output crc file");

  opt.addOption("-t", [&](string)
  {
    Config.iNumTrace = opt.popInt();
    Config.iNumberTrace = 1;
  }, "First frame to trace (optional)", "number");

  opt.addInt("-num", &Config.iNumberTrace, "Number of frames to trace");

  opt.addFlag("--use-early-callback", &Config.tDecSettings.bUseEarlyCallback, "Low latency phase 2. Call end decoding at decoding launch. This only makes sense with special support for hardware synchronization");
  opt.addInt("-core", &Config.tDecSettings.uNumCore, "Number of decoder cores");
  opt.addFlag("--non-realtime", &Config.tDecSettings.bNonRealtime, "Specifies that the channel is a non-realtime channel");
  opt.addInt("-ddrwidth", &Config.tDecSettings.uDDRWidth, "Width of DDR requests (16, 32, 64) (default: 32)");
  opt.addFlag("-nocache", &Config.tDecSettings.bDisableCache, "Inactivate the cache");

  extern std::string g_DecDevicePath;
  opt.addString("--device", &g_DecDevicePath, "Path of the driver device file used to talk with the IP. Default is /dev/allegroDecodeIP");

  opt.addFlag("-noyuv", &Config.bEnableYUVOutput,
              "Disable writing output YUV file",
              false);

  opt.addString("--md5", &Config.md5File, "Filename to the output MD5 of the YUV file");

  opt.addString("--log", &Config.logsFile, "A file where logged events will be dumped");

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
  opt.addFlag("-lowref", &Config.tDecSettings.eDpbMode,
              "Use --no-reordering instead",
              AL_DPB_NO_REORDERING);

  opt.addUint("--conceal-max-fps", &Config.tDecSettings.uConcealMaxFps, "Maximum fps to conceal invalid or corrupted stream header; 0 = no concealment");

  bool bHasDeprecated = opt.parse(argc, argv);

  if(Config.help)
  {
    Usage(opt, argv[0]);
    return Config;
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

  processOutputArgs(Config, sRasterOut);

  bool bMainOutputCompression;
  GetMainOutputStorageMode(Config.tDecSettings, bMainOutputCompression, 8);

  if(bMainOutputCompression)
  {
    if(bCertCRC)
      throw runtime_error("Certification CRC unavaible with fbc");
    bCertCRC = false;
  }

  if(fps > 0)
  {
    Config.tDecSettings.uFrameRate = fps * 1000;
    Config.tDecSettings.bForceFrameRate = true;
  }

  if(!sOutputBitDepth.empty())
    parseOutputBD(Config, sOutputBitDepth);

  if(!sOutputFormat.empty())
    parseOutputFormat(Config, sOutputFormat);

  {
    if(!preAllocArgs.empty())
    {
      parsePreAllocArgs(&Config.tDecSettings.tStream, Config.tDecSettings.eCodec, preAllocArgs);

      /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
      /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
      /* And because we don't know the codec here, always use 64 as MB/LCU size. */
      int iAlignValue = 8;

      if(Config.tDecSettings.eCodec == AL_CODEC_AVC)
        iAlignValue = 16;

      Config.tDecSettings.tStream.tDim.iWidth = RoundUp(Config.tDecSettings.tStream.tDim.iWidth, iAlignValue);
      Config.tDecSettings.tStream.tDim.iHeight = RoundUp(Config.tDecSettings.tStream.tDim.iHeight, iAlignValue);

      Config.bUsePreAlloc = true;
    }

    if(Config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT && !Config.bUsePreAlloc)
      throw std::runtime_error(" --split-input requires preallocation");

    if((Config.tDecSettings.tOutputPosition.iX || Config.tDecSettings.tOutputPosition.iY) && !Config.bUsePreAlloc)
      throw std::runtime_error(" --output-position requires preallocation");
  }

  if(Config.sIn.empty())
    throw runtime_error("No input file specified (use -h to get help)");

  return Config;
}

static void ConvertFrameBuffer(AL_TBuffer& input, AL_TBuffer*& pOutput, int iBdOut, const AL_TPosition& tPos, TFourCC tOutFourCC)
{
  (void)tPos;
  TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(&input);
  AL_TDimension tRecDim = AL_PixMapBuffer_GetDimension(&input);
  AL_EChromaMode eRecChromaMode = AL_GetChromaMode(tRecFourCC);

  TFourCC tConvFourCC;
  AL_TPicFormat tConvPicFormat;

  if(pOutput != NULL)
  {
    AL_TDimension tYuvDim = AL_PixMapBuffer_GetDimension(pOutput);

    if(tOutFourCC != FOURCC(NULL))
      tConvFourCC = tOutFourCC;
    else
      tConvFourCC = AL_PixMapBuffer_GetFourCC(pOutput);

    AL_GetPicFormat(tConvFourCC, &tConvPicFormat);

    if(tRecDim.iWidth != tYuvDim.iWidth || tRecDim.iHeight != tYuvDim.iHeight ||
       eRecChromaMode != tConvPicFormat.eChromaMode || iBdOut != tConvPicFormat.uBitDepth)
    {
      AL_Buffer_Destroy(pOutput);
      pOutput = NULL;
    }
  }

  AL_PixMapBuffer_SetDimension(&input, { tPos.iX + tRecDim.iWidth, tPos.iY + tRecDim.iHeight });

  if(pOutput == NULL)
  {
    AL_TDimension tDim = AL_PixMapBuffer_GetDimension(&input);

    tConvPicFormat = AL_TPicFormat {
      eRecChromaMode, static_cast<uint8_t>(iBdOut), AL_FB_RASTER,
      eRecChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : AL_C_ORDER_U_V, false, false
    };

    if(tOutFourCC != FOURCC(NULL))
      tConvFourCC = tOutFourCC;
    else
      tConvFourCC = AL_GetFourCC(tConvPicFormat);

    pOutput = AllocateDefaultYuvIOBuffer(tDim, tConvFourCC);

    if(pOutput == NULL)
      throw runtime_error("Couldn't allocate YuvBuffer");
  }

  if(ConvertPixMapBuffer(&input, pOutput))
    throw runtime_error("Couldn't convert buffer");

  AL_PixMapBuffer_SetDimension(&input, tRecDim);
  AL_PixMapBuffer_SetDimension(pOutput, tRecDim);
}

/******************************************************************************/
class BaseOutputWriter
{
public:
  BaseOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName);
  virtual ~BaseOutputWriter() {};

  void ProcessOutput(AL_TBuffer& tRecBuf, const AL_TInfoDecode& info, int iBdOut, TFourCC tOutFourCC);
  virtual void ProcessFrame(AL_TBuffer& tRecBuf, const AL_TInfoDecode& info, int iBdOut, TFourCC tOutFourCC) = 0;

protected:
  std::string m_sYuvFileName;
  std::string m_sIpCrcFileName;
  ofstream YuvFile;
  ofstream IpCrcFile;
};

BaseOutputWriter::BaseOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName)
{
  // Store the YUV filename, the file should be opened in all calls to a concrete ProcessFrame()
  m_sYuvFileName = sYuvFileName;

  // Store the CRC filename, the file should be opened when an attempt is made to write
  m_sIpCrcFileName = sIPCrcFileName;
}

void BaseOutputWriter::ProcessOutput(AL_TBuffer& tRecBuf, const AL_TInfoDecode& info, int iBdOut, TFourCC tOutFourCC)
{
  if(!m_sIpCrcFileName.empty() && !IpCrcFile.is_open())
  {
    OpenOutput(IpCrcFile, m_sIpCrcFileName, false);
    IpCrcFile << hex << uppercase;
  }

  if(IpCrcFile.is_open())
    IpCrcFile << std::setfill('0') << std::setw(8) << (int)info.uCRC << std::endl;

  ProcessFrame(tRecBuf, info, iBdOut, tOutFourCC);
}

/******************************************************************************/
class UncompressedOutputWriter : public BaseOutputWriter
{
public:
  ~UncompressedOutputWriter();

  UncompressedOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, const string& sMd5FileName, bool bRawMd5);
  void ProcessFrame(AL_TBuffer& tRecBuf, const AL_TInfoDecode& info, int iBdOut, TFourCC tOutFourCC) override;

private:
  std::string m_sCertCrcFileName;
  ofstream CertCrcFile; // Cert crc only computed for uncompressed output
  AL_TBuffer* YuvBuffer = NULL;
  Md5Calculator m_Md5Calculator;
  int convertBitDepthToEven(int iBd) const;
};

UncompressedOutputWriter::~UncompressedOutputWriter()
{
  if(NULL != YuvBuffer)
  {
    AL_Buffer_Destroy(YuvBuffer);
  }

  if(m_Md5Calculator.IsFileOpen())
  {
    m_Md5Calculator.Md5Output();
  }
}

UncompressedOutputWriter::UncompressedOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, const string& sMd5FileName, bool bRawMd5) :
  BaseOutputWriter(sYuvFileName, sIPCrcFileName),
  m_Md5Calculator(sMd5FileName)
{
  (void)bRawMd5;

  // Store the CRC filename, the file should be opened when an attempt is made to write
  m_sCertCrcFileName = sCertCrcFileName;

}

int UncompressedOutputWriter::convertBitDepthToEven(int iBd) const
{
  return ((iBd % 2) != 0) ? iBd + 1 : iBd;
}

void UncompressedOutputWriter::ProcessFrame(AL_TBuffer& tRecBuf, const AL_TInfoDecode& info, int iBdOut, TFourCC tOutFourCC)
{
  bool bWriteMd5 = m_Md5Calculator.IsFileOpen();

  // Open the YUV file if the filename has been set and it is not already open
  if(!m_sYuvFileName.empty() && !YuvFile.is_open())
    OpenOutput(YuvFile, m_sYuvFileName);

  if(!m_sCertCrcFileName.empty() && !CertCrcFile.is_open())
  {
    OpenOutput(CertCrcFile, m_sCertCrcFileName, false);
    CertCrcFile << hex << uppercase;
  }

  if(!m_sCertCrcFileName.empty() && !CertCrcFile.is_open())
  {
    OpenOutput(CertCrcFile, m_sCertCrcFileName, false);
    CertCrcFile << hex << uppercase;
  }

  if(!(YuvFile.is_open() || CertCrcFile.is_open() || bWriteMd5))
  {
    return;
  }

  AL_PixMapBuffer_SetDimension(&tRecBuf, info.tDim);
  AL_TPicFormat fmt = AL_GetDecPicFormat(info.eChromaMode, info.uBitDepthY, info.eFbStorageMode, false);
  AL_PixMapBuffer_SetFourCC(&tRecBuf, AL_GetDecFourCC(fmt));

  iBdOut = convertBitDepthToEven(iBdOut);

  bool bCrop = info.tCrop.bCropping;
  int32_t iCropLeft = info.tCrop.uCropOffsetLeft;
  int32_t iCropRight = info.tCrop.uCropOffsetRight;
  int32_t iCropTop = info.tCrop.uCropOffsetTop;
  int32_t iCropBottom = info.tCrop.uCropOffsetBottom;
  AL_TPosition tPos = { 0, 0 };

  if(info.tPos.iX || info.tPos.iY)
  {
    tPos = info.tPos;
    bCrop = true;
    iCropLeft += info.tPos.iX;
    iCropRight -= info.tPos.iX;
    iCropTop += info.tPos.iY;
    iCropBottom -= info.tPos.iY;
  }

  ConvertFrameBuffer(tRecBuf, YuvBuffer, iBdOut, tPos, tOutFourCC);

  auto const iSizePix = (iBdOut + 7) >> 3;

  if(bCrop)
    CropFrame(YuvBuffer, iSizePix, iCropLeft, iCropRight, iCropTop, iCropBottom);

  if(CertCrcFile.is_open())
    Compute_CRC(YuvBuffer, info.uBitDepthY, info.uBitDepthC, CertCrcFile);

  if(YuvFile.is_open())
  {
    WriteOneFrame(YuvFile, YuvBuffer);
  }

  if(bWriteMd5)
  {
    ComputeMd5SumFrame(YuvBuffer, m_Md5Calculator.GetCMD5());
  }

  if(bCrop)
    CropFrame(YuvBuffer, iSizePix, -iCropLeft, -iCropRight, -iCropTop, -iCropBottom);

}

/******************************************************************************/
struct Display
{
  Display()
  {
    hExitMain = Rtos_CreateEvent(false);
  }

  ~Display()
  {
    Rtos_DeleteEvent(hExitMain);
  }

  void AddOutputWriter(AL_e_FbStorageMode eFbStorageMode, bool bCompressionEnabled, const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, const string& sMd5FileName, bool bRawMd5);

  void Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo);
  void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut, TFourCC tFourCCOut);

  AL_HDecoder hDec = NULL;
  AL_EVENT hExitMain = NULL;
  std::map<AL_EFbStorageMode, std::shared_ptr<BaseOutputWriter>> writers;
  AL_EFbStorageMode eMainOutputStorageMode;
  int iBitDepth = 8;
  TFourCC tOutputFourCC = FOURCC(NULL);
  unsigned int NumFrames = 0;
  unsigned int MaxFrames = UINT_MAX;
  unsigned int FirstFrame = 0;
  mutex hMutex;
  int iNumFrameConceal = 0;
  std::shared_ptr<HDRWriter> pHDRWriter;
};

struct ResChgParam
{
  AL_HDecoder hDec;
  bool bUsePreAlloc;
  bool bPoolIsInit;
  PixMapBufPool bufPool;
  AL_TDecSettings* pDecSettings;
  AL_TAllocator* pAllocator;
  bool bAddHDRMetaData;
  AL_TPosition tOutputPosition;
  mutex hMutex;
};

struct DecodeParam
{
  AL_HDecoder hDec;
  AL_EVENT hExitMain = NULL;
  atomic<int> decodedFrames;
  ofstream* seiSyncOutput;
  map<AL_TBuffer*, std::vector<AL_TSeiMetaData*>> displaySeis;
};

struct DecoderErrorParam
{
  EDecErrorLevel eExitCondition;
  AL_EVENT hExitMain;
};

/******************************************************************************/
void Display::AddOutputWriter(AL_e_FbStorageMode eFbStorageMode, bool bCompressionEnabled, const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, const string& sMd5FileName, bool bRawMd5)
{
  (void)bCompressionEnabled;

  writers[eFbStorageMode] = std::shared_ptr<BaseOutputWriter>(new UncompressedOutputWriter(
                                                                sYuvFileName,
                                                                sIPCrcFileName,
                                                                sCertCrcFileName,
                                                                sMd5FileName,
                                                                bRawMd5));
}

/******************************************************************************/
/******************************************************************************/
static void printHexdump(ostream* logger, uint8_t* data, int size)
{
  int column = 0;
  int toPrint = size;

  *logger << std::hex;

  while(toPrint > 0)
  {
    *logger << setfill('0') << setw(2) << (int)data[size - toPrint];
    --toPrint;
    ++column;

    if(toPrint > 0)
    {
      if(column % 8 == 0)
        *logger << endl;
      else
        *logger << " ";
    }
  }

  *logger << std::dec;
}

static void writeSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, ostream* seiOut, int iNumFrame)
{
  if(!seiOut)
    return;

  if(iNumFrame != SEI_NOT_ASSOCIATED_WITH_FRAME)
    *seiOut << "Num Frame: " << iNumFrame << endl;

  *seiOut << "is_prefix: " << boolalpha << bIsPrefix << endl
          << "sei_payload_type: " << iPayloadType << endl
          << "sei_payload_size: " << iPayloadSize << endl
          << "raw:" << endl;
  printHexdump(seiOut, pPayload, iPayloadSize);
  *seiOut << endl << endl;
}

/******************************************************************************/
static void WriteSyncSei(std::vector<AL_TSeiMetaData*> seis, ofstream* seiOut, int iNumFrame)
{
  if(!seis.empty())
  {
    for(auto const& pSei: seis)
    {
      auto pPayload = pSei->payload;

      for(auto i = 0; i < pSei->numPayload; ++i, ++pPayload)
        writeSei(pPayload->bPrefix, pPayload->type, pPayload->pData, pPayload->size, seiOut, iNumFrame);
    }
  }
}

/******************************************************************************/
static void sInputParsed(AL_TBuffer* pParsedFrame, void* pUserParam, int iParsingID)
{
  auto pDisplaySeis = static_cast<map<AL_TBuffer*, std::vector<AL_TSeiMetaData*>>*>(pUserParam);

  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
    return;

  if(iParsingID > AL_HandleMetaData_GetNumHandles(pHandlesMeta))
    throw runtime_error("ParsingID is out of bounds");

  AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, iParsingID);

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSED)
  {
    AL_TBuffer* pStream = pDecMetaHandle->pHandle;

    if(!pStream)
      throw runtime_error("pStream is not allocated");

    auto seiMeta = (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_SEI);

    if(seiMeta != nullptr)
    {
      AL_Buffer_RemoveMetaData(pStream, (AL_TMetaData*)seiMeta);
      (*pDisplaySeis)[pParsedFrame].push_back(seiMeta);
    }

    return;
  }

  throw runtime_error("Input parsing error");
}

/******************************************************************************/
static void sFrameDecoded(AL_TBuffer* pDecodedFrame, void* pUserParam)
{
  auto pParam = static_cast<DecodeParam*>(pUserParam);
  auto seis = pParam->displaySeis[pDecodedFrame];

  int const currDecodedFrames = pParam->decodedFrames;
  pParam->decodedFrames++;

  if(pParam->seiSyncOutput)
  {
    WriteSyncSei(seis, pParam->seiSyncOutput, currDecodedFrames);

    for(auto const& pSei: seis)
      AL_MetaData_Destroy((AL_TMetaData*)pSei);

    pParam->displaySeis.erase(pDecodedFrame);
  }
}

/******************************************************************************/
static void sDecoderError(AL_ERR eError, void* pUserParam)
{
  auto pParam = static_cast<DecoderErrorParam*>(pUserParam);

  if(AL_IS_ERROR_CODE(eError) || pParam->eExitCondition == DEC_WARNING)
    Rtos_SetEvent(pParam->hExitMain);
}

/******************************************************************************/
static bool isEOS(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  return !pFrame && !pInfo;
}

/******************************************************************************/
static bool isReleaseFrame(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  return pFrame && !pInfo;
}

/******************************************************************************/
static void sFrameDisplay(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, void* pUserParam)
{
  auto pDisplay = reinterpret_cast<Display*>(pUserParam);

  if(pFrame)
    AL_Buffer_InvalidateMemory(pFrame);
  pDisplay->Process(pFrame, pInfo);
}

void Display::Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  unique_lock<mutex> lock(hMutex);

  bool bExitError = false;
  AL_ERR err = AL_SUCCESS;

  if(hDec)
  {
    err = AL_Decoder_GetFrameError(hDec, pFrame);
    bExitError |= AL_IS_ERROR_CODE(err);
  }

  if(bExitError || isEOS(pFrame, pInfo))
  {
    if(err != AL_SUCCESS)
      LogDimmedWarning("\n%s\n", AL_Codec_ErrorToString(err));

    if(err == AL_WARN_SEI_OVERFLOW)
      LogDimmedWarning("\nDecoder has discarded some SEI while the SEI metadata buffer was too small\n");

    if(bExitError)
      LogError("Error: %d\n", err);
    else
      LogVerbose(CC_GREY, "Complete\n\n");
    Rtos_SetEvent(hExitMain);
    return;
  }

  if(isReleaseFrame(pFrame, pInfo) || hDec == NULL)
    return;

  bool bIsExpectedStorageMode = (pInfo->eFbStorageMode == eMainOutputStorageMode);

  if(NumFrames < MaxFrames)
  {
    if(err == AL_WARN_CONCEAL_DETECT)
      iNumFrameConceal++;

    if(!AL_Buffer_GetData(pFrame))
      throw runtime_error("Data buffer is null");

    int iCurrentBitDepth = max(pInfo->uBitDepthY, pInfo->uBitDepthC);

    if(iBitDepth == OUTPUT_BD_FIRST)
      iBitDepth = iCurrentBitDepth;
    else if(iBitDepth == OUTPUT_BD_ALLOC)
      iBitDepth = AL_Decoder_GetMaxBD(hDec);

    int iEffectiveBitDepth = iBitDepth == OUTPUT_BD_STREAM ? iCurrentBitDepth : iBitDepth;

    ProcessFrame(*pFrame, *pInfo, iEffectiveBitDepth, tOutputFourCC);

    if(bIsExpectedStorageMode)
    {
      auto pHDR = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_HDR);

      if(pHDR != nullptr && pHDRWriter != nullptr)
        pHDRWriter->WriteHDRSEIs(pHDR->eColourDescription, pHDR->eTransferCharacteristics, pHDR->eColourMatrixCoeffs, pHDR->tHDRSEIs);
      // TODO: increase only when last frame
      DisplayFrameStatus(NumFrames);
    }
  }

  if(bIsExpectedStorageMode)
  {
    bool const bAdded = AL_Decoder_PutDisplayPicture(hDec, pFrame);

    if(!bAdded)
      throw runtime_error("bAdded must be true");
    NumFrames++;
  }

  if(NumFrames >= MaxFrames)
    Rtos_SetEvent(hExitMain);
}

/******************************************************************************/
void Display::ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut, TFourCC tFourCCOut)
{

  AL_EFbStorageMode eFbStorageMode = info.eFbStorageMode;

  if(writers.find(eFbStorageMode) != writers.end() && (NumFrames >= FirstFrame))
    writers[eFbStorageMode]->ProcessOutput(tRecBuf, info, iBdOut, tFourCCOut);
}

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
}

static string SequencePictureToString(AL_ESequenceMode sequencePicture)
{
  if(sequencePicture == AL_SM_UNKNOWN)
    return "unknown";

  if(sequencePicture == AL_SM_PROGRESSIVE)
    return "progressive";

  if(sequencePicture == AL_SM_INTERLACED)
    return "interlaced";
  return "max enum";
}

static void showStreamInfo(int BufferNumber, int BufferSize, AL_TStreamSettings const* pSettings, AL_TCropInfo const* pCropInfo, TFourCC tFourCC)
{
  auto& tDim = pSettings->tDim;
  int iWidth = tDim.iWidth;
  int iHeight = tDim.iHeight;

  stringstream ss;
  ss << "Resolution: " << iWidth << "x" << iHeight << endl;
  ss << "FourCC: " << FourCCToString(tFourCC) << endl;
  ss << "Profile: " << AL_GET_PROFILE_IDC(pSettings->eProfile) << endl;

  if(pSettings->iLevel != -1)
    ss << "Level: " << pSettings->iLevel << endl;
  ss << "Bitdepth: " << pSettings->iBitDepth << endl;

  if(AL_NeedsCropping(pCropInfo))
  {
    auto uCropWidth = pCropInfo->uCropOffsetLeft + pCropInfo->uCropOffsetRight;
    auto uCropHeight = pCropInfo->uCropOffsetTop + pCropInfo->uCropOffsetBottom;
    ss << "Crop top: " << pCropInfo->uCropOffsetTop << endl;
    ss << "Crop bottom: " << pCropInfo->uCropOffsetBottom << endl;
    ss << "Crop left: " << pCropInfo->uCropOffsetLeft << endl;
    ss << "Crop right: " << pCropInfo->uCropOffsetRight << endl;
    ss << "Display resolution: " << iWidth - uCropWidth << "x" << iHeight - uCropHeight << endl;
  }
  ss << "Sequence picture: " << SequencePictureToString(pSettings->eSequenceMode) << endl;
  ss << "Buffers needed: " << BufferNumber << " of size " << BufferSize << endl;

  LogInfo(CC_DARK_BLUE, "%s\n", ss.str().c_str());
}

static void sParsedSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, void* pUserParam)
{
  auto seiOutput = static_cast<ostream*>(pUserParam);
  writeSei(bIsPrefix, iPayloadType, pPayload, iPayloadSize, seiOutput, SEI_NOT_ASSOCIATED_WITH_FRAME);
}

void AddHDRMetaData(AL_TBuffer* pBufStream)
{
  if(AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_HDR))
    return;

  auto pHDReta = AL_HDRMetaData_Create();

  if(pHDReta)
    AL_Buffer_AddMetaData(pBufStream, (AL_TMetaData*)pHDReta);
}

static int sConfigureDecBufPool(PixMapBufPool& SrcBufPool, AL_TPicFormat tPicFormat, AL_TDimension tDim, int iPitchY, bool bConfigurePlanarAndSemiplanar)
{
  auto const tFourCC = AL_GetDecFourCC(tPicFormat);
  SrcBufPool.SetFormat(tDim, tFourCC);

  std::vector<AL_TPlaneDescription> vPlaneDesc;
  int iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat.eChromaOrder, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int iPitch = usedPlanes[iPlane] == AL_PLANE_Y ? iPitchY : AL_GetChromaPitch(tFourCC, iPitchY);
    vPlaneDesc.push_back(AL_TPlaneDescription { usedPlanes[iPlane], iOffset, iPitch });

    /* We ensure compatibility with 420/422. Only required when we use prealloc configured for
     * 444 chroma-mode (worst case) and the real chroma-mode is unknown. Breaks planes agnostic
     * allocation. */

    if(bConfigurePlanarAndSemiplanar && usedPlanes[iPlane] == AL_PLANE_U)
      vPlaneDesc.push_back(AL_TPlaneDescription { AL_PLANE_UV, iOffset, iPitch });

    iOffset += AL_DecGetAllocSize_Frame_PixPlane(tPicFormat.eStorageMode, tDim, iPitch, tPicFormat.eChromaMode, usedPlanes[iPlane]);

    if(g_MultiChunk)
    {
      SrcBufPool.AddChunk(iOffset, vPlaneDesc);
      vPlaneDesc.clear();
      iOffset = 0;
    }
  }

  if(!g_MultiChunk)
    SrcBufPool.AddChunk(iOffset, vPlaneDesc);

  return iOffset;
}

static AL_ERR sResolutionFound(int BufferNumber, int BufferSizeLib, AL_TStreamSettings const* pSettings, AL_TCropInfo const* pCropInfo, void* pUserParam)
{
  (void)BufferSizeLib;
  ResChgParam* p = (ResChgParam*)pUserParam;
  AL_TDecSettings* pDecSettings = p->pDecSettings;

  unique_lock<mutex> lock(p->hMutex);

  if(!p->hDec)
    return AL_ERROR;

  bool bMainOutputCompression;
  AL_e_FbStorageMode eMainOutputStorageMode = GetMainOutputStorageMode(*pDecSettings, bMainOutputCompression, pSettings->iBitDepth);

  auto tPicFormat = AL_GetDecPicFormat(pSettings->eChroma, pSettings->iBitDepth, eMainOutputStorageMode, bMainOutputCompression);
  auto tFourCC = AL_GetDecFourCC(tPicFormat);

  AL_TDimension tOutputDim = pSettings->tDim;

  /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
  /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
  /* And because we don't know the codec here, always use 64 as MB/LCU size. */
  tOutputDim.iWidth = RoundUp(tOutputDim.iWidth, 64);
  tOutputDim.iHeight = RoundUp(tOutputDim.iHeight, 64);

  auto minPitch = AL_Decoder_GetMinPitch(tOutputDim.iWidth, pSettings->iBitDepth, eMainOutputStorageMode);

  /* get size for print */
  int BufferSize = 0;

  if(p->bPoolIsInit)
    BufferSize = AL_DecGetAllocSize_Frame(tOutputDim, minPitch, pSettings->eChroma, bMainOutputCompression, eMainOutputStorageMode);
  else
  {
    bool bConfigurePlanarAndSemiplanar = p->bUsePreAlloc;
    BufferSize = sConfigureDecBufPool(p->bufPool, tPicFormat, tOutputDim, minPitch, bConfigurePlanarAndSemiplanar);
  }

  if(BufferSize < BufferSizeLib)
    throw runtime_error("Buffer size is insufficient");

  showStreamInfo(BufferNumber, BufferSize, pSettings, pCropInfo, tFourCC);

  /* stream resolution change */
  if(p->bPoolIsInit)
    return AL_SUCCESS;

  int iNumBuf = BufferNumber + uDefaultNumBuffersHeldByNextComponent;

  if(!p->bufPool.Init(p->pAllocator, iNumBuf, "decoded picture buffer"))
    return AL_ERR_NO_MEMORY;

  p->bPoolIsInit = true;

  for(int i = 0; i < iNumBuf; ++i)
  {
    auto pDecPict = p->bufPool.GetBuffer(AL_BUF_MODE_NONBLOCK);

    if(!pDecPict)
      throw runtime_error("pDecPict is null");
    AL_Buffer_MemSet(pDecPict, 0xDE);

    if(p->bAddHDRMetaData)
      AddHDRMetaData(pDecPict);
    bool const bAdded = AL_Decoder_PutDisplayPicture(p->hDec, pDecPict);

    if(!bAdded)
      throw runtime_error("bAdded must be true");

    AL_Buffer_Unref(pDecPict);
  }

  return AL_SUCCESS;
}

/******************************************************************************/

void ShowStatistics(double durationInSeconds, int iNumFrameConceal, int decodedFrameNumber, bool timeoutOccured)
{
  string guard = "Decoded time = ";

  if(timeoutOccured)
    guard = "TIMEOUT = ";

  auto msg = guard + "%.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n";
  LogInfo(msg.c_str(),
          durationInSeconds,
          decodedFrameNumber / durationInSeconds,
          iNumFrameConceal);
}

/******************************************************************************/
struct AsyncFileInput
{
  AsyncFileInput(AL_HDecoder hDec_, string path, BufPool& bufPool_, bool bSplitInput, AL_ECodec eCodec, bool bVclSplit)
    : hDec(hDec_), bufPool(bufPool_)
  {
    (void)eCodec;

    exit = false;
    OpenInput(ifFileStream, path);

    if(bSplitInput)
    {

      if(AL_IS_ITU_CODEC(eCodec))
        m_Loader.reset(new SplitInput(bufPool.m_pool.zBufSize, eCodec, bVclSplit));

      if(!m_Loader.get())
        throw runtime_error("Null unique pointer");
    }
    else
      m_Loader.reset(new BasicLoader());

    m_thread = thread(&AsyncFileInput::run, this);
  }

  ~AsyncFileInput()
  {
    exit = true;
    m_thread.join();
  }

private:
  void run()
  {
    Rtos_SetCurrentThreadName("FileInput");

    while(!exit)
    {
      shared_ptr<AL_TBuffer> pBufStream;
      try
      {
        pBufStream = shared_ptr<AL_TBuffer>(
          bufPool.GetBuffer(),
          &AL_Buffer_Unref);
      }
      catch(bufpool_decommited_error &)
      {
        continue;
      }

      uint8_t uBufFlags;
      auto uAvailSize = m_Loader->ReadStream(ifFileStream, pBufStream.get(), uBufFlags);

      if(!uAvailSize)
      {
        // end of input
        AL_Decoder_Flush(hDec);
        break;
      }

      auto bRet = AL_Decoder_PushStreamBuffer(hDec, pBufStream.get(), uAvailSize, uBufFlags);

      if(!bRet)
        throw runtime_error("Failed to push buffer");
    }
  }

  const AL_HDecoder hDec;
  ifstream ifFileStream;
  BufPool& bufPool;
  atomic<bool> exit;
  std::unique_ptr<InputLoader> m_Loader;
  thread m_thread;
};

constexpr int MAX_CHANNELS = 32;

int GetChannelsArgv(vector<char*>* argvChannels, int argc, char** argv)
{
  int curChan = 0;

  for(int i = 0; i < argc; ++i)
  {
    if(string(argv[i]) == "--next-chan")
    {
      ++curChan;

      if(curChan >= MAX_CHANNELS)
        throw runtime_error("Too many channels");

      argvChannels[curChan].push_back(argv[0]);
      continue;
    }

    argvChannels[curChan].push_back(argv[i]);
  }

  return curChan;
}

struct WorkerConfig
{
  Config* pConfig;
  CIpDevice* pIpDevice;
  bool bUseBoard;
};

/******************************************************************************/
void AdjustStreamBufferSettings(Config& config)
{
  unsigned int uMinStreamBuf = config.tDecSettings.iStackSize;
  config.uInputBufferNum = max(uMinStreamBuf, config.uInputBufferNum);

  config.zInputBufferSize = max(size_t(1), config.zInputBufferSize);
  config.zInputBufferSize = (config.bUsePreAlloc && config.zInputBufferSize == zDefaultInputBufferSize) ? AL_GetMaxNalSize(config.tDecSettings.tStream.tDim, config.tDecSettings.tStream.eChroma, config.tDecSettings.tStream.iBitDepth, config.tDecSettings.tStream.eProfile, config.tDecSettings.tStream.iLevel) : config.zInputBufferSize;
}

void SafeChannelMain(WorkerConfig& w)
{
  auto pAllocator = w.pIpDevice->GetAllocator();
  auto pScheduler = w.pIpDevice->GetScheduler();
  auto& Config = *w.pConfig;
  bool bUseBoard = w.bUseBoard;

  ofstream seiOutput;
  ofstream seiSyncOutput;

  if(!Config.seiFile.empty())
  {
    OpenOutput(seiOutput, Config.seiFile);

    if(Config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT)
      OpenOutput(seiSyncOutput, Config.seiFile + "_sync.txt");
  }

  FILE* out = stdout;

  if(!g_Verbosity)
    out = NULL;

  auto const err = AL_DecSettings_CheckValidity(&Config.tDecSettings, out);

  if(err != 0)
  {
    stringstream ss;
    ss << err << " errors(s). " << "Invalid settings, please check your command line.";
    throw runtime_error(ss.str());
  }

  auto const incoherencies = AL_DecSettings_CheckCoherency(&Config.tDecSettings, out);

  if(incoherencies == -1)
    throw runtime_error("Fatal coherency error in settings, please check your command line.");

  AdjustStreamBufferSettings(Config);

  bool bRawMd5 = false;

  BufPool bufPool;

  {
    AL_TBufPoolConfig BufPoolConfig {};
    BufPoolConfig.debugName = "stream";
    BufPoolConfig.zBufSize = Config.zInputBufferSize;
    BufPoolConfig.uNumBuf = Config.uInputBufferNum;

    auto pBufPoolAllocator = Config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT ? pAllocator : AL_GetDefaultAllocator();

    auto ret = bufPool.Init(pBufPoolAllocator, BufPoolConfig);

    if(!ret)
      throw runtime_error("Can't create BufPool");
  }

  Display display;

  bool bMainOutputCompression;
  display.eMainOutputStorageMode = GetMainOutputStorageMode(Config.tDecSettings, bMainOutputCompression, 8);

  bool bHasOutput = Config.bEnableYUVOutput || bCertCRC || !Config.sCrc.empty() || !Config.md5File.empty();

  if(bHasOutput)
  {
    const string sCertCrcFile = bCertCRC ? "crc_certif_res.hex" : "";

    {
      display.AddOutputWriter(display.eMainOutputStorageMode, bMainOutputCompression, Config.sMainOut, Config.sCrc, sCertCrcFile, Config.md5File, bRawMd5);
    }

  }

  display.iBitDepth = Config.iOutputBitDepth;
  display.tOutputFourCC = Config.tOutputFourCC;
  display.MaxFrames = Config.iMaxFrames;

  if(!Config.hdrFile.empty())
    display.pHDRWriter = shared_ptr<HDRWriter>(new HDRWriter(Config.hdrFile));

  ResChgParam ResolutionFoundParam;
  ResolutionFoundParam.bUsePreAlloc = Config.bUsePreAlloc;
  ResolutionFoundParam.pAllocator = pAllocator;
  ResolutionFoundParam.bPoolIsInit = false;
  ResolutionFoundParam.pDecSettings = &Config.tDecSettings;
  ResolutionFoundParam.tOutputPosition = Config.tDecSettings.tOutputPosition;
  ResolutionFoundParam.bAddHDRMetaData = display.pHDRWriter != nullptr;

  DecodeParam tDecodeParam {};
  tDecodeParam.hExitMain = display.hExitMain;
  tDecodeParam.seiSyncOutput = &seiSyncOutput;

  DecoderErrorParam tDecoderErrorParam {};
  tDecoderErrorParam.hExitMain = display.hExitMain;
  tDecoderErrorParam.eExitCondition = Config.eExitCondition;

  AL_TDecCallBacks CB {};
  CB.endParsingCB = { &sInputParsed, &tDecodeParam.displaySeis };
  CB.endDecodingCB = { &sFrameDecoded, &tDecodeParam };
  CB.displayCB = { &sFrameDisplay, &display };
  CB.resolutionFoundCB = { &sResolutionFound, &ResolutionFoundParam };
  CB.parsedSeiCB = { &sParsedSei, (void*)&seiOutput };
  CB.errorCB = { &sDecoderError, &tDecoderErrorParam };

  AL_HDecoder hDec;
  AL_ERR error;

  error = AL_Decoder_Create(&hDec, (AL_IDecScheduler*)pScheduler, pAllocator, &Config.tDecSettings, &CB);

  if(AL_IS_ERROR_CODE(error))
    throw codec_error(error);

  if(!hDec)
    throw runtime_error("hDec is null");

  auto decoderAlreadyDestroyed = false;
  auto scopeDecoder = scopeExit([&]() {
    if(!decoderAlreadyDestroyed)
    {
      display.hMutex.lock();
      display.hDec = NULL;
      display.hMutex.unlock();
      AL_Decoder_Destroy(hDec);
    }
  });

  // Param of Display Callback assignment
  display.hDec = hDec;
  tDecodeParam.hDec = hDec;
  ResolutionFoundParam.hDec = hDec;

  AL_Decoder_SetParam(hDec, bUseBoard ? "Fpga" : "Ref", Config.iNumTrace, Config.iNumberTrace, Config.bForceCleanBuffers, Config.ipCtrlMode == AL_IPCTRL_MODE_TRACE);

  if(Config.bUsePreAlloc && !AL_Decoder_PreallocateBuffers(hDec))
    if(auto eErr = AL_Decoder_GetLastError(hDec))
      throw codec_error(eErr);

  const AL_ECodec eCodec = Config.tDecSettings.eCodec;

  // Initial stream buffer filling
  auto const uBegin = GetPerfTime();
  bool timeoutOccured = false;

  for(int iLoop = 0; iLoop < Config.iLoop; ++iLoop)
  {
    bufPool.Commit();

    if(iLoop > 0)
      LogVerbose(CC_GREY, "  Looping\n");

    AsyncFileInput producer(hDec, Config.sIn, bufPool, Config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT, eCodec, Config.tDecSettings.eDecUnit == AL_VCL_NAL_UNIT);

    auto const maxWait = Config.iTimeoutInSeconds * 1000;
    auto const timeout = maxWait >= 0 ? maxWait : AL_WAIT_FOREVER;

    if(!Rtos_WaitEvent(display.hExitMain, timeout))
    {
      timeoutOccured = true;
    }
    bufPool.Decommit();
  }

  auto const uEnd = GetPerfTime();

  unique_lock<mutex> lock(display.hMutex);
  auto eErr = AL_Decoder_GetLastError(hDec);

  if(AL_IS_ERROR_CODE(eErr) || (AL_IS_WARNING_CODE(eErr) && Config.eExitCondition == DEC_WARNING))
    throw codec_error(eErr);

  if(!tDecodeParam.decodedFrames)
    throw runtime_error("No frame decoded");

  auto const duration = (uEnd - uBegin) / 1000.0;
  ShowStatistics(duration, display.iNumFrameConceal, tDecodeParam.decodedFrames, timeoutOccured);
}

static void ChannelMain(WorkerConfig& w, std::exception_ptr& exception)
{
  try
  {
    SafeChannelMain(w);
    exception = nullptr;
    return;
  }
  catch(codec_error const& error)
  {
    exception = std::current_exception();
  }
  catch(runtime_error const& error)
  {
    exception = std::current_exception();
  }
}

/******************************************************************************/
void SafeMain(int argc, char** argv)
{
  InitializePlateform();

  vector<char*> argvChannels[MAX_CHANNELS] {};
  int const maxChan = GetChannelsArgv(argvChannels, argc, argv);

  Config cfgChannels[MAX_CHANNELS];
  std::exception_ptr errorChannels[MAX_CHANNELS] {};
  WorkerConfig workerConfigs[MAX_CHANNELS];
  std::thread worker[MAX_CHANNELS];

  for(int chan = 0; chan <= maxChan; ++chan)
    cfgChannels[chan] = ParseCommandLine(argvChannels[chan].size(), argvChannels[chan].data());

  // Use first channel to configure the ip device
  auto Config = cfgChannels[0];

  if(Config.help)
    return;

  DisplayVersionInfo();
  AL_ELibDecoderArch eArch = AL_LIB_DECODER_ARCH_HOST;

  if(AL_Lib_Decoder_Init(eArch) != AL_SUCCESS)
    throw runtime_error("Can't setup decode library");

  // IP Device ------------------------------------------------------------

  CIpDeviceParam param;
  param.iSchedulerType = Config.iSchedulerType;
  param.iDeviceType = Config.iDeviceType;
  param.bTrackDma = Config.trackDma;
  param.uNumCore = Config.tDecSettings.uNumCore;
  param.iHangers = Config.hangers;
  param.ipCtrlMode = Config.ipCtrlMode;
  param.apbFile = Config.apbFile;

  std::shared_ptr<CIpDevice> pIpDevice = std::shared_ptr<CIpDevice>(new CIpDevice);

  if(!pIpDevice)
    throw runtime_error("Can't create IpDevice");

  pIpDevice->Configure(param);

  bool bUseBoard = (param.iDeviceType == AL_DEVICE_TYPE_BOARD); // retrieve auto-detected device type

  // mono channel case
  if(maxChan == 0)
  {
    WorkerConfig w {};
    w.pConfig = &cfgChannels[maxChan];
    w.pIpDevice = pIpDevice.get();
    w.bUseBoard = bUseBoard;

    workerConfigs[maxChan] = w;
    ChannelMain(workerConfigs[maxChan], errorChannels[maxChan]);

    if(errorChannels[maxChan])
      std::rethrow_exception(errorChannels[maxChan]);
  }
  // multichannel case
  else
  {
    for(int chan = 0; chan <= maxChan; ++chan)
    {
      WorkerConfig w
      {
        &cfgChannels[chan],
        pIpDevice.get(),
        bUseBoard,
      };

      workerConfigs[chan] = w;
      worker[chan] = std::thread(&ChannelMain, std::ref(workerConfigs[chan]), std::ref(errorChannels[chan]));
    }

    for(int chan = 0; chan <= maxChan; ++chan)
      worker[chan].join();

    for(int chan = 0; chan <= maxChan; ++chan)
    {
      if(errorChannels[chan])
      {
        cerr << "Channel " << chan << " has errors" << endl;
        std::rethrow_exception(errorChannels[chan]);
      }
    }
  }
  AL_Lib_Decoder_DeInit();
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
    return error.Code;
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    return 1;
  }
}

/******************************************************************************/

