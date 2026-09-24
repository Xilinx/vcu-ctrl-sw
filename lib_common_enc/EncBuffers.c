// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncBuffersInternal.h"

#include "lib_common/Utils.h"
#include "lib_common/Arithmetics.h"
#include "lib_common/Round.h"

#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/EncSize.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common_enc/QPTableInternal.h"

/****************************************************************************/
uint32_t AL_GetAllocSizeEP1(AL_ECodec eCodec)
{
  (void)eCodec;
  uint32_t uEP1Size = 0;
  uEP1Size += EP1_BUF_LAMBDAS.Size;

  if(AL_IS_ITU_CODEC(eCodec))
    uEP1Size += EP1_BUF_SCL_LST.Size;

  return AL_RoundUp(uEP1Size, HW_IP_BURST_ALIGNMENT);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeFlexibleEP2(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, uint8_t uQpLCUGranularity)
{
  return (uint32_t)(EP2_BUF_QP_CTRL.Size) + AL_QPTable_GetFlexibleSize(tDim, eCodec, uLog2MaxCuSize, uQpLCUGranularity);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP2(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize)
{
  return AL_GetAllocSizeFlexibleEP2(tDim, eCodec, uLog2MaxCuSize, 1);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP3PerCore(void)
{
  return (uint32_t)(EP3_BUF_RC_TABLE1.Size + EP3_BUF_RC_TABLE2.Size + EP3_BUF_RC_CTX.Size + EP3_BUF_RC_LVL.Size);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeEP3(void)
{
  uint32_t uMaxSize = AL_GetAllocSizeEP3PerCore() * AL_ENC_NUM_CORES;
  return AL_RoundUp(uMaxSize, 128);
}

/****************************************************************************/

/* Will be removed in 0.9 */
int32_t AL_CalculatePitchValue(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode)
{
  AL_TPicFormat tPicFormat;
  Rtos_Memset(&tPicFormat, 0, sizeof(tPicFormat));
  tPicFormat.uBitDepth = uBitDepth;
  tPicFormat.eStorageMode = eStorageMode;
  return AL_EncGetMinPitch(iWidth, &tPicFormat);
}

int32_t AL_EncGetMinPitch(int32_t iWidth, AL_TPicFormat const* pPicFormat)
{

  Rtos_Assert((AL_ENC_PITCH_ALIGNMENT % HW_IP_BURST_ALIGNMENT) == 0);
  return AL_GetLumaPixPlanePitch(iWidth, pPicFormat, AL_ENC_PITCH_ALIGNMENT);
}

/****************************************************************************/
AL_EFbStorageMode AL_GetSrcStorageMode(AL_ESrcMode eSrcMode)
{
  switch(eSrcMode)
  {
  case AL_SRC_TILE_64x4:
  case AL_SRC_COMP_64x4:
    return AL_FB_TILE_64x4;
  case AL_SRC_TILE_32x4:
  case AL_SRC_COMP_32x4:
    return AL_FB_TILE_32x4;
  default:
    return AL_FB_RASTER;
  }
}

/****************************************************************************/
bool AL_IsSrcCompressed(AL_ESrcMode eSrcMode)
{
  (void)eSrcMode;
  bool bCompressed = false;
  return bCompressed;
}

/****************************************************************************/
bool AL_IsSrcInterleaved(AL_ESrcMode eSrcMode)
{
  (void)eSrcMode;
  bool bInterleaved = false;
  return bInterleaved;
}

/****************************************************************************/
bool AL_IsSrcMSB(AL_ESrcMode eSrcMode)
{
  (void)eSrcMode;
  bool bMSB = false;

  return bMSB;
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_PixPlane(AL_TPicFormat const* pPicFormat, int32_t iPitch, int32_t iStrideHeight, AL_EPlaneId ePlaneId)
{
  return AL_RoundUp(AL_GetAllocSize_Frame_PixPlane(pPicFormat, (AL_TPitch) {iPitch, iStrideHeight / AL_GetNumLinesInPitch(pPicFormat->eStorageMode, pPicFormat->eChromaMode, pPicFormat->uBitDepth) }, ePlaneId), HW_IP_BURST_ALIGNMENT);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_Y(AL_ESrcMode eSrcFmt, int32_t iPitch, int32_t iStrideHeight)
{
  AL_TPicFormat tPicFormat = GetDefaultPicFormat();
  tPicFormat.eChromaMode = AL_CHROMA_MONO;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(AL_CHROMA_MONO);
  tPicFormat.uBitDepth = 8;
  tPicFormat.eStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  tPicFormat.bCompressed = AL_GET_COMP_MODE(eSrcFmt);
  tPicFormat.bMSB = false;
  return AL_GetAllocSizeSrc_PixPlane(&tPicFormat, iPitch, iStrideHeight, AL_PLANE_Y);
}

/****************************************************************************/
uint32_t AL_GetAllocSizeSrc_UV(AL_ESrcMode eSrcFmt, int32_t iPitch, int32_t iStrideHeight, AL_EChromaMode eChromaMode)
{
  AL_TPicFormat tPicFormat = GetDefaultPicFormat();
  tPicFormat.eChromaMode = eChromaMode;
  tPicFormat.ePlaneMode = GetInternalBufPlaneMode(eChromaMode);
  tPicFormat.uBitDepth = 8;
  tPicFormat.eStorageMode = AL_GetSrcStorageMode(eSrcFmt);
  tPicFormat.bCompressed = AL_GET_COMP_MODE(eSrcFmt);
  tPicFormat.bMSB = false;
  return AL_GetAllocSizeSrc_PixPlane(&tPicFormat, iPitch, iStrideHeight, AL_PLANE_UV);
}

/****************************************************************************/
static uint32_t AL_GetNumTileRowInPitch(AL_EFbStorageMode eStorageMode, bool bIsLuma)
{
  (void)eStorageMode;
  (void)bIsLuma;

  return 1;
}

/****************************************************************************/
uint32_t AL_GetRecPitch(AL_TDimension tTileDim, uint32_t uBitDepth, AL_EFbStorageMode eStorageMode, uint32_t uWidth, bool bIsLuma)
{
  (void)eStorageMode;
  (void)bIsLuma;

  int iTileSize = tTileDim.iWidth * tTileDim.iHeight * uBitDepth / 8;
  int iWidthInTile = AL_UnsignedRoundUpAndDivide(uWidth, tTileDim.iWidth, tTileDim.iWidth);
  int iHeightInTile = AL_GetNumTileRowInPitch(eStorageMode, bIsLuma);

  return iWidthInTile * iHeightInTile * iTileSize;
}

/****************************************************************************/
static uint32_t GetRecSize(AL_TDimension tDim, AL_TDimension tTileDim, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsLuma)
{
  uint32_t uNbTileRowInPitch = AL_GetNumTileRowInPitch(eStorageMode, bIsLuma);
  uint32_t uPitch = AL_GetRecPitch(tTileDim, uBitDepth, eStorageMode, tDim.iWidth, bIsLuma);
  uint32_t uNbPixRowInPitch = uNbTileRowInPitch * tTileDim.iHeight;
  uint32_t uNbGigaTiles = AL_RoundUpAndDivide(tDim.iHeight, uNbPixRowInPitch, uNbPixRowInPitch);
  uint32_t uSize = uPitch * uNbGigaTiles;

  return uSize;
}

/****************************************************************************/
static uint32_t GetRecSizeFull(AL_TDimension tDim, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, AL_EChromaMode eChromaMode)
{
  AL_TDimension tTileDim = { 64, 4 };

  uint32_t uSizeY = GetRecSize(tDim, tTileDim, uBitDepth, eStorageMode, true);

  if(eChromaMode == AL_CHROMA_MONO)
    return uSizeY;

  if(eChromaMode == AL_CHROMA_4_2_0)
    tDim.iHeight = AL_RoundUp(tDim.iHeight / 2, tTileDim.iHeight);

  uint32_t uSizeC = GetRecSize(tDim, tTileDim, uBitDepth, eStorageMode, false);

  if(eChromaMode == AL_CHROMA_4_4_4)
    uSizeC *= 2;

  return uSizeY + uSizeC;
}

/****************************************************************************/
static uint32_t GetRecFrameSizeLuma(AL_TDimension tDim, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, AL_EChromaMode eChromaMode, uint8_t uMaxRefBufSizeRatio)
{
  (void)eStorageMode;
  (void)eChromaMode;
  (void)uMaxRefBufSizeRatio;
  return GetRecSizeFull(tDim, uBitDepth, AL_FB_TILE_64x4, AL_CHROMA_MONO);
}

/****************************************************************************/
static uint32_t GetRecFrameSize(AL_TDimension tDim, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, AL_EChromaMode eChromaMode, uint8_t uMaxRefBufSizeRatio)
{
  (void)eStorageMode;
  (void)uMaxRefBufSizeRatio;

  uint32_t uSize = GetRecSizeFull(tDim, uBitDepth, eStorageMode, eChromaMode);

  return uSize;
}

/****************************************************************************/
static uint32_t GetAllocSize_Ref(AL_TDimension tRoundedDim, AL_TPicFormat const* pPicFormat, uint8_t uLCUSize, AL_EChEncOption eOptions, uint16_t uMVVRange, uint8_t uMaxRefBufSizeRatio, bool bLumaOnly)
{
  (void)uLCUSize, (void)eOptions, (void)uMVVRange;

  uint32_t uSize = bLumaOnly ? GetRecFrameSizeLuma(tRoundedDim, pPicFormat->uBitDepth, pPicFormat->eStorageMode, pPicFormat->eChromaMode, uMaxRefBufSizeRatio)
                   : GetRecFrameSize(tRoundedDim, pPicFormat->uBitDepth, pPicFormat->eStorageMode, pPicFormat->eChromaMode, uMaxRefBufSizeRatio);

  return uSize;
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, AL_TPicFormat const* pPicFormat, uint8_t uLCUSize, AL_EChEncOption eOptions, uint16_t uMVVRange, uint8_t uMaxRefBufSizeRatio)
{
  AL_TDimension RoundedDim;
  RoundedDim.iHeight = AL_RoundUp(tDim.iHeight, 64);
  RoundedDim.iWidth = AL_RoundUp(tDim.iWidth, 64);

  return GetAllocSize_Ref(RoundedDim, pPicFormat, uLCUSize, eOptions, uMVVRange, uMaxRefBufSizeRatio, false);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncReferenceLuma(AL_TDimension tDim, AL_TPicFormat const* pPicFormat, uint8_t uLCUSize, AL_EChEncOption eOptions, uint16_t uMVVRange, uint8_t uMaxRefBufSizeRatio)
{
  AL_TDimension RoundedDim;
  RoundedDim.iHeight = AL_RoundUp(tDim.iHeight, 64);
  RoundedDim.iWidth = AL_RoundUp(tDim.iWidth, 64);

  return GetAllocSize_Ref(RoundedDim, pPicFormat, uLCUSize, eOptions, uMVVRange, uMaxRefBufSizeRatio, true);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_CompData(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt)
{
  uint32_t uBlk16x16 = GetSquareBlkNumber(tDim, 16);
  return AL_GetCompDataSize(uBlk16x16, uLog2MaxCuSize, uBitDepth, eChromaMode, bUseEnt);
}

/****************************************************************************/
uint32_t AL_GetAllocSize_EncCompMap(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uNumCore, bool bUseEnt)
{
  (void)uLog2MaxCuSize, (void)uNumCore, (void)bUseEnt;
  uint32_t uBlk16x16 = GetSquareBlkNumber(tDim, 16);
  return AL_RoundUp(SIZE_LCU_INFO * uBlk16x16, 32);
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_MV(AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec Codec)
{
  uint32_t uNumBlk = 0;
  int32_t iMul = (Codec == AL_CODEC_HEVC) ? 1 :
                 2;
  switch(uLog2MaxCuSize)
  {
  case 4: uNumBlk = GetSquareBlkNumber(tDim, 16);
    break;
  case 5: uNumBlk = GetSquareBlkNumber(tDim, 32) << 2;
    break;
  case 6: uNumBlk = GetSquareBlkNumber(tDim, 64) << 4;
    break;
  default: Rtos_Assert(false);
  }

  return MVBUFF_MV_OFFSET + ((uNumBlk * 4 * sizeof(uint32_t)) * iMul);
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_WPP(int32_t iLCUPicHeight, int32_t iNumSlices, uint8_t uNumCore)
{
  uint32_t uNumLinesPerCmd = (((iLCUPicHeight + iNumSlices - 1) / iNumSlices) + uNumCore - 1) / uNumCore;
  uint32_t uAlignedSize = AL_RoundUp(uNumLinesPerCmd * sizeof(uint32_t), 128) * uNumCore * iNumSlices;
  return uAlignedSize;
}

/*****************************************************************************/
uint32_t AL_GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uLog2MaxCuSize)
{
  int32_t iWidthInLcu = (uWidth + ((1 << uLog2MaxCuSize) - 1)) >> uLog2MaxCuSize;
  int32_t iHeightInLcu = (uHeight + ((1 << uLog2MaxCuSize) - 1)) >> uLog2MaxCuSize;
  uint32_t uSize = (uint32_t)Max(iWidthInLcu * iHeightInLcu * 32, iWidthInLcu * iHeightInLcu * sizeof(uint32_t) + uNumSlices * AL_ENC_NUM_CORES * 128);
  uint32_t uAlignedSize = AL_RoundUp(uSize, 32);
  return uAlignedSize;
}

/*****************************************************************************/
uint32_t GetAllocSize_StreamPart(AL_EProfile eProfile, int32_t iNumCores, int32_t iNumSlices, bool bSliceSize, int32_t iNumTilesPerCore)
{
  (void)eProfile;

  int32_t iMaxPart = bSliceSize ? AL_MAX_ENC_SLICE : iNumSlices;
  int32_t iNumNal = 16;

  uint32_t uStreamPartSize = ((iMaxPart * iNumCores * iNumTilesPerCore) + iNumNal) * sizeof(AL_TStreamPart);
  uStreamPartSize = AL_RoundUp(uStreamPartSize, 128);

  return uStreamPartSize;
}

/****************************************************************************/
void AL_FillPlaneDesc_EncReference(AL_TPlaneDescription* pPlaneDesc, AL_TDimension tDim, AL_TPicFormat const* pPicFormat, AL_ECodec eCodec, uint8_t uLCUSize, AL_EChEncOption eOptions, uint16_t uMVVRange, uint8_t uFbcMaxBufSizeRatio)
{
  (void)eCodec; // if no fbc support
  AL_EChEncOption eTmpOption = eOptions;

  AL_TDimension tTileDim = { 64, 4 };

  if(AL_Plane_IsPixelPlane(pPlaneDesc->ePlaneId))
  {
    pPlaneDesc->iPitch = AL_GetRecPitch(tTileDim, pPicFormat->uBitDepth, pPicFormat->eStorageMode, tDim.iWidth, AL_Plane_IsLumaPlane(pPlaneDesc->ePlaneId));
    pPlaneDesc->iOffset = 0;

    if(pPlaneDesc->ePlaneId != AL_PLANE_Y)
    {
      pPlaneDesc->iOffset = AL_GetAllocSize_EncReferenceLuma(tDim, pPicFormat, uLCUSize, eTmpOption, uMVVRange, uFbcMaxBufSizeRatio);
    }

    if(pPlaneDesc->ePlaneId == AL_PLANE_V)
    {
      pPlaneDesc->iOffset += (AL_GetAllocSize_EncReference(tDim, pPicFormat, uLCUSize, eTmpOption, uMVVRange, uFbcMaxBufSizeRatio) - AL_GetAllocSize_EncReferenceLuma(tDim, pPicFormat, uLCUSize, eTmpOption, uMVVRange, uFbcMaxBufSizeRatio)) / 2;
    }

    return;
  }

  Rtos_Assert(false);
}
