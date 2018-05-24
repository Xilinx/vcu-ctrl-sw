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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_cfg
   @{
   \file
 *****************************************************************************/
#pragma once
#include "lib_app/InputFiles.h"
#include "lib_app/utils.h"

extern "C"
{
#include "lib_common_enc/Settings.h"
}

#include <string>
#include <vector>
using namespace std;


/*************************************************************************//*!
   \brief Mimics structure for RUN Section of cfg file
*****************************************************************************/
typedef struct tCfgRunInfo
{
  bool bUseBoard;
  SCHEDULER_TYPE iSchedulerType;
  bool bLoop;
  int iMaxPict;
  unsigned int iFirstPict;
  unsigned int iScnChgLookAhead;
  string sMd5Path;
  int eVQDescr;
  IpCtrlMode ipCtrlMode;
  std::string logsFile = "";
  bool trackDma = false;
  bool printPictureType = false;
}TCfgRunInfo;


/*************************************************************************//*!
   \brief Whole configuration file
*****************************************************************************/
struct ConfigFile
{
  // \brief YUV input file name(s)
  string YUVFileName;

  // \brief Output bitstream file name
  string BitstreamFileName;

  // \brief Reconstructed YUV output file name
  string RecFileName;

  // \brief Name of the file specifying the frame numbers where scene changes
  // happen
  string sCmdFileName;

  // \brief Name of the file specifying the region of interest per frame is specified
  // happen
  string sRoiFileName;



  // \brief Information relative to YUV input file (from section INPUT)
  TYUVFileInfo FileInfo;

  // \brief FOURCC Code of the reconstructed picture output file
  TFourCC RecFourCC;

  // \brief Sections RATE_CONTROL and SETTINGS
  AL_TEncSettings Settings;

  // \brief Section RUN
  TCfgRunInfo RunInfo;

  // \brief control the strictness when parsing the configuration file
  bool strict_mode;
};

/*************************************************************************//*!
   \brief Parses the specified Cfg File and fill corresponding structures
   \param[in] sCfgFileName Reference to a string object that specifies
   the name of the config file
   \param[inout] cfg Reference to ConfigFile object that will be updated
   according to the config file content.
   \param[out] warnStream warning stream
*****************************************************************************/
void ParseConfigFile(const string& sCfgFileName, ConfigFile& cfg, ostream& warnStream);

/*************************************************************************//*!
   \brief Retrives the numerical value corresponding to a string word
   \param[in]  sLine the string word
   \return the FOURCC value
*****************************************************************************/
int GetCmdlineValue(const string& sLine);

/*************************************************************************//*!
   \brief Retrieve a FourCC numerical value from a string word
   \param[in]  sLine the string word
   \return the FOURCC value
*****************************************************************************/
TFourCC GetCmdlineFourCC(const string& sLine);

/*************************************************************************//*!
   \brief Retrieve the numerical framerate value frome a string
   \param[in]  sLine the string word
   \param[out] iFps the Number of frame per second
   \param[out] iClkRatio the clock ratio (1000 or 1001)
*****************************************************************************/
void GetFpsCmdline(const string& sLine, uint16_t& iFps, uint16_t& iClkRatio);

/*@}*/

