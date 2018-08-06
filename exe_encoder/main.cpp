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
#include "lib_common/StreamBuffer.h"
#include "lib_common/Utils.h"
#include "lib_common/versions.h"
#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/IpEncFourCC.h"
}

#include "lib_conv_yuv/lib_conv_yuv.h"
#include "sink_encoder.h"
#include "sink_bitstream_writer.h"
#include "sink_frame_writer.h"
#include "sink_md5.h"
#include "sink_repeater.h"
#include "QPGenerator.h"

static int g_numFrameToRepeat;
static int g_StrideHeight = -1;
static int g_Stride = -1;
static bool g_ShouldAddDummySei = false;

using namespace std;

/*****************************************************************************/

void DisplayVersionInfo()
{
  Message(CC_DEFAULT, "%s - %s v%d.%d.%d - %s\n", AL_ENCODER_COMPANY,
          AL_ENCODER_PRODUCT_NAME,
          AL_ENCODER_VERSION,
          AL_ENCODER_COPYRIGHT);

  Message(CC_YELLOW, "%s\n", AL_ENCODER_COMMENTS);
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
  cfg.RunInfo.uInputSleepInMilliseconds = 0;
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
  opt.addFlag("--quiet,-q", &g_Verbosity, "do not print anything", 0);

  opt.addInt("--input-width", &cfg.FileInfo.PictWidth, "Specify YUV input width");
  opt.addInt("--input-height", &cfg.FileInfo.PictHeight, "Specify YUV input height");
  opt.addOption("--chroma-mode", [&]()
  {
    auto chromaMode = stringToChromaMode(opt.popWord());
    AL_SET_CHROMA_MODE(cfg.Settings.tChParam[0].ePicFormat, chromaMode);
  }, "Specify chroma-mode (CHROMA_MONO, CHROMA_4_0_0, CHROMA_4_2_0, CHROMA_4_2_2)");

  int ipbitdepth = -1;
  opt.addInt("--level", &cfg.Settings.tChParam[0].uLevel, "Specify the level we want to encode with (10 to 62)");
  opt.addCustom("--profile", &cfg.Settings.tChParam[0].eProfile, &GetCmdlineValue, "Specify the profile we want to encode with (example: HEVC_MAIN, AVC_MAIN, ...)");
  opt.addCustom("--ip-bitdepth", &ipbitdepth, &GetCmdlineValue, "Specify bitdepth of ip input (8 : 10)");
  opt.addCustom("--input-format", &cfg.FileInfo.FourCC, &GetCmdlineFourCC, "Specify YUV input format (I420, IYUV, YV12, NV12, Y800, Y010, P010, I0AL ...)");
  opt.addCustom("--rec-format", &cfg.RecFourCC, &GetCmdlineFourCC, "Specify output format");
  opt.addCustom("--ratectrl-mode", &cfg.Settings.tChParam[0].tRCParam.eRCMode, &GetCmdlineValue,
                "Specify rate control mode (CONST_QP, CBR, VBR"
                ", LOW_LATENCY"
                ")"
                );
  opt.addOption("--bitrate", [&]()
  {
    cfg.Settings.tChParam[0].tRCParam.uTargetBitRate = opt.popInt() * 1000;
  }, "Specify bitrate in Kbits/s");
  opt.addOption("--max-bitrate", [&]()
  {
    cfg.Settings.tChParam[0].tRCParam.uMaxBitRate = opt.popInt() * 1000;
  }, "Specify max bitrate in Kbits/s");
  opt.addOption("--framerate", [&]()
  {
    GetFpsCmdline(opt.popWord(),
                  cfg.Settings.tChParam[0].tRCParam.uFrameRate,
                  cfg.Settings.tChParam[0].tRCParam.uClkRatio);
  }, "Specify the frame rate used for encoding");
  opt.addInt("--sliceQP", &cfg.Settings.tChParam[0].tRCParam.iInitialQP, "Specify the initial slice QP");
  opt.addInt("--gop-length", &cfg.Settings.tChParam[0].tGopParam.uGopLength, "Specify the GOP length, 0 means I slice only");
  opt.addInt("--gop-numB", &cfg.Settings.tChParam[0].tGopParam.uNumB, "Number of consecutive B frame (0 .. 4)");
  opt.addCustom("--gop-mode", &cfg.Settings.tChParam[0].tGopParam.eMode, &GetCmdlineValue, "Specify gop control mode (DEFAULT_GOP, PYRAMIDAL_GOP)");
  opt.addInt("--first-picture", &cfg.RunInfo.iFirstPict, "first picture encoded (skip those before)");
  opt.addInt("--max-picture", &cfg.RunInfo.iMaxPict, "Maximum number of pictures encoded (1,2 .. -1 for ALL)");
  opt.addInt("--num-slices", &cfg.Settings.tChParam[0].uNumSlices, "Specify the number of slices to use");
  opt.addInt("--num-core", &cfg.Settings.tChParam[0].uNumCore, "Specify the number of cores to use (resolution needs to be sufficient)");
  opt.addString("--log", &cfg.RunInfo.logsFile, "A file where log event will be dumped");
  opt.addFlag("--loop", &cfg.RunInfo.bLoop, "loop at the end of the yuv file");
  opt.addFlag("--slicelat", &cfg.Settings.tChParam[0].bSubframeLatency, "enable subframe latency");
  opt.addFlag("--framelat", &cfg.Settings.tChParam[0].bSubframeLatency, "disable subframe latency", false);

  opt.addInt("--prefetch", &g_numFrameToRepeat, "prefetch n frames and loop between these frames for max picture count");
  opt.addFlag("--print-picture-type", &cfg.RunInfo.printPictureType, "write picture type for each frame in the file", true);
  opt.addFlag("--dummy-sei", &g_ShouldAddDummySei, "Add a dummy sei prefix and suffix");


  opt.parse(argc, argv);

  if(help)
  {
    Usage(opt, argv[0]);
    exit(0);
  }

  if(version)
  {
    DisplayVersionInfo();
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

  cfg.Settings.tChParam[0].uEncodingBitDepth = AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat);

  if(AL_IS_STILL_PROFILE(cfg.Settings.tChParam[0].eProfile))
    cfg.RunInfo.iMaxPict = 1;


  if(ipbitdepth != -1)
  {
    AL_SET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat, ipbitdepth);
  }
}

void ValidateConfig(ConfigFile& cfg)
{
  string invalid_settings("Invalid settings, check the [SETTINGS] section of your configuration file or check your commandline (use -h to get help)");

  if(cfg.YUVFileName.empty())
    throw runtime_error("No YUV input was given, specify it in the [INPUT] section of your configuration file or in your commandline (use -h to get help)");

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
    if(AL_GET_CHROMA_MODE(Settings.tChParam[0].ePicFormat) == CHROMA_MONO)
    {
      if(AL_GET_BITDEPTH(Settings.tChParam[0].ePicFormat) == 8)
        RecFourCC = FOURCC(Y800);
      else if(AL_Is10bitPacked(FileInfo.FourCC))
        RecFourCC = FOURCC(XV10);
      else
        RecFourCC = FOURCC(Y010);
    }
    else
      RecFourCC = FileInfo.FourCC;
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

  AL_TOffsetYC tOffsetYC = GetOffsetYC(tPitches.iLuma, iHeight, AL_IsTiled(tFourCC));
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
    hConv->ConvertSrcBuf(AL_GET_BITDEPTH(tChParam.ePicFormat), conversionBuffer, sourceBuffer.get());

  return sourceBuffer;
}

bool ConvertSrcBuffer(AL_TEncChanParam& tChParam, TYUVFileInfo& FileInfo, vector<uint8_t>& YuvBuffer, shared_ptr<AL_TBuffer>& SrcYuv)
{
  AL_TPicFormat const picFmt =
  {
    AL_GET_CHROMA_MODE(tChParam.ePicFormat),
    AL_GET_BITDEPTH(tChParam.ePicFormat),
    AL_GetSrcStorageMode(tChParam.eSrcMode)
  };
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
  AL_TPicFormat const tPictFormat =
  {
    FrameInfo.eCMode,
    FrameInfo.iBitDepth,
    AL_GetSrcStorageMode(eSrcMode)
  };

  TFourCC FourCC = AL_EncGetSrcFourCC(tPictFormat);
  AL_TPitches p = SetPitchYC(FrameInfo.iWidth, FourCC);
  int iStrideHeight = (FrameInfo.iHeight + 7) & ~ 7;

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
  tFrameInfo.iBitDepth = AL_GET_BITDEPTH(tChParam.ePicFormat);
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
  auto QpBufPoolConfig = GetQpBufPoolConfig(Settings, Settings.tChParam[0], frameBuffersCount);
  BufPool QpBufPool(pAllocator, QpBufPoolConfig);


  unique_ptr<EncoderSink> enc;
  enc.reset(new EncoderSink(cfg, pScheduler, pAllocator, QpBufPool
                            ));
  enc->shouldAddDummySei = g_ShouldAddDummySei;

  enc->BitstreamOutput = createBitstreamWriter(StreamFileName, cfg);
  enc->m_done = ([&]() {
    Rtos_SetEvent(hFinished);
  });

  IFrameSink* firstSink = enc.get();

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
    auto uBeforeTime = Rtos_GetTime();
    bRet = sendInputFileTo(YuvFile, SrcBufPool, SrcYuv.get(), cfg, pSrcConv.get(), firstSink, iPictCount, iReadCount);
    auto uTimelaps = Rtos_GetTime() - uBeforeTime;
    if(uTimelaps < cfg.RunInfo.uInputSleepInMilliseconds)
      Rtos_Sleep(cfg.RunInfo.uInputSleepInMilliseconds - uTimelaps);
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

