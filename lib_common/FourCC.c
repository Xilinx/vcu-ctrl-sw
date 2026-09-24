// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/FourCC.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Utils.h"
#include "lib_rtos/lib_rtos.h"

/* FOURCC from chars */
#define FOURCC2(A, B, C, D) ((TFourCC)(((uint32_t)((A))) \
                                       | ((uint32_t)((B)) << 8) \
                                       | ((uint32_t)((C)) << 16) \
                                       | ((uint32_t)((D)) << 24)))

TFourCC FourCCFromString(const char* str)
{
  return FOURCC2(str[0], str[1], str[2], str[3]);
}

typedef struct AL_TFourCCMapping
{
  TFourCC tfourCC;
  AL_TPicFormat tPictFormat;
}TFourCCMapping;

#define AL_FOURCC_MAPPING(FCC, ChromaMode, AlphaMode, BD, StorageMode, PlaneMode, PlaneOrder, SamplePackedMode, Compression, MSB) { FCC, { ChromaMode, AlphaMode, BD, StorageMode, PlaneMode, PlaneOrder, SamplePackedMode, Compression, MSB } \
}

static const TFourCCMapping FourCCMappings[] =
{
  // planar: 8b
  AL_FOURCC_MAPPING(FOURCC2('I', '4', '2', '0'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', 'Y', 'U', 'V'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'V', '1', '2'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YVU, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '4', '2', '2'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'V', '1', '6'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YVU, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '4', '4', '4'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'Y', '2'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'Y', '0'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'Y', 'M'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // planar: 10b
  , AL_FOURCC_MAPPING(FOURCC2('I', '0', 'A', 'L'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '2', 'A', 'L'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '4', 'A', 'L'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'V', 'P'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'V', 'O'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'V', 'M'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // planar: 12b
  , AL_FOURCC_MAPPING(FOURCC2('I', '0', 'C', 'L'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '2', 'C', 'L'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('I', '4', 'C', 'L'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'C', 'P'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'C', 'O'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', 'U', 'C', 'M'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_YUYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // semi-planar: 8b
  , AL_FOURCC_MAPPING(FOURCC2('N', 'V', '1', '2'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('N', 'V', '1', '6'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('N', 'V', '2', '4'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // semi-planar: 10b
  , AL_FOURCC_MAPPING(FOURCC2('P', '0', 'A', 'L'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('P', '2', 'A', 'L'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('P', '4', '1', '0'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // semi-planar: 12b
  , AL_FOURCC_MAPPING(FOURCC2('P', '0', 'C', 'L'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('P', '2', 'C', 'L'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // monochrome: 8b
  , AL_FOURCC_MAPPING(FOURCC2('Y', '8', '0', '0'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // monochrome: 10b
  , AL_FOURCC_MAPPING(FOURCC2('Y', '0', '1', '0'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // monochrome: 12b
  , AL_FOURCC_MAPPING(FOURCC2('Y', '0', '1', '2'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_RASTER, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // tile : 64x4: 8b
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', 'm', '8'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_64x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '0', '8'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '2', '8'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '4', '8'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_64x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // tile : 64x4: 10b
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', 'm', 'A'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_64x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '0', 'A'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '2', 'A'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '4', 'A'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_64x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // tile : 64x4: 12b
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', 'm', 'C'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_64x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '0', 'C'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '2', 'C'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_64x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '6', '4', 'C'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_64x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // tile : 32x4: 8b
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', 'm', '8'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_32x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '0', '8'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '2', '8'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '4', '8'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 8, AL_FB_TILE_32x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // tile : 32x4: 10b
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', 'm', 'A'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_32x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '0', 'A'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '2', 'A'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '4', 'A'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 10, AL_FB_TILE_32x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // tile : 32x4: 12b
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', 'm', 'C'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_32x4, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '0', 'C'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '2', 'C'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_32x4, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('T', '5', '4', 'C'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_DISABLED, 12, AL_FB_TILE_32x4, AL_PLANE_MODE_PLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // 10b packed
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '1', '0'), AL_CHROMA_4_0_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_MONOPLANE, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED_XV, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '1', '5'), AL_CHROMA_4_2_0, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED_XV, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('X', 'V', '2', '0'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 10, AL_FB_RASTER, AL_PLANE_MODE_SEMIPLANAR, AL_COMPONENT_ORDER_YUV, AL_SAMPLE_PACK_MODE_PACKED_XV, false, false)

  // 8b packed
  , AL_FOURCC_MAPPING(FOURCC2('A', 'Y', 'U', 'V'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_AFTER, 8, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_VUY, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  // 10b packed
  , AL_FOURCC_MAPPING(FOURCC2('Y', '4', '1', '0'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_AFTER, 10, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_UYV, AL_SAMPLE_PACK_MODE_PACKED, false, false)

  // Y416 10b LSB, MSB and 12b LSB, MSB
  , AL_FOURCC_MAPPING(FOURCC2('Y', '4', 'A', 'L'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_AFTER, 10, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_UYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)
  , AL_FOURCC_MAPPING(FOURCC2('Y', '4', 'C', 'L'), AL_CHROMA_4_4_4, AL_ALPHA_MODE_AFTER, 12, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_UYV, AL_SAMPLE_PACK_MODE_BYTE, false, false)

  , AL_FOURCC_MAPPING(FOURCC2('U', 'Y', 'V', 'Y'), AL_CHROMA_4_2_2, AL_ALPHA_MODE_DISABLED, 8, AL_FB_RASTER, AL_PLANE_MODE_INTERLEAVED, AL_COMPONENT_ORDER_UYVY, AL_SAMPLE_PACK_MODE_BYTE, false, false)

};

static int32_t const FourCCMappingSize = ARRAY_SIZE(FourCCMappings);

/****************************************************************************/
bool AL_GetPicFormat(TFourCC tFourCC, AL_TPicFormat* tPicFormat)
{
  if(tPicFormat == NULL)
    return false;

  TFourCCMapping const* pBeginMapping = &FourCCMappings[0];
  TFourCCMapping const* pEndMapping = pBeginMapping + FourCCMappingSize;

  for(TFourCCMapping const* pMapping = pBeginMapping; pMapping != pEndMapping; pMapping++)
  {
    if(pMapping->tfourCC == tFourCC)
    {
      *tPicFormat = pMapping->tPictFormat;
      return true;
    }
  }

  Rtos_Assert(false && "Unknown fourCC");

  return false;
}

/****************************************************************************/
TFourCC AL_GetFourCC(AL_TPicFormat const* pPicFormat)
{
  const TFourCCMapping* pBeginMapping = &FourCCMappings[0];
  const TFourCCMapping* pEndMapping = pBeginMapping + FourCCMappingSize;

  for(const TFourCCMapping* pMapping = pBeginMapping; pMapping != pEndMapping; pMapping++)
  {
    if(pMapping->tPictFormat.eStorageMode == pPicFormat->eStorageMode &&
       pMapping->tPictFormat.bCompressed == pPicFormat->bCompressed &&
       pMapping->tPictFormat.ePlaneMode == pPicFormat->ePlaneMode &&
       pMapping->tPictFormat.eComponentOrder == pPicFormat->eComponentOrder &&
       pMapping->tPictFormat.eChromaMode == pPicFormat->eChromaMode &&
       pMapping->tPictFormat.uBitDepth == pPicFormat->uBitDepth &&
       pMapping->tPictFormat.eAlphaMode == pPicFormat->eAlphaMode &&
       pMapping->tPictFormat.eSamplePackMode == pPicFormat->eSamplePackMode &&
       pMapping->tPictFormat.bMSB == pPicFormat->bMSB)
      return pMapping->tfourCC;
  }

  Rtos_Assert(false && "Unknown picture format");

  return FOURCC(NULL);
}

/****************************************************************************/
AL_EChromaMode AL_GetChromaMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eChromaMode : AL_CHROMA_MAX_ENUM;
}

/****************************************************************************/
AL_EAlphaMode AL_GetAlphaMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eAlphaMode : AL_ALPHA_MODE_MAX_ENUM;
}

/****************************************************************************/
AL_EPlaneMode AL_GetPlaneMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.ePlaneMode : AL_PLANE_MODE_MAX_ENUM;
}

/****************************************************************************/
AL_EComponentOrder AL_GetPlaneOrder(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eComponentOrder : AL_COMPONENT_ORDER_MAX_ENUM;
}

/****************************************************************************/
AL_ESamplePackMode AL_GetSamplePackMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eSamplePackMode : AL_SAMPLE_PACK_MODE_MAX_ENUM;
}

/****************************************************************************/
uint8_t AL_GetBitDepth(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.uBitDepth : -1;
}

/****************************************************************************/
int32_t AL_GetPixelSize(TFourCC tFourCC)
{
  if(tFourCC == FOURCC(Y410))
    return sizeof(uint32_t);

  int32_t iPixSize = (AL_GetBitDepth(tFourCC) > 8) ? sizeof(uint16_t) : sizeof(uint8_t);

  if(AL_IsInterleaved(tFourCC))
  {
    if(AL_GetChromaMode(tFourCC) == AL_CHROMA_4_4_4)
      iPixSize *= 4;
    else if(AL_GetChromaMode(tFourCC) == AL_CHROMA_4_2_2)
      iPixSize *= 2;
  }

  return iPixSize;
}

/****************************************************************************/
void AL_GetSubsampling(TFourCC fourcc, int32_t* sx, int32_t* sy)
{
  switch(AL_GetChromaMode(fourcc))
  {
  case AL_CHROMA_4_2_0:
    *sx = 2;
    *sy = 2;
    break;
  case AL_CHROMA_4_2_2:
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
bool AL_IsMonochrome(TFourCC tFourCC)
{
  return AL_GetChromaMode(tFourCC) == AL_CHROMA_MONO;
}

/*****************************************************************************/
bool AL_IsSemiPlanar(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && (tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR);
}

/*****************************************************************************/
bool AL_IsInterleaved(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && (tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED);
}

/*****************************************************************************/
bool AL_IsCompressed(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && tPicFormat.bCompressed;
}

/*****************************************************************************/
bool AL_Is10bPacked(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) &&
         AL_GetBitDepth(tFourCC) == 10 &&
         (tPicFormat.eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED ||
          tPicFormat.eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED_XV);
}

/*****************************************************************************/
bool AL_IsRasterMSB(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) && tPicFormat.bMSB;
}

/*****************************************************************************/
bool AL_IsTiled(TFourCC tFourCC)
{
  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);

  if(eStorageMode != AL_FB_RASTER)
    return true;

  return false;
}

/*****************************************************************************/

bool AL_IsNonCompTiled(TFourCC tFourCC)
{
  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(tFourCC);

  if(eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4)
    return true;
  return false;
}

/*****************************************************************************/
AL_EFbStorageMode AL_GetStorageMode(TFourCC tFourCC)
{
  AL_TPicFormat tPicFormat;
  return AL_GetPicFormat(tFourCC, &tPicFormat) ? tPicFormat.eStorageMode : AL_FB_RASTER;
}

AL_StringFourCC AL_FourCCToString(TFourCC tFourCC)
{
  AL_StringFourCC tStringFourCC;
  tStringFourCC.cFourcc[0] = tFourCC & 0xFF;
  tStringFourCC.cFourcc[1] = (tFourCC & 0xFF00) >> 8;
  tStringFourCC.cFourcc[2] = (tFourCC & 0xFF0000) >> 16;
  tStringFourCC.cFourcc[3] = (tFourCC & 0xFF000000) >> 24;
  tStringFourCC.cFourcc[4] = '\0';
  return tStringFourCC;
}

bool AL_IsCompatible(TFourCC tInFourCC, TFourCC tOutFourCC)
{
  if(tInFourCC == tOutFourCC)
    return true;

  if(AL_IsMonochrome(tOutFourCC))
    return (AL_GetPlaneMode(tInFourCC) == AL_PLANE_MODE_PLANAR || AL_GetPlaneMode(tInFourCC) == AL_PLANE_MODE_SEMIPLANAR)
           && (AL_GetBitDepth(tInFourCC) == AL_GetBitDepth(tOutFourCC))
           && (AL_IsRasterMSB(tInFourCC) == AL_IsRasterMSB(tOutFourCC))
           && (AL_IsTiled(tInFourCC) == AL_IsTiled(tOutFourCC))
           && (AL_IsCompressed(tInFourCC) == AL_IsCompressed(tOutFourCC));

  return false;
}
