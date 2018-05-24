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
#include <assert.h>

/* FOURCC from byte array */
#define FOURCC2(A) ((TFourCC)(((uint32_t)((A)[0])) \
                              | ((uint32_t)((A)[1]) << 8) \
                              | ((uint32_t)((A)[2]) << 16) \
                              | ((uint32_t)((A)[3]) << 24)))

/****************************************************************************/
TFourCC AL_GetSrcFourCC(AL_TPicFormat const picFmt)
{
  if(picFmt.uBitDepth > 8)
  {
    switch(picFmt.eChromaMode)
    {
    case CHROMA_4_2_0: return FOURCC(XV15);
    case CHROMA_4_2_2: return FOURCC(XV20);
    case CHROMA_MONO: return FOURCC(XV10);

    default: assert(0);
    }
  }
  else
  {
    switch(picFmt.eChromaMode)
    {
    case CHROMA_4_2_0: return FOURCC(NV12);
    case CHROMA_4_2_2: return FOURCC(NV16);
    case CHROMA_MONO: return FOURCC(Y800);
    default: assert(0);
    }
  }
  return 0;
}

/****************************************************************************/
static char getTileOrCompressedChar(AL_TPicFormat const picFmt)
{
  (void)picFmt;
  return 'T';
}

/****************************************************************************/
static char getChromaChar(AL_TPicFormat const picFmt)
{
  switch(picFmt.eChromaMode)
  {
  case CHROMA_4_2_2: return '2';
  case CHROMA_4_2_0: return '0';
  case CHROMA_MONO: return 'm';
  default: assert(0);
    return -1;
  }
}

/****************************************************************************/
TFourCC GetTiledFourCC(AL_TPicFormat const picFmt)
{
  char pFourCC[4];

  pFourCC[0] = getTileOrCompressedChar(picFmt);
  pFourCC[1] = picFmt.eStorageMode == AL_FB_TILE_64x4 ? '6' : '5';
  pFourCC[2] = getChromaChar(picFmt);
  pFourCC[3] = picFmt.uBitDepth > 8 ? 'A' : '8';

  return FOURCC2(pFourCC);
}

/****************************************************************************/
AL_EChromaMode AL_GetChromaMode(TFourCC tFourCC)
{
  if((tFourCC == FOURCC(I420)) || (tFourCC == FOURCC(IYUV))
     || (tFourCC == FOURCC(YV12)) || (tFourCC == FOURCC(NV12))
     || (tFourCC == FOURCC(I0AL)) || (tFourCC == FOURCC(P010))
     || (tFourCC == FOURCC(T608)) || (tFourCC == FOURCC(T60A))
     || (tFourCC == FOURCC(T508)) || (tFourCC == FOURCC(T50A))
     || (tFourCC == FOURCC(XV15))
     )
    return CHROMA_4_2_0;
  else if((tFourCC == FOURCC(YV16)) || (tFourCC == FOURCC(NV16))
          || (tFourCC == FOURCC(I422)) || (tFourCC == FOURCC(P210))
          || (tFourCC == FOURCC(I2AL))
          || (tFourCC == FOURCC(T628)) || (tFourCC == FOURCC(T62A))
          || (tFourCC == FOURCC(T528)) || (tFourCC == FOURCC(T52A))
          || (tFourCC == FOURCC(XV20))
          )
    return CHROMA_4_2_2;
  else if((tFourCC == FOURCC(Y800)) || (tFourCC == FOURCC(Y010))
          || (tFourCC == FOURCC(T6m8)) || (tFourCC == FOURCC(T6mA))
          || (tFourCC == FOURCC(T5m8)) || (tFourCC == FOURCC(T5mA))
          || (tFourCC == FOURCC(XV10))
          )
    return CHROMA_MONO;
  else
    assert(0);
  return (AL_EChromaMode) - 1;
}

/****************************************************************************/
uint8_t AL_GetBitDepth(TFourCC tFourCC)
{
  if((tFourCC == FOURCC(I420)) || (tFourCC == FOURCC(IYUV))
     || (tFourCC == FOURCC(YV12)) || (tFourCC == FOURCC(NV12))
     || (tFourCC == FOURCC(I422))
     || (tFourCC == FOURCC(YV16)) || (tFourCC == FOURCC(NV16))
     || (tFourCC == FOURCC(Y800))
     || (tFourCC == FOURCC(T6m8))
     || (tFourCC == FOURCC(T608)) || (tFourCC == FOURCC(T628))
     || (tFourCC == FOURCC(T5m8))
     || (tFourCC == FOURCC(T508)) || (tFourCC == FOURCC(T528))
     )
    return 8;
  else if((tFourCC == FOURCC(I0AL)) || (tFourCC == FOURCC(P010))
          || (tFourCC == FOURCC(I2AL)) || (tFourCC == FOURCC(P210))
          || (tFourCC == FOURCC(Y010))
          || (tFourCC == FOURCC(T6mA))
          || (tFourCC == FOURCC(T60A)) || (tFourCC == FOURCC(T62A))
          || (tFourCC == FOURCC(T5mA))
          || (tFourCC == FOURCC(T50A)) || (tFourCC == FOURCC(T52A))
          || (tFourCC == FOURCC(XV15)) || (tFourCC == FOURCC(XV20))
          || (tFourCC == FOURCC(XV10))
          )
    return 10;
  else
    assert(0);
  return -1;
}

/****************************************************************************/
void AL_GetSubsampling(TFourCC fourcc, int* sx, int* sy)
{
  switch(AL_GetChromaMode(fourcc))
  {
  case CHROMA_4_2_0:
    *sx = 2;
    *sy = 2;
    break;
  case CHROMA_4_2_2:
    *sx = 2;
    *sy = 1;
    break;
  default:
    *sx = 1;
    *sy = 1;
    break;
  }
}

/*****************************************************************************/
bool AL_Is10bitPacked(TFourCC tFourCC)
{
  (void)tFourCC;
  return tFourCC == FOURCC(XV15) ||
         tFourCC == FOURCC(XV20) ||
         tFourCC == FOURCC(XV10);
}

/*****************************************************************************/
bool AL_IsSemiPlanar(TFourCC tFourCC)
{
  return tFourCC == FOURCC(NV12) || tFourCC == FOURCC(P010)
         || tFourCC == FOURCC(NV16) || tFourCC == FOURCC(P210)
         || tFourCC == FOURCC(XV15) || tFourCC == FOURCC(XV20)
         || AL_IsTiled(tFourCC)
  ;
}


/*****************************************************************************/
bool AL_IsCompressed(TFourCC tFourCC)
{
  (void)tFourCC;
  bool bIsCompressed = false;
  return bIsCompressed;
}

/*****************************************************************************/
static bool AL_Is64x4Tiled(TFourCC tFourCC)
{
  return tFourCC == FOURCC(T608) ||
         tFourCC == FOURCC(T628) ||
         tFourCC == FOURCC(T6m8) ||
         tFourCC == FOURCC(T60A) ||
         tFourCC == FOURCC(T62A) ||
         tFourCC == FOURCC(T6mA)
  ;
}

/*****************************************************************************/
static bool AL_Is32x4Tiled(TFourCC tFourCC)
{
  return tFourCC == FOURCC(T508) ||
         tFourCC == FOURCC(T528) ||
         tFourCC == FOURCC(T5m8) ||
         tFourCC == FOURCC(T50A) ||
         tFourCC == FOURCC(T52A) ||
         tFourCC == FOURCC(T5mA)
  ;
}

/*****************************************************************************/
bool AL_IsTiled(TFourCC tFourCC)
{
  return AL_Is32x4Tiled(tFourCC) || AL_Is64x4Tiled(tFourCC);
}

/*****************************************************************************/
AL_EFbStorageMode AL_GetStorageMode(TFourCC tFourCC)
{
  if(AL_Is64x4Tiled(tFourCC))
    return AL_FB_TILE_64x4;

  if(AL_Is32x4Tiled(tFourCC))
    return AL_FB_TILE_32x4;

  return AL_FB_RASTER;
}

