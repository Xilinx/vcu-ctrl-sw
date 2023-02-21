// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/Profiles.h"

bool AL_AVC_CheckLevel(int level);
uint32_t AL_AVC_GetMaxNumberOfSlices(AL_EProfile profile, int level, int numUnitInTicks, int timeScale, int numMbsInPic);
uint32_t AL_AVC_GetMaxCPBSize(int level);
uint32_t AL_AVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight, int iSpsMaxRef, bool bIntraProfile, bool bDecodeIntraOnly);

uint8_t AL_AVC_GetLevelFromFrameSize(int numMbPerFrame);
uint8_t AL_AVC_GetLevelFromMBRate(int mbRate);
uint8_t AL_AVC_GetLevelFromBitrate(int bitrate);
uint8_t AL_AVC_GetLevelFromDPBSize(int dpbSize);

