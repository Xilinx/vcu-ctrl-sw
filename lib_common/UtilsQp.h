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

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"

/****************************************************************************/
static const uint8_t AL_LAMBDAs_AUTO_QP[52] =
{ // Inter : Table containing pow(2,QP/6-2).
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 3, 3, 3, 3, 3, 4,
  4, 4, 5, 5, 6, 7, 9, 12, 16, 21,
  27, 34
};

static const uint8_t DEFAULT_QP_CTRL_TABLE[20] =
{
  0xc4, 0x09, 0xe8, 0x03, 0x00, 0x01, 0x88, 0x13, 0x58, 0x1b, 0x28, 0x23,
  0x01, 0x03, 0x0A, 0x01, 0x02, 0x03, 0x06, 0x33
};

static const uint8_t DEFAULT_QP_CTRL_TABLE_VP9[20] =
{
  0xDC, 0x05, 0xF4, 0x01, 0x00, 0x00, 0x58, 0x1B, 0x28, 0x23, 0xFF, 0xFF,
  0x05, 0x0A, 0x00, 0x05, 0x0A, 0x00, 0x00, 0xFF
};

static const uint8_t DEFAULT_QP_CTRL_TABLE_VP9_2[20] =
{
  0xc4, 0x09, 0xe8, 0x03, 0x00, 0x01, 0x88, 0x13, 0x58, 0x1b, 0x28, 0x23,
  0x05, 0x0F, 0x32, 0x05, 0x0A, 0x0F, 0x00, 0xFF
};

/***************************************************************************/
void AL_UpdateAutoQpCtrl(uint8_t* pQpCtrl, int iSumCplx, int iNumPUs, int iSliceQP, bool bUseDefault, bool bVP9);

