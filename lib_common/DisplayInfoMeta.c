// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/DisplayInfoMeta.h"
#include "lib_rtos/lib_rtos.h"

AL_TDisplayInfoMetaData* AL_DisplayInfoMetaData_Clone(AL_TDisplayInfoMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_TDisplayInfoMetaData* pNewMeta = AL_DisplayInfoMetaData_Create();

  if(!pNewMeta)
    return NULL;

  AL_DisplayInfoMetaData_Copy((AL_TDisplayInfoMetaData*)pMeta, pNewMeta);

  return pNewMeta;
}

static AL_TMetaData* SrcMeta_Clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_DisplayInfoMetaData_Clone((AL_TDisplayInfoMetaData*)pMeta);
}

static bool destroy(AL_TMetaData* pMeta)
{
  AL_TDisplayInfoMetaData* pDisplayInfoMeta = (AL_TDisplayInfoMetaData*)pMeta;
  Rtos_Free(pDisplayInfoMeta);
  return true;
}

AL_TDisplayInfoMetaData* AL_DisplayInfoMetaData_Create(void)
{
  AL_TDisplayInfoMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_DISPLAY_INFO;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = SrcMeta_Clone;

  AL_DisplayInfoMetaData_Reset(pMeta);

  return pMeta;
}

void AL_DisplayInfoMetaData_Reset(AL_TDisplayInfoMetaData* pMeta)
{
  pMeta->ePicStruct = AL_PS_FRM;
  pMeta->tCrop.bCropping = false;
  pMeta->tCrop.uCropOffsetBottom = 0;
  pMeta->tCrop.uCropOffsetLeft = 0;
  pMeta->tCrop.uCropOffsetRight = 0;
  pMeta->tCrop.uCropOffsetTop = 0;
  pMeta->uCrc = 0;
  pMeta->uStreamBitDepthC = 8;
  pMeta->uStreamBitDepthY = 8;
  pMeta->eOutputID = AL_OUTPUT_MAIN;
  pMeta->tPos.iX = 0;
  pMeta->tPos.iY = 0;
}

void AL_DisplayInfoMetaData_Copy(AL_TDisplayInfoMetaData* pMetaSrc, AL_TDisplayInfoMetaData* pMetaDst)
{
  *pMetaDst = *pMetaSrc;
}
