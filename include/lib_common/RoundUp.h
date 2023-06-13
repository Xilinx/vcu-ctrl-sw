// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "PicFormat.h"

static inline int AL_RoundUp(int iVal, int iRnd)
{
  return iVal >= 0 ? ((iVal + iRnd - 1) / iRnd) * iRnd : (iVal / iRnd) * iRnd;
}

static inline int AL_RoundUpAndMul(uint32_t iVal, uint32_t iRound, uint8_t iMul)
{
  return AL_RoundUp(iVal, iRound) * iMul;
}

static inline int AL_RoundUpAndDivide(uint32_t iVal, uint32_t iRound, uint16_t iDiv)
{
  return AL_RoundUp(iVal, iRound) / iDiv;
}

static inline bool AL_AreDimensionsEqual(AL_TDimension tDim1, AL_TDimension tDim2)
{
  return tDim1.iWidth == tDim2.iWidth && tDim1.iHeight == tDim2.iHeight;
}

static inline uint32_t AL_UnsignedRoundUp(uint32_t uVal, int iRnd)
{
  return ((uVal + iRnd - 1) / iRnd) * iRnd;
}

static inline AL_PADDR AL_PhysAddrRoundUp(AL_PADDR uVal, int iRnd)
{
  return ((uVal + iRnd - 1) / iRnd) * iRnd;
}
