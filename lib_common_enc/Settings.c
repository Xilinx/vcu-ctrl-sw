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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup Encoder_Settings
   @{
   \file
 *****************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/Settings.h"
#include "lib_common/ChannelResources.h"
#include "lib_common/Utils.h"
#include "lib_common/StreamBufferPrivate.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/EncSize.h"
#include "lib_common_enc/DPBConstraints.h"
#include "lib_common/SEI.h"

int LAMBDA_FACTORS[] = { 51, 90, 151, 151, 151, 151 }; // I, P, B(temporal id low to high)
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
  assert(AL_IS_HEVC(eProfile));

  if(AL_GET_PROFILE_CODED_AND_IDC(eProfile) != AL_PROFILE_HEVC_RExt)
    return 1;

  return !AL_IS_LOW_BITRATE_PROFILE(eProfile) ? 2 : 1;
}

/****************************************************************************/
static uint32_t AL_sSettings_GetMaxCPBSize(AL_TEncChanParam const* pChParam)
{
  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  uint32_t uCpbSize = 0;

  if(AL_IS_AVC(pChParam->eProfile))
  {
    switch(pChParam->uLevel)
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
  else if(AL_IS_HEVC(pChParam->eProfile))
  {
    if(pChParam->uTier)
    {
      switch(pChParam->uLevel)
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
      switch(pChParam->uLevel)
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

/*************************************************************************/
typedef struct AL_t_LevelLimit
{
  uint32_t uLimit;
  uint8_t uLevel;
}AL_TLevelLimit;

static uint8_t AL_sSettings_GetRequiredLevel(uint32_t uVal, const AL_TLevelLimit* pLevelLimits, int iNbLimits)
{
  for(int i = 0; i < iNbLimits; i++)
  {
    if(uVal <= pLevelLimits[i].uLimit)
      return pLevelLimits[i].uLevel;
  }

  return 255u;
}

// Max Frame MB
#define AVC_MAX_FRAME_MB_SIZE 11
static const AL_TLevelLimit AVC_MAX_FRAME_MB[AVC_MAX_FRAME_MB_SIZE] =
{
  { 99u, 10u },
  { 396u, 11u },
  { 792u, 21u },
  { 1620u, 22u },
  { 3600u, 31u },
  { 5120u, 32u },
  { 8192u, 40u },
  { 8704u, 42u },
  { 22080u, 50u },
  { 36864u, 51u },
  { 139264u, 60u },
};

// Max MB Rate
#define AVC_MAX_MB_RATE_SIZE 17
static const AL_TLevelLimit AVC_MAX_MB_RATE[AVC_MAX_MB_RATE_SIZE] =
{
  { 1485u, 10u },
  { 3000u, 11u },
  { 6000u, 12u },
  { 11880u, 13u },
  { 19800u, 21u },
  { 20250u, 22u },
  { 40500u, 30u },
  { 108000u, 31u },
  { 216000u, 32u },
  { 245760u, 40u },
  { 522240u, 42u },
  { 589824u, 50u },
  { 983040u, 51u },
  { 2073600u, 52u },
  { 4177920u, 60u },
  { 8355840u, 61u },
  { 16711680u, 62u }
};

// Max Video processing rate
#define AVC_MAX_VIDEO_BITRATE_SIZE 15
static const AL_TLevelLimit AVC_MAX_VIDEO_BITRATE[AVC_MAX_VIDEO_BITRATE_SIZE] =
{
  { 64u, 10u },
  { 128u, 9u }, // 1b
  { 192u, 11u },
  { 384u, 12u },
  { 768u, 13u },
  { 2000u, 20u },
  { 4000u, 21u },
  { 10000u, 30u },
  { 14000u, 31u },
  { 20000u, 32u },
  { 50000u, 41u },
  { 135000u, 50u },
  { 240000u, 51u },
  { 480000u, 61u },
  { 800000u, 62u }
};

// Max Video processing rate
#define AVC_MAX_VIDEO_DPB_SIZE_SIZE 12
static const AL_TLevelLimit AVC_MAX_VIDEO_DPB_SIZE[AVC_MAX_VIDEO_DPB_SIZE_SIZE] =
{
  { 396u, 10u },
  { 900u, 11u },
  { 2376u, 12u },
  { 4752u, 21u },
  { 8100u, 22u },
  { 18000u, 31u },
  { 20480u, 32u },
  { 32768u, 40u },
  { 34816u, 42u },
  { 110400u, 50u },
  { 184320u, 51u },
  { 696320u, 60u }
};

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the AVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelAVC(AL_TEncChanParam const* pChParam)
{
  uint8_t uLevel;

  int iMaxMB = ((pChParam->uWidth + 15) >> 4) * ((pChParam->uHeight + 15) >> 4);
  int iBrVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  int iMaxBR = (pChParam->tRCParam.uMaxBitRate + (iBrVclFactor - 1)) / iBrVclFactor;
  int iDPBSize = AL_DPBConstraint_GetMaxDPBSize(pChParam) * iMaxMB;

  uLevel = AL_sSettings_GetRequiredLevel(iMaxMB, AVC_MAX_FRAME_MB, AVC_MAX_FRAME_MB_SIZE);

  iMaxMB *= pChParam->tRCParam.uFrameRate;
  uLevel = Max(AL_sSettings_GetRequiredLevel(iMaxMB, AVC_MAX_MB_RATE, AVC_MAX_MB_RATE_SIZE), uLevel);

  uLevel = Max(AL_sSettings_GetRequiredLevel(iMaxBR, AVC_MAX_VIDEO_BITRATE, AVC_MAX_VIDEO_BITRATE_SIZE), uLevel);
  uLevel = Max(AL_sSettings_GetRequiredLevel(iDPBSize, AVC_MAX_VIDEO_DPB_SIZE, AVC_MAX_VIDEO_DPB_SIZE_SIZE), uLevel);

  return uLevel;
}

// Max Frame Size
#define HEVC_MAX_LUMA_SAMPLES_SIZE 8
static const AL_TLevelLimit HEVC_MAX_LUMA_SAMPLES[HEVC_MAX_LUMA_SAMPLES_SIZE] =
{
  { 36864u, 10u },
  { 122880u, 20u },
  { 245760u, 21u },
  { 552960u, 30u },
  { 983040u, 31u },
  { 2228224u, 40u },
  { 8912896u, 50u },
  { 35651584u, 60u }
};

// Max MB processing rate
#define HEVC_MAX_MB_RATE_SIZE 12
static const AL_TLevelLimit HEVC_MAX_MB_RATE[HEVC_MAX_MB_RATE_SIZE] =
{
  { 552960u, 10u },
  { 3686400u, 20u },
  { 7372800u, 21u },
  { 16588800u, 30u },
  { 33177600u, 31u },
  { 66846720u, 40u },
  { 133693440u, 41u },
  { 267386880u, 50u },
  { 534773760u, 51u },
  { 1069547520u, 52u },
  { 2139095040u, 61u },
  { 4278190080u, 62u }
};

// Max Video processing rate - High Tier
#define HEVC_MAX_VIDEO_BITRATE_HIGH_SIZE 12
static const AL_TLevelLimit HEVC_MAX_VIDEO_BITRATE_HIGH[HEVC_MAX_VIDEO_BITRATE_HIGH_SIZE] =
{
  { 128u, 10u },
  { 1500u, 20u },
  { 3000u, 21u },
  { 6000u, 30u },
  { 10000u, 31u },
  { 30000u, 40u },
  { 50000u, 41u },
  { 100000u, 50u },
  { 160000u, 51u },
  { 240000u, 52u },
  { 480000u, 61u },
  { 800000u, 62u }
};

// Max Video processing rate - MainTier
#define HEVC_MAX_VIDEO_BITRATE_MAIN_SIZE 12
static const AL_TLevelLimit HEVC_MAX_VIDEO_BITRATE_MAIN[HEVC_MAX_VIDEO_BITRATE_MAIN_SIZE] =
{
  { 128u, 10u },
  { 1500u, 20u },
  { 3000u, 21u },
  { 6000u, 30u },
  { 10000u, 31u },
  { 12000u, 40u },
  { 20000u, 41u },
  { 25000u, 50u },
  { 40000u, 51u },
  { 60000u, 52u },
  { 120000u, 61u },
  { 240000u, 62u }
};

static uint8_t AL_sGetHEVCMaxLevelDPBSize(uint32_t uMaxLumaPs, uint32_t uPicSizeInSamplesY)
{
  // Values computed from HEVC Annex A - with maxDpbPicBuf = 6
  if(uPicSizeInSamplesY <= (uMaxLumaPs >> 2))
    return 16;
  else if(uPicSizeInSamplesY <= (uMaxLumaPs >> 1))
    return 12;
  else if(uPicSizeInSamplesY <= ((3 * uMaxLumaPs) >> 2))
    return 8;

  return 6;
}

static uint8_t AL_sGetHEVCLevelFromDPBSize(uint32_t uPicSizeInSamplesY, uint8_t uRequiredDPBSize)
{
  for(int i = 0; i < HEVC_MAX_LUMA_SAMPLES_SIZE; i++)
  {
    uint8_t uHEVCMaxDPBSize = AL_sGetHEVCMaxLevelDPBSize(HEVC_MAX_LUMA_SAMPLES[i].uLimit, uPicSizeInSamplesY);

    if(uRequiredDPBSize <= uHEVCMaxDPBSize)
      return HEVC_MAX_LUMA_SAMPLES[i].uLevel;
  }

  return 255;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the HEVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelHEVC(AL_TEncChanParam const* pChParam)
{
  uint32_t uMaxSample = pChParam->uWidth * pChParam->uHeight;

  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  int iHbrFactor = AL_sSettings_GetHbrFactor(pChParam->eProfile);
  int iBrVclFactor = iCpbVclFactor * iHbrFactor;
  uint32_t uBitRate = (pChParam->tRCParam.uMaxBitRate + (iBrVclFactor - 1)) / iBrVclFactor;
  uint8_t uRequiredDPBSize = AL_DPBConstraint_GetMaxDPBSize(pChParam);

  uint8_t uLevel = AL_sSettings_GetRequiredLevel(uMaxSample, HEVC_MAX_LUMA_SAMPLES, HEVC_MAX_LUMA_SAMPLES_SIZE);

  uMaxSample *= pChParam->tRCParam.uFrameRate;
  uLevel = Max(AL_sSettings_GetRequiredLevel(uMaxSample, HEVC_MAX_MB_RATE, HEVC_MAX_MB_RATE_SIZE), uLevel);

  if(pChParam->uTier)
    uLevel = Max(AL_sSettings_GetRequiredLevel(uBitRate, HEVC_MAX_VIDEO_BITRATE_HIGH, HEVC_MAX_VIDEO_BITRATE_HIGH_SIZE), uLevel);
  else
    uLevel = Max(AL_sSettings_GetRequiredLevel(uBitRate, HEVC_MAX_VIDEO_BITRATE_MAIN, HEVC_MAX_VIDEO_BITRATE_MAIN_SIZE), uLevel);

  uLevel = Max(AL_sGetHEVCLevelFromDPBSize(uMaxSample, uRequiredDPBSize), uLevel);

  return uLevel;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the HEVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevel(AL_TEncChanParam const* pChParam)
{
  if(AL_IS_HEVC(pChParam->eProfile))
    return AL_sSettings_GetMinLevelHEVC(pChParam);
  else if(AL_IS_AVC(pChParam->eProfile))
    return AL_sSettings_GetMinLevelAVC(pChParam);
  return -1;
}

/***************************************************************************/
static void AL_sSettings_SetDefaultAVCParam(AL_TEncSettings* pSettings)
{
  pSettings->tChParam[0].uMaxCuSize = 4; // 16x16

  if(pSettings->eScalingList == AL_SCL_MAX_ENUM)
    pSettings->eScalingList = AL_SCL_FLAT;
}

/***************************************************************************/
static void AL_sSettings_SetDefaultHEVCParam(AL_TEncSettings* pSettings)
{
  if(pSettings->eScalingList == AL_SCL_MAX_ENUM)
    pSettings->eScalingList = AL_SCL_DEFAULT;

  if(pSettings->tChParam[0].uCabacInitIdc > 1)
    pSettings->tChParam[0].uCabacInitIdc = 1;
}

/***************************************************************************/

/***************************************************************************/
void AL_Settings_SetDefaultRCParam(AL_TRCParam* pRCParam)
{
  pRCParam->eRCMode = AL_RC_CONST_QP;
  pRCParam->uTargetBitRate = 4000000;
  pRCParam->uMaxBitRate = 4000000;
  pRCParam->iInitialQP = -1;
  pRCParam->iMinQP = 0;
  pRCParam->iMaxQP = 51;
  pRCParam->uFrameRate = 30;
  pRCParam->uClkRatio = 1000;
  pRCParam->uInitialRemDelay = 135000; // 1.5 s
  pRCParam->uCPBSize = 270000; // 3.0 s
  pRCParam->uIPDelta = -1;
  pRCParam->uPBDelta = -1;
  pRCParam->uMaxPelVal = 255;
  pRCParam->uMaxPSNR = 4200;
  pRCParam->eOptions = AL_RC_OPT_SCN_CHG_RES;

  pRCParam->bUseGoldenRef = false;
  pRCParam->uGoldenRefFrequency = 10;
  pRCParam->uPGoldenDelta = 2;

  pRCParam->uMaxPictureSize = 0;
}

/***************************************************************************/
void AL_Settings_SetDefaults(AL_TEncSettings* pSettings)
{
  assert(pSettings);
  Rtos_Memset(pSettings, 0, sizeof(*pSettings));

  AL_TEncChanParam* pChan = &pSettings->tChParam[0];
  pChan->uWidth = 0;
  pChan->uHeight = 0;

  pChan->eProfile = AL_PROFILE_HEVC_MAIN;
  pChan->uLevel = 51;
  pChan->uTier = 0; // MAIN_TIER
  pChan->eEncTools = AL_OPT_LF | AL_OPT_LF_X_SLICE | AL_OPT_LF_X_TILE;
  pChan->eEncOptions |= AL_OPT_RDO_COST_MODE;

  pChan->ePicFormat = AL_420_8BITS;
  pChan->uSrcBitDepth = 8;

  AL_TGopParam* pGop = &pChan->tGopParam;
  pGop->eMode = AL_GOP_MODE_DEFAULT;
  pGop->uFreqIDR = 0x7FFFFFFF;
  pGop->uGopLength = 30;
  Rtos_Memset(pGop->tempDQP, 0, sizeof(pGop->tempDQP));

  pGop->eGdrMode = AL_GDR_OFF;

  AL_Settings_SetDefaultRCParam(&pChan->tRCParam);

  pChan->iTcOffset = -1;
  pChan->iBetaOffset = -1;

  pChan->uNumCore = NUMCORE_AUTO;
  pChan->uNumSlices = 1;

  pSettings->uEnableSEI = SEI_NONE;
  pSettings->bEnableAUD = true;
  pSettings->bEnableFillerData = true;
  pSettings->eAspectRatio = AL_ASPECT_RATIO_AUTO;
  pSettings->eColourDescription = COLOUR_DESC_BT_709;

  pSettings->eQpCtrlMode = UNIFORM_QP;// ADAPTIVE_AUTO_QP;
  pChan->eLdaCtrlMode = AUTO_LDA;
  assert(sizeof(pChan->LdaFactors) == sizeof(LAMBDA_FACTORS));
  Rtos_Memcpy(pChan->LdaFactors, LAMBDA_FACTORS, sizeof(LAMBDA_FACTORS));

  pSettings->eScalingList = AL_SCL_MAX_ENUM;

  pSettings->bForceLoad = true;
  pChan->pMeRange[SLICE_P][0] = -1; // Horz
  pChan->pMeRange[SLICE_P][1] = -1; // Vert
  pChan->pMeRange[SLICE_B][0] = -1; // Horz
  pChan->pMeRange[SLICE_B][1] = -1; // Vert
  pChan->uMaxCuSize = 5; // 32x32
  pChan->uMinCuSize = 3; // 8x8
  pChan->uMaxTuSize = 5; // 32x32
  pChan->uMinTuSize = 2; // 4x4
  pChan->uMaxTransfoDepthIntra = 1;
  pChan->uMaxTransfoDepthInter = 1;

  pSettings->NumLayer = 1;
  pSettings->NumView = 1;
  pChan->eEntropyMode = AL_MODE_CABAC;
  pChan->eWPMode = AL_WP_DEFAULT;

  pChan->eSrcMode = AL_SRC_NVX;




  pSettings->LookAhead = 0;
  pSettings->TwoPass = 0;
  pSettings->bEnableFirstPassSceneChangeDetection = false;


  pChan->eVideoMode = AL_VM_PROGRESSIVE;
}

/***************************************************************************/
#define MSG(msg) { if(pOut) fprintf(pOut, msg "\r\n"); }

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


  if(AL_IS_AVC(pSettings->tChParam[0].eProfile))
    AL_sSettings_SetDefaultAVCParam(pSettings);


  if(AL_IS_HEVC(pSettings->tChParam[0].eProfile))
    AL_sSettings_SetDefaultHEVCParam(pSettings);
}


/***************************************************************************/
int AL_Settings_CheckValidity(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, FILE* pOut)
{
  int err = 0;

  if(!AL_sSettings_CheckProfile(pChParam->eProfile))
  {
    ++err;
    MSG("Invalid parameter : Profile");
  }

  if((pChParam->eProfile == AL_PROFILE_HEVC_MAIN10) && (AL_GET_BITDEPTH(pChParam->ePicFormat) > 8) && (HW_IP_BIT_DEPTH < 10))
  {
    ++err;
    MSG("The hardware IP doesn't support 10-bit encoding");
  }

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_4_4)
  {
    ++err;
    MSG("The specified ChromaMode is not supported by the IP");
  }

  if(!AL_sSettings_CheckLevel(pChParam->eProfile, pChParam->uLevel))
  {
    ++err;
    MSG("Invalid parameter : Level");
  }

  int iMaxQP = 51;

  if(pChParam->tRCParam.iInitialQP > iMaxQP)
  {
    ++err;
    MSG("Invalid parameter : SliceQP");
  }

  if(-12 > pChParam->iCrPicQpOffset || pChParam->iCrPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter : CrQpOffset");
  }

  if(-12 > pChParam->iCbPicQpOffset || pChParam->iCbPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter : CbQpOffset");
  }

  if(pChParam->tRCParam.uTargetBitRate < 10)
  {
    if((pChParam->tRCParam.eRCMode == AL_RC_CBR) || (pChParam->tRCParam.eRCMode == AL_RC_VBR))
    {
      ++err;
      MSG("Invalid parameter : BitRate");
    }
  }

  if(pChParam->tRCParam.eRCMode == AL_RC_LOW_LATENCY)
  {
    if(pChParam->tGopParam.uNumB > 0)
    {
      ++err;
      MSG("B Picture not allowed in LOW_LATENCY Mode");
    }
  }

  if(pChParam->bSubframeLatency && (pChParam->tGopParam.uNumB > 0))
  {
    ++err;
    MSG("B Picture not allowed in subframe latency");
  }

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_DEFAULT)
  {
    if(pChParam->tGopParam.uGopLength > 1000)
    {
      ++err;
      MSG("Invalid parameter : Gop.Length");
    }

    if(pChParam->tGopParam.uNumB > 4)
    {
      ++err;
      MSG("Invalid parameter : Gop.NumB");
    }
  }

  int iRound = AL_IS_HEVC(pChParam->eProfile) ? 32 : 16;
  int iMaxSlices = (pChParam->uHeight + (iRound / 2)) / iRound;

  if((pChParam->uNumSlices < 1) || (pChParam->uNumSlices > iMaxSlices) || (pChParam->uNumSlices > AL_MAX_ENC_SLICE) || ((pChParam->bSubframeLatency) && (pChParam->uNumSlices > AL_MAX_SLICES_SUBFRAME)))
  {
    ++err;
    MSG("Invalid parameter : NumSlices");
  }


  if(pChParam->eEncOptions & AL_OPT_FORCE_MV_CLIP)
  {
    if(pSettings->uClipHrzRange < 64 || pSettings->uClipHrzRange > pChParam->uWidth)
    {
      ++err;
      MSG("Horizontal clipping range must be between 64 and Picture width! ");
    }

    if(pSettings->uClipVrtRange < 8 || pSettings->uClipVrtRange > pChParam->uHeight)
    {
      ++err;
      MSG("Vertical clipping range must be between 8 and Picture Height! ");
    }
  }


  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);

  if((pChParam->uWidth % 2 != 0) && ((eChromaMode == CHROMA_4_2_0) || (eChromaMode == CHROMA_4_2_2)))
  {
    ++err;
    MSG("Width shall be multiple of 2 on 420 or 422 chroma mode!");
  }

  if((pChParam->uHeight % 2 != 0) && (eChromaMode == CHROMA_4_2_0))
  {
    ++err;
    MSG("Height shall be multiple of 2 on 420 chroma mode!");
  }

  int iNumB = pChParam->tGopParam.uNumB;

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && iNumB != 3 && iNumB != 5 && iNumB != 7)
  {
    ++err;
    MSG("!! PYRAMIDAL GOP pattern only allows 3, 5, 7 B Frames !!");
  }

  if(AL_IS_HEVC(pChParam->eProfile))
  {
    int iOffset = pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter : CrQpOffset");
    }

    iOffset = pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter : CbQpOffset");
    }


    if(pChParam->uMaxCuSize != 5)
    {
      ++err;
      MSG("!! MaxCUSize Not allowed !!");
    }
  }
  else if(AL_IS_AVC(pChParam->eProfile))
  {
    if(pChParam->tRCParam.iInitialQP >= 0)
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrSliceQpOffset;

      if(pSettings->eQpCtrlMode == UNIFORM_QP && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter : SliceQP, CrQpOffset");
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbSliceQpOffset;

      if(pSettings->eQpCtrlMode == UNIFORM_QP && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter : SliceQP, CbQpOffset");
      }
    }

    if((pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL) && (AL_GET_PROFILE_IDC(pChParam->eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE)))
    {
      ++err;
      MSG("!! PYRAMIDAL GOP pattern doesn't allows baseline profile !!");
    }
  }

  if((pSettings->eQpCtrlMode & MASK_AUTO_QP) && (pSettings->eQpCtrlMode & MASK_QP_TABLE))
  {
    ++err;
    MSG("!! QP control mode not allowed !!");
  }

  if(pChParam->eVideoMode >= AL_VM_MAX_ENUM)
  {
    ++err;
    MSG("!! Invalid parameter : VideoMode");
  }


  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE && !AL_IS_HEVC(pChParam->eProfile))
  {
    ++err;
    MSG("!! Interlaced Video mode is not supported in this profile !!");
  }


  if(pSettings->LookAhead < 0)
  {
    ++err;
    MSG("!! Invalid parameter : LookAheadMode should be 0 or above !!");
  }

  if(pSettings->TwoPass < 0 || pSettings->TwoPass > 2)
  {
    ++err;
    MSG("!! Invalid parameter : TwoPass should be 0, 1 or 2 !!");
  }

  if(pSettings->TwoPass != 0 && pSettings->LookAhead != 0)
  {
    ++err;
    MSG("!! Shouldn't have TwoPass and LookAhead at the same time !!");
  }

  if((pSettings->TwoPass != 0 || pSettings->LookAhead != 0) && pChParam->bSubframeLatency)
  {
    ++err;
    MSG("!! Shouldn't have SliceLat and TwoPass/LookAhead at the same time !!");
  }

  if((pSettings->TwoPass != 0 || pSettings->LookAhead != 0) && pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL)
  {
    ++err;
    MSG("!! Shouldn't have Pyramidal GOP and TwoPass/LookAhead at the same time !!");
  }

  if(pSettings->LookAhead == 0 && pSettings->bEnableFirstPassSceneChangeDetection)
  {
    ++err;
    MSG("!! Shouldn't have FirstPassSceneChangeDetection enabled without LookAhead !!");
  }




  return err;
}

/******************************************************************************/
static uint32_t GetHevcMaxTileRow(uint8_t uLevel)
{
  switch(uLevel)
  {
  case 10:
  case 20:
  case 21:
    return 1;
  case 30:
    return 2;
  case 31:
    return 3;
  case 40:
  case 41:
    return 5;
  case 50:
  case 51:
  case 52:
    return 11;
  case 60:
  case 61:
  case 62:
    return 22;
  default:
    printf("level:%d\n", uLevel);
    assert(0);
    return 1;
  }
}

/***************************************************************************/
bool checkProfileCoherency(int iBitDepth, AL_EChromaMode eChroma, AL_EProfile eProfile)
{
  switch(iBitDepth)
  {
  case 8: break;  // 8 bits is supported by all profiles
  case 10:
  {
    if(!AL_IS_10BIT_PROFILE(eProfile))
      return false;
    break;
  }
  default: return false;
  }
  switch(eChroma)
  {
  case CHROMA_4_0_0:
  {
    if(!AL_IS_MONO_PROFILE(eProfile))
      return false;
    break;
  }
  case CHROMA_4_2_0:
  {
    if(!AL_IS_420_PROFILE(eProfile))
      return false;
    break;
  }
  case CHROMA_4_2_2:
  {
    if(!AL_IS_422_PROFILE(eProfile))
      return false;
    break;
  }
  default: return false;
  }

  return true;
}

/***************************************************************************/
AL_EProfile getHevcMinimumProfile(int iBitDepth, AL_EChromaMode eChroma)
{
  switch(iBitDepth)
  {
  case 8:
  {
    switch(eChroma)
    {
    case CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO;
    case CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN;
    case CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422;
    default: assert(0);
    }
  }
  case 10:
    switch(eChroma)
    {
    case CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO10;
    case CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN10;
    case CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422_10;
    default: assert(0);
    }

  default:
    assert(0);
  }

  return AL_PROFILE_HEVC;
}

/***************************************************************************/
AL_EProfile getAvcMinimumProfile(int iBitDepth, AL_EChromaMode eChroma)
{
  switch(iBitDepth)
  {
  case 8:
  {
    switch(eChroma)
    {
    case CHROMA_4_0_0: return AL_PROFILE_AVC_HIGH;
    case CHROMA_4_2_0: return AL_PROFILE_AVC_C_BASELINE;
    case CHROMA_4_2_2: return AL_PROFILE_AVC_HIGH_422;
    default: assert(0);
    }
  }
  case 10:
  {
    switch(eChroma)
    {
    case CHROMA_4_0_0: return AL_PROFILE_AVC_HIGH10;
    case CHROMA_4_2_0: return AL_PROFILE_AVC_HIGH10;
    case CHROMA_4_2_2: return AL_PROFILE_AVC_HIGH_422;
    default: assert(0);
    }
  }
  default:
    assert(0);
  }

  assert(0);

  return AL_PROFILE_AVC;
}


/***************************************************************************/
int AL_Settings_CheckCoherency(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, TFourCC tFourCC, FILE* pOut)
{
  assert(pSettings);

  int numIncoherency = 0;

  int16_t iMaxPRange = AL_IS_AVC(pChParam->eProfile) ? AVC_MAX_HORIZONTAL_RANGE_P : HEVC_MAX_HORIZONTAL_RANGE_P;
  int16_t iMaxBRange = AL_IS_AVC(pChParam->eProfile) ? AVC_MAX_HORIZONTAL_RANGE_B : HEVC_MAX_HORIZONTAL_RANGE_B;

  int iBitDepth = AL_GET_BITDEPTH(pChParam->ePicFormat);

  if(AL_GetBitDepth(tFourCC) < iBitDepth)
    ++numIncoherency;

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);

  if(!checkProfileCoherency(iBitDepth, eChromaMode, pSettings->tChParam[0].eProfile))
  {
    MSG("!! Warning : Adapting profile to support bitdepth and chroma mode");
    pSettings->tChParam[0].eProfile = AL_IS_AVC(pChParam->eProfile) ? getAvcMinimumProfile(iBitDepth, eChromaMode) : getHevcMinimumProfile(iBitDepth, eChromaMode);
    ++numIncoherency;
  }


  if((pSettings->eQpCtrlMode & MASK_QP_TABLE) == ROI_QP)
    pSettings->eQpCtrlMode |= RELATIVE_QP;

  if(pSettings->eQpCtrlMode & (MASK_AUTO_QP | MASK_QP_TABLE))
  {
    if((pChParam->uMaxCuSize - pChParam->uCuQPDeltaDepth) < pChParam->uMinCuSize)
    {
      MSG("!! Warning : CuQpDeltaDepth doesn't match MinCUSize !!");
      ++numIncoherency;
      pChParam->uCuQPDeltaDepth = pChParam->uMaxCuSize - pChParam->uMinCuSize;
    }

    if(pChParam->uCuQPDeltaDepth > 2)
    {
      MSG("!! Warning : CuQpDeltaDepth shall be less then or equal to 2 !");
      ++numIncoherency;
      pChParam->uCuQPDeltaDepth = 2;
    }
  }
  else
  {
    pChParam->uCuQPDeltaDepth = 0;
  }

  if(pChParam->tRCParam.eRCMode == AL_RC_CBR)
  {
    pChParam->tRCParam.uMaxBitRate = pChParam->tRCParam.uTargetBitRate;
  }

  if(pChParam->tRCParam.uTargetBitRate > pChParam->tRCParam.uMaxBitRate)
  {
    MSG("!! Warning specified MaxBitRate has to be greater than or equal to [Target]BitRate and will be adjusted!!");
    pChParam->tRCParam.uMaxBitRate = pChParam->tRCParam.uTargetBitRate;
    ++numIncoherency;
  }

  AL_EChromaMode eInputChromaMode = AL_GetChromaMode(tFourCC);

  if(AL_IS_HEVC(pChParam->eProfile) || AL_IS_AVC(pChParam->eProfile))
  {
    if(pChParam->tRCParam.eRCMode == AL_RC_CBR)
    {
      AL_TDimension tDim = { pChParam->uWidth, pChParam->uHeight };
      uint32_t maxNalSizeInByte = GetPcmVclNalSize(tDim, eInputChromaMode, iBitDepth);
      uint64_t maxBitRate = 8LL * maxNalSizeInByte * pChParam->tRCParam.uFrameRate;

      if(pChParam->tRCParam.uTargetBitRate > maxBitRate)
      {
        MSG("!! Warning specified TargetBitRate is too high for this use case and will be adjusted!!");
        pChParam->tRCParam.uTargetBitRate = maxBitRate;
        pChParam->tRCParam.uMaxBitRate = maxBitRate;
        ++numIncoherency;
      }
    }
  }

  AL_sCheckRange(&pChParam->pMeRange[SLICE_P][0], iMaxPRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[SLICE_P][1], iMaxPRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[SLICE_B][0], iMaxBRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[SLICE_B][1], iMaxBRange, pOut);

  if((pChParam->uSliceSize > 0) && (pChParam->eEncTools & AL_OPT_WPP))
  {
    pChParam->eEncTools &= ~AL_OPT_WPP;
    MSG("!! Wavefront Parallel Processing is not allowed with SliceSize; it will be adjusted!!");
    ++numIncoherency;
  }

  if(AL_IS_INTRA_PROFILE(pChParam->eProfile) && pChParam->tGopParam.uGopLength != 0)
  {
    pChParam->tGopParam.uGopLength = 0;
    MSG("!! Gop.Length shall be set to 0 for Intra only profile; it will be adjusted!!");
    ++numIncoherency;

    if(AL_IS_AVC(pChParam->eProfile))
      pChParam->tGopParam.uFreqIDR = 0;
  }

  if(AL_IS_AVC(pChParam->eProfile))
  {
    pChParam->uMaxTuSize = 3;
    pChParam->iCbSliceQpOffset = pChParam->iCrSliceQpOffset = 0;

    if(pChParam->eEncTools & AL_OPT_WPP)
      pChParam->eEncTools &= ~AL_OPT_WPP;

    if(pChParam->uMaxCuSize != 4 || pChParam->uMinCuSize != 3)
    {
      pChParam->uMaxCuSize = 4;
      pChParam->uMinCuSize = 3;
      MSG("!! The Specified MaxCUSize and MinCUSize are not allowed; they will be adjusted!!");
      ++numIncoherency;
    }

    if(AL_GET_PROFILE_IDC(pChParam->eProfile) < AL_GET_PROFILE_IDC(AL_PROFILE_AVC_HIGH))
    {
      if(pChParam->uMaxTuSize != 2)
      {
        // cannot be set by cfg
        pChParam->uMaxTuSize = 2;
      }

      if(pChParam->iCrPicQpOffset != pChParam->iCbPicQpOffset)
      {
        pChParam->iCrPicQpOffset = pChParam->iCbPicQpOffset;
        MSG("!! The Specified CrQpOffset is not allowed; it will be adjusted!!");
        ++numIncoherency;
      }

      if(pSettings->eScalingList != AL_SCL_FLAT)
      {
        pSettings->eScalingList = AL_SCL_FLAT;
        MSG("!! The specified ScalingList is not allowed; it will be adjusted!!");
        ++numIncoherency;
      }

      if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) != CHROMA_4_2_0)
      {
        AL_SET_CHROMA_MODE(pChParam->ePicFormat, CHROMA_4_2_0);
        MSG("!! The specified ChromaMode and Profile are not allowed; they will be adjusted!!");
        ++numIncoherency;
      }
    }

    if(AL_GET_PROFILE_IDC(pChParam->eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE))
    {
      if(pChParam->eEntropyMode == AL_MODE_CABAC)
      {
        pChParam->eEntropyMode = AL_MODE_CAVLC;
        MSG("!! CABAC encoding is not allowed in baseline profile; CAVLC will be used instead !!");
        ++numIncoherency;
      }
    }

    if(pChParam->eProfile == AL_PROFILE_AVC_BASELINE
       || pChParam->eProfile == AL_PROFILE_AVC_C_BASELINE
       || pChParam->eProfile == AL_PROFILE_AVC_C_HIGH)
    {
      if(pChParam->tGopParam.uNumB != 0)
      {
        pChParam->tGopParam.uNumB = 0;
        MSG("!! The specified NumB is not allowed in this profile; it will be adjusted!!");
        ++numIncoherency;
      }
    }

    if(pChParam->uCuQPDeltaDepth > 0)
    {
      MSG("!! In AVC CuQPDeltaDepth is not allowed; it will be adjusted !!");
      pChParam->uCuQPDeltaDepth = 0;
      ++numIncoherency;
    }
  }


  if(pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B && (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH ||
                                                              pChParam->tGopParam.uGopLength < MIN_LOW_DELAY_B_GOP_LENGTH))
  {
    pChParam->tGopParam.uGopLength = (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH) ? MAX_LOW_DELAY_B_GOP_LENGTH : MIN_LOW_DELAY_B_GOP_LENGTH;
    MSG("!! The specified Gop.Length value in low delay B mode is not allowed, value will be adjusted !!");
    ++numIncoherency;
  }

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && ((pChParam->tGopParam.uGopLength - 1) % (pChParam->tGopParam.uNumB + 1)) != 0)
  {
    int iRound = pChParam->tGopParam.uNumB + 1;
    pChParam->tGopParam.uGopLength = (((pChParam->tGopParam.uGopLength + iRound - 1) / iRound) * iRound) + 1;
    MSG("!! The specified Gop.Length value in pyramidal gop mode is not reachable, value will be adjusted !!");
    ++numIncoherency;
  }

  if((pSettings->eQpCtrlMode == ADAPTIVE_AUTO_QP || pSettings->eQpCtrlMode == AUTO_QP) && pChParam->tRCParam.iInitialQP >= 0)
  {
    if(AL_IS_HEVC(pChParam->eProfile))
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCrPicQpOffset = pChParam->iCrSliceQpOffset = 0;
        MSG("!!With this QPControlMode, this CrQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCbPicQpOffset = pChParam->iCbSliceQpOffset = 0;
        MSG("!!With this QPControlMode, this CbQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
    }
    else if(AL_IS_AVC(pChParam->eProfile))
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCrPicQpOffset = 0;
        MSG("!!With this QPControlMode, this CrQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCbPicQpOffset = 0;
        MSG("!!With this QPControlMode, this CbQPOffset cannot be set for this SliceQP!!");
        ++numIncoherency;
      }
    }
  }

  if(pChParam->tGopParam.uFreqLT > 0 && !pChParam->tGopParam.bEnableLT)
  {
    pChParam->tGopParam.bEnableLT = true;
    MSG("!! Enabling long term references as a long term frequency is provided !!");
    ++numIncoherency;
  }

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL && pChParam->tGopParam.bEnableLT)
  {
    pChParam->tGopParam.bEnableLT = false;
    pChParam->tGopParam.uFreqLT = 0;
    MSG("!! Long Term reference are not allowed with PYRAMIDAL GOP, it will be adjusted !!");
    ++numIncoherency;
  }

  if(AL_IS_HEVC(pChParam->eProfile) || AL_IS_AVC(pChParam->eProfile))
  {
    uint8_t uMinLevel = AL_sSettings_GetMinLevel(pChParam);

    if(uMinLevel == 255)
    {
      MSG("!! The specified configuration requires a level too high for the IP encoder!!");
      return -1;
    }

    if(pChParam->uLevel < uMinLevel)
    {
      if(!AL_sSettings_CheckLevel(pChParam->eProfile, uMinLevel))
      {
        MSG("!! The specified configuration requires a level too high for the IP encoder!!");
        ++numIncoherency;
      }
      else
      {
        MSG("!! The specified Level is too low and will be adjusted !!");
        ++numIncoherency;
      }
      pChParam->uLevel = uMinLevel;
    }

    if(AL_IS_HEVC(pChParam->eProfile) && pChParam->uLevel < 40 && pChParam->uTier)
    {
      pChParam->uTier = 0;
    }

    if(pChParam->tRCParam.eRCMode != AL_RC_CONST_QP)
    {
      uint64_t uCPBSize = ((AL_64U)pChParam->tRCParam.uCPBSize * pChParam->tRCParam.uMaxBitRate) / 90000LL;
      uint32_t uMaxCPBSize = AL_sSettings_GetMaxCPBSize(pChParam);

      if(uCPBSize > uMaxCPBSize)
      {
        MSG("!! Warning specified CPBSize is higher than the Max CPBSize allowed for this level and will be adjusted !!");
        ++numIncoherency;
        pChParam->tRCParam.uCPBSize = uMaxCPBSize * 90000LL / pChParam->tRCParam.uMaxBitRate;
      }
    }
  }

  if(pChParam->tRCParam.uCPBSize < pChParam->tRCParam.uInitialRemDelay)
  {
    MSG("!! Warning specified InitialDelay is bigger than CPBSize and will be adjusted !!");
    ++numIncoherency;
    pChParam->tRCParam.uInitialRemDelay = pChParam->tRCParam.uCPBSize;
  }

  if(AL_IS_HEVC(pChParam->eProfile))
  {
    int iNumCore = pChParam->uNumCore;
    AL_CoreConstraint constraint;
    AL_CoreConstraint_Init(&constraint, ENCODER_CORE_FREQUENCY, ENCODER_CORE_FREQUENCY_MARGIN, ENCODER_CYCLES_FOR_BLK_32X32, 0, AL_ENC_CORE_MAX_WIDTH);

    if(iNumCore == NUMCORE_AUTO)
      iNumCore = AL_CoreConstraint_GetExpectedNumberOfCores(&constraint, pChParam->uWidth, pChParam->uHeight, pChParam->tRCParam.uFrameRate * 1000, pChParam->tRCParam.uClkRatio);

    if(iNumCore > 1 && pChParam->uNumSlices > GetHevcMaxTileRow(pChParam->uLevel))
    {
      pChParam->uNumSlices = Clip3(pChParam->uNumSlices, 1, GetHevcMaxTileRow(pChParam->uLevel));
      MSG("!!With this Configuration, this NumSlices cannot be set!!");
      ++numIncoherency;
    }
  }


  if(pChParam->tGopParam.eGdrMode == AL_GDR_VERTICAL)
    pChParam->eEncTools |= AL_OPT_CONST_INTRA_PRED;

  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE)
  {
    assert(AL_IS_HEVC(pChParam->eProfile));
    pSettings->uEnableSEI |= SEI_PT;
  }

  if(pSettings->bEnableFirstPassSceneChangeDetection)
  {
    uint32_t uRatio = pChParam->uWidth * 1000 / pChParam->uHeight;
    uint32_t uVal = pChParam->uWidth * pChParam->uHeight / 50;

    if(uRatio > 2500 || uRatio < 400 || uVal < 18000 || uVal > 170000)
    {
      MSG("!! SCDFirstPass mode is not supported with the current resolution, it'll be disabled !!");
      pSettings->bEnableFirstPassSceneChangeDetection = false;
      ++numIncoherency;
    }
  }



  if(pChParam->tRCParam.eRCMode == AL_RC_CONST_QP && pChParam->tRCParam.iInitialQP < 0)
    pChParam->tRCParam.iInitialQP = 30;
  return numIncoherency;
}

/*@}*/

