/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

/*************************************************************************//*!
   \addtogroup Traces
   @{
   \file
*****************************************************************************/
#pragma once

/*************************************************************************//*!
   \brief Trace type enum
*****************************************************************************/
typedef enum e_TraceType
{
  PARSING_INPUT_TRACE = 0,
  DECODING_INPUT_TRACE = 1,
  PARSING_OUTPUT_TRACE = 2,
  DECODING_OUTPUT_TRACE = 3,
}AL_ETraceType;

/*************************************************************************//*!
   \brief Output trace mode
*****************************************************************************/
typedef enum e_TraceMode
{
  AL_TRACE_NONE = 0,
  AL_TRACE_ON_ERR = 1,
  AL_TRACE_LATEST = 2,
  AL_TRACE_ALL = 3,
  AL_TRACE_FRAME = 4,
  AL_TRACE_STATUS = 5,
}AL_ETraceMode;

/*@}*/

