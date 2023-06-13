// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

extern "C"
{
#include "lib_common/BufferAPI.h"
#include "lib_common/PicFormat.h"
}

struct IConvSrc
{
  virtual ~IConvSrc() = default;

  virtual void ConvertSrcBuf(uint8_t uBitDepth, AL_TBuffer const* pSrcIn, AL_TBuffer* pSrcOut) = 0;
};

struct TFrameInfo
{
  AL_TDimension tDimension;
  uint8_t iBitDepth;
  AL_EChromaMode eCMode;
};
