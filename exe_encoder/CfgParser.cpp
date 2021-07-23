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

#include "CfgParser.h"
#include "lib_common/SEI.h"
#include "lib_cfg_parsing/Parser.h"
#include "lib_rtos/types.h" // AL_MAX_NUM_B_PICT

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <type_traits>

using std::cout;
using std::numeric_limits;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::vector;
using std::stringstream;
using std::remove_reference;
using std::to_string;

struct Temporary
{
  Temporary()
  {
  }

  string sScalingListFile {};
  string sZapperFile {};
  TConfigYUVInput TempInput;
  bool bWidthIsParsed = false;
  bool bHeightIsParsed = false;
  bool bNameIsParsed = false;
  bool bParseLambdaFactors = false;

};

static TFourCC GetFourCCValue(const string& sVal)
{
  // backward compatibility
  if(!sVal.compare("FILE_MONOCHROME"))
    return FOURCC(Y800);
  else if(!sVal.compare("FILE_YUV_4_2_0"))
    return FOURCC(I420);
  else if(!sVal.compare("RXMA"))
    return FOURCC(XV10);
  else if(!sVal.compare("RX0A"))
    return FOURCC(XV15);
  else if(!sVal.compare("RX2A"))
    return FOURCC(XV20);

  // read FourCC
  uint32_t uFourCC = 0;

  if(sVal.size() >= 1)
    uFourCC = ((uint32_t)sVal[0]);

  if(sVal.size() >= 2)
    uFourCC |= ((uint32_t)sVal[1]) << 8;

  if(sVal.size() >= 3)
    uFourCC |= ((uint32_t)sVal[2]) << 16;

  if(sVal.size() >= 4)
    uFourCC |= ((uint32_t)sVal[3]) << 24;

  return (TFourCC)uFourCC;
}

static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
}

static void populateInputSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Input;
  parser.addPath(curSection, "YUVFile", cfg.MainInput.YUVFileName, "The YUV file name for the enhanced layer in SHVC case", allCodecs());
  parser.addArith(curSection, "Width", cfg.MainInput.FileInfo.PictWidth, "Frame width in pixels.", {
    { isOnlyCodec(Codec::Avc), 80, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH },
    { isOnlyCodec(Codec::Hevc), 256, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH },
    { aomCodecs(), 128, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH },
    { isOnlyCodec(Codec::Jpeg), 16, AL_ENC_CORE_MAX_WIDTH_JPEG },
  });
  parser.addArith(curSection, "Height", cfg.MainInput.FileInfo.PictHeight, "Frame height in pixels.", {
    { isOnlyCodec(Codec::Avc), 96, 4096 },
    { filterCodecs({ Codec::Hevc, Codec::Vp9, Codec::Av1 }), 128, 4096 },
    { isOnlyCodec(Codec::Jpeg), 2, 16384 }
  });
  auto constexpr resolutionNote = "Allowed value can be lower/higher, however some contraints are added.";
  parser.addNote(curSection, "Width", resolutionNote);
  parser.addNote(curSection, "Height", resolutionNote);
  map<string, EnumDescription<int>> fourCCs;
  fourCCs["Y800"] = { FOURCC(Y800), "YUV files contains 8-bit monochrome.", allCodecs() };

  fourCCs["I420"] = { FOURCC(I420), "YUV file contains 4:2:0 8-bit video samples stored in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples).", allCodecs() };
  fourCCs["IYUV"] = { FOURCC(IYUV), "Same as I420.", allCodecs() };
  fourCCs["YV12"] = { FOURCC(YV12), "Same as I420 with inverted U and V order.", allCodecs() };
  fourCCs["NV12"] = { FOURCC(NV12), "YUV file contains 4:2:0 8-bit video samples stored in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };

  fourCCs["NV16"] = { FOURCC(NV16), "YUV file contains 4:2:2 8-bit video samples stored in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma sample.", allCodecs() };
  fourCCs["I422"] = { FOURCC(I422), "YUV file contains 4:2:2 8-bit video samples stored in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples).", allCodecs() };
  fourCCs["YV16"] = { FOURCC(YV16), "Same as I422.", allCodecs() };
  fourCCs["YUY2"] = { FOURCC(YUY2), "", allCodecs() };

#if HW_IP_BITDEPTH >= 10
  fourCCs["Y010"] = { FOURCC(Y010), "YUV file contains 10-bit monochrome samples.", allCodecs() };
  fourCCs["P010"] = { FOURCC(P010), "YUV file contains 4:2:0 10-bit video samples each stored in a 16-bit word in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };
  fourCCs["I0AL"] = { FOURCC(I0AL), "YUV file contains 4:2:0 10-bit video samples each stored in a 16-bit word in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples).", allCodecs() };

  fourCCs["I2AL"] = { FOURCC(I2AL), "YUV file contains 4:2:2 10-bit video samples each stored in a 16-bit word in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples).", allCodecs() };
  fourCCs["P210"] = { FOURCC(P210), "YUV file contains 4:2:2 10-bit video samples each stored in a 16-bit word in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };
  fourCCs["YUVP"] = { FOURCC(YUVP), "", allCodecs() };
#endif

#if HW_IP_BITDEPTH >= 12
  fourCCs["Y012"] = { FOURCC(Y012), "YUV file contains 10-bit monochrome samples.", allCodecs() };
  fourCCs["I0CL"] = { FOURCC(I0CL), "YUV file contains 4:2:0 12-bit video samples each stored in a 16-bit word in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples)." allCodecs() };
  fourCCs["P012"] = { FOURCC(P012), "YUV file contains 4:2:0 12-bit video samples each stored in a 16-bit word in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };
  fourCCs["I2CL"] = { FOURCC(I2CL), "YUV file contains 4:2:2 12-bit video samples each stored in a 16-bit word in planar format with all picture luma (Y) samples followed by chroma samples (all U samples then all V samples).", allCodecs() };
  fourCCs["P212"] = { FOURCC(P212), "YUV file contains 4:2:2 12-bit video samples each stored in a 16-bit word in semi-planar format with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };

#endif

#if HW_IP_BITDEPTH >= 10
  fourCCs("XV10") = { FOURCC(XV10), "YUV file contains packed 10-bit monochrome samples with 3 samples per 32-bit words (the 2 LSBs are not used).", allCodecs() };
  fourCCs("XV15") = { FOURCC(XV15), "YUV file contains packed 4:2:0 10-bit samples in semi-planar format with 3 samples per 32-bit words (the 2 LSBs are not used) and with all picture luma (Y) samples followed by interleaved U and V chroma sample.", allCodecs() };
  fourCCs("XV20") = { FOURCC(XV20), "YUV file contains packed 4:2:2 10 bits samples in semi-planar format with 3 samples per 32bits words (The 2 LSBs are not used) and with all picture luma (Y) samples followed by interleaved U and V chroma samples.", allCodecs() };
#endif

  parser.addCustom(curSection, "Format", [&](std::deque<Token>& tokens)
  {
    /* we might want to be able to show users which format are available */
    if(!hasOnlyOneIdentifier(tokens))
      throw std::runtime_error("Failed to parse FOURCC value.");
    cfg.MainInput.FileInfo.FourCC = GetFourCCValue(tokens[0].text);
  }, [&]() { return FourCCToString(cfg.MainInput.FileInfo.FourCC); }, "Specifies the YUV input format.", { ParameterType::String }, {
    { allCodecs(), describeEnum(fourCCs), descriptionEnum(fourCCs) }
  }, { "For more info, visit: https://www.fourcc.org." });
  parser.addNote(curSection, "Format", "Most of the YUV test sequences available on the internet use either I420, I0AL, I422 or I2AL format. there is no conversion from 4:2:0 to 4:2:2 nor from 4:2:2 to 4:2:0. The subsampling of the YUV input file format shall match the ChromaMode");
  parser.addSeeAlso(curSection, "Format", { Section::Settings, "ChromaMode" });
  parser.addPath(curSection, "CmdFile", cfg.sCmdFileName, "File containing the dynamic commands to send to the encoder");
  parser.addPath(curSection, "ROIFile", cfg.MainInput.sRoiFileName, "File containing the Regions of Interest used to encode");
  parser.addNote(curSection, "ROIFile", "The file is used only when QPCtrlMode parameter from SETTINGS section is set to ROI_QP");
  parser.addSeeAlso(curSection, "ROIFile", { Section::Settings, "QPCtrlMode" });
  parser.addPath(curSection, "QpTablesFolder", cfg.MainInput.sQPTablesFolder, "When QPCtrlMode is set to LOAD_QP, the parameter specifies the location of the files containing the QP tables to use for each frame");
  parser.addSeeAlso(curSection, "QpTablesFolder", { Section::Settings, "QPCtrlMode" });
  parser.addPath(curSection, "TwoPassFile", cfg.sTwoPassFileName, "File containing the first pass statistics");
  parser.addPath(curSection, "HDRFile", cfg.sHDRFileName, "Name of the file specifying HDR SEI contents.", filterCodecs({ Codec::Hevc, Codec::Avc }));

  parser.addArith(curSection, "FrameRate", cfg.MainInput.FileInfo.FrameRate, "Number of frames per second of the YUV input file. When this parameter is not present, its value is set equal to the FrameRate specified in the RATE_CONTROL section. When this parameter is greater than the FrameRate specified in the RATE_CONTROL section, the encoder will drop some frames; when this parameter is lower than the FrameRate specified in the RATE_CONTROL section, the encoder will repeat some frames", {
    { allCodecs(), 0 },
  });
  parser.addNote(curSection, "FrameRate", "If unset or set to 0, we use [RATE_CONTROL]FrameRate's value.");
  parser.addNote(curSection, "FrameRate", "When this parameter is greater than [RATE_CONTROL]FrameRate specified in the rate control section, the encoder will drop some frames.");
  parser.addNote(curSection, "FrameRate", "When this parameter is lower than [RATE_CONTROL]FrameRate specified in the rate control section, the encoder will repeat some frames.");
  parser.addSeeAlso(curSection, "FrameRate", { Section::RateControl, "Framerate" });

  parser.addCustom(curSection, "CropWidth", [&](std::deque<Token>& tokens)
  {
    cfg.Settings.tChParam[0].bEnableSrcCrop = true;
    cfg.Settings.tChParam[0].uSrcCropWidth = parseArithmetic<int>(tokens);
  }, [&]() { return std::to_string(cfg.Settings.tChParam[0].uSrcCropWidth); }, "Width of crop window in pixels, and shall be multiple of 8.", { ParameterType::ArithExpr }, {
    { isOnlyCodec(Codec::Avc), "[80; 4016]", {}
    },
    { isOnlyCodec(Codec::Hevc), "[256; 3840]", {}
    },
    { aomCodecs(), "[128; 3968]", {}
    },
    { isOnlyCodec(Codec::Jpeg), "[16; 16368]", {}
    },
  });
  parser.addCustom(curSection, "CropHeight", [&](std::deque<Token>& tokens)
  {
    cfg.Settings.tChParam[0].bEnableSrcCrop = true;
    cfg.Settings.tChParam[0].uSrcCropHeight = parseArithmetic<int>(tokens);
  }, [&]() { return std::to_string(cfg.Settings.tChParam[0].uSrcCropHeight); }, "Height of crop window in pixels, and shall be multiple of 8.", { ParameterType::ArithExpr },
  {
    { isOnlyCodec(Codec::Avc), "[96; 4000]", {}
    },
    { filterCodecs({ Codec::Hevc, Codec::Vp9, Codec::Av1 }), "[128; 3968]", {}
    },
    { isOnlyCodec(Codec::Jpeg), "[2; 16382]", {}
    },
  });

  parser.addArith(curSection, "CropPosX", cfg.Settings.tChParam[0].uSrcCropPosX, "Abscissa of crop window first sample in pixels, and shall be multiple of 2.", {
    { isOnlyCodec(Codec::Avc), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 80 },
    { isOnlyCodec(Codec::Hevc), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 256 },
    { aomCodecs(), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 128 },
    { isOnlyCodec(Codec::Jpeg), 0, AL_ENC_CORE_MAX_WIDTH_JPEG - 16 },
  });
  parser.addArith(curSection, "CropPosY", cfg.Settings.tChParam[0].uSrcCropPosY, "Ordinate of crop window first sample in pixels, and shall be multiple of 2.", {
    { isOnlyCodec(Codec::Avc), 96, 4096 - 96 },
    { filterCodecs({ Codec::Hevc, Codec::Vp9, Codec::Av1 }), 128, 4096 - 128 },
    { isOnlyCodec(Codec::Jpeg), 2, 16384 - 2 },
  });
}

static void populateOutputSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Output;
  parser.addPath(curSection, "BitstreamFile", cfg.BitstreamFileName, "Elementary stream output file name", allCodecs());
  parser.addPath(curSection, "RecFile", cfg.RecFileName, "Optional output file name for reconstructed picture (in YUV format). When this parameter is not present, the reconstructed YUV file is not saved", allCodecs());
  parser.addNote(curSection, "RecFile", "If unset, the reconstruted picture is not saved. The reconstructed picture can be used for quality measurement, for example the SSIM between source and reconstructed pictures can be computed using the following tool: http://compression.ru/video/quality_measure/video_measurement_tool_en.html");
  parser.addCustom(curSection, "Format", [&](std::deque<Token>& tokens)
  {
    /* we might want to be able to show users which format are available */
    if(!hasOnlyOneIdentifier(tokens))
      throw std::runtime_error("Failed to parse FOURCC value.");
    cfg.RecFourCC = GetFourCCValue(tokens[0].text);
  }, [&]() { return FourCCToString(cfg.MainInput.FileInfo.FourCC); }, "Specifies Reconstructed YUV output format.");
  parser.addNote(curSection, "Format", "If unset, the [OUTPUT]Format is equals to [INPUT]Format");

  parser.addArith(curSection, "CropPosX", cfg.Settings.tChParam[0].uOutputCropPosX, "Abscissa of the first pixel for output Crop. This crop information will be added to the stream header and will be applied by the decoder", {
    { isOnlyCodec(Codec::Avc), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 80 },
    { isOnlyCodec(Codec::Hevc), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 256 },
    { aomCodecs(), 0, AL_ENC_NUM_CORES * AL_ENC_CORE_MAX_WIDTH - 128 },
    { isOnlyCodec(Codec::Jpeg), 0, AL_ENC_CORE_MAX_WIDTH_JPEG - 16 },
  });
  parser.addArith(curSection, "CropPosY", cfg.Settings.tChParam[0].uOutputCropPosY, "Ordinate of the first pixel for output Crop. This crop information will be added to the stream header and will be applied by the decoder", {
    { isOnlyCodec(Codec::Avc), 96, 4096 - 96 },
    { filterCodecs({ Codec::Hevc, Codec::Vp9, Codec::Av1 }), 128, 4096 - 128 },
    { isOnlyCodec(Codec::Jpeg), 2, 16384 - 2 },
  });
  parser.addArith(curSection, "CropWidth", cfg.Settings.tChParam[0].uOutputCropWidth, "Width of the output crop region. This crop information will be added to the stream header and will be applied by the decoder", {
    { isOnlyCodec(Codec::Avc), 80, 4016 },
    { isOnlyCodec(Codec::Hevc), 256, 3840 },
    { aomCodecs(), 128, 3968 },
    { isOnlyCodec(Codec::Jpeg), 16, 16368 },
  });
  parser.addArith(curSection, "CropHeight", cfg.Settings.tChParam[0].uOutputCropHeight, "Height of the output crop region. This crop information will be added to the stream header and will be applied by the decoder", {
    { isOnlyCodec(Codec::Avc), 96, 4000 },
    { filterCodecs({ Codec::Hevc, Codec::Vp9, Codec::Av1 }), 128, 3968 },
    { isOnlyCodec(Codec::Jpeg), 2, 16382 },
  });
}

static void SetFpsAndClkRatio(int value, uint16_t& iFps, uint16_t& iClkRatio)
{
  iFps = value / 1000;
  iClkRatio = 1000;

  if(value %= 1000)
    iClkRatio += (1000 - value) / ++iFps;
}

static void populateRCParam(Section curSection, ConfigParser& parser, AL_TRCParam& RCParam)
{
  map<string, EnumDescription<int>> rateCtrlModes;
  rateCtrlModes["CONST_QP"] = { AL_RC_CONST_QP, "No rate control, all pictures use the same QP defined by the SliceQP parameter", aomituCodecs() };
  rateCtrlModes["CBR"] = { AL_RC_CBR, "Use Constant BitRate control", aomituCodecs() };
  rateCtrlModes["VBR"] = { AL_RC_VBR, "Use Variable BitRate control", aomituCodecs() };
  rateCtrlModes["LOW_LATENCY"] = { AL_RC_LOW_LATENCY, "Use specific Variable BitRate for low latency application. The IP should include the optional hardware rate control block.", aomituCodecs() };
  rateCtrlModes["CAPPED_VBR"] = { AL_RC_CAPPED_VBR, "Use Variable BitRate control with capped quality based on a coarse PSNR threshold", aomituCodecs() };
  rateCtrlModes["PLUGIN"] = { AL_RC_PLUGIN, "Use the rate control plugin", aomituCodecs() };

  parser.addEnum(curSection, "RateCtrlMode", RCParam.eRCMode, rateCtrlModes, "Selects the way the bit rate is controlled");
  parser.addNote(curSection, "RateCtrlMode", "When RateCtrlMode is CBR or VBR: QP for each slice is given by the bit rate controller to match the requested BitRate. SliceQP is used as initial QP. The value AUTO let's the rate controller select the initial QP. The sliceQP = AUTO is the recommended mode when RateCtrlMode is CBR or VBR.");
  parser.addNote(curSection, "RateCtrlMode", "When RateCtrlMode is CONST_QP: All Slices have the same QP defined by SliceQP. BitRate and MaxBitRate are ignored.");
  parser.addSeeAlso(curSection, "RateCtrlMode", { curSection, "SliceQP" });
  parser.addSeeAlso(curSection, "RateCtrlMode", { curSection, "BitRate" });
  parser.addSeeAlso(curSection, "RateCtrlMode", { curSection, "MaxBitRate" });
  parser.addArithMultipliedByConstant(curSection, "BitRate", RCParam.uTargetBitRate, 1000, "Target bit rate in Kbits/s. Unused if RateCtrlMode=CONST_QP");
  parser.addSeeAlso(curSection, "BitRate", { curSection, "RateCtrlMode" });
  parser.addArithMultipliedByConstant(curSection, "MaxBitRate", RCParam.uMaxBitRate, 1000, "Maximum bit rate in Kbits/s. This is used in VBR. This should be the maximum transmission bandwidth available. (The encoder shouldn't exceed this value on average on a moving window of CPBSize seconds). This option is automatically set to BitRate in CBR.");
  parser.addNote(curSection, "MaxBitRate", "When RateCtrlMode is CBR, MaxBitRate shall be set to the same value as BitRate. When RateCtrlMode is VBR or LOW_LATENCY, MaxBitRate shall be greater than BitRate (usual 1.5 or 2 x BitRate)");
  parser.addSeeAlso(curSection, "MaxBitRate", { curSection, "RateCtrlMode" });
  parser.addSeeAlso(curSection, "MaxBitRate", { curSection, "BitRate" });
  parser.addSeeAlso(curSection, "MaxBitRate", { curSection, "CPBSize" });
  parser.addCustom(curSection, "FrameRate", [&](std::deque<Token>& tokens)
  {
    auto tmp = parseArithmetic<double>(tokens) * 1000;
    SetFpsAndClkRatio(tmp, RCParam.uFrameRate, RCParam.uClkRatio);
  }, [&]()
  {
    double frameRate = RCParam.uFrameRate * 1000.0 / RCParam.uClkRatio;
    return std::to_string(frameRate);
  }, "Number of frames per second");
  map<string, EnumDescription<int>> autoEnum;
  autoEnum["AUTO"] = { -1, "The Slice Quantization Parameter is determined by the software to be the best fit of the HDR", aomituCodecs() };
  autoEnum["OPTIMIZED"] = { -2, "The Slice Quantization Paramieter is optimized to be the best fit of the specific sequence", aomituCodecs() };
  vector<ArithInfo<int>> qpArithInfo {
    {
      ituCodecs(), 0, 51
    }, {
      aomCodecs(), 1, 255
    }, {
      isOnlyCodec(Codec::Jpeg), 1, 100
    },
  };
  parser.addArithOrEnum(curSection, "SliceQP", RCParam.iInitialQP, autoEnum, "Quantization parameter. When RateCtrlMode == CONST_QP, the specified QP is applied to all slices. In other cases, the specified QP is used as initial QP. When RATECtrlMode = CBR the specified QP is used as initial QP. The SliceQP = AUTO is the recommended mode when RateCtrlMode is CBR or VBR.", qpArithInfo);
  parser.addSeeAlso(curSection, "SliceQP", { curSection, "RateCtrlMode" });
  parser.addArithOrEnum(curSection, "MaxQP", RCParam.iMaxQP, autoEnum, "Maximum QP value allowed", qpArithInfo);
  parser.addArithOrEnum(curSection, "MinQP", RCParam.iMinQP, autoEnum, "Minimum QP value allowed. This parameter is especially useful when using VBR rate control. In VBR, the value AUTO can be used to let the encoder select the MinQP according to SliceQP", qpArithInfo);
  parser.addArithFunc<decltype(RCParam.uInitialRemDelay), double>(curSection, "InitialDelay", RCParam.uInitialRemDelay, [](double value)
  {
    return (decltype(RCParam.uInitialRemDelay))(value * 90000);
  }, [](decltype(RCParam.uInitialRemDelay) value) { return (double)value / 90000; }, "Specifies the initial removal delay as specified in the HRD model, in seconds.");
  parser.addNote(curSection, "InitialDelay", "Not used when RateCtrlMode = CONST_QP");
  parser.addSeeAlso(curSection, "InitialDelay", { curSection, "RateCtrlMode" });
  parser.addArithFunc<decltype(RCParam.uCPBSize), double>(curSection, "CPBSize", RCParam.uCPBSize, [](double value)
  {
    return (decltype(RCParam.uCPBSize))(value * 90000);
  }, [](decltype(RCParam.uCPBSize) value) { return (double)value / 90000; }, "Specifies the size of the Coded Picture Buffer as specified in the HRD model, in seconds.");
  parser.addNote(curSection, "CPBSize", "Not used when RateCtrlMode = CONST_QP");
  parser.addSeeAlso(curSection, "CPBSize", { curSection, "RateCtrlMode" });
  parser.addArithOrEnum(curSection, "IPDelta", RCParam.uIPDelta, autoEnum, "Specifies the QP difference between I and P frames");
  parser.addArithOrEnum(curSection, "PBDelta", RCParam.uPBDelta, autoEnum, "Specifies the QP difference between P and B frames");
  parser.addFlag(curSection, "ScnChgResilience", RCParam.eOptions, AL_RC_OPT_SCN_CHG_RES, "Enables a rate control feature allowing a more conservative behavior in order to improve the video quality on scene change. However, the overall quality can be decreased.");
  parser.addBool(curSection, "UseGoldenRef", RCParam.bUseGoldenRef);
  parser.addArithFunc<decltype(RCParam.uMaxPSNR), double>(curSection, "MaxQuality", RCParam.uMaxPSNR, [](double value)
  {
    return std::max(std::min((decltype(RCParam.uMaxPSNR))((value + 28.0) * 100), (decltype(RCParam.uMaxPSNR)) 4800), (decltype(RCParam.uMaxPSNR)) 2800);
  }, [](decltype(RCParam.uMaxPSNR) value) { return (double)value / 100 - 28.0; });
  map<string, EnumDescription<int>> MaxPictureSizeEnums;
  MaxPictureSizeEnums["DISABLE"] = { 0, "Disable", aomituCodecs() };
  parser.addCustom(curSection, "MaxPictureSizeInBits", [MaxPictureSizeEnums, &RCParam](std::deque<Token>& tokens)
  {
    int size = 0;

    if(hasOnlyOneIdentifier(tokens))
      size = parseEnum(tokens, MaxPictureSizeEnums);
    else
      size = parseArithmetic<int>(tokens);

    for(auto& picSize :  RCParam.pMaxPictureSize)
      picSize = size;
  }, [&]()
  {
    return "";
  }, "Specifies a coarse picture size in bits that shouldn't be exceeded. Available Values: <Arithmetic expression> or DISABLE to not constrain the picture size");
  parser.addArithOrEnum(curSection, "MaxPictureSizeInBits.I", RCParam.pMaxPictureSize[AL_SLICE_I], MaxPictureSizeEnums, "Specifies a coarse size (in bits) for I-frame that shouldn't be exceeded");
  parser.addArithOrEnum(curSection, "MaxPictureSizeInBits.P", RCParam.pMaxPictureSize[AL_SLICE_P], MaxPictureSizeEnums, "Specifies a coarse size (in bits) for P-frame that shouldn't be exceeded");
  parser.addArithOrEnum(curSection, "MaxPictureSizeInBits.B", RCParam.pMaxPictureSize[AL_SLICE_B], MaxPictureSizeEnums, "Specifies a coarse size (in bits) for B-frame that shouldn't be exceeded");
  parser.addCustom(curSection, "MaxPictureSize", [MaxPictureSizeEnums, &RCParam](std::deque<Token>& tokens)
  {
    int size = 0;

    if(hasOnlyOneIdentifier(tokens))
      size = parseEnum(tokens, MaxPictureSizeEnums);
    else
      size = parseArithmetic<int>(tokens) * 1000;

    for(auto& picSize :  RCParam.pMaxPictureSize)
      picSize = size;
  }, [&]()
  {
    return "";
  }, "Specifies a coarse picture size in Kbits that shouldn't be exceeded. Available Values: <Arithmetic expression> or DISABLE to not constrain the picture size");
  parser.addArithFuncOrEnum<remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_I])>::type, int>(curSection, "MaxPictureSize.I", RCParam.pMaxPictureSize[AL_SLICE_I], [](int size)
  {
    return (remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_I])>::type)size * 1000;
  }, [](remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_I])>::type size)
  {
    return (int)size / 1000;
  }, MaxPictureSizeEnums, "Specifies a coarse size (in Kbits) for I-frame that shouldn't be exceeded");
  parser.addArithFuncOrEnum<remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_P])>::type, int>(curSection, "MaxPictureSize.P", RCParam.pMaxPictureSize[AL_SLICE_P], [](int size)
  {
    return (remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_P])>::type)size * 1000;
  }, [](remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_P])>::type size)
  {
    return (int)size / 1000;
  }, MaxPictureSizeEnums, "Specifies a coarse size (in Kbits) for P-frame that shouldn't be exceeded");
  parser.addArithFuncOrEnum<remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_B])>::type, int>(curSection, "MaxPictureSize.B", RCParam.pMaxPictureSize[AL_SLICE_B], [](int size)
  {
    return (remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_B])>::type)size * 1000;
  }, [](remove_reference<decltype(RCParam.pMaxPictureSize[AL_SLICE_B])>::type size)
  {
    return (int)size / 1000;
  }, MaxPictureSizeEnums, "Specifies a coarse size (in Kbits) for B-frame that shouldn't be exceeded");
  parser.addFlag(curSection, "EnableSkip", RCParam.eOptions, AL_RC_OPT_ENABLE_SKIP);
  parser.addFlag(curSection, "SCPrevention", RCParam.eOptions, AL_RC_OPT_SC_PREVENTION, "Enable Scene Change Prevention");
  parser.addArith(curSection, "MaxConsecutiveSkip", RCParam.uMaxConsecSkip, "Maximum consecutive skip pictures allowed");
}

static void populateRateControlSection(ConfigParser& parser, ConfigFile& cfg)
{
  populateRCParam(Section::RateControl, parser, cfg.Settings.tChParam[0].tRCParam);
}

static void populateGopSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto& GopParam = cfg.Settings.tChParam[0].tGopParam;
  auto curSection = Section::Gop;
  map<string, EnumDescription<int>> gopCtrlModes;
  gopCtrlModes["DEFAULT_GOP"] = { AL_GOP_MODE_DEFAULT, "Basic Group Of Pictures settings.", aomituCodecs() };
  gopCtrlModes["LOW_DELAY_P"] = { AL_GOP_MODE_LOW_DELAY_P, "Group Of Pictures pattern with a single I-picture at the beginning followed with P-pictures only. Each P-picture uses the previous picture as reference.", aomituCodecs() };
  gopCtrlModes["LOW_DELAY_B"] = { AL_GOP_MODE_LOW_DELAY_B, "Group Of Pictures pattern with a single I-picture at the beginning followed with B-pictures only. Each B-picture use the previous picture as first reference; the second reference depends on the [GOP]Gop.Length parameter.", aomituCodecs() };
  gopCtrlModes["PYRAMIDAL_GOP"] = { AL_GOP_MODE_PYRAMIDAL, "Advanced Group Of Pictures pattern with hierarchical B-pictures. The size of the hierarchy depends on the [GOP]Gop.NumB parameter.", aomituCodecs() };
  gopCtrlModes["DEFAULT_GOP_B"] = { AL_GOP_MODE_DEFAULT_B, "Basic Group Of Pictures settings, includes only B-pictures.", aomituCodecs() };
  gopCtrlModes["PYRAMIDAL_GOP_B"] = { AL_GOP_MODE_PYRAMIDAL_B, "Advanced Group Of Pictures pattern width hierarchical B-pictures, includes only B-pictures.", aomituCodecs() };
  gopCtrlModes["ADAPTIVE_GOP"] = { AL_GOP_MODE_ADAPTIVE, "Advanced Group Of Pictures pattern width adaptive B-pictures.", aomituCodecs() };
  parser.addEnum(curSection, "GopCtrlMode", GopParam.eMode, gopCtrlModes, "Specifies the Group Of Pictures configuration mode.");
  parser.addArith(curSection, "Gop.Length", GopParam.uGopLength, "Group Of Pictures length in frames including the I picture. 0 for Intra only. Used only when GopCtrlMode is set to DEFAULT_GOP or PYRAMIDAL_GOP. Should be set to 0 for Intra only.", {
    { aomituCodecs(), 0, 1000 }
  });
  parser.addNote(curSection, "Gop.Length", "When [GOP]GopCtrlMode is PYRAMIDAL_GOP, the [GOP]Gop.Length shall be set to a value matching the following equation: (N x ([GOP]Gop.NumB +1) + 1) with N >= 1");
  map<string, EnumDescription<int>> freqIdrEnums;
  freqIdrEnums["DISABLE"] = { -1, "Disable IDR insertion", ituCodecs() };
  freqIdrEnums["SC_ONLY"] = { INT32_MAX, "Intra Decoding Refresh on Scene Change only", ituCodecs() };
  parser.addArithOrEnum(curSection, "Gop.FreqIDR", GopParam.uFreqIDR, freqIdrEnums, "Specifies the minimum number of frames between two IDR pictures. IDR insertion depends on the position of the GOP boundary", {
    { ituCodecs(), 0, INT32_MAX - 1 }
  });
  parser.addBool(curSection, "Gop.EnableLT", GopParam.bEnableLT, "Enables the Long Term Reference Picture (LTRP) feature");
  parser.addNote(curSection, "Gop.EnableLT", "Must be set to TRUE when using LTRP feature in CmdFile.");
  parser.addSeeAlso(curSection, "Gop.EnableLT", { Section::Input, "CmdFile" });
  parser.addSeeAlso(curSection, "Gop.EnableLT", { Section::Input, "Format" });
  parser.addArith(curSection, "Gop.FreqLT", GopParam.uFreqLT, "Specifies the Long Term reference picture refresh frequency in number of frames", {
    { allCodecs(), 0, UINT32_MAX - 1 }
  });
  parser.addArith(curSection, "Gop.NumB", GopParam.uNumB, "Maximum number of consecutive B frames in a Group Of Pictures. Used only when GopCtrlMode is set to DEFAULT_GOP, LOW_DELAY_B or PYRAMIDAL_GOP.", {
    { ituCodecs(), 0, 4 },
    { aomCodecs(), 0, 2 },
  });
  string numBNote = "When [GOP]GopCtrlMode != DEFAULT_GOP";
  numBNote += ", [GOP]Gop.NumB may go up to " + to_string(AL_MAX_NUM_B_PICT) + ".";
  numBNote += "When GopCtrlMode is set to DEFAULT_GOP, Gop.NumB shall be in range 0 to 2. When GopCtrlMode is set to PYRAMIDAL_GOP, Gop.NumB shall be 3, 5 or 7";
  parser.addNote(curSection, "Gop.NumB", numBNote.c_str());
  parser.addNote(curSection, "Gop.NumB", "Typical value for Gop.NumB is between 0 and 2. Increasing the number of consecutive B pictures has three effects. First, it requires more frame buffers to store the source pictures, which can be an issue with large resolution. \
  The second is, that it increases the latency at both sides: encode and decode. The last one is, that it can also decrease the video quality (depending on the video content) because the P-pictures are then far from their reference picture (the previous P or I picture) \
   and it could be difficult to find the motion vectors.");
  parser.addSeeAlso(curSection, "Gop.NumB", { curSection, "GopCtrlMode" });
  parser.addArray(curSection, "Gop.TempDQP", GopParam.tempDQP, "Specifies a deltaQP for pictures with temporal id 1 to 4");
  map<string, EnumDescription<int>> gdrModes;
  gdrModes["GDR_HORIZONTAL"] = { AL_GDR_HORIZONTAL, "Gradual Decoding Refresh using a horizontal bar moving from top to bottom", aomituCodecs() };
  gdrModes["GDR_VERTICAL"] = { AL_GDR_VERTICAL, "Gradual Decoding Refresh using a vertical bar moving from left to right", isOnlyCodec(Codec::Avc) };
  gdrModes["DISABLE"] = { AL_GDR_OFF, "Disable Gradual Decoding Refresh", aomituCodecs() };
  gdrModes["GDR_OFF"] = { AL_GDR_OFF, "Disable Gradual Decoding Refresh", aomituCodecs() };
  parser.addEnum(curSection, "Gop.GdrMode", GopParam.eGdrMode, gdrModes, "When GopCtrlMode is LOW_DELAY_{P,B}, this parameter specifies whether a Gradual Decoder Refresh scheme should be used or not");
  parser.addNote(curSection, "Gop.GdrMode", "When GDR is enabled, the Gop.FreqIDR specifies the frequency at which the refresh pattern should happen. To allow full picture refreshing, this parameter should be greater than the number of CTB/MB rows (GDR_HORIZONTAL) or columns (GDR_VERTICAL).");
}

static void populateSettingsSection(ConfigParser& parser, ConfigFile& cfg, Temporary& temp, std::ostream& warnStream)
{
  (void)warnStream;
  auto curSection = Section::Settings;
  map<string, EnumDescription<int>> profiles;

  profiles["HEVC_MONO10"] = { AL_PROFILE_HEVC_MONO10, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MONO"] = { AL_PROFILE_HEVC_MONO, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444_STILL"] = { AL_PROFILE_HEVC_MAIN_444_STILL, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444_10_INTRA"] = { AL_PROFILE_HEVC_MAIN_444_10_INTRA, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444_INTRA"] = { AL_PROFILE_HEVC_MAIN_444_INTRA, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444_10"] = { AL_PROFILE_HEVC_MAIN_444_10, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444"] = { AL_PROFILE_HEVC_MAIN_444, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_422_10_INTRA"] = { AL_PROFILE_HEVC_MAIN_422_10_INTRA, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_422_10"] = { AL_PROFILE_HEVC_MAIN_422_10, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_422_12"] = { AL_PROFILE_HEVC_MAIN_422_12, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_444_12"] = { AL_PROFILE_HEVC_MAIN_444_12, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_422"] = { AL_PROFILE_HEVC_MAIN_422, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_INTRA"] = { AL_PROFILE_HEVC_MAIN_INTRA, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN_STILL"] = { AL_PROFILE_HEVC_MAIN_STILL, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN10_INTRA"] = { AL_PROFILE_HEVC_MAIN10_INTRA, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN10"] = { AL_PROFILE_HEVC_MAIN10, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN12"] = { AL_PROFILE_HEVC_MAIN12, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  profiles["HEVC_MAIN"] = { AL_PROFILE_HEVC_MAIN, "See HEVC/H.265 specification", isOnlyCodec(Codec::Hevc) };
  /* Baseline is mapped to Constrained_Baseline */
  profiles["AVC_BASELINE"] = { AL_PROFILE_AVC_C_BASELINE, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_C_BASELINE"] = { AL_PROFILE_AVC_C_BASELINE, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_MAIN"] = { AL_PROFILE_AVC_MAIN, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH10_INTRA"] = { AL_PROFILE_AVC_HIGH10_INTRA, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH10"] = { AL_PROFILE_AVC_HIGH10, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH_422_INTRA"] = { AL_PROFILE_AVC_HIGH_422_INTRA, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH_422"] = { AL_PROFILE_AVC_HIGH_422, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH"] = { AL_PROFILE_AVC_HIGH, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_C_HIGH"] = { AL_PROFILE_AVC_C_HIGH, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_PROG_HIGH"] = { AL_PROFILE_AVC_PROG_HIGH, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_CAVLC_444"] = { AL_PROFILE_AVC_CAVLC_444, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH_444_INTRA"] = { AL_PROFILE_AVC_HIGH_444_INTRA, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["AVC_HIGH_444_PRED"] = { AL_PROFILE_AVC_HIGH_444_PRED, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_HIGH10_INTRA_CBG"] = { AL_PROFILE_XAVC_HIGH10_INTRA_CBG, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_HIGH10_INTRA_VBR"] = { AL_PROFILE_XAVC_HIGH10_INTRA_VBR, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_HIGH_422_INTRA_CBG"] = { AL_PROFILE_XAVC_HIGH_422_INTRA_CBG, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_HIGH_422_INTRA_VBR"] = { AL_PROFILE_XAVC_HIGH_422_INTRA_VBR, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_LONG_GOP_MAIN_MP4"] = { AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_LONG_GOP_HIGH_MP4"] = { AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_LONG_GOP_HIGH_MXF"] = { AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  profiles["XAVC_LONG_GOP_HIGH_422_MXF"] = { AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF, "See AVC/H.264 specification", isOnlyCodec(Codec::Avc) };
  parser.addEnum(curSection, "Profile", cfg.Settings.tChParam[0].eProfile, profiles, "Specifies the profile to which the bitstream conforms");
  parser.addArithFunc<decltype(cfg.Settings.tChParam[0].uLevel), double>(curSection, "Level", cfg.Settings.tChParam[0].uLevel, [](double value)
  {
    return decltype(cfg.Settings.tChParam[0].uLevel)((value + 0.01f) * 10);
  }, [](decltype(cfg.Settings.tChParam[0].uLevel) value) { return (double)value / 10; }, "Specifies the Level to which the bitstream conforms", {
    { isOnlyCodec(Codec::Hevc), 1.0, 5.1 },
    { isOnlyCodec(Codec::Avc), 1.0, 5.2 },
    { isOnlyCodec(Codec::Av1), 2.0, 7.3 },
  });
  map<string, EnumDescription<int>> tiers;
  tiers["MAIN_TIER"] = { 0, "Use Main Tier profile", isOnlyCodec(Codec::Hevc) };
  tiers["HIGH_TIER"] = { 1, "Use High Tier profile", isOnlyCodec(Codec::Hevc) };
  parser.addEnum(curSection, "Tier", cfg.Settings.tChParam[0].uTier, tiers, "Specifies the tier to which the bitstream conforms");
  parser.addArith(curSection, "NumSlices", cfg.Settings.tChParam[0].uNumSlices, "Number of slices used for each frame. Each slice contains one or more full LCU row(s) and they are spread over the frame as regularly as possible", {
    { aomituCodecs(), 1, }
  });
  map<string, EnumDescription<int>> sliceSizeEnums;
  sliceSizeEnums["DISABLE"] = { 0, "Disable Slice size", ituCodecs() };
  parser.addArithFuncOrEnum<decltype(cfg.Settings.tChParam[0].uSliceSize), int>(curSection, "SliceSize", cfg.Settings.tChParam[0].uSliceSize, [](int sliceSize)
  {
    return (decltype(cfg.Settings.tChParam[0].uSliceSize))sliceSize * 95 / 100;
  }, [](decltype(cfg.Settings.tChParam[0].uSliceSize) sliceSize)
  {
    return (int)sliceSize * 100 / 95;
  }, sliceSizeEnums, "Target Slice Size. If set to 0, slices are defined by the NumSlices parameter, Otherwise it specifies the target slice size, in bytes, that the encoder uses to automatically split the bitstream into approximately equally sized slices, with a granularity of one LCU. This impacts performance, adding an overhead of one LCU per slice to the processing time of a command.", {
    { ituCodecs(), 100, UINT16_MAX },
  });
  parser.addNote(curSection, "SliceSize", "This parameter is directly sent to the Encoder IP and specifies only the size of the Slice Data. It doesn't include any margin for the Slice header. So it is recommended to set the SliceSize parameter with the target value lowered by 5%. For example if your target value is 1500 bytes per slice, you should set SliceSize equals to 1425.");

  parser.addNote(curSection, "SliceSize", "Not supported in AVC multicores");
  parser.addBool(curSection, "DependentSlice", cfg.Settings.bDependentSlice, "When there are several slices per frame (e.g. NumSlices is greater than 1 or SliceSize is greater than 0), this parameter specifies whether the additional slices are dependent slice segments or regular slices", isOnlyCodec(Codec::Hevc));
  parser.addSeeAlso(curSection, "DependentSlice", { curSection, "NumSlices" });
  parser.addSeeAlso(curSection, "DependentSlice", { curSection, "SliceSize" });
  parser.addBool(curSection, "SubframeLatency", cfg.Settings.tChParam[0].bSubframeLatency, "Enable the subframe latency mode. This option requires to use multiple slices per frame.");
  parser.addSeeAlso(curSection, "SubframeLatency", { curSection, "NumSlices" });
  parser.addBool(curSection, "UniformSliceType", cfg.Settings.tChParam[0].bUseUniformSliceType, "Enable uniform slice types encoding", isOnlyCodec(Codec::Avc));
  map<string, EnumDescription<int>> startCodeBytesAligned;
  startCodeBytesAligned["SC_AUTO"] = { AL_START_CODE_AUTO, "Software select the start code bytes aligment", ituCodecs() };
  startCodeBytesAligned["SC_3_BYTES"] = { AL_START_CODE_3_BYTES, "Force start code 3-bytes alignment: 0x00 0x00 0x01", ituCodecs() };
  startCodeBytesAligned["SC_4_BYTES"] = { AL_START_CODE_4_BYTES, "Force start code 4-bytes alignment: 0x00 0x00 0x00 0x01", ituCodecs() };
  parser.addEnum(curSection, "StartCode", cfg.Settings.tChParam[0].eStartCodeBytesAligned, startCodeBytesAligned, "Start code size alignment");
  map<string, EnumDescription<int>> seis;
  seis["SEI_NONE"] = { AL_SEI_NONE, "No SEI message is inserted", ituCodecs() };
  seis["SEI_BP"] = { AL_SEI_BP, "Buffering Period SEI", ituCodecs() };
  seis["SEI_PT"] = { AL_SEI_PT, "Picture Timing SEI", ituCodecs() };
  seis["SEI_RP"] = { AL_SEI_RP, "Recovery Point SEI", ituCodecs() };
  seis["SEI_MDCV"] = { AL_SEI_MDCV, "", ituCodecs() };
  seis["SEI_CLL"] = { AL_SEI_CLL, "", ituCodecs() };
  seis["SEI_ATC"] = { AL_SEI_ATC, "", ituCodecs() };
  seis["SEI_ST2094_10"] = { AL_SEI_ST2094_10, "", ituCodecs() };
  seis["SEI_ST2094_40"] = { AL_SEI_ST2094_40, "", ituCodecs() };
  seis["SEI_ALL"] = { (int)AL_SEI_ALL, "means SEI_PT | SEI_BP | SEI_RP", ituCodecs() };
  parser.addEnum(curSection, "EnableSEI", cfg.Settings.uEnableSEI, seis, "Determines which Supplemental Enhancement Information are sent with the stream");
  parser.addBool(curSection, "EnableAUD", cfg.Settings.bEnableAUD, "Determines if Access Unit Delimiter are added to the stream or not", ituCodecs());
  map<string, EnumDescription<int>> fillerEnums;
  fillerEnums["DISABLE"] = { AL_FILLER_DISABLE, "Disable Filler data", aomituCodecs() };
  fillerEnums["ENABLE"] = { AL_FILLER_ENC, "Filler data is written by the encoder", aomituCodecs() }; // for backward compatibility
  fillerEnums["ENC"] = { AL_FILLER_ENC, "Filler data is written by the encoder", aomituCodecs() };
  fillerEnums["APP"] = { AL_FILLER_APP, "Filler data is written by the application", aomituCodecs() };
  parser.addEnum(curSection, "EnableFillerData", cfg.Settings.eEnableFillerData, fillerEnums, "Specifies whether Filler Data are inserted when target bitrate cannot be achieved in CBR");
  map<string, EnumDescription<int>> aspectRatios;
  aspectRatios["ASPECT_RATIO_AUTO"] = { AL_ASPECT_RATIO_AUTO, "Aspect Ratio is selected automatically", ituCodecs() };
  aspectRatios["ASPECT_RATIO_1_1"] = { AL_ASPECT_RATIO_1_1, "1:1 Aspect Ratio", ituCodecs() };
  aspectRatios["ASPECT_RATIO_4_3"] = { AL_ASPECT_RATIO_4_3, "4:3 Aspect Ratio", ituCodecs() };
  aspectRatios["ASPECT_RATIO_16_9"] = { AL_ASPECT_RATIO_16_9, "16:9 Aspect ratio", ituCodecs() };
  aspectRatios["ASPECT_RATIO_NONE"] = { AL_ASPECT_RATIO_NONE, "Aspect ratio information is not present in the stream", ituCodecs() };
  parser.addEnum(curSection, "AspectRatio", cfg.Settings.eAspectRatio, aspectRatios, "Selects the display aspect ratio of the video sequence to be written in SPS/VUI");

  map<string, EnumDescription<int>> colourDescriptions;
  colourDescriptions["COLOUR_DESC_RESERVED"] = { AL_COLOUR_DESC_RESERVED, "Reserved", ituCodecs() };
  colourDescriptions["COLOUR_DESC_BT_709"] = { AL_COLOUR_DESC_BT_709, "BT.709", ituCodecs() };
  colourDescriptions["COLOUR_DESC_UNSPECIFIED"] = { AL_COLOUR_DESC_UNSPECIFIED, "Unspecified", ituCodecs() };
  colourDescriptions["COLOUR_DESC_BT_470_NTSC"] = { AL_COLOUR_DESC_BT_470_NTSC, "BT.470 NTSC", ituCodecs() };
  colourDescriptions["COLOUR_DESC_BT_601_PAL"] = { AL_COLOUR_DESC_BT_601_PAL, "BT.601 PAL", ituCodecs() };
  colourDescriptions["COLOUR_DESC_BT_601_NTSC"] = { AL_COLOUR_DESC_BT_601_NTSC, "BT.601 NTSC", ituCodecs() };
  colourDescriptions["COLOUR_DESC_SMPTE_240M"] = { AL_COLOUR_DESC_SMPTE_240M, "SMPTE 240M", ituCodecs() };
  colourDescriptions["COLOUR_DESC_GENERIC_FILM"] = { AL_COLOUR_DESC_GENERIC_FILM, "Generic film", ituCodecs() };
  colourDescriptions["COLOUR_DESC_BT_2020"] = { AL_COLOUR_DESC_BT_2020, "BT.2020", ituCodecs() };
  colourDescriptions["COLOUR_DESC_SMPTE_ST_428"] = { AL_COLOUR_DESC_SMPTE_ST_428, "SMPTE ST.428", ituCodecs() };
  colourDescriptions["COLOUR_DESC_SMPTE_RP_431"] = { AL_COLOUR_DESC_SMPTE_RP_431, "SMPTE RP.431", ituCodecs() };
  colourDescriptions["COLOUR_DESC_SMPTE_EG_432"] = { AL_COLOUR_DESC_SMPTE_EG_432, "SMPTE EG.432", ituCodecs() };
  colourDescriptions["COLOUR_DESC_EBU_3213"] = { AL_COLOUR_DESC_EBU_3213, "EBU.3213", ituCodecs() };
  parser.addEnum(curSection, "ColourDescription", cfg.Settings.eColourDescription, colourDescriptions, "Defines the colorimetry information that is written in the SPS/VUI header");

  map<string, EnumDescription<int>> transferCharacteristics;
  transferCharacteristics["TRANSFER_BT_709"] = { AL_TRANSFER_CHARAC_BT_709, "", ituCodecs() };
  transferCharacteristics["TRANSFER_UNSPECIFIED"] = { AL_TRANSFER_CHARAC_UNSPECIFIED, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_470_SYSTEM_M"] = { AL_TRANSFER_CHARAC_BT_470_SYSTEM_M, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_470_SYSTEM_B"] = { AL_TRANSFER_CHARAC_BT_470_SYSTEM_B, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_601"] = { AL_TRANSFER_CHARAC_BT_601, "", ituCodecs() };
  transferCharacteristics["TRANSFER_SMPTE_240M"] = { AL_TRANSFER_CHARAC_SMPTE_240M, "", ituCodecs() };
  transferCharacteristics["TRANSFER_LINEAR"] = { AL_TRANSFER_CHARAC_LINEAR, "", ituCodecs() };
  transferCharacteristics["TRANSFER_LOG"] = { AL_TRANSFER_CHARAC_LOG, "", ituCodecs() };
  transferCharacteristics["TRANSFER_LOG_EXTENDED"] = { AL_TRANSFER_CHARAC_LOG_EXTENDED, "", ituCodecs() };
  transferCharacteristics["TRANSFER_IEC_61966_2_4"] = { AL_TRANSFER_CHARAC_IEC_61966_2_4, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_1361"] = { AL_TRANSFER_CHARAC_BT_1361, "", ituCodecs() };
  transferCharacteristics["TRANSFER_IEC_61966_2_1"] = { AL_TRANSFER_CHARAC_IEC_61966_2_1, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_2020_10B"] = { AL_TRANSFER_CHARAC_BT_2020_10B, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_2020_12B"] = { AL_TRANSFER_CHARAC_BT_2020_12B, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_2100_PQ"] = { AL_TRANSFER_CHARAC_BT_2100_PQ, "", ituCodecs() };
  transferCharacteristics["TRANSFER_SMPTE_428"] = { AL_TRANSFER_CHARAC_SMPTE_428, "", ituCodecs() };
  transferCharacteristics["TRANSFER_BT_2100_HLG"] = { AL_TRANSFER_CHARAC_BT_2100_HLG, "", ituCodecs() };
  parser.addEnum(curSection, "TransferCharac", cfg.Settings.eTransferCharacteristics, transferCharacteristics, "Specifies the reference opto-electronic transfer characteristic function (HDR setting)");

  map<string, EnumDescription<int>> colourMatrices;
  colourMatrices["COLOUR_MAT_GBR"] = { AL_COLOUR_MAT_COEFF_GBR, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_709"] = { AL_COLOUR_MAT_COEFF_BT_709, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_UNSPECIFIED"] = { AL_COLOUR_MAT_COEFF_UNSPECIFIED, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_USFCC_CFR"] = { AL_COLOUR_MAT_COEFF_USFCC_CFR, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_601_625"] = { AL_COLOUR_MAT_COEFF_BT_601_625, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_601_525"] = { AL_COLOUR_MAT_COEFF_BT_601_525, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_SMPTE_240M"] = { AL_COLOUR_MAT_COEFF_BT_SMPTE_240M, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_YCGCO"] = { AL_COLOUR_MAT_COEFF_BT_YCGCO, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_2100_YCBCR"] = { AL_COLOUR_MAT_COEFF_BT_2100_YCBCR, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_2020_CLS"] = { AL_COLOUR_MAT_COEFF_BT_2020_CLS, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_SMPTE_2085"] = { AL_COLOUR_MAT_COEFF_SMPTE_2085, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_CHROMA_DERIVED_NCLS"] = { AL_COLOUR_MAT_COEFF_CHROMA_DERIVED_NCLS, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_CHROMA_DERIVED_CLS"] = { AL_COLOUR_MAT_COEFF_CHROMA_DERIVED_CLS, "", ituCodecs() };
  colourMatrices["COLOUR_MAT_BT_2100_ICTCP"] = { AL_COLOUR_MAT_COEFF_BT_2100_ICTCP, "", ituCodecs() };
  parser.addEnum(curSection, "ColourMatrix", cfg.Settings.eColourMatrixCoeffs, colourMatrices, "Specifies the matrix coefficients used in deriving luma and chroma signals from RGB (HDR setting)");

  map<string, EnumDescription<int>> chromaModes;
  chromaModes["CHROMA_MONO"] = { AL_CHROMA_MONO, "The stream is encoded in monochrome", allCodecs() };
  chromaModes["CHROMA_4_0_0"] = { AL_CHROMA_4_0_0, "Same as CHROMA_MONO", allCodecs() };
  chromaModes["CHROMA_4_2_0"] = { AL_CHROMA_4_2_0, "The stream is encoded with 4:2:0 chroma subsampling", allCodecs() };
  chromaModes["CHROMA_4_2_2"] = { AL_CHROMA_4_2_2, "The stream is encoded with 4:2:2 chroma subsampling", allCodecs() };

  parser.addCustom(curSection, "ChromaMode", [chromaModes, &cfg](std::deque<Token>& tokens)
  {
    AL_EChromaMode mode = (AL_EChromaMode)parseEnum(tokens, chromaModes);
    AL_SET_CHROMA_MODE(&cfg.Settings.tChParam[0].ePicFormat, mode);
  }, [chromaModes, &cfg]()
  {
    AL_EChromaMode mode = AL_GET_CHROMA_MODE(cfg.Settings.tChParam[0].ePicFormat);
    return getDefaultEnumValue(mode, chromaModes);
  },
                   "Set the expected chroma mode of the encoder. Depending on the input FourCC, this might lead to a conversion. Together with the BitDepth, these options determine the final FourCC the encoder is expecting.", { ParameterType::String }, {
    { allCodecs(), describeEnum(chromaModes), descriptionEnum(chromaModes) }
  }, noNote());
  map<string, EnumDescription<int>> entropymodes;
  entropymodes["MODE_CAVLC"] = { AL_MODE_CAVLC, "Residual block in CAVLC semantics", isOnlyCodec(Codec::Avc) };
  entropymodes["MODE_CABAC"] = { AL_MODE_CABAC, "Residual block in CABAC semantics", isOnlyCodec(Codec::Avc) };
  parser.addEnum(curSection, "EntropyMode", cfg.Settings.tChParam[0].eEntropyMode, entropymodes, "Selects the entropy coding mode");
  map<string, EnumDescription<int>> bitDepthValues;
  bitDepthValues["8"] = { 8, "BitDepth 8", allCodecs() };
  bitDepthValues["10"] = { 10, "BitDepth 10", aomituCodecs() };
  parser.addCustom(curSection, "BitDepth", [&](std::deque<Token>& tokens)
  {
    auto bitdepth = parseArithmetic<int>(tokens);
    AL_SET_BITDEPTH(&cfg.Settings.tChParam[0].ePicFormat, bitdepth);
  }, [&]() {
    return std::to_string(AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat));
  }, "Number of bits used to encode one pixel", { ParameterType::String }, toCallbackInfo(bitDepthValues));

  map<string, EnumDescription<int>> scalingmodes;
  scalingmodes["FLAT"] = { AL_SCL_FLAT, "Scaling list flat", ituCodecs() };
  scalingmodes["DEFAULT"] = { AL_SCL_DEFAULT, "Default value", ituCodecs() };

  scalingmodes["CUSTOM"] = { AL_SCL_CUSTOM, "Custom: a file defining the coefficient of the quantization matrices shall be provided with FileScalingList", ituCodecs() };
  parser.addEnum(curSection, "ScalingList", cfg.Settings.eScalingList, scalingmodes, "Specifies the scaling list mode");
  parser.addSeeAlso(curSection, "ScalingList", { curSection, "FileScalingList" });

  parser.addPath(curSection, "FileScalingList", temp.sScalingListFile, "if ScalingList is CUSTOM, specifies the file containing the quantization matrices", ituCodecs());
  parser.addSeeAlso(curSection, "FileScalingList", { curSection, "ScalingList" });

  map<string, EnumDescription<int>> qpctrls;
  qpctrls["UNIFORM_QP"] = { AL_GENERATE_UNIFORM_QP, "All CUs of the slice use the same QP", aomituCodecs() };
  qpctrls["ROI_QP"] = { AL_GENERATE_ROI_QP, "Region Of Interest values", aomituCodecs() };
  qpctrls["AUTO_QP"] = { AL_GENERATE_AUTO_QP, "The QP is chosen according to the CU content using a pre-programmed lookup table", aomituCodecs() };
  qpctrls["ADAPTIVE_AUTO_QP"] = { AL_GENERATE_ADAPTIVE_AUTO_QP, "Same as AUTO_QP, but the lookup table parameters can be updated at each frame depending on the content", aomituCodecs() };
  qpctrls["RELATIVE_QP"] = { AL_GENERATE_RELATIVE_QP, "Can be use with LOAD_QP via the '|' operator to fill the external buffer with relative values instead of absolute QP values", aomituCodecs() };
  qpctrls["LOAD_QP"] = { AL_GENERATE_LOAD_QP, "The QPs of each CTB come from an external buffer loaded from a file. The file shall be named QPs.hex and located in the directory specified by QPTablesFolder", aomituCodecs() };

  parser.addCustom(curSection, "QPCtrlMode", [qpctrls, &cfg](std::deque<Token>& tokens)
  {
    cfg.RunInfo.eGenerateQpMode = (AL_EGenerateQpMode)parseEnum(tokens, qpctrls);
    auto& settings = cfg.Settings;

    settings.eQpTableMode = AL_QP_TABLE_NONE;
    settings.eQpCtrlMode = AL_QP_CTRL_NONE;

    if(AL_IsAutoQP(cfg.RunInfo.eGenerateQpMode))
    {
      auto isAdaptive = cfg.RunInfo.eGenerateQpMode & AL_GENERATE_ADAPTIVE_AUTO_QP;
      settings.eQpCtrlMode = isAdaptive ? AL_QP_CTRL_ADAPTIVE_AUTO : AL_QP_CTRL_AUTO;
    }

    if(AL_HasQpTable(cfg.RunInfo.eGenerateQpMode))
    {
      bool isRelativeTable = ((cfg.RunInfo.eGenerateQpMode & AL_GENERATE_RELATIVE_QP) != 0);
      settings.eQpTableMode = isRelativeTable ? AL_QP_TABLE_RELATIVE : AL_QP_TABLE_ABSOLUTE;

      if(cfg.RunInfo.eGenerateQpMode == AL_GENERATE_ROI_QP)
        settings.eQpTableMode = AL_QP_TABLE_RELATIVE;
    }
  }, [qpctrls, &cfg]()
  {
    AL_EGenerateQpMode mode = cfg.RunInfo.eGenerateQpMode;
    return getDefaultEnumValue(mode, qpctrls);
  }, string { "Specifies how to generate the QP per CU" }, { ParameterType::String }, {
    { aomituCodecs(), describeEnum(qpctrls), descriptionEnum(qpctrls) }
  });
  parser.addNote(curSection, "QPCtrlMode", "When QPCtrlMode is set to LOAD_QP, it can be combined with the flag RELATIVE_QP using '|' operator to fill the external buffer with relative values instead of absolute QP values. Example: LOAD_QP | RELATIVE_QP");

  map<string, EnumDescription<int>> ldamodes;
  ldamodes["DEFAULT_LDA"] = { AL_DEFAULT_LDA, "Uses the pre-programmed lambda factors", aomituCodecs() };
  ldamodes["AUTO_LDA"] = { AL_AUTO_LDA, "Automatically select between DEFAULT_LDA and DYNAMIC_LDA depending on RateCtrlMode and GopCtrlMode", aomituCodecs() };
  ldamodes["DYNAMIC_LDA"] = { AL_DYNAMIC_LDA, "Uses lambda factors depending on GOP pattern", aomituCodecs() };
  ldamodes["LOAD_LDA"] = { AL_LOAD_LDA, "Uses lambda factors from a provided file called 'Lambdas.hex' in same folder as the application", aomituCodecs() };
  parser.addEnum(curSection, "LambdaCtrlMode", cfg.Settings.tChParam[0].eLdaCtrlMode, ldamodes, "Specifies the lambda values used for rate-distortion optimization");
  parser.addCustom(curSection, "LambdaFactors", [&](std::deque<Token> tokens)
  {
    auto pChan = &cfg.Settings.tChParam[0];
    auto const NumFactors = sizeof(pChan->LdaFactors) / sizeof(*pChan->LdaFactors);
    auto ldaFactors = parseArray<double>(tokens, NumFactors);
    auto const rescale = 256;

    for(auto i = 0; i < (int)NumFactors; ++i)
      pChan->LdaFactors[i] = ldaFactors[i] * rescale;

    temp.bParseLambdaFactors = true;
  }, [&]() {
    auto pChan = &cfg.Settings.tChParam[0];
    auto const NumFactors = sizeof(pChan->LdaFactors) / sizeof(*pChan->LdaFactors);
    return getDefaultArrayValue(pChan->LdaFactors, NumFactors, 256);
  }, "Specifies a lambda factor for each pictures: I, P and B by increasing temporal id");
  map<string, EnumDescription<int>> cabacInitBackPort;
  cabacInitBackPort["DISABLE"] = { 0, "Disable", ituCodecs() };
  cabacInitBackPort["ENABLE"] = { 1, "Enable", filterCodecs({ Codec::Hevc, Codec::Vvc }) };
  parser.addArithOrEnum(curSection, "CabacInit", cfg.Settings.tChParam[0].uCabacInitIdc, cabacInitBackPort, "Specifies the CABAC initialization table", {
    { isOnlyCodec(Codec::Avc), 0, 2 },
    { filterCodecs({ Codec::Hevc, Codec::Vvc }), 0, 1 },
  });
  parser.addArith(curSection, "PicCbQpOffset", cfg.Settings.tChParam[0].iCbPicQpOffset, "Specifies the QP offset for the first chroma channel (Cb) at picture level", {
    { ituCodecs(), -12, 12 }
  });
  parser.addArith(curSection, "PicCrQpOffset", cfg.Settings.tChParam[0].iCrPicQpOffset, "Specifies the QP offset for the second chroma channel (Cr) at picture level", {
    { ituCodecs(), -12, 12 }
  });
  parser.addArith(curSection, "SliceCbQpOffset", cfg.Settings.tChParam[0].iCbSliceQpOffset, "Specifies the QP offset for the first chroma channel (Cb) at slice level", {
    { isOnlyCodec(Codec::Hevc), -12, 12 }
  });
  parser.addArith(curSection, "SliceCrQpOffset", cfg.Settings.tChParam[0].iCrSliceQpOffset, "Specifies the QP offset for the second chroma channel (Cr) at slice level", {
    { isOnlyCodec(Codec::Hevc), -12, 12 }
  });
  parser.addNote(curSection, "SliceCrQpOffset", "(PicCbQPOffset + SliceCbQPOffset) shall be in range -12 to +12. (PicCrQPOffset + SliceCrQPOffset) shall be in range -12 to +12.");
  parser.addArith(curSection, "CuQpDeltaDepth", cfg.Settings.tChParam[0].uCuQPDeltaDepth, "Specifies the QP per CU granularity, Used only when QPCtrlMode is set to either LOAD_QP/AUTO_QP/ADAPTIVE_AUTO_QP", {
    { isOnlyCodec(Codec::Hevc), 0, 2 }
  });
  parser.addNote(curSection, "CuQpDeltaDepth", "0: down to 32x32 ; 1: down to 16x16 ; 2: down to 8x8");
  parser.addSeeAlso(curSection, "CuQpDeltaDepth", { curSection, "QpCtrlMode" });
  parser.addArith(curSection, "LoopFilter.BetaOffset", cfg.Settings.tChParam[0].iBetaOffset, "Specifies the beta offset (AVC/HEVC) or the Filter level (VP9) for the deblocking filter", {
    { aomituCodecs(), -6, +6 }
  });
  parser.addArith(curSection, "LoopFilter.TcOffset", cfg.Settings.tChParam[0].iTcOffset, "Specifies the Alpha_c0 offset (AVC), Tc offset (HEVC) or sharpness level (VP9) for the deblocking filter", {
    { aomituCodecs(), -6, +6 }
  });
  parser.addFlag(curSection, "LoopFilter.CrossSlice", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF_X_SLICE, "In-loop filtering across the left and upper boundaries of each slice of the frame", ituCodecs());
  parser.addFlag(curSection, "LoopFilter.CrossTile", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF_X_TILE, "In-loop filtering across the left and upper boundaries of each tile of the frame", isOnlyCodec(Codec::Hevc));
  parser.addFlag(curSection, "LoopFilter", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF, "Specifies if the deblocking filter should be used or not");
  parser.addFlag(curSection, "ConstrainedIntraPred", cfg.Settings.tChParam[0].eEncTools, AL_OPT_CONST_INTRA_PRED, "Specifies the value of constrained_intra_pred_flag syntax element", ituCodecs());
  parser.addFlag(curSection, "WaveFront", cfg.Settings.tChParam[0].eEncTools, AL_OPT_WPP);
  parser.addNote(curSection, "WaveFront", "When a video sequence (resolution and frame rate) requires more than one core for encoding and WaveFront is disabled, the encoder automatically uses tiles instead.");
  string l2cacheDesc = "";
  l2cacheDesc = "Specifies if the L2 cache is used of not";
  parser.addBool(curSection, "CacheLevel2", cfg.Settings.iPrefetchLevel2, l2cacheDesc);
  parser.addFlag(curSection, "AvcLowLat", cfg.Settings.tChParam[0].eEncOptions, AL_OPT_LOWLAT_SYNC, "Enables a special synchronization mode for AVC low latency encoding (Validation only)", isOnlyCodec(Codec::Avc));
  parser.addBool(curSection, "SliceLat", cfg.Settings.tChParam[0].bSubframeLatency, "Enables slice latency mode");
  parser.addBool(curSection, "LowLatInterrupt", cfg.Settings.tChParam[0].bSubframeLatency, "Deprecated. Enables interrupt at slice level instead of frame level for low latency encoding. This parameter is use for validation purpose only");
  map<string, EnumDescription<int>> numCoreEnums;
  numCoreEnums["AUTO"] = { 0, "The number of cores is selected automatically", aomituCodecs() };
  parser.addArithOrEnum(curSection, "NumCore", cfg.Settings.tChParam[0].uNumCore, numCoreEnums, "Specifies the number of core that should be used to encode the stream", {
    { aomituCodecs(), 0, AL_ENC_NUM_CORES },
  });
  parser.addNote(curSection, "NumCore", "The allowed number of core depends on parameters like: frame resolution, framerate, and hardware capabilities. When the specified number of core rise to an invalid configuration the encoder fails with message 'Error while creating channel'. ");
  parser.addFlag(curSection, "CostMode", cfg.Settings.tChParam[0].eEncOptions, AL_OPT_RDO_COST_MODE, "This parameter allows to reinforce the influence of the chrominance in the RDO choice. Useful to improve the visual quality of video sequences with low saturation.");
  map<string, EnumDescription<int>> videoModes;
  videoModes["PROGRESSIVE"] = { AL_VM_PROGRESSIVE, "Progressive video mode", ituCodecs() };
  videoModes["INTERLACED_TOP"] = { AL_VM_INTERLACED_TOP, "Interlaced video mode: top-bottom", isOnlyCodec(Codec::Hevc) };
  videoModes["INTERLACED_BOTTOM"] = { AL_VM_INTERLACED_BOTTOM, "Interlaced video mode: bottom-top", isOnlyCodec(Codec::Hevc) };
  parser.addEnum(curSection, "VideoMode", cfg.Settings.tChParam[0].eVideoMode, videoModes, "When using a profile, this parameter allows to specify if the video is progressive or interlaced. In interlaced mode, the corresponding flags in header and SEI message will be added.");
  map<string, EnumDescription<int>> twoPassEnums;
  twoPassEnums["DISABLE"] = { 0, "single pass mode", aomituCodecs() };
  twoPassEnums["1"] = { 1, "first pass of a two-pass encoding", aomituCodecs() };
  twoPassEnums["2"] = { 2, "second pass of a two-pass encoding", aomituCodecs() };
  parser.addArithOrEnum(curSection, "TwoPass", cfg.Settings.TwoPass, twoPassEnums, "Enables/Disables the two_pass encoding mode and specifies which pass is considered for the current encoding. In two-pass encoding some information from the first pass are stored in a file and read back by the second pass");
  map<string, EnumDescription<int>> lookAheadEnums;
  lookAheadEnums["DISABLE"] = { 0, "Disable", aomituCodecs() };
  parser.addArithOrEnum(curSection, "LookAhead", cfg.Settings.LookAhead, lookAheadEnums, "Enables/Disables the lookAhead encoding mode and specifies the number of frames in advance for the first analysis pass. this option increase the encoding latency and the number of required memory buffers.", {
    { aomituCodecs(), 1, 20 },
  });
  parser.addNote(curSection, "LookAhead", "TwoPass, SCDFirstPass and LookAhead are exclusive modes. Only one of them can be enabled at the same time.");
  parser.addBool(curSection, "SCDFirstPass", cfg.Settings.bEnableFirstPassSceneChangeDetection, "During first pass, to encode faster, enable only the scene change detection");

}

static void populateRunSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Run;

  // parser.addBool(curSection, "UseBoard", cfg.RunInfo.bUseBoard, "Specifies if we are using the reference model (DISABLE) or the actual hardware (ENABLE)");
  parser.addCustom(curSection, "UseBoard", [&](std::deque<Token>& tokens)
  {
    bool bUseBoard = parseBoolEnum(tokens, createBoolEnums(allCodecs()));
    cfg.RunInfo.iDeviceType = bUseBoard ? DEVICE_TYPE_BOARD : DEVICE_TYPE_REFSW;
  }, [&]() { return (cfg.RunInfo.iDeviceType == DEVICE_TYPE_BOARD) ? "ENABLE" : "DISABLE"; }, "Specifies if we are using the reference model (DISABLE) or the actual hardware (ENABLE)");

  parser.addBool(curSection, "Loop", cfg.RunInfo.bLoop, "Specifies if it should loop back to the beginning of YUV input stream when it reaches the end of the file");
  map<string, EnumDescription<int>> maxPicts;
  maxPicts["ALL"] = { INT_MAX, "Encode all frames, to reach the end of the YUV input stream", allCodecs() };
  parser.addArithOrEnum(curSection, "MaxPicture", cfg.RunInfo.iMaxPict, maxPicts, "Number of frame to encode", {
    { allCodecs(), 1, INT_MAX },
  });
  parser.addNote(curSection, "MaxPicture", "ALL should not be used with Loop = TRUE, otherwise the encoder will never end");
  parser.addSeeAlso(curSection, "MaxPicture", { curSection, "Loop" });
  parser.addArith(curSection, "FirstPicture", cfg.RunInfo.iFirstPict, "Specifies the first frame to encode", {
    { aomituCodecs(), 0, INT_MAX },
  });
  parser.addNote(curSection, "FirstPicture", "Allowed values: integer value between 0 and the number of pictures in the input YUV file.");
  parser.addArith(curSection, "ScnChgLookAhead", cfg.RunInfo.iScnChgLookAhead, "When CmdFile is used with defeined Scene change position, this parameter specify how many frame in advance, the notification should be send to the encoder.", {
    { allCodecs(), 0, INT_MAX },
  });
  parser.addNote(curSection, "ScnChgLookAhead", "Allowed values: integer value between 0 and Gop.Length.");
  parser.addSeeAlso(curSection, "ScnChgLookAhead", { Section::Input, "CmdFile" });
  parser.addSeeAlso(curSection, "ScnChgLookAhead", { Section::Gop, "Gop.Length" });
  map<string, EnumDescription<int>> inputSleepEnums;
  inputSleepEnums["DISABLE"] = { 0, "Disable", allCodecs() };
  parser.addArithOrEnum(curSection, "InputSleep", cfg.RunInfo.uInputSleepInMilliseconds, inputSleepEnums, "Adds the specified interval (in milliseconds) between frame processing", {
    { allCodecs(), 0, INT_MAX },
  });
  parser.addPath(curSection, "BitrateFile", cfg.RunInfo.bitrateFile, "The generated stream size for each picture and bitrate informations will be written to this file");
}

static void try_to_push_secondary_input(ConfigFile& cfg, Temporary& temp, vector<TConfigYUVInput>& inputList)
{
  if(temp.bWidthIsParsed && temp.bHeightIsParsed && temp.bNameIsParsed)
  {
    temp.TempInput.FileInfo.FourCC = cfg.MainInput.FileInfo.FourCC;
    temp.TempInput.FileInfo.FrameRate = cfg.MainInput.FileInfo.FrameRate;
    inputList.push_back(temp.TempInput);

    temp.bWidthIsParsed = false;
    temp.bHeightIsParsed = false;
    temp.bNameIsParsed = false;
  }
}

static void populateSecondaryInputParam(ConfigParser& parser, Temporary& temp, Section eCurSection, bool bQPControl)
{
  parser.addCustom(eCurSection, "YUVFile", [&](std::deque<Token>& tokens)
  {
    temp.TempInput.YUVFileName = parseString(tokens);
    temp.bNameIsParsed = true;
  }, [&]() { return temp.TempInput.YUVFileName; }, "The YUV file name");

  parser.addCustom(eCurSection, "Width", [&](std::deque<Token>& tokens)
  {
    temp.TempInput.FileInfo.PictWidth = parseArithmetic<int>(tokens);
    temp.bWidthIsParsed = true;
  }, [&]() { return std::to_string(temp.TempInput.FileInfo.PictWidth); }, "Frame width (in pixels)", { ParameterType::ArithExpr });
  parser.addCustom(eCurSection, "Height", [&](std::deque<Token>& tokens)
  {
    temp.TempInput.FileInfo.PictHeight = parseArithmetic<int>(tokens);
    temp.bHeightIsParsed = true;
  }, [&]() { return std::to_string(temp.TempInput.FileInfo.PictHeight); }, "Frame height (in pixels)", { ParameterType::ArithExpr });

  if(bQPControl)
  {
    parser.addPath(eCurSection, "ROIFile", temp.TempInput.sRoiFileName, "File containing the Regions of Interest associated to the current yuv input");
    parser.addPath(eCurSection, "QpTablesFolder", temp.TempInput.sQPTablesFolder, "The location of the files containing the QP tables associated to the current yuv input");
  }
}

static void populateDynamicInputSection(ConfigParser& parser, Temporary& temp)
{
  return populateSecondaryInputParam(parser, temp, Section::DynamicInput, false);
}

static void try_finalize_section(ConfigParser& parser, ConfigFile& cfg, Temporary& temp)
{
  (void)cfg;
  (void)temp;
  switch(parser.curSection)
  {
  case Section::DynamicInput:
    try_to_push_secondary_input(cfg, temp, cfg.DynamicInputs);
    break;
  default:
    break;
  }
}

static void populateIdentifiers(ConfigParser& parser, ConfigFile& cfg, Temporary& temporaries, std::ostream& warnStream)
{
  populateInputSection(parser, cfg);
  populateOutputSection(parser, cfg);
  populateRateControlSection(parser, cfg);
  populateGopSection(parser, cfg);
  populateSettingsSection(parser, cfg, temporaries, warnStream);
  populateRunSection(parser, cfg);
  populateDynamicInputSection(parser, temporaries);
}

static void parseSection(ConfigParser& parser, Tokenizer& tokenizer, ConfigFile& cfg, Temporary& temp)
{
  (void)cfg;
  (void)temp;
  Token section = tokenizer.getToken();
  Token closeBracket = tokenizer.getToken();

  if(closeBracket.type != TokenType::CloseBracket)
    throw TokenError(closeBracket, "expected closing bracket while parsing section");

  if(section.type != TokenType::Identifier)
    throw TokenError(section, "expected section name while parsing section");

  try_finalize_section(parser, cfg, temp);

  parser.updateSection(section.text);
}

static void parseIdentifier(ConfigParser& parser, Tokenizer& tokenizer, Token const& identToken)
{
  /* We have all the expected identifier names in a map and function to call if
   * they are detected. each identifier should have it own keyword checking */
  Token equal = tokenizer.getToken();

  if(equal.type != TokenType::Equal)
    throw TokenError(equal, "expected equal sign while parsing identifier");
  Token token = tokenizer.getToken();
  std::deque<Token> tokens {};

  while(token.type != TokenType::EndOfLine && token.type != TokenType::EndOfFile)
  {
    tokens.push_back(token);
    token = tokenizer.getToken();
  }

  if(tokens.size() == 0)
    throw TokenError(token, "expected value after equal sign");

  parser.parseIdentifiers(identToken, tokens);
}

static string readWholeFileInMemory(char const* filename)
{
  std::ifstream in(filename);

  if(!in.is_open())
    throw std::runtime_error("Failed to open " + string(filename));

  stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

typedef enum e_SLMode
{
  SL_4x4_Y_INTRA,
  SL_4x4_Cb_INTRA,
  SL_4x4_Cr_INTRA,
  SL_4x4_Y_INTER,
  SL_4x4_Cb_INTER,
  SL_4x4_Cr_INTER,
  SL_8x8_Y_INTRA,
  SL_8x8_Cb_INTRA,
  SL_8x8_Cr_INTRA,
  SL_8x8_Y_INTER,
  SL_8x8_Cb_INTER,
  SL_8x8_Cr_INTER,
  SL_16x16_Y_INTRA,
  SL_16x16_Cb_INTRA,
  SL_16x16_Cr_INTRA,
  SL_16x16_Y_INTER,
  SL_16x16_Cb_INTER,
  SL_16x16_Cr_INTER,
  SL_32x32_Y_INTRA,
  SL_32x32_Y_INTER,
  SL_DC,
  SL_ERR
}ESLMode;

static string chomp(string sLine)
{
  // Trim left
  auto zFirst = sLine.find_first_not_of(" \t");
  sLine.erase(0, zFirst);

  // Remove CR
  zFirst = sLine.find_first_of("\r\n");

  if(zFirst != sLine.npos)
    sLine.erase(zFirst, sLine.npos);
  return sLine;
}

#define KEYWORD(T) (!sLine.compare(0, sizeof(T) - 1, T))
static bool ParseScalingListMode(string& sLine, ESLMode& Mode)
{
  if(KEYWORD("[4x4 Y Intra]"))
    Mode = SL_4x4_Y_INTRA;
  else if(KEYWORD("[4x4 Cb Intra]"))
    Mode = SL_4x4_Cb_INTRA;
  else if(KEYWORD("[4x4 Cr Intra]"))
    Mode = SL_4x4_Cr_INTRA;
  else if(KEYWORD("[4x4 Y Inter]"))
    Mode = SL_4x4_Y_INTER;
  else if(KEYWORD("[4x4 Cb Inter]"))
    Mode = SL_4x4_Cb_INTER;
  else if(KEYWORD("[4x4 Cr Inter]"))
    Mode = SL_4x4_Cr_INTER;
  else if(KEYWORD("[8x8 Y Intra]"))
    Mode = SL_8x8_Y_INTRA;
  else if(KEYWORD("[8x8 Cb Intra]"))
    Mode = SL_8x8_Cb_INTRA;
  else if(KEYWORD("[8x8 Cr Intra]"))
    Mode = SL_8x8_Cr_INTRA;
  else if(KEYWORD("[8x8 Y Inter]"))
    Mode = SL_8x8_Y_INTER;
  else if(KEYWORD("[8x8 Cb Inter]"))
    Mode = SL_8x8_Cb_INTER;
  else if(KEYWORD("[8x8 Cr Inter]"))
    Mode = SL_8x8_Cr_INTER;
  else if(KEYWORD("[16x16 Y Intra]"))
    Mode = SL_16x16_Y_INTRA;
  else if(KEYWORD("[16x16 Cb Intra]"))
    Mode = SL_16x16_Cb_INTRA;
  else if(KEYWORD("[16x16 Cr Intra]"))
    Mode = SL_16x16_Cr_INTRA;
  else if(KEYWORD("[16x16 Y Inter]"))
    Mode = SL_16x16_Y_INTER;
  else if(KEYWORD("[16x16 Cb Inter]"))
    Mode = SL_16x16_Cb_INTER;
  else if(KEYWORD("[16x16 Cr Inter]"))
    Mode = SL_16x16_Cr_INTER;
  else if(KEYWORD("[32x32 Y Intra]"))
    Mode = SL_32x32_Y_INTRA;
  else if(KEYWORD("[32x32 Y Inter]"))
    Mode = SL_32x32_Y_INTER;
  else if(KEYWORD("[DC]"))
    Mode = SL_DC;
  else
    return false;

  return true;
}

static bool IsScalingListModeAllowed(AL_EProfile eProfile, AL_EPicFormat ePicFormat, ESLMode eMode)
{
  if(!AL_IS_AVC(eProfile))
    return true;

  if(AL_GET_CHROMA_MODE(ePicFormat) == AL_CHROMA_4_4_4)
    return eMode < SL_16x16_Y_INTRA;

  return (eMode <= SL_8x8_Y_INTRA) || (eMode == SL_8x8_Y_INTER);
}

static bool ParseMatrice(std::ifstream& SLFile, string& sLine, int& iLine, AL_TEncSettings& Settings, ESLMode Mode)
{
  int iNumCoefW = (Mode < SL_8x8_Y_INTRA) ? 4 : 8;
  int iNumCoefH = (Mode == SL_DC) ? 1 : iNumCoefW;
  static constexpr int MAX_LUMA_DC_COEFF = 50;

  int iSizeID = (Mode < SL_8x8_Y_INTRA) ? 0 : (Mode < SL_16x16_Y_INTRA) ? 1 : (Mode < SL_32x32_Y_INTRA) ? 2 : 3;
  int iMatrixID = (Mode < SL_32x32_Y_INTRA) ? Mode % 6 : (Mode == SL_32x32_Y_INTRA) ? 0 : 3;

  uint8_t* pMatrix = (Mode == SL_DC) ? Settings.DcCoeff : Settings.ScalingList[iSizeID][iMatrixID];

  for(int i = 0; i < iNumCoefH; ++i)
  {
    getline(SLFile, sLine);
    ++iLine;

    stringstream ss(sLine);

    for(int j = 0; j < iNumCoefW; ++j)
    {
      string sVal;
      ss >> sVal;

      if(!sVal.empty() && isdigit(sVal[0]))
      {
        pMatrix[j + i * iNumCoefH] = std::stoi(sVal);
      }
      else
        return false;
    }
  }

  if(Mode == SL_8x8_Y_INTRA || Mode == SL_8x8_Y_INTER || Mode == SL_4x4_Y_INTER || Mode == SL_4x4_Y_INTRA)
    pMatrix[0] = std::min<int>(pMatrix[0], MAX_LUMA_DC_COEFF);

  if(Mode == SL_DC)
  {
    int iSizeMatrixID = (iSizeID == 3 && iMatrixID == 3) ? 7 : (iSizeID - 2) * 6 + iMatrixID;
    Settings.DcCoeffFlag[iSizeMatrixID] = 1;
  }
  else
    Settings.SclFlag[iSizeID][iMatrixID] = 1;
  return true;
}

static bool ParseScalingListFile(const string& sSLFileName, AL_TEncSettings& Settings, std::ostream& warnstream)
{
  std::ifstream SLFile(sSLFileName.c_str());

  if(!SLFile.is_open())
  {
    warnstream << " => No Scaling list file, using Default" << endl;
    return false;
  }
  ESLMode eMode;

  int iLine = 0;

  memset(Settings.SclFlag, 0, sizeof(Settings.SclFlag));
  memset(Settings.DcCoeffFlag, 0, sizeof(Settings.DcCoeffFlag));

  memset(Settings.ScalingList, -1, sizeof(Settings.ScalingList));
  memset(Settings.DcCoeff, -1, sizeof(Settings.DcCoeff));

  for(;;)
  {
    string sLine;
    getline(SLFile, sLine);

    if(SLFile.fail())
      break;

    ++iLine;

    sLine = chomp(sLine);

    if(sLine.empty() || sLine[0] == '#') // Comment
    {
      // nothing to do
    }
    else if(sLine[0] == '[') // Mode select
    {
      if(!ParseScalingListMode(sLine, eMode) || !IsScalingListModeAllowed(Settings.tChParam[0].eProfile, Settings.tChParam[0].ePicFormat, eMode))
      {
        warnstream << iLine << " => Invalid command line in Scaling list file, using Default" << endl;
        return false;
      }

      if(eMode < SL_ERR)
      {
        if(!ParseMatrice(SLFile, sLine, iLine, Settings, eMode))
        {
          warnstream << iLine << " => Invalid Matrice in Scaling list file, using Default" << endl;
          return false;
        }
      }
    }
  }

  return true;
}

static void GetScalingList(AL_TEncSettings& Settings, string const& sScalingListFile, std::ostream& errstream)
{
  if(Settings.eScalingList == AL_SCL_CUSTOM)
  {
    if(!ParseScalingListFile(sScalingListFile, Settings, errstream))
    {
      Settings.eScalingList = AL_SCL_DEFAULT;
    }
  }
}

/* Perform some settings coherence checks after complete parsing. */
static void PostParsingChecks(AL_TEncSettings& Settings)
{
  auto& GopParam = Settings.tChParam[0].tGopParam;

  if((GopParam.eMode & AL_GOP_FLAG_DEFAULT)
     && (GopParam.eMode & AL_GOP_FLAG_PYRAMIDAL)
     && (GopParam.eMode != AL_GOP_MODE_ADAPTIVE)
     && (GopParam.uNumB > 0))
    throw std::runtime_error("Unsupported Gop.NumB value in this gop mode");

  if(!(GopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
     && GopParam.eGdrMode != AL_GDR_OFF)
    throw std::runtime_error("GDR mode is not supported if the gop mode is not low delay");

}

static void DefaultLambdaFactors(AL_TEncSettings& Settings, bool bParseLambdaFactors)
{
  if(bParseLambdaFactors || Settings.tChParam[0].eLdaCtrlMode != AL_LOAD_LDA)
    return;

  auto pChan = &Settings.tChParam[0];
  auto const NumFactors = sizeof(pChan->LdaFactors) / sizeof(*pChan->LdaFactors);
  auto const identity = 256;

  for(auto i = 0; i < (int)NumFactors; ++i)
    pChan->LdaFactors[i] = identity;
}

static void PostParsingInit(ConfigFile& cfg, Temporary const& temporaries, std::ostream& warnStream)
{
  GetScalingList(cfg.Settings, temporaries.sScalingListFile, warnStream);
  DefaultLambdaFactors(cfg.Settings, temporaries.bParseLambdaFactors);

}

static void ParseConfig(string const& toParse, ConfigFile& cfg, Temporary& temporaries, std::ostream& warnStream = cerr, bool debug = false)
{
  /* ParseConfig should only modify the values that were parsed. It can't set
   * default values as this would break the use case of multiple ParseConfig
   * calls (for example the --set command.) as it would possibly reset the values
   * changes in previous calls to ParseConfig. */
  std::ofstream nullsink;
  nullsink.setstate(std::ios_base::badbit);
  std::ostream* logger;

  if(debug)
    logger = &cout;
  else
    logger = &nullsink;

  Tokenizer tokenizer {
    toParse, logger
  };

  ConfigParser parser {};
  populateIdentifiers(parser, cfg, temporaries, warnStream);
  bool parsing = true;

  while(parsing)
  {
    try
    {
      Token token = tokenizer.getToken();
      switch(token.type)
      {
      default:
        break;
      case TokenType::OpenBracket:
        parseSection(parser, tokenizer, cfg, temporaries);
        break;
      case TokenType::Identifier:
        parseIdentifier(parser, tokenizer, token);
        break;
      case TokenType::EndOfFile:
        parsing = false;
        break;
      }
    }
    catch(std::runtime_error& e)
    {
      if(cfg.strict_mode)
        throw;
      else
        cerr << "Error: " << e.what() << endl;
    }
  }

  try_finalize_section(parser, cfg, temporaries);
}

static void createDescriptionChunks(std::deque<string>& chunks, string& desc)
{
  stringstream description(desc);

  constexpr int maxChunkSize = 70;
  string curChunk = "";
  string word = "";

  while(description >> word)
  {
    word += " ";

    if(curChunk.length() + word.length() >= maxChunkSize)
    {
      chunks.push_back(curChunk);
      curChunk = "";
    }
    curChunk += word;
  }

  if(!curChunk.empty())
    chunks.push_back(curChunk);
}

static void printSectionBanner(Section section)
{
  cout << "--------------------------- [" << toString(section) << "] ---------------------------" << endl << endl;
}

static void printSectionName(Section section)
{
  cout << "[" << toString(section) << "]" << endl;
}

static void printIdentifierDescription(string& identifier, std::deque<string>& chunks)
{
  string firstChunk = "";

  if(!chunks.empty())
  {
    firstChunk = chunks.front();
    chunks.pop_front();
  }

  stringstream ss {};
  ss << std::setfill(' ') << std::setw(24) << std::left << identifier << " " << firstChunk << endl;

  for(auto& chunk : chunks)
    ss << std::setfill(' ') << std::setw(25) << " " << chunk << endl;

  cout << ss.str() << endl;
}

/* Section can exist multiple time and doesn't really have default parameters */
static bool isDynamicSection(Section const& sectionName)
{
  bool notSupported = false;
  (void)sectionName;

  notSupported = notSupported || sectionName == Section::DynamicInput;

  return notSupported;
}

void PrintConfigFileUsage(ConfigFile cfg, bool showAdvancedFeature)
{
  Temporary temporaries {};
  ConfigParser parser {};
  parser.showAdvancedFeature = showAdvancedFeature;
  populateIdentifiers(parser, cfg, temporaries, cerr);

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;
    printSectionBanner(sectionName);

    for(auto identifier_ : section_.second)
    {
      if(identifier_.second.isAdvancedFeature && !parser.showAdvancedFeature)
        continue;

      auto identifier = identifier_.second.showName;
      auto desc = identifier_.second.description;

      std::deque<string> chunks {};
      createDescriptionChunks(chunks, desc);

      string types {
        "Type(s): "
      };

      for(auto const& type : identifier_.second.types)
      {
        types += "<" + toString(type) + ">";

        if(type != identifier_.second.types.back())
          types += " or ";
      }

      createDescriptionChunks(chunks, types);

      if(!isDynamicSection(sectionName))
      {
        auto defaultValue = "Default value: \"" + identifier_.second.defaultValue() + "\"";
        createDescriptionChunks(chunks, defaultValue);
      }

      string available {
        "Available value(s):"
      };
      createDescriptionChunks(chunks, available);

      for(auto info = identifier_.second.info.cbegin(); info != identifier_.second.info.cend(); ++info)
      {
        string codecs {
          '<'
        };

        for(auto const& codec : info->codecs)
        {
          codecs += toString(codec);

          if(codec != info->codecs.back())
            codecs += ", ";
        }

        codecs += ">: " + info->available;

        createDescriptionChunks(chunks, codecs);
      }

      for(auto const& note: identifier_.second.notes)
      {
        auto n = "Note: " + note;
        createDescriptionChunks(chunks, n);
      }

      for(auto it = identifier_.second.seealso.cbegin(); it != identifier_.second.seealso.cend(); ++it)
      {
        string seealso {
          "See also: [" + toString(it->section) + "]" + it->name
        };
        createDescriptionChunks(chunks, seealso);
      }

      printIdentifierDescription(identifier, chunks);
    }
  }
}

static void printIdentifierDescriptionJson(string& identifier, std::deque<string>& chunks)
{
  string firstChunk = "";

  if(!chunks.empty())
  {
    firstChunk = chunks.front();
    chunks.pop_front();
  }

  stringstream ss {};
  ss << "\"name\": \"" << identifier << "\", " << endl;
  ss << "\"description\": \"" << firstChunk;

  for(auto& chunk : chunks)
    ss << chunk;

  ss << "\", ";

  cout << ss.str() << endl;
}

void PrintConfigFileUsageJson(ConfigFile cfg, bool showAdvancedFeature)
{
  Temporary temporaries {};
  ConfigParser parser {};
  parser.showAdvancedFeature = showAdvancedFeature;
  populateIdentifiers(parser, cfg, temporaries, cerr);

  cout << "[" << endl;

  bool first = true;

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;

    for(auto identifier_ : section_.second)
    {
      if(identifier_.second.isAdvancedFeature && !parser.showAdvancedFeature)
        continue;

      if(!first)
        cout << "," << endl;

      cout << "{" << endl;
      cout << "\"section\": \"" << toString(sectionName) << "\", " << endl;

      first = false;

      auto identifier = identifier_.second.showName;
      auto desc = identifier_.second.description;

      std::deque<string> chunks {};
      createDescriptionChunks(chunks, desc);
      printIdentifierDescriptionJson(identifier, chunks);

      string defaultVal = "unknown";

      cout << "\"types\":" << "[";

      for(auto const& type: identifier_.second.types)
      {
        cout << "\"" << toString(type) << "\"";

        if(type != identifier_.second.types.back())
          cout << ", ";
      }

      cout << "]," << endl;

      if(!isDynamicSection(sectionName))
        defaultVal = identifier_.second.defaultValue();
      cout << "\"default\":" << "\"" << defaultVal << "\", " << endl;

      cout << "\"info\":" << "[" << endl;

      for(auto info = identifier_.second.info.cbegin(); info != identifier_.second.info.cend(); ++info)
      {
        cout << "{" << endl;
        cout << "\"codecs\":" << "[";

        for(auto const& codec: info->codecs)
        {
          cout << "\"" << toString(codec) << "\"";

          if(codec != info->codecs.back())
            cout << ", ";
        }

        cout << "]," << endl;

        cout << "\"available\":" << "\"" << info->available << "\", " << endl;

        cout << "\"enums\":" << endl;
        cout << "{" << endl;

        for(auto it = info->availableDescription.cbegin(); it != info->availableDescription.cend(); ++it)
        {
          cout << "\"" << it->first << "\":\"" << it->second << "\"";

          if(std::next(it) != info->availableDescription.cend())
            cout << ",";

          cout << endl;
        }

        cout << "}" << endl;

        cout << "}";

        if(std::next(info) != identifier_.second.info.cend())
          cout << ",";

        cout << endl;
      }

      cout << "]," << endl;

      cout << "\"notes\":" << "[";

      for(auto const& note: identifier_.second.notes)
      {
        cout << "\"" << note << "\"";

        if(note != identifier_.second.notes.back())
          cout << ", ";
      }

      cout << "]," << endl;

      cout << "\"seealso\":" << "[";

      for(auto it = identifier_.second.seealso.cbegin(); it != identifier_.second.seealso.cend(); ++it)
      {
        cout << endl;
        cout << "{" << endl;
        cout << "\"section\":";
        cout << "\"" << toString(it->section) << "\"," << endl;
        cout << "\"name\":";
        cout << "\"" << it->name << "\"" << endl;
        cout << "}" << endl;

        if(std::next(it) != identifier_.second.seealso.cend())
          cout << ",";
      }

      cout << "]" << endl;

      cout << "}";
    }
  }

  cout << "]" << endl;
}

void PrintSection(Section sectionName, map<string, Callback> const& identifiers, bool showAdvancedFeature)
{
  printSectionName(sectionName);

  for(auto identifier_ : identifiers)
  {
    if(identifier_.second.isAdvancedFeature && !showAdvancedFeature)
      continue;
    auto identifier = identifier_.second.showName;
    auto value = identifier_.second.defaultValue();
    auto desc = identifier_.second.description;

    if(desc.find("deprecated") != string::npos)
      continue;

    if(value.empty())
      cout << "#";
    cout << identifier << " = " << value << endl;
  }

  cout << endl;
}

void PrintConfig(ConfigFile cfg, bool showAdvancedFeature)
{
  Temporary temporaries {};
  ConfigParser parser {};
  parser.showAdvancedFeature = showAdvancedFeature;
  populateIdentifiers(parser, cfg, temporaries, cerr);

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;
    auto& identifiers = section_.second;

    if(!isDynamicSection(sectionName))
      PrintSection(sectionName, identifiers, parser.showAdvancedFeature);
    else if(sectionName == Section::DynamicInput)
    {
      for(auto& input : cfg.DynamicInputs)
      {
        temporaries.TempInput = input;
        PrintSection(sectionName, identifiers, parser.showAdvancedFeature);
      }
    }
  }
}

void ParseConfig(string const& toParse, ConfigFile& cfg, std::ostream& warnStream, bool debug)
{
  Temporary temporaries {};
  ParseConfig(toParse, cfg, temporaries, warnStream, debug);
}

static void ResetConfig(ConfigFile& cfg)
{
  cfg.DynamicInputs.clear();

}

void ParseConfigFile(string const& cfgFilename, ConfigFile& cfg, std::ostream& warnStream, bool debug)
{
  ResetConfig(cfg);
  Temporary temporaries {};
  ParseConfig(readWholeFileInMemory(cfgFilename.c_str()), cfg, temporaries, warnStream, debug);
  PostParsingInit(cfg, temporaries, warnStream);
  PostParsingChecks(cfg.Settings);
}

template<>
double get(ArithToken<double> const& arith)
{
  if(!arith.valid)
  {
    Token const& token = arith.token;
    return std::strtod(token.text.c_str(), NULL);
  }
  else
    return arith.value;
}

