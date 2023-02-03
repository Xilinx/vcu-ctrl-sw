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

