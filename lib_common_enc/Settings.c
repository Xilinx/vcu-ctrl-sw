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
#include "lib_common_enc/ParamConstraints.h"
#include "lib_common_enc/DPBConstraints.h"
#include "lib_common/SEI.h"
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/HevcLevelsLimit.h"

static int const HEVC_MAX_CU_SIZE = 5; // 32x32
static int const AVC_MAX_CU_SIZE = 4; // 16x16
static int const MIN_CU_SIZE = AL_MIN_SUPPORTED_LCU_SIZE; // 8x8

static int LAMBDA_FACTORS[] = { 51, 90, 151, 151, 151, 151 }; // I, P, B(temporal id low to high)
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
  case AL_PROFILE_XAVC_HIGH10_INTRA_CBG:
  case AL_PROFILE_XAVC_HIGH10_INTRA_VBR:
  case AL_PROFILE_XAVC_HIGH_422_INTRA_CBG:
  case AL_PROFILE_XAVC_HIGH_422_INTRA_VBR:
  case AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF:
    return true;
  default:
    return false;
  }
}

/***************************************************************************/
static bool AL_sSettings_CheckLevel(AL_EProfile eProfile, uint8_t uLevel)
{
  if(AL_IS_HEVC(eProfile))
    return AL_HEVC_CheckLevel(uLevel);
  else if(AL_IS_AVC(eProfile))
    return AL_AVC_CheckLevel(uLevel);
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

  if(AL_GET_PROFILE_CODEC_AND_IDC(eProfile) != AL_PROFILE_HEVC_RExt)
    return 1;

  return !AL_IS_LOW_BITRATE_PROFILE(eProfile) ? 2 : 1;
}

/****************************************************************************/
static uint32_t AL_sSettings_GetMaxCPBSize(AL_TEncChanParam const* pChParam)
{
  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  uint32_t uCpbSize = 0;

  if(AL_IS_AVC(pChParam->eProfile))
    uCpbSize = AL_AVC_GetMaxCPBSize(pChParam->uLevel);
  else if(AL_IS_HEVC(pChParam->eProfile))
    uCpbSize = AL_HEVC_GetMaxCPBSize(pChParam->uLevel, pChParam->uTier);

  return uCpbSize * iCpbVclFactor;
}

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

  uLevel = AL_AVC_GetLevelFromFrameSize(iMaxMB);

  iMaxMB *= pChParam->tRCParam.uFrameRate;
  uLevel = Max(AL_AVC_GetLevelFromMBRate(iMaxMB), uLevel);
  uLevel = Max(AL_AVC_GetLevelFromBitrate(iMaxBR), uLevel);
  uLevel = Max(AL_AVC_GetLevelFromDPBSize(iDPBSize), uLevel);

  return uLevel;
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

  uint8_t uLevel = AL_HEVC_GetLevelFromFrameSize(uMaxSample);

  uMaxSample *= pChParam->tRCParam.uFrameRate;
  uLevel = Max(AL_HEVC_GetLevelFromPixRate(uMaxSample), uLevel);
  uLevel = Max(AL_HEVC_GetLevelFromBitrate(uBitRate, pChParam->uTier), uLevel);
  uLevel = Max(AL_HEVC_GetLevelFromDPBSize(uRequiredDPBSize, uMaxSample), uLevel);

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


/****************************************************************************/
static AL_TMtx4x4 const XAVC_1280x720_DefaultScalingLists4x4[2] =
{
  {
    16, 21, 27, 34,
    21, 27, 34, 41,
    27, 34, 41, 46,
    34, 41, 46, 54,
  },
  { 0 },
};

/****************************************************************************/
static AL_TMtx4x4 const XAVC_1440x1080_DefaultScalingLists4x4[2] =
{
  {
    16, 22, 28, 40,
    22, 28, 40, 44,
    28, 40, 44, 48,
    40, 44, 48, 60,
  },
  { 0 },
};

/****************************************************************************/
static AL_TMtx4x4 const XAVC_1920x1080_DefaultScalingLists4x4[2] =
{
  {
    16, 20, 26, 32,
    20, 26, 32, 38,
    26, 32, 38, 44,
    32, 38, 44, 50,
  },
  { 0 },
};

/****************************************************************************/
static AL_TMtx8x8 const XAVC_1280x720_DefaultScalingLists8x8[2] =
{
  {
    16, 18, 19, 21, 22, 24, 26, 32,
    18, 19, 19, 21, 22, 24, 26, 32,
    19, 19, 21, 22, 22, 24, 26, 32,
    21, 21, 22, 22, 23, 24, 26, 34,
    22, 22, 22, 23, 24, 25, 26, 34,
    24, 24, 24, 24, 25, 26, 34, 36,
    26, 26, 26, 26, 26, 34, 36, 38,
    32, 32, 32, 34, 34, 36, 38, 42,
  },
  { 0 },
};

/****************************************************************************/
static AL_TMtx8x8 const XAVC_1440x1080_DefaultScalingLists8x8[2] =
{
  {
    16, 18, 19, 21, 24, 27, 30, 33,
    18, 19, 21, 24, 27, 30, 33, 78,
    19, 21, 24, 27, 30, 33, 78, 81,
    21, 24, 27, 30, 33, 78, 81, 84,
    24, 27, 30, 33, 78, 81, 84, 87,
    27, 30, 33, 78, 81, 84, 87, 90,
    30, 33, 78, 81, 84, 87, 90, 93,
    33, 78, 81, 84, 87, 90, 93, 96,
  },
  { 0 },
};

/****************************************************************************/
static AL_TMtx8x8 const XAVC_1920x1080_DefaultScalingLists8x8[2] =
{
  {
    16, 18, 19, 20, 22, 23, 24, 26,
    18, 19, 20, 22, 23, 24, 26, 32,
    19, 20, 22, 23, 24, 26, 32, 36,
    20, 22, 23, 24, 26, 32, 36, 42,
    22, 23, 24, 26, 32, 36, 42, 59,
    23, 24, 26, 32, 36, 42, 59, 63,
    24, 26, 32, 36, 42, 59, 63, 68,
    26, 32, 36, 42, 59, 63, 68, 72,
  },
  { 0 },
};

/***************************************************************************/
static void XAVC_CheckCoherency(AL_TEncSettings* pSettings)
{
  pSettings->bEnableAUD = true;
  pSettings->eEnableFillerData = AL_FILLER_DISABLE;
  pSettings->eAspectRatio = AL_ASPECT_RATIO_1_1;
  AL_TEncChanParam* pChannel = &pSettings->tChParam[0];
  pChannel->eEncTools &= (~AL_OPT_LF_X_TILE);
  pChannel->eEncTools &= (~AL_OPT_LF_X_SLICE);
  pChannel->eEncTools &= (~AL_OPT_LF);

  if(AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    pSettings->iPrefetchLevel2 = 0;

    AL_SET_BITDEPTH(&pChannel->ePicFormat, 10);
    pChannel->uSrcBitDepth = 10;
    pChannel->uNumSlices = 8;

    AL_TGopParam* pGop = &pChannel->tGopParam;
    pGop->uGopLength = 1;
    pGop->uFreqIDR = 1;

    if(AL_IS_XAVC_CBG(pChannel->eProfile))
    {
      pSettings->eScalingList = AL_SCL_CUSTOM;
      pSettings->SclFlag[0][0] = 0;
      pSettings->SclFlag[0][1] = 1;
      Rtos_Memcpy(pSettings->ScalingList[0][1], (pChannel->uWidth == 1280) ? XAVC_1280x720_DefaultScalingLists4x4[0] : (pChannel->uWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists4x4[0] : XAVC_1920x1080_DefaultScalingLists4x4[0], 16);
      pSettings->SclFlag[0][2] = 1;
      Rtos_Memcpy(pSettings->ScalingList[0][2], (pChannel->uWidth == 1280) ? XAVC_1280x720_DefaultScalingLists4x4[0] : (pChannel->uWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists4x4[0] : XAVC_1920x1080_DefaultScalingLists4x4[0], 16);
      pSettings->SclFlag[0][3] = 0;
      pSettings->SclFlag[0][4] = 0;
      pSettings->SclFlag[0][5] = 0;
      pSettings->SclFlag[1][0] = 1;
      Rtos_Memcpy(pSettings->ScalingList[1][0], (pChannel->uWidth == 1280) ? XAVC_1280x720_DefaultScalingLists8x8[0] : (pChannel->uWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists8x8[0] : XAVC_1920x1080_DefaultScalingLists8x8[0], 64);
      pSettings->SclFlag[1][1] = 0;
      pChannel->iCbPicQpOffset = (pChannel->uWidth == 1920) ? 3 : 4;
      pChannel->iCrPicQpOffset = (pChannel->uWidth == 1920) ? 3 : 4;
    }
    return;
  }

  int iSliceFactors = (pChannel->uLevel <= 42) ? 1 : 2;

  if((pChannel->uNumSlices != (4 * iSliceFactors)) && (pChannel->uNumSlices != (6 * iSliceFactors)))
    pChannel->uNumSlices = 4 * iSliceFactors;

  if(AL_IS_422_PROFILE(pChannel->eProfile))
  {
    AL_SET_BITDEPTH(&pChannel->ePicFormat, 10);
    pChannel->uSrcBitDepth = 10;
  }

  if(AL_IS_XAVC_MP4(pChannel->eProfile))
  {
    int const iMaxTimeBetween2ConsecutiveIDRInSeconds = 5;
    AL_TRCParam* pRateControl = &pChannel->tRCParam;
    int iFramesPerMaxTime = iMaxTimeBetween2ConsecutiveIDRInSeconds * ((pRateControl->uFrameRate * pRateControl->uClkRatio) / 1001);
    AL_TGopParam* pGop = &pChannel->tGopParam;
    int iGopPerMaxTime = iFramesPerMaxTime / pGop->uGopLength;
    pGop->uFreqIDR = Min(iGopPerMaxTime * pGop->uGopLength, pGop->uFreqIDR);
  }
}

/***************************************************************************/
static void AL_sSettings_SetDefaultXAVCParam(AL_TEncSettings* pSettings)
{
  XAVC_CheckCoherency(pSettings);
}


/***************************************************************************/
static void AL_sSettings_SetDefaultAVCParam(AL_TEncSettings* pSettings)
{
  pSettings->tChParam[0].uMaxCuSize = AVC_MAX_CU_SIZE;

  if(pSettings->eScalingList == AL_SCL_MAX_ENUM)
    pSettings->eScalingList = AL_SCL_FLAT;

  if(AL_IS_XAVC(pSettings->tChParam[0].eProfile))
    AL_sSettings_SetDefaultXAVCParam(pSettings);
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

  pRCParam->pMaxPictureSize[AL_SLICE_I] = 0;
  pRCParam->pMaxPictureSize[AL_SLICE_P] = 0;
  pRCParam->pMaxPictureSize[AL_SLICE_B] = 0;
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
  pGop->uFreqIDR = INT32_MAX;
  pGop->uGopLength = 30;
  Rtos_Memset(pGop->tempDQP, 0, sizeof(pGop->tempDQP));

  pGop->eGdrMode = AL_GDR_OFF;

  AL_Settings_SetDefaultRCParam(&pChan->tRCParam);

  pChan->iTcOffset = -1;
  pChan->iBetaOffset = -1;

  pChan->uNumCore = NUMCORE_AUTO;
  pChan->uNumSlices = 1;

  pSettings->uEnableSEI = AL_SEI_NONE;
  pSettings->bEnableAUD = true;
  pSettings->eEnableFillerData = AL_FILLER_ENC;
  pSettings->eAspectRatio = AL_ASPECT_RATIO_AUTO;
  pSettings->eColourDescription = AL_COLOUR_DESC_BT_709;
  pSettings->eTransferCharacteristics = AL_TRANSFER_CHARAC_UNSPECIFIED;
  pSettings->eColourMatrixCoeffs = AL_COLOUR_MAT_COEFF_UNSPECIFIED;

  pSettings->eQpCtrlMode = AL_QP_CTRL_NONE;
  pSettings->eQpTableMode = AL_QP_TABLE_NONE;
  pChan->eLdaCtrlMode = AL_AUTO_LDA;
  assert(sizeof(pChan->LdaFactors) == sizeof(LAMBDA_FACTORS));
  Rtos_Memcpy(pChan->LdaFactors, LAMBDA_FACTORS, sizeof(LAMBDA_FACTORS));

  pSettings->eScalingList = AL_SCL_MAX_ENUM;

  pSettings->bForceLoad = true;
  pChan->pMeRange[AL_SLICE_P][0] = -1; // Horz
  pChan->pMeRange[AL_SLICE_P][1] = -1; // Vert
  pChan->pMeRange[AL_SLICE_B][0] = -1; // Horz
  pChan->pMeRange[AL_SLICE_B][1] = -1; // Vert
  pChan->uMaxCuSize = HEVC_MAX_CU_SIZE;
  pChan->uMinCuSize = MIN_CU_SIZE;
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


  pChan->MaxNumMergeCand = 5;
}

/***************************************************************************/
#define MSGF(msg, ...) { if(pOut) fprintf(pOut, msg "\r\n", __VA_ARGS__); }
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
    MSG("Invalid parameter: Profile");
  }

  if((pChParam->eProfile == AL_PROFILE_HEVC_MAIN10) && (AL_GET_BITDEPTH(pChParam->ePicFormat) > 8) && (HW_IP_BIT_DEPTH < 10))
  {
    ++err;
    MSG("The hardware IP doesn't support 10-bit encoding");
  }

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == AL_CHROMA_4_4_4)
  {
    ++err;
    MSG("The specified ChromaMode is not supported by the IP");
  }

  if(!AL_sSettings_CheckLevel(pChParam->eProfile, pChParam->uLevel))
  {
    ++err;
    MSG("Invalid parameter: Level");
  }

  if(AL_IS_INTRA_PROFILE(pChParam->eProfile) && (pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL))
  {
    ++err;
    MSG("Pyramidal and IntraProfile is not supported");
  }

  if(pChParam->uMinCuSize != MIN_CU_SIZE)
  {
    ++err;
    MSG("Invalid parameter: MinCuSize");
  }

  bool bHEVCCuSupported = pChParam->uMaxCuSize == HEVC_MAX_CU_SIZE;

  if(AL_IS_HEVC(pChParam->eProfile) && !bHEVCCuSupported)
  {
    ++err;
    MSG("Invalid parameter: MaxCuSize");
  }


  if(AL_IS_AVC(pChParam->eProfile) && (pChParam->uMaxCuSize != AVC_MAX_CU_SIZE))
  {
    ++err;
    MSG("Invalid parameter: MaxCuSize");
  }




  int const iRound = (1 << pChParam->uMaxCuSize);

  if(pChParam->uNumCore != NUMCORE_AUTO)
  {
    AL_NumCoreDiagnostic diagnostic;

    if(!AL_Constraint_NumCoreIsSane(pChParam->uWidth, pChParam->uNumCore, pChParam->uMaxCuSize, &diagnostic))
    {
      ++err;

      MSGF("Invalid parameter: NumCore. The width should at least be %d CTB per core. With the specified number of core, it is %d CTB per core. (Multi core alignement constraint might be the reason of this error if the CTB are equal)", diagnostic.requiredWidthInCtbPerCore, diagnostic.actualWidthInCtbPerCore);
    }
  }

  int iMaxQP = 51;

  if(pChParam->tRCParam.iInitialQP > iMaxQP)
  {
    ++err;
    MSG("Invalid parameter: SliceQP");
  }

  if(-12 > pChParam->iCrPicQpOffset || pChParam->iCrPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter: CrQpOffset");
  }

  if(-12 > pChParam->iCbPicQpOffset || pChParam->iCbPicQpOffset > 12)
  {
    ++err;
    MSG("Invalid parameter: CbQpOffset");
  }

  if(pChParam->tRCParam.uTargetBitRate < 10)
  {
    if((pChParam->tRCParam.eRCMode == AL_RC_CBR) || (pChParam->tRCParam.eRCMode == AL_RC_VBR))
    {
      ++err;
      MSG("Invalid parameter: BitRate");
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

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_DEFAULT)
  {
    if(pChParam->tGopParam.uGopLength > 1000)
    {
      ++err;
      MSG("Invalid parameter: Gop.Length");
    }

    if(pChParam->tGopParam.uNumB > 4)
    {
      ++err;
      MSG("Invalid parameter: Gop.NumB");
    }
  }

  int iMaxSlices = (pChParam->uHeight + (iRound / 2)) / iRound;

  if((pChParam->uNumSlices < 1) || (pChParam->uNumSlices > iMaxSlices) || (pChParam->uNumSlices > AL_MAX_ENC_SLICE) || ((pChParam->bSubframeLatency) && (pChParam->uNumSlices > AL_MAX_SLICES_SUBFRAME)))
  {
    ++err;
    MSG("Invalid parameter: NumSlices");
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
  ECheckResolutionError eResError = AL_ParamConstraints_CheckResolution(pChParam->eProfile, eChromaMode, pChParam->uWidth, pChParam->uHeight);

  if(eResError == CRERROR_WIDTHCHROMA)
  {
    ++err;
    MSG("Width shall be multiple of 2 on 420 or 422 chroma mode!");
  }
  else if(eResError == CRERROR_HEIGHTCHROMA)
  {
    ++err;
    MSG("Width shall be multiple of 2 on 420 or 422 chroma mode!");
  }

  int iNumB = pChParam->tGopParam.uNumB;

  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && iNumB != 3 && iNumB != 5 && iNumB != 7 && iNumB != 15)
  {
    ++err;
    MSG("!! PYRAMIDAL GOP pattern only allows 3, 5, 7, 15 B Frames !!");
  }

  if(AL_IS_HEVC(pChParam->eProfile))
  {
    int iOffset = pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter: CrQpOffset");
    }

    iOffset = pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;

    if(-12 > iOffset || iOffset > 12)
    {
      ++err;
      MSG("Invalid parameter: CbQpOffset");
    }
  }
  else if(AL_IS_AVC(pChParam->eProfile))
  {
    if(pChParam->tRCParam.iInitialQP >= 0)
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrSliceQpOffset;

      if(((pSettings->eQpCtrlMode == AL_QP_CTRL_NONE) && (pSettings->eQpTableMode == AL_QP_TABLE_NONE)) && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter: SliceQP, CrQpOffset");
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbSliceQpOffset;

      if(((pSettings->eQpCtrlMode == AL_QP_CTRL_NONE) && (pSettings->eQpTableMode == AL_QP_TABLE_NONE)) && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG("Invalid parameter: SliceQP, CbQpOffset");
      }
    }

    if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && (AL_GET_PROFILE_IDC(pChParam->eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE)))
    {
      ++err;
      MSG("!! PYRAMIDAL GOP pattern doesn't allows baseline profile !!");
    }
  }

  if(AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode) && ((pSettings->eQpTableMode != AL_QP_TABLE_RELATIVE) && pSettings->eQpTableMode != AL_QP_TABLE_NONE))
  {
    ++err;
    MSG("!! QP control mode not allowed !!");
  }

  if(pChParam->eVideoMode >= AL_VM_MAX_ENUM)
  {
    ++err;
    MSG("!! Invalid parameter: VideoMode");
  }


  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE && !AL_IS_HEVC(pChParam->eProfile))
  {
    ++err;
    MSG("!! Interlaced Video mode is not supported in this profile !!");
  }


  if(pSettings->LookAhead < 0)
  {
    ++err;
    MSG("!! Invalid parameter: LookAheadMode should be 0 or above !!");
  }

  if(pSettings->TwoPass < 0 || pSettings->TwoPass > 2)
  {
    ++err;
    MSG("!! Invalid parameter: TwoPass should be 0, 1 or 2 !!");
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

  if((pSettings->TwoPass != 0 || pSettings->LookAhead != 0) && (pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL))
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
  case AL_CHROMA_4_0_0:
  {
    if(!AL_IS_MONO_PROFILE(eProfile))
      return false;
    break;
  }
  case AL_CHROMA_4_2_0:
  {
    if(!AL_IS_420_PROFILE(eProfile))
      return false;
    break;
  }
  case AL_CHROMA_4_2_2:
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
    case AL_CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO;
    case AL_CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN;
    case AL_CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422;
    default: assert(0);
    }
  }
  case 10:
    switch(eChroma)
    {
    case AL_CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO10;
    case AL_CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN10;
    case AL_CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422_10;
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
    case AL_CHROMA_4_0_0: return AL_PROFILE_AVC_HIGH;
    case AL_CHROMA_4_2_0: return AL_PROFILE_AVC_C_BASELINE;
    case AL_CHROMA_4_2_2: return AL_PROFILE_AVC_HIGH_422;
    default: assert(0);
    }
  }
  case 10:
  {
    switch(eChroma)
    {
    case AL_CHROMA_4_0_0: return AL_PROFILE_AVC_HIGH10;
    case AL_CHROMA_4_2_0: return AL_PROFILE_AVC_HIGH10;
    case AL_CHROMA_4_2_2: return AL_PROFILE_AVC_HIGH_422;
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

  if(AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode) || AL_IS_QP_TABLE_REQUIRED(pSettings->eQpTableMode))
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

  AL_sCheckRange(&pChParam->pMeRange[AL_SLICE_P][0], iMaxPRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[AL_SLICE_P][1], iMaxPRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[AL_SLICE_B][0], iMaxBRange, pOut);
  AL_sCheckRange(&pChParam->pMeRange[AL_SLICE_B][1], iMaxBRange, pOut);

  if((pChParam->uSliceSize > 0) && (pChParam->eEncTools & AL_OPT_WPP))
  {
    pChParam->eEncTools &= ~AL_OPT_WPP;
    MSG("!! Wavefront Parallel Processing is not allowed with SliceSize; it will be adjusted!!");
    ++numIncoherency;
  }

  if(AL_IS_INTRA_PROFILE(pChParam->eProfile))
  {
    if(pChParam->tGopParam.uGopLength > 1)
    {
      pChParam->tGopParam.uGopLength = 1;
      MSG("!! Gop.Length shall be set to 0 or 1 for Intra only profile; it will be adjusted!!");
      ++numIncoherency;
    }

    if(pChParam->tGopParam.uFreqIDR > 1)
    {
      pChParam->tGopParam.uFreqIDR = 1;
      MSG("!! Gop.uFreqIDR shall be set to 0 or 1 for Intra only profile; it will be adjusted!!");
      ++numIncoherency;
    }

    if(pSettings->iPrefetchLevel2 != 0)
    {
      pSettings->iPrefetchLevel2 = 0;
      MSG("!! iPrefetchLevel2 shall be set to 0 for Intra only profile; it will be adjusted!!");
      ++numIncoherency;
    }
  }

  if(AL_IS_AVC(pChParam->eProfile))
  {
    pChParam->uMaxTuSize = 3;
    pChParam->iCbSliceQpOffset = pChParam->iCrSliceQpOffset = 0;

    if(pChParam->eEncTools & AL_OPT_WPP)
      pChParam->eEncTools &= ~AL_OPT_WPP;

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

      if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) != AL_CHROMA_4_2_0)
      {
        AL_SET_CHROMA_MODE(&pChParam->ePicFormat, AL_CHROMA_4_2_0);
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

    if(pSettings->uEnableSEI & AL_SEI_CLL)
    {
      // Silent disable if SEI_ALL
      if(pSettings->uEnableSEI != AL_SEI_ALL)
      {
        MSG("!! AVC does not contain Content Light Level SEIs; disabling it !!");
        ++numIncoherency;
      }

      pSettings->uEnableSEI &= ~AL_SEI_CLL;
    }
  }


  int const MAX_LOW_DELAY_B_GOP_LENGTH = 4;
  int const MIN_LOW_DELAY_B_GOP_LENGTH = 1;

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B && (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH ||
                                                              pChParam->tGopParam.uGopLength < MIN_LOW_DELAY_B_GOP_LENGTH))
  {
    pChParam->tGopParam.uGopLength = (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH) ? MAX_LOW_DELAY_B_GOP_LENGTH : MIN_LOW_DELAY_B_GOP_LENGTH;
    MSG("!! The specified Gop.Length value in low delay B mode is not allowed, value will be adjusted !!");
    ++numIncoherency;
  }

  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && (pChParam->tGopParam.uGopLength % (pChParam->tGopParam.uNumB + 1)) != 0)
  {
    int iRound = pChParam->tGopParam.uNumB + 1;
    pChParam->tGopParam.uGopLength = ((pChParam->tGopParam.uGopLength + iRound - 1) / iRound) * iRound;
    MSG("!! The specified Gop.Length value in pyramidal gop mode is not reachable, value will be adjusted !!");
    ++numIncoherency;
  }

  if(AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode) && pChParam->tRCParam.iInitialQP >= 0)
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

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL)
  {
    if(pChParam->tGopParam.bEnableLT)
    {
      pChParam->tGopParam.bEnableLT = false;
      pChParam->tGopParam.uFreqLT = 0;
      MSG("!! Long Term reference are not allowed with PYRAMIDAL GOP, it will be adjusted !!");
      ++numIncoherency;
    }

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
    pSettings->uEnableSEI |= AL_SEI_PT;
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



  // Fill zero value with surounding non-zero value if any. Warning : don't change the order of if statements below !!
  if(pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P] == 0)
    pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P] = pChParam->tRCParam.pMaxPictureSize[AL_SLICE_I];

  if(pChParam->tRCParam.pMaxPictureSize[AL_SLICE_B] == 0)
    pChParam->tRCParam.pMaxPictureSize[AL_SLICE_B] = pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P];

  if(pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P] == 0)
    pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P] = pChParam->tRCParam.pMaxPictureSize[AL_SLICE_B];

  if(pChParam->tRCParam.pMaxPictureSize[AL_SLICE_I] == 0)
    pChParam->tRCParam.pMaxPictureSize[AL_SLICE_I] = pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P];


  if(AL_IS_XAVC(pChParam->eProfile))
    XAVC_CheckCoherency(pSettings);

  return numIncoherency;
}

/*@}*/

