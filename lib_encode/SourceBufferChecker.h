// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/FourCC.h"
#include "lib_common_enc/EncChanParam.h"

typedef struct
{
  AL_TDimension currentDim;
  AL_TDimension maxDim;
  AL_TPicFormat picFmt;
  AL_ESrcMode srcMode;
  TFourCC fourCC;
}AL_TSrcBufferChecker;

void AL_SrcBuffersChecker_Init(AL_TSrcBufferChecker* pCtx, AL_TEncChanParam const* pChParam);
bool AL_SrcBuffersChecker_UpdateResolution(AL_TSrcBufferChecker* pCtx, AL_TDimension tNewDim);
bool AL_SrcBuffersChecker_CanBeUsed(AL_TSrcBufferChecker* pCtx, AL_TBuffer* buffer);

