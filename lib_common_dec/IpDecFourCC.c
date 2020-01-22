/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "lib_common_dec/IpDecFourCC.h"
#include "lib_assert/al_assert.h"

TFourCC AL_GetDecFourCC(AL_TPicFormat const picFmt)
{
  if(AL_FB_RASTER == picFmt.eStorageMode)
  {
    AL_Assert(picFmt.eChromaMode == AL_CHROMA_MONO || picFmt.eChromaMode == AL_CHROMA_4_4_4 || picFmt.eChromaOrder == AL_C_ORDER_SEMIPLANAR);
    AL_Assert(picFmt.uBitDepth == 8 || picFmt.b10bPacked);
  }

  return AL_GetFourCC(picFmt);
}

AL_TPicFormat AL_GetDecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed)
{
  bool b10bPacked = false;
  b10bPacked = AL_FB_RASTER == eStorageMode && 10 == uBitDepth;

  AL_TPicFormat picFormat = { eChromaMode, uBitDepth, eStorageMode, eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : AL_C_ORDER_SEMIPLANAR, bIsCompressed, b10bPacked };

  if(eChromaMode == AL_CHROMA_4_4_4)
    picFormat.eChromaOrder = AL_C_ORDER_U_V;
  return picFormat;
}

