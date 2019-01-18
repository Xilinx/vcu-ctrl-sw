/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
   \addtogroup Buffers
   @{
   \file
 **************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Stream section. Act as a kind of scatter gather list containing the
   stream parts inside a buffer.
*****************************************************************************/
typedef struct
{
  uint32_t uOffset; /*!< Start offset of the section (in bytes from the begining of the buffer) */
  uint32_t uLength; /*!< Length in bytes of the section */
  uint32_t uFlags; /*!< flags associated with the section; see macro SECTION_xxxxx_FLAG */
}AL_TStreamSection;

typedef enum
{
  SECTION_SEI_PREFIX_FLAG = 0x80000000, /*< this section data is from a SEI prefix */
  SECTION_SYNC_FLAG = 0x40000000, /*< this section data is from an IDR */
  SECTION_END_FRAME_FLAG = 0x20000000, /*< this section denotes the end of a frame */
  SECTION_CONFIG_FLAG = 0x10000000 /*< section data is an sps, pps, vps, aud */
}AL_SectionFlags;

/*@}*/

