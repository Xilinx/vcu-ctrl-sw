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

#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferSrcMeta.h"
#include <assert.h>

static bool SrcMeta_Destroy(AL_TMetaData* pMeta)
{
  Rtos_Free(pMeta);
  return true;
}

AL_TSrcMetaData* AL_SrcMetaData_Create(AL_TDimension tDim, AL_TPlane tYPlane, AL_TPlane tUVPlane, TFourCC tFourCC)
{
  AL_TSrcMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_SOURCE;
  pMeta->tMeta.MetaDestroy = SrcMeta_Destroy;

  pMeta->tDim = tDim;

  pMeta->tPlanes[AL_PLANE_Y] = tYPlane;
  pMeta->tPlanes[AL_PLANE_UV] = tUVPlane;

  AL_TPlane tEmptyPlane = { 0, 0 };

  for(int iId = AL_PLANE_MAP_Y; iId < AL_PLANE_MAX_ENUM; ++iId)
    pMeta->tPlanes[iId] = tEmptyPlane;

  pMeta->tFourCC = tFourCC;

  return pMeta;
}

void AL_SrcMetaData_AddPlane(AL_TSrcMetaData* pMeta, AL_TPlane tPlane, AL_EPlaneId ePlaneId)
{
  pMeta->tPlanes[ePlaneId] = tPlane;
}

AL_TSrcMetaData* AL_SrcMetaData_Clone(AL_TSrcMetaData* pMeta)
{
  AL_TSrcMetaData* pClone = AL_SrcMetaData_Create(pMeta->tDim, pMeta->tPlanes[AL_PLANE_Y], pMeta->tPlanes[AL_PLANE_UV], pMeta->tFourCC);

  for(int iId = AL_PLANE_MAP_Y; iId < AL_PLANE_MAX_ENUM; ++iId)
    AL_SrcMetaData_AddPlane(pClone, pMeta->tPlanes[iId], iId);

  return pClone;
}

int AL_SrcMetaData_GetOffsetY(AL_TSrcMetaData* pMeta)
{
  assert(pMeta->tPlanes[AL_PLANE_Y].iOffset <= pMeta->tPlanes[AL_PLANE_UV].iOffset);
  return pMeta->tPlanes[AL_PLANE_Y].iOffset;
}

int AL_SrcMetaData_GetOffsetUV(AL_TSrcMetaData* pMeta)
{
  assert(pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight <= pMeta->tPlanes[AL_PLANE_UV].iOffset ||
         (AL_IsTiled(pMeta->tFourCC) &&
          (pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight / 4 <= pMeta->tPlanes[AL_PLANE_UV].iOffset)));
  return pMeta->tPlanes[AL_PLANE_UV].iOffset;
}

int AL_SrcMetaData_GetLumaSize(AL_TSrcMetaData* pMeta)
{
  if(AL_IsTiled(pMeta->tFourCC))
    return pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight / 4;
  return pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight;
}

int AL_SrcMetaData_GetChromaSize(AL_TSrcMetaData* pMeta)
{
  AL_EChromaMode eCMode = AL_GetChromaMode(pMeta->tFourCC);

  if(eCMode == CHROMA_MONO)
    return 0;

  int const iHeightC = (eCMode == CHROMA_4_2_0) ? pMeta->tDim.iHeight / 2 : pMeta->tDim.iHeight;

  if(AL_IsTiled(pMeta->tFourCC))
    return pMeta->tPlanes[AL_PLANE_UV].iPitch * iHeightC / 4;
  return pMeta->tPlanes[AL_PLANE_UV].iPitch * iHeightC * 2;
}

