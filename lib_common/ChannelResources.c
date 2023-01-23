/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "ChannelResources.h"
#include "Utils.h"
#include "lib_rtos/lib_rtos.h"

static int divideRoundUp(uint64_t dividende, uint64_t divisor)
{
  return (dividende + divisor - 1) / divisor;
}

static int GetMinCoresCount(int width, int maxWidth)
{
  return divideRoundUp(width, maxWidth);
}

static int GetCoreResources(int coreFrequency, int margin)
{
  return coreFrequency - (coreFrequency / 100) * margin;
}

static int ChoseCoresCount(int width, int height, int frameRate, int clockRatio, int resourcesByCore, int maxWidth, int cycles32x32)
{
  uint64_t channelResources = AL_GetResources(width, height, frameRate, clockRatio, cycles32x32);
  return Max(GetMinCoresCount(width, maxWidth), divideRoundUp(channelResources, resourcesByCore));
}

void AL_CoreConstraint_Init(AL_CoreConstraint* constraint, int coreFrequency, int margin, int const* hardwareCyclesCounts, int minWidth, int maxWidth, int lcuSize)
{
  constraint->minWidth = minWidth;
  constraint->maxWidth = maxWidth;
  constraint->lcuSize = lcuSize;
  constraint->resources = GetCoreResources(coreFrequency, margin);
  constraint->cycles32x32[0] = hardwareCyclesCounts[0];
  constraint->cycles32x32[1] = hardwareCyclesCounts[1];
  constraint->cycles32x32[2] = hardwareCyclesCounts[2];
  constraint->cycles32x32[3] = hardwareCyclesCounts[3];
  constraint->enableMultiCore = true;
}

int AL_CoreConstraint_GetExpectedNumberOfCores(AL_CoreConstraint* constraint, int width, int height, int chromaModeIdc, int frameRate, int clockRatio)
{
  return ChoseCoresCount(width, height, frameRate, clockRatio, constraint->resources, constraint->maxWidth, constraint->cycles32x32[chromaModeIdc]);
}

int AL_CoreConstraint_GetMinCoresCount(AL_CoreConstraint* constraint, int width)
{
  return GetMinCoresCount(width, constraint->maxWidth);
}

static int getLcuCount(int width, int height)
{
  /* Fixed LCU Size chosen for resources calculs */
  int const lcuPicHeight = 32;
  int const lcuPicWidth = 32;
  return divideRoundUp(width, lcuPicWidth) * divideRoundUp(height, lcuPicHeight);
}

uint64_t AL_GetResources(int width, int height, int frameRate, int clockRatio, int cycles32x32)
{
  if(clockRatio == 0)
    return 0;

  uint64_t lcuCount = getLcuCount(width, height);
  uint64_t dividende = lcuCount * (uint64_t)frameRate;
  uint64_t divisor = (uint64_t)clockRatio;
  return divideRoundUp(dividende, divisor) * (uint64_t)cycles32x32;
}

static int ToCtb(int val, int ctbSize)
{
  return divideRoundUp(val, ctbSize);
}

bool AL_Constraint_NumCoreIsSane(AL_ECodec codec, int width, int numCore, int log2MaxCuSize, AL_NumCoreDiagnostic* diagnostic)
{
  (void)codec;
  /*
   * Hardware limitation, for each core, we need at least:
   * -> 3 CTB for VP9 / HEVC64
   * -> 4 CTB for HEVC32
   * -> 5 MB for AVC
   * Each core starts on a tile.
   * Tiles are aligned on 64 bytes.
   * For JPEG, each core works on a different frame.
   */

  int corePerFrame = numCore;

  int ctbSize = 1 << log2MaxCuSize;
  int const MIN_CTB_PER_CORE = 9 - log2MaxCuSize;
  int widthPerCoreInCtb = ToCtb(width / corePerFrame, ctbSize);

  int offset = 0;
  int roundedOffset = 0; // A core needs to starts at a 64 bytes aligned offset

  if(diagnostic)
    Rtos_Memset(diagnostic, 0, sizeof(*diagnostic));

  for(int core = 0; core < corePerFrame; ++core)
  {
    offset = roundedOffset;
    int curCoreMinWidthInCtb = MIN_CTB_PER_CORE * ctbSize;
    offset += curCoreMinWidthInCtb;
    roundedOffset = RoundUp(offset, 64);
  }

  if(diagnostic)
  {
    diagnostic->requiredWidthInCtbPerCore = MIN_CTB_PER_CORE;
    diagnostic->actualWidthInCtbPerCore = widthPerCoreInCtb;
  }

  return widthPerCoreInCtb >= MIN_CTB_PER_CORE && offset <= RoundUp(width, ctbSize);
}

