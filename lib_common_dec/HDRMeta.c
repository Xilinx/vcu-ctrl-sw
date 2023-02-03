/******************************************************************************
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

