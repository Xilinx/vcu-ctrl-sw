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

using std::cout;
using std::cerr;
using std::endl;
using std::string;

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

static std::string FourCCToString(TFourCC tFourCC)
{
  std::stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
}

static void populateInputSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Input;
  parser.addPath(curSection, "YUVFile", cfg.MainInput.YUVFileName, "YUV input file");
  parser.addCustom(curSection, "Width", [&](std::deque<Token>& tokens)
  {
    cfg.MainInput.FileInfo.PictWidth = parseArithmetic<int>(tokens);
  },
                   [&]() { return std::to_string(cfg.MainInput.FileInfo.PictWidth); },
                   "Specifies the YUV input width");

  parser.addCustom(curSection, "Height", [&](std::deque<Token>& tokens)
  {
    cfg.MainInput.FileInfo.PictHeight = parseArithmetic<int>(tokens);
  },
                   [&]() { return std::to_string(cfg.MainInput.FileInfo.PictHeight); },
                   "Specifies the YUV input height");
  parser.addCustom(curSection, "Format", [&](std::deque<Token>& tokens)
  {
    /* we might want to be able to show users which format are available */
    if(!hasOnlyOneIdentifier(tokens))
      throw std::runtime_error("Failed to parse FOURCC value");
    cfg.MainInput.FileInfo.FourCC = GetFourCCValue(tokens[0].text);
  }, [&]() { return FourCCToString(cfg.MainInput.FileInfo.FourCC); }, "Specifies the YUV input format");
  parser.addPath(curSection, "CmdFile", cfg.sCmdFileName, "File containing the dynamic commands to send to the encoder");
  parser.addPath(curSection, "ROIFile", cfg.MainInput.sRoiFileName, "File containing the Regions of Interest used to encode");
  parser.addPath(curSection, "QpTablesFolder", cfg.MainInput.sQPTablesFolder, "Specifies the location of the files containing the QP tables to use for each frame");
  parser.addPath(curSection, "TwoPassFile", cfg.sTwoPassFileName, "File containing the first pass statistics");
  parser.addPath(curSection, "HDRFile", cfg.sHDRFileName, "Name of the file specifying HDR SEI contents");
  parser.addArith(curSection, "FrameRate", cfg.MainInput.FileInfo.FrameRate, "Specifies the number of frames per second of the source, if it isn't set, we take the RATE_CONTROL FrameRate value. If this parameter is greater than the frame rate specified in the rate control section, the encoder will drop some frames; when this parameter is lower than the frame rate specified in the rate control section, the encoder will repeat some frames");

}

static void populateOutputSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Output;
  parser.addPath(curSection, "BitstreamFile", cfg.BitstreamFileName, "Compressed output file");
  parser.addPath(curSection, "RecFile", cfg.RecFileName, "Reconstructed YUV output file, if not set, the reconstructed picture is not saved");
  parser.addCustom(curSection, "Format", [&](std::deque<Token>& tokens)
  {
    /* we might want to be able to show users which format are available */
    if(!hasOnlyOneIdentifier(tokens))
      throw std::runtime_error("Failed to parse FOURCC value");
    cfg.RecFourCC = GetFourCCValue(tokens[0].text);
  }, [&]() { return FourCCToString(cfg.MainInput.FileInfo.FourCC); }, "Specifies Reconstructed YUV output format, if not set, the output format is the same than in the INPUT section");

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
  std::map<string, int> rateCtrlModes;
  rateCtrlModes["CONST_QP"] = AL_RC_CONST_QP;
  rateCtrlModes["CBR"] = AL_RC_CBR;
  rateCtrlModes["VBR"] = AL_RC_VBR;
  rateCtrlModes["LOW_LATENCY"] = AL_RC_LOW_LATENCY;
  rateCtrlModes["CAPPED_VBR"] = AL_RC_CAPPED_VBR;
  rateCtrlModes["PLUGIN"] = AL_RC_PLUGIN;

  parser.addEnum(curSection, "RateCtrlMode", RCParam.eRCMode, rateCtrlModes, "Selects the way the bit rate is controlled");
  parser.addArithMultipliedByConstant(curSection, "BitRate", RCParam.uTargetBitRate, 1000, "Target bit rate in Kbits/s. Unused if RateCtrlMode=CONST_QP");
  parser.addArithMultipliedByConstant(curSection, "MaxBitRate", RCParam.uMaxBitRate, 1000, "Maximum bit rate in Kbits/s. This is used in VBR. This should be the maximum transmission bandwidth available. (The encoder shouldn't exceed this value on average on a moving window of CpbSize seconds). This option is automatically set to BitRate in CBR.");
  parser.addCustom(curSection, "FrameRate", [&](std::deque<Token>& tokens)
  {
    auto tmp = parseArithmetic<double>(tokens) * 1000;
    SetFpsAndClkRatio(tmp, RCParam.uFrameRate, RCParam.uClkRatio);
  }, [&]()
  {
    double frameRate = RCParam.uFrameRate * 1000.0 / RCParam.uClkRatio;
    return std::to_string(frameRate);
  }, "Number of frames per second");
  std::map<string, int> autoEnum {};
  autoEnum["AUTO"] = -1;
  parser.addArithOrEnum(curSection, "SliceQP", RCParam.iInitialQP, autoEnum, "Quantization parameter. When RateCtrlMode = CONST_QP, the specified QP is applied to all slices. In other cases, the specified QP is used as initial QP");
  parser.addArithOrEnum(curSection, "MaxQP", RCParam.iMaxQP, autoEnum, "Maximum QP value allowed");
  parser.addArithOrEnum(curSection, "MinQP", RCParam.iMinQP, autoEnum, "Minimum QP value allowed. This parameter is especially useful when using VBR rate control. In VBR, the value AUTO can be used to let the encoder select the MinQP according to SliceQP");
  parser.addArithFunc<uint32_t, double>(curSection, "InitialDelay", RCParam.uInitialRemDelay, [](double value)
  {
    return (uint32_t)(value * 90000);
  }, [](uint32_t value) { return (double)value / 90000; }, "Specifies the initial removal delay as specified in the HRD model, in seconds. (unused in CONST_QP)");
  parser.addArithFunc<uint32_t, double>(curSection, "CPBSize", RCParam.uCPBSize, [&](double value)
  {
    return (uint32_t)(value * 90000);
  }, [](uint32_t value) { return (double)value / 90000; }, "Specifies the size of the Coded Picture Buffer as specified in the HRD model, in seconds. (unused in CONST_QP)");
  parser.addArithOrEnum(curSection, "IPDelta", RCParam.uIPDelta, autoEnum, "Specifies the QP difference between I and P frames");
  parser.addArithOrEnum(curSection, "PBDelta", RCParam.uPBDelta, autoEnum, "Specifies the QP difference between P and B frames");
  parser.addFlag(curSection, "ScnChgResilience", RCParam.eOptions, AL_RC_OPT_SCN_CHG_RES);
  parser.addArithFunc<uint16_t, double>(curSection, "MaxQuality", RCParam.uMaxPSNR, [](double value)
  {
    return std::max(std::min((uint16_t)((value + 28.0) * 100), (uint16_t)4800), (uint16_t)2800);
  }, [](uint16_t value) { return (double)value / 100 - 28.0; });
  std::map<string, int> MaxPicSizeEnums;
  MaxPicSizeEnums["DISABLE"] = 0;
  parser.addCustom(curSection, "MaxPictureSize", [&](std::deque<Token>& tokens)
  {
    int size = 0;

    if(hasOnlyOneIdentifier(tokens))
      size = parseEnum(tokens, MaxPicSizeEnums);
    else
      size = parseArithmetic<int>(tokens) * 1000;

    for(auto& picSize :  RCParam.pMaxPictureSize)
      picSize = size;
  }, [&]()
  {
    return "";
  }, "Specifies a coarse picture size in Kbits that shouldn't be exceeded. Available Values: <Arithmetic expression> or DISABLE to not contrain the picture size");
  parser.addArithFuncOrEnum(curSection, "MaxPictureSize.I", RCParam.pMaxPictureSize[AL_SLICE_I], [](int size)
  {
    return size * 1000;
  }, [](uint32_t size)
  {
    return size / 1000;
  }, MaxPicSizeEnums, "Specifies a coarse size (in Kbits) for I-frame that shouldn't be exceeded");
  parser.addArithFuncOrEnum(curSection, "MaxPictureSize.P", RCParam.pMaxPictureSize[AL_SLICE_P], [](int size)
  {
    return size * 1000;
  }, [](uint32_t size)
  {
    return size / 1000;
  }, MaxPicSizeEnums, "Specifies a coarse size (in Kbits) for P-frame that shouldn't be exceeded");
  parser.addArithFuncOrEnum(curSection, "MaxPictureSize.B", RCParam.pMaxPictureSize[AL_SLICE_B], [](int size)
  {
    return size * 1000;
  }, [](uint32_t size)
  {
    return size / 1000;
  }, MaxPicSizeEnums, "Specifies a coarse size (in Kbits) for B-frame that shouldn't be exceeded");
  parser.addFlag(curSection, "EnableSkip", RCParam.eOptions, AL_RC_OPT_ENABLE_SKIP);
  parser.addFlag(curSection, "SCPrevention", RCParam.eOptions, AL_RC_OPT_SC_PREVENTION, "Enable Scene Change Prevention");

#if AL_ENABLE_CUTREE // Disable for quality models delivery for now
  parser.addBool(curSection, "CuTree", RCParam.bEnableCuTree, "Enable the CU Tree");
  parser.addArith(curSection, "CuTree.Strength", RCParam.iCuTreeStrength, "Strength of the CU Tree algorithm");
#endif
}

static void populateRateControlSection(ConfigParser& parser, ConfigFile& cfg)
{
  populateRCParam(Section::RateControl, parser, cfg.Settings.tChParam[0].tRCParam);
}

static void populateGopSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto& GopParam = cfg.Settings.tChParam[0].tGopParam;
  auto curSection = Section::Gop;
  std::map<string, int> gopCtrlModes {};
  gopCtrlModes["DEFAULT_GOP"] = AL_GOP_MODE_DEFAULT;
  gopCtrlModes["LOW_DELAY_P"] = AL_GOP_MODE_LOW_DELAY_P;
  gopCtrlModes["LOW_DELAY_B"] = AL_GOP_MODE_LOW_DELAY_B;
  gopCtrlModes["PYRAMIDAL_GOP"] = AL_GOP_MODE_PYRAMIDAL;
  gopCtrlModes["DEFAULT_GOP_B"] = AL_GOP_MODE_DEFAULT_B;
  gopCtrlModes["PYRAMIDAL_GOP_B"] = AL_GOP_MODE_PYRAMIDAL_B;
  gopCtrlModes["ADAPTIVE_GOP"] = AL_GOP_MODE_ADAPTIVE;
  parser.addEnum(curSection, "GopCtrlMode", GopParam.eMode, gopCtrlModes, "Specifies the Group Of Pictures configuration mode");
  parser.addArith(curSection, "Gop.Length", GopParam.uGopLength, "GOP length in frames including the I picture. 0 for Intra only.");
  std::map<string, int> freqIdrEnums {};
  freqIdrEnums["SC_ONLY"] = INT32_MAX;
  parser.addArithOrEnum(curSection, "Gop.FreqIDR", GopParam.uFreqIDR, freqIdrEnums, "Specifies the minimum number of frames between two IDR pictures (AVC, HEVC). IDR insertion depends on the position of the GOP boundary. -1 to disable IDR insertion");
  parser.addBool(curSection, "Gop.EnableLT", GopParam.bEnableLT);
  parser.addCustom(curSection, "Gop.FreqLT", [&](std::deque<Token>& tokens)
  {
    GopParam.uFreqLT = parseArithmetic<uint32_t>(tokens);
    GopParam.bEnableLT = GopParam.bEnableLT || (GopParam.uFreqLT != 0);
  }, [&]() { return std::to_string(GopParam.uFreqLT); }, "Specifies the Long Term reference picture refresh frequency in number of frames");
  parser.addArith(curSection, "Gop.NumB", GopParam.uNumB, "Maximum number of consecutive B frames in a GOP");
  parser.addArray(curSection, "Gop.TempDQP", GopParam.tempDQP, "Specifies a deltaQP for pictures with temporal id 1 to 4");
  std::map<string, int> gdrModes {};
  gdrModes["GDR_HORIZONTAL"] = AL_GDR_HORIZONTAL;
  gdrModes["GDR_VERTICAL"] = AL_GDR_VERTICAL;
  gdrModes["DISABLE"] = AL_GDR_OFF;
  gdrModes["GDR_OFF"] = AL_GDR_OFF;
  parser.addEnum(curSection, "Gop.GdrMode", GopParam.eGdrMode, gdrModes, "When GopCtrlMode is LOW_DELAY_{P,B}, this parameter specifies whether a Gradual Decoder Refresh scheme should be used or not");
}

static void populateSettingsSection(ConfigParser& parser, ConfigFile& cfg, Temporary& temp, std::ostream& warnStream)
{
  (void)warnStream;
  auto curSection = Section::Settings;
  std::map<string, int> profiles {};

  profiles["HEVC_MONO10"] = AL_PROFILE_HEVC_MONO10;
  profiles["HEVC_MONO"] = AL_PROFILE_HEVC_MONO;
  profiles["HEVC_MAIN_422_10_INTRA"] = AL_PROFILE_HEVC_MAIN_422_10_INTRA;
  profiles["HEVC_MAIN_422_10"] = AL_PROFILE_HEVC_MAIN_422_10;
  profiles["HEVC_MAIN_422"] = AL_PROFILE_HEVC_MAIN_422;
  profiles["HEVC_MAIN_INTRA"] = AL_PROFILE_HEVC_MAIN_INTRA;
  profiles["HEVC_MAIN_STILL"] = AL_PROFILE_HEVC_MAIN_STILL;
  profiles["HEVC_MAIN10_INTRA"] = AL_PROFILE_HEVC_MAIN10_INTRA;
  profiles["HEVC_MAIN10"] = AL_PROFILE_HEVC_MAIN10;
  profiles["HEVC_MAIN"] = AL_PROFILE_HEVC_MAIN;
  /* Baseline is mapped to Constrained_Baseline */
  profiles["AVC_BASELINE"] = AL_PROFILE_AVC_C_BASELINE;
  profiles["AVC_C_BASELINE"] = AL_PROFILE_AVC_C_BASELINE;
  profiles["AVC_MAIN"] = AL_PROFILE_AVC_MAIN;
  profiles["AVC_HIGH10_INTRA"] = AL_PROFILE_AVC_HIGH10_INTRA;
  profiles["AVC_HIGH10"] = AL_PROFILE_AVC_HIGH10;
  profiles["AVC_HIGH_422_INTRA"] = AL_PROFILE_AVC_HIGH_422_INTRA;
  profiles["AVC_HIGH_422"] = AL_PROFILE_AVC_HIGH_422;
  profiles["AVC_HIGH"] = AL_PROFILE_AVC_HIGH;
  profiles["AVC_C_HIGH"] = AL_PROFILE_AVC_C_HIGH;
  profiles["AVC_PROG_HIGH"] = AL_PROFILE_AVC_PROG_HIGH;
  profiles["XAVC_HIGH10_INTRA_CBG"] = AL_PROFILE_XAVC_HIGH10_INTRA_CBG;
  profiles["XAVC_HIGH10_INTRA_VBR"] = AL_PROFILE_XAVC_HIGH10_INTRA_VBR;
  profiles["XAVC_HIGH_422_INTRA_CBG"] = AL_PROFILE_XAVC_HIGH_422_INTRA_CBG;
  profiles["XAVC_HIGH_422_INTRA_VBR"] = AL_PROFILE_XAVC_HIGH_422_INTRA_VBR;
  profiles["XAVC_LONG_GOP_MAIN_MP4"] = AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4;
  profiles["XAVC_LONG_GOP_HIGH_MP4"] = AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4;
  profiles["XAVC_LONG_GOP_HIGH_MXF"] = AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF;
  profiles["XAVC_LONG_GOP_HIGH_422_MXF"] = AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF;
  parser.addEnum(curSection, "Profile", cfg.Settings.tChParam[0].eProfile, profiles, "Specifies the profile to which the bitstream conforms");
  parser.addArithFunc<uint8_t, double>(curSection, "Level", cfg.Settings.tChParam[0].uLevel, [](double value)
  {
    return uint8_t((value + 0.01f) * 10);
  }, [](uint8_t value) { return (double)value / 10; }, "Specifies the Level to which the bitstream conforms");
  std::map<string, int> tiers {};
  tiers["MAIN_TIER"] = 0;
  tiers["HIGH_TIER"] = 1;
  parser.addEnum(curSection, "Tier", cfg.Settings.tChParam[0].uTier, tiers, "Specifies the tier to which the bitstream conforms");
  parser.addArith(curSection, "NumSlices", cfg.Settings.tChParam[0].uNumSlices, "Number of slices used for each frame. Each slice contains one or more full LCU row(s) and they are spread over the frame as regularly as possible");
  std::map<string, int> sliceSizeEnums {};
  sliceSizeEnums["DISABLE"] = 0;
  parser.addArithFuncOrEnum(curSection, "SliceSize", cfg.Settings.tChParam[0].uSliceSize, [](int sliceSize)
  {
    return sliceSize * 95 / 100;
  }, [](uint32_t sliceSize)
  {
    return sliceSize * 100 / 95;
  }, sliceSizeEnums, "Target Slice Size (AVC, HEVC only, not supported in AVC multicore) If set to 0, slices are defined by the NumSlices parameter, Otherwise it specifies the target slice size, in bytes, that the encoder uses to automatically split the bitstream into approximately equally sized slices, with a granularity of one LCU.");
  parser.addBool(curSection, "DependentSlice", cfg.Settings.bDependentSlice, "When there are several slices per frames, this parameter specifies whether the additional slices are dependent slice segments or regular slices (HEVC only)");
  parser.addBool(curSection, "SubframeLatency", cfg.Settings.tChParam[0].bSubframeLatency, "Enable the subframe latency mode");
  std::map<string, int> seis {};
  seis["SEI_NONE"] = AL_SEI_NONE;
  seis["SEI_BP"] = AL_SEI_BP;
  seis["SEI_PT"] = AL_SEI_PT;
  seis["SEI_RP"] = AL_SEI_RP;
  seis["SEI_MDCV"] = AL_SEI_MDCV;
  seis["SEI_CLL"] = AL_SEI_CLL;
  seis["SEI_ALL"] = AL_SEI_ALL;
  parser.addEnum(curSection, "EnableSEI", cfg.Settings.uEnableSEI, seis, "Determines which Supplemental Enhancement Information are sent with the stream");
  parser.addBool(curSection, "EnableAUD", cfg.Settings.bEnableAUD, "Determines if Access Unit Delimiter are added to the stream or not");
  std::map<string, int> fillerEnums {};
  fillerEnums["DISABLE"] = AL_FILLER_DISABLE;
  fillerEnums["ENABLE"] = AL_FILLER_ENC; // for backward compatibility
  fillerEnums["ENC"] = AL_FILLER_ENC;
  fillerEnums["APP"] = AL_FILLER_APP;
  parser.addEnum(curSection, "EnableFillerData", cfg.Settings.eEnableFillerData, fillerEnums, "Specifies if filler data can be added to the stream or not");
  std::map<string, int> aspectRatios;
  aspectRatios["ASPECT_RATIO_AUTO"] = AL_ASPECT_RATIO_AUTO;
  aspectRatios["ASPECT_RATIO_1_1"] = AL_ASPECT_RATIO_1_1;
  aspectRatios["ASPECT_RATIO_4_3"] = AL_ASPECT_RATIO_4_3;
  aspectRatios["ASPECT_RATIO_16_9"] = AL_ASPECT_RATIO_16_9;
  aspectRatios["ASPECT_RATIO_NONE"] = AL_ASPECT_RATIO_NONE;
  parser.addEnum(curSection, "AspectRatio", cfg.Settings.eAspectRatio, aspectRatios, "Selects the display aspect ratio of the video sequence to be written in SPS/VUI");
  std::map<string, int> colourDescriptions;
  colourDescriptions["COLOUR_DESC_RESERVED"] = AL_COLOUR_DESC_RESERVED;
  colourDescriptions["COLOUR_DESC_BT_709"] = AL_COLOUR_DESC_BT_709;
  colourDescriptions["COLOUR_DESC_UNSPECIFIED"] = AL_COLOUR_DESC_UNSPECIFIED;
  colourDescriptions["COLOUR_DESC_BT_470_NTSC"] = AL_COLOUR_DESC_BT_470_NTSC;
  colourDescriptions["COLOUR_DESC_BT_601_PAL"] = AL_COLOUR_DESC_BT_601_PAL;
  colourDescriptions["COLOUR_DESC_BT_601_NTSC"] = AL_COLOUR_DESC_BT_601_NTSC;
  colourDescriptions["COLOUR_DESC_SMPTE_240M"] = AL_COLOUR_DESC_SMPTE_240M;
  colourDescriptions["COLOUR_DESC_GENERIC_FILM"] = AL_COLOUR_DESC_GENERIC_FILM;
  colourDescriptions["COLOUR_DESC_BT_2020"] = AL_COLOUR_DESC_BT_2020;
  colourDescriptions["COLOUR_DESC_SMPTE_ST_428"] = AL_COLOUR_DESC_SMPTE_ST_428;
  colourDescriptions["COLOUR_DESC_SMPTE_RP_431"] = AL_COLOUR_DESC_SMPTE_RP_431;
  colourDescriptions["COLOUR_DESC_SMPTE_EG_432"] = AL_COLOUR_DESC_SMPTE_EG_432;
  colourDescriptions["COLOUR_DESC_EBU_3213"] = AL_COLOUR_DESC_EBU_3213;
  parser.addEnum(curSection, "ColourDescription", cfg.Settings.eColourDescription, colourDescriptions);

  std::map<string, int> transferCharacteristics;
  transferCharacteristics["TRANSFER_UNSPECIFIED"] = AL_TRANSFER_CHARAC_UNSPECIFIED;
  transferCharacteristics["TRANSFER_BT_2100_PQ"] = AL_TRANSFER_CHARAC_BT_2100_PQ;
  parser.addEnum(curSection, "TransferCharac", cfg.Settings.eTransferCharacteristics, transferCharacteristics, "Specifies the reference opto-electronic transfer characteristic function (HDR setting)");
  std::map<string, int> colourMatrices;
  colourMatrices["COLOUR_MAT_UNSPECIFIED"] = AL_COLOUR_MAT_COEFF_UNSPECIFIED;
  colourMatrices["COLOUR_MAT_BT_2100_YCBCR"] = AL_COLOUR_MAT_COEFF_BT_2100_YCBCR;
  parser.addEnum(curSection, "ColourMatrix", cfg.Settings.eColourMatrixCoeffs, colourMatrices, "Specifies the matrix coefficients used in deriving luma and chroma signals from RGB (HDR setting)");

  std::map<string, int> chromaModes {};
  chromaModes["CHROMA_MONO"] = AL_CHROMA_MONO;
  chromaModes["CHROMA_4_0_0"] = AL_CHROMA_4_0_0;
  chromaModes["CHROMA_4_2_0"] = AL_CHROMA_4_2_0;
  chromaModes["CHROMA_4_2_2"] = AL_CHROMA_4_2_2;

  parser.addCustom(curSection, "ChromaMode", [chromaModes, &cfg](std::deque<Token>& tokens)
  {
    AL_EChromaMode mode = (AL_EChromaMode)parseEnum(tokens, chromaModes);
    AL_SET_CHROMA_MODE(&cfg.Settings.tChParam[0].ePicFormat, mode);
  }, [chromaModes, &cfg]()
  {
    AL_EChromaMode mode = AL_GET_CHROMA_MODE(cfg.Settings.tChParam[0].ePicFormat);
    return getDefaultEnumValue(mode, chromaModes);
  },
                   "Set the expected chroma mode of the encoder. Depending on the input fourcc, this might lead to a conversion. Together with the BitDepth, these options determine the final FourCC the encoder is expecting.");
  std::map<string, int> entropymodes {};
  entropymodes["MODE_CAVLC"] = AL_MODE_CAVLC;
  entropymodes["MODE_CABAC"] = AL_MODE_CABAC;
  parser.addEnum(curSection, "EntropyMode", cfg.Settings.tChParam[0].eEntropyMode, entropymodes, "Selects the entropy coding mode");
  parser.addCustom(curSection, "BitDepth", [&](std::deque<Token>& tokens)
  {
    auto bitdepth = parseArithmetic<int>(tokens);
    AL_SET_BITDEPTH(&cfg.Settings.tChParam[0].ePicFormat, bitdepth);
  }, [&]() {
    return std::to_string(AL_GET_BITDEPTH(cfg.Settings.tChParam[0].ePicFormat));
  }, "Number of bits used to encode one pixel");

  std::map<string, int> scalingmodes {};
  scalingmodes["FLAT"] = AL_SCL_FLAT;
  scalingmodes["DEFAULT"] = AL_SCL_DEFAULT;
  scalingmodes["CUSTOM"] = AL_SCL_CUSTOM;
  parser.addEnum(curSection, "ScalingList", cfg.Settings.eScalingList, scalingmodes, "Specifies the scaling list mode");
  parser.addPath(curSection, "FileScalingList", temp.sScalingListFile, "if ScalingList is CUSTOM, specifies the file containing the quantization matrices");
  std::map<string, int> qpctrls {};
  qpctrls["UNIFORM_QP"] = AL_GENERATE_UNIFORM_QP;
  qpctrls["ROI_QP"] = AL_GENERATE_ROI_QP;
  qpctrls["AUTO_QP"] = AL_GENERATE_AUTO_QP;
  qpctrls["ADAPTIVE_AUTO_QP"] = AL_GENERATE_ADAPTIVE_AUTO_QP;
  qpctrls["RELATIVE_QP"] = AL_GENERATE_RELATIVE_QP;
  qpctrls["LOAD_QP"] = AL_GENERATE_LOAD_QP;
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
    else if(AL_HasQpTable(cfg.RunInfo.eGenerateQpMode))
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
  }, std::string { "Specifies how to generate the QP per CU" } +startValueDesc() + describeEnum(qpctrls));

  std::map<string, int> ldamodes {};
  ldamodes["DEFAULT_LDA"] = AL_DEFAULT_LDA;
  ldamodes["AUTO_LDA"] = AL_AUTO_LDA;
  ldamodes["DYNAMIC_LDA"] = AL_DYNAMIC_LDA;
  ldamodes["LOAD_LDA"] = AL_LOAD_LDA;
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
  parser.addBool(curSection, "CabacInit", cfg.Settings.tChParam[0].uCabacInitIdc, "Specifies the CABAC initialization table index (AVC) or flag (HEVC)");
  parser.addArith(curSection, "PicCbQpOffset", cfg.Settings.tChParam[0].iCbPicQpOffset, "Specifies the QP offset for the first chroma channel (Cb) at picture level (AVC/HEVC)");
  parser.addArith(curSection, "PicCrQpOffset", cfg.Settings.tChParam[0].iCrPicQpOffset, "Specifies the QP offset for the second chroma channel (Cr) at picture level (AVC/HEVC)");
  parser.addArith(curSection, "SliceCbQpOffset", cfg.Settings.tChParam[0].iCbSliceQpOffset, "Specifies the QP offset for the first chroma channel (Cb) at slice level (HEVC)");
  parser.addArith(curSection, "SliceCrQpOffset", cfg.Settings.tChParam[0].iCrSliceQpOffset, "Specifies the QP offset for the second chroma channel (Cr) at slice level (HEVC)");
  parser.addArith(curSection, "CuQpDeltaDepth", cfg.Settings.tChParam[0].uCuQPDeltaDepth, "Specifies the QP per CU granularity, Used only when QPCtrlMode is set to either LOAD_QP/AUTO_QP/ADAPTIVE_AUTO_QP");
  parser.addArith(curSection, "LoopFilter.BetaOffset", cfg.Settings.tChParam[0].iBetaOffset, "Specifies the beta offset (AVC/HEVC) or the Filter level (VP9) for the deblocking filter");
  parser.addArith(curSection, "LoopFilter.TcOffset", cfg.Settings.tChParam[0].iTcOffset, "Specifies the Alpha_c0 offset (AVC), Tc offset (HEVC) or sharpness level (VP9) for the deblocking filter");
  parser.addFlag(curSection, "LoopFilter.CrossSlice", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF_X_SLICE, "In-loop filtering across the left and upper boundaries of each tile of the frame (HEVC, AVC)");
  parser.addFlag(curSection, "LoopFilter.CrossTile", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF_X_TILE, "In-loop filtering across the left and upper boundaries of each tile of the frame (HEVC)");
  parser.addFlag(curSection, "LoopFilter", cfg.Settings.tChParam[0].eEncTools, AL_OPT_LF, "Specifies if the deblocking filter should be used or not");
  parser.addFlag(curSection, "ConstrainedIntraPred", cfg.Settings.tChParam[0].eEncTools, AL_OPT_CONST_INTRA_PRED, "Specifies the value of constrained_intra_pred_flag syntax element (AVC/HEVC)");
  parser.addFlag(curSection, "WaveFront", cfg.Settings.tChParam[0].eEncTools, AL_OPT_WPP);
  string l2cacheDesc = "";
  l2cacheDesc = "Specifies if the L2 cache is used of not";
  parser.addBool(curSection, "CacheLevel2", cfg.Settings.iPrefetchLevel2, l2cacheDesc);
  parser.addFlag(curSection, "AvcLowLat", cfg.Settings.tChParam[0].eEncOptions, AL_OPT_LOWLAT_SYNC, "Enables a special synchronization mode for AVC low latency encoding (Validation only)");
  parser.addBool(curSection, "SliceLat", cfg.Settings.tChParam[0].bSubframeLatency, "Enables slice latency mode");
  parser.addBool(curSection, "LowLatInterrupt", cfg.Settings.tChParam[0].bSubframeLatency, "deprecated, same behaviour as SliceLat");
  std::map<string, int> numCoreEnums;
  numCoreEnums["AUTO"] = 0;
  parser.addArithOrEnum(curSection, "NumCore", cfg.Settings.tChParam[0].uNumCore, numCoreEnums, "Number of core to use for this encoding");
  parser.addFlag(curSection, "CostMode", cfg.Settings.tChParam[0].eEncOptions, AL_OPT_RDO_COST_MODE);
  std::map<string, int> videoModes;
  videoModes["PROGRESSIVE"] = AL_VM_PROGRESSIVE;
  videoModes["INTERLACED_TOP"] = AL_VM_INTERLACED_TOP;
  videoModes["INTERLACED_BOTTOM"] = AL_VM_INTERLACED_BOTTOM;
  parser.addEnum(curSection, "VideoMode", cfg.Settings.tChParam[0].eVideoMode, videoModes);
  std::map<string, int> twoPassEnums;
  twoPassEnums["DISABLE"] = 0;
  parser.addArithOrEnum(curSection, "TwoPass", cfg.Settings.TwoPass, twoPassEnums, "Index of the pass currently encoded (in Twopass mode)");
  parser.addArithOrEnum(curSection, "LookAhead", cfg.Settings.LookAhead, twoPassEnums, "Size of the LookAhead");
  parser.addBool(curSection, "SCDFirstPass", cfg.Settings.bEnableFirstPassSceneChangeDetection, "During first pass, to encode faster, enable only the scene change detection");

}

static void populateRunSection(ConfigParser& parser, ConfigFile& cfg)
{
  auto curSection = Section::Run;

  // parser.addBool(curSection, "UseBoard", cfg.RunInfo.bUseBoard, "Specifies if we are using the reference model (DISABLE) or the actual hardware (ENABLE)");
  parser.addCustom(curSection, "UseBoard", [&](std::deque<Token>& tokens)
  {
    bool bUseBoard = parseBoolEnum(tokens, createBoolEnums());
    cfg.RunInfo.iDeviceType = bUseBoard ? DEVICE_TYPE_BOARD : DEVICE_TYPE_REFSW;
  }, [&]() { return (cfg.RunInfo.iDeviceType == DEVICE_TYPE_BOARD) ? "ENABLE" : "DISABLE"; }, "Specifies if we are using the reference model (DISABLE) or the actual hardware (ENABLE)");

  parser.addBool(curSection, "Loop", cfg.RunInfo.bLoop, "Specifies if it should loop back to the beginning of YUV input stream when it reaches the end of the file");
  std::map<string, int> maxPicts {};
  maxPicts["ALL"] = -1;
  parser.addArithOrEnum(curSection, "MaxPicture", cfg.RunInfo.iMaxPict, maxPicts, "Number of frame to encode");
  parser.addArith(curSection, "FirstPicture", cfg.RunInfo.iFirstPict, "Specifies the first frame to encode");
  parser.addArith(curSection, "ScnChgLookAhead", cfg.RunInfo.iScnChgLookAhead);
  parser.addArith(curSection, "InputSleep", cfg.RunInfo.uInputSleepInMilliseconds, "Time period in milliseconds. The encoder is given frames each time period (at a minimum)");
  parser.addPath(curSection, "BitrateFile", cfg.RunInfo.bitrateFile, "The generated stream size for each picture and bitrate informations will be written to this file");
}

static void try_to_push_secondary_input(ConfigFile& cfg, Temporary& temp, std::vector<TConfigYUVInput>& inputList)
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
  }, [&]() { return temp.TempInput.YUVFileName; }, "The YUV source in a different resolution than main input");

  parser.addCustom(eCurSection, "Width", [&](std::deque<Token>& tokens)
  {
    temp.TempInput.FileInfo.PictWidth = parseArithmetic<int>(tokens);
    temp.bWidthIsParsed = true;
  }, [&]() { return std::to_string(temp.TempInput.FileInfo.PictWidth); }, "The width of the current source");
  parser.addCustom(eCurSection, "Height", [&](std::deque<Token>& tokens)
  {
    temp.TempInput.FileInfo.PictHeight = parseArithmetic<int>(tokens);
    temp.bHeightIsParsed = true;
  }, [&]() { return std::to_string(temp.TempInput.FileInfo.PictHeight); }, "The height of the current source");

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

  std::stringstream ss;
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

static uint8_t ISAVCModeAllowed[SL_ERR] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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

    std::stringstream ss(sLine);

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

static void RandomMatrice(AL_TEncSettings& Settings, ESLMode Mode)
{
  static int iRandMt = 0;
  int iNumCoefW = (Mode < SL_8x8_Y_INTRA) ? 4 : 8;
  int iNumCoefH = (Mode == SL_DC) ? 1 : iNumCoefW;

  int iSizeID = (Mode < SL_8x8_Y_INTRA) ? 0 : (Mode < SL_16x16_Y_INTRA) ? 1 : (Mode < SL_32x32_Y_INTRA) ? 2 : 3;
  int iMatrixID = (Mode < SL_32x32_Y_INTRA) ? Mode % 6 : (Mode == SL_32x32_Y_INTRA) ? 0 : 3;

  uint8_t* pMatrix = (Mode == SL_DC) ? Settings.DcCoeff : Settings.ScalingList[iSizeID][iMatrixID];
  uint32_t iRand = iRandMt++;

  for(int i = 0; i < iNumCoefH; ++i)
  {
    for(int j = 0; j < iNumCoefW; ++j)
    {
      iRand = (1103515245 * iRand + 12345);   // Unix
      pMatrix[j + i * iNumCoefH] = (iRand % 255) + 1;
    }
  }
}

static void GenerateMatrice(AL_TEncSettings& Settings)
{
  for(int iMode = 0; iMode < SL_ERR; ++iMode)
    RandomMatrice(Settings, (ESLMode)iMode);
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
      if(!ParseScalingListMode(sLine, eMode) || (AL_IS_AVC(Settings.tChParam[0].eProfile) && !ISAVCModeAllowed[eMode]))
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
  std::stringstream description(desc);

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

  std::stringstream ss {};
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

void PrintConfigFileUsage(ConfigFile cfg)
{
  Temporary temporaries {};
  ConfigParser parser {};
  populateIdentifiers(parser, cfg, temporaries, cerr);

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;
    printSectionBanner(sectionName);

    for(auto identifier_ : section_.second)
    {
      auto identifier = identifier_.second.showName;
      auto desc = identifier_.second.desc;

      if(!isDynamicSection(sectionName))
        desc += ". Default Value: \"" + identifier_.second.defaultValue() + "\"";

      std::deque<string> chunks {};
      createDescriptionChunks(chunks, desc);
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

  std::stringstream ss {};
  ss << "\"name\": \"" << identifier << "\", " << endl;
  ss << "\"desc\": \"" << firstChunk;

  for(auto& chunk : chunks)
    ss << " " << chunk;

  ss << "\", ";

  cout << ss.str() << endl;
}

void PrintConfigFileUsageJson(ConfigFile cfg)
{
  Temporary temporaries {};
  ConfigParser parser {};
  populateIdentifiers(parser, cfg, temporaries, cerr);

  cout << "[" << endl;

  bool first = true;

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;

    for(auto identifier_ : section_.second)
    {
      if(!first)
        cout << "," << endl;

      cout << "{" << endl;
      cout << "\"section\": \"" << toString(sectionName) << "\", " << endl;

      first = false;

      auto identifier = identifier_.second.showName;
      auto desc = identifier_.second.desc;

      std::deque<string> chunks {};
      createDescriptionChunks(chunks, desc);
      printIdentifierDescriptionJson(identifier, chunks);

      std::string defaultVal = "unknown";

      if(!isDynamicSection(sectionName))
        defaultVal = identifier_.second.defaultValue();

      cout << "\"default\":" << "\"" << defaultVal << "\", " << endl;
      cout << "\"available\":" << "\"" << identifier_.second.available << "\"" << endl;

      cout << "}";
    }
  }

  cout << "]" << endl;
}

void PrintSection(Section sectionName, std::map<std::string, Callback> const& identifiers)
{
  printSectionName(sectionName);

  for(auto identifier_ : identifiers)
  {
    auto identifier = identifier_.second.showName;
    auto value = identifier_.second.defaultValue();
    auto desc = identifier_.second.desc;

    if(desc.find("deprecated") != string::npos)
      continue;

    if(value.empty())
      cout << "#";
    cout << identifier << " = " << value << endl;
  }

  cout << endl;
}

void PrintConfig(ConfigFile cfg)
{
  Temporary temporaries {};
  ConfigParser parser {};
  populateIdentifiers(parser, cfg, temporaries, cerr);

  for(auto& section_ : parser.identifiers)
  {
    auto sectionName = section_.first;
    auto& identifiers = section_.second;

    if(!isDynamicSection(sectionName))
      PrintSection(sectionName, identifiers);
    else if(sectionName == Section::DynamicInput)
    {
      for(auto& input : cfg.DynamicInputs)
      {
        temporaries.TempInput = input;
        PrintSection(sectionName, identifiers);
      }
    }
  }
}

void ParseConfig(string const& toParse, ConfigFile& cfg, std::ostream& warnStream, bool debug)
{
  Temporary temporaries {};
  ParseConfig(toParse, cfg, temporaries, warnStream, debug);
}

void ParseConfigFile(string const& cfgFilename, ConfigFile& cfg, std::ostream& warnStream, bool debug)
{
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

