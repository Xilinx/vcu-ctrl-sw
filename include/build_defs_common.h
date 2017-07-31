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

#pragma once
#define AL_ENABLE_INTERRUPT 1
#define ENABLE_WATCHDOG 1

#ifndef ENABLE_TRACES
#define ENABLE_TRACES 1
#endif


#ifndef AL_MAX_ENC_SLICE
#define AL_MAX_ENC_SLICE 200
#endif

#define ENABLE_RTOS_SYNC 1

/* Fixed LCU Size chosen for resources calculs */
#define AL_LCU_BASIC_WIDTH 32
#define AL_LCU_BASIC_HEIGHT 32

#define AL_ALIGN_FRM_BUF 256

////////////////////////// DEBUG TOOLS ///////////////////////////
#define DEBUG_PATH "."

#ifndef AL_ENABLE_SITE
#define AL_ENABLE_SITE 1
#endif

/* Choose default raster format packing option if not defined in client config */
#ifndef AL_DEC_RASTER_3x10B_ON_32B
#define AL_DEC_RASTER_3x10B_ON_32B 1
#endif

