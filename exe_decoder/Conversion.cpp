// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

