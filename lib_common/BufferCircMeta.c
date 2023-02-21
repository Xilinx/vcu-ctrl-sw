// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/lib_rtos.h"
#include "BufferCircMeta.h"

static bool destroy(AL_TMetaData* pMeta)
{
  Rtos_Free(pMeta);
  return true;
}

AL_TCircMetaData* AL_CircMetaData_Clone(AL_TCircMetaData* pMeta)
{
  return AL_CircMetaData_Create(pMeta->iOffset, pMeta->iAvailSize, pMeta->bLastBuffer);
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_CircMetaData_Clone((AL_TCircMetaData*)pMeta);
}

AL_TCircMetaData* AL_CircMetaData_Create(int32_t iOffset, int32_t iAvailSize, bool bLastBuffer)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_CIRCULAR;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;
  pMeta->iOffset = iOffset;
  pMeta->iAvailSize = iAvailSize;
  pMeta->bLastBuffer = bLastBuffer;

  return pMeta;
}

