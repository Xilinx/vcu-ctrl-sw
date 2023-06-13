// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common/Utils.h"
#include "lib_common/BufferPixMapMeta.h"

#include "lib_assert/al_assert.h"

/*****************************************************************************/
int32_t RndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  int const iBurstAlignment = eFrameBufferStorageMode == AL_FB_RASTER ? 256 : HW_IP_BURST_ALIGNMENT;
  int const iRndWidth = RoundUp(iWidth, 64);
  return ComputeRndPitch(iRndWidth, uBitDepth, eFrameBufferStorageMode, iBurstAlignment);
}

/******************************************************************************/
int32_t RndHeight(int32_t iHeight)
{
  int const iAlignment = 64;
  return RoundUp(iHeight, iAlignment);
}

/****************************************************************************/
int AL_GetAllocSize_HevcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int iBlk16x16 = GetSquareBlkNumber(tDim, 64) * 16;
  return HEVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int AL_GetAllocSize_AvcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int iBlk16x16 = GetSquareBlkNumber(tDim, 16);
  return AVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int AL_GetAllocSize_DecCompMap(AL_TDimension tDim)
{
  int iBlk16x16 = GetSquareBlkNumber(tDim, 16);
  return SIZE_LCU_INFO * iBlk16x16;
}

/*****************************************************************************/
int AL_GetAllocSize_HevcMV(AL_TDimension tDim)
{
  int iNumBlk = GetSquareBlkNumber(tDim, 64) * 16;
  return 4 * iNumBlk * sizeof(int32_t);
}

/*****************************************************************************/
int AL_GetAllocSize_AvcMV(AL_TDimension tDim)
{
  int iNumBlk = GetSquareBlkNumber(tDim, 16);
  return 16 * iNumBlk * sizeof(int32_t);
}

/****************************************************************************/
static int GetChromaAllocSize(AL_EChromaMode eChromaMode, int iAllocSizeY)
{
  switch(eChromaMode)
  {
  case AL_CHROMA_MONO:
    return 0;
  case AL_CHROMA_4_2_0:
    return iAllocSizeY >> 1;
  case AL_CHROMA_4_2_2:
    return iAllocSizeY;
  case AL_CHROMA_4_4_4:
    return iAllocSizeY << 1;
  default:
    AL_Assert(0);
    break;
  }

  return 0;
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame_PixPlane(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, AL_EPlaneId ePlaneId)
{
  AL_EChromaOrder eChromaOrder = GetChromaOrder(eChromaMode);

  if(!AL_Plane_Exists(eChromaOrder, false, ePlaneId))
    return 0;

  int iSize = iPitch * RndHeight(tDim.iHeight) / AL_GetNumLinesInPitch(eFbStorage);

  if(ePlaneId == AL_PLANE_UV)
    iSize = GetChromaAllocSize(eChromaMode, iSize);

  return iSize;
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame_Y(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch)
{
  return AL_DecGetAllocSize_Frame_PixPlane(eFbStorage, tDim, iPitch, AL_CHROMA_MONO, AL_PLANE_Y);
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame_UV(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode)
{
  return AL_DecGetAllocSize_Frame_PixPlane(eFbStorage, tDim, iPitch, eChromaMode, AL_PLANE_UV);
}

/****************************************************************************/
uint32_t AL_GetRefListOffsets(TRefListOffsets* pOffsets, AL_ECodec eCodec, AL_EChromaOrder eChromaOrder, uint8_t uAddrSizeInBytes)
{
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  const int iNbPixPlanes = Max(2, AL_Plane_GetBufferPixelPlanes(eChromaOrder, usedPlanes));
  TRefListOffsets tOffsets;
  (void)eCodec;

  uint32_t uOffset = uAddrSizeInBytes * MAX_REF * iNbPixPlanes; // size of RefList Buff Addrs

  tOffsets.uColocPocOffset = uOffset;
  uOffset += uAddrSizeInBytes * MAX_REF; // size of coloc POCs Buff Addrs

  tOffsets.uColocMVOffset = uOffset;
  uOffset += uAddrSizeInBytes * MAX_REF; // size of coloc MVs Buff Addrs

  tOffsets.uMapOffset = uOffset;
  uOffset += uAddrSizeInBytes * MAX_REF * iNbPixPlanes;  // size of RefList Map Addr

  if(pOffsets)
    *pOffsets = tOffsets;

  return uOffset;
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame(AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  uint32_t uSize = 0;
  AL_EChromaOrder eChromaOrder = GetChromaOrder(eChromaMode);

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPlanes(eChromaOrder, bFbCompression, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    uSize += AL_DecGetAllocSize_Frame_PixPlane(eFbStorageMode, tDim, iPitch, eChromaMode, usedPlanes[iPlane]);
  }

  return uSize;
}

/*****************************************************************************/
int AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  int iPitch = RndPitch(tDim.iWidth, uBitDepth, eFbStorageMode);
  return AL_DecGetAllocSize_Frame(tDim, iPitch, eChromaMode, bFbCompression, eFbStorageMode);
}

/*****************************************************************************/
AL_TMetaData* AL_CreateRecBufMetaData(AL_TDimension tDim, int iMinPitch, TFourCC tFourCC)
{
  AL_TPixMapMetaData* pSrcMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);
  pSrcMeta->tDim = tDim;

  AL_TPicFormat tPicFormat;

  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
  {
    AL_MetaData_Destroy((AL_TMetaData*)pSrcMeta);
    AL_Assert(bSuccess);
    return NULL;
  }

  int iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat.eChromaOrder, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int iPitch = usedPlanes[iPlane] == AL_PLANE_Y ? iMinPitch : AL_GetChromaPitch(tFourCC, iMinPitch);
    AL_PixMapMetaData_AddPlane(pSrcMeta, (AL_TPlane) {0, iOffset, iPitch }, usedPlanes[iPlane]);

    if(usedPlanes[iPlane] == AL_PLANE_U)
      AL_PixMapMetaData_AddPlane(pSrcMeta, (AL_TPlane) {0, iOffset, iPitch }, AL_PLANE_UV);

    iOffset += AL_DecGetAllocSize_Frame_PixPlane(tPicFormat.eStorageMode, tDim, iPitch, tPicFormat.eChromaMode, usedPlanes[iPlane]);
  }

  return (AL_TMetaData*)pSrcMeta;
}

/*!@}*/

