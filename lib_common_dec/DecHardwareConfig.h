// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Error.h"

AL_EChromaMode AL_HWConfig_Dec_GetSupportedChromaMode(void);
int32_t AL_HWConfig_Dec_GetSupportedBitDepth(void);

AL_ERR AL_HWConfig_Dec_CheckBitDepth(int iBitDepth);
AL_ERR AL_HWConfig_Dec_CheckChromaMode(AL_EChromaMode eChromaMode);

AL_ERR AL_HWConfig_Dec_CheckFrameResolution(AL_TDimension tFrameDim, uint8_t uLog2Ctu, uint8_t uLog2MinCb);
AL_ERR AL_HWConfig_Dec_CheckMaxFrameResolution(AL_TDimension tFrameDim, AL_TDimension tMaxDim);
