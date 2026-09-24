// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncSchedulerCommon.h"

#include "lib_common/Profiles.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/IpEncFourCC.h"

void SetChannelInfo(AL_TCommonChannelInfo* pChanInfo, const AL_TEncChanParam* pChParam)
{
  AL_TDimension tDim = { pChParam->uEncWidth, pChParam->uEncHeight };

  bool bComp = false;

  uint32_t uFbcMaxBufSizeRatio = 0;

  pChanInfo->tRecPicFormat = AL_EncGetRecPicFormat(AL_GET_CHROMA_MODE(pChParam->ePicFormat), AL_GET_BITDEPTH(pChParam->ePicFormat), bComp, pChParam->eRecStorageMode);
  pChanInfo->RecFourCC = AL_GetFourCC(&pChanInfo->tRecPicFormat);

  pChanInfo->eCodec = AL_GET_CODEC(pChParam->eProfile);

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  pChanInfo->iNbPlanes = AL_Plane_GetBufferPlanes(pChanInfo->tRecPicFormat, usedPlanes);

  for(int32_t iPlane = 0; iPlane < pChanInfo->iNbPlanes; iPlane++)
  {
    AL_TPlaneDescription* pPlaneDesc = &pChanInfo->tPlanesDesc[iPlane];
    pPlaneDesc->ePlaneId = usedPlanes[iPlane];
    AL_FillPlaneDesc_EncReference(pPlaneDesc, tDim, &pChanInfo->tRecPicFormat, pChanInfo->eCodec, 1 << pChParam->uLog2MaxCuSize, pChParam->eEncOptions, pChParam->uMVVRange, uFbcMaxBufSizeRatio);
  }

  pChanInfo->uRecPicSize = AL_GetAllocSize_EncReference(tDim, &pChanInfo->tRecPicFormat, 1 << pChParam->uLog2MaxCuSize, pChParam->eEncOptions, pChParam->uMVVRange, uFbcMaxBufSizeRatio);
}

void SetRecPic(AL_TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo)
{
  pRecPic->pBuf = AL_PixMapBuffer_Create(pAllocator, NULL, pRecInfo->tPicDim, pChanInfo->RecFourCC);
  AL_PixMapBuffer_AddPlanes(pRecPic->pBuf, hRecBuf, pChanInfo->uRecPicSize, &pChanInfo->tPlanesDesc[0], pChanInfo->iNbPlanes);
  pRecPic->tInfo = *pRecInfo;
}
