// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common/Round.h"
#include "lib_common/Utils.h"
#include "lib_common/Arithmetics.h"
#include "lib_common/BufferPixMapMeta.h"

/*****************************************************************************/
int32_t AL_DecGetLumaPixPlanePitch(int32_t iWidth, AL_TPicFormat const* pPicFormat)
{
  int32_t const iBurstAlignment = pPicFormat->eStorageMode == AL_FB_RASTER ? AL_DEC_PITCH_ALIGNMENT : HW_IP_BURST_ALIGNMENT;
  int32_t const iRndWidth = AL_RoundUp(iWidth, 64);
  Rtos_Assert((iBurstAlignment % HW_IP_BURST_ALIGNMENT) == 0);
  return AL_GetLumaPixPlanePitch(iRndWidth, pPicFormat, iBurstAlignment);
}

/******************************************************************************/
int32_t AL_DecGetPixPlaneHeight(int32_t iHeight, AL_TPicFormat const* pPicFormat)
{
  // Height alignment required by customers to the LCU size
  int32_t const iLcuAlignment = 64;
  return AL_RoundUp(iHeight, iLcuAlignment) / AL_GetNumLinesInPitch(pPicFormat->eStorageMode, pPicFormat->eChromaMode, pPicFormat->uBitDepth);
}

/****************************************************************************/
int32_t AL_GetAllocSize_HevcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int32_t iBlk16x16 = GetSquareBlkNumber(tDim, 64) * 16;
  return HEVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int32_t AL_GetAllocSize_AvcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode)
{
  int32_t iBlk16x16 = GetSquareBlkNumber(tDim, 16);
  return AVC_LCU_CMP_SIZE[eChromaMode] * iBlk16x16;
}

/****************************************************************************/
int32_t AL_GetAllocSize_DecCompMap(AL_TDimension tDim)
{
  int32_t iBlk16x16 = GetSquareBlkNumber(tDim, 64) * 16;
  return SIZE_LCU_INFO * iBlk16x16;
}

/*****************************************************************************/
int32_t AL_GetAllocSize_HevcMV(AL_TDimension tDim)
{
  int32_t iNumBlk = GetSquareBlkNumber(tDim, 64) * 16;
  return 4 * iNumBlk * sizeof(int32_t);
}

/*****************************************************************************/
int32_t AL_GetAllocSize_AvcMV(AL_TDimension tDim)
{
  int32_t iNumBlk = GetSquareBlkNumber(tDim, 16);
  int32_t iSize = 16 * iNumBlk * sizeof(int32_t);
  return iSize;
}

/*****************************************************************************/
inline int32_t AL_DecGetAllocSize_Frame_PixPlane(AL_TPicFormat const* pPicFormat, AL_TDimension tDim, int32_t iPitch, AL_EPlaneId ePlaneId)
{
  return AL_GetAllocSize_Frame_PixPlane(pPicFormat, (AL_TPitch) {iPitch, AL_DecGetPixPlaneHeight(tDim.iHeight, pPicFormat) }, ePlaneId);
}

/*****************************************************************************/
int32_t AL_DecGetAllocSize_Frame_Y(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int32_t iPitch)
{
  AL_TPicFormat tPicFormat = GetDefaultPicFormat();
  tPicFormat.eChromaMode = AL_CHROMA_MONO;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(AL_CHROMA_MONO);
  tPicFormat.uBitDepth = 8;
  tPicFormat.eStorageMode = eFbStorage;
  tPicFormat.bCompressed = false;
  return AL_DecGetAllocSize_Frame_PixPlane(&tPicFormat, tDim, iPitch, AL_PLANE_Y);
}

/*****************************************************************************/
int32_t AL_DecGetAllocSize_Frame_UV(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int32_t iPitch, AL_EChromaMode eChromaMode)
{
  AL_TPicFormat tPicFormat = GetDefaultPicFormat();
  tPicFormat.eChromaMode = eChromaMode;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(AL_CHROMA_MONO);
  tPicFormat.uBitDepth = 8;
  tPicFormat.eStorageMode = eFbStorage;
  tPicFormat.bCompressed = false;
  return AL_DecGetAllocSize_Frame_PixPlane(&tPicFormat, tDim, iPitch, AL_PLANE_UV);
}

/****************************************************************************/
uint32_t AL_GetRefListOffsets(TRefListOffsets* pOffsets, AL_ECodec eCodec, AL_EChromaMode eChromaMode, uint8_t uMaxRef, uint8_t uAddrSizeInBytes)
{
  uint8_t uOffsetToNextSet = 1 << ceil_log2(uMaxRef);

  const int32_t iNbPixPlanes = eChromaMode == AL_CHROMA_4_4_4 ? 3 : 2;
  TRefListOffsets tOffsets;

  uint32_t uOffset = uAddrSizeInBytes * uOffsetToNextSet * iNbPixPlanes; // size of RefList Buff Addrs

  if(AL_IS_ITU_CODEC(eCodec) || eCodec == AL_CODEC_INVALID)
  {
    tOffsets.uColocPocOffset = uOffset;
    uOffset += uAddrSizeInBytes * uOffsetToNextSet; // size of coloc POCs Buff Addrs

    tOffsets.uColocMVOffset = uOffset;
    uOffset += uAddrSizeInBytes * uOffsetToNextSet; // size of coloc MVs Buff Addrs
  }

  tOffsets.uMapOffset = uOffset;
  uOffset += uAddrSizeInBytes * uOffsetToNextSet * iNbPixPlanes;  // size of RefList Map Addr

  if(pOffsets)
    *pOffsets = tOffsets;

  return uOffset;
}

/*****************************************************************************/
inline int32_t AL_DecGetAllocSize_Frame(AL_TDimension tDim, int32_t iPitch, AL_TPicFormat tPicFormat)
{
  return AL_GetAllocSize_Frame_Full(tDim, &tPicFormat, (AL_TPitch) {iPitch, AL_DecGetPixPlaneHeight(tDim.iHeight, &tPicFormat) }, 64);
}

/*****************************************************************************/
int32_t AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  AL_TPicFormat tPicFormat = GetDefaultPicFormat();
  tPicFormat.eChromaMode = eChromaMode;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(eChromaMode);
  tPicFormat.uBitDepth = uBitDepth;
  tPicFormat.eStorageMode = eFbStorageMode;
  tPicFormat.bCompressed = bFbCompression;
  tPicFormat.eSamplePackMode = GetInternalBufSamplePackMode(eFbStorageMode, uBitDepth);
  int32_t iPitch = AL_DecGetLumaPixPlanePitch(tDim.iWidth, &tPicFormat);
  return AL_DecGetAllocSize_Frame(tDim, iPitch, tPicFormat);
}

/*****************************************************************************/
AL_TMetaData* AL_CreateRecBufMetaData(AL_TDimension tDim, int32_t iMinPitch, TFourCC tFourCC)
{
  AL_TPixMapMetaData* pSrcMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);
  pSrcMeta->tDim = tDim;

  AL_TPicFormat tPicFormat;

  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);

  if(!bSuccess)
  {
    AL_MetaData_Destroy((AL_TMetaData*)pSrcMeta);
    Rtos_Assert(bSuccess);
    return NULL;
  }

  int32_t iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int32_t iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat, usedPlanes);

  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int32_t iPitch = usedPlanes[iPlane] == AL_PLANE_Y || usedPlanes[iPlane] == AL_PLANE_YUV ? iMinPitch : AL_GetChromaPitch(tFourCC, iMinPitch);
    AL_PixMapMetaData_AddPlane(pSrcMeta, (AL_TPlane) {0, iOffset, iPitch }, usedPlanes[iPlane]);

    if(usedPlanes[iPlane] == AL_PLANE_U)
      AL_PixMapMetaData_AddPlane(pSrcMeta, (AL_TPlane) {0, iOffset, iPitch }, AL_PLANE_UV);

    iOffset += AL_DecGetAllocSize_Frame_PixPlane(&tPicFormat, tDim, iPitch, usedPlanes[iPlane]);
  }

  return (AL_TMetaData*)pSrcMeta;
}

/*****************************************************************************/
static AL_TBuffer* CreateBuffer(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pRefCountCallback, void* pUserParam, AL_TMetaData* pMeta, size_t zSize, char const* pDebugName)
{
  AL_TBuffer* pBuf = AL_Buffer_Create_And_AllocateNamed(pAllocator, zSize, pRefCountCallback, pDebugName);

  if(!pBuf)
    return NULL;

  if(!AL_Buffer_AddMetaData(pBuf, pMeta))
  {
    AL_Buffer_Destroy(pBuf);
    return NULL;
  }

  AL_Buffer_SetUserData(pBuf, pUserParam);

  return pBuf;
}

/*****************************************************************************/
static uint32_t GetBufferSize(AL_TDecFrameBufferParams const* pFrameBufferParams, uint32_t iMinPitch)
{
  uint32_t iSizeYuv = AL_DecGetAllocSize_Frame(pFrameBufferParams->tDim, iMinPitch, pFrameBufferParams->tPicFormat);
  return iSizeYuv;
}

/*****************************************************************************/
AL_TBuffer* AL_DecCreateFrameBuffer(AL_TDecFrameBufferParams const* pFrameBufferParams, AL_TAllocator* pAllocator, PFN_RefCount_CallBack pRefCountCallback, void* pUserParam)
{
  AL_TDimension tDim = pFrameBufferParams->tDim;
  TFourCC tFourCC = AL_GetFourCC(&pFrameBufferParams->tPicFormat);
  int32_t iMinPitch = AL_DecGetLumaPixPlanePitch(tDim.iWidth, &pFrameBufferParams->tPicFormat);
  int32_t iSizeYuv = GetBufferSize(pFrameBufferParams, iMinPitch);

  AL_TMetaData* pMeta = AL_CreateRecBufMetaData(tDim, iMinPitch, tFourCC);

  if(!pMeta)
    return NULL;

  AL_TBuffer* pBuf = CreateBuffer(pAllocator, pRefCountCallback, pUserParam, pMeta, iSizeYuv, "yuv-itl");

  if(pBuf == NULL)
    AL_MetaData_Destroy(pMeta);

  return pBuf;
}
