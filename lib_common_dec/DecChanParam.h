// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/*****************************************************************************
   \addtogroup Decoder
   !@{
   \file
*****************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecSynchro.h"
#include "lib_common_dec/DecOutputSettings.h"

/*****************************************************************************
   \brief Channel Parameter structure
*****************************************************************************/
typedef struct AL_TDecChannelParam
{
  int32_t iWidth; /*< width in pixels */
  int32_t iHeight; /*< height in pixels */
  uint8_t uLog2MaxCuSize;
  uint32_t uFrameRate;
  uint32_t uClkRatio;
  uint32_t uMaxLatency;
  uint8_t uNumCore;
  uint8_t uNumTileColumns;
  bool bForceMultipleDec1PerFrame; /*!< Force using several dec1 per frame (when slice or tile pattern  permits) */
  bool bNonRealtime;
  uint8_t uDDRWidth;
  bool bLowLat;
  bool bParallelWPP;
  bool bDisableCache;
  bool bFrameBufferCompression;
  bool bCallDecodeCallbackAtStart; /*< LLP2: this only makes sense with special support for hw synchro */
  AL_EFbStorageMode eFBStorageMode;
  AL_ECodec eCodec;

  AL_EChromaMode eMaxChromaMode;
  int32_t iMaxSlices;
  int32_t iMaxTiles;
  AL_EDecUnit eDecUnit;
  AL_TDecOutputSettings tOutputSettings;
}AL_TDecChanParam;

/*!@}*/
