// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/Index.h"
#include "lib_common_dec/DecBuffersInternal.h"

/*****************************************************************************
   \brief Slice Parameters : Mimics structure for IP registers
*****************************************************************************/
typedef struct AL_TDecSliceParam
{
  uint8_t uMaxMergeCand;
  uint8_t uCabacInitIdc;
  bool bColocFromL0;
  bool bMvdL1ZeroFlag;
  uint16_t uSliceId;
  uint8_t uNumRefIdxL0Minus1;
  uint8_t uNumRefIdxL1Minus1;
  bool bWeightedPred;
  bool bWeightedBiPred;
  bool bValidConceal;
  uint8_t uSliceHeaderLength;
  bool bTileNgbA;
  bool bTileNgbB;
  bool bTileNgbC;
  bool bTileNgbD;
  bool bTileNgbE;
  bool bTileNgbH;
  bool bTileNgbI;
  uint16_t uNumEntryPoint;
  uint8_t pPicIdL0s[AL_MAX_REF];
  uint8_t pPicIdL1s[AL_MAX_REF];
  AL_TIndex tColocPicID;
  AL_TIndex tConcealPicID;

  int8_t iCbQpOffset;
  int8_t iCrQpOffset;
  int16_t iSliceQq;
  int8_t iTcOffsetDiv2;
  int8_t iBetaOffsetDiv2;

  uint16_t uTileWidth;
  uint16_t uTileHeight;
  uint16_t uFirstTileLcu;
  uint16_t uFirstLcuTileId;
  uint16_t uLcuTileWidth;
  uint16_t uLcuTileHeight;

  uint32_t uSliceFirstLcu;
  uint32_t uSliceNumLcu;

  uint32_t uNextSliceSegment;
  uint32_t uFirstLcuSliceSegment;
  uint32_t uFirstLcuSlice;

  union
  {
    bool bDirectSpatial;
    bool bTemporalMvp;
  };
  bool bIsLastSlice;
  bool bDependentSlice;
  bool bSaoFilterLuma;
  bool bSaoFilterChroma;
  bool bDisableLoopFilter;
  bool bAcrossSliceLoopFilter;
  bool bCuChromaQpOffset;
  bool bNextIsDependent;
  bool bNextIsAnotherTile;
  bool bTile;
  bool bIsRef;

  AL_ESliceType eSliceType;
  uint32_t uStrAvailSize;
  uint32_t uCompOffset;
  uint32_t uStrOffset;
  uint32_t uParsingId;

  /* Keep this at last position of structure since it allows to copy only
   * necessary entry_point_offset content.
   */
  uint32_t pEntryPointOffsets[AL_MAX_ENTRY_POINT + 1];

}AL_TDecSliceParam;
