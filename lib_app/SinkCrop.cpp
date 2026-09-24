// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/SinkCrop.hpp"

#include <stdexcept>
#include <memory>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/DisplayInfoMeta.h"
#include "lib_rtos/lib_rtos.h"
}

SinkCrop::SinkCrop(std::unique_ptr<IFrameSink>& pSink, AL_TCropInfo* pCropInfo) :
  m_pSink(std::move(pSink))
{
  Rtos_Assert(m_pSink != nullptr);

  if(pCropInfo)
  {
    m_bFixedCrop = true;
    m_tCrop = *pCropInfo;
  }
}

static int32_t GetPixSize(AL_TBuffer* pBuf)
{
  AL_TPicFormat tPicFormat;

  if(!AL_GetPicFormat(AL_PixMapBuffer_GetFourCC(pBuf), &tPicFormat))
    throw std::runtime_error("Unknow picture format");

  if(tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED)
    return (tPicFormat.uBitDepth == 8 || (tPicFormat.uBitDepth == 10 && tPicFormat.eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED)) ? sizeof(uint32_t) : sizeof(AL_64U);
  else
    return tPicFormat.uBitDepth > 8 ? sizeof(uint16_t) : sizeof(uint8_t);
}

void SinkCrop::ProcessFrame(AL_TBuffer* pBuf)
{
  AL_TDisplayInfoMetaData* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DISPLAY_INFO));
  AL_TCropInfo& tCrop = (pMeta && !m_bFixedCrop) ? pMeta->tCrop : m_tCrop;

  int32_t iPixSize = GetPixSize(pBuf);

  if(tCrop.bCropping)
    ApplyCrop(pBuf, iPixSize, (int)tCrop.uCropOffsetLeft, (int)tCrop.uCropOffsetRight, (int)tCrop.uCropOffsetTop, (int)tCrop.uCropOffsetBottom);

  m_pSink->ProcessFrame(pBuf);

  if(tCrop.bCropping)
    ApplyCrop(pBuf, iPixSize, -(int)tCrop.uCropOffsetLeft, -(int)tCrop.uCropOffsetRight, -(int)tCrop.uCropOffsetTop, -(int)tCrop.uCropOffsetBottom);
}

void SinkCrop::ApplyCrop(AL_TBuffer* pYUV, int32_t iPixSize, int32_t iLeft, int32_t iRight, int32_t iTop, int32_t iBottom)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pYUV, AL_META_TYPE_PIXMAP);

  pMeta->tDim.iWidth -= iLeft + iRight;
  pMeta->tDim.iHeight -= iTop + iBottom;

  pMeta->tPlanes[AL_PLANE_Y].iOffset += iTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_Y) + iLeft * iPixSize;

  AL_EChromaMode eChromaMode = AL_GetChromaMode(pMeta->tFourCC);

  if(eChromaMode != AL_CHROMA_MONO)
  {
    AL_EPlaneMode ePlaneMode = AL_GetPlaneMode(pMeta->tFourCC);

    iTop /= (eChromaMode == AL_CHROMA_4_2_0) ? 2 : 1;
    iLeft /= (eChromaMode == AL_CHROMA_4_4_4) ? 1 : 2;

    if(ePlaneMode == AL_PLANE_MODE_PLANAR)
    {
      pMeta->tPlanes[AL_PLANE_U].iOffset += iTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_U) + iLeft * iPixSize;
      pMeta->tPlanes[AL_PLANE_V].iOffset += iTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_V) + iLeft * iPixSize;
    }
    else if(ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
    {
      pMeta->tPlanes[AL_PLANE_UV].iOffset += iTop * AL_PixMapBuffer_GetPlanePitch(pYUV, AL_PLANE_UV) + iLeft * iPixSize * 2;
    }
  }
}
