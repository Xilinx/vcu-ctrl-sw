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
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include <assert.h>
#include <string.h>

#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/Settings.h"
#include "lib_common/ChannelResources.h"
#include "lib_common/Utils.h"
#include "lib_common/StreamBufferPrivate.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common/SEI.h"
#ifndef HW_IP_BIT_DEPTH
#define HW_IP_BIT_DEPTH 10
#endif

/***************************************************************************/
static bool AL_sSettings_CheckProfile(AL_EProfile eProfile)
{
  switch(eProfile)
  {
  case AL_PROFILE_HEVC_MAIN:
  case AL_PROFILE_HEVC_MAIN10:
  case AL_PROFILE_HEVC_MAIN_STILL:
  case AL_PROFILE_HEVC_MONO:
  case AL_PROFILE_HEVC_MONO10:
  case AL_PROFILE_HEVC_MAIN_422:
  case AL_PROFILE_HEVC_MAIN_422_10:
  case AL_PROFILE_HEVC_MAIN_INTRA:
  case AL_PROFILE_HEVC_MAIN10_INTRA:
  case AL_PROFILE_HEVC_MAIN_422_INTRA:
  case AL_PROFILE_HEVC_MAIN_422_10_INTRA:
  case AL_PROFILE_AVC_BASELINE:
  case AL_PROFILE_AVC_C_BASELINE:
  case AL_PROFILE_AVC_MAIN:
  case AL_PROFILE_AVC_HIGH:
  case AL_PROFILE_AVC_HIGH10:
  case AL_PROFILE_AVC_HIGH_422:
  case AL_PROFILE_AVC_C_HIGH:
  case AL_PROFILE_AVC_PROG_HIGH:
  case AL_PROFILE_AVC_HIGH10_INTRA:
  case AL_PROFILE_AVC_HIGH_422_INTRA:
    return true;
  default:
    return false;
  }
}

/***************************************************************************/
static bool AL_sSettings_CheckLevel(AL_EProfile eProfile, uint8_t uLevel)
{
  if(AL_IS_HEVC(eProfile))
  {
    if((uLevel == 10)
       || ((uLevel >= 20) && (uLevel <= 21))
       || ((uLevel >= 30) && (uLevel <= 31))
       || ((uLevel >= 40) && (uLevel <= 41))
       || ((uLevel >= 50) && (uLevel <= 52))
       || ((uLevel >= 60) && (uLevel <= 62)))
      return true;
  }
  else if(AL_IS_AVC(eProfile))
  {
    if(((uLevel >= 10) && (uLevel <= 13))
       || ((uLevel >= 20) && (uLevel <= 22))
       || ((uLevel >= 30) && (uLevel <= 32))
       || ((uLevel >= 40) && (uLevel <= 42))
       || ((uLevel >= 50) && (uLevel <= 52))
       || ((uLevel >= 60) && (uLevel <= 62)))
      return true;
  }
  return false;
}

/****************************************************************************/
static int AL_sSettings_GetCpbVclFactor(AL_EProfile eProfile)
{
  switch(eProfile)
  {
  case AL_PROFILE_AVC_BASELINE:
  case AL_PROFILE_AVC_MAIN:
  case AL_PROFILE_AVC_EXTENDED: return 1200;
  case AL_PROFILE_AVC_HIGH: return 1500;
  case AL_PROFILE_AVC_HIGH10: return 3600;
  case AL_PROFILE_AVC_HIGH_422: return 4800;

  case AL_PROFILE_HEVC_MONO: return 667;
  case AL_PROFILE_HEVC_MONO12: return 1000;
  case AL_PROFILE_HEVC_MONO16: return 1333;
  case AL_PROFILE_HEVC_MAIN: return 1000;
  case AL_PROFILE_HEVC_MAIN10: return 1000;
  case AL_PROFILE_HEVC_MAIN12: return 1500;
  case AL_PROFILE_HEVC_MAIN_STILL: return 1000;
  case AL_PROFILE_HEVC_MAIN_422: return 1667;
  case AL_PROFILE_HEVC_MAIN_422_10: return 1667;
  case AL_PROFILE_HEVC_MAIN_422_12: return 2000;
  case AL_PROFILE_HEVC_MAIN_444_10: return 2500;
  case AL_PROFILE_HEVC_MAIN_444_12: return 3000;
  case AL_PROFILE_HEVC_MAIN_444: return 2000;
  case AL_PROFILE_HEVC_MAIN_INTRA: return 1000;
  case AL_PROFILE_HEVC_MAIN10_INTRA: return 1000;
  case AL_PROFILE_HEVC_MAIN12_INTRA: return 1500;
  case AL_PROFILE_HEVC_MAIN_422_INTRA: return 1667;
  case AL_PROFILE_HEVC_MAIN_422_10_INTRA: return 1667;
  case AL_PROFILE_HEVC_MAIN_422_12_INTRA: return 2000;
  case AL_PROFILE_HEVC_MAIN_444_INTRA: return 2000;
  case AL_PROFILE_HEVC_MAIN_444_10_INTRA: return 2500;
  case AL_PROFILE_HEVC_MAIN_444_12_INTRA: return 3000;
  case AL_PROFILE_HEVC_MAIN_444_16_INTRA: return 4000;
  case AL_PROFILE_HEVC_MAIN_444_STILL: return 2000;
  case AL_PROFILE_HEVC_MAIN_444_16_STILL: return 4000;

  default:
    return -1;
  }
}

/****************************************************************************/
static int AL_sSettings_GetHbrFactor(AL_EProfile eProfile)
{
  return (AL_IS_HEVC(eProfile) && !AL_IS_LOW_BITRATE_PROFILE(eProfile)) ? 2 : 1;
}

/****************************************************************************/
static uint32_t AL_sSettings_GetMaxCPBSize(AL_TEncSettings const* pSettings)
{
  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pSettings->tChParam.eProfile);
  uint32_t uCpbSize = 0;

  if(AL_IS_AVC(pSettings->tChParam.eProfile))
  {
    switch(pSettings->tChParam.uLevel)
    {
    case 10: uCpbSize = 175u;
      break;
    case 9: uCpbSize = 350u;
      break; // 1b
    case 11: uCpbSize = 500u;
      break;
    case 12: uCpbSize = 1000u;
      break;
    case 13: uCpbSize = 2000u;
      break;
    case 20: uCpbSize = 2000u;
      break;
    case 21: uCpbSize = 4000u;
      break;
    case 22: uCpbSize = 4000u;
      break;
    case 30: uCpbSize = 10000u;
      break;
    case 31: uCpbSize = 14000u;
      break;
    case 32: uCpbSize = 20000u;
      break;
    case 40: uCpbSize = 25000u;
      break;
    case 41: uCpbSize = 62500u;
      break;
    case 42: uCpbSize = 62500u;
      break;
    case 50: uCpbSize = 135000u;
      break;
    case 51: uCpbSize = 240000u;
      break;
    case 52: uCpbSize = 240000u;
      break;
    case 60: uCpbSize = 240000u;
      break;
    case 61: uCpbSize = 480000u;
      break;
    case 62: uCpbSize = 800000u;
      break;
    default: assert(0);
      break;
    }
  }
  else if(AL_IS_HEVC(pSettings->tChParam.eProfile))
  {
    if(pSettings->tChParam.uTier)
    {
      switch(pSettings->tChParam.uLevel)
      {
      case 10: uCpbSize = 350u;
        break;
      case 20: uCpbSize = 1500u;
        break;
      case 21: uCpbSize = 3000u;
        break;
      case 30: uCpbSize = 6000u;
        break;
      case 31: uCpbSize = 10000u;
        break;
      case 40: uCpbSize = 30000u;
        break;
      case 41: uCpbSize = 50000u;
        break;
      case 50: uCpbSize = 100000u;
        break;
      case 51: uCpbSize = 160000u;
        break;
      case 52: uCpbSize = 240000u;
        break;
      case 60: uCpbSize = 240000u;
        break;
      case 61: uCpbSize = 480000u;
        break;
      case 62: uCpbSize = 800000u;
        break;
      }
    }
    else
    {
      switch(pSettings->tChParam.uLevel)
      {
      case 10: uCpbSize = 350u;
        break;
      case 20: uCpbSize = 1500u;
        break;
      case 21: uCpbSize = 3000u;
        break;
      case 30: uCpbSize = 6000u;
        break;
      case 31: uCpbSize = 10000u;
        break;
      case 40: uCpbSize = 12000u;
        break;
      case 41: uCpbSize = 20000u;
        break;
      case 50: uCpbSize = 25000u;
        break;
      case 51: uCpbSize = 40000u;
        break;
      case 52: uCpbSize = 60000u;
        break;
      case 60: uCpbSize = 60000u;
        break;
      case 61: uCpbSize = 120000u;
        break;
      case 62: uCpbSize = 240000u;
        break;
      }
    }
  }
  return uCpbSize * iCpbVclFactor;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the AVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelAVC(AL_TEncSettings const* pSettings)
{
  uint8_t uLevel1 = 10;
  uint8_t uLevel2 = 10;
  uint8_t uLevel3 = 10;

  int iMaxMB = ((pSettings->tChParam.uWidth + 15) >> 4) * ((pSettings->tChParam.uHeight + 15) >> 4);
  int iBrVclFactor = AL_sSettings_GetCpbVclFactor(pSettings->tChParam.eProfile);
  int iMaxBR = (pSettings->tChParam.tRCParam.uMaxBitRate + (iBrVclFactor - 1)) / iBrVclFactor;

  // Max Frame Size
  if(iMaxMB <= 99)
    uLevel1 = 10;
  else if(iMaxMB <= 396)
    uLevel1 = 11;
  else if(iMaxMB <= 792)
    uLevel1 = 21;
  else if(iMaxMB <= 1620)
    uLevel1 = 22;
  else if(iMaxMB <= 3600)
    uLevel1 = 31;
  else if(iMaxMB <= 5120)
    uLevel1 = 32;
  else if(iMaxMB <= 8192)
    uLevel1 = 40;
  else if(iMaxMB <= 8704)
    uLevel1 = 42;
  else if(iMaxMB <= 22080)
    uLevel1 = 50;
  else if(iMaxMB <= 36864)
    uLevel1 = 51;
  else if(iMaxMB <= 139264)
    uLevel1 = 60;
  else
    return -1;

  // Max MB processing rate
  iMaxMB *= pSettings->tChParam.tRCParam.uFrameRate;

  if(iMaxMB <= 1485)
    uLevel2 = 10;
  else if(iMaxMB <= 3000)
    uLevel2 = 11;
  else if(iMaxMB <= 6000)
    uLevel2 = 12;
  else if(iMaxMB <= 11880)
    uLevel2 = 13;
  else if(iMaxMB <= 19800)
    uLevel2 = 21;
  else if(iMaxMB <= 20250)
    uLevel2 = 22;
  else if(iMaxMB <= 40500)
    uLevel2 = 30;
  else if(iMaxMB <= 108000)
    uLevel2 = 31;
  else if(iMaxMB <= 216000)
    uLevel2 = 32;
  else if(iMaxMB <= 245760)
    uLevel2 = 40;
  else if(iMaxMB <= 522240)
    uLevel2 = 42;
  else if(iMaxMB <= 589824)
    uLevel2 = 50;
  else if(iMaxMB <= 983040)
    uLevel2 = 51;
  else if(iMaxMB <= 2073600)
    uLevel2 = 52;
  else if(iMaxMB <= 4177920)
    uLevel2 = 60;
  else if(iMaxMB <= 8355840)
    uLevel2 = 61;
  else if(iMaxMB <= 16711680)
    uLevel2 = 62;
  else
    return -1;

  // Max Video BitRate
  if(iMaxBR <= 64)
    uLevel3 = 10;
  else if(iMaxBR <= 128)
    uLevel3 = 9; // 1b
  else if(iMaxBR <= 192)
    uLevel3 = 11;
  else if(iMaxBR <= 384)
    uLevel3 = 12;
  else if(iMaxBR <= 768)
    uLevel3 = 13;
  else if(iMaxBR <= 2000)
    uLevel3 = 20;
  else if(iMaxBR <= 4000)
    uLevel3 = 21;
  else if(iMaxBR <= 10000)
    uLevel3 = 30;
  else if(iMaxBR <= 14000)
    uLevel3 = 31;
  else if(iMaxBR <= 20000)
    uLevel3 = 32;
  else if(iMaxBR <= 50000)
    uLevel3 = 41;
  else if(iMaxBR <= 135000)
    uLevel3 = 50;
  else if(iMaxBR <= 240000)
    uLevel3 = 51;
  else if(iMaxBR <= 480000)
    uLevel3 = 61;
  else if(iMaxBR <= 800000)
    uLevel3 = 62;
  else
    return -1;

  return (uLevel1 > uLevel2) ? (uLevel1 > uLevel3) ? uLevel1 : uLevel3
         : (uLevel2 > uLevel3) ? uLevel2 : uLevel3;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the HEVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelHEVC(AL_TEncSettings const* pSettings)
{
  uint8_t uLevel1 = 10u;
  uint8_t uLevel2 = 10u;
  uint8_t uLevel3 = 10u;

  uint32_t uMaxSample = pSettings->tChParam.uWidth * pSettings->tChParam.uHeight;

  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pSettings->tChParam.eProfile);
  int iHbrFactor = AL_sSettings_GetHbrFactor(pSettings->tChParam.eProfile);
  int iBrVclFactor = iCpbVclFactor * iHbrFactor;
  uint32_t uBitRate = (pSettings->tChParam.tRCParam.uMaxBitRate + (iBrVclFactor - 1)) / iBrVclFactor;

  // Max Frame Size
  if(uMaxSample <= 36864u)
    uLevel1 = 10u;
  else if(uMaxSample <= 122880u)
    uLevel1 = 20u;
  else if(uMaxSample <= 245760u)
    uLevel1 = 21u;
  else if(uMaxSample <= 552960u)
    uLevel1 = 30u;
  else if(uMaxSample <= 983040u)
    uLevel1 = 31u;
  else if(uMaxSample <= 2228224u)
    uLevel1 = 40u;
  else if(uMaxSample <= 8912896u)
    uLevel1 = 50u;
  else if(uMaxSample <= 35651584u)
    uLevel1 = 60u;
  else
    return -1;

  // Max MB processing rate
  uMaxSample *= pSettings->tChParam.tRCParam.uFrameRate;

  if(uMaxSample <= 552960u)
    uLevel2 = 10u;
  else if(uMaxSample <= 3686400u)
    uLevel2 = 20u;
  else if(uMaxSample <= 7372800u)
    uLevel2 = 21u;
  else if(uMaxSample <= 16588800u)
    uLevel2 = 30u;
  else if(uMaxSample <= 33177600u)
    uLevel2 = 31u;
  else if(uMaxSample <= 66846720u)
    uLevel2 = 40u;
  else if(uMaxSample <= 133693440u)
    uLevel2 = 41u;
  else if(uMaxSample <= 267386880u)
    uLevel2 = 50u;
  else if(uMaxSample <= 534773760u)
    uLevel2 = 51u;
  else if(uMaxSample <= 1069547520u)
    uLevel2 = 52u;
  else if(uMaxSample <= 2139095040u)
    uLevel2 = 61u;
  else if(uMaxSample <= 4278190080u)
    uLevel2 = 62u;
  else
    return -1;

  // Max Video BitRate
  if(pSettings->tChParam.uTier)
  {
    // High Tier
    if(uBitRate <= 128u)
      uLevel3 = 10u;
    else if(uBitRate <= 1500u)
      uLevel3 = 20u;
    else if(uBitRate <= 3000u)
      uLevel3 = 21u;
    else if(uBitRate <= 6000u)
      uLevel3 = 30u;
    else if(uBitRate <= 10000u)
      uLevel3 = 31u;
    else if(uBitRate <= 30000u)
      uLevel3 = 40u;
    else if(uBitRate <= 50000u)
      uLevel3 = 41u;
    else if(uBitRate <= 100000u)
      uLevel3 = 50u;
    else if(uBitRate <= 160000u)
      uLevel3 = 51u;
    else if(uBitRate <= 240000u)
      uLevel3 = 52u;
    else if(uBitRate <= 480000u)
      uLevel3 = 61u;
    else if(uBitRate <= 800000u)
      uLevel3 = 62u;
    else
      return -1;
  }
  else
  {
    // Main Tier
    if(uBitRate <= 128)
      uLevel3 = 10;
    else if(uBitRate <= 1500)
      uLevel3 = 20;
    else if(uBitRate <= 3000)
      uLevel3 = 21;
    else if(uBitRate <= 6000)
      uLevel3 = 30;
    else if(uBitRate <= 10000)
      uLevel3 = 31;
    else if(uBitRate <= 12000)
      uLevel3 = 40;
    else if(uBitRate <= 20000)
      uLevel3 = 41;
    else if(uBitRate <= 25000)
      uLevel3 = 50;
    else if(uBitRate <= 40000)
      uLevel3 = 51;
    else if(uBitRate <= 60000)
      uLevel3 = 52;
    else if(uBitRate <= 120000)
      uLevel3 = 61;
    else if(uBitRate <= 240000)
      uLevel3 = 62;
    else
      return -1;
  }

  return (uLevel1 > uLevel2) ? (uLevel1 > uLevel3) ? uLevel1 : uLevel3
         : (uLevel2 > uLevel3) ? uLevel2 : uLevel3;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the HEVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevel(AL_TEncSettings const* pSettings)
{
  if(AL_IS_HEVC(pSettings->tChParam.eProfile))
    return AL_sSettings_GetMinLevelHEVC(pSettings);

  if(AL_IS_AVC(pSettings->tChParam.eProfile))
    return AL_sSettings_GetMinLevelAVC(pSettings);
  return -1;
}

/***************************************************************************/
static void AL_sSettings_SetDefaultAVCParam(AL_TEncSettings* pSettings)
{
  pSettings->tChParam.uMaxCuSize = 4; // 16x16
}

/***************************************************************************/


/***************************************************************************/
void AL_Settings_SetDefaults(AL_TEncSettings* pSettings)
{
  assert(pSettings);
  Rtos_Memset(pSettings, 0, sizeof(*pSettings));

  pSettings->tChParam.eProfile = AL_PROFILE_HEVC_MAIN;
  pSettings->tChParam.uLevel = 51;
  pSettings->tChParam.uTier = 0; // MAIN_TIER
  pSettings->tChParam.eOptions = AL_OPT_LF | AL_OPT_LF_X_SLICE | AL_OPT_LF_X_TILE;
  pSettings->tChParam.eOptions |= AL_OPT_RDO_COST_MODE;

  pSettings->tChParam.ePicFormat = AL_420_8BITS;

  pSettings->tChParam.tGopParam.eMode = AL_GOP_MODE_DEFAULT;
  pSettings->tChParam.tGopParam.uFreqIDR = 0x7FFFFFFF;

  pSettings->tChParam.tGopParam.uGopLength = 30;
  pSettings->tChParam.tGopParam.eGdrMode = AL_GDR_OFF;


  pSettings->tChParam.tRCParam.eRCMode = AL_RC_CONST_QP;
  pSettings->tChParam.tRCParam.uTargetBitRate = 4000000;
  pSettings->tChParam.tRCParam.uMaxBitRate = 4000000;
  pSettings->tChParam.tRCParam.iInitialQP = 30;
  pSettings->tChParam.tRCParam.iMinQP = 0;
  pSettings->tChParam.tRCParam.iMaxQP = 51;
  pSettings->tChParam.tRCParam.uFrameRate = 30;
  pSettings->tChParam.tRCParam.uClkRatio = 1000;
  pSettings->tChParam.tRCParam.uInitialRemDelay = 135000; // 1.5 s
  pSettings->tChParam.tRCParam.uCPBSize = 270000; // 3.0 s
  pSettings->tChParam.tRCParam.uIPDelta = -1;
  pSettings->tChParam.tRCParam.uPBDelta = -1;
  pSettings->tChParam.tRCParam.eOptions = AL_RC_OPT_NONE;

  pSettings->tChParam.tRCParam.bUseGoldenRef = false;
  pSettings->tChParam.tRCParam.uGoldenRefFrequency = 10;
  pSettings->tChParam.tRCParam.uPGoldenDelta = 2;

  pSettings->tChParam.iTcOffset = -1;
  pSettings->tChParam.iBetaOffset = -1;

  pSettings->tChParam.eColorSpace = UNKNOWN;

  pSettings->tChParam.uNumCore = NUMCORE_AUTO;
  pSettings->tChParam.uNumSlices = 1;

  pSettings->uEnableSEI = SEI_NONE;
  pSettings->bEnableAUD = true;
  pSettings->bEnableFillerData = true;
  pSettings->eAspectRatio = AL_ASPECT_RATIO_AUTO;
  pSettings->eColourDescription = COLOUR_DESC_BT_470_PAL;

  pSettings->eQpCtrlMode = UNIFORM_QP;// ADAPTIVE_AUTO_QP;
  pSettings->tChParam.eLdaCtrlMode = DEFAULT_LDA;

  pSettings->eScalingList = AL_SCL_DEFAULT;

  pSettings->bForceLoad = true;
  pSettings->tChParam.pMeRange[SLICE_P][0] = -1; // Horz
  pSettings->tChParam.pMeRange[SLICE_P][1] = -1; // Vert
  pSettings->tChParam.pMeRange[SLICE_B][0] = -1; // Horz
  pSettings->tChParam.pMeRange[SLICE_B][1] = -1; // Vert
  pSettings->tChParam.uMaxCuSize = 5; // 32x32
  pSettings->tChParam.uMinCuSize = 3; // 8x8
  pSettings->tChParam.uMaxTuSize = 5; // 32x32
  pSettings->tChParam.uMinTuSize = 2; // 4x4
  pSettings->tChParam.uMaxTransfoDepthIntra = 1;
  pSettings->tChParam.uMaxTransfoDepthInter = 1;

  pSettings->NumView = 1;
  pSettings->tChParam.eEntropyMode = AL_MODE_CABAC;
  pSettings->tChParam.eWPMode = AL_WP_DEFAULT;

  pSettings->tChParam.eSrcMode = AL_SRC_NVX;


}

/***************************************************************************/
#define MSG(msg) { if(pOut) fprintf(pOut, msg "\r\n"); }

#define STRINGER(a) # a

/****************************************************************************/
static void AL_sCheckRange(int16_t* pRange, const int16_t iMaxRange, FILE* pOut)
{
  if(*pRange > iMaxRange)
  {
    MSG("The Specified Range is too high !!");
    *pRange = iMaxRange;
  }
}

void AL_Settings_SetDefaultParam(AL_TEncSettings* pSettings)
{
  if(AL_IS_AVC(pSettings->tChParam.eProfile))
    AL_sSettings_SetDefaultAVCParam(pSettings);


  if(AL_IS_HEVC(pSettings->tChParam.eProfile) && pSettings->tChParam.uCabacInitIdc > 1)
  {
    pSettings->tChParam.uCabacInitIdc = 1;
  }
}

/***************************************************************************/
int AL_Settings_CheckValidity(AL_TEncSettings* pSettings, FILE* pOut)
{
  int iMaxQP = 51;
  int iMaxSlices = 0;
  int iRound = 0;
  int err = 0;

  uint8_t uNumCore = pSettings->tChParam.uNumCore;

  if(uNumCore == NUMCORE_AUTO)
  {
    const int maximumResourcesForOneCore = GetCoreResources(ENCODER_CORE_FREQUENCY, ENCODER_CORE_FREQUENCY_MARGIN, ENCODER_CYCLES_FOR_BLK_32X32);
    uNumCore = ChoseCoresCount(pSettings->tChParam.uWidth, pSettings->tChParam.uHeight, pSettings->tChParam.tRCParam.uFrameRate, pSettings->tChParam.tRCParam.uClkRatio, maximumResourcesForOneCore);
  }

  if(!AL_sSettings_CheckProfile(pSettings->tChParam.eProfile))
  {
    ++err;
    MSG("Invalid parameter : Profile");
  }

  if((pSettings->tChParam.eProfile == AL_PROFILE_HEVC_MAIN10) && (AL_GET_BITDEPTH(pSettings->tChParam.ePicFormat) > 8) && (HW_IP_BIT_DEPTH < 10))
  {
    ++err;
    MSG("The hardware IP doesn't support 10-bit encoding");
  }


  if(AL_GET_CHROMA_MODE(pSettings->tChParam.ePicFormat) == CHROMA_4_4_4)
  {
    ++err;
    MSG("The specified ChromaMode is not supported by the IP");
  }

  if(!AL_sSettings_CheckLevel(pSettings->tChParam.eProfile, pSettings->tChParam.uLevel))
  {
    ++err;
    MSG("Invalid parameter : Level");
  }

  if(pSettings->tChParam.tRCParam.iInitialQP > iMaxQP)
  {
    ++err;
    MSG("Invalid parameter : SliceQP");
  }

  if(-12 > pSettings->tChParam.iCrPicQpOffset || pSettings->tChParam.iCrPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter : CrQpOffset");
  }

  if(-12 > pSettings->tChParam.iCbPicQpOffset || pSettings->tChParam.iCbPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter : CbQpOffset");
  }

  if(pSettings->tChParam.tRCParam.uTargetBitRate < 10)
  {
    if((pSettings->tChParam.tRCParam.eRCMode == AL_RC_CBR) || (pSettings->tChParam.tRCParam.eRCMode == AL_RC_VBR))
    {
      ++err;
      MSG("Invalid parameter : BitRate");
    }
  }

  if(pSettings->tChParam.tRCParam.eRCMode == AL_RC_LOW_LATENCY)
  {
    if(pSettings->tChParam.tGopParam.uNumB > 0)
    {
      ++err;
      MSG("B Picture not allowed in LOW_LATENCY Mode");
    }
  }

  if(pSettings->tChParam.tGopParam.eMode == AL_GOP_MODE_DEFAULT)
  {
    if(pSettings->tChParam.tGopParam.uGopLength > 1000)
    {
      ++err;
      MSG("Invalid parameter : Gop.Length");
    }

    if(pSettings->tChParam.tGopParam.uNumB > 4)
    {
      ++err;
      MSG("Invalid parameter : Gop.NumB");
    }
  }
  iRound = AL_IS_HEVC(pSettings->tChParam.eProfile) ? 32 : 16;
  iMaxSlices = (pSettings->tChParam.uHeight + (iRound / 2)) / iRound;

  if((pSettings->tChParam.uNumSlices < 1) || (pSettings->tChParam.uNumSlices > iMaxSlices) || (pSettings->tChParam.uNumSlices > AL_MAX_ENC_SLICE))
  {
    ++err;
    MSG("Invalid parameter : NumSlices");
  }

  if(pSettings->tChParam.eOptions & AL_OPT_WPP)
  {
    if(pSettings->tChParam.uNumSlices * uNumCore * iRound > pSettings->tChParam.uHeight)
    {
      ++err;
      MSG("Invalid parameter : NumSlices (Too many slices for multi-core Wavefront encoding)");
    }
  }

  if(pSettings->tChParam.uWidth % 8 != 0 || pSettings->tChParam.uHeight % 8 != 0)
  {
    ++err;
    MSG("Width and Height shall be multiple of 8 ! ");
  }

  if(pSettings->tChParam.tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && (pSettings->tChParam.tGopParam.uNumB != 3 && pSettings->tChParam.tGopParam.uNumB != 5 && pSettings->tChParam.tGopParam.uNumB != 7))
  {
    ++err;
    MSG("!! PYRAMIDAL GOP pattern can only allowed 3, 5, 7 B Frames !!");
  }

  if(AL_IS_HEVC(pSettings->tChParam.eProfile))
  {
    int iOffset = pSettings->tChParam.iCrPicQpOffset + pSettings->tChParam.iCrSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter : CrQpOffset");
    }

    iOffset = pSettings->tChParam.iCbPicQpOffset + pSettings->tChParam.iCbSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter : CbQpOffset");
    }
  }
  else if(AL_IS_AVC(pSettings->tChParam.eProfile))
  {
    if(pSettings->tChParam.tRCParam.iInitialQP >= 0)
    {
      int iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCrSliceQpOffset;

      if(pSettings->eQpCtrlMode == UNIFORM_QP && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter : SliceQP, CrQpOffset");
      }
      iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCbSliceQpOffset;

      if(pSettings->eQpCtrlMode == UNIFORM_QP && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter : SliceQP, CbQpOffset");
      }
    }

    if(uNumCore > 1 && pSettings->tChParam.uSliceSize > 0)
    {
      ++err;
      MSG("Fixed-Size slices are not allowed in multi-core AVC encoding");
    }
  }

  if((pSettings->eQpCtrlMode & MASK_AUTO_QP) && (pSettings->eQpCtrlMode & MASK_QP_TABLE))
  {
    ++err;
    MSG("!! QP control mode not allowed !!");
  }

  return err;
}

/******************************************************************************/
static uint32_t GetHevcMaxTileRow(AL_TEncSettings const* pSettings)
{
  switch(pSettings->tChParam.uLevel)
  {
  case 10:
  case 20:
  case 21:
    return 1;
    break;
  case 30:
    return 2;
    break;
  case 31:
    return 3;
    break;
  case 40:
  case 41:
    return 5;
    break;
  case 50:
  case 51:
  case 52:
    return 11;
    break;
  case 60:
  case 61:
  case 62:
    return 22;
    break;
  default:
    printf("level:%d\n", pSettings->tChParam.uLevel);
    assert(0);
    return 1;
  }
}

/***************************************************************************/
int AL_Settings_CheckCoherency(AL_TEncSettings* pSettings, TFourCC tFourCC, FILE* pOut)
{
  int numIncoherency = 0;
  int iBitDepth;
  AL_EChromaMode eInputChromaMode;

  int16_t iMaxPRange = AL_IS_AVC(pSettings->tChParam.eProfile) ? AVC_MAX_HORIZONTAL_RANGE_P : HEVC_MAX_HORIZONTAL_RANGE_P;
  int16_t iMaxBRange = AL_IS_AVC(pSettings->tChParam.eProfile) ? AVC_MAX_HORIZONTAL_RANGE_B : HEVC_MAX_HORIZONTAL_RANGE_B;

  assert(pSettings);

  iBitDepth = AL_GET_BITDEPTH(pSettings->tChParam.ePicFormat);

  if((!AL_IS_10BIT_PROFILE(pSettings->tChParam.eProfile) && (iBitDepth != 8))
     || (AL_IS_10BIT_PROFILE(pSettings->tChParam.eProfile) && (iBitDepth < 8 || iBitDepth > 10)))
  {
    MSG("!! The specified BitDepth is not allowed in this profile; the stream will be encoded in 8 bit !!");
    pSettings->tChParam.ePicFormat = AL_420_8BITS;
    ++numIncoherency;
  }

  if(AL_GetBitDepth(tFourCC) < iBitDepth)
  {
    MSG("!! Warning : The input BitDepth is smaller than encoding bitdepth !!");
    ++numIncoherency;
  }


  if((pSettings->eQpCtrlMode & MASK_QP_TABLE) == ROI_QP)
    pSettings->eQpCtrlMode |= RELATIVE_QP;

  if(pSettings->eQpCtrlMode & (MASK_AUTO_QP | MASK_QP_TABLE))
  {
    if((pSettings->tChParam.uMaxCuSize - pSettings->tChParam.uCuQPDeltaDepth) < pSettings->tChParam.uMinCuSize)
    {
      MSG("!! Warning : CuQpDeltaDepth doesn't match MinCUSize !!");
      ++numIncoherency;
      pSettings->tChParam.uCuQPDeltaDepth = pSettings->tChParam.uMaxCuSize - pSettings->tChParam.uMinCuSize;
    }

    if(pSettings->tChParam.uCuQPDeltaDepth > 2)
    {
      MSG("!! Warning : CuQpDeltaDepth shall be less then or equal to 2 !");
      ++numIncoherency;
      pSettings->tChParam.uCuQPDeltaDepth = 2;
    }
  }
  else
  {
    pSettings->tChParam.uCuQPDeltaDepth = 0;
  }

  if(pSettings->tChParam.tRCParam.eRCMode == AL_RC_CBR)
  {
    pSettings->tChParam.tRCParam.uMaxBitRate = pSettings->tChParam.tRCParam.uTargetBitRate;
  }

  // ChromaMode and profile depend on input format
  eInputChromaMode = AL_GetChromaMode(tFourCC);

  {
    if(pSettings->tChParam.tRCParam.eRCMode == AL_RC_CBR)
    {
      AL_TDimension tDim = { pSettings->tChParam.uWidth, pSettings->tChParam.uHeight };
      uint32_t maxNalSizeInByte = GetPcmVclNalSize(tDim, eInputChromaMode, iBitDepth);
      uint64_t maxBitRate = 8LL * maxNalSizeInByte * pSettings->tChParam.tRCParam.uFrameRate;

      if(pSettings->tChParam.tRCParam.uTargetBitRate > maxBitRate)
      {
        MSG("!! Warning specified TargetBitRate is too high for this use case and will be adjusted!!");
        pSettings->tChParam.tRCParam.uTargetBitRate = maxBitRate;
        pSettings->tChParam.tRCParam.uMaxBitRate = maxBitRate;
        ++numIncoherency;
      }
    }

    uint8_t uMinLevel = AL_sSettings_GetMinLevel(pSettings);

    if(uMinLevel == 255)
    {
      MSG("!! The specified configuration requires a level too high for the IP encoder!!");
      return -1;
    }

    if(pSettings->tChParam.uLevel < uMinLevel)
    {
      if(!AL_sSettings_CheckLevel(pSettings->tChParam.eProfile, uMinLevel))
      {
        MSG("!! The specified configuration requires a level too high for the IP encoder!!");
        ++numIncoherency;
      }
      else
      {
        MSG("!! The specified Level is too low and will be adjusted !!");
        ++numIncoherency;
      }
      pSettings->tChParam.uLevel = uMinLevel;
    }
  }

  if(AL_IS_HEVC(pSettings->tChParam.eProfile) && pSettings->tChParam.uLevel < 40 && pSettings->tChParam.uTier)
  {
    pSettings->tChParam.uTier = 0;
  }

  {
    if(pSettings->tChParam.tRCParam.eRCMode != AL_RC_CONST_QP)
    {
      uint64_t uCPBSize = ((AL_64U)pSettings->tChParam.tRCParam.uCPBSize * pSettings->tChParam.tRCParam.uMaxBitRate) / 90000LL;
      uint32_t uMaxCPBSize = AL_sSettings_GetMaxCPBSize(pSettings);

      if(uCPBSize > uMaxCPBSize)
      {
        MSG("!! Warning specified CPBSize is higher than the Max CPBSize allowed for this level and will be adjusted !!");
        ++numIncoherency;
        pSettings->tChParam.tRCParam.uCPBSize = uMaxCPBSize * 90000LL / pSettings->tChParam.tRCParam.uMaxBitRate;
      }
    }
  }

  if(pSettings->tChParam.tRCParam.eRCMode == AL_RC_CBR || pSettings->tChParam.tRCParam.eRCMode == AL_RC_VBR)
  {
    if(pSettings->tChParam.tRCParam.uCPBSize < pSettings->tChParam.tRCParam.uInitialRemDelay)
    {
      MSG("!! Warning specified InitialDelay is bigger than CPBSize and will be adjusted !!");
      ++numIncoherency;
      pSettings->tChParam.tRCParam.uInitialRemDelay = pSettings->tChParam.tRCParam.uCPBSize;
    }
  }

  AL_sCheckRange(&pSettings->tChParam.pMeRange[SLICE_P][0], iMaxPRange, pOut);
  AL_sCheckRange(&pSettings->tChParam.pMeRange[SLICE_P][1], iMaxPRange, pOut);
  AL_sCheckRange(&pSettings->tChParam.pMeRange[SLICE_B][0], iMaxBRange, pOut);
  AL_sCheckRange(&pSettings->tChParam.pMeRange[SLICE_B][1], iMaxBRange, pOut);

  if((eInputChromaMode == CHROMA_4_2_2 && (!AL_IS_422_PROFILE(pSettings->tChParam.eProfile) || AL_GET_CHROMA_MODE(pSettings->tChParam.ePicFormat) == CHROMA_4_2_0)) ||
     (eInputChromaMode == CHROMA_4_2_0 && AL_GET_CHROMA_MODE(pSettings->tChParam.ePicFormat) == CHROMA_4_2_2))
  {
    if(eInputChromaMode == CHROMA_4_2_2)
      pSettings->tChParam.eProfile = ((pSettings->tChParam.eProfile & AL_PROFILE_AVC) == AL_PROFILE_AVC) ? AL_PROFILE_AVC_HIGH_422 : AL_PROFILE_HEVC_MAIN_422_10;

    AL_SET_CHROMA_MODE(pSettings->tChParam.ePicFormat, (eInputChromaMode == CHROMA_4_2_2) ? CHROMA_4_2_2 : CHROMA_4_2_0);
    MSG("!! The specified ChromaMode and Profile are not allowed with this input format; they will be adjusted!!");
    ++numIncoherency;
  }

  if((pSettings->tChParam.uSliceSize > 0) && (pSettings->tChParam.eOptions & AL_OPT_WPP))
  {
    pSettings->tChParam.eOptions &= ~AL_OPT_WPP;
    MSG("!! Wavefront Parallel Processing is not allowed with SliceSize; it will be adjusted!!");
    ++numIncoherency;
  }

  if(AL_IS_INTRA_PROFILE(pSettings->tChParam.eProfile) && pSettings->tChParam.tGopParam.uGopLength != 0)
  {
    pSettings->tChParam.tGopParam.uGopLength = 0;
    MSG("!! Gop.Length shall be set to 0 for Intra only profile; it will be adjusted!!");
    ++numIncoherency;

    if(AL_IS_AVC(pSettings->tChParam.eProfile))
      pSettings->tChParam.tGopParam.uFreqIDR = 0;
  }

  if(AL_IS_AVC(pSettings->tChParam.eProfile))
  {
    pSettings->tChParam.uMaxTuSize = 3;
    pSettings->tChParam.iCbSliceQpOffset = pSettings->tChParam.iCrSliceQpOffset = 0;

    if(pSettings->tChParam.eOptions & AL_OPT_WPP)
      pSettings->tChParam.eOptions &= ~AL_OPT_WPP;

    if(pSettings->tChParam.uMaxCuSize != 4 || pSettings->tChParam.uMinCuSize != 3)
    {
      pSettings->tChParam.uMaxCuSize = 4;
      pSettings->tChParam.uMinCuSize = 3;
      MSG("!! The Specified MaxCUSize and MinCUSize are not allowed; they will be adjusted!!");
      ++numIncoherency;
    }

    if(AL_GET_PROFILE_IDC(pSettings->tChParam.eProfile) < AL_GET_PROFILE_IDC(AL_PROFILE_AVC_HIGH))
    {
      if(pSettings->tChParam.uMaxTuSize != 2)
      {
        // cannot be set by cfg
        pSettings->tChParam.uMaxTuSize = 2;
      }

      if(pSettings->tChParam.iCrPicQpOffset != pSettings->tChParam.iCbPicQpOffset)
      {
        pSettings->tChParam.iCrPicQpOffset = pSettings->tChParam.iCbPicQpOffset;
        MSG("!! The Specified CrQpOffset is not allowed; it will be adjusted!!");
        ++numIncoherency;
      }

      if(pSettings->eScalingList != AL_SCL_FLAT)
      {
        pSettings->eScalingList = AL_SCL_FLAT;
        MSG("!! The specified ScalingList is not allowed; it will be adjusted!!");
        ++numIncoherency;
      }

      if(AL_GET_CHROMA_MODE(pSettings->tChParam.ePicFormat) != CHROMA_4_2_0)
      {
        AL_SET_CHROMA_MODE(pSettings->tChParam.ePicFormat, CHROMA_4_2_0);
        MSG("!! The specified ChromaMode and Profile are not allowed; they will be adjusted!!");
        ++numIncoherency;
      }
    }

    if(AL_GET_PROFILE_IDC(pSettings->tChParam.eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE))
    {
      if(pSettings->tChParam.eEntropyMode == AL_MODE_CABAC)
      {
        pSettings->tChParam.eEntropyMode = AL_MODE_CAVLC;
        MSG("!! CABAC encoding is not allowed in baseline profile; CAVLC will be used instead !!");
        ++numIncoherency;
      }
    }

    if(pSettings->tChParam.eProfile == AL_PROFILE_AVC_BASELINE
       || pSettings->tChParam.eProfile == AL_PROFILE_AVC_C_BASELINE
       || pSettings->tChParam.eProfile == AL_PROFILE_AVC_C_HIGH)
    {
      if(pSettings->tChParam.tGopParam.uNumB != 0)
      {
        pSettings->tChParam.tGopParam.uNumB = 0;
        MSG("!! The specified NumB is not allowed in this profile; it will be adjusted!!");
        ++numIncoherency;
      }
    }

    if(pSettings->tChParam.uCuQPDeltaDepth > 0)
    {
      MSG("!! In AVC CuQPDeltaDepth is not allowed; it will be adjusted !!");
      pSettings->tChParam.uCuQPDeltaDepth = 0;
      ++numIncoherency;
    }
  }


  if(pSettings->tChParam.tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B && (pSettings->tChParam.tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH ||
                                                                        pSettings->tChParam.tGopParam.uGopLength < MIN_LOW_DELAY_B_GOP_LENGTH))
  {
    pSettings->tChParam.tGopParam.uGopLength = (pSettings->tChParam.tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH) ? MAX_LOW_DELAY_B_GOP_LENGTH : MIN_LOW_DELAY_B_GOP_LENGTH;
    MSG("!! The specified Gop.Length value in low delay B mode is not allowed, value will be adjusted !!");
    ++numIncoherency;
  }
  
  if(pSettings->tChParam.tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && ((pSettings->tChParam.tGopParam.uGopLength - 1) % (pSettings->tChParam.tGopParam.uNumB + 1)) != 0)
  {
    int iRound = pSettings->tChParam.tGopParam.uNumB + 1;
    pSettings->tChParam.tGopParam.uGopLength = (((pSettings->tChParam.tGopParam.uGopLength + iRound - 1) / iRound) * iRound) + 1;
    MSG("!! The specified Gop.Length value in pyramidal gop mode is not reachable, value will be adjusted !!");
    ++numIncoherency;
  }
  
  if((pSettings->eQpCtrlMode == ADAPTIVE_AUTO_QP || pSettings->eQpCtrlMode == AUTO_QP) && pSettings->tChParam.tRCParam.iInitialQP >= 0)
  {
    if(AL_IS_HEVC(pSettings->tChParam.eProfile))
    {
      int iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCrPicQpOffset + pSettings->tChParam.iCrSliceQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pSettings->tChParam.iCrPicQpOffset = pSettings->tChParam.iCrSliceQpOffset = 0;
        MSG("!!With this QPControlMode, this CrQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
      iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCbPicQpOffset + pSettings->tChParam.iCbSliceQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pSettings->tChParam.iCbPicQpOffset = pSettings->tChParam.iCbSliceQpOffset = 0;
        MSG("!!With this QPControlMode, this CbQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
    }
    else if(AL_IS_AVC(pSettings->tChParam.eProfile))
    {
      int iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCrPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pSettings->tChParam.iCrPicQpOffset = 0;
        MSG("!!With this QPControlMode, this CrQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
      iQP = pSettings->tChParam.tRCParam.iInitialQP + pSettings->tChParam.iCbPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pSettings->tChParam.iCbPicQpOffset = 0;
        MSG("!!With this QPControlMode, this CbQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
    }
  }

  if(AL_IS_HEVC(pSettings->tChParam.eProfile))
  {
    uint8_t uNumCore = pSettings->tChParam.uNumCore;
    int maximumResourcesForOneCore = GetCoreResources(ENCODER_CORE_FREQUENCY, ENCODER_CORE_FREQUENCY_MARGIN, ENCODER_CYCLES_FOR_BLK_32X32);

    if(uNumCore == NUMCORE_AUTO)
      uNumCore = ChoseCoresCount(pSettings->tChParam.uWidth, pSettings->tChParam.uHeight,
                                 pSettings->tChParam.tRCParam.uFrameRate, pSettings->tChParam.tRCParam.uClkRatio, maximumResourcesForOneCore);

    if(uNumCore > 1 && pSettings->tChParam.uNumSlices > GetHevcMaxTileRow(pSettings))
    {
      pSettings->tChParam.uNumSlices = Clip3(pSettings->tChParam.uNumSlices, 1, GetHevcMaxTileRow(pSettings));
      MSG("!!With this Configuration, this NumSlices cannot be set!!");
      ++numIncoherency;
    }
  }

  if(pSettings->tChParam.tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && pSettings->tChParam.tGopParam.uFreqLT > 0)
  {
    pSettings->tChParam.tGopParam.uFreqLT = 0;
    MSG("!! Long Term reference are not allowed with PYRAMIDAL GOP, it will be adjusted !!");
    ++numIncoherency;
  }

  if(pSettings->tChParam.tGopParam.eGdrMode == AL_GDR_VERTICAL)
    pSettings->tChParam.eOptions |= AL_OPT_CONST_INTRA_PRED;

  return numIncoherency;
}

/*@}*/

