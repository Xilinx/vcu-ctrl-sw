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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common_enc/EncChanParam.h"

/*************************************************************************//*!
   \brief The L2 cache constant parameters
*****************************************************************************/
#define L2P_W 64 // L2 prefetch Block Width
#define L2P_H 8  // L2 prefetch Block Height
#define ME_HMIN_AVC 8  // Motion estimation minimum range in AVC profile
#define ME_HMIN_HEVC 16 // Motion estimation minimum range in HEVC profile
#define L2P_WPP_CORE_GAP 4  // Minimum number of Ctb between 2 cores in Wavefront
#define L2P_REFILL_MARGIN 8  // Min number of L2 prefetch block (L2P_W x L2P_H)
#define L2P_MAX_BLOCK 4096

#define ALIGN_UP(Val, Step) (((Val) + (Step) - 1) & ~((Step) - 1))
#define ALIGN_DOWN(Val, Step) ((Val) & ~((Step) - 1))

/*****************************************************************************/
void AL_L2P_GetL2PrefetchMaxRange(AL_TEncChanParam const* pChParam, AL_ESliceType eSliceType, uint8_t uNumCore, uint16_t* pHorzRange, uint16_t* pVertRange);

int32_t AL_L2P_GetL2PrefetchMinSize(AL_TEncChanParam const* pChParam, uint8_t uNumCore);
int32_t AL_L2P_GetL2PrefetchMaxSize(AL_TEncChanParam const* pChParam, uint8_t uNumCore);

/*@}*/

