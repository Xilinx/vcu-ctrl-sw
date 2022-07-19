/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_app
   @{
   \file
 *****************************************************************************/
#pragma once
#include "lib_app/InputFiles.h"
#include "lib_app/utils.h"
#include "QPGenerator.h"

extern "C"
{
#include "lib_common_enc/Settings.h"
}

#include <string>
#include <vector>
#include <array>
#include <iostream>

/*************************************************************************//*!
   \brief Mimics structure for RUN Section of cfg file
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct tCfgRunInfo
{
  std::string encDevicePath;
  AL_EDeviceType iDeviceType;
  AL_ESchedulerType iSchedulerType;
  bool bLoop;
  int iMaxPict;
  unsigned int iFirstPict;
  unsigned int iScnChgLookAhead;
  std::string sRecMd5Path;
  std::string sStreamMd5Path;
  AL_EIpCtrlMode ipCtrlMode;
  std::string logsFile = "";
  std::string apbFile = "";
  bool trackDma = false;
  bool printPictureType = false;
  bool printRateCtrlStat = false;
  std::string bitrateFile = "";
  AL_64U uInputSleepInMilliseconds;
  AL_EGenerateQpMode eGenerateQpMode = AL_GENERATE_UNIFORM_QP;
}TCfgRunInfo;

/*************************************************************************//*!
   \brief Mimics structure for a configuration of an YUV Input
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct tConfigYUVInput
{
  // \brief YUV input file name(s)
  std::string YUVFileName;

  // \brief Map file name used when the encoder receives a compressed YUV file.
  std::string sMapFileName;

  // \brief Information relative to the YUV input file
  TYUVFileInfo FileInfo;

  // \brief Folder where qp tables files are located, if load qp enabled.
  std::string sQPTablesFolder;

  // \brief Name of the file specifying the region of interest per frame is specified
  // happen
  std::string sRoiFileName;
}TConfigYUVInput;

/*************************************************************************//*!
   \brief Source format
*****************************************************************************/
typedef enum e_SrcFormat
{
  AL_SRC_FORMAT_RASTER,
  AL_SRC_FORMAT_TILE_64x4,
  AL_SRC_FORMAT_TILE_32x4,
  AL_SRC_FORMAT_COMP_64x4,
  AL_SRC_FORMAT_COMP_32x4,
  AL_SRC_FORMAT_MAX_ENUM,
}AL_ESrcFormat;

/*************************************************************************//*!
   \brief Whole configuration file
*****************************************************************************/
AL_INTROSPECT(category = "debug") struct ConfigFile
{
  // \brief Path to the cfg location
  std::string sCfgPath;

  // \brief Main YUV input
  TConfigYUVInput MainInput;

  // \brief List of inputs for resolution change
  std::vector<TConfigYUVInput> DynamicInputs;

  // \brief Output bitstream file name
  std::string BitstreamFileName;

  // \brief Reconstructed YUV output file name
  std::string RecFileName;

  // \brief Name of the file specifying the frame numbers where scene changes
  // happen
  std::string sCmdFileName;

  // \brief Name of the file that reads/writes video statistics for TwoPassMode
  std::string sTwoPassFileName;

  // \brief Name of the file specifying HDR SEI contents
  std::string sHDRFileName;

  // \brief FOURCC Code of the reconstructed picture output file
  TFourCC RecFourCC;

  // \brief Source format of encoder input
  AL_ESrcFormat eSrcFormat;

  // \brief Sections RATE_CONTROL and SETTINGS
  AL_TEncSettings Settings;

  // \brief Section RUN
  TCfgRunInfo RunInfo;

  // \brief control the strictness when parsing the configuration file
  bool strict_mode;

};

struct Temporary
{
  Temporary()
  {
  }

  std::string sScalingListFile {};
  std::string sZapperFile {};
  TConfigYUVInput TempInput;
  bool bWidthIsParsed = false;
  bool bHeightIsParsed = false;
  bool bNameIsParsed = false;
  bool bParseLambdaFactors = false;

};

struct CfgParser final
{
  void ParseConfigFile(std::string const& sCfgFileName, ConfigFile& cfg, std::ostream& warnStream = std::cerr, bool debug = false);
  void ParseConfig(std::string const& toParse, ConfigFile& cfg, std::ostream& warnStream = std::cerr, bool debug = false);
  void PrintConfigFileUsage(ConfigFile cfg = {}, bool showAdvancedFeature = true);
  void PrintConfigFileUsageJson(ConfigFile cfg = {}, bool showAdvancedFeature = true);
  void PrintConfig(ConfigFile cfg, bool showAdvancedFeature = true);
  void PostParsingConfiguration(ConfigFile& cfg, std::ostream& warnStream = std::cerr);

private:
  Temporary temporaries {};
};
/*@}*/

