// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Allocator.h"

static int32_t const NUMCORE_AUTO = 0;
static int32_t const MAX_BIT_DEPTH_MINUS_8 = 4;
static int32_t const MAX_POC_LSB_MINUS_4 = 12;
static int32_t const AL_INVALID_POC = -1;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (int32_t)(sizeof(x) / sizeof((x)[0]))
#endif
#define IS_NODE_VALID(tNodeID) (AL_IS_VALID_INDEX(tNodeID))

/****************************************************************************/
int32_t GetBlkNumber(AL_TDimension tDim, uint32_t uBlkWidth, uint32_t uBlkHeight);

/****************************************************************************/
int32_t GetSquareBlkNumber(AL_TDimension tDim, uint32_t uBlkSize);

/****************************************************************************/
AL_HANDLE AlignedAlloc(AL_TAllocator* pAllocator, const char* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset);

/*****************************************************************************
   \brief Reference picture status
 ***************************************************************************/
typedef enum
{
  SHORT_TERM_REF,
  LONG_TERM_REF,
  UNUSED_FOR_REF,
  NON_EXISTING_REF,
  AL_MARKING_REF_MAX_ENUM, /* sentinel */
}AL_EMarkingRef;
