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

#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferPixMapMeta.h"
#include <assert.h>

static bool SrcMeta_Destroy(AL_TMetaData* pMeta)
{
  Rtos_Free(pMeta);
  return true;
}

AL_TPixMapMetaData* AL_PixMapMetaData_Clone(AL_TPixMapMetaData* pMeta)
{
  AL_TPixMapMetaData* pClone = AL_PixMapMetaData_Create(pMeta->tDim, pMeta->tPlanes[AL_PLANE_Y], pMeta->tPlanes[AL_PLANE_UV], pMeta->tFourCC);

  for(int iId = AL_PLANE_MAP_Y; iId < AL_PLANE_MAX_ENUM; ++iId)
    AL_PixMapMetaData_AddPlane(pClone, pMeta->tPlanes[iId], iId);

  return pClone;
}

static AL_TMetaData* SrcMeta_Clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_PixMapMetaData_Clone((AL_TPixMapMetaData*)pMeta);
}

void AL_PixMapMetaData_AddPlane(AL_TPixMapMetaData* pMeta, AL_TPlane tPlane, AL_EPlaneId ePlaneId)
{
  pMeta->tPlanes[ePlaneId] = tPlane;
}

AL_TPixMapMetaData* AL_PixMapMetaData_CreateEmpty(TFourCC tFourCC)
{
  AL_TPixMapMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_PIXMAP;
  pMeta->tMeta.MetaDestroy = SrcMeta_Destroy;
  pMeta->tMeta.MetaClone = SrcMeta_Clone;

  pMeta->tDim.iWidth = 0;
  pMeta->tDim.iHeight = 0;

  AL_TPlane tEmptyPlane = { -1, 0, 0 };

  for(int iId = AL_PLANE_Y; iId < AL_PLANE_MAX_ENUM; ++iId)
    AL_PixMapMetaData_AddPlane(pMeta, tEmptyPlane, iId);

  pMeta->tFourCC = tFourCC;

  return pMeta;
}

AL_TPixMapMetaData* AL_PixMapMetaData_Create(AL_TDimension tDim, AL_TPlane tYPlane, AL_TPlane tUVPlane, TFourCC tFourCC)
{
  AL_TPixMapMetaData* pMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);

  if(!pMeta)
    return NULL;

  pMeta->tDim = tDim;

  AL_PixMapMetaData_AddPlane(pMeta, tYPlane, AL_PLANE_Y);
  AL_PixMapMetaData_AddPlane(pMeta, tUVPlane, AL_PLANE_UV);

  return pMeta;
}

int AL_PixMapMetaData_GetOffsetY(AL_TPixMapMetaData* pMeta)
{
  assert(pMeta->tPlanes[AL_PLANE_Y].iOffset <= pMeta->tPlanes[AL_PLANE_UV].iOffset);
  return pMeta->tPlanes[AL_PLANE_Y].iOffset;
}

int AL_PixMapMetaData_GetOffsetUV(AL_TPixMapMetaData* pMeta)
{
  assert(pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight <= pMeta->tPlanes[AL_PLANE_UV].iOffset ||
         (AL_IsTiled(pMeta->tFourCC) &&
          (pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight / 4 <= pMeta->tPlanes[AL_PLANE_UV].iOffset)));
  return pMeta->tPlanes[AL_PLANE_UV].iOffset;
}

int AL_PixMapMetaData_GetLumaSize(AL_TPixMapMetaData* pMeta)
{
  if(AL_IsTiled(pMeta->tFourCC))
    return pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight / 4;
  return pMeta->tPlanes[AL_PLANE_Y].iPitch * pMeta->tDim.iHeight;
}

int AL_PixMapMetaData_GetChromaSize(AL_TPixMapMetaData* pMeta)
{
  AL_EChromaMode eCMode = AL_GetChromaMode(pMeta->tFourCC);

  if(eCMode == AL_CHROMA_MONO)
    return 0;

  int const iHeightC = (eCMode == AL_CHROMA_4_2_0) ? pMeta->tDim.iHeight / 2 : pMeta->tDim.iHeight;

  if(AL_IsTiled(pMeta->tFourCC))
    return pMeta->tPlanes[AL_PLANE_UV].iPitch * iHeightC / 4;

  if(AL_IsSemiPlanar(pMeta->tFourCC))
    return pMeta->tPlanes[AL_PLANE_UV].iPitch * iHeightC;

  return pMeta->tPlanes[AL_PLANE_UV].iPitch * iHeightC * 2;
}
