/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

/**************************************************************************//*!
   \addtogroup qp_table Quantization Parameter Table
   @{
   \file
******************************************************************************/
#pragma once
#include "lib_common/PicFormat.h"
#include "lib_common/Profiles.h"

#define AL_QPTABLE_NUM_SEGMENTS 8
#define AL_QPTABLE_SEGMENTS_SIZE (AL_QPTABLE_NUM_SEGMENTS * 2)

#define MASK_QP_NUMBITS_MICRO 6
#define MASK_QP_MICRO ((1 << MASK_QP_NUMBITS_MICRO) - 1)

#define MASK_QP_NUMBITS MASK_QP_NUMBITS_MICRO

#define DEFAULT_LAMBDA_FACT 1 << 5
#define MASK_QP ((1 << MASK_QP_NUMBITS) - 1)
#define MASK_FORCE_INTRA (1 << MASK_QP_NUMBITS)
#define MASK_FORCE_MV0 (1 << (MASK_QP_NUMBITS + 1))
#define MASK_FORCE (MASK_FORCE_INTRA | MASK_FORCE_MV0)

/*@}*/

