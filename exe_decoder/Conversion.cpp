/******************************************************************************
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

#include <cstring>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

#include "Conversion.h"

void CropFrame(AL_TBuffer* pYUV, int iSizePix, int32_t iCropLeft, int32_t iCropRight, int32_t iCropTop, int32_t iCropBottom)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pYUV, AL_META_TYPE_PIXMAP);

  pMeta->tDim.iWidth -= iCropLeft + iCropRight;
  pMeta->tDim.iHeight -= iCropTop + iCropBottom;

  pMeta->tPlanes[AL_PLANE_Y].iOffset += iCropTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_Y) + iCropLeft * iSizePix;

  AL_EChromaMode eMode = AL_GetChromaMode(pMeta->tFourCC);

  if(eMode != AL_CHROMA_MONO)
  {
    iCropTop /= (eMode == AL_CHROMA_4_2_0) ? 2 : 1;
    iCropLeft /= (eMode == AL_CHROMA_4_4_4) ? 1 : 2;
    pMeta->tPlanes[AL_PLANE_U].iOffset += iCropTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_U) + iCropLeft * iSizePix;
    pMeta->tPlanes[AL_PLANE_V].iOffset += iCropTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_V) + iCropLeft * iSizePix;
  }
}

