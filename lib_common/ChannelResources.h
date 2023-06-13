// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/Profiles.h"

typedef struct
{
  int minWidth;
  int maxWidth;
  int lcuSize;
  int resources;
  int cycles32x32[4];
  bool enableMultiCore;
}AL_CoreConstraint;

void AL_CoreConstraint_Init(AL_CoreConstraint* constraint, int coreFrequency, int margin, int const* hardwareCyclesCounts, int minWidth, int maxWidth, int lcuSize);
int AL_CoreConstraint_GetExpectedNumberOfCores(AL_CoreConstraint* constraint, int width, int height, int chromaModeIdc, int frameRate, int clockRatio);
int AL_CoreConstraint_GetMinCoresCount(AL_CoreConstraint* constraint, int width);

uint64_t AL_GetResources(int width, int height, int frameRate, int clockRatio, int cycles32x32);

/* Doesn't support NUMCORE_AUTO, only works on actual number of cores. */
typedef struct
{
  int requiredWidthInCtbPerCore;
  int actualWidthInCtbPerCore; /* calculated without taking the alignment of the cores in consideration */
}AL_NumCoreDiagnostic;

bool AL_Constraint_NumCoreIsSane(AL_ECodec codec, int width, int numCore, int log2MaxCuSize, AL_NumCoreDiagnostic* diagnostic);

