// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "ConvSrc.h"

struct CYuvSrcConv : public IConvSrc
{
  explicit CYuvSrcConv(TFrameInfo const& FrameInfo);
  virtual ~CYuvSrcConv() override = default;

  virtual void ConvertSrcBuf(uint8_t uBitDepth, AL_TBuffer const* pSrcIn, AL_TBuffer* pSrcOut) override;

protected:
  TFrameInfo const m_FrameInfo;
};
