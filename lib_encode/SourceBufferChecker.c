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

#include "SourceBufferChecker.h"
#include "lib_common/BufferSrcMeta.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"

static uint32_t getSourceBufferSize(AL_TEncChanParam const* pChParam)
{
  AL_TDimension tDim = { pChParam->uWidth, pChParam->uHeight };
  return GetAllocSize_Src(tDim, AL_GET_BITDEPTH(pChParam->ePicFormat), AL_GET_CHROMA_MODE(pChParam->ePicFormat), pChParam->eSrcMode);
}

void AL_SrcBuffersChecker_Init(AL_TSrcBufferChecker* pCtx, AL_TEncChanParam const* pChParam)
{
  pCtx->width = pChParam->uWidth;
  pCtx->height = pChParam->uHeight;
  AL_TPicFormat const picFmt =
  {
    AL_GET_CHROMA_MODE(pChParam->ePicFormat),
    AL_GET_BITDEPTH(pChParam->ePicFormat),
    AL_GetSrcStorageMode(pChParam->eSrcMode)
  };
  pCtx->fourCC = AL_EncGetSrcFourCC(picFmt);
  pCtx->minimumSize = getSourceBufferSize(pChParam);
}

static int GetPitchYValue(int iWidth)
{
  return (iWidth + 31) & 0xFFFFFFE0; // 32 bytes alignment
}

static bool CheckMetaData(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  AL_TSrcMetaData* pMetaDataBuf = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  if(pMetaDataBuf == NULL)
    return false;

  if(pMetaDataBuf->tDim.iWidth > pMetaDataBuf->tPitches.iLuma)
    return false;

  if(pMetaDataBuf->tDim.iWidth != pCtx->width)
    return false;

  if(pMetaDataBuf->tDim.iHeight != pCtx->height)
    return false;

  if(pMetaDataBuf->tFourCC != pCtx->fourCC)
    return false;

  int iMinPitch;

  if((pMetaDataBuf->tFourCC == FOURCC(RX0A)) ||
     (pMetaDataBuf->tFourCC == FOURCC(RX2A)) ||
     (pMetaDataBuf->tFourCC == FOURCC(RXmA))
     )
    iMinPitch = GetPitchYValue((pCtx->width + 2) / 3 * 4);
  else
  iMinPitch = GetPitchYValue(pCtx->width);

  if(pMetaDataBuf->tPitches.iLuma < iMinPitch)
    return false;

  if(pMetaDataBuf->tPitches.iLuma % 32)
    return false;

  if(AL_GetChromaMode(pMetaDataBuf->tFourCC) != CHROMA_MONO)
  {
    if(pMetaDataBuf->tDim.iWidth > pMetaDataBuf->tPitches.iChroma)
      return false;

    if(pMetaDataBuf->tPitches.iChroma != pMetaDataBuf->tPitches.iLuma)
      return false;

    if((pMetaDataBuf->tOffsetYC.iLuma < pMetaDataBuf->tOffsetYC.iChroma) &&
       (pMetaDataBuf->tOffsetYC.iChroma < AL_SrcMetaData_GetLumaSize(pMetaDataBuf))
       )
      return false;

    if((pMetaDataBuf->tOffsetYC.iChroma < pMetaDataBuf->tOffsetYC.iLuma) &&
       (pMetaDataBuf->tOffsetYC.iLuma < AL_SrcMetaData_GetChromaSize(pMetaDataBuf))
       )
      return false;
  }

  return true;
}

bool AL_SrcBuffersChecker_CanBeUsed(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  if(pBuf == NULL)
    return false;

  if((int)pBuf->zSize < pCtx->minimumSize)
    return false;

  return CheckMetaData(pCtx, pBuf);
}

