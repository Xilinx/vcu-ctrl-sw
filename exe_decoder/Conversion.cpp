// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <cstring>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

#include "Conversion.h"

void CropFrame(AL_TBuffer* pYUV, int iSizePix, AL_TCropInfo tCrop)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pYUV, AL_META_TYPE_PIXMAP);

  pMeta->tDim.iWidth -= tCrop.uCropOffsetLeft + tCrop.uCropOffsetRight;
  pMeta->tDim.iHeight -= tCrop.uCropOffsetTop + tCrop.uCropOffsetBottom;

  pMeta->tPlanes[AL_PLANE_Y].iOffset += tCrop.uCropOffsetTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_Y) + tCrop.uCropOffsetLeft * iSizePix;

  AL_EChromaMode eMode = AL_GetChromaMode(pMeta->tFourCC);

  if(eMode != AL_CHROMA_MONO)
  {
    tCrop.uCropOffsetTop /= (eMode == AL_CHROMA_4_2_0) ? 2 : 1;
    tCrop.uCropOffsetLeft /= (eMode == AL_CHROMA_4_4_4) ? 1 : 2;
    pMeta->tPlanes[AL_PLANE_U].iOffset += tCrop.uCropOffsetTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_U) + tCrop.uCropOffsetLeft * iSizePix;
    pMeta->tPlanes[AL_PLANE_V].iOffset += tCrop.uCropOffsetTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_V) + tCrop.uCropOffsetLeft * iSizePix;
  }
}

