/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

