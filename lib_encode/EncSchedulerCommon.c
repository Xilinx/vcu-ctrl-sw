/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "EncSchedulerCommon.h"

#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/IpEncFourCC.h"

void SetChannelInfo(AL_TCommonChannelInfo* pChanInfo, const AL_TEncChanParam* pChParam)
{
  AL_TDimension tDim = { pChParam->uEncWidth, pChParam->uEncHeight };

  bool bComp = false;

  pChanInfo->tRecPicFormat = AL_EncGetRecPicFormat(AL_GET_CHROMA_MODE(pChParam->ePicFormat), AL_GET_BITDEPTH(pChParam->ePicFormat), bComp);
  pChanInfo->RecFourCC = AL_GetFourCC(pChanInfo->tRecPicFormat);

  pChanInfo->bIsAvc = AL_IS_AVC(pChParam->eProfile);

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  pChanInfo->iNbPlanes = AL_Plane_GetBufferPlanes(pChanInfo->tRecPicFormat.eChromaOrder, pChanInfo->tRecPicFormat.bCompressed, usedPlanes);

  for(int iPlane = 0; iPlane < pChanInfo->iNbPlanes; iPlane++)
  {
    AL_TPlaneDescription* pPlaneDesc = &pChanInfo->tPlanesDesc[iPlane];
    pPlaneDesc->ePlaneId = usedPlanes[iPlane];
    AL_FillPlaneDesc_EncReference(pPlaneDesc, tDim, pChanInfo->tRecPicFormat.eChromaMode, pChanInfo->tRecPicFormat.uBitDepth, pChanInfo->bIsAvc, 1 << pChParam->uLog2MaxCuSize, pChParam->uMVVRange, pChParam->eEncOptions);
  }

  pChanInfo->uRecPicSize = AL_GetAllocSize_EncReference(tDim, pChanInfo->tRecPicFormat.uBitDepth, 1 << pChParam->uLog2MaxCuSize, pChanInfo->tRecPicFormat.eChromaMode, pChParam->eEncOptions, pChParam->uMVVRange);
}

void SetRecPic(AL_TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo)
{
  pRecPic->pBuf = AL_PixMapBuffer_Create(pAllocator, NULL, pRecInfo->tPicDim, pChanInfo->RecFourCC);
  AL_PixMapBuffer_AddPlanes(pRecPic->pBuf, hRecBuf, pChanInfo->uRecPicSize, &pChanInfo->tPlanesDesc[0], pChanInfo->iNbPlanes);
  pRecPic->tInfo = *pRecInfo;
}

