/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

  pChanInfo->uRecSize = AL_GetAllocSize_EncReference(tDim, uBitDepth, eChromaMode, bComp);
  AL_TPicFormat picRecFormat = AL_EncGetRecPicFormat(eChromaMode, uBitDepth, bComp);

  pChanInfo->RecFourCC = AL_GetFourCC(picRecFormat);
}

void SetRecPic(AL_TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo)
{
  pRecPic->pBuf = AL_PixMapBuffer_Create(pAllocator, NULL, pRecInfo->tPicDim, pChanInfo->RecFourCC);

  AL_EChromaMode eChromaMode = AL_GetChromaMode(pChanInfo->RecFourCC);
  uint8_t uBitDepth = AL_GetBitDepth(pChanInfo->RecFourCC);
  bool bIsComp = AL_IsCompressed(pChanInfo->RecFourCC);

  // Following code assumes continuous AL_EPlaneId enum. Also, monochrome should be properly handled.
  AL_TPlaneDescription tPlanesDesc[AL_PLANE_MAX_ENUM];
  int iNbPlanes = (int)(bIsComp ? AL_PLANE_MAP_UV : AL_PLANE_UV) + 1;

  for(int iCurrentPlane = 0; iCurrentPlane < iNbPlanes; iCurrentPlane++)
  {
    tPlanesDesc[iCurrentPlane].ePlaneId = (AL_EPlaneId)iCurrentPlane;
    AL_EncRecBuffer_FillPlaneDesc(&tPlanesDesc[iCurrentPlane], pRecInfo->tPicDim, eChromaMode, uBitDepth, pChanInfo->bIsAvc);
  }

  AL_PixMapBuffer_AddPlanes(pRecPic->pBuf, hRecBuf, pChanInfo->uRecSize, &tPlanesDesc[0], iNbPlanes);

  pRecPic->tInfo = *pRecInfo;
}
