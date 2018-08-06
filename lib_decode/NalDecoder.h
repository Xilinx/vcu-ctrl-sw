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
#include "DefaultDecoder.h"

typedef struct
{
  AL_ENut seiPrefix;
  AL_ENut seiSuffix;
  AL_ENut sps;
  AL_ENut pps;
  AL_ENut vps;
  AL_ENut eos;
}AL_NonVclNuts;

typedef struct
{
  AL_PARSE_RESULT (* parseSps)(AL_TAup*, AL_TRbspParser*);
  AL_PARSE_RESULT (* parsePps)(AL_TAup*, AL_TRbspParser*, AL_TDecCtx*);
  void (* parseVps)(AL_TAup*, AL_TRbspParser*);
  bool (* parseSei)(AL_TAup*, AL_TRbspParser*);
  void (* decodeSliceData)(AL_TAup*, AL_TDecCtx*, AL_ENut, bool, int*);
  bool (* isSliceData)(AL_ENut nut);
}AL_NalParser;

void AL_DecodeOneNal(AL_NonVclNuts, AL_NalParser, AL_TAup*, AL_TDecCtx*, AL_ENut, bool, int*);

