// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/BufferPictureMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool PictureMeta_Destroy(AL_TMetaData* pMeta)
{
  AL_TPictureMetaData* pPictureMeta = (AL_TPictureMetaData*)pMeta;
  Rtos_Free(pPictureMeta);
  return true;
}

AL_TPictureMetaData* AL_PictureMetaData_Clone(AL_TPictureMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_TPictureMetaData* pPictureMeta = AL_PictureMetaData_Create();

  if(!pPictureMeta)
    return NULL;
  pPictureMeta->eType = pMeta->eType;
  pPictureMeta->bSkipped = pMeta->bSkipped;
  return pPictureMeta;
}

static AL_TMetaData* PictureMeta_Clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_PictureMetaData_Clone((AL_TPictureMetaData*)pMeta);
}

AL_TPictureMetaData* AL_PictureMetaData_Create()
{
  AL_TPictureMetaData* pMeta = (AL_TPictureMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_PICTURE;
  pMeta->tMeta.MetaDestroy = PictureMeta_Destroy;
  pMeta->tMeta.MetaClone = PictureMeta_Clone;

  pMeta->eType = AL_SLICE_MAX_ENUM;
  pMeta->bSkipped = false;

  return pMeta;
}

