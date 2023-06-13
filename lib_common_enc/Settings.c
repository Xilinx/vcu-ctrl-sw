// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup Encoder_Settings
   @{
   \file
 *****************************************************************************/

#include <stdio.h>

#include "lib_rtos/lib_rtos.h"
#include "lib_assert/al_assert.h"
#include "lib_common_enc/Settings.h"
#include "lib_common/ChannelResources.h"
#include "lib_common/Utils.h"
#include "lib_common/StreamBufferPrivate.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/EncSize.h"
#include "lib_common_enc/ParamConstraints.h"
#include "lib_common_enc/DPBConstraints.h"
#include "lib_common_enc/EncChanParamInternal.h"
#include "lib_common/SEI.h"
#include "lib_common/ScalingList.h"
#include "lib_common/HardwareConfig.h"
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/HevcLevelsLimit.h"

static int const HEVC_MAX_CTB_SIZE = 5; // 32x32
static int const HEVC_MIN_CTB_SIZE = 5; // 32x32

static int const AVC_MAX_CU_SIZE = 4; // 16x16
static int const MIN_CU_SIZE = AL_MIN_SUPPORTED_LCU_SIZE; // 8x8

static int LAMBDA_FACTORS[] = { 51, 90, 151, 151, 151, 151 }; // I, P, B(temporal id low to high)
/***************************************************************************/
static bool AL_sSettings_CheckProfile(AL_EProfile eProfile)
{
  switch(eProfile)
  {
  case AL_PROFILE_AVC_BASELINE:
  case AL_PROFILE_AVC_C_BASELINE:
  case AL_PROFILE_AVC_MAIN:
  case AL_PROFILE_AVC_HIGH:
  case AL_PROFILE_AVC_HIGH10:
  case AL_PROFILE_AVC_HIGH_422:
  case AL_PROFILE_AVC_HIGH_444_PRED:
  case AL_PROFILE_AVC_C_HIGH:
  case AL_PROFILE_AVC_PROG_HIGH:
  case AL_PROFILE_AVC_HIGH10_INTRA:
  case AL_PROFILE_AVC_HIGH_422_INTRA:
  case AL_PROFILE_AVC_HIGH_444_INTRA:
  case AL_PROFILE_AVC_CAVLC_444_INTRA:
  case AL_PROFILE_XAVC_HIGH10_INTRA_CBG:
  case AL_PROFILE_XAVC_HIGH10_INTRA_VBR:
  case AL_PROFILE_XAVC_HIGH_422_INTRA_CBG:
  case AL_PROFILE_XAVC_HIGH_422_INTRA_VBR:
  case AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF:
  case AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF:
  case AL_PROFILE_HEVC_MAIN:
  case AL_PROFILE_HEVC_MAIN10:
  case AL_PROFILE_HEVC_MAIN12:
  case AL_PROFILE_HEVC_MAIN_STILL:
  case AL_PROFILE_HEVC_MONO:
  case AL_PROFILE_HEVC_MONO10:
  case AL_PROFILE_HEVC_MAIN_422:
  case AL_PROFILE_HEVC_MAIN_422_10:
  case AL_PROFILE_HEVC_MAIN_422_12:
  case AL_PROFILE_HEVC_MAIN_444:
  case AL_PROFILE_HEVC_MAIN_444_10:
  case AL_PROFILE_HEVC_MAIN_444_12:
  case AL_PROFILE_HEVC_MAIN_INTRA:
  case AL_PROFILE_HEVC_MAIN10_INTRA:
  case AL_PROFILE_HEVC_MAIN_422_INTRA:
  case AL_PROFILE_HEVC_MAIN_422_10_INTRA:
  case AL_PROFILE_HEVC_MAIN_444_INTRA:
  case AL_PROFILE_HEVC_MAIN_444_10_INTRA:
  case AL_PROFILE_HEVC_MAIN_444_STILL:
    return true;
  default:
    return false;
  }
}

/***************************************************************************/
static bool AL_sSettings_CheckLevel(AL_EProfile eProfile, uint8_t uLevel)
{
  (void)uLevel;
  switch(AL_GET_CODEC(eProfile))
  {
  case AL_CODEC_AVC: return AL_AVC_CheckLevel(uLevel);
  case AL_CODEC_HEVC: return AL_HEVC_CheckLevel(uLevel);
  default: return false;
  }
}

/****************************************************************************/
static int AL_sSettings_GetCpbVclFactor(AL_EProfile eProfile)
{
  switch(eProfile)
  {
  case AL_PROFILE_AVC_BASELINE:
  case AL_PROFILE_AVC_MAIN:
  case AL_PROFILE_AVC_EXTENDED: return 1000;
  case AL_PROFILE_AVC_HIGH: return 1250;
  case AL_PROFILE_AVC_HIGH10: return 3000;
  case AL_PROFILE_AVC_HIGH_422:
  case AL_PROFILE_AVC_CAVLC_444_INTRA:
  case AL_PROFILE_AVC_HIGH_444_INTRA:
  case AL_PROFILE_AVC_HIGH_444_PRED: return 4000;
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
  AL_Assert(AL_IS_HEVC(eProfile));

  if(AL_GET_PROFILE_CODEC_AND_IDC(eProfile) != AL_PROFILE_HEVC_RExt)
    return 1;

  return !AL_IS_LOW_BITRATE_PROFILE(eProfile) ? 2 : 1;
}

/****************************************************************************/
static uint32_t AL_sSettings_GetMaxCPBSize(AL_TEncChanParam const* pChParam)
{
  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  uint32_t uCpbSize = 0;
  switch(AL_GET_CODEC(pChParam->eProfile))
  {
  case AL_CODEC_AVC: uCpbSize = AL_AVC_GetMaxCPBSize(pChParam->uLevel);
    break;
  case AL_CODEC_HEVC: uCpbSize = AL_HEVC_GetMaxCPBSize(pChParam->uLevel, pChParam->uTier);
    break;
  default: break;
  }

  return uCpbSize * iCpbVclFactor;
}

/*************************************************************************//*!
   \brief Retrieves the minimum level required by the AVC specification
   according to resolution, profile, framerate and bitrate
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the AVC/H264 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelAVC(AL_TEncChanParam const* pChParam)
{
  uint8_t uLevel;

  int iMaxMB = ((pChParam->uEncWidth + 15) >> 4) * ((pChParam->uEncHeight + 15) >> 4);
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
   according to resolution, profile, framerate, bitrate and tile column number
   \param[in] pSettings pointer to encoder Settings structure
   \return The minimum level needed to conform with the HEVC/H265 specification
*****************************************************************************/
static uint8_t AL_sSettings_GetMinLevelHEVC(AL_TEncChanParam const* pChParam)
{
  uint32_t uMaxSample = pChParam->uEncWidth * pChParam->uEncHeight;

  int iCpbVclFactor = AL_sSettings_GetCpbVclFactor(pChParam->eProfile);
  int iHbrFactor = AL_sSettings_GetHbrFactor(pChParam->eProfile);
  int iBrVclFactor = iCpbVclFactor * iHbrFactor;
  uint32_t uBitRate = (pChParam->tRCParam.uMaxBitRate + (iBrVclFactor - 1)) / iBrVclFactor;
  uint8_t uRequiredDPBSize = AL_DPBConstraint_GetMaxDPBSize(pChParam);
  int iTileCols = pChParam->uNumCore;

  uint8_t uLevel = AL_HEVC_GetLevelFromFrameSize(uMaxSample);

  uMaxSample *= pChParam->tRCParam.uFrameRate;
  uLevel = Max(AL_HEVC_GetLevelFromPixRate(uMaxSample), uLevel);
  uLevel = Max(AL_HEVC_GetLevelFromBitrate(uBitRate, pChParam->uTier), uLevel);
  uLevel = Max(AL_HEVC_GetLevelFromDPBSize(uRequiredDPBSize, uMaxSample), uLevel);
  uLevel = Max(AL_HEVC_GetLevelFromTileCols(iTileCols), uLevel);

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
  switch(AL_GET_CODEC(pChParam->eProfile))
  {
  case AL_CODEC_AVC: return AL_sSettings_GetMinLevelAVC(pChParam);
  case AL_CODEC_HEVC: return AL_sSettings_GetMinLevelHEVC(pChParam);
  default: return -1;
  }
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
  pSettings->uEnableSEI |= AL_SEI_PT;
  pSettings->uEnableSEI |= AL_SEI_BP;
  pSettings->uEnableSEI |= AL_SEI_RP;
  AL_TEncChanParam* pChannel = &pSettings->tChParam[0];
  pChannel->eEncTools &= (~AL_OPT_LF_X_TILE);
  pChannel->eEncTools &= (~AL_OPT_LF_X_SLICE);
  pChannel->eEncTools &= (~AL_OPT_LF);
  pChannel->bUseUniformSliceType = true;

  if(AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    pSettings->iPrefetchLevel2 = 0;

    AL_SET_BITDEPTH(&pChannel->ePicFormat, 10);
    pChannel->uSrcBitDepth = 10;
    pChannel->uNumSlices = 8;

    AL_TRCParam* pRateControl = &pChannel->tRCParam;
    pRateControl->eRCMode = AL_RC_VBR;

    const int pCycles32x32Counts[] = ENCODER_CYCLES_FOR_BLK_32X32;
    AL_CoreConstraint constraint;
    AL_CoreConstraint_Init(&constraint, ENCODER_CORE_FREQUENCY, ENCODER_CORE_FREQUENCY_MARGIN, pCycles32x32Counts, 0, AL_ENC_CORE_MAX_WIDTH, 1);
    int iNumCore = AL_CoreConstraint_GetExpectedNumberOfCores(&constraint, pChannel->uEncWidth, pChannel->uEncHeight, AL_GET_CHROMA_MODE(pChannel->ePicFormat), pRateControl->uFrameRate * 1000, pRateControl->uClkRatio);

    if(iNumCore > 1)
    {
      // AVC GEN1 multicore estimator overestimate bits
      pRateControl->uTargetBitRate *= 0.85;
      pRateControl->uMaxBitRate *= 0.85;
    }

    AL_TGopParam* pGop = &pChannel->tGopParam;
    pGop->uGopLength = 0;
    pGop->uFreqIDR = 0;

    if(AL_IS_XAVC_CBG(pChannel->eProfile))
    {
      pSettings->eScalingList = AL_SCL_CUSTOM;
      pSettings->SclFlag[0][0] = 0;
      pSettings->SclFlag[0][1] = 1;
      Rtos_Memcpy(pSettings->ScalingList[0][1], (pChannel->uEncWidth == 1280) ? XAVC_1280x720_DefaultScalingLists4x4[0] : (pChannel->uEncWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists4x4[0] : XAVC_1920x1080_DefaultScalingLists4x4[0], 16);
      pSettings->SclFlag[0][2] = 1;
      Rtos_Memcpy(pSettings->ScalingList[0][2], (pChannel->uEncWidth == 1280) ? XAVC_1280x720_DefaultScalingLists4x4[0] : (pChannel->uEncWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists4x4[0] : XAVC_1920x1080_DefaultScalingLists4x4[0], 16);
      pSettings->SclFlag[0][3] = 0;
      pSettings->SclFlag[0][4] = 0;
      pSettings->SclFlag[0][5] = 0;
      pSettings->SclFlag[1][0] = 1;
      Rtos_Memcpy(pSettings->ScalingList[1][0], (pChannel->uEncWidth == 1280) ? XAVC_1280x720_DefaultScalingLists8x8[0] : (pChannel->uEncWidth == 1440) ? XAVC_1440x1080_DefaultScalingLists8x8[0] : XAVC_1920x1080_DefaultScalingLists8x8[0], 64);
      pSettings->SclFlag[1][1] = 0;

      pChannel->iCbPicQpOffset = (pChannel->uEncWidth == 1920) ? 3 : 4;
      pChannel->iCrPicQpOffset = (pChannel->uEncWidth == 1920) ? 3 : 4;

      pRateControl->eRCMode = AL_RC_CBR;
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

  int const iMaxTimeBetween2ConsecutiveIDRInSeconds = 5;
  AL_TRCParam* pRateControl = &pChannel->tRCParam;
  int iFramesPerMaxTime = iMaxTimeBetween2ConsecutiveIDRInSeconds * ((pRateControl->uFrameRate * pRateControl->uClkRatio) / 1001);
  AL_TGopParam* pGop = &pChannel->tGopParam;
  int iGopPerMaxTime = iFramesPerMaxTime / pGop->uGopLength;
  pGop->uFreqIDR = Min(iGopPerMaxTime * pGop->uGopLength, pGop->uFreqIDR);
  pGop->uFreqIDR = Min(pGop->uGopLength, pGop->uFreqIDR);
}

/***************************************************************************/
static void AL_sSettings_SetDefaultAVCParam(AL_TEncSettings* pSettings)
{
  pSettings->tChParam[0].uLog2MaxCuSize = AVC_MAX_CU_SIZE;

  if(pSettings->eScalingList == AL_SCL_MAX_ENUM)
    pSettings->eScalingList = AL_SCL_FLAT;
}

/***************************************************************************/
static void AL_sSettings_SetDefaultHEVCParam(AL_TEncSettings* pSettings)
{
  if(pSettings->eScalingList == AL_SCL_MAX_ENUM)
    pSettings->eScalingList = AL_SCL_DEFAULT;
}

/***************************************************************************/
void AL_Settings_SetDefaultRCParam(AL_TRCParam* pRCParam)
{
  pRCParam->eRCMode = AL_RC_CONST_QP;
  pRCParam->uTargetBitRate = 4000000;
  pRCParam->uMaxBitRate = 4000000;
  pRCParam->iInitialQP = -1;

  for(int i = 0; i < ARRAY_SIZE(pRCParam->iMaxQP); i++)
  {
    pRCParam->iMinQP[i] = -1;
    pRCParam->iMaxQP[i] = -1;
  }

  pRCParam->uFrameRate = 30;
  pRCParam->uClkRatio = 1000;
  pRCParam->uInitialRemDelay = 135000; // 1.5 s
  pRCParam->uCPBSize = 270000; // 3.0 s
  pRCParam->uIPDelta = -1;
  pRCParam->uPBDelta = -1;
  pRCParam->uMaxPelVal = 255;
  pRCParam->uMinPSNR = 3300;
  pRCParam->uMaxPSNR = 4200;
  pRCParam->eOptions = AL_RC_OPT_NONE;

  pRCParam->bUseGoldenRef = true;
  pRCParam->uGoldenRefFrequency = 10;
  pRCParam->uPGoldenDelta = 2;
  pRCParam->uMaxConsecSkip = UINT32_MAX;
}

/***************************************************************************/
#define AL_DEFAULT_PROFILE AL_PROFILE_HEVC_MAIN

static int const iBetaOffsetAuto = -1;

/***************************************************************************/
void AL_Settings_SetDefaults(AL_TEncSettings* pSettings)
{
  AL_Assert(pSettings);
  Rtos_Memset(pSettings, 0, sizeof(*pSettings));

  AL_TEncChanParam* pChan = &pSettings->tChParam[0];
  pChan->Direct8x8Infer = true;
  pChan->StrongIntraSmooth = true;

  pChan->eProfile = AL_DEFAULT_PROFILE;
  pChan->uLevel = 51;
  pChan->eEncTools = AL_OPT_LF | AL_OPT_LF_X_SLICE | AL_OPT_LF_X_TILE;
  pChan->eEncOptions |= AL_OPT_RDO_COST_MODE;

  pChan->ePicFormat = AL_420_8BITS;
  pChan->uSrcBitDepth = 8;

  AL_TGopParam* pGop = &pChan->tGopParam;
  pGop->eMode = AL_GOP_MODE_DEFAULT;
  pGop->uFreqIDR = UINT32_MAX;
  pGop->uFreqRP = UINT32_MAX;
  pGop->uGopLength = 30;

  int iAdjust = 1;

  for(int8_t i = 0; i < 4; ++i)
    pGop->tempDQP[i] = (i + 1) * iAdjust;

  pGop->bWriteAvcHdrSvcExt = true;

  pGop->eGdrMode = AL_GDR_OFF;

  AL_Settings_SetDefaultRCParam(&pChan->tRCParam);

  pChan->iTcOffset = -1;
  pChan->iBetaOffset = iBetaOffsetAuto;

  pChan->uNumCore = NUMCORE_AUTO;
  pChan->uNumSlices = 1;

  pSettings->uEnableSEI = AL_SEI_NONE;
  pSettings->bEnableAUD = true;
  pSettings->eEnableFillerData = AL_FILLER_ENC;
  pSettings->eAspectRatio = AL_ASPECT_RATIO_AUTO;
  pSettings->tColorConfig.eColourDescription = AL_COLOUR_DESC_UNSPECIFIED;
  pSettings->tColorConfig.eTransferCharacteristics = AL_TRANSFER_CHARAC_UNSPECIFIED;
  pSettings->tColorConfig.eColourMatrixCoeffs = AL_COLOUR_MAT_COEFF_UNSPECIFIED;

  pSettings->eQpCtrlMode = AL_QP_CTRL_NONE;
  pSettings->eQpTableMode = AL_QP_TABLE_NONE;

  pChan->eLdaCtrlMode = AL_AUTO_LDA;
  AL_Assert(sizeof(pChan->LdaFactors) == sizeof(LAMBDA_FACTORS));
  Rtos_Memcpy(pChan->LdaFactors, LAMBDA_FACTORS, sizeof(LAMBDA_FACTORS));

  pSettings->eScalingList = AL_SCL_MAX_ENUM;

  pSettings->bForceLoad = true;
  pChan->pMeRange[AL_SLICE_P][0] = -1; // Horz
  pChan->pMeRange[AL_SLICE_P][1] = -1; // Vert
  pChan->pMeRange[AL_SLICE_B][0] = -1; // Horz
  pChan->pMeRange[AL_SLICE_B][1] = -1; // Vert
  pChan->uLog2MaxCuSize = HEVC_MAX_CTB_SIZE;
  pChan->uLog2MinCuSize = MIN_CU_SIZE;
  pChan->uLog2MaxTuSize = 5; // 32x32
  pChan->uLog2MinTuSize = 2; // 4x4
  pChan->uMaxTransfoDepthIntra = 1;
  pChan->uMaxTransfoDepthInter = 1;

  pSettings->NumLayer = 1;
  pSettings->NumView = 1;
  pChan->eEntropyMode = AL_MODE_CABAC;

  pChan->eSrcMode = AL_SRC_RASTER;

  pChan->eVideoMode = AL_VM_PROGRESSIVE;

  pChan->MaxNumMergeCand = 5;

  pChan->eStartCodeBytesAligned = AL_START_CODE_AUTO;

#if (defined(ANDROID) || defined(__ANDROID_API__))
  pChan->eStartCodeBytesAligned = AL_START_CODE_4_BYTES;
#endif

}

/***************************************************************************/
#define MSG(msg) { if(pOut) fprintf(pOut, msg "\r\n"); }
#define MSG_WARNING(msg) MSG("[WARNING]: " msg)
#define MSG_ERROR(msg) MSG("[ERROR]: " msg)

#define MSGF(msg, ...) { if(pOut) fprintf(pOut, msg "\r\n", __VA_ARGS__); }
#define MSGF_WARNING(msg, ...) MSGF("[WARNING]: " msg, __VA_ARGS__)
#define MSGF_ERROR(msg, ...) MSGF("[ERROR]: " msg, __VA_ARGS__)

void AL_Settings_SetDefaultParam(AL_TEncSettings* pSettings)
{

  int iMinQP, iMaxQP;
  AL_ParamConstraints_GetQPBounds(AL_GET_CODEC(pSettings->tChParam[0].eProfile), &iMinQP, &iMaxQP);

  for(int i = 0; i < ARRAY_SIZE(pSettings->tChParam[0].tRCParam.iMinQP); i++)
  {
    if(pSettings->tChParam[0].tRCParam.iMinQP[i] == -1)
      pSettings->tChParam[0].tRCParam.iMinQP[i] = iMinQP;
  }

  for(int i = 0; i < ARRAY_SIZE(pSettings->tChParam[0].tRCParam.iMaxQP); i++)
  {
    if(pSettings->tChParam[0].tRCParam.iMaxQP[i] == -1)
      pSettings->tChParam[0].tRCParam.iMaxQP[i] = iMaxQP;
  }

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
    MSG_ERROR("Invalid parameter: Profile");
  }

  if(AL_IS_10BIT_PROFILE(pChParam->eProfile) && (AL_GET_BITDEPTH(pChParam->ePicFormat) > 8) && (AL_HWConfig_Enc_GetSupportedBitDepth() < 10))
  {
    ++err;
    MSG_ERROR("The hardware IP doesn't support 10-bit encoding");
  }

  if(AL_IS_12BIT_PROFILE(pChParam->eProfile) && (AL_GET_BITDEPTH(pChParam->ePicFormat) > 10) && (AL_HWConfig_Enc_GetSupportedBitDepth() < 12))
  {
    ++err;
    MSG_ERROR("The hardware IP doesn't support 12-bit encoding");
  }

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) > AL_HWConfig_Enc_GetSupportedChromaMode())
  {
    {
      ++err;
      MSG_ERROR("The specified ChromaMode is not supported by the IP");
    }
  }

  if(pChParam->bEnableSrcCrop)
  {
    int HStep = AL_GET_BITDEPTH(pChParam->ePicFormat) == 10 ? 24 : 32; // In 10-bit there are 24 samples every 32 bytes
    int VStep = 1; // should be 2 in 4:2:0 but customer requires it to be 1 in any case !

    if((pChParam->uSrcCropPosX % HStep) || (pChParam->uSrcCropPosY % VStep))
    {
      ++err;
      MSG_ERROR("The input crop area doesn't fit the alignment constraints for the current buffer format");
    }
  }

  if(pChParam->bEnableSrcCrop)
  {
    if(pChParam->uSrcCropPosX + pChParam->uSrcCropWidth > pChParam->uSrcWidth ||
       pChParam->uSrcCropPosY + pChParam->uSrcCropHeight > pChParam->uSrcHeight)
    {
      ++err;
      MSG_ERROR("The input crop area doesn't fit in the input picture");
    }
  }

  if(!AL_sSettings_CheckLevel(pChParam->eProfile, pChParam->uLevel))
  {
    ++err;
    MSG_ERROR("Invalid parameter: Level");
  }

  if(AL_IS_INTRA_PROFILE(pChParam->eProfile) && (pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL))
  {
    ++err;
    MSG_ERROR("Pyramidal and IntraProfile is not supported");
  }

  if(pChParam->uLog2MinCuSize != MIN_CU_SIZE)
  {
    ++err;
    MSG_ERROR("Invalid parameter: MinCuSize");
  }

  bool bHEVCCuSupported = pChParam->uLog2MaxCuSize == HEVC_MIN_CTB_SIZE;

  if(AL_IS_HEVC(pChParam->eProfile) && !bHEVCCuSupported)
  {
    ++err;
    MSG_ERROR("Invalid parameter: Log2MaxCuSize");
  }

  if(AL_IS_AVC(pChParam->eProfile) && (pChParam->uLog2MaxCuSize != AVC_MAX_CU_SIZE))
  {
    ++err;
    MSG_ERROR("Invalid parameter: Log2MaxCuSize");
  }

  if(pChParam->uNumCore != NUMCORE_AUTO)
  {
    AL_NumCoreDiagnostic diagnostic;

    if(!AL_Constraint_NumCoreIsSane(AL_GET_CODEC(pChParam->eProfile), pChParam->uEncWidth, pChParam->uNumCore, pChParam->uLog2MaxCuSize, &diagnostic))
    {
      ++err;
      MSGF_ERROR("Invalid parameter: NumCore. The width should at least be %d CTB per core. With the specified number of core, it is %d CTB per core. (Multi core alignment constraint might be the reason of this error if the CTB are equal)", diagnostic.requiredWidthInCtbPerCore, diagnostic.actualWidthInCtbPerCore);
    }

    if(AL_IS_HEVC(pChParam->eProfile) && pSettings->tChParam[0].uCabacInitIdc > 1)
    {
      ++err;
      MSG_ERROR("Invalid parameter: CabacInit");
    }

    int MinCoreWidth = 256;

    if(AL_IS_HEVC(pChParam->eProfile) && (pChParam->uEncWidth < MinCoreWidth * pChParam->uNumCore))
    {
      ++err;
      MSG_ERROR("Invalid parameter: NumCore. The width should at least be 256 pixels per core for HEVC conformance");
    }
  }

  if(AL_IS_AVC(pChParam->eProfile) && pSettings->tChParam[0].uCabacInitIdc > 2)
  {
    ++err;
    MSG_ERROR("Invalid parameter: CabacInit");
  }
  int const iCTBSize = (1 << pChParam->uLog2MaxCuSize);

  if(pChParam->uEncHeight <= iCTBSize)
  {
    ++err;
    MSG_ERROR("Invalid parameter: Height. The encoded height should at least be 2 CTB");
  }

  int iMinQP, iMaxQP;
  AL_ParamConstraints_GetQPBounds(AL_GET_CODEC(pChParam->eProfile), &iMinQP, &iMaxQP);

  for(int i = 0; i < ARRAY_SIZE(pChParam->tRCParam.iMaxQP); i++)
  {
    if(pChParam->tRCParam.iMaxQP[i] < pChParam->tRCParam.iMinQP[i])
    {
      ++err;
      MSG_ERROR("Invalid parameters: MinQP and MaxQP");
    }

    if(pChParam->tRCParam.iMaxQP[i] > iMaxQP)
    {
      ++err;
      MSG_ERROR("Invalid parameter: MaxQP");
    }

    if(pChParam->tRCParam.iMinQP[i] < iMinQP)
    {
      ++err;
      MSG_ERROR("Invalid parameter: MinQP");
    }
  }

  if(pChParam->tRCParam.iInitialQP > iMaxQP)
  {
    ++err;
    MSG_ERROR("Invalid parameter: SliceQP");
  }

  if(pChParam->tRCParam.uTargetBitRate < 10)
  {
    if(AL_IS_CBR(pChParam->tRCParam.eRCMode) || (pChParam->tRCParam.eRCMode == AL_RC_VBR))
    {
      ++err;
      MSG_ERROR("Invalid parameter: BitRate");
    }
  }

  if(AL_IS_AV1(pChParam->eProfile) && AL_IS_HWRC_ENABLED(&pChParam->tRCParam))
  {
    ++err;
    MSG_ERROR("Hardware Rate Control (LOW_LATENCY or MaxPictureSize) is not supported in AV1");
  }

  if(pChParam->tRCParam.eRCMode == AL_RC_LOW_LATENCY)
  {
    if(pChParam->tGopParam.uNumB > 0)
    {
      ++err;
      MSG_ERROR("B Picture not allowed in LOW_LATENCY Mode");
    }
  }

  if(pChParam->bSubframeLatency && (pChParam->tGopParam.uNumB > 0))
  {
    ++err;
    MSG_ERROR("B Picture not allowed in subframe latency");
  }

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_DEFAULT)
  {
    if(pChParam->tGopParam.uGopLength > 1000)
    {
      ++err;
      MSG_ERROR("Invalid parameter: Gop.Length");
    }

    if(pChParam->tGopParam.uNumB > 4)
    {
      ++err;
      MSG_ERROR("Invalid parameter: Gop.NumB");
    }
  }

  int iMaxSlices = (pChParam->uEncHeight + iCTBSize - 1) / iCTBSize;

  if((pChParam->uNumSlices < 1) || (pChParam->uNumSlices > iMaxSlices) || (pChParam->uNumSlices > AL_MAX_ENC_SLICE) || ((pChParam->bSubframeLatency) && (pChParam->uNumSlices > AL_MAX_SLICES_SUBFRAME)))
  {
    ++err;
    MSG_ERROR("Invalid parameter: NumSlices");
  }

  if(pChParam->eEncOptions & AL_OPT_FORCE_MV_CLIP)
  {
    if(pSettings->uClipHrzRange < 64 || pSettings->uClipHrzRange > pChParam->uEncWidth)
    {
      ++err;
      MSG_ERROR("Horizontal clipping range must be between 64 and Picture width! ");
    }

    if(pSettings->uClipVrtRange < 8 || pSettings->uClipVrtRange > pChParam->uEncHeight)
    {
      ++err;
      MSG_ERROR("Vertical clipping range must be between 8 and Picture Height! ");
    }
  }

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
  ECheckResolutionError eResError = AL_ParamConstraints_CheckResolution(pChParam->eProfile, eChromaMode, 1 << pChParam->uLog2MaxCuSize, pChParam->uEncWidth, pChParam->uEncHeight);

  if(eResError == CRERROR_WIDTHCHROMA)
  {
    ++err;
    MSG_ERROR("Width shall be multiple of 2 on 420 or 422 chroma mode!");
  }
  else if(eResError == CRERROR_HEIGHTCHROMA)
  {
    ++err;
    MSG_ERROR("Width shall be multiple of 2 on 420 or 422 chroma mode!");
  }
  else if(eResError == CRERROR_64x64_MIN_RES)
  {
    ++err;
    MSG_ERROR("LCU64x64 encoding is currently limited to resolution higher or equal to 72x72!");
  }
  else if(eResError == CERROR_RES_ALIGNMENT)
  {
    ++err;
    MSG_ERROR("In AV1 or VP9 Profile, resolution must be multiple of 8!");
  }

  int iNumB = pChParam->tGopParam.uNumB;

  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && iNumB != 3 && iNumB != 5 && iNumB != 7 && iNumB != 15)
  {
    ++err;
    MSG_ERROR("PYRAMIDAL GOP pattern only allows 3, 5, 7, 15 B Frames");
  }

  if(!AL_ParamConstraints_CheckChromaOffsets(pChParam->eProfile, pChParam->iCbPicQpOffset, pChParam->iCrPicQpOffset,
                                             pChParam->iCbSliceQpOffset, pChParam->iCrSliceQpOffset))
  {
    ++err;
    MSG_ERROR("Invalid chroma QP offset. Check following parameters: CrQpOffset/CbQpOffset/SliceCbQpOffset/SliceCrQpOffset");
  }

  if(AL_IS_AVC(pChParam->eProfile))
  {
    if(pChParam->tRCParam.iInitialQP >= 0)
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrSliceQpOffset;

      if(((pSettings->eQpCtrlMode == AL_QP_CTRL_NONE) && (pSettings->eQpTableMode == AL_QP_TABLE_NONE)) && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG_ERROR("Invalid parameter: SliceQP, CrQpOffset");
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbSliceQpOffset;

      if(((pSettings->eQpCtrlMode == AL_QP_CTRL_NONE) && (pSettings->eQpTableMode == AL_QP_TABLE_NONE)) && (0 > iQP || iQP > 51))
      {
        ++err;
        MSG_ERROR("Invalid parameter: SliceQP, CbQpOffset");
      }
    }

    if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && (AL_GET_PROFILE_IDC(pChParam->eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE)))
    {
      ++err;
      MSG_ERROR("PYRAMIDAL GOP pattern doesn't allows baseline profile");
    }
  }

  if(pChParam->eVideoMode >= AL_VM_MAX_ENUM)
  {
    ++err;
    MSG_ERROR("Invalid parameter: VideoMode");
  }

  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE)
  {
    if(!AL_IS_HEVC(pChParam->eProfile) && !AL_IS_AVC(pChParam->eProfile))
    {
      ++err;
      MSG_ERROR("Interlaced Video mode is not supported in this profile");
    }

  }

  if(pSettings->LookAhead < 0)
  {
    ++err;
    MSG_ERROR("Invalid parameter: LookAheadMode should be 0 or above");
  }

  if(pSettings->TwoPass < 0 || pSettings->TwoPass > 2)
  {
    ++err;
    MSG_ERROR("Invalid parameter: TwoPass should be 0, 1 or 2");
  }

  if(pSettings->TwoPass != 0 && pSettings->LookAhead != 0)
  {
    ++err;
    MSG_ERROR("Shouldn't have TwoPass and LookAhead at the same time");
  }

  if((pSettings->TwoPass != 0 || pSettings->LookAhead != 0) && pChParam->bSubframeLatency)
  {
    ++err;
    MSG_ERROR("Shouldn't have SliceLat and TwoPass/LookAhead at the same time");
  }

  if((pSettings->TwoPass != 0 || pSettings->LookAhead != 0) && (pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL))
  {
    ++err;
    MSG_ERROR("Shouldn't have Pyramidal GOP and TwoPass/LookAhead at the same time");
  }

  if(pSettings->LookAhead == 0 && pSettings->bEnableFirstPassSceneChangeDetection)
  {
    ++err;
    MSG_ERROR("Shouldn't have FirstPassSceneChangeDetection enabled without LookAhead");
  }

  int iCropMaskX = (eChromaMode == AL_CHROMA_4_2_0 || eChromaMode == AL_CHROMA_4_2_2) ? 1 : 0;
  int iCropMaskY = (eChromaMode == AL_CHROMA_4_2_0) ? 1 : 0;

  if((pChParam->uOutputCropPosX & iCropMaskX) || (pChParam->uOutputCropWidth & iCropMaskX)
     || (pChParam->uOutputCropPosY & iCropMaskY) || (pChParam->uOutputCropHeight & iCropMaskY))
  {
    ++err;
    MSG_ERROR("Invalid Output crop parameter for this chroma subsampling mode");
  }

  int iCropWidth = pChParam->uOutputCropWidth ? pChParam->uOutputCropWidth : pChParam->uEncWidth;
  int iCropHeight = pChParam->uOutputCropHeight ? pChParam->uOutputCropHeight : pChParam->uEncHeight;

  if(pChParam->uOutputCropPosX + iCropWidth > pChParam->uEncWidth
     || pChParam->uOutputCropPosY + iCropHeight > pChParam->uEncHeight)
  {
    ++err;
    MSG_ERROR("Output crop region shall fit within the encoded picture");
  }

  if(AL_IS_VVC(pChParam->eProfile))
  {
    if(AL_GET_BITDEPTH(pChParam->ePicFormat) > 8 || eChromaMode == AL_CHROMA_4_4_4 || pChParam->tRCParam.eRCMode != AL_RC_CONST_QP || pSettings->eQpCtrlMode != AL_QP_CTRL_NONE
       || pSettings->eQpTableMode != AL_QP_TABLE_NONE || pChParam->tGopParam.bEnableLT == true
       )
    {
      ++err;
      MSG_ERROR("VVC encoder only supports 8bits const_qp simple encoding");
    }
  }

  if(pSettings->bDependentSlice && AL_GET_CODEC(pChParam->eProfile) != AL_CODEC_HEVC)
  {
    err++;
    MSG_ERROR("The parameter DependentSlice is only available for HEVC");
  }

  if(pChParam->bSubframeLatency && AL_IS_AVC(pChParam->eProfile))
  {
    int const iCTBSize = (1 << pChParam->uLog2MaxCuSize);

    if((pChParam->uEncHeight / iCTBSize) < (int)pChParam->uNumCore)
    {
      ++err;
      MSG_ERROR("In SliceLat and with AVC, the encoding height must be equal to (CTB size) * (numCore)");
    }
  }

  return err;
}

/***************************************************************************/
bool checkBitDepthCoherency(int iBitDepth, AL_EProfile eProfile)
{
  switch(iBitDepth)
  {
  case 8: return true;  // 8 bits is supported by all profile
  case 10: return AL_IS_10BIT_PROFILE(eProfile);
  case 12: return AL_IS_12BIT_PROFILE(eProfile);
  default: return false;
  }

  return false;
}

/***************************************************************************/
bool checkChromaCoherency(AL_EChromaMode eChroma, AL_EProfile eProfile)
{
  switch(eChroma)
  {
  case AL_CHROMA_4_0_0: return AL_IS_MONO_PROFILE(eProfile);
  case AL_CHROMA_4_2_0: return AL_IS_420_PROFILE(eProfile);
  case AL_CHROMA_4_2_2: return AL_IS_422_PROFILE(eProfile);
  case AL_CHROMA_4_4_4: return AL_IS_444_PROFILE(eProfile);
  default: return false;
  }

  return false;
}

/***************************************************************************/
bool checkProfileCoherency(int iBitDepth, AL_EChromaMode eChroma, AL_EProfile eProfile)
{

  if(!checkBitDepthCoherency(iBitDepth, eProfile))
    return false;

  return checkChromaCoherency(eChroma, eProfile);
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
    case AL_CHROMA_4_4_4: return AL_PROFILE_HEVC_MAIN_444;
    default: AL_Assert(0);
    }

    break;
  }
  case 10:
    switch(eChroma)
    {
    case AL_CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO10;
    case AL_CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN10;
    case AL_CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422_10;
    case AL_CHROMA_4_4_4: return AL_PROFILE_HEVC_MAIN_444_10;
    default: AL_Assert(0);
    }

    break;

  case 12:
    switch(eChroma)
    {
    // case AL_CHROMA_4_0_0: return AL_PROFILE_HEVC_MONO12;
    case AL_CHROMA_4_2_0: return AL_PROFILE_HEVC_MAIN12;
    case AL_CHROMA_4_2_2: return AL_PROFILE_HEVC_MAIN_422_12;
    case AL_CHROMA_4_4_4: return AL_PROFILE_HEVC_MAIN_444_12;
    default: AL_Assert(0);
    }

    break;

  default:
    AL_Assert(0);
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
    case AL_CHROMA_4_4_4: return AL_PROFILE_AVC_HIGH_444_PRED;
    default: AL_Assert(0);
    }

    break;
  }
  case 10:
  {
    switch(eChroma)
    {
    case AL_CHROMA_4_0_0: return AL_PROFILE_AVC_HIGH10;
    case AL_CHROMA_4_2_0: return AL_PROFILE_AVC_HIGH10;
    case AL_CHROMA_4_2_2: return AL_PROFILE_AVC_HIGH_422;
    case AL_CHROMA_4_4_4: return AL_PROFILE_AVC_HIGH_444_PRED;
    default: AL_Assert(0);
    }

    break;
  }
  case 12:
    return AL_PROFILE_AVC_HIGH_444_PRED;
  default:
    AL_Assert(0);
  }

  AL_Assert(0);

  return AL_PROFILE_AVC;
}

static AL_EProfile getMinimumProfile(AL_ECodec eCodec, int iBitDepth, AL_EChromaMode eChromaMode)
{
  (void)iBitDepth;
  (void)eChromaMode;
  switch(eCodec)
  {
  case AL_CODEC_AVC: return getAvcMinimumProfile(iBitDepth, eChromaMode);
  case AL_CODEC_HEVC: return getHevcMinimumProfile(iBitDepth, eChromaMode);
  default:
    AL_Assert(0);
  }

  return AL_PROFILE_UNKNOWN;
}

/****************************************************************************/
static void AL_sCheckRange16b(int16_t* pRange, int* pNumIncoherency, const int16_t iMinVal, const int16_t iMaxVal, char* pRangeName, FILE* pOut)
{
  if((*pRange >= 0) && (iMinVal > *pRange || *pRange > iMaxVal))
  {
    *pRange = Min(iMaxVal, Max(iMinVal, *pRange));
    MSGF_WARNING("Range of %s is [%d; %d]", pRangeName, iMinVal, iMaxVal);
    ++(*pNumIncoherency);
  }
}

/***************************************************************************/
static void CorrectBitRateParams(AL_TEncChanParam* pChParam, TFourCC tFourCC, int iBitDepth, int* numIncoherency, FILE* pOut)
{
  if(AL_IS_CBR(pChParam->tRCParam.eRCMode))
  {
    pChParam->tRCParam.uMaxBitRate = pChParam->tRCParam.uTargetBitRate;
  }

  if(pChParam->tRCParam.uTargetBitRate > pChParam->tRCParam.uMaxBitRate)
  {
    MSG_WARNING("The specified MaxBitRate has to be greater than or equal to [Target]BitRate and will be adjusted");
    pChParam->tRCParam.uMaxBitRate = pChParam->tRCParam.uTargetBitRate;
    (*numIncoherency)++;
  }

  if(AL_IS_HEVC(pChParam->eProfile) || AL_IS_AVC(pChParam->eProfile))
  {
    if(AL_IS_CBR(pChParam->tRCParam.eRCMode))
    {
      AL_EChromaMode eInputChromaMode = AL_GetChromaMode(tFourCC);
      AL_TDimension tDim = { pChParam->uEncWidth, pChParam->uEncHeight };
      uint32_t maxNalSizeInByte = GetPcmVclNalSize(tDim, eInputChromaMode, iBitDepth);
      uint64_t maxBitRate = 8LL * maxNalSizeInByte * pChParam->tRCParam.uFrameRate;

      if(pChParam->tRCParam.uTargetBitRate > maxBitRate)
      {
        MSG_WARNING("The specified TargetBitRate is too high for this use case and will be adjusted");
        pChParam->tRCParam.uTargetBitRate = maxBitRate;
        pChParam->tRCParam.uMaxBitRate = maxBitRate;
        (*numIncoherency)++;
      }
    }
  }
}

/***************************************************************************/
int AL_Settings_CheckCoherency(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, TFourCC tFourCC, FILE* pOut)
{
  AL_Assert(pSettings);

  int numIncoherency = 0;

  int iBitDepth = AL_GET_BITDEPTH(pChParam->ePicFormat);

  if(AL_GetBitDepth(tFourCC) < iBitDepth)
    ++numIncoherency;

  uint16_t uMaxNumB = Max(pChParam->tGopParam.uGopLength - 2, 0);

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_DEFAULT &&
     pChParam->tGopParam.uNumB > uMaxNumB)
  {
    MSG_WARNING("Number of B frames per GOP is too high");
    pChParam->tGopParam.uNumB = uMaxNumB;
  }

  if(!AL_IS_AVC(pChParam->eProfile) && pChParam->bUseUniformSliceType)
  {
    MSG_WARNING("Use uniform slice type is not available in this codec");
    pChParam->bUseUniformSliceType = false;
  }

  if((pChParam->uSliceSize > 0) && (pChParam->uLog2MaxCuSize > 5))
  {
    MSG_WARNING("Slice size is not available with Log2MaxCuSize > 5");
    pChParam->uSliceSize = 0;
    ++numIncoherency;
  }

  bool bIsLoopFilterEnable = (pChParam->eEncTools & AL_OPT_LF);

  if(pChParam->tGopParam.eGdrMode != AL_GDR_OFF)
  {
    bool bCheckSEI = false;

    if(AL_IS_AVC(pChParam->eProfile))
      bCheckSEI = true;

    if(AL_IS_HEVC(pChParam->eProfile))
      bCheckSEI = true;

    if(bCheckSEI && (pSettings->uEnableSEI & AL_SEI_RP) == 0)
    {
      pSettings->uEnableSEI |= AL_SEI_RP;
      MSG_WARNING("Force Recovery Point SEI in GDR");
    }

    if(bIsLoopFilterEnable)
    {
      {
        pChParam->eEncTools &= ~AL_OPT_LF;
        MSG_WARNING("Loop filter is not allowed with GDR enabled");
        bIsLoopFilterEnable = false;
      }
    }

    int iLimit = (pChParam->tGopParam.eGdrMode == AL_GDR_VERTICAL) ? pChParam->uEncWidth : pChParam->uEncHeight;
    int iLcuSize = 1 << pChParam->uLog2MaxCuSize;
    int iNbLcu = (iLimit + iLcuSize - 1) / iLcuSize;

    if((int)pChParam->tGopParam.uFreqRP < iNbLcu)
    {
      pChParam->tGopParam.uFreqRP = iNbLcu;
      MSG_WARNING("Frequency of recovery point must be at least equal to the number of LCU line/column");
    }
  }

#if (defined(ANDROID) || defined(__ANDROID_API__))

  if(AL_IS_ITU_CODEC(AL_GET_CODEC(pChParam->eProfile)))
  {
    if(pChParam->eStartCodeBytesAligned != AL_START_CODE_4_BYTES)
    {
      MSG_WARNING("Start code bytes must be 4-bytes aligned");
      pChParam->eStartCodeBytesAligned = AL_START_CODE_4_BYTES;
    }
  }
#endif

  if(!bIsLoopFilterEnable)
  {
    pChParam->iBetaOffset = 0;
    pChParam->iTcOffset = 0;
  }

  AL_EChromaMode const eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);

  if(!checkProfileCoherency(iBitDepth, eChromaMode, pSettings->tChParam[0].eProfile))
  {
    MSG_WARNING("Adapting profile to support bitdepth and chroma mode");
    pSettings->tChParam[0].eProfile = getMinimumProfile(AL_GET_CODEC(pSettings->tChParam[0].eProfile), iBitDepth, eChromaMode);
    ++numIncoherency;
  }

  bool bHasQPTables = AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode) || AL_IS_QP_TABLE_REQUIRED(pSettings->eQpTableMode);

  if(bHasQPTables)
  {

    if((pChParam->uLog2MaxCuSize - pChParam->uCuQPDeltaDepth) < pChParam->uLog2MinCuSize)
    {
      MSG_WARNING("CuQpDeltaDepth doesn't match Log2MinCUSize");
      ++numIncoherency;
      pChParam->uCuQPDeltaDepth = pChParam->uLog2MaxCuSize - pChParam->uLog2MinCuSize;
    }

    if((pChParam->uLog2MaxCuSize == 6 && pChParam->uCuQPDeltaDepth > 3) &&
       (pChParam->uLog2MaxCuSize != 6 && pChParam->uCuQPDeltaDepth > 2))
    {
      MSG_WARNING("CuQpDeltaDepth is too high!");
      ++numIncoherency;
      pChParam->uCuQPDeltaDepth = pChParam->uLog2MaxCuSize == 6 ? 3 : 2;
    }

    if(AL_IS_AUTO_OR_ADAPTIVE_QP_CTRL(pSettings->eQpCtrlMode) && pChParam->uLog2MaxCuSize == 6 && pChParam->uCuQPDeltaDepth == 0)
    {
      MSG_WARNING("CuQPDeltaDepth shall be superior to zero in AUTO_QP mode!");
      ++numIncoherency;
      pChParam->uCuQPDeltaDepth = 1;
    }

  }
  else
  {
    pChParam->uCuQPDeltaDepth = 0;
  }

  CorrectBitRateParams(pChParam, tFourCC, iBitDepth, &numIncoherency, pOut);

  if(pChParam->tGopParam.uNumB > 0)
  {
    int16_t iDefaultDelta = AL_IS_ITU_CODEC(AL_GET_CODEC(pChParam->eProfile)) ? 0 : 1;

    if((pChParam->tRCParam.uPBDelta >= iDefaultDelta) ^ (pChParam->tRCParam.uIPDelta >= iDefaultDelta))
    {
      MSG_WARNING("Both or none of PBDelta and IPDelta parameters must be set at the same time. They will be adjusted.");
      pChParam->tRCParam.uPBDelta = -1;
      pChParam->tRCParam.uIPDelta = -1;
    }
  }

  int16_t iMeMinVRange;
  int16_t iMeMaxRange[2][2];
  switch(AL_GET_CODEC(pChParam->eProfile))
  {
  case AL_CODEC_AVC:
    iMeMaxRange[AL_SLICE_P][0] = AVC_MAX_HORIZONTAL_RANGE_P;
    iMeMaxRange[AL_SLICE_P][1] = AVC_MAX_VERTICAL_RANGE_P;
    iMeMaxRange[AL_SLICE_B][0] = AVC_MAX_HORIZONTAL_RANGE_B;
    iMeMaxRange[AL_SLICE_B][1] = AVC_MAX_VERTICAL_RANGE_B;
    iMeMinVRange = 8;
    break;
  default:
    iMeMaxRange[AL_SLICE_P][0] = HEVC_MAX_HORIZONTAL_RANGE_P;
    iMeMaxRange[AL_SLICE_P][1] = HEVC_MAX_VERTICAL_RANGE_P;
    iMeMaxRange[AL_SLICE_B][0] = HEVC_MAX_HORIZONTAL_RANGE_B;
    iMeMaxRange[AL_SLICE_B][1] = HEVC_MAX_VERTICAL_RANGE_B;
    iMeMinVRange = 16;
    break;
  }

  AL_sCheckRange16b(&pChParam->pMeRange[AL_SLICE_P][0], &numIncoherency, 0, iMeMaxRange[AL_SLICE_P][0], "Hrz P-frame Me-range", pOut);
  AL_sCheckRange16b(&pChParam->pMeRange[AL_SLICE_P][1], &numIncoherency, iMeMinVRange, iMeMaxRange[AL_SLICE_P][1], "Vrt P-frame Me-range", pOut);
  AL_sCheckRange16b(&pChParam->pMeRange[AL_SLICE_B][0], &numIncoherency, 0, iMeMaxRange[AL_SLICE_B][0], "Hrz B-frame Me-range", pOut);
  AL_sCheckRange16b(&pChParam->pMeRange[AL_SLICE_B][1], &numIncoherency, iMeMinVRange, iMeMaxRange[AL_SLICE_B][1], "Vrt B-frame Me-range", pOut);

  if((pChParam->uSliceSize > 0) && (pChParam->eEncTools & AL_OPT_WPP))
  {
    pChParam->eEncTools &= ~AL_OPT_WPP;
    MSG_WARNING("Wavefront Parallel Processing is not allowed with SliceSize; it will be adjusted");
    ++numIncoherency;
  }

  if(AL_IS_INTRA_PROFILE(pChParam->eProfile))
  {

    if(pSettings->iPrefetchLevel2 != 0)
    {
      pSettings->iPrefetchLevel2 = 0;
      MSG_WARNING("iPrefetchLevel2 shall be set to 0 for Intra only profile; it will be adjusted");
      ++numIncoherency;
    }

    if(pChParam->tGopParam.uGopLength > 1)
    {
      pChParam->tGopParam.uGopLength = 0;
      MSG_WARNING("Gop.Length shall be set to 0 or 1 for Intra only profile; it will be adjusted");
      ++numIncoherency;
    }

    if(pChParam->tGopParam.uFreqIDR > 1)
    {
      pChParam->tGopParam.uFreqIDR = 0;
      MSG_WARNING("Gop.uFreqIDR shall be set to 0 or 1 for Intra only profile; it will be adjusted");
      ++numIncoherency;
    }
  }

  if(AL_IS_AVC(pChParam->eProfile))
  {
    pChParam->uLog2MaxTuSize = 3;
    pChParam->iCbSliceQpOffset = pChParam->iCrSliceQpOffset = 0;

    if(pChParam->eEncTools & AL_OPT_WPP)
      pChParam->eEncTools &= ~AL_OPT_WPP;

    if((pChParam->eProfile != AL_PROFILE_AVC_CAVLC_444_INTRA) &&
       (AL_GET_PROFILE_IDC(pChParam->eProfile) < AL_GET_PROFILE_IDC(AL_PROFILE_AVC_HIGH)))
    {
      if(pChParam->uLog2MaxTuSize != 2)
      {
        // cannot be set by cfg
        pChParam->uLog2MaxTuSize = 2;
      }

      if(pChParam->iCrPicQpOffset != pChParam->iCbPicQpOffset)
      {
        pChParam->iCrPicQpOffset = pChParam->iCbPicQpOffset;
        MSG_WARNING("The Specified CrQpOffset is not allowed; it will be adjusted");
        ++numIncoherency;
      }

      if(pSettings->eScalingList != AL_SCL_FLAT)
      {
        pSettings->eScalingList = AL_SCL_FLAT;
        MSG_WARNING("The specified ScalingList is not allowed; it will be adjusted");
        ++numIncoherency;
      }

      if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) != AL_CHROMA_4_2_0)
      {
        AL_SET_CHROMA_MODE(&pChParam->ePicFormat, AL_CHROMA_4_2_0);
        MSG_WARNING("The specified ChromaMode and Profile are not allowed; they will be adjusted");
        ++numIncoherency;
      }
    }

    if((pChParam->eProfile == AL_PROFILE_AVC_CAVLC_444_INTRA) ||
       (AL_GET_PROFILE_IDC(pChParam->eProfile) == AL_GET_PROFILE_IDC(AL_PROFILE_AVC_BASELINE)))
    {
      if(pChParam->eEntropyMode == AL_MODE_CABAC)
      {
        pChParam->eEntropyMode = AL_MODE_CAVLC;
        MSG_WARNING("CABAC encoding is not allowed with this profile; CAVLC will be used instead");
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
        MSG_WARNING("The specified NumB is not allowed in this profile; it will be adjusted");
        ++numIncoherency;
      }
    }

    if(pChParam->uCuQPDeltaDepth > 0)
    {
      MSG_WARNING("In AVC CuQPDeltaDepth is not allowed; it will be adjusted");
      pChParam->uCuQPDeltaDepth = 0;
      ++numIncoherency;
    }

  }

  if(pChParam->bForcePpsIdToZero && !(pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY) && (pChParam->tGopParam.uNumB != 0))
  {
    pChParam->bForcePpsIdToZero = false;
    MSG_WARNING("ForcePpsIdToZero can't be used with the reordered B-Frames. It will be disabled");
    ++numIncoherency;
  }

  int const MAX_LOW_DELAY_B_GOP_LENGTH = 4;
  int const MIN_LOW_DELAY_B_GOP_LENGTH = 1;

  if(pChParam->tRCParam.bUseGoldenRef && (pChParam->tGopParam.uNumB > 0 || pChParam->tRCParam.eRCMode != AL_RC_CONST_QP))
    pChParam->tRCParam.bUseGoldenRef = false;

  if(pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B && (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH ||
                                                              pChParam->tGopParam.uGopLength < MIN_LOW_DELAY_B_GOP_LENGTH))
  {
    pChParam->tGopParam.uGopLength = (pChParam->tGopParam.uGopLength > MAX_LOW_DELAY_B_GOP_LENGTH) ? MAX_LOW_DELAY_B_GOP_LENGTH : MIN_LOW_DELAY_B_GOP_LENGTH;
    MSG_WARNING("The specified Gop.Length value in low delay B mode is not allowed, value will be adjusted");
    ++numIncoherency;
  }

  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL) && (pChParam->tGopParam.uGopLength % (pChParam->tGopParam.uNumB + 1)) != 0)
  {
    int iRound = pChParam->tGopParam.uNumB + 1;
    pChParam->tGopParam.uGopLength = ((pChParam->tGopParam.uGopLength + iRound - 1) / iRound) * iRound;
    MSG_WARNING("The specified Gop.Length value in pyramidal gop mode is not reachable, value will be adjusted");
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
        MSG_WARNING("With this QPControlMode, this CrQPOffset cannot be set for this SliceQP");
        ++numIncoherency;
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCbPicQpOffset = pChParam->iCbSliceQpOffset = 0;
        MSG_WARNING("With this QPControlMode, this CbQPOffset cannot be set for this SliceQP");
        ++numIncoherency;
      }
    }
    else if(AL_IS_AVC(pChParam->eProfile))
    {
      int iQP = pChParam->tRCParam.iInitialQP + pChParam->iCrPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCrPicQpOffset = 0;
        MSG_WARNING("With this QPControlMode, this CrQPOffset cannot be set for this SliceQP");
        ++numIncoherency;
      }
      iQP = pChParam->tRCParam.iInitialQP + pChParam->iCbPicQpOffset;

      if(iQP - 3 < 0 || iQP + 3 > 51)
      {
        pChParam->iCbPicQpOffset = 0;
        MSG_WARNING("With this QPControlMode, this CbQPOffset cannot be set for this SliceQP");
        ++numIncoherency;
      }
    }
  }

  if(pChParam->tGopParam.uFreqLT > 0 && !pChParam->tGopParam.bEnableLT)
  {
    pChParam->tGopParam.bEnableLT = true;
    MSG_WARNING("Enabling long term references as a long term frequency is provided");
    ++numIncoherency;
  }

  if(pChParam->tGopParam.bEnableLT && pChParam->tGopParam.uFreqLT <= 0)
  {
    pChParam->tGopParam.uFreqLT = pChParam->tGopParam.uGopLength;
    MSG_WARNING("Setting long term frequency to gop length as long term is enabled");
    ++numIncoherency;
  }

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL)
  {
    if(pChParam->tGopParam.bEnableLT)
    {
      pChParam->tGopParam.bEnableLT = false;
      pChParam->tGopParam.uFreqLT = 0;
      MSG_WARNING("Long Term reference are not allowed with PYRAMIDAL GOP, it will be adjusted");
      ++numIncoherency;
    }

  }

  if(AL_HAS_LEVEL(pChParam->eProfile))
  {
    uint8_t uMinLevel = AL_sSettings_GetMinLevel(pChParam);

    if(uMinLevel == 255)
    {
      MSG_ERROR("The specified configuration requires a level too high for the IP encoder");
      return -1;
    }

    if(pChParam->uLevel < uMinLevel)
    {
      if(!AL_sSettings_CheckLevel(pChParam->eProfile, uMinLevel))
      {
        MSG_WARNING("The specified configuration requires a level too high for the IP encoder");
        ++numIncoherency;
      }
      else
      {
        MSG_WARNING("The specified Level is too low and will be adjusted");
        ++numIncoherency;
      }
      pChParam->uLevel = uMinLevel;
    }
  }

  if(AL_IS_HEVC(pChParam->eProfile) && pChParam->uLevel < 40 && pChParam->uTier)
  {
    pChParam->uTier = 0;
  }

  if(AL_IS_HEVC(pChParam->eProfile) || AL_IS_AVC(pChParam->eProfile))
  {
    if(pChParam->tRCParam.eRCMode != AL_RC_CONST_QP)
    {
      uint64_t uCPBSize = ((AL_64U)pChParam->tRCParam.uCPBSize * pChParam->tRCParam.uMaxBitRate) / 90000LL;
      uint32_t uMaxCPBSize = AL_sSettings_GetMaxCPBSize(pChParam);

      if(uCPBSize > uMaxCPBSize)
      {
        MSG_WARNING("The specified CPBSize is higher than the Max CPBSize allowed for this level and will be adjusted");
        ++numIncoherency;
        pChParam->tRCParam.uCPBSize = uMaxCPBSize * 90000LL / pChParam->tRCParam.uMaxBitRate;
      }
    }
  }

  if(pChParam->tRCParam.uCPBSize < pChParam->tRCParam.uInitialRemDelay)
  {
    MSG_WARNING("The specified InitialDelay is bigger than CPBSize and will be adjusted");
    ++numIncoherency;
    pChParam->tRCParam.uInitialRemDelay = pChParam->tRCParam.uCPBSize;
  }

  if(AL_IS_HEVC(pChParam->eProfile))
  {
    int iNumCore = pChParam->uNumCore;
    AL_CoreConstraint constraint;
    const int pCycles32x32Counts[] = ENCODER_CYCLES_FOR_BLK_32X32;
    AL_CoreConstraint_Init(&constraint, ENCODER_CORE_FREQUENCY, ENCODER_CORE_FREQUENCY_MARGIN, pCycles32x32Counts, 0, AL_ENC_CORE_MAX_WIDTH, 1);

    if(iNumCore == NUMCORE_AUTO)
      iNumCore = AL_CoreConstraint_GetExpectedNumberOfCores(&constraint, pChParam->uEncWidth, pChParam->uEncHeight, AL_GET_CHROMA_MODE(pChParam->ePicFormat), pChParam->tRCParam.uFrameRate * 1000, pChParam->tRCParam.uClkRatio);

    bool bHasTile = iNumCore > 1;

    if(bHasTile && pChParam->uNumSlices > AL_HEVC_GetMaxTileRows(pChParam->uLevel))
    {
      pChParam->uNumSlices = Clip3(pChParam->uNumSlices, 1, AL_HEVC_GetMaxTileRows(pChParam->uLevel));
      MSG_WARNING("With this Configuration, this NumSlices cannot be set");
      ++numIncoherency;
    }
  }

  if(pChParam->tGopParam.eGdrMode == AL_GDR_VERTICAL)
    pChParam->eEncTools |= AL_OPT_CONST_INTRA_PRED;

  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE)
  {
    AL_Assert(AL_IS_HEVC(pChParam->eProfile));
    pSettings->uEnableSEI |= AL_SEI_PT;
  }

  if(pSettings->bEnableFirstPassSceneChangeDetection)
  {
    uint32_t uRatio = pChParam->uEncWidth * 1000 / pChParam->uEncHeight;
    uint32_t uVal = pChParam->uEncWidth * pChParam->uEncHeight / 50;

    if(uRatio > 2500 || uRatio < 400 || uVal < 18000 || uVal > 170000)
    {
      MSG_WARNING("SCDFirstPass mode is not supported with the current resolution, it'll be disabled");
      pSettings->bEnableFirstPassSceneChangeDetection = false;
      ++numIncoherency;
    }
  }

  if(pChParam->tRCParam.eRCMode == AL_RC_CONST_QP && pChParam->tRCParam.iInitialQP < 0)
  {
    pChParam->tRCParam.iInitialQP = 30;

  }

  // Fill zero value with surrounding non-zero value if any. Warning: don't change the order of if statements below
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

