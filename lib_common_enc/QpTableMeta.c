// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_enc/QpTableMeta.h"
#include "lib_common/Utils.h"

#include "lib_rtos/lib_rtos.h"

static bool destroy(AL_TMetaData* pBaseMeta)
{
  AL_TQpTableMetaData* pMeta = (AL_TQpTableMetaData*)pBaseMeta;

  Rtos_Free(pMeta);
  return true;
}

static AL_TMetaData* clone(AL_TMetaData const* pBaseMeta)
{
  if(!pBaseMeta)
    return NULL;

  AL_TQpTableMetaData const* pMeta = (AL_TQpTableMetaData const*)pBaseMeta;
  AL_TQpTableMetaData* pNewMeta = (AL_TQpTableMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pNewMeta)
    return NULL;

  pNewMeta->tMeta = pMeta->tMeta;

  for(int32_t i = 0; i < ARRAY_SIZE(pMeta->tQpTable); ++i)
    pNewMeta->tQpTable[i] = pMeta->tQpTable[i];

  return (AL_TMetaData*)pNewMeta;
}

AL_TQpTableMetaData* AL_QpTableMetaData_Create(void)
{
  AL_TQpTableMetaData* pMeta = (AL_TQpTableMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_QP_TABLE;
  pMeta->tMeta.MetaClone = clone;
  pMeta->tMeta.MetaDestroy = destroy;

  for(int32_t i = 0; i < ARRAY_SIZE(pMeta->tQpTable); ++i)
  {
    pMeta->tQpTable[i].iChunkIdx = 0;
    pMeta->tQpTable[i].uOffset = 0;
  }

  return pMeta;
}
