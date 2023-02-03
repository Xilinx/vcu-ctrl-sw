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
#include "lib_common/Profiles.h"

bool AL_AVC_CheckLevel(int level);
uint32_t AL_AVC_GetMaxNumberOfSlices(AL_EProfile profile, int level, int numUnitInTicks, int timeScale, int numMbsInPic);
uint32_t AL_AVC_GetMaxCPBSize(int level);
uint32_t AL_AVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight, int iSpsMaxRef, bool bIntraProfile, bool bDecodeIntraOnly);

uint8_t AL_AVC_GetLevelFromFrameSize(int numMbPerFrame);
uint8_t AL_AVC_GetLevelFromMBRate(int mbRate);
uint8_t AL_AVC_GetLevelFromBitrate(int bitrate);
uint8_t AL_AVC_GetLevelFromDPBSize(int dpbSize);

