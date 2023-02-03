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
  int actualWidthInCtbPerCore; /* calculated without taking the alignement of the cores in consideration */
}AL_NumCoreDiagnostic;

bool AL_Constraint_NumCoreIsSane(AL_ECodec codec, int width, int numCore, int log2MaxCuSize, AL_NumCoreDiagnostic* diagnostic);

