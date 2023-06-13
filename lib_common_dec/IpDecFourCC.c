// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/IpDecFourCC.h"
#include "lib_assert/al_assert.h"

TFourCC AL_GetDecFourCC(AL_TPicFormat const picFmt)
{
  if(AL_FB_RASTER == picFmt.eStorageMode)
  {
    AL_Assert(picFmt.eChromaMode == AL_CHROMA_MONO || picFmt.eChromaMode == AL_CHROMA_4_4_4 || picFmt.eChromaOrder == AL_C_ORDER_SEMIPLANAR);
    AL_Assert(picFmt.uBitDepth == 8 || picFmt.b10bPacked);
  }

  return AL_GetFourCC(picFmt);
}

AL_EChromaOrder AL_ChromaModeToChromaOrder(AL_EChromaMode eChromaMode)
{
  return GetChromaOrder(eChromaMode);
}

AL_TPicFormat AL_GetDecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed)
{
  bool b10bPacked = false;
  b10bPacked = AL_FB_RASTER == eStorageMode && 10 == uBitDepth;

  AL_TPicFormat picFormat =
  {
    eChromaMode,
    uBitDepth,
    eStorageMode,
    AL_ChromaModeToChromaOrder(eChromaMode),
    bIsCompressed,
    b10bPacked
  };
  return picFormat;
}

