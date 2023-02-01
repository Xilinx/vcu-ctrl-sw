/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#include "lib_common_enc/IpEncFourCC.h"

#include "lib_common/FourCC.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/
TFourCC AL_EncGetSrcFourCC(AL_TPicFormat const picFmt)
{
  if(AL_FB_RASTER == picFmt.eStorageMode)
  {
    AL_Assert(picFmt.eChromaMode == AL_CHROMA_MONO || picFmt.eChromaOrder == AL_C_ORDER_SEMIPLANAR ||
              (picFmt.eChromaMode == AL_CHROMA_4_4_4 && picFmt.eChromaOrder == AL_C_ORDER_U_V));
    AL_Assert(picFmt.uBitDepth == 8 || picFmt.b10bPacked);
  }

  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetSrcPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed)
{
  bool b10bPacked = false;
  b10bPacked = AL_FB_RASTER == eStorageMode && 10 == uBitDepth;

  AL_TPicFormat picFormat =
  {
    eChromaMode,
    uBitDepth,
    eStorageMode,
    eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR),
    bIsCompressed,
    b10bPacked
  };
  return picFormat;
}

/****************************************************************************/
TFourCC AL_GetRecFourCC(AL_TPicFormat const picFmt)
{
  AL_Assert(picFmt.eStorageMode == AL_FB_TILE_64x4);
  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetRecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsCompressed)
{
  AL_TPicFormat picFmt =
  {
    eChromaMode,
    uBitDepth,
    AL_FB_TILE_64x4,
    eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR),
    bIsCompressed,
    false
  };
  return picFmt;
}

