/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include "ISchedulerCommon.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "lib_common_enc/EncBuffersInternal.h"

void SetChannelInfo(AL_TCommonChannelInfo* pChanInfo, AL_TEncChanParam* pChParam)
{
  uint32_t uBitDepth = AL_GET_BITDEPTH(pChParam->ePicFormat);
  uint32_t eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
  bool bComp = false;
  pChanInfo->uWidth = pChParam->uWidth;
  pChanInfo->uHeight = pChParam->uHeight;
  AL_TDimension tDim = { pChParam->uWidth, pChParam->uHeight };
  pChanInfo->uRecSizeY = AL_GetAllocSize_EncReference(tDim, uBitDepth, CHROMA_MONO, 0);
  pChanInfo->uRecSize = AL_GetAllocSize_EncReference(tDim, uBitDepth, eChromaMode, bComp);
  AL_TPicFormat picRecFormat = { eChromaMode, uBitDepth, AL_FB_TILE_64x4 };
  pChanInfo->RecFourCC = AL_GetRecFourCC(picRecFormat);
  pChanInfo->uRecPitchY = AL_GetRecPitch(uBitDepth, pChParam->uWidth);
  pChanInfo->uRecPitchC = pChanInfo->uRecPitchY;
}

static void setRecChannelWideInfo(TRecPic* pRecPic, AL_TCommonChannelInfo* pChanInfo)
{
  pRecPic->tBuf.iWidth = pChanInfo->uWidth;
  pRecPic->tBuf.iHeight = pChanInfo->uHeight;
  pRecPic->tBuf.iPitchY = pChanInfo->uRecPitchY;
  pRecPic->tBuf.iPitchC = pChanInfo->uRecPitchC;
  pRecPic->tBuf.tFourCC = pChanInfo->RecFourCC;
  pRecPic->tBuf.tMD.uSize = pChanInfo->uRecSize;
  pRecPic->tBuf.tOffsetYC.iLuma = 0;
  pRecPic->tBuf.tOffsetYC.iChroma = pChanInfo->uRecSizeY;
}

static void setRecSpecificInfo(TRecPic* pRecPic, AL_TReconstructedInfo* pRecInfo)
{
  pRecPic->ePicStruct = pRecInfo->ePicStruct;
  pRecPic->iPOC = pRecInfo->iPOC;
  pRecPic->tBuf.tMD.pAllocator = (AL_TAllocator*)(uintptr_t)pRecInfo->uID;
}

static void setRecMemoryRegion(TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf)
{
  pRecPic->tBuf.tMD.pVirtualAddr = AL_Allocator_GetVirtualAddr(pAllocator, hRecBuf);
  pRecPic->tBuf.tMD.uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pAllocator, hRecBuf);
  pRecPic->tBuf.tMD.hAllocBuf = hRecBuf;
}

void SetRecPic(TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo)
{
  setRecChannelWideInfo(pRecPic, pChanInfo);
  setRecSpecificInfo(pRecPic, pRecInfo);
  setRecMemoryRegion(pRecPic, pAllocator, hRecBuf);
}

