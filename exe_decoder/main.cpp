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

#include <cassert>
#include <cstdarg>
#include <stdlib.h>
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
#include "lib_app/BufPool.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common/FourCC.h"
#include "lib_common/StreamBuffer.h"
}

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

#ifndef HW_IP_BIT_DEPTH
#define HW_IP_BIT_DEPTH 10
#endif

using namespace std;

const char* ToString(AL_ERR eErrCode)
{
  switch(eErrCode)
  {
  case AL_ERR_INIT_FAILED: return "Failed to initialize the decoder due to parameters inconsistency";
  case AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE: return "Channel not created, no channel available";
  case AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE: return "Channel not created, processing power of the available cores insufficient";
  case AL_ERR_CHAN_CREATION_NOT_ENOUGH_CORES: return "Channel not created, couldn't spread the load on enough cores";
  case AL_ERR_REQUEST_MALFORMED: return "Channel not created: request was malformed";
  case AL_ERR_NO_FRAME_DECODED: return "No frame decoded";
  case AL_ERR_RESOLUTION_CHANGE: return "Resolution Change is not supported";
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

/******************************************************************************/

static bool bCertCRC = false;

AL_TDecSettings getDefaultDecSettings()
{
  AL_TDecSettings settings {};

  settings.iStackSize = 2;
  settings.iBitDepth = -1;
  settings.uNumCore = 0; // AUTO
  settings.uFrameRate = 60000;
  settings.uClkRatio = 1000;
  settings.uDDRWidth = 32;
  settings.eDecUnit = AL_AU_UNIT;
  settings.eDpbMode = AL_DPB_NORMAL;
  settings.eFBStorageMode = AL_FB_RASTER;
  settings.tStream.tDim = { -1, -1 };
  settings.tStream.eChroma = CHROMA_MAX_ENUM;
  settings.tStream.iBitDepth = -1;
  settings.tStream.iProfileIdc = -1;

  return settings;
}

static int const zDefaultInputBufferSize = 32 * 1024;

struct Config
{
  bool help = false;

  string sIn;
  string sOut;
  string sCrc;

  AL_TDecSettings tDecSettings = getDefaultDecSettings();
  int iUseBoard = 1; // board
  SCHEDULER_TYPE iSchedulerType = SCHEDULER_TYPE_MCU;
  int iNumTrace = -1;
  int iNumberTrace = 0;
  bool bConceal = false;
  bool bEnableYUVOutput = true;
  unsigned int uInputBufferNum = 2;
  size_t zInputBufferSize = zDefaultInputBufferSize;
  IpCtrlMode ipCtrlMode = IPCTRL_MODE_STANDARD;
  string logsFile = "";
  bool trackDma = false;
  int hangers = 0;
  int iLoop = 1;
  int iTimeOutInSeconds = 0;
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

void getExpectedSeparator(stringstream& ss, char expectedSep)
{
  char sep;
  ss >> sep;

  if(sep != expectedSep)
    throw runtime_error("wrong prealloc arguments format");
}

bool invalidPreallocSettings(AL_TStreamSettings const& settings)
{
  return settings.iProfileIdc <= 0 || settings.iLevel <= 0
         || settings.tDim.iWidth <= 0 || settings.tDim.iHeight <= 0 || settings.eChroma == CHROMA_MAX_ENUM;
}

void parsePreAllocArgs(AL_TStreamSettings* settings, string& toParse)
{
  stringstream ss(toParse);
  ss.unsetf(ios::dec);
  ss.unsetf(ios::hex);
  char chroma[4] {};
  ss >> settings->tDim.iWidth;
  getExpectedSeparator(ss, 'x');
  ss >> settings->tDim.iHeight;
  getExpectedSeparator(ss, ':');
  ss >> chroma[0];
  ss >> chroma[1];
  ss >> chroma[2];
  getExpectedSeparator(ss, ':');
  ss >> settings->iBitDepth;
  getExpectedSeparator(ss, ':');
  ss >> settings->iLevel;
  getExpectedSeparator(ss, ':');
  ss >> settings->iProfileIdc;

  if(string(chroma) == "400")
    settings->eChroma = CHROMA_4_0_0;
  else if(string(chroma) == "420")
    settings->eChroma = CHROMA_4_2_0;
  else if(string(chroma) == "422")
    settings->eChroma = CHROMA_4_2_2;
  else if(string(chroma) == "444")
    settings->eChroma = CHROMA_4_4_4;
  else
    throw runtime_error("wrong prealloc chroma format");

  if(ss.fail() || ss.tellg() != streampos(-1))
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

  auto opt = CommandLineParser();

  opt.addFlag("--help,-h", &Config.help, "Shows this help");
  opt.addString("-in,-i", &Config.sIn, "Input bitstream");
  opt.addString("-out,-o", &Config.sOut, "Output YUV");
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
  opt.addInt("-ddrwidth", &Config.tDecSettings.uDDRWidth, "Width of DDR requests (16, 32, 64) (default: 32)");
  opt.addFlag("-nocache", &Config.tDecSettings.bDisableCache, "Inactivate the cache");
  opt.addOption("--fbc", [&]()
  {
    Config.tDecSettings.eFBStorageMode = AL_FB_TILE_32x4;
    Config.tDecSettings.bFrameBufferCompression = true;
  }, "Enables internal frame buffer compression");
  opt.addOption("--raster", [&]()
  {
    Config.tDecSettings.eFBStorageMode = AL_FB_RASTER;
    Config.tDecSettings.bFrameBufferCompression = false;
  }, "Store frame buffers in raster format");
  opt.addOption("--tile", [&]()
  {
    Config.tDecSettings.eFBStorageMode = AL_FB_TILE_32x4;
  }, "Store frame buffers in tiles format");


  opt.addOption("-t", [&]()
  {
    Config.iNumTrace = opt.popInt();
    Config.iNumberTrace = 1;
  }, "First frame to trace (optional)");

  opt.addCustom("-clk", &Config.tDecSettings.uClkRatio, &IntWithOffset<1000>, "Set clock ratio");

  opt.addFlag("-lowref", &Config.tDecSettings.eDpbMode,
              "Specify decoder DPB Low ref (stream musn't have B-frame & reference must be at best 1",
              AL_DPB_LOW_REF);

  opt.addFlag("-avc", &Config.tDecSettings.bIsAvc,
              "Specify the input bitstream codec (default: HEVC)",
              true);

  opt.addFlag("-hevc", &Config.tDecSettings.bIsAvc,
              "Specify the input bitstream codec (default: HEVC)",
              false);

  opt.addFlag("-noyuv", &Config.bEnableYUVOutput,
              "Disable writing output YUV file",
              false);

  opt.addInt("-loop", &Config.iLoop, "Number of Decoding loop (optional)");

  opt.addString("--log", &Config.logsFile, "A file where logged events will be dumped");


  string preAllocArgs = "";
  opt.addInt("--timeout", &Config.iTimeOutInSeconds, "Specify timeout in seconds");
  opt.addString("--prealloc-args", &preAllocArgs, "Specify the stream dimension: 1920x1080:422:10:level:profile-idc");

  opt.parse(argc, argv);

  if(Config.help)
  {
    Usage(opt, argv[0]);
    return Config;
  }

  if(Config.tDecSettings.bFrameBufferCompression)
  {
    if(!Config.sOut.empty())
      throw runtime_error("Output unavaible with fbc");
    Config.bEnableYUVOutput = false;
  }

  if(Config.sOut.empty())
    Config.sOut = "dec.yuv";

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
    Config.zInputBufferSize = (!preAllocArgs.empty() && Config.zInputBufferSize == zDefaultInputBufferSize) ? AL_GetMaxNalSize(Config.tDecSettings.tStream.tDim, Config.tDecSettings.tStream.eChroma) : Config.zInputBufferSize;
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

  if(AL_GetChromaMode(meta->tFourCC) != CHROMA_MONO)
    sampleCount += ((sampleCount * 2) / (sx * sy));

  return sampleCount;
}

static TFourCC GetInternalFourCC(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eFBStorageMode)
{
  if(eFBStorageMode == AL_FB_RASTER)
  {
    if(uBitDepth == 8)
    {
      switch(eChromaMode)
      {
      case CHROMA_4_2_0: return FOURCC(I420);
      case CHROMA_4_2_2: return FOURCC(I422);
      case CHROMA_MONO: return FOURCC(Y800);
      default: assert(0);
      }
    }
    else
    {
      switch(eChromaMode)
      {
      case CHROMA_4_2_0: return FOURCC(I0AL);
      case CHROMA_4_2_2: return FOURCC(I2AL);
      case CHROMA_MONO: return FOURCC(Y010);
      default: assert(0);
      }
    }
  }
  else
  {
    if(uBitDepth == 8)
    {
      switch(eChromaMode)
      {
      case CHROMA_4_2_0: return FOURCC(T608);
      case CHROMA_4_2_2: return FOURCC(T628);
      case CHROMA_MONO: return FOURCC(T608);
      default: assert(0);
      }
    }
    else
    {
      switch(eChromaMode)
      {
      case CHROMA_4_2_0: return FOURCC(T60A);
      case CHROMA_4_2_2: return FOURCC(T62A);
      case CHROMA_MONO: return FOURCC(T60A);
      default: assert(0);
      }
    }
  }
  assert(0);
  return FOURCC(I420);
}

AL_TO_IP Get8BitsConversionFunction(int iPicFmt)
{
  auto const CHROMA_MONO_8bitTo8bit = 0x00080800;
  auto const CHROMA_MONO_8bitTo10bit = 0x000A0800;

  auto const CHROMA_420_8bitTo8bit = 0x00080801;
  auto const CHROMA_420_8bitTo10bit = 0x000A0801;

  auto const CHROMA_422_8bitTo8bit = 0x00080802;
  auto const CHROMA_422_8bitTo10bit = 0x000A0802;
  switch(iPicFmt)
  {
  case CHROMA_420_8bitTo8bit:
    return NV12_To_I420;
  case CHROMA_420_8bitTo10bit:
    return NV12_To_I0AL;
  case CHROMA_422_8bitTo8bit:
    return NV16_To_I422;
  case CHROMA_422_8bitTo10bit:
    return NV16_To_I2AL;
  case CHROMA_MONO_8bitTo8bit:
    return Y800_To_Y800;
  case CHROMA_MONO_8bitTo10bit:
    return Y800_To_Y010;
  default:
    assert(0);
    return nullptr;
  }
}

AL_TO_IP Get10BitsConversionFunction(int iPicFmt)
{
  auto const CHROMA_MONO_10bitTo10bit = 0x000A0A00;
  auto const CHROMA_MONO_10bitTo8bit = 0x00080A00;

  auto const CHROMA_420_10bitTo10bit = 0x000A0A01;
  auto const CHROMA_420_10bitTo8bit = 0x00080A01;

  auto const CHROMA_422_10bitTo10bit = 0x000A0A02;
  auto const CHROMA_422_10bitTo8bit = 0x00080A02;

  bool bPlanar = false;
  switch(iPicFmt)
  {
  case CHROMA_420_10bitTo10bit:
    return bPlanar ? P010_To_I0AL : RX0A_To_I0AL;
  case CHROMA_420_10bitTo8bit:
    return bPlanar ? P010_To_I420 : RX0A_To_I420;
  case CHROMA_422_10bitTo10bit:
    return bPlanar ? P210_To_I2AL : RX2A_To_I2AL;
  case CHROMA_422_10bitTo8bit:
    return bPlanar ? P210_To_I422 : RX2A_To_I422;
  case CHROMA_MONO_10bitTo10bit:
    return bPlanar ? P010_To_Y010 : RX0A_To_Y010;
  case CHROMA_MONO_10bitTo8bit:
    return bPlanar ? P010_To_Y800 : RX0A_To_Y800;
  default:
    assert(0);
    return nullptr;
  }
}

AL_TO_IP GetTileConversionFunction(int iPicFmt)
{
  auto const CHROMA_MONO_8bitTo8bit = 0x00080800;
  auto const CHROMA_MONO_8bitTo10bit = 0x000A0800;

  auto const CHROMA_420_8bitTo8bit = 0x00080801;
  auto const CHROMA_420_8bitTo10bit = 0x000A0801;

  auto const CHROMA_422_8bitTo8bit = 0x00080802;
  auto const CHROMA_422_8bitTo10bit = 0x000A0802;

  auto const CHROMA_MONO_10bitTo10bit = 0x000A0A00;
  auto const CHROMA_MONO_10bitTo8bit = 0x00080A00;

  auto const CHROMA_420_10bitTo10bit = 0x000A0A01;
  auto const CHROMA_420_10bitTo8bit = 0x00080A01;

  auto const CHROMA_422_10bitTo10bit = 0x000A0A02;
  auto const CHROMA_422_10bitTo8bit = 0x00080A02;
  switch(iPicFmt)
  {
  case CHROMA_420_8bitTo8bit:
    return T608_To_I420;
  case CHROMA_420_8bitTo10bit:
    return T608_To_I0AL;
  case CHROMA_422_8bitTo8bit:
    return T628_To_I422;
  case CHROMA_422_8bitTo10bit:
    return T628_To_I2AL;
  case CHROMA_MONO_8bitTo8bit:
    return T608_To_Y800;
  case CHROMA_MONO_8bitTo10bit:
    return T608_To_Y010;

  case CHROMA_420_10bitTo10bit:
    return T60A_To_I0AL;
  case CHROMA_420_10bitTo8bit:
    return T60A_To_I420;
  case CHROMA_422_10bitTo10bit:
    return T62A_To_I2AL;
  case CHROMA_422_10bitTo8bit:
    return T62A_To_I422;
  case CHROMA_MONO_10bitTo10bit:
    return T60A_To_Y010;
  case CHROMA_MONO_10bitTo8bit:
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

static void FillInternalOffsets(AL_TSrcMetaData* pMeta, AL_EFbStorageMode eFBStorageMode)
{
  auto iBdIn = AL_GetBitDepth(pMeta->tFourCC);

  pMeta->tOffsetYC.iLuma = 0;
  AL_TDimension tDim = pMeta->tDim;
  pMeta->tOffsetYC.iChroma = AL_GetAllocSize_DecReference(tDim, CHROMA_MONO, iBdIn, eFBStorageMode);
}

static void ConvertFrameBuffer(AL_TBuffer& input, int iBdIn, AL_TBuffer& output, int iBdOut, AL_EFbStorageMode eFBStorageMode)
{
  auto pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&input, AL_META_TYPE_SOURCE);
  auto eChromaMode = AL_GetChromaMode(pRecMeta->tFourCC);
  pRecMeta->tFourCC = GetInternalFourCC(eChromaMode, iBdIn, eFBStorageMode);
  auto const iSizePix = (iBdOut + 7) >> 3;
  uint32_t uSize = GetPictureSizeInSamples(pRecMeta) * iSizePix;
  FillInternalOffsets(pRecMeta, eFBStorageMode);

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
  pYuvMeta->tPitches.iLuma = iSizePix * pRecMeta->tDim.iWidth;
  pYuvMeta->tPitches.iChroma = iSizePix * ((eChromaMode == CHROMA_4_4_4) ? pRecMeta->tDim.iWidth : pRecMeta->tDim.iWidth >> 1);
  pYuvMeta->tOffsetYC.iLuma = pYuvMeta->tPitches.iLuma / iSizePix * pYuvMeta->tDim.iHeight;
  int iCScale = eChromaMode == CHROMA_4_2_2 ? 2 : eChromaMode == CHROMA_4_2_0 ? 4 : 1;
  pYuvMeta->tOffsetYC.iChroma = pYuvMeta->tOffsetYC.iLuma + pYuvMeta->tOffsetYC.iLuma / iCScale;

  auto AllegroConvert = GetConversionFunction(pRecMeta->tFourCC, iBdOut);
  AllegroConvert(&input, &output);
}

/******************************************************************************/
static void ProcessFrame(AL_TBuffer& tRecBuf, AL_TBuffer& tYuvBuf, AL_TInfoDecode info, int iBdOut, std::ofstream& ofYuvFile, std::ofstream& ofIPCrcFile, std::ofstream& ofCertCrcFile)
{
  auto pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&tRecBuf, AL_META_TYPE_SOURCE);
  auto pYuvMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&tYuvBuf, AL_META_TYPE_SOURCE);

  if(ofIPCrcFile.is_open())
    ofIPCrcFile << std::setfill('0') << std::setw(8) << (int)info.uCRC << std::endl;

  if(ofYuvFile.is_open() || ofCertCrcFile.is_open())
  {
    int iBdIn = max(info.uBitDepthY, info.uBitDepthC);

    if(iBdIn > 8)
      iBdIn = 10;

    if(iBdOut > 8)
      iBdOut = 10;

    auto const iSizePix = (iBdOut + 7) >> 3;

    ConvertFrameBuffer(tRecBuf, iBdIn, tYuvBuf, iBdOut, info.eFbStorageMode);

    if(info.tCrop.bCropping)
      CropFrame(&tYuvBuf, iSizePix, info.tCrop.uCropOffsetLeft, info.tCrop.uCropOffsetRight, info.tCrop.uCropOffsetTop, info.tCrop.uCropOffsetBottom);

    if(ofCertCrcFile.is_open())
    {
      // compute crc
      int sx = 1, sy = 1;
      AL_GetSubsampling(pRecMeta->tFourCC, &sx, &sy);
      int const iNumPix = pYuvMeta->tDim.iHeight * pYuvMeta->tDim.iWidth;
      int const iNumPixC = iNumPix / sx / sy;
      auto eChromaMode = AL_GetChromaMode(pRecMeta->tFourCC);

      if(iBdOut == 8)
      {
        uint8_t* pBuf = AL_Buffer_GetData(&tYuvBuf);
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, ofCertCrcFile);
      }
      else
      {
        uint16_t* pBuf = (uint16_t*)AL_Buffer_GetData(&tYuvBuf);
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, ofCertCrcFile);
      }
    }

    if(ofYuvFile.is_open())
    {
      auto uSize = GetPictureSizeInSamples(pYuvMeta) * iSizePix;
      ofYuvFile.write((const char*)AL_Buffer_GetData(&tYuvBuf), uSize);
    }
  }
}

/******************************************************************************/
struct TCbParam
{
  ~TCbParam()
  {
    Rtos_DeleteEvent(hFinished);
  }

  AL_HDecoder hDec;
  AL_EVENT hFinished;
  ofstream& YuvFile;
  ofstream& IpCrcFile;
  ofstream& CertCrcFile;
  AL_TBuffer* YuvBuffer;
  int iBitDepth;
  AL_UINT num_frame;
  mutex hMutex;
};

struct ResChgParam
{
  AL_HDecoder hDec;
  bool bPoolIsInit;
  BufPool bufPool;
  AL_TAllocator* pAllocator;
  mutex hMutex;
};

struct DecodeParam
{
  AL_HDecoder hDec;
  int decodedFrames;
};

/******************************************************************************/
static void sFrameDecoded(AL_TBuffer* pDecodedFrame, void* pUserParam)
{
  auto pParam = reinterpret_cast<DecodeParam*>(pUserParam);

  if(!pDecodedFrame)
  {
    auto error = codec_error(AL_Decoder_GetLastError(pParam->hDec));
    cerr << endl << "Codec error: " << error.what() << endl;
    exit(error.Code);
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
  auto pParam = reinterpret_cast<TCbParam*>(pUserParam);
  unique_lock<mutex> lck(pParam->hMutex);

  if(isEOS(pFrame, pInfo))
  {
    Message(CC_GREY, "Complete");
    Rtos_SetEvent(pParam->hFinished);
    return;
  }

  if(isReleaseFrame(pFrame, pInfo))
    return;

  if(pParam->iBitDepth == 0)
    pParam->iBitDepth = max(pInfo->uBitDepthY, pInfo->uBitDepthC);
  else if(pParam->iBitDepth == -1)
    pParam->iBitDepth = AL_Decoder_GetMaxBD(pParam->hDec);

  assert(AL_Buffer_GetData(pFrame));

  ProcessFrame(*pFrame, *pParam->YuvBuffer, *pInfo, pParam->iBitDepth, pParam->YuvFile, pParam->IpCrcFile, pParam->CertCrcFile);
  AL_Decoder_PutDisplayPicture(pParam->hDec, pFrame);

  DisplayFrameStatus(pParam->num_frame);
  pParam->num_frame++;
}

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
};

static void sResolutionFound(int BufferNumber, int BufferSize, AL_TDimension tDim, AL_TCropInfo tCropInfo, TFourCC tFourCC, void* pUserParam)
{
  ResChgParam* p = (ResChgParam*)pUserParam;

  unique_lock<mutex> lck(p->hMutex);

  if(!p->hDec)
    return;

  stringstream ss;
  int iWidth = tDim.iWidth;
  int iHeight = tDim.iHeight;
  ss << "Resolution : " << iWidth << "x" << iHeight << endl;
  ss << "FourCC : " << FourCCToString(tFourCC) << endl;

  if(AL_NeedsCropping(&tCropInfo))
  {
    auto uCropWidth = tCropInfo.uCropOffsetLeft + tCropInfo.uCropOffsetRight;
    auto uCropHeight = tCropInfo.uCropOffsetTop + tCropInfo.uCropOffsetBottom;
    ss << "Crop top    : " << tCropInfo.uCropOffsetTop << endl;
    ss << "Crop bottom : " << tCropInfo.uCropOffsetBottom << endl;
    ss << "Crop left   : " << tCropInfo.uCropOffsetLeft << endl;
    ss << "Crop right  : " << tCropInfo.uCropOffsetRight << endl;
    ss << "Display Resolution : " << iWidth - uCropWidth << "x" << iHeight - uCropHeight << endl;
  }
  ss << "Buffer needed : " << BufferNumber << " of size " << BufferSize << endl;

  Message(CC_DARK_BLUE, "%s", ss.str().c_str());

  /* We do not support in stream resolution change */
  if(p->bPoolIsInit)
    throw codec_error(AL_ERR_RESOLUTION_CHANGE);

  const int buffersHeldByNextComponent = 1; /* We need at least 1 buffer to copy the output on a file */
  AL_TBufPoolConfig BufPoolConfig;
  BufPoolConfig.zBufSize = BufferSize;
  BufPoolConfig.uMaxBuf = BufferNumber + buffersHeldByNextComponent;
  BufPoolConfig.uMinBuf = BufferNumber + buffersHeldByNextComponent;
  BufPoolConfig.debugName = "yuv";

  AL_TPitches tPitches {};
  AL_TOffsetYC tOffsetYC {};
  AL_TDimension tDimension = { iWidth, iHeight };
  BufPoolConfig.pMetaData = (AL_TMetaData*)AL_SrcMetaData_Create(tDimension, tPitches, tOffsetYC, tFourCC);

  if(!AL_BufPool_Init(&p->bufPool, p->pAllocator, &BufPoolConfig))
    throw codec_error(AL_ERR_NO_MEMORY);

  p->bPoolIsInit = true;

  for(int i = 0; i < BufferNumber; ++i)
  {
    auto pDecPict = AL_BufPool_GetBuffer(&p->bufPool, AL_BUF_MODE_NONBLOCK);
    assert(pDecPict);
    AL_Decoder_PutDisplayPicture(p->hDec, pDecPict);
    AL_Buffer_Unref(pDecPict);
  }
}

/******************************************************************************/
static uint32_t ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream)
{
  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);
  auto zSize = pBufStream->zSize;

  ifFileStream.read((char*)pBuf, zSize);
  return (uint32_t)ifFileStream.gcount();
}

/******************************************************************************/
static void AddBuffer(AL_HDecoder hDec, AL_TBuffer* pBufStream, size_t zAvailSize)
{
  assert(pBufStream);
  assert(hDec);

  auto bRet = AL_Decoder_PushBuffer(hDec, pBufStream, zAvailSize, AL_BUF_MODE_BLOCK);

  if(!bRet)
    throw runtime_error("Failed to push buffer");
}

/******************************************************************************/
void SafeMain(int argc, char** argv)
{
  auto const Config = ParseCommandLine(argc, argv);

  if(Config.help)
    return;

  int iNumFrameConceal = 0;

  DisplayVersionInfo();

  ifstream ifFileStream;
  OpenInput(ifFileStream, Config.sIn);

  ofstream ofYuvFile;

  if(Config.bEnableYUVOutput)
    OpenOutput(ofYuvFile, Config.sOut);

  ofstream CertCrcFile;

  if(bCertCRC)
  {
    OpenOutput(CertCrcFile, "crc_certif_res.hex", false);
    CertCrcFile << hex << uppercase;
  }

  ofstream IpCrcFile;

  if(!Config.sCrc.empty())
  {
    OpenOutput(IpCrcFile, Config.sCrc, false);
    IpCrcFile << hex << uppercase;
  }


  // IP Device ------------------------------------------------------------
  auto iUseBoard = Config.iUseBoard;

  function<AL_TIpCtrl*(AL_TIpCtrl*)> wrapIpCtrl;
  switch(Config.ipCtrlMode)
  {
  default:
    wrapIpCtrl = [](AL_TIpCtrl* ipCtrl) -> AL_TIpCtrl*
                 {
                   return ipCtrl;
                 };
    break;
  }

  auto pIpDevice = CreateIpDevice(&iUseBoard, Config.iSchedulerType, Config.tDecSettings.eDecUnit, wrapIpCtrl, Config.trackDma, Config.tDecSettings.uNumCore, Config.hangers);

  auto pAllocator = pIpDevice->m_pAllocator.get();
  auto pDecChannel = pIpDevice->m_pDecChannel;

  AL_TPitches tPitches {};
  AL_TOffsetYC tOffsetYC {};
  AL_TDimension tDimension {};
  AL_TMetaData* Meta = (AL_TMetaData*)AL_SrcMetaData_Create(tDimension, tPitches, tOffsetYC, 0);
  auto YuvBuffer = AL_Buffer_Create_And_Allocate(AL_GetDefaultAllocator(), 100, NULL);

  if(!YuvBuffer)
    throw runtime_error("Couldn't allocate YuvBuffer");
  AL_Buffer_AddMetaData(YuvBuffer, Meta);

  auto scopeBuffer = scopeExit([&]() {
    AL_Allocator_Free(YuvBuffer->pAllocator, YuvBuffer->hBuf);
    AL_Buffer_Destroy(YuvBuffer);
  });

  BufPool bufPool;

  {
    AL_TBufPoolConfig BufPoolConfig {};

    BufPoolConfig.zBufSize = Config.zInputBufferSize;
    BufPoolConfig.uMaxBuf = Config.uInputBufferNum;
    BufPoolConfig.uMinBuf = Config.uInputBufferNum;
    BufPoolConfig.pMetaData = nullptr;
    BufPoolConfig.debugName = "stream";

    auto ret = AL_BufPool_Init(&bufPool, AL_GetDefaultAllocator(), &BufPoolConfig);

    if(!ret)
      throw runtime_error("Can't create BufPool");
  }

  TCbParam tDisplayParam =
  {
    NULL, NULL, ofYuvFile, IpCrcFile, CertCrcFile, YuvBuffer, Config.tDecSettings.iBitDepth, 0, {}
  };
  tDisplayParam.hFinished = Rtos_CreateEvent(false);

  ResChgParam ResolutionFoundParam;
  ResolutionFoundParam.pAllocator = pAllocator;
  ResolutionFoundParam.bPoolIsInit = false;

  DecodeParam tDecodeParam {};
  AL_TDecSettings Settings = Config.tDecSettings;

  AL_TDecCallBacks CB {};
  CB.endDecodingCB = { &sFrameDecoded, &tDecodeParam };
  CB.displayCB = { &sFrameDisplay, &tDisplayParam };
  CB.resolutionFoundCB = { &sResolutionFound, &ResolutionFoundParam };

  Settings.iBitDepth = HW_IP_BIT_DEPTH;

  AL_HDecoder hDec;
  auto error = AL_Decoder_Create(&hDec, (AL_TIDecChannel*)pDecChannel, pAllocator, &Settings, &CB);

  if(!hDec || error != AL_SUCCESS)
    throw codec_error(AL_ERR_INIT_FAILED);

  bool timeoutOccured = false;
  auto scopeDecoder = scopeExit([&]() {
    if(!timeoutOccured)
      AL_Decoder_Destroy(hDec);
  });

  // Param of Display Callback assignment
  tDisplayParam.hDec = hDec;
  tDecodeParam.hDec = hDec;
  ResolutionFoundParam.hDec = hDec;

  AL_Decoder_SetParam(hDec, Config.bConceal, iUseBoard ? true : false, Config.iNumTrace, Config.iNumberTrace);

  if(!invalidPreallocSettings(Config.tDecSettings.tStream))
  {
    if(!AL_Decoder_PreallocateBuffers(hDec))
      if(auto eErr = AL_Decoder_GetLastError(hDec))
        throw codec_error(eErr);
  }

  // Initial stream buffer filling
  auto const uBegin = GetPerfTime();
  int iLoop = 0;
  int iTimeOutInMilliSeconds = Config.iTimeOutInSeconds * 1000.0;

  for(;;)
  {
    for(;;)
    {
      auto pBufStream = shared_ptr<AL_TBuffer>(
        AL_BufPool_GetBuffer(&bufPool, AL_BUF_MODE_BLOCK),
        &AL_Buffer_Unref);

      auto uAvailSize = ReadStream(ifFileStream, pBufStream.get());

      if(!uAvailSize)
        break;

      AddBuffer(hDec, pBufStream.get(), uAvailSize);

      auto eErr = AL_Decoder_GetLastError(hDec);

      if(eErr == AL_WARN_CONCEAL_DETECT)
      {
        iNumFrameConceal++;
        eErr = AL_SUCCESS;
      }

      if(eErr)
        throw codec_error(eErr);

      if(iTimeOutInMilliSeconds > 0)
      {
        auto const uTimeOut = GetPerfTime();
        auto const durationInMilliSeconds = (uTimeOut - uBegin);

        if(durationInMilliSeconds >= (unsigned)iTimeOutInMilliSeconds)
        {
          timeoutOccured = true;
          unique_lock<mutex> res_lock(ResolutionFoundParam.hMutex);
          AL_Decoder_Destroy(hDec);
          Rtos_WaitEvent(tDisplayParam.hFinished, AL_WAIT_FOREVER);
          ResolutionFoundParam.hDec = nullptr;

          unique_lock<mutex> lck(tDisplayParam.hMutex);
          auto const durationInSeconds = durationInMilliSeconds / 1000.0;
          Message(CC_DEFAULT, "\n\nTIMEOUT = %.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n",
                  durationInSeconds,
                  tDecodeParam.decodedFrames / durationInSeconds,
                  iNumFrameConceal);
          return;
        }
      }
    }

    // Flush decoding request
    AL_Decoder_Flush(hDec);
    Rtos_WaitEvent(tDisplayParam.hFinished, AL_WAIT_FOREVER);

    if(++iLoop >= Config.iLoop)
      break;

    // Rewind
    ifFileStream.clear();
    ifFileStream.seekg(0);
    Message(CC_GREY, "  Looping\n");
  }

  auto const uEnd = GetPerfTime();

  unique_lock<mutex> lck(tDisplayParam.hMutex);

  if(auto eErr = AL_Decoder_GetLastError(hDec))
    throw codec_error(eErr);

  if(!tDecodeParam.decodedFrames)
    throw codec_error(AL_ERR_NO_FRAME_DECODED);

  auto const duration = (uEnd - uBegin) / 1000.0;
  Message(CC_DEFAULT, "\n\nDecoded time = %.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n",
          duration,
          tDecodeParam.decodedFrames / duration,
          iNumFrameConceal);
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

