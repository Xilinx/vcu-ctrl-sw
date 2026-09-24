// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/Profiles.h"
#include "lib_rtos/types.h"

uint32_t AL_AVC_GetSpecificationMaxNumberOfSlices(int32_t level);
uint32_t AL_AVC_GetMaxNumberOfSlices(AL_EProfile profile, int32_t level, int32_t numUnitInTicks, int32_t timeScale, int32_t numMbsInPic);
uint32_t AL_AVC_GetMaxCPBSize(int32_t level);
uint32_t AL_AVC_GetMaxDPBSize(int32_t iLevel, int32_t iWidth, int32_t iHeight, int32_t iSpsMaxRef, bool bIntraProfile, bool bDecodeIntraOnly);
uint16_t AL_AVC_GetMaxMotionVectorWidth(int32_t iLevel);
uint16_t AL_AVC_GetMaxMotionVectorHeight(int32_t iLevel);

uint8_t AL_AVC_GetLevelFromFrameSize(int32_t numMbPerFrame);
uint8_t AL_AVC_GetLevelFromMBRate(int32_t mbRate);
uint8_t AL_AVC_GetLevelFromBitrate(int32_t bitrate);
uint8_t AL_AVC_GetLevelFromDPBSize(int32_t dpbSize);
