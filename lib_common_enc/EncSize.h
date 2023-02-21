// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"

#define RES_SIZE_16x16_AVC 832/*!< residuals size of a 16x16 LCU */
#define RES_SIZE_16x16_HEVC 768/*!< residuals size of a 16x16 LCU */
#define SIZE_LCU_INFO 16 /*!< LCU compressed size + LCU offset */

int AL_GetCompDataSize(uint32_t uNumLCU, uint8_t uLog2MaxCUSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt);

#define AL_MAX_SUPPORTED_LCU_SIZE 6
#define AL_MIN_SUPPORTED_LCU_SIZE 3

#define AL_MAX_FIXED_SLICE_HEADER_SIZE 32
#define QP_CTRL_TABLE_SIZE 48

#define AL_MAX_STREAM_BUFFER (AL_MAX_SLICES_SUBFRAME * 10)
#define AL_MAX_LOOK_AHEAD 40
#define AL_MAX_SOURCE_BUFFER (AL_MAX_NUM_B_PICT * 2 + AL_MAX_LOOK_AHEAD + 1)

