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

