// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

static inline AL_64S AL_RoundUp(AL_64S iVal, AL_64S iRnd)
{
  return iVal >= 0 ? ((iVal + iRnd - 1) / iRnd) * iRnd : (iVal / iRnd) * iRnd;
}

static inline AL_64S AL_RoundDown(AL_64S iVal, AL_64S iRnd)
{
  return iVal >= 0 ? (iVal / iRnd) * iRnd : ((iVal - iRnd + 1) / iRnd) * iRnd;
}

static inline AL_64S AL_RoundUpAndMul(AL_64S iVal, AL_64S iRound, AL_64S iMul)
{
  return AL_RoundUp(iVal, iRound) * iMul;
}

static inline AL_64S AL_RoundUpAndDivide(AL_64S iVal, AL_64S iRound, AL_64S iDiv)
{
  return AL_RoundUp(iVal, iRound) / iDiv;
}

static inline AL_64U AL_UnsignedRoundUp(AL_64U zVal, AL_64S iRnd)
{
  return ((zVal + iRnd - 1) / iRnd) * iRnd;
}

static inline AL_64U AL_UnsignedRoundDown(AL_64U zVal, AL_64S iRnd)
{
  return (zVal / iRnd) * iRnd;
}

static inline AL_64U AL_UnsignedRoundUpAndMul(AL_64U zVal, AL_64S iRound, AL_64S iMul)
{
  return AL_UnsignedRoundUp(zVal, iRound) * iMul;
}

static inline AL_64U AL_UnsignedRoundUpAndDivide(AL_64U zVal, AL_64S iRound, AL_64S iDiv)
{
  return AL_UnsignedRoundUp(zVal, iRound) / iDiv;
}

static inline AL_64S AL_Round2AndDivide(AL_64S iVal, AL_64S iRnd)
{
  if(iVal == 0)
    return iVal;
  return (iVal + (iRnd / 2)) / iRnd;
}

static inline AL_64S AL_Round2SignedAndDivide(AL_64S iVal, AL_64S iRnd)
{
  if(iVal >= 0)
    return AL_Round2AndDivide(iVal, iRnd);
  return -AL_Round2AndDivide(-iVal, iRnd);
}

static inline AL_PADDR AL_PhysAddrRoundUp(AL_PADDR uVal, AL_64S iRnd)
{
  return ((uVal + iRnd - 1) / iRnd) * iRnd;
}

static inline AL_64U AL_UnsignedCeilDiv(AL_64U iVal, AL_64U iDiv)
{
  return (iVal + iDiv - 1) / iDiv;
}
