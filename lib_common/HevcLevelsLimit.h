/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

