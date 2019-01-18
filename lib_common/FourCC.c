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

#include "lib_common/FourCC.h"
#include <assert.h>

/* FOURCC from chars */
#define FOURCC2(A, B, C, D) ((TFourCC)(((uint32_t)((A))) \
                                       | ((uint32_t)((B)) << 8) \
                                       | ((uint32_t)((C)) << 16) \
                                       | ((uint32_t)((D)) << 24)))

typedef struct AL_t_FourCCMapping
{
  TFourCC tfourCC;
  AL_TPicFormat tPictFormat;
}TFourCCMapping;

#define AL_FOURCC_MAPPING(FCC, ChromaMode, BD, StorageMode, ChromaOrder, Compression, Packed10) { FCC, { ChromaMode, BD, StorageMode, ChromaOrder, Compression, Packed10 } \
}

static const TFourCCMapping FourCCMappings[] =
{
  // planar: 8b
  AL_FOURCC_MAPPING(FOURCC2('I', '4', '2', '0'), CHROMA_4_2_0, 8, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', 'Y', 'U', 'V'), CHROMA_4_2_0, 8, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'V', '1', '2'), CHROMA_4_2_0, 8, AL_FB_RASTER, AL_C_ORDER_V_U, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '4', '2', '2'), CHROMA_4_2_2, 8, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'V', '1', '6'), CHROMA_4_2_2, 8, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)
  // planar: 10b
  , AL_FOURCC_MAPPING(FOURCC2('I', '0', 'A', 'L'), CHROMA_4_2_0, 10, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '2', 'A', 'L'), CHROMA_4_2_2, 10, AL_FB_RASTER, AL_C_ORDER_U_V, false, false)

  // semi-planar: 8b
  , AL_FOURCC_MAPPING(FOURCC2('N', 'V', '1', '2'), CHROMA_4_2_0, 8, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('N', 'V', '1', '6'), CHROMA_4_2_2, 8, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, false)
  // semi-planar: 10b
  , AL_FOURCC_MAPPING(FOURCC2('P', '0', '1', '0'), CHROMA_4_2_0, 10, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('P', '2', '1', '0'), CHROMA_4_2_2, 10, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, false)

  // monochrome
  , AL_FOURCC_MAPPING(FOURCC2('Y', '8', '0', '0'), CHROMA_4_0_0, 8, AL_FB_RASTER, AL_C_ORDER_NO_CHROMA, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', '0', '1', '0'), CHROMA_4_0_0, 10, AL_FB_RASTER, AL_C_ORDER_NO_CHROMA, false, false)

  // tile : 64x4
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '0', '8'), CHROMA_4_2_0, 8, AL_FB_TILE_64x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '2', '8'), CHROMA_4_2_2, 8, AL_FB_TILE_64x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', 'm', '8'), CHROMA_4_0_0, 8, AL_FB_TILE_64x4, AL_C_ORDER_NO_CHROMA, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '0', 'A'), CHROMA_4_2_0, 10, AL_FB_TILE_64x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '2', 'A'), CHROMA_4_2_2, 10, AL_FB_TILE_64x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', 'm', 'A'), CHROMA_4_0_0, 10, AL_FB_TILE_64x4, AL_C_ORDER_NO_CHROMA, false, false)
  // tile : 32x4
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '0', '8'), CHROMA_4_2_0, 8, AL_FB_TILE_32x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '2', '8'), CHROMA_4_2_2, 8, AL_FB_TILE_32x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', 'm', '8'), CHROMA_4_0_0, 8, AL_FB_TILE_32x4, AL_C_ORDER_NO_CHROMA, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '0', 'A'), CHROMA_4_2_0, 10, AL_FB_TILE_32x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '2', 'A'), CHROMA_4_2_2, 10, AL_FB_TILE_32x4, AL_C_ORDER_SEMIPLANAR, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', 'm', 'A'), CHROMA_4_0_0, 10, AL_FB_TILE_32x4, AL_C_ORDER_NO_CHROMA, false, false)


  // 10b packed
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '1', '0'), CHROMA_4_0_0, 10, AL_FB_RASTER, AL_C_ORDER_NO_CHROMA, false, true)
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '1', '5'), CHROMA_4_2_0, 10, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, true)
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '2', '0'), CHROMA_4_2_2, 10, AL_FB_RASTER, AL_C_ORDER_SEMIPLANAR, false, true)
};

static const int FourCCMappingSize = sizeof(FourCCMappings) / sizeof(FourCCMappings[0]);

/****************************************************************************/
bool AL_GetPicFormat(TFourCC tFourCC, AL_TPicFormat* tPicFormat)
{
  const TFourCCMapping* pBeginMapping = &FourCCMappings[0];
  const TFourCCMapping* pEndMapping = pBeginMapping + FourCCMappingSize;

  for(const TFourCCMapping* pMapping = pBeginMapping; pMapping != pEndMapping; pMapping++)
  {
    if(pMapping->tfourCC == tFourCC)
    {
      *tPicFormat = pMapping->tPictFormat;
      return true;
    }
  }

  assert(0);

  return false;
}

/****************************************************************************/
TFourCC AL_GetFourCC(AL_TPicFormat tPictFormat)
{
  const TFourCCMapping* pBeginMapping = &FourCCMappings[0];
  const TFourCCMapping* pEndMapping = pBeginMapping + FourCCMappingSize;

  for(const TFourCCMapping* pMapping = pBeginMapping; pMapping != pEndMapping; pMapping++)
  {
    if(pMapping->tPictFormat.eStorageMode == tPictFormat.eStorageMode &&
       pMapping->tPictFormat.bCompressed == tPictFormat.bCompressed &&
       pMapping->tPictFormat.eChromaOrder == tPictFormat.eChromaOrder &&
       pMapping->tPictFormat.eChromaMode == tPictFormat.eChromaMode &&
       pMapping->tPictFormat.uBitDepth == tPictFormat.uBitDepth &&
       pMapping->tPictFormat.b10bPacked == tPictFormat.b10bPacked)
      return pMapping->tfourCC;
  }

  assert(0);

  return 0;
}

/****************************************************************************/
AL_EChromaMode AL_GetChromaMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eChromaMode : (AL_EChromaMode) - 1;
}

/****************************************************************************/
uint8_t AL_GetBitDepth(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.uBitDepth : -1;
}

/****************************************************************************/
int AL_GetPixelSize(TFourCC tFourCC)
{
  return (AL_GetBitDepth(tFourCC) > 8) ? sizeof(uint16_t) : sizeof(uint8_t);
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
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && tPicFormat.b10bPacked;
}

/*****************************************************************************/
bool AL_IsMonochrome(TFourCC tFourCC)
{
  return AL_GetChromaMode(tFourCC) == CHROMA_MONO;
}

/*****************************************************************************/
bool AL_IsSemiPlanar(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && (tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR);
}

/*****************************************************************************/
bool AL_IsCompressed(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && tPicFormat.bCompressed;
}

/*****************************************************************************/
bool AL_IsTiled(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && (tPicFormat.eStorageMode != AL_FB_RASTER);
}

/*****************************************************************************/
AL_EFbStorageMode AL_GetStorageMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eStorageMode : AL_FB_RASTER;
}

