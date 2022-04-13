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

#include "lib_common/BufferPixMapMeta.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/IpEncFourCC.h"

void SetChannelInfo(AL_TCommonChannelInfo* pChanInfo, AL_TEncChanParam* pChParam)
{
  uint32_t uBitDepth = AL_GET_BITDEPTH(pChParam->ePicFormat);
  uint32_t eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
  AL_TDimension tDim = { pChParam->uEncWidth, pChParam->uEncHeight };

  pChanInfo->bIsAvc = AL_IS_AVC(pChParam->eProfile);
  bool bComp = false;
  AL_EChEncOption eOptions = 0;

  pChanInfo->uRecSize = AL_GetAllocSize_EncReference(tDim, uBitDepth, 1 << pChParam->uLog2MaxCuSize, eChromaMode, eOptions, 0);
  AL_TPicFormat picRecFormat = AL_EncGetRecPicFormat(eChromaMode, uBitDepth, bComp);

  pChanInfo->RecFourCC = AL_GetFourCC(picRecFormat);
}

void SetRecPic(AL_TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo)
{
  pRecPic->pBuf = AL_PixMapBuffer_Create(pAllocator, NULL, pRecInfo->tPicDim, pChanInfo->RecFourCC);

  AL_TPicFormat tPicFormat;
  bool bSuccess = AL_GetPicFormat(pChanInfo->RecFourCC, &tPicFormat);
  AL_Assert(bSuccess);

  AL_TPlaneDescription tPlanesDesc[AL_PLANE_MAX_ENUM];
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPlanes(tPicFormat.eChromaOrder, tPicFormat.bCompressed, usedPlanes);

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    tPlanesDesc[iPlane].ePlaneId = usedPlanes[iPlane];
    AL_EncRecBuffer_FillPlaneDesc(&tPlanesDesc[iPlane], pRecInfo->tPicDim, tPicFormat.eChromaMode, tPicFormat.uBitDepth, pChanInfo->bIsAvc, 0, 0, 0);
  }

  AL_PixMapBuffer_AddPlanes(pRecPic->pBuf, hRecBuf, pChanInfo->uRecSize, &tPlanesDesc[0], iNbPlanes);

  pRecPic->tInfo = *pRecInfo;
}
