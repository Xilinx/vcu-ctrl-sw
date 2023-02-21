// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/HDRMeta.h"
#include "lib_rtos/lib_rtos.h"

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_THDRMetaData* pNewMeta = AL_HDRMetaData_Create();

  if(!pNewMeta)
    return NULL;

  AL_HDRMetaData_Copy((AL_THDRMetaData*)pMeta, pNewMeta);

  return (AL_TMetaData*)pNewMeta;
}

static bool destroy(AL_TMetaData* pMeta)
{
  AL_THDRMetaData* pHDRMeta = (AL_THDRMetaData*)pMeta;
  Rtos_Free(pHDRMeta);
  return true;
}

AL_THDRMetaData* AL_HDRMetaData_Create(void)
{
  AL_THDRMetaData* pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_HDR;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;

  AL_HDRMetaData_Reset(pMeta);

  return pMeta;
}

void AL_HDRMetaData_Reset(AL_THDRMetaData* pMeta)
{
  pMeta->eColourDescription = AL_COLOUR_DESC_UNSPECIFIED;
  pMeta->eTransferCharacteristics = AL_TRANSFER_CHARAC_UNSPECIFIED;
  pMeta->eColourMatrixCoeffs = AL_COLOUR_MAT_COEFF_UNSPECIFIED;
  AL_HDRSEIs_Reset(&pMeta->tHDRSEIs);
}

void AL_HDRMetaData_Copy(AL_THDRMetaData* pMetaSrc, AL_THDRMetaData* pMetaDst)
{
  *pMetaDst = *pMetaSrc;
}

