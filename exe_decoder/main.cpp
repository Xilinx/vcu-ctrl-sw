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

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <climits>
#include <atomic>
#include <memory>
#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <sstream>
#include <mutex>
#include <queue>
#include <map>
extern "C"
{
#include "lib_common/BufferSrcMeta.h"
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common/StreamBuffer.h"
}

#include "lib_app/BufPool.h"
#include "lib_app/console.h"
#include "lib_app/convert.h"
#include "lib_app/timing.h"
#include "lib_app/utils.h"
#include "lib_app/CommandLineParser.h"


#include "Conversion.h"
#include "al_resource.h"
#include "IpDevice.h"
#include "CodecUtils.h"
#include "crc.h"

using namespace std;

const char* ToString(AL_ERR eErrCode)
{
  switch(eErrCode)
  {
  case AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE: return "Channel not created, no channel available";
  case AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE: return "Channel not created, processing power of the available cores insufficient";
  case AL_ERR_CHAN_CREATION_NOT_ENOUGH_CORES: return "Channel not created, couldn't spread the load on enough cores";
  case AL_ERR_REQUEST_MALFORMED: return "Channel not created: request was malformed";
  case AL_ERR_NO_MEMORY: return "Memory shortage detected (dma, embedded memory or virtual memory shortage)";
  case AL_SUCCESS: return "Success";
  default: return "Unknown error";
  }
}

struct codec_error : public runtime_error
{
  codec_error(AL_ERR eErrCode) : runtime_error(ToString(eErrCode)), Code(eErrCode)
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
static uint8_t constexpr NUMCORE_AUTO = 0;

AL_TDecSettings getDefaultDecSettings()
{
  AL_TDecSettings settings {};

  settings.iStackSize = 2;
  settings.iBitDepth = -1;
  settings.uNumCore = NUMCORE_AUTO;
  settings.uFrameRate = 60000;
  settings.uClkRatio = 1000;
  settings.uDDRWidth = 32;
  settings.eDecUnit = AL_AU_UNIT;
  settings.eDpbMode = AL_DPB_NORMAL;
  settings.eFBStorageMode = AL_FB_RASTER;
  settings.tStream.tDim = { -1, -1 };
  settings.tStream.eChroma = AL_CHROMA_MAX_ENUM;
  settings.tStream.iBitDepth = -1;
  settings.tStream.iProfileIdc = -1;
  settings.tStream.eSequenceMode = AL_SM_MAX_ENUM;
  settings.eCodec = AL_CODEC_HEVC;
  settings.eBufferOutputMode = AL_OUTPUT_INTERNAL;
  settings.bUseIFramesAsSyncPoint = false;

  return settings;
}

static int const zDefaultInputBufferSize = 32 * 1024;

struct Config
{
  bool help = false;

  string sIn;
  string sMainOut; // Output rec file
  string sCrc;

  AL_TDecSettings tDecSettings = getDefaultDecSettings();
  int iUseBoard = 1; // board
  SCHEDULER_TYPE iSchedulerType = SCHEDULER_TYPE_MCU;
  int iNumTrace = -1;
  int iNumberTrace = 0;
  bool bForceCleanBuffers = false;
  bool bConceal = false;
  bool bEnableYUVOutput = true;
  unsigned int uInputBufferNum = 2;
  size_t zInputBufferSize = zDefaultInputBufferSize;
  IpCtrlMode ipCtrlMode = IPCTRL_MODE_STANDARD;
  string logsFile = "";
  bool trackDma = false;
  int hangers = 0;
  int iLoop = 1;
  int iTimeoutInSeconds = -1;
  int iMaxFrames = INT_MAX;
  int iFirstFrame = 0;
  string seiFile = "";
};

/******************************************************************************/
static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cerr << "Usage: " << ExeName << " -in <bitstream_file> -out <yuv_file> [options]" << endl;
  cerr << "Options:" << endl;

  for(auto& name : opt.displayOrder)
  {
    auto& o = opt.options.at(name);
    cerr << "  " << o.desc << endl;
  }

  cerr << "Examples:" << endl;
  cerr << "  " << ExeName << " -avc  -in bitstream.264 -out decoded.yuv -bd 8 " << endl;
  cerr << "  " << ExeName << " -hevc -in bitstream.265 -out decoded.yuv -bd 10" << endl;
  cerr << endl;
}

template<int Offset>
static int IntWithOffset(const string& word)
{
  return atoi(word.c_str()) + Offset;
}

/******************************************************************************/
static AL_EFbStorageMode getMainOutputStorageMode(const AL_TDecSettings& decSettings, bool& bOutputCompression)
{
  AL_EFbStorageMode eOutputStorageMode = decSettings.eFBStorageMode;
  bOutputCompression = decSettings.bFrameBufferCompression;


  return eOutputStorageMode;
}

/******************************************************************************/
void processOutputArgs(Config& config, const string& sOut, const string& sRasterOut)
{
  (void)sRasterOut;

  if(!config.bEnableYUVOutput)
  {
    if(!sOut.empty() || !sRasterOut.empty())
    {
      throw runtime_error("Can't specify output yuv with -noyuv option enabled");
    }
  }

  config.tDecSettings.eBufferOutputMode = AL_OUTPUT_INTERNAL;

  if(sOut.empty())
  {
    config.sMainOut = config.bEnableYUVOutput ? "dec.yuv" : "";
  }
  else
  {
    config.sMainOut = sOut;
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

bool invalidPreallocSettings(AL_TStreamSettings const& settings)
{
  return settings.iProfileIdc <= 0 || settings.iLevel <= 0
         || settings.tDim.iWidth <= 0 || settings.tDim.iHeight <= 0 || settings.eChroma == AL_CHROMA_MAX_ENUM || settings.eSequenceMode == AL_SM_MAX_ENUM;
}

void parsePreAllocArgs(AL_TStreamSettings* settings, string& toParse)
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
  ss >> settings->iProfileIdc;
  getExpectedSeparator(ss, ':');
  ss >> settings->iLevel;

  /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
  /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
  /* And because we don't know the codec here, always use 64 as MB/LCU size. */
  settings->tDim.iWidth = RoundUp(settings->tDim.iWidth, 64);
  settings->tDim.iHeight = RoundUp(settings->tDim.iHeight, 64);

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

  if(invalidPreallocSettings(*settings))
    throw runtime_error("wrong prealloc arguments");
}

/******************************************************************************/
static Config ParseCommandLine(int argc, char* argv[])
{
  Config Config;

  bool quiet = false;
  int fps = 0;
  bool version = false;

  string sOut;
  string sRasterOut;

  auto opt = CommandLineParser();

  opt.addFlag("--help,-h", &Config.help, "Shows this help");
  opt.addFlag("--version", &version, "Show version");
  opt.addString("-in,-i", &Config.sIn, "Input bitstream");
  opt.addString("-out,-o", &sOut, "Output YUV");
  opt.addInt("-nbuf", &Config.uInputBufferNum, "Specify the number of input feeder buffer");
  opt.addInt("-nsize", &Config.zInputBufferSize, "Specify the size (in bytes) of input feeder buffer");
  opt.addInt("-num", &Config.iNumberTrace, "Number of frames to trace");
  opt.addFlag("--quiet,-q", &quiet, "quiet mode");
  opt.addInt("-core", &Config.tDecSettings.uNumCore, "number of hevc_decoder cores");
  opt.addInt("-fps", &fps, "force framerate");
  opt.addInt("-bd", &Config.tDecSettings.iBitDepth, "Output YUV bitdepth (0:auto, 8, 10)");
  opt.addString("-crc_ip", &Config.sCrc, "Output crc file");
  opt.addFlag("-wpp", &Config.tDecSettings.bParallelWPP, "Wavefront parallelization processing activation");
  opt.addFlag("-lowlat", &Config.tDecSettings.bLowLat, "Low latency decoding activation");
  opt.addFlag("--use-early-callback", &Config.tDecSettings.bUseEarlyCallback, "Low latency phase 2. Call end decoding at decoding launch. This only makes sense with special support for hardware synchronization");
  opt.addInt("-ddrwidth", &Config.tDecSettings.uDDRWidth, "Width of DDR requests (16, 32, 64) (default: 32)");
  opt.addFlag("-nocache", &Config.tDecSettings.bDisableCache, "Inactivate the cache");
  opt.addOption("--raster", [&]()
  {
    Config.tDecSettings.eFBStorageMode = AL_FB_RASTER;
    Config.tDecSettings.bFrameBufferCompression = false;
  }, "Store frame buffers in raster format");
  opt.addOption("--tile", [&]()
  {
    Config.tDecSettings.eFBStorageMode = AL_FB_TILE_32x4;
    Config.tDecSettings.bFrameBufferCompression = false;
  }, "Store frame buffers in tiles format");



  opt.addOption("-t", [&]()
  {
    Config.iNumTrace = opt.popInt();
    Config.iNumberTrace = 1;
  }, "First frame to trace (optional)");

  opt.addCustom("-clk", &Config.tDecSettings.uClkRatio, &IntWithOffset<1000>, "Set clock ratio");

  opt.addFlag("-lowref", &Config.tDecSettings.eDpbMode,
              "[DEPRECATED] Use --no-reordering instead. Indicates to decoder that the stream doesn't contain B-frame & reference must be at best 1)",
              AL_DPB_NO_REORDERING);

  opt.addFlag("--no-reordering", &Config.tDecSettings.eDpbMode,
              "Indicates to decoder that the stream doesn't contain B-frame & reference must be at best 1)",
              AL_DPB_NO_REORDERING);

  opt.addOption("-slicelat", [&]()
  {
    Config.tDecSettings.eDecUnit = AL_VCL_NAL_UNIT;
    Config.tDecSettings.eDpbMode = AL_DPB_NO_REORDERING;
  }, "Specify decoder latency (default: Frame Latency)");

  opt.addFlag("-framelat", &Config.tDecSettings.eDecUnit,
              "Specify decoder latency (default: Frame Latency)",
              AL_AU_UNIT);

  opt.addFlag("-avc", &Config.tDecSettings.eCodec,
              "Specify the input bitstream codec (default: HEVC)",
              AL_CODEC_AVC);

  opt.addFlag("-hevc", &Config.tDecSettings.eCodec,
              "Specify the input bitstream codec (default: HEVC)",
              AL_CODEC_HEVC);


  opt.addFlag("-noyuv", &Config.bEnableYUVOutput,
              "Disable writing output YUV file",
              false);

  opt.addFlag("--sync-i-frames", &Config.tDecSettings.bUseIFramesAsSyncPoint,
              "Allow decoder to sync on I frames is configurations' nals are presents",
              true);

  opt.addInt("-loop", &Config.iLoop, "Number of Decoding loop (optional)");

  opt.addString("--log", &Config.logsFile, "A file where logged events will be dumped");


  string preAllocArgs = "";
  opt.addInt("--timeout", &Config.iTimeoutInSeconds, "Specify timeout in seconds");
  opt.addInt("--max-frames", &Config.iMaxFrames, "Abort after max number of decoded frames (approximative abort)");
  opt.addString("--prealloc-args", &preAllocArgs, "Specify stream's parameters: '1920x1080:video-mode:422:10:profile-idc:level'.");
  opt.addString("--sei-file", &Config.seiFile, "File in which the SEI decoded by the decoder will be dumped");

  opt.parse(argc, argv);

  if(Config.help)
  {
    Usage(opt, argv[0]);
    return Config;
  }

  if(version)
  {
    DisplayVersionInfo();
    DisplayBuildInfo();
    exit(0);
  }

  processOutputArgs(Config, sOut, sRasterOut);

  bool bMainOutputCompression;
  getMainOutputStorageMode(Config.tDecSettings, bMainOutputCompression);

  if(bMainOutputCompression)
  {
    if(bCertCRC)
      throw runtime_error("Certification CRC unavaible with fbc");
    bCertCRC = false;
  }

  if(quiet)
    g_Verbosity = 0;

  if(fps > 0)
  {
    Config.tDecSettings.uFrameRate = fps * 1000;
    Config.tDecSettings.bForceFrameRate = true;
  }

  {
    if(!preAllocArgs.empty())
      parsePreAllocArgs(&Config.tDecSettings.tStream, preAllocArgs);

    if(Config.tDecSettings.uNumCore > AL_DEC_NUM_CORES)
      throw runtime_error("Invalid number of cores");

    if(Config.tDecSettings.uDDRWidth != 16 && Config.tDecSettings.uDDRWidth != 32 && Config.tDecSettings.uDDRWidth != 64)
      throw runtime_error("Invalid DDR width");

    // silently correct user settings
    Config.uInputBufferNum = max(1u, Config.uInputBufferNum);
    Config.zInputBufferSize = max(size_t(1), Config.zInputBufferSize);
    Config.zInputBufferSize = (!preAllocArgs.empty() && Config.zInputBufferSize == zDefaultInputBufferSize) ? AL_GetMaxNalSize(Config.tDecSettings.eCodec, Config.tDecSettings.tStream.tDim, Config.tDecSettings.tStream.eChroma, Config.tDecSettings.tStream.iBitDepth, Config.tDecSettings.tStream.iLevel, Config.tDecSettings.tStream.iProfileIdc) : Config.zInputBufferSize;
    Config.tDecSettings.iStackSize = max(1, Config.tDecSettings.iStackSize);
  }

  if(Config.sIn.empty())
    throw runtime_error("No input file specified (use -h to get help)");

  return Config;
}

typedef function<void (AL_TBuffer const*, AL_TBuffer*)> AL_TO_IP;
typedef void AL_TO_IP_SCALE (const AL_TBuffer*, AL_TBuffer*, uint8_t, uint8_t);

AL_TO_IP Bind(AL_TO_IP_SCALE* convertFunc, int horzScale, int vertScale)
{
  auto conversion = [=](AL_TBuffer const* src, AL_TBuffer* dst)
                    {
                      convertFunc(src, dst, horzScale, vertScale);
                    };

  return conversion;
}

static int GetPictureSizeInSamples(AL_TSrcMetaData* meta)
{
  int sx;
  int sy;
  AL_GetSubsampling(meta->tFourCC, &sx, &sy);

  int sampleCount = meta->tDim.iWidth * meta->tDim.iHeight;

  if(AL_GetChromaMode(meta->tFourCC) != AL_CHROMA_MONO)
    sampleCount += ((sampleCount * 2) / (sx * sy));

  return sampleCount;
}

AL_TO_IP Get8BitsConversionFunction(int iPicFmt)
{
  auto const AL_CHROMA_MONO_8bitTo8bit = 0x00080800;
  auto const AL_CHROMA_MONO_8bitTo10bit = 0x000A0800;

  auto const AL_CHROMA_420_8bitTo8bit = 0x00080801;
  auto const AL_CHROMA_420_8bitTo10bit = 0x000A0801;

  auto const AL_CHROMA_422_8bitTo8bit = 0x00080802;
  auto const AL_CHROMA_422_8bitTo10bit = 0x000A0802;
  switch(iPicFmt)
  {
  case AL_CHROMA_420_8bitTo8bit:
    return NV12_To_I420;
  case AL_CHROMA_420_8bitTo10bit:
    return NV12_To_I0AL;
  case AL_CHROMA_422_8bitTo8bit:
    return NV16_To_I422;
  case AL_CHROMA_422_8bitTo10bit:
    return NV16_To_I2AL;
  case AL_CHROMA_MONO_8bitTo8bit:
    return Y800_To_Y800;
  case AL_CHROMA_MONO_8bitTo10bit:
    return Y800_To_Y010;
  default:
    assert(0);
    return nullptr;
  }
}

AL_TO_IP Get10BitsConversionFunction(int iPicFmt)
{
  auto const AL_CHROMA_MONO_10bitTo10bit = 0x000A0A00;
  auto const AL_CHROMA_MONO_10bitTo8bit = 0x00080A00;

  auto const AL_CHROMA_420_10bitTo10bit = 0x000A0A01;
  auto const AL_CHROMA_420_10bitTo8bit = 0x00080A01;

  auto const AL_CHROMA_422_10bitTo10bit = 0x000A0A02;
  auto const AL_CHROMA_422_10bitTo8bit = 0x00080A02;
  switch(iPicFmt)
  {
  case AL_CHROMA_420_10bitTo10bit:
    return XV15_To_I0AL;
  case AL_CHROMA_420_10bitTo8bit:
    return XV15_To_I420;
  case AL_CHROMA_422_10bitTo10bit:
    return XV20_To_I2AL;
  case AL_CHROMA_422_10bitTo8bit:
    return XV20_To_I422;
  case AL_CHROMA_MONO_10bitTo10bit:
    return XV15_To_Y010;
  case AL_CHROMA_MONO_10bitTo8bit:
    return XV15_To_Y800;
  default:
    assert(0);
    return nullptr;
  }
}

AL_TO_IP GetTileConversionFunction(int iPicFmt)
{
  auto const AL_CHROMA_MONO_8bitTo8bit = 0x00080800;
  auto const AL_CHROMA_MONO_8bitTo10bit = 0x000A0800;

  auto const AL_CHROMA_420_8bitTo8bit = 0x00080801;
  auto const AL_CHROMA_420_8bitTo10bit = 0x000A0801;

  auto const AL_CHROMA_422_8bitTo8bit = 0x00080802;
  auto const AL_CHROMA_422_8bitTo10bit = 0x000A0802;

  auto const AL_CHROMA_MONO_10bitTo10bit = 0x000A0A00;
  auto const AL_CHROMA_MONO_10bitTo8bit = 0x00080A00;

  auto const AL_CHROMA_420_10bitTo10bit = 0x000A0A01;
  auto const AL_CHROMA_420_10bitTo8bit = 0x00080A01;

  auto const AL_CHROMA_422_10bitTo10bit = 0x000A0A02;
  auto const AL_CHROMA_422_10bitTo8bit = 0x00080A02;
  switch(iPicFmt)
  {
  case AL_CHROMA_420_8bitTo8bit:
    return T608_To_I420;
  case AL_CHROMA_420_8bitTo10bit:
    return T608_To_I0AL;
  case AL_CHROMA_422_8bitTo8bit:
    return T628_To_I422;
  case AL_CHROMA_422_8bitTo10bit:
    return T628_To_I2AL;
  case AL_CHROMA_MONO_8bitTo8bit:
    return T608_To_Y800;
  case AL_CHROMA_MONO_8bitTo10bit:
    return T608_To_Y010;

  case AL_CHROMA_420_10bitTo10bit:
    return T60A_To_I0AL;
  case AL_CHROMA_420_10bitTo8bit:
    return T60A_To_I420;
  case AL_CHROMA_422_10bitTo10bit:
    return T62A_To_I2AL;
  case AL_CHROMA_422_10bitTo8bit:
    return T62A_To_I422;
  case AL_CHROMA_MONO_10bitTo10bit:
    return T60A_To_Y010;
  case AL_CHROMA_MONO_10bitTo8bit:
    return T60A_To_Y800;
  default:
    assert(0);
    return nullptr;
  }
}

AL_TO_IP GetConversionFunction(TFourCC input, int iBdOut)
{
  auto const eChromaMode = AL_GetChromaMode(input);
  auto const iBdIn = AL_GetBitDepth(input);

#define GetConvFormat(ChromaMode, iBdIn, iBdOut) ((ChromaMode) | ((iBdIn) << 8) | ((iBdOut) << 16))

  int iPicFmt = GetConvFormat(eChromaMode, iBdIn, iBdOut);

  if(AL_IsTiled(input))
    return GetTileConversionFunction(iPicFmt);
  else if(iBdIn == 8)
    return Get8BitsConversionFunction(iPicFmt);
  else
    return Get10BitsConversionFunction(iPicFmt);
}

static void ConvertFrameBuffer(AL_TBuffer& input, int iBdIn, AL_TBuffer& output, int iBdOut)
{
  auto pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&input, AL_META_TYPE_SOURCE);
  AL_TPicFormat tPicFormat = AL_GetDecPicFormat(AL_GetChromaMode(pRecMeta->tFourCC), iBdIn, AL_GetStorageMode(pRecMeta->tFourCC), AL_IsCompressed(pRecMeta->tFourCC));

  pRecMeta->tFourCC = AL_GetDecFourCC(tPicFormat);
  auto const iSizePix = (iBdOut + 7) >> 3;
  uint32_t uSize = GetPictureSizeInSamples(pRecMeta) * iSizePix;

  if(uSize != output.zSize)
  {
    AL_Allocator_Free(output.pAllocator, output.hBuf);
    output.hBuf = AL_Allocator_Alloc(output.pAllocator, uSize);
    AL_Buffer_SetData(&output, AL_Allocator_GetVirtualAddr(output.pAllocator, output.hBuf));
    output.zSize = uSize;
  }

  auto pYuvMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&output, AL_META_TYPE_SOURCE);
  pYuvMeta->tDim.iWidth = pRecMeta->tDim.iWidth;
  pYuvMeta->tDim.iHeight = pRecMeta->tDim.iHeight;
  pYuvMeta->tPlanes[AL_PLANE_Y].iPitch = iSizePix * pRecMeta->tDim.iWidth;
  pYuvMeta->tPlanes[AL_PLANE_UV].iPitch = iSizePix * ((tPicFormat.eChromaMode == AL_CHROMA_4_4_4) ? pRecMeta->tDim.iWidth : pRecMeta->tDim.iWidth >> 1);
  pYuvMeta->tPlanes[AL_PLANE_Y].iOffset = 0;
  pYuvMeta->tPlanes[AL_PLANE_UV].iOffset = pYuvMeta->tPlanes[AL_PLANE_Y].iPitch * pYuvMeta->tDim.iHeight;

  auto AllegroConvert = GetConversionFunction(pRecMeta->tFourCC, iBdOut);
  AllegroConvert(&input, &output);
}

/******************************************************************************/
class BaseOutputWriter
{
public:
  BaseOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName);
  virtual ~BaseOutputWriter() {};

  void ProcessOutput(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut);
  virtual void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut) = 0;

protected:
  ofstream YuvFile;
  ofstream IpCrcFile;
};

BaseOutputWriter::BaseOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName)
{
  if(!sYuvFileName.empty())
  {
    OpenOutput(YuvFile, sYuvFileName);
  }

  if(!sIPCrcFileName.empty())
  {
    OpenOutput(IpCrcFile, sIPCrcFileName, false);
    IpCrcFile << hex << uppercase;
  }
}

void BaseOutputWriter::ProcessOutput(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut)
{
  if(IpCrcFile.is_open())
    IpCrcFile << std::setfill('0') << std::setw(8) << (int)info.uCRC << std::endl;

  ProcessFrame(tRecBuf, info, iBdOut);
}

/******************************************************************************/
class UncompressedOutputWriter : public BaseOutputWriter
{
public:
  ~UncompressedOutputWriter();

  UncompressedOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName);
  void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut) override;

private:
  ofstream CertCrcFile; // Cert crc only computed for uncompressed output
  AL_TBuffer* YuvBuffer = NULL;
};

UncompressedOutputWriter::~UncompressedOutputWriter()
{
  if(NULL != YuvBuffer)
  {
    AL_Buffer_Destroy(YuvBuffer);
  }
}

UncompressedOutputWriter::UncompressedOutputWriter(const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName) :
  BaseOutputWriter(sYuvFileName, sIPCrcFileName)
{
  if(!sCertCrcFileName.empty())
  {
    OpenOutput(CertCrcFile, sCertCrcFileName, false);
    CertCrcFile << hex << uppercase;
  }

  // Conversion buffer allocation
  AL_TPlane tPlane {};
  AL_TDimension tDimension {};
  AL_TMetaData* Meta = (AL_TMetaData*)AL_SrcMetaData_Create(tDimension, tPlane, tPlane, 0);
  YuvBuffer = AL_Buffer_Create_And_Allocate(AL_GetDefaultAllocator(), 100, NULL);

  if(!YuvBuffer)
    throw runtime_error("Couldn't allocate YuvBuffer");
  AL_Buffer_AddMetaData(YuvBuffer, Meta);
}

void UncompressedOutputWriter::ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut)
{
  auto pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&tRecBuf, AL_META_TYPE_SOURCE);
  auto pYuvMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(YuvBuffer, AL_META_TYPE_SOURCE);

  if(YuvFile.is_open() || CertCrcFile.is_open())
  {
    int iBdIn = max(info.uBitDepthY, info.uBitDepthC);

    if(iBdIn > 8)
      iBdIn = 10;

    if(iBdOut > 8)
      iBdOut = 10;

    auto const iSizePix = (iBdOut + 7) >> 3;

    ConvertFrameBuffer(tRecBuf, iBdIn, *YuvBuffer, iBdOut);

    if(info.tCrop.bCropping)
      CropFrame(YuvBuffer, iSizePix, info.tCrop.uCropOffsetLeft, info.tCrop.uCropOffsetRight, info.tCrop.uCropOffsetTop, info.tCrop.uCropOffsetBottom);

    if(CertCrcFile.is_open())
    {
      // compute crc
      int sx = 1, sy = 1;
      AL_GetSubsampling(pRecMeta->tFourCC, &sx, &sy);
      int const iNumPix = pYuvMeta->tDim.iHeight * pYuvMeta->tDim.iWidth;
      int const iNumPixC = iNumPix / sx / sy;
      auto eChromaMode = AL_GetChromaMode(pRecMeta->tFourCC);

      if(iBdOut == 8)
      {
        uint8_t* pBuf = AL_Buffer_GetData(YuvBuffer);
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, CertCrcFile);
      }
      else
      {
        uint16_t* pBuf = (uint16_t*)AL_Buffer_GetData(YuvBuffer);
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, CertCrcFile);
      }
    }

    if(YuvFile.is_open())
    {
      auto uSize = GetPictureSizeInSamples(pYuvMeta) * iSizePix;
      YuvFile.write((const char*)AL_Buffer_GetData(YuvBuffer), uSize);
    }
  }
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

  void AddOutputWriter(AL_e_FbStorageMode eFbStorageMode, bool bCompressionEnabled, const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, bool bIsAVC);

  void Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo);
  void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut);

  AL_HDecoder hDec = NULL;
  AL_EVENT hExitMain = NULL;
  std::map<AL_EFbStorageMode, std::shared_ptr<BaseOutputWriter>> writers;
  int iBitDepth = 8;
  unsigned int NumFrames = 0;
  unsigned int MaxFrames = UINT_MAX;
  unsigned int FirstFrame = 0;
  mutex hMutex;
  int iNumFrameConceal = 0;
};

struct ResChgParam
{
  AL_HDecoder hDec;
  bool bPoolIsInit;
  BufPool bufPool;
  AL_TDecSettings* pDecSettings;
  AL_TAllocator* pAllocator;
  AL_TDecSettings* pSettings;
  mutex hMutex;
};

struct DecodeParam
{
  AL_HDecoder hDec;
  AL_EVENT hExitMain = NULL;
  atomic<int> decodedFrames;
};

/******************************************************************************/
void Display::AddOutputWriter(AL_e_FbStorageMode eFbStorageMode, bool bCompressionEnabled, const string& sYuvFileName, const string& sIPCrcFileName, const string& sCertCrcFileName, bool bIsAVC)
{
  (void)bIsAVC;
  (void)bCompressionEnabled;
  {
    writers[eFbStorageMode] = std::shared_ptr<BaseOutputWriter>(new UncompressedOutputWriter(sYuvFileName, sIPCrcFileName, sCertCrcFileName));
  }
}

/******************************************************************************/
static void sFrameDecoded(AL_TBuffer* pDecodedFrame, void* pUserParam)
{
  auto pParam = reinterpret_cast<DecodeParam*>(pUserParam);

  if(!pDecodedFrame)
  {
    Rtos_SetEvent(pParam->hExitMain);
  }
  pParam->decodedFrames++;
};

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
  pDisplay->Process(pFrame, pInfo);
}

void Display::Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  unique_lock<mutex> lock(hMutex);

  AL_ERR err = AL_Decoder_GetFrameError(hDec, pFrame);
  bool bExitError = AL_IS_ERROR_CODE(err);

  if(bExitError || isEOS(pFrame, pInfo))
  {
    if(err == AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS)
      Message(CC_GREY, "\nDecoder has discarded some SPS not compatible with the channel settings\n");

    if(bExitError)
      Message(CC_RED, "Error: %d", err);
    else
      Message(CC_GREY, "Complete");
    Rtos_SetEvent(hExitMain);
    return;
  }

  if(err == AL_WARN_CONCEAL_DETECT)
    iNumFrameConceal++;

  if(isReleaseFrame(pFrame, pInfo))
    return;

  if(iBitDepth == 0)
    iBitDepth = max(pInfo->uBitDepthY, pInfo->uBitDepthC);
  else if(iBitDepth == -1)
    iBitDepth = AL_Decoder_GetMaxBD(hDec);

  assert(AL_Buffer_GetData(pFrame));

  ProcessFrame(*pFrame, *pInfo, iBitDepth);

  bool shouldDisplayPicture = true;

  if(shouldDisplayPicture)
  {
    AL_Decoder_PutDisplayPicture(hDec, pFrame);

    // TODO: increase only when last frame
    DisplayFrameStatus(NumFrames);
    NumFrames++;

    if(NumFrames > MaxFrames)
      Rtos_SetEvent(hExitMain);
  }
}

/******************************************************************************/
void Display::ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut)
{
  if(writers.find(info.eFbStorageMode) != writers.end() && (NumFrames >= FirstFrame))
    writers[info.eFbStorageMode]->ProcessOutput(tRecBuf, info, iBdOut);
}

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
};

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
  ss << "Profile: " << pSettings->iProfileIdc << endl;
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

  Message(CC_DARK_BLUE, "%s\n", ss.str().c_str());
}

void printHexdump(ostream* logger, uint8_t* data, int size)
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

static void sParsedSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, void* pUserParam)
{
  auto seiOutput = static_cast<ostream*>(pUserParam);

  if(!seiOutput)
    return;
  *seiOutput << "is_prefix: " << boolalpha << bIsPrefix << endl
             << "sei_payload_type: " << iPayloadType << endl
             << "sei_payload_size: " << iPayloadSize << endl
             << "raw:" << endl;
  printHexdump(seiOutput, pPayload, iPayloadSize);
  *seiOutput << endl << endl;
}

static AL_ERR sResolutionFound(int BufferNumber, int BufferSizeLib, AL_TStreamSettings const* pSettings, AL_TCropInfo const* pCropInfo, void* pUserParam)
{
  ResChgParam* p = (ResChgParam*)pUserParam;
  AL_TDecSettings* pDecSettings = p->pDecSettings;

  unique_lock<mutex> lock(p->hMutex);

  if(!p->hDec)
    return AL_ERROR;

  bool bMainOutputCompression;
  AL_e_FbStorageMode eMainOutputStorageMode = getMainOutputStorageMode(*pDecSettings, bMainOutputCompression);

  auto tPicFormat = AL_GetDecPicFormat(pSettings->eChroma, pSettings->iBitDepth, eMainOutputStorageMode, bMainOutputCompression);
  auto tFourCC = AL_GetDecFourCC(tPicFormat);

  int minPitch = AL_Decoder_GetMinPitch(pSettings->tDim.iWidth, pSettings->iBitDepth, eMainOutputStorageMode);
  int BufferSize = AL_DecGetAllocSize_Frame(pSettings->tDim, minPitch, pSettings->eChroma, bMainOutputCompression, eMainOutputStorageMode);
  assert(BufferSize >= BufferSizeLib);

  showStreamInfo(BufferNumber, BufferSize, pSettings, pCropInfo, tFourCC);

  /* stream resolution change */
  if(p->bPoolIsInit)
    return AL_SUCCESS;

  AL_TBufPoolConfig BufPoolConfig;
  BufPoolConfig.zBufSize = BufferSize;
  BufPoolConfig.uNumBuf = BufferNumber + uDefaultNumBuffersHeldByNextComponent;
  BufPoolConfig.debugName = "yuv";

  auto pMeta = AL_CreateRecBufMetaData(pSettings->tDim, minPitch, tFourCC);
  BufPoolConfig.pMetaData = pMeta;

  if(!p->bufPool.Init(p->pAllocator, BufPoolConfig))
    return AL_ERR_NO_MEMORY;

  p->bPoolIsInit = true;

  for(int i = 0; i < (int)BufPoolConfig.uNumBuf; ++i)
  {
    auto pDecPict = p->bufPool.GetBuffer(AL_BUF_MODE_NONBLOCK);
    assert(pDecPict);
    Rtos_Memset(AL_Buffer_GetData(pDecPict), 0xDE, pDecPict->zSize);
    AL_Decoder_PutDisplayPicture(p->hDec, pDecPict);
    AL_Buffer_Unref(pDecPict);
  }

  return AL_SUCCESS;
}

/******************************************************************************/
static uint32_t ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream)
{
  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);

  ifFileStream.read((char*)pBuf, pBufStream->zSize);
  return (uint32_t)ifFileStream.gcount();
}

/******************************************************************************/


void ShowStatistics(double durationInSeconds, int iNumFrameConceal, int decodedFrameNumber, bool timeoutOccured)
{
  string guard = "Decoded time = ";

  if(timeoutOccured)
    guard = "TIMEOUT = ";

  auto msg = "\n\n" + guard + "%.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n";
  Message(CC_DEFAULT, msg.c_str(),
          durationInSeconds,
          decodedFrameNumber / durationInSeconds,
          iNumFrameConceal);
}

/******************************************************************************/
struct AsyncFileInput
{
  AsyncFileInput(AL_HDecoder hDec_, string path, BufPool& bufPool_)
    : hDec(hDec_), bufPool(bufPool_)
  {
    exit = false;
    OpenInput(ifFileStream, path);
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

      auto uAvailSize = ReadStream(ifFileStream, pBufStream.get());

      if(!uAvailSize)
      {
        // end of input
        AL_Decoder_Flush(hDec);
        break;
      }

      auto bRet = AL_Decoder_PushBuffer(hDec, pBufStream.get(), uAvailSize);

      if(!bRet)
        throw runtime_error("Failed to push buffer");
    }
  }

  const AL_HDecoder hDec;
  ifstream ifFileStream;
  BufPool& bufPool;
  atomic<bool> exit;
  thread m_thread;
};

/******************************************************************************/
void SafeMain(int argc, char** argv)
{
  auto const Config = ParseCommandLine(argc, argv);

  if(Config.help)
    return;

  DisplayVersionInfo();

  ofstream seiOutput;

  if(!Config.seiFile.empty())
    OpenOutput(seiOutput, Config.seiFile);

  // IP Device ------------------------------------------------------------
  auto iUseBoard = Config.iUseBoard;


  function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl;
  switch(Config.ipCtrlMode)
  {
  default:
    wrapIpCtrl = [](AL_TIpCtrl* ipCtrl) -> AL_TIpCtrl*
                 {
                   return ipCtrl;
                 };
    break;
  }

  auto pIpDevice = CreateIpDevice(&iUseBoard, Config.iSchedulerType, wrapIpCtrl, Config.trackDma, Config.tDecSettings.uNumCore, Config.hangers);


  auto pAllocator = pIpDevice->m_pAllocator.get();
  auto pDecChannel = pIpDevice->m_pDecChannel;

  BufPool bufPool;

  {
    AL_TBufPoolConfig BufPoolConfig {};

    BufPoolConfig.zBufSize = Config.zInputBufferSize;
    BufPoolConfig.uNumBuf = Config.uInputBufferNum;
    BufPoolConfig.pMetaData = nullptr;
    BufPoolConfig.debugName = "stream";

    auto ret = bufPool.Init(AL_GetDefaultAllocator(), BufPoolConfig);

    if(!ret)
      throw runtime_error("Can't create BufPool");
  }

  Display display;

  bool bHasOutput = Config.bEnableYUVOutput || bCertCRC || !Config.sCrc.empty();

  if(bHasOutput)
  {
    const bool bIsAVC = AL_CODEC_AVC == Config.tDecSettings.eCodec;
    const string sCertCrcFile = bCertCRC ? "crc_certif_res.hex" : "";

    bool bMainOutputCompression;
    AL_e_FbStorageMode eMainOutputStorageMode = getMainOutputStorageMode(Config.tDecSettings, bMainOutputCompression);

    display.AddOutputWriter(eMainOutputStorageMode, bMainOutputCompression, Config.sMainOut, Config.sCrc, sCertCrcFile, bIsAVC);

  }

  display.iBitDepth = Config.tDecSettings.iBitDepth;
  display.MaxFrames = Config.iMaxFrames;

  AL_TDecSettings Settings = Config.tDecSettings;

  ResChgParam ResolutionFoundParam;
  ResolutionFoundParam.pAllocator = pAllocator;
  ResolutionFoundParam.bPoolIsInit = false;
  ResolutionFoundParam.pDecSettings = &Settings;

  DecodeParam tDecodeParam {};
  tDecodeParam.hExitMain = display.hExitMain;

  AL_TDecCallBacks CB {};
  CB.endDecodingCB = { &sFrameDecoded, &tDecodeParam };
  CB.displayCB = { &sFrameDisplay, &display };
  CB.resolutionFoundCB = { &sResolutionFound, &ResolutionFoundParam };
  CB.parsedSeiCB = { &sParsedSei, (void*)&seiOutput };

  Settings.iBitDepth = HW_IP_BIT_DEPTH;

  AL_HDecoder hDec;
  auto error = AL_Decoder_Create(&hDec, (AL_TIDecChannel*)pDecChannel, pAllocator, &Settings, &CB);

  if(AL_IS_ERROR_CODE(error))
    throw codec_error(error);

  assert(hDec);

  auto decoderAlreadyDestroyed = false;
  auto scopeDecoder = scopeExit([&]() {
    if(!decoderAlreadyDestroyed)
      AL_Decoder_Destroy(hDec);
  });

  // Param of Display Callback assignment
  display.hDec = hDec;
  tDecodeParam.hDec = hDec;
  ResolutionFoundParam.hDec = hDec;

  AL_Decoder_SetParam(hDec, Config.bConceal, iUseBoard ? true : false, Config.iNumTrace, Config.iNumberTrace, Config.bForceCleanBuffers);

  if(!invalidPreallocSettings(Config.tDecSettings.tStream))
  {
    if(!AL_Decoder_PreallocateBuffers(hDec))
      if(auto eErr = AL_Decoder_GetLastError(hDec))
        throw codec_error(eErr);
  }

  // Initial stream buffer filling
  auto const uBegin = GetPerfTime();
  bool timeoutOccured = false;

  for(int iLoop = 0; iLoop < Config.iLoop; ++iLoop)
  {
    if(iLoop > 0)
      Message(CC_GREY, "  Looping\n");

    AsyncFileInput producer(hDec, Config.sIn, bufPool);

    auto const maxWait = Config.iTimeoutInSeconds * 1000;
    auto const timeout = maxWait >= 0 ? maxWait : AL_WAIT_FOREVER;

    if(!Rtos_WaitEvent(display.hExitMain, timeout))
      timeoutOccured = true;
    bufPool.Decommit();
  }

  auto const uEnd = GetPerfTime();

  unique_lock<mutex> lock(display.hMutex);
  auto eErr = AL_Decoder_GetLastError(hDec);

  if(AL_IS_ERROR_CODE(eErr))
    throw codec_error(eErr);

  if(!tDecodeParam.decodedFrames)
    throw runtime_error("No frame decoded");

  auto const duration = (uEnd - uBegin) / 1000.0;
  ShowStatistics(duration, display.iNumFrameConceal, tDecodeParam.decodedFrames, timeoutOccured);
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

