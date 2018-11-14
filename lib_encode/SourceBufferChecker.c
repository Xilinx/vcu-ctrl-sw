/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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
#include "lib_common/Utils.h"

static uint32_t getExpectedSourceBufferSize(AL_TSrcBufferChecker* pCtx, int pitch, int strideHeight)
{
  AL_TDimension tDim = { pCtx->width, pCtx->height };
  return AL_GetAllocSizeSrc(tDim, pCtx->picFmt.uBitDepth, pCtx->picFmt.eChromaMode, pCtx->srcMode, pitch, strideHeight);
}

void AL_SrcBuffersChecker_Init(AL_TSrcBufferChecker* pCtx, AL_TEncChanParam const* pChParam)
{
  pCtx->width = AL_GetSrcWidth(*pChParam);
  pCtx->height = AL_GetSrcHeight(*pChParam);

  pCtx->picFmt = AL_EncGetSrcPicFormat(AL_GET_CHROMA_MODE(pChParam->ePicFormat), pChParam->uSrcBitDepth, AL_GetSrcStorageMode(pChParam->eSrcMode), AL_IsSrcCompressed(pChParam->eSrcMode));
  pCtx->fourCC = AL_GetFourCC(pCtx->picFmt);
  pCtx->srcMode = pChParam->eSrcMode;
}

static int GetPitchYValue(int iWidth)
{
  return (iWidth + 31) & 0xFFFFFFE0; // 32 bytes alignment
}

static bool CheckMetaData(AL_TSrcBufferChecker* pCtx, AL_TSrcMetaData* pMetaDataBuf)
{
  if(pMetaDataBuf == NULL)
    return false;

  if(pMetaDataBuf->tDim.iWidth > pMetaDataBuf->tPlanes[AL_PLANE_Y].iPitch)
    return false;

  if(pMetaDataBuf->tDim.iWidth != pCtx->width)
    return false;

  if(pMetaDataBuf->tDim.iHeight != pCtx->height)
    return false;

  if(pMetaDataBuf->tFourCC != pCtx->fourCC)
    return false;

  int iMinPitch;

  if((pMetaDataBuf->tFourCC == FOURCC(XV15)) ||
     (pMetaDataBuf->tFourCC == FOURCC(XV20)) ||
     (pMetaDataBuf->tFourCC == FOURCC(XV10))
     )
    iMinPitch = GetPitchYValue((pCtx->width + 2) / 3 * 4);
  else
  iMinPitch = GetPitchYValue(pCtx->width);

  if(pMetaDataBuf->tPlanes[AL_PLANE_Y].iPitch < iMinPitch)
    return false;

  if(pMetaDataBuf->tPlanes[AL_PLANE_Y].iPitch % 32)
    return false;

  if(AL_GetChromaMode(pMetaDataBuf->tFourCC) != CHROMA_MONO)
  {
    if(pMetaDataBuf->tDim.iWidth > pMetaDataBuf->tPlanes[AL_PLANE_UV].iPitch)
      return false;

    if(pMetaDataBuf->tPlanes[AL_PLANE_UV].iPitch != pMetaDataBuf->tPlanes[AL_PLANE_Y].iPitch)
      return false;

    if((pMetaDataBuf->tPlanes[AL_PLANE_Y].iOffset < pMetaDataBuf->tPlanes[AL_PLANE_UV].iOffset) &&
       (pMetaDataBuf->tPlanes[AL_PLANE_UV].iOffset < AL_SrcMetaData_GetLumaSize(pMetaDataBuf))
       )
      return false;

    if((pMetaDataBuf->tPlanes[AL_PLANE_UV].iOffset < pMetaDataBuf->tPlanes[AL_PLANE_Y].iOffset) &&
       (pMetaDataBuf->tPlanes[AL_PLANE_Y].iOffset < AL_SrcMetaData_GetChromaSize(pMetaDataBuf))
       )
      return false;
  }

  return true;
}

bool AL_SrcBuffersChecker_CanBeUsed(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  if(pBuf == NULL)
    return false;

  AL_TSrcMetaData* pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  if(!CheckMetaData(pCtx, pMeta))
    return false;

  int const iPitch = pMeta->tPlanes[AL_PLANE_Y].iPitch;
  // We assume the strideHeight used is the smallest one. The check could be irrelevant if your strideHeight is higher.
  int const strideHeight = RoundUp(pMeta->tDim.iHeight, 8);
  uint32_t const minSize = getExpectedSourceBufferSize(pCtx, iPitch, strideHeight);

  if(pBuf->zSize < minSize)
    return false;

  return true;
}

