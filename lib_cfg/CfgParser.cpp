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
 (c)2008 ALLEGRO DVT
-----------------------------------------------------------------------------
****************************************************************************/
#include "CfgParser.h"

extern "C"
{
#include "lib_common/SliceConsts.h"
#include "lib_common_enc/Settings.h"
}

#include <assert.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>

/*****************************************************************************/
static bool strict_mode = false;

static void ConfigError()
{
  if (strict_mode)
    throw std::runtime_error("Invalid line in configuration file");
}

/*****************************************************************************/
static bool GetString(const string& sLine, string& sStr)
{
  size_t zPos1 = sLine.find('=');
  if(zPos1 == sLine.npos)
    return false;
  size_t zPos2 = sLine.find_first_not_of(" \t", zPos1+1);
  size_t zPos3 = sLine.find_last_not_of(" \t");

  sStr = sLine.substr(zPos2, zPos3-zPos2+1);

  return true;
}

/*****************************************************************************/
#if defined(_WIN32)
inline
static void WindowsPath(string& sStr)
{
  size_t zSize = sStr.size();

  for(size_t i=0; i < zSize; i++)
    if(sStr[i] == '/')
      sStr[i] = '\\';
}
#endif

/*****************************************************************************/
inline
static void LinuxPath(string& sStr)
{
  size_t zSize = sStr.size();

  for(size_t i=0; i < zSize; i++)
    if(sStr[i] == '\\')
      sStr[i] = '/';
}

#if defined(_WIN32)
  #define ToNativePath WindowsPath
#elif defined(__linux__)
  #define ToNativePath LinuxPath
#else
  #define ToNativePath
#endif

/*****************************************************************************/
float GetFloatValue(const string& sLine, size_t zPos2)
{
  size_t zPos3 = sLine.find_first_not_of("0123456789", zPos2);
  if(zPos3 == sLine.npos) zPos3 = sLine.length();

  float fEnt  = float(::atoi(sLine.substr(zPos2, zPos3 - zPos2).c_str()));
  float fFrac = 0.0f;

  if(sLine[zPos3] == '.' || sLine[zPos3] == ',')
  {
    size_t zPos4 = sLine.find_first_not_of("0123456789", zPos3+1);
    if(zPos4 == sLine.npos) zPos4 = sLine.length();

    int iSize = zPos4 - zPos3 - 1;
    if(iSize > 0)
    {
      fFrac = float(::atoi(sLine.substr(zPos3+1, iSize).c_str()));

      while(iSize--)
        fFrac /= 10.0f;
    }
  }

  return fEnt + fFrac;
}

static float GetFloat(const string& sLine)
{
  size_t zPos1 = sLine.find('=');
  if(zPos1 == sLine.npos)
    return 0;
  size_t zPos2 = sLine.find_first_not_of(" \t", zPos1+1);
  return GetFloatValue(sLine, zPos2);
}

/*****************************************************************************/
static bool GetValue(const string& sLine, size_t zStartPos, int& Value, size_t& zLength)
{

#define IF_KEYWORD_0(T) if(!sLine.compare(zStartPos, sizeof(#T)-1, #T)) { Value = T; zLength = sizeof(#T)-1; return true; }
#define IF_KEYWORD_1(T, V) if(!sLine.compare(zStartPos, sizeof(#T)-1, #T)) { Value = V; zLength = sizeof(#T)-1; return true; }
#define IF_KEYWORD_P(P, T) if(!sLine.compare(zStartPos, sizeof(#T)-1, #T)) { Value = P##T; zLength = sizeof(#T)-1; return true; }
#define IF_KEYWORD_A(T) if(!sLine.compare(zStartPos, sizeof(#T)-1, #T)) { Value = AL_##T; zLength = sizeof(#T)-1; return true; }

  int iSign = 0;
  if(sLine[zStartPos] == '-')
  {
    iSign = -1;
    zStartPos++;
  }
  else if(sLine[zStartPos] == '+')
  {
    iSign = 1;
    zStartPos++;
  }

  // Hexadecimal
  if(!sLine.compare(zStartPos, 2, "0x"))
  {
    auto zPos = sLine.find_first_not_of("0123456789AaBbCcDdEeFf", zStartPos+2);
    if(zPos == string::npos)
      zPos = sLine.length();

    Value   =  ::strtol(sLine.substr(zStartPos+2).c_str(), NULL, 16);
    zLength = zPos - zStartPos;
    return iSign == 0;
  }
  // Decimal
  if(::isdigit(sLine[zStartPos]))
  {
    if(!iSign)
      iSign = 1;

    auto zPos = sLine.find_first_not_of("0123456789", zStartPos);
    if(zPos == string::npos)
      zPos = sLine.length();

    Value   = iSign * ::atoi(sLine.substr(zStartPos).c_str());
    zLength = zPos - zStartPos;
    return true;
  }

  // predefined value
  if(iSign)
    return false;

       IF_KEYWORD_1(TRUE, 1)
  else IF_KEYWORD_1(FALSE, 0)
  else IF_KEYWORD_0(ENABLE)
  else IF_KEYWORD_0(DISABLE)
  else IF_KEYWORD_0(UNIFORM_QP)
  else IF_KEYWORD_0(CHOOSE_QP)
  else IF_KEYWORD_0(RAMP_QP)
  else IF_KEYWORD_0(RANDOM_QP)
  else IF_KEYWORD_0(BORDER_QP)
  else IF_KEYWORD_0(AUTO_QP)
  else IF_KEYWORD_0(ADAPTIVE_AUTO_QP)
  else IF_KEYWORD_0(RELATIVE_QP)
  else IF_KEYWORD_0(RANDOM_SKIP)
  else IF_KEYWORD_0(RANDOM_I_ONLY)
  else IF_KEYWORD_P(AL_RC_, CONST_QP)
  else IF_KEYWORD_P(AL_RC_, CBR)
  else IF_KEYWORD_P(AL_RC_, VBR)
  else IF_KEYWORD_P(AL_RC_, LOW_LATENCY)
  else IF_KEYWORD_1(DEFAULT_GOP, AL_GOP_MODE_DEFAULT)
  else IF_KEYWORD_1(PYRAMIDAL_GOP, AL_GOP_MODE_PYRAMIDAL)
  else IF_KEYWORD_1(LOW_DELAY_P, AL_GOP_MODE_LOW_DELAY_P)
  else IF_KEYWORD_1(LOW_DELAY_B, AL_GOP_MODE_LOW_DELAY_B)
  else IF_KEYWORD_0(DEFAULT_LDA)
  else IF_KEYWORD_0(CUSTOM_LDA)
  else IF_KEYWORD_0(AUTO_LDA)
  else IF_KEYWORD_0(DYNAMIC_LDA)
  else IF_KEYWORD_0(LOAD_LDA)
  else IF_KEYWORD_1(SC_ONLY, 0x7FFFFFFF)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MONO)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN_422_10_INTRA)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN_422_10)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN_422)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN_INTRA)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN_STILL)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN10_INTRA)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN10)
  else IF_KEYWORD_P(AL_PROFILE_, HEVC_MAIN) // always after MAIN10 and MAIN_STILL
  //else IF_KEYWORD_P(AL_PROFILE_, AVC_BASELINE) // Baseline is mapped to Constrained_Baseline
  else IF_KEYWORD_1(AVC_BASELINE, AL_PROFILE_AVC_C_BASELINE)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_C_BASELINE)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_MAIN)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_HIGH10_INTRA)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_HIGH10)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_HIGH_422_INTRA)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_HIGH_422)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_HIGH) // always after HIGH10 and HIGH_422
  else IF_KEYWORD_P(AL_PROFILE_, AVC_C_HIGH)
  else IF_KEYWORD_P(AL_PROFILE_, AVC_PROG_HIGH)
  else IF_KEYWORD_1(MAIN_TIER, 0)
  else IF_KEYWORD_1(HIGH_TIER, 1)
  else IF_KEYWORD_0(SEI_NONE)
  else IF_KEYWORD_0(SEI_BP)
  else IF_KEYWORD_0(SEI_PT)
  else IF_KEYWORD_0(SEI_RP)
  else IF_KEYWORD_0(SEI_ALL)
  else IF_KEYWORD_A(MODE_CAVLC)
  else IF_KEYWORD_A(MODE_CABAC)
  else IF_KEYWORD_A(TILE_32x8)
  else IF_KEYWORD_A(NVX)
  else IF_KEYWORD_P(AL_, ASPECT_RATIO_NONE)
  else IF_KEYWORD_P(AL_, ASPECT_RATIO_AUTO)
  else IF_KEYWORD_P(AL_, ASPECT_RATIO_16_9)
  else IF_KEYWORD_P(AL_, ASPECT_RATIO_4_3)
  else IF_KEYWORD_0(COLOUR_DESC_BT_470_PAL)
  else IF_KEYWORD_0(COLOUR_DESC_BT_709)
  else IF_KEYWORD_1(ADAPTIVE_B, AL_ADAPTIVE_B)
  else IF_KEYWORD_0(CHROMA_MONO)
  else IF_KEYWORD_0(CHROMA_4_2_0)
  else IF_KEYWORD_0(CHROMA_4_2_2)
  else IF_KEYWORD_0(CHROMA_4_4_4)
  else IF_KEYWORD_P(AL_SCL_, FLAT)
  else IF_KEYWORD_P(AL_SCL_, DEFAULT)
  else IF_KEYWORD_P(AL_SCL_, CUSTOM)
  else IF_KEYWORD_P(AL_SCL_, RANDOM)
  else IF_KEYWORD_A(TRACE_NONE)
  else IF_KEYWORD_A(TRACE_FRAME)
  else IF_KEYWORD_A(TRACE_ALL)
  else IF_KEYWORD_A(TRACE_STATUS)
  else IF_KEYWORD_1(TRACE_ALWAYS, AL_TRACE_ALL) // backward compatibility
  else IF_KEYWORD_1(ALL, -1)
  else IF_KEYWORD_A(BURST_256)
  else IF_KEYWORD_A(BURST_128)
  else IF_KEYWORD_A(BURST_64)
  else IF_KEYWORD_0(LOAD_QP)
  else IF_KEYWORD_0(LOAD_LDA)
  else IF_KEYWORD_1(AUTO, 0xFFFFFFFF)
  else IF_KEYWORD_1(GDR_HORIZONTAL, AL_GDR_HORIZONTAL)
  else IF_KEYWORD_1(GDR_VERTICAL, AL_GDR_VERTICAL)
  return false;

#undef IF_KEYWORD_0
#undef IF_KEYWORD_1

}

int GetCmdlineValue(const string & sLine)
{
  size_t zLength;
  int Val = 0;
  if (GetValue(sLine, 0, Val, zLength)) {
    return Val;
  } else {
        return -1;
  }
}
/*****************************************************************************/

static int GetValueWrapped(const string& sLine, ostream &errstream)
{
  int Value  = 0;
  int ValTmp = 0;
  size_t zLength;

  char Op = '=';
  char OldOp = '=';

  size_t zPos1 = sLine.find('=');
  if(zPos1 == sLine.npos)
    return -1;

  size_t zPos2 = sLine.find_first_not_of(" \t", zPos1+1);

  while (zPos2 != sLine.npos)
  {
    if(!GetValue(sLine, zPos2, ValTmp, zLength))
    {
      errstream << "Invalid value: " << sLine << endl;
      ConfigError();
      return Value;
    }

    switch(Op)
    {
    case '=' : Value  = ValTmp; break;
    case '+' : Value += ValTmp; break;
    case '-' : Value -= ValTmp; break;
    case '|' : Value |= ValTmp; break;
    case '*' :
               if(OldOp == '=' || OldOp == '*')
                 Value *= ValTmp;
               else
               {
                 errstream << "Invalid Operator" << endl;
                 ConfigError();
               }
               break;
    default:
               errstream << "Syntax error or invalid value !!!" << endl;
               ConfigError();
               return Value;
    }

    zPos2 = sLine.find_first_not_of(" \t", zPos2+zLength);
    if(zPos2 != sLine.npos)
    {
      OldOp = Op;
      Op    = sLine[zPos2];
      zPos2 = sLine.find_first_not_of(" \t", zPos2+1);
    }
  }
  return Value;
}

static int GetValue(const string& sLine)
{
  return GetValueWrapped(sLine, std::cerr);
}

/*****************************************************************************/
static void GetFlag(AL_EChEncOption *pFlags, uint32_t uFlag, const string & sLine)
{
  *pFlags = (AL_EChEncOption)((uint32_t)*pFlags & ~uFlag);
  if(GetValue(sLine))
    *pFlags = (AL_EChEncOption)((uint32_t)*pFlags | uFlag);
}

static void GetFlag(AL_ERateCtrlOption *pFlags, uint32_t uFlag, const string & sLine)
{
  *pFlags = (AL_ERateCtrlOption)((uint32_t)*pFlags & ~uFlag);
  if(GetValue(sLine))
    *pFlags = (AL_ERateCtrlOption)((uint32_t)*pFlags | uFlag);
}

/*****************************************************************************/

void SetFpsandClkRatio(int value, uint16_t &iFps, uint16_t & iClkRatio)
{
  iFps = value / 1000;
  iClkRatio = 1000;

  if(value %= 1000)
    iClkRatio += (1000 - value) / ++iFps;
}

void GetFpsCmdline(const string& sLine, uint16_t & iFps, uint16_t & iClkRatio)
{
  int iTmp = int(GetFloatValue(sLine, 0) * 1000);
  SetFpsandClkRatio(iTmp, iFps, iClkRatio);
}
/*****************************************************************************/
void GetFps(const string& sLine, uint16_t & iFps, uint16_t & iClkRatio)
{
  int iTmp = int(GetFloat(sLine) * 1000);
  SetFpsandClkRatio(iTmp, iFps, iClkRatio);
}

static TFourCC GetFourCCValue(const string & sVal)
{
  // backward compatibility
  if(!sVal.compare("FILE_MONOCHROME"))
    return FOURCC(Y800);
  else if(!sVal.compare("FILE_YUV_4_2_0"))
    return FOURCC(I420);

  // read FourCC
  uint32_t uFourCC = 0;
  if(sVal.size() >= 1)
    uFourCC  = ((uint32_t) sVal[0]);
  if(sVal.size() >= 2)
    uFourCC |= ((uint32_t) sVal[1]) <<  8;
  if(sVal.size() >= 3)
    uFourCC |= ((uint32_t) sVal[2]) << 16;
  if(sVal.size() >= 4)
    uFourCC |= ((uint32_t) sVal[3]) << 24;

  return (TFourCC) uFourCC;
}

TFourCC GetCmdlineFourCC(const string & sLine)
{
  return GetFourCCValue(sLine);
}

/*****************************************************************************/
static TFourCC GetFourCC(const string & sLine)
{
  size_t zPos1 = sLine.find('=');

  if(zPos1 == sLine.npos)
    return -1;

  size_t zPos2 = sLine.find_first_not_of(" \t", zPos1+1);
  size_t zPos3 = sLine.find_first_of(" \t", zPos2+1);

  string sVal = sLine.substr(zPos2, zPos3-zPos2);

  return GetFourCCValue(sVal);
}

/*****************************************************************************/
#define KEYWORD(T) (!sLine.compare(0, sizeof(T)-1, T))

typedef enum e_Section
{
  CFG_SEC_GLOBAL      ,
  CFG_SEC_INPUT       ,
  CFG_SEC_OUTPUT      ,
  CFG_SEC_SETTINGS    ,
  CFG_SEC_TRACE       ,
  CFG_SEC_RUN         ,
  CFG_SEC_RATE_CONTROL,
  CFG_SEC_GOP,
  CFG_SEC_HARDWARE,
} ESection;


/*****************************************************************************/
static bool ParseSection(string& sLine, ESection & Section)
{
       if(KEYWORD("[INPUT]"))        Section = CFG_SEC_INPUT;
  else if(KEYWORD("[OUTPUT]"))       Section = CFG_SEC_OUTPUT;
  else if(KEYWORD("[SETTINGS]"))     Section = CFG_SEC_SETTINGS;
  else if(KEYWORD("[RUN]"))          Section = CFG_SEC_RUN;
  else if(KEYWORD("[TRACE]"))        Section = CFG_SEC_TRACE;
  else if(KEYWORD("[RATE_CONTROL]")) Section = CFG_SEC_RATE_CONTROL;
  else if(KEYWORD("[GOP]"))          Section = CFG_SEC_GOP;
  else if(KEYWORD("[HARDWARE]"))     Section = CFG_SEC_HARDWARE;
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseInput(string & sLine, string & YUVFileName,
                                       TYUVFileInfo & FileInfo,
                                       string & sScnChgFileName,
                                       string & sLTFileName)
{
       if(KEYWORD("YUVFile"))      return GetString(sLine, YUVFileName);
  else if(KEYWORD("Width"))        FileInfo.PictWidth  = GetValue(sLine);
  else if(KEYWORD("Height"))       FileInfo.PictHeight = GetValue(sLine);
  else if(KEYWORD("Format"))       FileInfo.FourCC = TFourCC(GetFourCC(sLine));
  else if(KEYWORD("ScnChgFile"))   GetString(sLine, sScnChgFileName);
  else if(KEYWORD("LTFile"))       GetString(sLine, sLTFileName);
  else if(KEYWORD("FrameRate"))    FileInfo.FrameRate  = GetValue(sLine);
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseOutput(string & sLine, string & BitstreamFileName,
                                 string & RecFileName,
                                 TFourCC & RecFourCC)
{
       if(KEYWORD("BitstreamFile")) return GetString(sLine, BitstreamFileName);
  else if(KEYWORD("RecFile"))       return GetString(sLine, RecFileName);
  else if(KEYWORD("Format"))        RecFourCC = TFourCC(GetFourCC(sLine));
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseRateControl(string & sLine, AL_TEncSettings & Settings)
{
       if(KEYWORD("RateCtrlMode"))    Settings.tChParam.tRCParam.eRCMode        = AL_ERateCtrlMode(GetValue(sLine));
  else if(KEYWORD("BitRate"))         Settings.tChParam.tRCParam.uTargetBitRate = GetValue(sLine)*1000;
  else if(KEYWORD("MaxBitRate"))      Settings.tChParam.tRCParam.uMaxBitRate    = GetValue(sLine)*1000;
  else if(KEYWORD("FrameRate"))       GetFps(sLine, Settings.tChParam.tRCParam.uFrameRate, Settings.tChParam.tRCParam.uClkRatio);
  else if(KEYWORD("SliceQP"))         Settings.tChParam.tRCParam.iInitialQP     = GetValue(sLine);
  else if(KEYWORD("MaxQP"))           Settings.tChParam.tRCParam.iMaxQP         = GetValue(sLine);
  else if(KEYWORD("MinQP"))           Settings.tChParam.tRCParam.iMinQP         = GetValue(sLine);

  else if(KEYWORD("InitialDelay"))    Settings.tChParam.tRCParam.uInitialRemDelay  = uint32_t(GetFloat(sLine) * 90000);
  else if(KEYWORD("CPBSize"))         Settings.tChParam.tRCParam.uCPBSize          = uint32_t(GetFloat(sLine) * 90000);
  else if(KEYWORD("IPDelta"))         Settings.tChParam.tRCParam.uIPDelta          = GetValue(sLine);
  else if(KEYWORD("PBDelta"))         Settings.tChParam.tRCParam.uPBDelta          = GetValue(sLine);

  else if(KEYWORD("ScnChgResilience")) GetFlag(&Settings.tChParam.tRCParam.eOptions, AL_RC_OPT_SCN_CHG_RES, sLine);

  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseGop(string & sLine, AL_TEncSettings & Settings)
{
       if(KEYWORD("GopCtrlMode")) Settings.tChParam.tGopParam.eMode      = AL_EGopCtrlMode(GetValue(sLine));
  else if(KEYWORD("Gop.Length"))  Settings.tChParam.tGopParam.uGopLength = GetValue(sLine);
  else if(KEYWORD("Gop.FreqIDR")) Settings.tChParam.tGopParam.uFreqIDR   = GetValue(sLine);
  else if(KEYWORD("Gop.FreqLT"))  Settings.tChParam.tGopParam.uFreqLT    = GetValue(sLine);
  else if(Settings.tChParam.tGopParam.eMode == AL_GOP_MODE_DEFAULT)
  {
    if(KEYWORD("Gop.NumB"))        Settings.tChParam.tGopParam.uNumB      = GetValue(sLine);
    else
      return false;
  }
  else if(Settings.tChParam.tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL)
  {
    if(KEYWORD("Gop.NumB"))   Settings.tChParam.tGopParam.uNumB      = GetValue(sLine);
    else
      return false;
  }
  else if(Settings.tChParam.tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
  {
    if(KEYWORD("Gop.GdrMode"))     Settings.tChParam.tGopParam.eGdrMode   = AL_EGdrMode(GetValue(sLine));
  }

  return true;
}

/*****************************************************************************/
static bool ParseSettings(string & sLine, AL_TEncSettings & Settings, string& sScalingListFile)
{
       if(KEYWORD("Profile"))                Settings.tChParam.eProfile  = AL_EProfile(GetValue(sLine));
  else if(KEYWORD("Level"))                  Settings.tChParam.uLevel    = uint8_t((GetFloat(sLine) + 0.01f)*10);
  else if(KEYWORD("Tier"))                   Settings.tChParam.uTier     = GetValue(sLine);

  else if(KEYWORD("NumSlices"))              Settings.tChParam.uNumSlices = GetValue(sLine);
  else if(KEYWORD("SliceSize"))              Settings.tChParam.uSliceSize = GetValue(sLine) * 95 / 100; //add entropy precision error, explicit ceil() for Windows/Unix rounding mismatch.
  else if(KEYWORD("DependentSlice"))         Settings.bDependentSlice     = GetValue(sLine) ? true : false;
  else if(KEYWORD("SubframeLatency"))        Settings.tChParam.bSubframeLatency = GetValue(sLine);

  else if(KEYWORD("EnableSEI"))              Settings.uEnableSEI         = uint32_t(GetValue(sLine));
  else if(KEYWORD("EnableAUD"))              Settings.bEnableAUD         = GetValue(sLine) ? true : false;
  else if(KEYWORD("EnableFillerData"))       Settings.bEnableFillerData  = GetValue(sLine) ? true : false;

  else if(KEYWORD("AspectRatio"))            Settings.eAspectRatio       = AL_EAspectRatio(GetValue(sLine));
  else if(KEYWORD("ColourDescription"))      Settings.eColourDescription = AL_EColourDescription(GetValue(sLine));
  else if(KEYWORD("ChromaMode"))             AL_SET_CHROMA_MODE(Settings.tChParam.ePicFormat, AL_EChromaMode(GetValue(sLine)));

  else if(KEYWORD("EntropyMode"))            Settings.tChParam.eEntropyMode = AL_EEntropyMode(GetValue(sLine));
  else if(KEYWORD("BitDepth"))               AL_SET_BITDEPTH(Settings.tChParam.ePicFormat, GetValue(sLine));
  else if(KEYWORD("ScalingList"))            Settings.eScalingList         = (AL_EScalingList)GetValue(sLine);
  else if(KEYWORD("FileScalingList"))        GetString(sLine, sScalingListFile);
  else if(KEYWORD("QPCtrlMode"))             Settings.eQpCtrlMode          = AL_EQpCtrlMode(GetValue(sLine));
  else if(KEYWORD("LambdaCtrlMode"))         Settings.eLdaCtrlMode         = AL_ELdaCtrlMode(GetValue(sLine));

  else if(KEYWORD("CabacInit"))              Settings.tChParam.uCabacInitIdc    = uint8_t(GetValue(sLine));
  else if(KEYWORD("PicCbQpOffset"))          Settings.tChParam.iCbPicQpOffset   = int8_t(GetValue(sLine));
  else if(KEYWORD("PicCrQpOffset"))          Settings.tChParam.iCrPicQpOffset   = int8_t(GetValue(sLine));
  else if(KEYWORD("SliceCbQpOffset"))        Settings.tChParam.iCbSliceQpOffset = int8_t(GetValue(sLine));
  else if(KEYWORD("SliceCrQpOffset"))        Settings.tChParam.iCrSliceQpOffset = int8_t(GetValue(sLine));


  else if(KEYWORD("CuQpDeltaDepth"))         Settings.tChParam.uCuQPDeltaDepth  = int8_t(GetValue(sLine));

  else if(KEYWORD("LoopFilter.BetaOffset"))  Settings.tChParam.iBetaOffset = int8_t(GetValue(sLine));
  else if(KEYWORD("LoopFilter.TcOffset"))    Settings.tChParam.iTcOffset   = int8_t(GetValue(sLine));
  else if(KEYWORD("LoopFilter.CrossSlice"))  GetFlag(&Settings.tChParam.eOptions, AL_OPT_LF_X_SLICE, sLine);
  else if(KEYWORD("LoopFilter.CrossTile"))   GetFlag(&Settings.tChParam.eOptions, AL_OPT_LF_X_TILE, sLine);
  else if(KEYWORD("LoopFilter"))             GetFlag(&Settings.tChParam.eOptions, AL_OPT_LF, sLine);

  else if(KEYWORD("ConstrainedIntraPred"))   GetFlag(&Settings.tChParam.eOptions, AL_OPT_CONST_INTRA_PRED, sLine);

  else if(KEYWORD("WaveFront"))              GetFlag(&Settings.tChParam.eOptions, AL_OPT_WPP, sLine);
  //else if(KEYWORD("Tile"))                   GetFlag((uint32_t *)&Settings.tChParam.eOptions, AL_OPT_TILE, sLine);
  else if(KEYWORD("ForceLoad"))              Settings.bForceLoad   = GetValue(sLine) ? true : false;
  else if(KEYWORD("ForceMvOut"))             GetFlag(&Settings.tChParam.eOptions, AL_OPT_FORCE_MV_OUT, sLine);
  else if(KEYWORD("ForceMvClip"))            GetFlag(&Settings.tChParam.eOptions, AL_OPT_FORCE_MV_CLIP, sLine);
  else if(KEYWORD("CacheLevel2"))            Settings.iPrefetchLevel2     = GetValue(sLine) * 1024;
  else if(KEYWORD("ClipHrzRange"))           Settings.uClipHrzRange = uint32_t(GetValue(sLine));
  else if(KEYWORD("ClipVrtRange"))           Settings.uClipVrtRange = uint32_t(GetValue(sLine));
  else if(KEYWORD("FixPredictor"))           GetFlag(&Settings.tChParam.eOptions, AL_OPT_FIX_PREDICTOR, sLine);
  else if(KEYWORD("VrtRange_P"))             Settings.tChParam.pMeRange[SLICE_P][1] = GetValue(sLine);
  else if(KEYWORD("SrcFormat"))              Settings.tChParam.eSrcConvMode = (AL_ESrcConvMode)GetValue(sLine);
  else if(KEYWORD("DisableIntra"))           Settings.bDisIntra  = bool(GetValue(sLine));
  else if(KEYWORD("AvcLowLat"))              GetFlag(&Settings.tChParam.eOptions, AL_OPT_LOWLAT_SYNC, sLine);
  else if(KEYWORD("LowLatInterrupt"))        GetFlag(&Settings.tChParam.eOptions, AL_OPT_LOWLAT_INT, sLine);
  else if(KEYWORD("NumCore"))                Settings.tChParam.uNumCore = GetValue(sLine);
  else if(KEYWORD("CostMode"))               GetFlag(&Settings.tChParam.eOptions, AL_OPT_RDO_COST_MODE, sLine);
  // backward compatibility
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseRun(string& sLine, TCfgRunInfo&  RunInfo)
{
       if(KEYWORD("UseBoard"))        RunInfo.bUseBoard  = (GetValue(sLine) != 0);
  else if(KEYWORD("Loop"))            RunInfo.bLoop      = (GetValue(sLine) != 0);
  else if(KEYWORD("MaxPicture"))      RunInfo.iMaxPict   = GetValue(sLine);
  else if(KEYWORD("FirstPicture"))    RunInfo.iFirstPict = GetValue(sLine);
  else if(KEYWORD("ScnChgLookAhead")) RunInfo.iScnChgLookAhead = GetValue(sLine);
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseTraces(string& sLine, TConfigTrace& CfgTrace)
{
       if(KEYWORD("Path"))  GetString(sLine, CfgTrace.sPath);
  else if(KEYWORD("Mode"))  CfgTrace.eMode = AL_ETraceMode(GetValue(sLine));
  else if(KEYWORD("Frame")) CfgTrace.iFrame = GetValue(sLine);
  else
    return false;

  return true;
}
/*****************************************************************************/
static bool ParseHardware(string& , AL_TEncSettings & )
{
  return false;
}
/*****************************************************************************/

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

/*****************************************************************************/
typedef enum e_SLMode
{
  SL_4x4_Y_INTRA     ,
  SL_4x4_Cb_INTRA    ,
  SL_4x4_Cr_INTRA    ,
  SL_4x4_Y_INTER     ,
  SL_4x4_Cb_INTER    ,
  SL_4x4_Cr_INTER    ,
  SL_8x8_Y_INTRA     ,
  SL_8x8_Cb_INTRA    ,
  SL_8x8_Cr_INTRA    ,
  SL_8x8_Y_INTER     ,
  SL_8x8_Cb_INTER    ,
  SL_8x8_Cr_INTER    ,
  SL_16x16_Y_INTRA   ,
  SL_16x16_Cb_INTRA  ,
  SL_16x16_Cr_INTRA  ,
  SL_16x16_Y_INTER   ,
  SL_16x16_Cb_INTER  ,
  SL_16x16_Cr_INTER  ,
  SL_32x32_Y_INTRA   ,
  SL_32x32_Y_INTER   ,
  SL_DC        ,
  SL_ERR
} ESLMode;


/*****************************************************************************/
uint8_t ISAVCModeAllowed[SL_ERR] = {1,1,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0};

static bool ParseScalingListMode(std::string& sLine, ESLMode & Mode)
{
       if(KEYWORD("[4x4 Y Intra]"))     Mode = SL_4x4_Y_INTRA;
  else if(KEYWORD("[4x4 Cb Intra]"))    Mode = SL_4x4_Cb_INTRA;
  else if(KEYWORD("[4x4 Cr Intra]"))    Mode = SL_4x4_Cr_INTRA;
  else if(KEYWORD("[4x4 Y Inter]"))     Mode = SL_4x4_Y_INTER;
  else if(KEYWORD("[4x4 Cb Inter]"))    Mode = SL_4x4_Cb_INTER;
  else if(KEYWORD("[4x4 Cr Inter]"))    Mode = SL_4x4_Cr_INTER;
  else if(KEYWORD("[8x8 Y Intra]"))     Mode = SL_8x8_Y_INTRA;
  else if(KEYWORD("[8x8 Cb Intra]"))    Mode = SL_8x8_Cb_INTRA;
  else if(KEYWORD("[8x8 Cr Intra]"))    Mode = SL_8x8_Cr_INTRA;
  else if(KEYWORD("[8x8 Y Inter]"))     Mode = SL_8x8_Y_INTER;
  else if(KEYWORD("[8x8 Cb Inter]"))    Mode = SL_8x8_Cb_INTER;
  else if(KEYWORD("[8x8 Cr Inter]"))    Mode = SL_8x8_Cr_INTER;
  else if(KEYWORD("[16x16 Y Intra]"))   Mode = SL_16x16_Y_INTRA;
  else if(KEYWORD("[16x16 Cb Intra]"))  Mode = SL_16x16_Cb_INTRA;
  else if(KEYWORD("[16x16 Cr Intra]"))  Mode = SL_16x16_Cr_INTRA;
  else if(KEYWORD("[16x16 Y Inter]"))   Mode = SL_16x16_Y_INTER;
  else if(KEYWORD("[16x16 Cb Inter]"))  Mode = SL_16x16_Cb_INTER;
  else if(KEYWORD("[16x16 Cr Inter]"))  Mode = SL_16x16_Cr_INTER;
  else if(KEYWORD("[32x32 Y Intra]"))   Mode = SL_32x32_Y_INTRA;
  else if(KEYWORD("[32x32 Y Inter]"))   Mode = SL_32x32_Y_INTER;
  else if(KEYWORD("[DC]"))  Mode = SL_DC;
  else
    return false;

  return true;
}

/*****************************************************************************/
static bool ParseMatrice(ifstream &SLFile, string& sLine, int &iLine, AL_TEncSettings & Settings, ESLMode Mode)
{
  int iNumCoefW = (Mode < SL_8x8_Y_INTRA) ? 4 : 8;
  int iNumCoefH = (Mode == SL_DC) ? 1 : iNumCoefW;

  int iSizeID = (Mode < SL_8x8_Y_INTRA) ? 0 : (Mode < SL_16x16_Y_INTRA) ? 1 : (Mode < SL_32x32_Y_INTRA) ? 2 : 3 ;
  int iMatrixID = (Mode < SL_32x32_Y_INTRA) ? Mode % 6 : (Mode == SL_32x32_Y_INTRA) ? 0 : 3;

  uint8_t * pMatrix = (Mode == SL_DC) ? Settings.DcCoeff : Settings.ScalingList[iSizeID][iMatrixID];
  for(int i = 0; i < iNumCoefH; ++i)
  {
    getline(SLFile, sLine);
    ++iLine;

    stringstream ss(sLine);
    for(int j = 0; j < iNumCoefW; ++j)
    {
      string sVal;
      ss >> sVal;
      if(!sVal.empty() && isdigit(sVal[0])) {
          pMatrix[j + i * iNumCoefH] = std::stoi(sVal);
      } else
        return false;
    }
  }
  Settings.bScalingListPresentFlags |= 1 << (iSizeID * 4 + iMatrixID);
  return true;
}
/*****************************************************************************/
static void RandomMatrice(AL_TEncSettings & Settings, ESLMode Mode)
{
  static int iRandMt = 0;
  int iNumCoefW = (Mode < SL_8x8_Y_INTRA) ? 4 : 8;
  int iNumCoefH = (Mode == SL_DC) ? 1 : iNumCoefW;

  int iSizeID = (Mode < SL_8x8_Y_INTRA) ? 0 : (Mode < SL_16x16_Y_INTRA) ? 1 : (Mode < SL_32x32_Y_INTRA) ? 2 : 3 ;
  int iMatrixID = (Mode < SL_32x32_Y_INTRA) ? Mode % 6 : (Mode == SL_32x32_Y_INTRA) ? 0 : 3;

  uint8_t * pMatrix = (Mode == SL_DC) ? Settings.DcCoeff : Settings.ScalingList[iSizeID][iMatrixID];
  uint32_t iRand = iRandMt ++;

  for(int i = 0; i < iNumCoefH; ++i)
  {
    for(int j = 0; j < iNumCoefW; ++j)
    {
      iRand = (1103515245 * iRand + 12345);   // Unix
      pMatrix[j + i * iNumCoefH] = (iRand % 255) + 1;
    }
  }
}
static void GenerateMatrice(AL_TEncSettings & Settings)
{
  for (int iMode = 0; iMode < SL_ERR; ++iMode)
    RandomMatrice(Settings, (ESLMode)iMode);
}
/*****************************************************************************/

static bool ParseScalingListFileWrapped(const string& sSLFileName, AL_TEncSettings & Settings, ostream &errstream)
{
  ifstream SLFile(sSLFileName.c_str());

  if(!SLFile.is_open())
  {
    errstream << " => No Scaling list file, using Default" << endl;
    return false;
  }
  ESLMode eMode;

  int iLine = 0;

  memset(Settings.SclFlag    , 0, sizeof(Settings.SclFlag));
  memset(Settings.DcCoeffFlag, 0, sizeof(Settings.DcCoeffFlag));

  memset(Settings.ScalingList, -1, sizeof(Settings.ScalingList));
  memset(Settings.DcCoeff, -1, sizeof(Settings.DcCoeff));

  for(;;)
  {
    string sLine;
    getline(SLFile, sLine);
    if(SLFile.fail()) break;

    ++iLine;

    sLine = chomp(sLine);

    if(sLine.empty() || sLine[0] == '#') // Comment
    {
      // nothing to do
    }
    else if(sLine[0] == '[') // Mode select
    {
      if(!ParseScalingListMode(sLine, eMode) || (AL_IS_AVC(Settings.tChParam.eProfile) && !ISAVCModeAllowed[eMode]))
      {
        errstream << iLine << " => Invalid command line in Scaling list file, using Default" << endl;
        return false;
      }
      if(eMode < SL_ERR)
      {
        if(!ParseMatrice(SLFile, sLine, iLine, Settings, eMode))
        {
          errstream << iLine << " => Invalid Matrice in Scaling list file, using Default" << endl;
          return false;
        }
      }
    }
  }
  return true;
}

/*****************************************************************************/
static void ParseOneLine(int &iLine,
                         ESection &Section,
                         string& sScalingListFile,
                         string& sLine,
                         string & YUVFileName,
                         string & BitstreamFileName,
                         string & RecFileName,
                         string & sScnChgFileName,
                         string & sLTFileName,
                         TYUVFileInfo & FileInfo,
                         TFourCC      & RecFourCC,
                         AL_TEncSettings & Settings,
                         TCfgRunInfo  & RunInfo,
                         TConfigTrace & CfgTrace,
                         ostream & output)
{
  // Remove comments
    string::size_type pos = sLine.find_first_of('#');
    if(pos != sLine.npos)
      sLine.resize(pos);

    sLine = chomp(sLine);

    if(sLine.empty())
    {
      // nothing to do
    }
    else if(sLine[0] == '[') // Section select
    {
      if(!ParseSection(sLine, Section))
      {
        output << iLine << " => Unknown section : " << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_INPUT)
    {
      if(!ParseInput(sLine, YUVFileName, FileInfo, sScnChgFileName, sLTFileName))
      {
        output << iLine << " => Invalid command line in section INPUT : " << endl << sLine << endl;
        ConfigError();
      }
      Settings.tChParam.uWidth  = FileInfo.PictWidth;
      Settings.tChParam.uHeight = FileInfo.PictHeight;
    }
    else if(Section == CFG_SEC_OUTPUT)
    {
      if(!ParseOutput(sLine, BitstreamFileName, RecFileName, RecFourCC))
      {
        output << iLine << " => Invalid command line in section OUTPUT : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_RATE_CONTROL)
    {
      if(!ParseRateControl(sLine, Settings))
      {
        output << iLine << " => Invalid command line in section RATE_CONTROL : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_GOP)
    {
      if(!ParseGop(sLine, Settings))
      {
        output << iLine << " => Invalid command line in section GOP : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_SETTINGS)
    {
      if(!ParseSettings(sLine, Settings, sScalingListFile))
      {
        output << iLine << " => Invalid command line in section SETTINGS : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_RUN)
    {
      if(!ParseRun(sLine, RunInfo))
      {
        output << iLine << " => Invalid command line in section RUN : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_TRACE)
    {
      if(!ParseTraces(sLine, CfgTrace))
      {
        output << iLine << " => Invalid command line in section TRACE : " << endl << sLine << endl;
        ConfigError();
      }
    }
    else if(Section == CFG_SEC_HARDWARE)
    {
      if(!ParseHardware(sLine, Settings))
      {
        output << iLine << " => Invalid command line in section HARDWARE : " << endl << sLine << endl;
        ConfigError();
      }
    }
}

inline
static bool ParseScalingListFile(const string& sSLFileName, AL_TEncSettings & Settings)
{
  return ParseScalingListFileWrapped(sSLFileName, Settings, std::cerr);
}

static void WarningNoTraceFrameWrapped(int& iLine, TConfigTrace& CfgTrace, TCfgRunInfo& RunInfo, ostream& errstream)
{
    if((CfgTrace.eMode == AL_TRACE_FRAME) && (CfgTrace.iFrame == UINT_MAX)) {
        errstream << iLine << " Warning : no frame specified for TRACE_FRAME.";
        errstream << "Use MaxPicture from [RUN] Section instead. " << endl;
        CfgTrace.iFrame = RunInfo.iMaxPict - 1;
    }
}

/* Perform some settings coherence checks after complete parsing. */
static void PostParsingChecks(AL_TEncSettings & Settings)
{
  (void) Settings;
}


static void WarningNoTraceFrame(int& iLine, TConfigTrace& CfgTrace, TCfgRunInfo& RunInfo)
{
  WarningNoTraceFrameWrapped(iLine, CfgTrace, RunInfo, std::cerr);
}

static void GetScalingListWrapped(AL_TEncSettings& Settings, string& sScalingListFile, ostream& errstream)
{
    if(Settings.eScalingList == AL_SCL_CUSTOM) {
        if(!ParseScalingListFileWrapped(sScalingListFile, Settings, errstream)) {
            Settings.eScalingList = AL_SCL_DEFAULT;
        }
    } else if(Settings.eScalingList == AL_SCL_RANDOM) {
        Settings.eScalingList = AL_SCL_CUSTOM;
        GenerateMatrice(Settings);
    }
}

static void GetScalingList(AL_TEncSettings& Settings, string& sScalingListFile)
{
  GetScalingListWrapped(Settings, sScalingListFile, std::cerr);
}

/*****************************************************************************/

static bool ParseConfigFile(const string& sCfgFileName, string & YUVFileName,
                                                 string & BitstreamFileName,
                                                 string & RecFileName,
                                                 string & sScnChgFileName,
                                                 string & sLTFileName,
                                                 TYUVFileInfo & FileInfo,
                                                 TFourCC      & RecFourCC,
                                                 AL_TEncSettings & Settings,
                                                 TCfgRunInfo  & RunInfo,
                                                 TConfigTrace & CfgTrace)
{
  ifstream CfgFile(sCfgFileName);

  if(!CfgFile.is_open())
    return false;

  ESection Section = CFG_SEC_GLOBAL;

  CfgTrace.iFrame = UINT_MAX;
  string sScalingListFile = "";
  int iLine = 0;
  for(;;)
  {
    string sLine;
    getline(CfgFile, sLine);
    if(CfgFile.fail()) break;

    ParseOneLine(iLine, Section, sScalingListFile,
      sLine, YUVFileName, BitstreamFileName, RecFileName, sScnChgFileName, sLTFileName,
      FileInfo, RecFourCC, Settings, RunInfo, CfgTrace, cerr);

    ++iLine;
  }

  PostParsingChecks(Settings);

  ToNativePath(YUVFileName);
  ToNativePath(BitstreamFileName);
  ToNativePath(RecFileName);
  ToNativePath(CfgTrace.sPath);
  ToNativePath(sScalingListFile);
  ToNativePath(sLTFileName);
  ToNativePath(sScnChgFileName);

  WarningNoTraceFrame(iLine, CfgTrace, RunInfo);
  GetScalingList(Settings, sScalingListFile);
  return true;
}

void ParseConfigFile(const string& sCfgFileName, ConfigFile& cfg)
{
  strict_mode = cfg.strict_mode;
  if(!ParseConfigFile(sCfgFileName,
        cfg.YUVFileName,
        cfg.BitstreamFileName,
        cfg.RecFileName,
        cfg.sScnChgFileName,
        cfg.sLTFileName,
        cfg.FileInfo,
        cfg.RecFourCC,
        cfg.Settings,
        cfg.RunInfo,
        cfg.CfgTrace))
    throw runtime_error("Cannot parse config file: '" + sCfgFileName + "'");
}
/*****************************************************************************/

#undef IF_KEYWORD

/*****************************************************************************/
