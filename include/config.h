/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/


/////////////////////// IP CONFIGURATION ////////////////////////
#define AL_UID_ENCODER 0x00019BB3
#define AL_ENC_NUM_CORES 4
#define AL_DEC_NUM_CORES 2

#define HW_IP_BIT_DEPTH 10
#define AL_CORE_MAX_WIDTH 4096
#define AL_L2P_MAX_SIZE (26 * 40 * 4096)
#define AL_ALIGN_HEIGHT 64
#define AL_ALIGN_PITCH 256

#define AL_BLK16x16_QP_TABLE 0

#define AL_SCHEDULER_CORE_FREQUENCY 666666666 // ~667 MHz
#define AL_SCHEDULER_FREQUENCY_MARGIN 10
#define AL_SCHEDULER_BLK_32x32_CYCLE 4900
#define AL_MAX_RESSOURCE ((AL_SCHEDULER_CORE_FREQUENCY - ((AL_SCHEDULER_CORE_FREQUENCY / 100) * AL_SCHEDULER_FREQUENCY_MARGIN)) / AL_SCHEDULER_BLK_32x32_CYCLE)

#include "build_defs_common.h"

#define AVC_MAX_HORIZONTAL_RANGE_B 8
#define AVC_MAX_HORIZONTAL_RANGE_P 16
#define AVC_MAX_VERTICAL_RANGE_B 8
#define AVC_MAX_VERTICAL_RANGE_P 16
#define HEVC_MAX_HORIZONTAL_RANGE_B 16
#define HEVC_MAX_HORIZONTAL_RANGE_P 32
#define HEVC_MAX_VERTICAL_RANGE_B 16
#define HEVC_MAX_VERTICAL_RANGE_P 32
