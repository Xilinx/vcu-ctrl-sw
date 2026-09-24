// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "ChannelResources.h"
#include "Utils.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"
#include "lib_common/Round.h"
#include "lib_common/Arithmetics.h"

static int32_t divideRoundUp(AL_64U dividende, AL_64U divisor)
{
  return (dividende + divisor - 1) / divisor;
}

static int32_t GetMinCoresCount(int32_t width, int32_t maxWidth)
{
  return divideRoundUp(width, maxWidth);
}

int32_t GetCoreResources(int32_t coreFrequency, int32_t margin)
{
  return coreFrequency - (coreFrequency / 100) * margin;
}

static int32_t ChoseCoresCount(int32_t width, int32_t height, int32_t frameRate, int32_t clockRatio, int32_t resourcesByCore, int32_t maxWidth, int32_t cycles32x32)
{
  AL_64U channelResources = AL_GetResources(width, height, frameRate, clockRatio, cycles32x32);
  return Max(GetMinCoresCount(width, maxWidth), divideRoundUp(channelResources, resourcesByCore));
}

void AL_CoreConstraint_Init(AL_CoreConstraint* constraint, int32_t coreFrequency, int32_t margin, uint32_t const* hardwareCyclesCounts, int32_t minWidth, int32_t maxWidth, int32_t lcuSize)
{
  constraint->minWidth = minWidth;
  constraint->maxWidth = maxWidth;
  constraint->lcuSize = lcuSize;
  constraint->resources = GetCoreResources(coreFrequency, margin);

  for(int32_t i = 0; i < 4; ++i)
    constraint->cycles32x32[i] = hardwareCyclesCounts[i];

  constraint->enableMultiCore = true;
}

int32_t AL_CoreConstraint_GetExpectedNumberOfCores(AL_CoreConstraint const* constraint, int32_t width, int32_t height, int32_t chromaModeIdc, int32_t frameRate, int32_t clockRatio)
{
  return ChoseCoresCount(width, height, frameRate, clockRatio, constraint->resources, constraint->maxWidth, constraint->cycles32x32[chromaModeIdc]);
}

int32_t AL_CoreConstraint_GetMinCoresCount(AL_CoreConstraint const* constraint, int32_t width)
{
  return GetMinCoresCount(width, constraint->maxWidth);
}

static int32_t getLcuCount(int32_t width, int32_t height)
{
  /* Fixed LCU Size chosen for resources calculus */
  int32_t const lcuPicHeight = 32;
  int32_t const lcuPicWidth = 32;
  return divideRoundUp(width, lcuPicWidth) * divideRoundUp(height, lcuPicHeight);
}

AL_64U AL_GetResources(int32_t width, int32_t height, int32_t frameRate, int32_t clockRatio, int32_t cycles32x32)
{
  if(clockRatio == 0)
    return 0;

  AL_64U lcuCount = getLcuCount(width, height);
  AL_64U dividende = lcuCount * (AL_64U)frameRate;
  AL_64U divisor = (AL_64U)clockRatio;
  return divideRoundUp(dividende, divisor) * (AL_64U)cycles32x32;
}

static int32_t ToCtb(int32_t val, int32_t ctbSize)
{
  return divideRoundUp(val, ctbSize);
}

bool AL_Constraint_NumTileIsSane(AL_ECodec codec, int32_t width, int32_t numTile, int32_t log2MaxCuSize, AL_NumCoreDiagnostic* diagnostic)
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

  int32_t tilePerFrame = numTile;
  static int32_t const LOG2_MIN_ENC_WIDTH = 9;

  int32_t ctbSize = 1 << log2MaxCuSize;
  int32_t min_ctb_per_tile = LOG2_MIN_ENC_WIDTH - log2MaxCuSize;
  int32_t widthPerTileInCtb = ToCtb(width / tilePerFrame, ctbSize);

  int32_t offset = 0;
  int32_t roundedOffset = 0; // A core needs to starts at a 64 bytes aligned offset

  if(diagnostic)
    Rtos_Memset(diagnostic, 0, sizeof(*diagnostic));

  for(int32_t tile = 0; tile < tilePerFrame; ++tile)
  {
    offset = roundedOffset;
    int32_t curTileMinWidthInCtb = min_ctb_per_tile * ctbSize;
    offset += curTileMinWidthInCtb;
    roundedOffset = AL_RoundUp(offset, 64);
  }

  if(diagnostic)
  {
    diagnostic->requiredWidthInCtbPerCore = min_ctb_per_tile;
    diagnostic->actualWidthInCtbPerCore = widthPerTileInCtb;
  }

  return widthPerTileInCtb >= min_ctb_per_tile && offset <= AL_RoundUp(width, ctbSize);
}
