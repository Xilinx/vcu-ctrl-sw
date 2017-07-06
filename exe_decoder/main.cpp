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

#include "lib_common_dec/DecError.h"

#include "lib_decode/lib_decode.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common/FourCC.h"
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


using namespace std;

const char* ToString(AL_ERR eErrCode)
{
  switch(eErrCode)
  {
  case ERR_INIT_FAILED: return "Failed to initialize the decoder";
  case ERR_CHANNEL_NOT_CREATED: return "Channel not created";
  case ERR_NO_FRAME_DECODED: return "No frame decoded";
  case ERR_RESOLUTION_CHANGE: return "Resolution Change is not supported";
  case ERR_NO_MORE_MEMORY: return "No more memory ! You need more space !";
  case AL_ERR_BUFFER_EXCEPTION: return "No frame decoded";
  case AL_ERR_NOT_SUPPORTED: return "Stream Not supported";
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
  settings.iNumBufHeldByNextComponent = 2;

  return settings;
}

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
  size_t zInputBufferSize = 32 * 1024;
  IpCtrlMode ipCtrlMode = IPCTRL_MODE_STANDARD;
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

  opt.addOption("-clk", [&]()
  {
    Config.tDecSettings.uClkRatio = opt.popInt() + 1000;
  }, "Set clock ratio");

  opt.addFlag("-slicelat", &Config.tDecSettings.eDecUnit,
              "Specify decoder latency (default: Frame Latency)",
              AL_VCL_NAL_UNIT);

  opt.addFlag("-framelat", &Config.tDecSettings.eDecUnit,
              "Specify decoder latency (default: Frame Latency)",
              AL_AU_UNIT);

  opt.addFlag("-avc", &Config.tDecSettings.bIsAvc,
              "Specify the input bitstream codec (default: HEVC)",
              true);

  opt.addFlag("-hevc", &Config.tDecSettings.bIsAvc,
              "Specify the input bitstream codec (default: HEVC)",
              false);

  opt.addFlag("-noyuv", &Config.bEnableYUVOutput,
              "Disable writing output YUV file",
              false);

  opt.parse(argc, argv);

  if(Config.help)
  {
    Usage(opt, argv[0]);
    return Config;
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
    // silently correct user settings
    Config.uInputBufferNum = max(1u, Config.uInputBufferNum);
    Config.zInputBufferSize = max(size_t(1), Config.zInputBufferSize);
    Config.tDecSettings.iStackSize = max(1, Config.tDecSettings.iStackSize);

    if(Config.tDecSettings.uNumCore > AL_DEC_NUM_CORES)
      throw runtime_error("Invalid number of cores");

    if(Config.tDecSettings.uDDRWidth != 16 && Config.tDecSettings.uDDRWidth != 32 && Config.tDecSettings.uDDRWidth != 64)
      throw runtime_error("Invalid DDR width");
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

static
int GetPictureSizeInSamples(AL_TSrcMetaData* meta)
{
  int sx;
  int sy;
  AL_GetSubsampling(meta->tFourCC, &sx, &sy);

  int sampleCount = meta->iWidth * meta->iHeight;

  if(AL_GetChromaMode(meta->tFourCC) != CHROMA_MONO)
    sampleCount += ((sampleCount * 2) / (sx * sy));

  return sampleCount;
}

static TFourCC GetInternalFourCC(AL_EChromaMode eChromaMode, uint8_t uBitDepth)
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
  assert(0);
  return FOURCC(I420);
}

AL_TO_IP GetConversionFunction(TFourCC input, int iBdOut)
{
  auto eChromaMode = AL_GetChromaMode(input);
  auto iBdIn = AL_GetBitDepth(input);

#define GetConvFormat(ChromaMode, iBdIn, iBdOut) ((ChromaMode) | ((iBdIn) << 8) | ((iBdOut) << 16))

  auto const CHROMA_MONO_8bitTo8bit = 0x00080800;
  auto const CHROMA_MONO_10bitTo10bit = 0x000A0A00;
  auto const CHROMA_MONO_8bitTo10bit = 0x000A0800;
  auto const CHROMA_MONO_10bitTo8bit = 0x00080A00;

  auto const CHROMA_420_8bitTo8bit = 0x00080801;
  auto const CHROMA_420_10bitTo10bit = 0x000A0A01;
  auto const CHROMA_420_8bitTo10bit = 0x000A0801;
  auto const CHROMA_420_10bitTo8bit = 0x00080A01;

  auto const CHROMA_422_8bitTo8bit = 0x00080802;
  auto const CHROMA_422_10bitTo10bit = 0x000A0A02;
  auto const CHROMA_422_8bitTo10bit = 0x000A0802;
  auto const CHROMA_422_10bitTo8bit = 0x00080A02;

  int iPicFmt = GetConvFormat(eChromaMode, iBdIn, iBdOut);
  switch(iPicFmt)
  {
  case CHROMA_420_8bitTo8bit:
    return Bind(&NV1X_To_I42X, 2, 2);
  case CHROMA_420_10bitTo10bit:
    return Bind(&RX0A_To_IXAL, 2, 2);
  case CHROMA_420_8bitTo10bit:
    return Bind(&NV1X_To_IXAL, 2, 2);
  case CHROMA_420_10bitTo8bit:
    return Bind(&RX0A_To_I42X, 2, 2);
  case CHROMA_422_8bitTo8bit:
    return Bind(&NV1X_To_I42X, 2, 1);
  case CHROMA_422_10bitTo10bit:
    return Bind(&RX0A_To_IXAL, 2, 1);
  case CHROMA_422_8bitTo10bit:
    return Bind(&NV1X_To_IXAL, 2, 1);
  case CHROMA_422_10bitTo8bit:
    return Bind(&RX0A_To_I42X, 2, 1);
  case CHROMA_MONO_8bitTo8bit:
    return Y800_To_Y800;
  case CHROMA_MONO_10bitTo10bit:
    return RX0A_To_Y010;
  case CHROMA_MONO_8bitTo10bit:
    return Y800_To_Y010;
  case CHROMA_MONO_10bitTo8bit:
    return RX0A_To_Y800;
  default:
    assert(0);
    return nullptr;
  }
}

static void ConvertFrameBuffer(AL_TBuffer& input, int iBdIn, AL_TBuffer& output, int iBdOut)
{
  auto pRecMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&input, AL_META_TYPE_SOURCE);

  auto eChromaMode = AL_GetChromaMode(pRecMeta->tFourCC);

  pRecMeta->tFourCC = GetInternalFourCC(eChromaMode, iBdIn);

  auto const iSizePix = (iBdOut + 7) >> 3;

  uint32_t uSize = GetPictureSizeInSamples(pRecMeta) * iSizePix;

  if(uSize != output.zSize)
  {
    AL_Allocator_Free(output.pAllocator, output.hBuf);
    output.hBuf = AL_Allocator_Alloc(output.pAllocator, uSize);
    output.pData = AL_Allocator_GetVirtualAddr(output.pAllocator, output.hBuf);
    output.zSize = uSize;
  }

  auto pYuvMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(&output, AL_META_TYPE_SOURCE);
  pYuvMeta->iWidth = pRecMeta->iWidth;
  pYuvMeta->iHeight = pRecMeta->iHeight;
  pYuvMeta->tPitches.iLuma = iSizePix * pRecMeta->iWidth;
  pYuvMeta->tPitches.iChroma = iSizePix * ((eChromaMode == CHROMA_4_4_4) ? pRecMeta->iWidth : pRecMeta->iWidth >> 1);

  auto AllegroConvert = GetConversionFunction(pRecMeta->tFourCC, iBdOut);
  AllegroConvert(&input, &output);

  pYuvMeta->tFourCC = pRecMeta->tFourCC;
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

    ConvertFrameBuffer(tRecBuf, iBdIn, tYuvBuf, iBdOut);

    if(info.tCrop.bCropping)
      CropFrame(&tYuvBuf, iSizePix, info.tCrop.uCropOffsetLeft, info.tCrop.uCropOffsetRight, info.tCrop.uCropOffsetTop, info.tCrop.uCropOffsetBottom);

    if(ofCertCrcFile.is_open())
    {
      // compute crc
      int sx = 1, sy = 1;
      AL_GetSubsampling(pRecMeta->tFourCC, &sx, &sy);
      int const iNumPix = pYuvMeta->iHeight * pYuvMeta->iWidth;
      int const iNumPixC = iNumPix / sx / sy;
      auto eChromaMode = AL_GetChromaMode(pRecMeta->tFourCC);

      if(iBdOut == 8)
      {
        uint8_t* pBuf = tYuvBuf.pData;
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, ofCertCrcFile);
      }
      else
      {
        uint16_t* pBuf = (uint16_t*)tYuvBuf.pData;
        Compute_CRC(info.uBitDepthY, info.uBitDepthC, iBdOut, iNumPix, iNumPixC, eChromaMode, pBuf, ofCertCrcFile);
      }
    }

    if(ofYuvFile.is_open())
    {
      auto uSize = GetPictureSizeInSamples(pYuvMeta) * iSizePix;
      ofYuvFile.write((const char*)tYuvBuf.pData, uSize);
    }
  }
}

/******************************************************************************/
struct TCbParam
{
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
  AL_TBufPool BufPool;
  AL_TAllocator* pAllocator;
  mutex hMutex;
};

struct DecodeParam
{
  int decodedFrames;
};

/******************************************************************************/
static void sFrameDecoded(AL_TBuffer* /*pDecodedFrame*/, void* pUserParam)
{
  auto pParam = reinterpret_cast<DecodeParam*>(pUserParam);
  pParam->decodedFrames++;
};

/******************************************************************************/
static void sFrameDisplay(AL_TBuffer* pFrame, AL_TInfoDecode tInfo, void* pUserParam)
{
  auto pParam = reinterpret_cast<TCbParam*>(pUserParam);
  unique_lock<mutex> lck(pParam->hMutex);

  if(!pFrame)
  {
    Message(CC_GREY, "Complete");
    Rtos_SetEvent(pParam->hFinished);
    return;
  }

  if(pParam->iBitDepth == 0)
    pParam->iBitDepth = max(tInfo.uBitDepthY, tInfo.uBitDepthC);
  else if(pParam->iBitDepth == -1)
    pParam->iBitDepth = AL_Decoder_GetMaxBD(pParam->hDec);

  assert(AL_Buffer_GetBufferData(pFrame));

  ProcessFrame(*pFrame, *pParam->YuvBuffer, tInfo, pParam->iBitDepth, pParam->YuvFile, pParam->IpCrcFile, pParam->CertCrcFile);
  AL_Decoder_ReleaseDecPict(pParam->hDec, pFrame);

  DisplayFrameStatus(pParam->num_frame);
  pParam->num_frame++;
}

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
};

static void sResolutionFound(int BufferNumber, int BufferSize, int iWidth, int iHeight, AL_TCropInfo tCropInfo, TFourCC tFourCC, void* pUserParam)
{
  ResChgParam* p = (ResChgParam*)pUserParam;

  unique_lock<mutex> lck(p->hMutex);

  stringstream ss;
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
    throw codec_error(ERR_RESOLUTION_CHANGE);

  AL_TBufPoolConfig BufPoolConfig;

  BufPoolConfig.zBufSize = BufferSize;
  BufPoolConfig.uMaxBuf = BufferNumber;
  BufPoolConfig.uMinBuf = BufferNumber;

  AL_TPitches tPitches {};
  AL_TOffsetYC tOffsetYC {};
  BufPoolConfig.pMetaData = (AL_TMetaData*)AL_SrcMetaData_Create(iWidth, iHeight, tPitches, tOffsetYC, tFourCC);

  if(!AL_BufPool_Init(&p->BufPool, p->pAllocator, &BufPoolConfig))
    throw codec_error(ERR_NO_MORE_MEMORY);

  p->bPoolIsInit = true;

  for(int i = 0; i < BufferNumber; ++i)
  {
    auto pDecPict = AL_BufPool_GetBuffer(&p->BufPool, AL_BUF_MODE_NONBLOCK);
    assert(pDecPict);
    AL_Decoder_PutDecPict(p->hDec, pDecPict);
    AL_BufPool_ReleaseBuffer(&p->BufPool, pDecPict);
  }
}

/******************************************************************************/
static uint32_t ReadStream(ifstream& ifFileStream, AL_TBuffer* pBufStream)
{
  uint8_t* pBuf = AL_Buffer_GetBufferData(pBufStream);
  auto zSize = AL_Buffer_GetSizeData(pBufStream);

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

  //
  // IP Device ------------------------------------------------------------
  auto iUseBoard = Config.iUseBoard;
  auto pIpDevice = CreateIpDevice(&iUseBoard, Config.iSchedulerType, Config.tDecSettings.eDecUnit, Config.ipCtrlMode, Config.tDecSettings.uNumCore);

  auto pAllocator = pIpDevice->m_pAllocator;
  auto scopeAllocator = scopeExit([&]() {
    AL_Allocator_Destroy(pAllocator);
  });

  auto pDecChannel = pIpDevice->m_pDecChannel;

  AL_TPitches tPitches {};
  AL_TOffsetYC tOffsetYC {};
  AL_TMetaData* Meta = (AL_TMetaData*)AL_SrcMetaData_Create(0, 0, tPitches, tOffsetYC, 0);
  auto YuvBuffer = AL_Buffer_Create_And_Allocate(AL_GetDefaultAllocator(), 100, NULL);

  if(!YuvBuffer)
    throw runtime_error("Couldn't allocate YuvBuffer");
  AL_Buffer_AddMetaData(YuvBuffer, Meta);

  auto scopeBuffer = scopeExit([&]() {
    AL_Allocator_Free(YuvBuffer->pAllocator, YuvBuffer->hBuf);
    AL_Buffer_Destroy(YuvBuffer);
  });

  TCbParam tDisplayParam =
  {
    NULL, NULL, ofYuvFile, IpCrcFile, CertCrcFile, YuvBuffer, Config.tDecSettings.iBitDepth, 0, {}
  };
  tDisplayParam.hFinished = Rtos_CreateEvent(false);
  auto scopeMutex = scopeExit([&]() {
    Rtos_DeleteEvent(tDisplayParam.hFinished);
  });

  ResChgParam ResolutionFoundParam;
  ResolutionFoundParam.pAllocator = pAllocator;
  ResolutionFoundParam.bPoolIsInit = false;

  auto scopeBufPool = scopeExit([&]() {
    if(ResolutionFoundParam.bPoolIsInit)
      AL_BufPool_Deinit(&ResolutionFoundParam.BufPool);
  });

  DecodeParam tDecodeParam {};
  AL_TDecSettings Settings = Config.tDecSettings;

  AL_TDecCallBacks CB {};
  CB.endDecodingCB = { &sFrameDecoded, &tDecodeParam };
  CB.displayCB = { &sFrameDisplay, &tDisplayParam };
  CB.resolutionFoundCB = { &sResolutionFound, &ResolutionFoundParam };

  Settings.iBitDepth = HW_IP_BIT_DEPTH;

  AL_HDecoder hDec = AL_Decoder_Create((AL_TIDecChannel*)pDecChannel, pAllocator, &Settings, &CB);

  if(!hDec)
    throw codec_error(ERR_INIT_FAILED);

  auto scopeDecoder = scopeExit([&]() {
    pIpDevice.reset();
    AL_Decoder_Destroy(hDec);
  });

  // Param of Display Callback assignment
  tDisplayParam.hDec = hDec;

  ResolutionFoundParam.hDec = hDec;

  AL_Decoder_SetParam(hDec, Config.bConceal, iUseBoard ? true : false, Config.iNumTrace, Config.iNumberTrace);

  AL_TBufPool bufPool;

  {
    AL_TBufPoolConfig BufPoolConfig {};

    BufPoolConfig.zBufSize = Config.zInputBufferSize;
    BufPoolConfig.uMaxBuf = Config.uInputBufferNum;
    BufPoolConfig.uMinBuf = Config.uInputBufferNum;
    BufPoolConfig.pMetaData = nullptr;

    auto ret = AL_BufPool_Init(&bufPool, AL_GetDefaultAllocator(), &BufPoolConfig);

    if(!ret)
      throw runtime_error("Can't create BufPool");
  }

  auto destroyBufPool = scopeExit([&]() {
    AL_BufPool_Deinit(&bufPool);
  });

  // Initial stream buffer filling
  auto const uBegin = GetPerfTime();

  for(;;)
  {
    auto pBufStream = AL_BufPool_GetBuffer(&bufPool, AL_BUF_MODE_BLOCK);

    auto uAvailSize = ReadStream(ifFileStream, pBufStream);

    if(!uAvailSize)
      break;

    AddBuffer(hDec, pBufStream, uAvailSize);

    AL_BufPool_ReleaseBuffer(&bufPool, pBufStream);

    auto eErr = AL_Decoder_GetLastError(hDec);

    if(eErr == AL_WARN_CONCEAL_DETECT)
    {
      iNumFrameConceal++;
      eErr = AL_SUCCESS;
    }

    if(eErr)
      throw codec_error(eErr);
  }

  /* flush decoding request */
  AL_Decoder_Flush(hDec);

  Rtos_WaitEvent(tDisplayParam.hFinished, AL_WAIT_FOREVER);

  auto const uEnd = GetPerfTime();

  unique_lock<mutex> lck(tDisplayParam.hMutex);

  if(auto eErr = AL_Decoder_GetLastError(hDec))
    throw codec_error(eErr);

  if(!tDecodeParam.decodedFrames)
    throw codec_error(ERR_NO_FRAME_DECODED);

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

