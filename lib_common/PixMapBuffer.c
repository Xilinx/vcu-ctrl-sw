// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "PixMapBufferInternal.h"
#include "include/lib_common/PicFormat.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common/FbcMapSize.h"
#include "lib_common/Round.h"

AL_TBuffer* AL_PixMapBuffer_Create(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack, AL_TDimension tDim, TFourCC tFourCC)
{
  AL_TBuffer* pBuf = AL_Buffer_CreateEmpty(pAllocator, pCallBack);

  if(pBuf == NULL)
    goto fail_buffer;

  AL_TPixMapMetaData* pMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);

  if(pMeta == NULL)
    goto fail_meta;

  if(!AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pMeta))
    goto fail_add;

  pMeta->tDim = tDim;

  return pBuf;

  fail_add:
  AL_MetaData_Destroy((AL_TMetaData*)pMeta);
  fail_meta:
  AL_Buffer_Destroy(pBuf);
  fail_buffer:
  return NULL;
}

static inline size_t GetChromaSize(size_t iLumaSize, AL_EChromaMode eCMode, AL_EPlaneMode ePMode)
{
  switch(ePMode)
  {
  case AL_PLANE_MODE_SEMIPLANAR:
    iLumaSize *= 2;
    break;
  case AL_PLANE_MODE_INTERLEAVED:
    return 0;
  default:
    break;
  }
  switch(eCMode)
  {
  case AL_CHROMA_4_2_2:
    return iLumaSize / 2;
  case AL_CHROMA_4_2_0:
    return iLumaSize / 4;
  case AL_CHROMA_4_0_0:
    return 0;
  case AL_CHROMA_4_4_4:
    return iLumaSize;
  default:
    return 0;
  }
}

static int32_t AL_PlaneId_To_PlaneSizeId(AL_EPlaneId ePlaneId)
{
  switch(ePlaneId)
  {
  case AL_PLANE_Y:
  case AL_PLANE_U:
  case AL_PLANE_V:
    return (int)ePlaneId;
  case AL_PLANE_UV:
    return (int)AL_PLANE_U;
  case AL_PLANE_YUV:
    return (int)AL_PLANE_Y;
  case AL_PLANE_MAP_Y:
  case AL_PLANE_MAP_U:
  case AL_PLANE_MAP_V:
    return (int)ePlaneId - 2;
  case AL_PLANE_MAP_UV:
    return (int)AL_PLANE_MAP_U - 2;
  default:
    return 0;
  }
}

void AL_PixMapBuffer_ComputePlaneSizes(AL_TDimension tDim, AL_TPicFormat tPicFormat, size_t* pPlanesSize, int32_t* iOutPixPitchY, int32_t* iOutPixPitchC, int32_t* iOutMapPitch, uint8_t uWidthAlignment)
{
  int32_t iAlignedWidth, iAlignedHeight, iTileHeight = 0, iTileWidth = 0, iPixPitch = 0;
  int32_t iMapTileHeight = 0, iMapAlignedHeight, iMapPitch = 0;

  if(IsTile(tPicFormat.eStorageMode))
  {
    // Compute aligned dimensions & Pitches for compressed format
    iTileHeight = GetTileHeight(tPicFormat.eStorageMode);
    iTileWidth = GetTileWidth(tPicFormat.eStorageMode, tPicFormat.uBitDepth);

    iAlignedWidth = AL_RoundUp(tDim.iWidth, iTileWidth);
    iAlignedHeight = AL_RoundUp(tDim.iHeight, iTileHeight);
  }
  else
  {
    // Compute aligned dimensions & Pitch for raster format

    iAlignedWidth = AL_RoundUp(tDim.iWidth, uWidthAlignment);
    iAlignedHeight = AL_RoundUp(tDim.iHeight, 8);
  }
  iPixPitch = AL_GetLumaPixPlanePitch(iAlignedWidth, &tPicFormat, uWidthAlignment);

  if(tPicFormat.bCompressed)
  {
  }
  else
  {
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_Y)] = 0;
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_U)] = 0;
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_V)] = 0;
  }

  // Compute First Pixel & Map Planes size
  pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_Y)] = iPixPitch * iAlignedHeight / AL_GetNumLinesInPitch(tPicFormat.eStorageMode, tPicFormat.eChromaMode, tPicFormat.uBitDepth);
  pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_U)] = 0;
  pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_V)] = 0;

  if(tPicFormat.bCompressed)
  {
    iMapTileHeight = GetTileHeight(tPicFormat.eStorageMode);

    uint8_t uTileVerticalAlignment = 1;

    iMapAlignedHeight = AL_RoundUp(tDim.iHeight, iMapTileHeight);
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_Y)] = iMapPitch * ((iMapAlignedHeight / uTileVerticalAlignment) / iMapTileHeight);
  }

  if(tPicFormat.eChromaMode == AL_CHROMA_MONO || tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED) // Mono plane formats
    goto finish_planes_calculations;

  // Compute chroma planes sizes
  if(tPicFormat.eChromaMode == AL_CHROMA_4_2_0 && IsTile(tPicFormat.eStorageMode)) // Have to realign to the tile height
  {
    // Assumption: Planar Tiled Mode don't exist (this was a previous code assumption)
    iAlignedHeight = AL_RoundUp(tDim.iHeight / 2, iTileHeight);
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_UV)] = iPixPitch * iAlignedHeight / AL_GetNumLinesInPitch(tPicFormat.eStorageMode, tPicFormat.eChromaMode, tPicFormat.uBitDepth);

    if(tPicFormat.bCompressed)
    {
      iMapAlignedHeight = AL_RoundUp(tDim.iHeight / 2, iMapTileHeight);

      uint8_t uTileVerticalAlignment = 1;

      pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_UV)] = iMapPitch * ((iMapAlignedHeight / uTileVerticalAlignment) / iMapTileHeight);
    }
  }
  else
  {
    pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_U)] = GetChromaSize(pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_Y)], tPicFormat.eChromaMode, tPicFormat.ePlaneMode);

    if(tPicFormat.bCompressed)
      pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_U)] = pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_Y)];

    if(tPicFormat.ePlaneMode == AL_PLANE_MODE_PLANAR || tPicFormat.eChromaMode == AL_CHROMA_4_4_4) // Force 444 to planar
    {
      pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_V)] = pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_U)];

      if(tPicFormat.bCompressed)
        pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_V)] = pPlanesSize[AL_PlaneId_To_PlaneSizeId(AL_PLANE_MAP_U)];
    }
  }

  finish_planes_calculations:

  // Align returned PixPitch to the tile height (I personally don't know why but there were
  // specific calculations that were made to account for this so I suppose that there's a reason)

  *iOutPixPitchY = iPixPitch;
  *iOutPixPitchC = AL_GetChromaPixPlanePitch(&tPicFormat, iPixPitch);
  *iOutMapPitch = iMapPitch;
}

static void AddPlanesToMeta(AL_TPixMapMetaData* pMeta, int32_t iChunkIdx, const AL_TPlaneDescription* pPlDesc, int32_t iNbPlanes)
{
  for(int32_t i = 0; i < iNbPlanes; i++, pPlDesc++)
  {
    AL_TPlane tPlane = { iChunkIdx, pPlDesc->iOffset, pPlDesc->iPitch };
    AL_PixMapMetaData_AddPlane(pMeta, tPlane, pPlDesc->ePlaneId);
  }
}

bool AL_PixMapBuffer_Allocate_And_AddPlanes(AL_TBuffer* pBuf, size_t zSize, const AL_TPlaneDescription* pPlDesc, int32_t iNbPlanes, char const* name)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  int32_t iChunkIdx = AL_Buffer_AllocateChunkNamed(pBuf, zSize, name);

  if(iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return false;

  AddPlanesToMeta(pMeta, iChunkIdx, pPlDesc, iNbPlanes);

  return true;
}

static int32_t AL_PixMapBuffer_CreatePlaneDescription(AL_TPlaneDescription* pPlaneDesc, size_t* pPlanesSize, AL_EPlaneId* usedPlanes, int32_t iNbPlanes, int32_t iPixPitchY, int32_t iPixPitchC, int32_t iMapPitch)
{
  int32_t iTotalPlanesSize = 0;

  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    AL_EPlaneId ePlaneId = usedPlanes[iPlane];

    int32_t currSize = pPlanesSize[AL_PlaneId_To_PlaneSizeId(ePlaneId)];
    int32_t currPitch = ePlaneId >= AL_PLANE_MAP_Y ? iMapPitch : ePlaneId == AL_PLANE_Y || ePlaneId == AL_PLANE_YUV ? iPixPitchY : iPixPitchC;

    pPlaneDesc[iPlane] = (AL_TPlaneDescription) {
      ePlaneId, iTotalPlanesSize, currPitch
    };
    iTotalPlanesSize += currSize;
  }

  return iTotalPlanesSize;
}

AL_TBuffer* AL_PixMapBuffer_Create_And_AddPlanes(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack, AL_TDimension tAllocDim, AL_TDimension tDim, AL_TPicFormat tPicFormat, uint8_t uWidthAlignment, char const* name)
{
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  size_t pPlanesSize[AL_MAX_BUFFER_PLANES];
  AL_TPlaneDescription pPlaneDesc[AL_MAX_BUFFER_PLANES];

  TFourCC tFourCC = AL_GetFourCC(&tPicFormat);
  AL_TBuffer* pBuf = AL_PixMapBuffer_Create(pAllocator, pCallBack, tAllocDim, tFourCC);

  if(pBuf == NULL)
    return NULL;

  int32_t iNbPlanes = AL_Plane_GetBufferPlanes(tPicFormat, usedPlanes);
  int32_t iPixPitchY, iPixPitchC, iMapPitch;
  AL_PixMapBuffer_ComputePlaneSizes(tDim, tPicFormat, pPlanesSize, &iPixPitchY, &iPixPitchC, &iMapPitch, uWidthAlignment);

  int32_t iTotalPlanesSize = AL_PixMapBuffer_CreatePlaneDescription(pPlaneDesc, pPlanesSize, usedPlanes, iNbPlanes, iPixPitchY, iPixPitchC, iMapPitch);

  if(!AL_PixMapBuffer_Allocate_And_AddPlanes(pBuf, iTotalPlanesSize, &pPlaneDesc[0], iNbPlanes, name))
  {
    AL_Buffer_Destroy(pBuf);
    return NULL;
  }

  return pBuf;
}

bool AL_PixMapBuffer_AddPlanes(AL_TBuffer* pBuf, AL_HANDLE hChunk, size_t zSize, const AL_TPlaneDescription* pPlDesc, int32_t iNbPlanes)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  int32_t iChunkIdx = AL_Buffer_AddChunk(pBuf, hChunk, zSize);

  if(iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return false;

  AddPlanesToMeta(pMeta, iChunkIdx, pPlDesc, iNbPlanes);

  return true;
}

AL_VADDR AL_PixMapBuffer_GetPlaneAddress(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return NULL;

  AL_VADDR pPlaneVADDR = AL_Buffer_GetDataChunk(pBuf, pMeta->tPlanes[ePlaneId].iChunkIdx);

  if(pPlaneVADDR == NULL)
    return NULL;

  return pPlaneVADDR + pMeta->tPlanes[ePlaneId].iOffset;
}

AL_PADDR AL_PixMapBuffer_GetPlanePhysicalAddress(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return 0;

  AL_PADDR tPlanePADDR = AL_Buffer_GetPhysicalAddressChunk(pBuf, pMeta->tPlanes[ePlaneId].iChunkIdx);

  if(tPlanePADDR == 0)
    return 0;

  return tPlanePADDR + pMeta->tPlanes[ePlaneId].iOffset;
}

int32_t AL_PixMapBuffer_GetPlanePitch(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL || pMeta->tPlanes[ePlaneId].iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return 0;

  return pMeta->tPlanes[ePlaneId].iPitch;
}

AL_TDimension AL_PixMapBuffer_GetDimension(AL_TBuffer const* pBuf)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
  {
    AL_TDimension tEmptyDim = { 0, 0 };
    return tEmptyDim;
  }

  return pMeta->tDim;
}

bool AL_PixMapBuffer_SetDimension(AL_TBuffer* pBuf, AL_TDimension tDim)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  pMeta->tDim = tDim;

  return true;
}

TFourCC AL_PixMapBuffer_GetFourCC(AL_TBuffer const* pBuf)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return 0;

  return pMeta->tFourCC;
}

bool AL_PixMapBuffer_SetFourCC(AL_TBuffer* pBuf, TFourCC tFourCC)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  pMeta->tFourCC = tFourCC;

  return true;
}

int32_t AL_PixMapBuffer_GetPlaneChunkIdx(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return AL_BUFFER_BAD_CHUNK;

  return pMeta->tPlanes[ePlaneId].iChunkIdx;
}

int32_t AL_PixMapBuffer_GetDefinedPlanes(AL_TBuffer const* pBuf, AL_EPlaneId planes[AL_PLANE_MAX_ENUM])
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return 0;

  int32_t iNbDefinedPlanes = 0;

  for(int32_t iPlane = AL_PLANE_Y; iPlane < AL_PLANE_MAX_ENUM; iPlane++)
  {
    if(pMeta->tPlanes[iPlane].iChunkIdx != AL_BUFFER_BAD_CHUNK)
    {
      planes[iNbDefinedPlanes] = (AL_EPlaneId)iPlane;
      iNbDefinedPlanes++;
    }
  }

  return iNbDefinedPlanes;
}

uint32_t AL_PixMapBuffer_GetPositionOffset(AL_TBuffer const* pBuf, AL_TPosition tPos, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData const* pMeta = (AL_TPixMapMetaData const*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  uint32_t uPitch = pMeta->tPlanes[ePlaneId].iPitch;
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pMeta->tFourCC);
  uint8_t uBitdepth = AL_GetBitDepth(pMeta->tFourCC);

  if(ePlaneId != AL_PLANE_Y)
  {
    if(eChromaMode == AL_CHROMA_4_2_0)
      tPos.iY /= 2;

    if((eChromaMode == AL_CHROMA_4_2_0 || eChromaMode == AL_CHROMA_4_2_2) && (ePlaneId != AL_PLANE_UV))
      tPos.iX /= 2;
  }

  AL_EFbStorageMode eStorageMode = AL_GetStorageMode(pMeta->tFourCC);
  switch(eStorageMode)
  {
  case AL_FB_RASTER:

    if(uBitdepth > 8)
    {
      return tPos.iY * uPitch + tPos.iX * 4 / 3;
    }
    else
      return tPos.iY * uPitch + tPos.iX;
    break;
  case AL_FB_TILE_32x4:
  case AL_FB_TILE_64x4:
    return uPitch * tPos.iY / 4 + GetTileSize(eStorageMode, uBitdepth) * tPos.iX / GetTileWidth(eStorageMode, uBitdepth);
    break;
  default:
    return 0;
    break;
  }
}
