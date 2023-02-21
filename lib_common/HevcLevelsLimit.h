// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "LevelLimit.h"

bool AL_HEVC_CheckLevel(int level);
uint32_t AL_HEVC_GetMaxNumberOfSlices(int level);
uint32_t AL_HEVC_GetMaxTileColumns(int level);
uint32_t AL_HEVC_GetMaxTileRows(int level);
uint32_t AL_HEVC_GetMaxCPBSize(int level, int tier);
uint32_t AL_HEVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight, bool bIsIntraProfile, bool bIsStillProfile, bool bDecodeIntraOnly);

uint8_t AL_HEVC_GetLevelFromFrameSize(int numPixPerFrame);
uint8_t AL_HEVC_GetLevelFromPixRate(int pixRate);
uint8_t AL_HEVC_GetLevelFromBitrate(int bitrate, int tier);
uint8_t AL_HEVC_GetLevelFromTileCols(int tileCols);
uint8_t AL_HEVC_GetLevelFromDPBSize(int dpbSize, int pixRate);

