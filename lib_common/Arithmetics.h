// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/assert.h"
#include "lib_common/Round.h"

static inline AL_64U BitsToBytes(AL_64U uBits)
{
  return AL_UnsignedRoundUpAndDivide(uBits, 8, 8);
}

/***************************************************************************/
static inline AL_64U BytesToBits(AL_64U uBytes)
{
  return uBytes * 8;
}

/***************************************************************************/
static inline AL_64U UnsignedClip3(AL_64U uVal, AL_64U uMin, AL_64U uMax)
{
  return ((uVal) < (uMin)) ? (uMin) : ((uVal) > (uMax)) ? (uMax) : (uVal);
}

/***************************************************************************/
static inline AL_64S Clip3(AL_64S iVal, AL_64S iMin, AL_64S iMax)
{
  return ((iVal) < (iMin)) ? (iMin) : ((iVal) > (iMax)) ? (iMax) : (iVal);
}

/***************************************************************************/
static inline AL_64S Max(AL_64S iVal1, AL_64S iVal2)
{
  return (iVal1 < iVal2) ? iVal2 : iVal1;
}

/***************************************************************************/
static inline AL_64S Min(AL_64S iVal1, AL_64S iVal2)
{
  return (iVal1 > iVal2) ? iVal2 : iVal1;
}

/***************************************************************************/
static inline AL_64S Abs(AL_64S iVal)
{
  return (iVal > 0) ? iVal : -iVal;
}

/***************************************************************************/
static inline AL_64S Sign(AL_64S iVal)
{
  return (iVal > 0) ? 1 : ((iVal < 0) ? -1 : 0);
}

/***************************************************************************/
static inline AL_64U UnsignedMax(AL_64U uVal1, AL_64U uVal2)
{
  return (uVal1 < uVal2) ? uVal2 : uVal1;
}

/***************************************************************************/
static inline AL_64U UnsignedMin(AL_64U uVal1, AL_64U uVal2)
{
  return (uVal1 > uVal2) ? uVal2 : uVal1;
}

/***************************************************************************/
static AL_64S const tab_ceil_log2[] =
{
/*  0.. 7 */
  0, 0, 1, 2, 2, 3, 3, 3,
/*  8..15 */
  3, 4, 4, 4, 4, 4, 4, 4,
/* 16..23 */
  4, 5, 5, 5, 5, 5, 5, 5,
/* 24..31 */
  5, 5, 5, 5, 5, 5, 5, 5
};

/***************************************************************************/
static inline AL_64S ceil_log2(AL_64S n)
{
  Rtos_Assert(n >= 0);

  if(n < 32)
    return tab_ceil_log2[n];

  AL_64S v = 0;

  n--;

  // count the number of bit used to store the decremented n
  while(n != 0)
  {
    n >>= 1;
    ++v;
  }

  return v;
}

/***************************************************************************/
static inline AL_64S floor_log2(AL_64S n)
{
  AL_64S s = -1;

  while(n != 0)
  {
    n >>= 1;
    ++s;
  }

  return s;
}
