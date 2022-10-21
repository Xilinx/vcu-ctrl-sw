/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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
  AL_EChromaOrder eChromaOrder = eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA :
                                 (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR);

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
uint32_t AL_GetRefListOffsets(TRefListOffsets* pOffsets, AL_ECodec eCodec, AL_EChromaOrder eChromaOrder, uint32_t uAddrSize)
{
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  const int iNbPixPlanes = Max(2, AL_Plane_GetBufferPixelPlanes(eChromaOrder, usedPlanes));
  TRefListOffsets tOffsets;
  (void)eCodec;

  uint32_t uOffset = uAddrSize * MAX_REF * iNbPixPlanes; // size of RefList Buff Addrs

  tOffsets.uColocPocOffset = uOffset;
  uOffset += uAddrSize * MAX_REF; // size of coloc POCs Buff Addrs

  tOffsets.uColocMVOffset = uOffset;
  uOffset += uAddrSize * MAX_REF; // size of coloc MVs Buff Addrs

  tOffsets.uMapOffset = uOffset;
  uOffset += uAddrSize * MAX_REF * iNbPixPlanes;  // size of RefList Map Addr

  if(pOffsets)
    *pOffsets = tOffsets;

  return uOffset;
}

/*****************************************************************************/
int AL_DecGetAllocSize_Frame(AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, bool bFbCompression, AL_EFbStorageMode eFbStorageMode)
{
  (void)bFbCompression;

  uint32_t uAllocSizeY = AL_DecGetAllocSize_Frame_PixPlane(eFbStorageMode, tDim, iPitch, eChromaMode, AL_PLANE_Y);
  uint32_t uSize = uAllocSizeY + GetChromaAllocSize(eChromaMode, uAllocSizeY);

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

  bool bSuccess = AL_GetPicFormat(tFourCC, &tPicFormat);
  AL_Assert(bSuccess);

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

