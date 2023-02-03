/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

AL_EChromaOrder AL_ChromaModeToChromaOrder(AL_EChromaMode eChromaMode)
{
  return eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR);
}

AL_TPicFormat AL_GetDecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed)
{
  bool b10bPacked = false;
  b10bPacked = AL_FB_RASTER == eStorageMode && 10 == uBitDepth;

  AL_TPicFormat picFormat =
  {
    eChromaMode,
    uBitDepth,
    eStorageMode,
    AL_ChromaModeToChromaOrder(eChromaMode),
    bIsCompressed,
    b10bPacked
  };
  return picFormat;
}

