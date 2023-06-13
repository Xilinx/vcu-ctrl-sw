// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_enc/IpEncFourCC.h"

#include "lib_common/FourCC.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/
TFourCC AL_EncGetSrcFourCC(AL_TPicFormat const picFmt)
{
  if(AL_FB_RASTER == picFmt.eStorageMode)
  {
    AL_Assert(picFmt.eChromaMode == AL_CHROMA_MONO || picFmt.eChromaOrder == AL_C_ORDER_SEMIPLANAR ||
              (picFmt.eChromaMode == AL_CHROMA_4_4_4 && picFmt.eChromaOrder == AL_C_ORDER_U_V));
    AL_Assert(picFmt.uBitDepth == 8 || picFmt.b10bPacked);
  }

  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetSrcPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed)
{
  bool b10bPacked = false;
  b10bPacked = AL_FB_RASTER == eStorageMode && 10 == uBitDepth;

  AL_TPicFormat picFormat =
  {
    eChromaMode,
    uBitDepth,
    eStorageMode,
    GetChromaOrder(eChromaMode),
    bIsCompressed,
    b10bPacked
  };
  return picFormat;
}

/****************************************************************************/
TFourCC AL_GetRecFourCC(AL_TPicFormat const picFmt)
{
  AL_Assert(picFmt.eStorageMode == AL_FB_TILE_64x4);
  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetRecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsCompressed)
{
  AL_TPicFormat picFmt =
  {
    eChromaMode,
    uBitDepth,
    AL_FB_TILE_64x4,
    GetChromaOrder(eChromaMode),
    bIsCompressed,
    false
  };
  return picFmt;
}

