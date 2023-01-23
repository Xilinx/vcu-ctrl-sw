/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

