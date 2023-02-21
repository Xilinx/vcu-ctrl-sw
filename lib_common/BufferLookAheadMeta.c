// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/BufferLookAheadMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool LookAheadMeta_Destroy(AL_TMetaData* pMeta)
{
  AL_TLookAheadMetaData* pLookAheadMeta = (AL_TLookAheadMetaData*)pMeta;
  Rtos_Free(pLookAheadMeta);
  return true;
}

AL_TLookAheadMetaData* AL_LookAheadMetaData_Clone(AL_TLookAheadMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_TLookAheadMetaData* pLookAheadMeta = AL_LookAheadMetaData_Create();

  if(!pLookAheadMeta)
    return NULL;
  AL_LookAheadMetaData_Copy(pMeta, pLookAheadMeta);
  return pLookAheadMeta;
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_LookAheadMetaData_Clone((AL_TLookAheadMetaData*)pMeta);
}

AL_TLookAheadMetaData* AL_LookAheadMetaData_Create()
{
  AL_TLookAheadMetaData* pMeta = (AL_TLookAheadMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_LOOKAHEAD;
  pMeta->tMeta.MetaDestroy = LookAheadMeta_Destroy;
  pMeta->tMeta.MetaClone = clone;

  AL_LookAheadMetaData_Reset(pMeta);

  return pMeta;
}

void AL_LookAheadMetaData_Copy(AL_TLookAheadMetaData* pMetaSrc, AL_TLookAheadMetaData* pMetaDest)
{
  if(!pMetaSrc || !pMetaDest)
    return;

  pMetaDest->iPictureSize = pMetaSrc->iPictureSize;
  pMetaDest->eSceneChange = pMetaSrc->eSceneChange;
  pMetaDest->iIPRatio = pMetaSrc->iIPRatio;
  pMetaDest->iComplexity = pMetaSrc->iComplexity;
  pMetaDest->iTargetLevel = pMetaSrc->iTargetLevel;

  for(int i = 0; i < 5; i++)
    pMetaDest->iPercentIntra[i] = pMetaSrc->iPercentIntra[i];
}

void AL_LookAheadMetaData_Reset(AL_TLookAheadMetaData* pMeta)
{
  pMeta->iPictureSize = -1;
  pMeta->eSceneChange = AL_SC_NONE;
  pMeta->iIPRatio = 1000;
  pMeta->iComplexity = 1000;
  pMeta->iTargetLevel = -1;

  for(int8_t i = 0; i < 5; i++)
    pMeta->iPercentIntra[i] = -1;
}

