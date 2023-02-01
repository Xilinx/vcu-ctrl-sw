/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#include "PixMapBufferInternal.h"
#include "lib_common/BufferAPIInternal.h"

AL_TBuffer* AL_PixMapBuffer_Create(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack, AL_TDimension tDim, TFourCC tFourCC)
{
  AL_TBuffer* pBuf = AL_Buffer_CreateEmpty(pAllocator, pCallBack);

  if(pBuf == NULL)
    goto fail_buffer;

  AL_TPixMapMetaData* pMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);

  if(pMeta == NULL)
    goto fail_meta;

  if(!AL_Buffer_AddMetaData(pBuf, (AL_TMetaData*)pMeta))
    goto fail_add;

  pMeta->tDim = tDim;

  return pBuf;

  fail_add:
  AL_MetaData_Destroy((AL_TMetaData*)pMeta);
  fail_meta:
  AL_Buffer_Destroy(pBuf);
  fail_buffer:
  return NULL;
}

static void AddPlanesToMeta(AL_TPixMapMetaData* pMeta, int iChunkIdx, const AL_TPlaneDescription* pPlDesc, int iNbPlanes)
{
  for(int i = 0; i < iNbPlanes; i++, pPlDesc++)
  {
    AL_TPlane tPlane = { iChunkIdx, pPlDesc->iOffset, pPlDesc->iPitch };
    AL_PixMapMetaData_AddPlane(pMeta, tPlane, pPlDesc->ePlaneId);
  }
}

bool AL_PixMapBuffer_Allocate_And_AddPlanes(AL_TBuffer* pBuf, size_t zSize, const AL_TPlaneDescription* pPlDesc, int iNbPlanes, char const* name)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  int iChunkIdx = AL_Buffer_AllocateChunkNamed(pBuf, zSize, name);

  if(iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return false;

  AddPlanesToMeta(pMeta, iChunkIdx, pPlDesc, iNbPlanes);

  return true;
}

bool AL_PixMapBuffer_AddPlanes(AL_TBuffer* pBuf, AL_HANDLE hChunk, size_t zSize, const AL_TPlaneDescription* pPlDesc, int iNbPlanes)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  int iChunkIdx = AL_Buffer_AddChunk(pBuf, hChunk, zSize);

  if(iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return false;

  AddPlanesToMeta(pMeta, iChunkIdx, pPlDesc, iNbPlanes);

  return true;
}

uint8_t* AL_PixMapBuffer_GetPlaneAddress(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return NULL;

  uint8_t* pPlaneVADDR = AL_Buffer_GetDataChunk(pBuf, pMeta->tPlanes[ePlaneId].iChunkIdx);

  if(pPlaneVADDR == NULL)
    return NULL;

  return pPlaneVADDR + pMeta->tPlanes[ePlaneId].iOffset;
}

AL_PADDR AL_PixMapBuffer_GetPlanePhysicalAddress(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return 0;

  AL_PADDR tPlanePADDR = AL_Buffer_GetPhysicalAddressChunk(pBuf, pMeta->tPlanes[ePlaneId].iChunkIdx);

  if(tPlanePADDR == 0)
    return 0;

  return tPlanePADDR + pMeta->tPlanes[ePlaneId].iOffset;
}

int AL_PixMapBuffer_GetPlanePitch(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL || pMeta->tPlanes[ePlaneId].iChunkIdx == AL_BUFFER_BAD_CHUNK)
    return 0;

  return pMeta->tPlanes[ePlaneId].iPitch;
}

AL_TDimension AL_PixMapBuffer_GetDimension(AL_TBuffer const* pBuf)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
  {
    AL_TDimension tEmptyDim = { 0, 0 };
    return tEmptyDim;
  }

  return pMeta->tDim;
}

bool AL_PixMapBuffer_SetDimension(AL_TBuffer* pBuf, AL_TDimension tDim)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  pMeta->tDim = tDim;

  return true;
}

TFourCC AL_PixMapBuffer_GetFourCC(AL_TBuffer const* pBuf)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return 0;

  return pMeta->tFourCC;
}

bool AL_PixMapBuffer_SetFourCC(AL_TBuffer* pBuf, TFourCC tFourCC)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return false;

  pMeta->tFourCC = tFourCC;

  return true;
}

int AL_PixMapBuffer_GetPlaneChunkIdx(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  if(pMeta == NULL)
    return AL_BUFFER_BAD_CHUNK;

  return pMeta->tPlanes[ePlaneId].iChunkIdx;
}

uint32_t AL_PixMapBuffer_GetPositionOffset(AL_TBuffer const* pBuf, AL_TPosition tPos, AL_EPlaneId ePlaneId)
{
  AL_TPixMapMetaData* pMeta = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);

  uint32_t uPitch = pMeta->tPlanes[ePlaneId].iPitch;
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pMeta->tFourCC);
  uint8_t uBitdepth = AL_GetBitDepth(pMeta->tFourCC);

  if(ePlaneId != AL_PLANE_Y)
  {
    if(eChromaMode == AL_CHROMA_4_2_0)
      tPos.iY /= 2;

    if((eChromaMode == AL_CHROMA_4_2_0 || eChromaMode == AL_CHROMA_4_2_2) && (ePlaneId != AL_PLANE_UV))
      tPos.iX /= 2;
  }

  if(uBitdepth > 8)
  {
    return tPos.iY * uPitch + tPos.iX * 4 / 3;
  }
  else
    return tPos.iY * uPitch + tPos.iX;
}
