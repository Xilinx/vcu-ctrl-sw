// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/AomDefines.h"

/*****************************************************************************/
#define AOM_BLOCK_SIZE_GROUPS 4
#define AOM_SKIP_CONTEXTS 3
#define AOM_INTERINTRA_MODES 4
#define AOM_IS_INTER_CONTEXTS 4
#define AOM_COMP_INTER_CONTEXTS 5
#define AOM_MV_CLASSES 11
#define AOM_MV_JOINTS 4
#define AOM_SINGLE_REFS 7
#define AOM_PRIMARY_REF_NONE 7
#define AOM_CLASS0_SIZE 2
#define AOM_SELECT_INTEGER_MV 2
#define AOM_SELECT_SCREEN_CONTENT_TOOLS 2
#define AOM_NUM_REF_FRAMES 8
#define AOM_TOTAL_REFS_PER_FRAME 8
#define AOM_SUPERRES_NUM 8
#define AOM_SUPERRES_DENOM_MIN 9
#define AOM_SUPERRES_DENOM_BITS 3
#define AOM_SUPERRES_SCALE_BITS 14
#define AOM_SUPERRES_EXTRA_BITS 8
#define SUPERRES_SCALE_MASK ((1 << AOM_SUPERRES_SCALE_BITS) - 1)

#define AOM_MAX_TILE_WIDTH 4096
#define AOM_MAX_TILE_AREA (4096 * 2304)
#define AOM_MAX_TILE_ROWS 64
#define AOM_MAX_TILE_COLS 64
#define AOM_MAX_SEGMENTS 8
#define AL_AOM_NUM_PLANES 3
#define AL_AOM_LF_NUM_LEVEL 4
#define AL_AOM_LF_NUM_MODE_DELTA_FRAME 2

/*****************************************************************************/
#define AL_TOrderHint int32_t

/*****************************************************************************/
typedef enum AL_EAomFrameType
{
  AOM_KEY_FRAME = 0,
  AOM_INTER_FRAME = 1,
  AOM_INTRA_ONLY_FRAME = 2,
  AOM_SWITCH_FRAME = 3,
}AL_EAomFrameType;

/*****************************************************************************/
typedef enum AL_EAomTxMode
{
  AL_AOM_ONLY_4X4 = 0,
  AL_AOM_TX_MODE_LARGEST = 1,
  AL_AOM_TX_MODE_SELECT = 2,
}AL_EAomTxMode;
