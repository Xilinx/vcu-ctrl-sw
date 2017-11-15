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

#include "lib_common/FourCC.h"
#include "lib_common_enc/IpEncFourCC.h"
#include <assert.h>

/****************************************************************************/
static TFourCC Get64x4FourCC(AL_TPicFormat const picFmt)
{
  bool const bTenBpc = picFmt.uBitDepth > 8;
  switch(picFmt.eChromaMode)
  {
  case CHROMA_4_2_2: return bTenBpc ? FOURCC(T62A) : FOURCC(T628);
  case CHROMA_4_2_0: return bTenBpc ? FOURCC(T60A) : FOURCC(T608);
  case CHROMA_MONO: return bTenBpc ? FOURCC(T6mA) : FOURCC(T6m8);
  default: assert(0);
    return -1;
  }
}

/****************************************************************************/
static TFourCC Get32x4FourCC(AL_TPicFormat const picFmt)
{
  bool const bTenBpc = picFmt.uBitDepth > 8;
  switch(picFmt.eChromaMode)
  {
  case CHROMA_4_2_2: return bTenBpc ? FOURCC(T52A) : FOURCC(T528);
  case CHROMA_4_2_0: return bTenBpc ? FOURCC(T50A) : FOURCC(T508);
  case CHROMA_MONO: return bTenBpc ? FOURCC(T5mA) : FOURCC(T5m8);
  default: assert(0);
    return -1;
  }
}

/****************************************************************************/
TFourCC AL_EncGetSrcFourCC(AL_TPicFormat const picFmt)
{
  switch(picFmt.eStorageMode)
  {
  case AL_FB_TILE_64x4:
    return Get64x4FourCC(picFmt);
  case AL_FB_TILE_32x4:
    return Get32x4FourCC(picFmt);
  case AL_FB_RASTER:
  default:
    return AL_GetSrcFourCC(picFmt);
  }
}

/****************************************************************************/
TFourCC AL_GetRecFourCC(AL_TPicFormat const picFmt)
{
  assert(picFmt.eStorageMode == AL_FB_TILE_64x4);
  return Get64x4FourCC(picFmt);
}

