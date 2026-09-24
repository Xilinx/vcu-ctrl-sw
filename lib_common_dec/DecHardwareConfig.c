// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecHardwareConfig.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Arithmetics.h"
#include "lib_common_dec/Scheduler_Defs.h"

/*****************************************************************************/
AL_EChromaMode AL_HWConfig_Dec_GetSupportedChromaMode(void)
{
  return AL_CHROMA_4_2_2;
}

/*****************************************************************************/
int32_t AL_HWConfig_Dec_GetSupportedBitDepth(void)
{
  return 10;
}

/*****************************************************************************/
AL_ERR AL_HWConfig_Dec_CheckBitDepth(int iBitDepth)
{
  if(iBitDepth > AL_HWConfig_Dec_GetSupportedBitDepth())
  {
    Rtos_Log(AL_LOG_ERROR, "Bitdepth '%i' is not supported by the HARDWARE. Maximum supported bitdepth is '%i'.\n", iBitDepth, AL_HWConfig_Dec_GetSupportedBitDepth());
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
AL_ERR AL_HWConfig_Dec_CheckChromaMode(AL_EChromaMode eChromaMode)
{
  if(eChromaMode > AL_HWConfig_Dec_GetSupportedChromaMode())
  {
    Rtos_Log(AL_LOG_ERROR, "Chromamode '%i' is not supported by the HARDWARE. Maximum chromamode supported is '%i'.\n", eChromaMode, AL_HWConfig_Dec_GetSupportedChromaMode());
    return AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
AL_ERR AL_HWConfig_Dec_CheckFrameResolution(AL_TDimension tFrameDim, uint8_t uLog2Ctu, uint8_t uLog2MinCb)
{
  int32_t iMinResolution = ((AL_CORE_MIN_CU_NB - 1) << uLog2Ctu) + (1 << uLog2MinCb);

  if(tFrameDim.iWidth < iMinResolution)
  {
    Rtos_Log(AL_LOG_ERROR, "Width '%i' is not supported by the HARDWARE. Minimum supported width is '%i'.\n", tFrameDim.iWidth, iMinResolution);
    return AL_WARN_SPS_MIN_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(tFrameDim.iHeight < iMinResolution)
  {
    Rtos_Log(AL_LOG_ERROR, "Height '%i' is not supported by the HARDWARE. Minimum supported height is '%i'.\n", tFrameDim.iHeight, iMinResolution);
    return AL_WARN_SPS_MIN_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
AL_ERR AL_HWConfig_Dec_CheckMaxFrameResolution(AL_TDimension tFrameDim, AL_TDimension tMaxDim)
{
  if(tFrameDim.iWidth > tMaxDim.iWidth)
  {
    Rtos_Log(AL_LOG_ERROR, "Width '%i' is not supported by the HARDWARE. Maximum supported width is '%i'.\n", tFrameDim.iWidth, tMaxDim.iWidth);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(tFrameDim.iHeight > tMaxDim.iHeight)
  {
    Rtos_Log(AL_LOG_ERROR, "Height '%i' is not supported by the HARDWARE. Maximum supported height is '%i'.\n", tFrameDim.iHeight, tMaxDim.iHeight);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  return AL_SUCCESS;
}
