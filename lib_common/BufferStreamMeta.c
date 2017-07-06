/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include "lib_common/BufferStreamMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool StreamMeta_Destroy(AL_TMetaData* pMeta)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)pMeta;
  Rtos_Free(pStreamMeta->pSections);
  Rtos_Free(pMeta);
  return true;
}

AL_TStreamMetaData* AL_StreamMetaData_Create(uint16_t uMaxNumSection, uint32_t uMaxSize)
{
  AL_TStreamMetaData* pMeta;

  if(uMaxNumSection == 0)
    return NULL;

  pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_STREAM;
  pMeta->tMeta.MetaDestroy = StreamMeta_Destroy;

  pMeta->uOffset = 0;
  pMeta->uAvailSize = 0;
  pMeta->uMaxSize = uMaxSize;

  pMeta->uMaxNumSection = uMaxNumSection;
  pMeta->uFirstSection = 0;
  pMeta->uNumSection = 0;
  pMeta->uNumMissingSection = 0;

  pMeta->pSections = Rtos_Malloc(sizeof(AL_TStreamSection) * uMaxNumSection);

  if(!pMeta->pSections)
    goto fail_alloc_section;

  return pMeta;

  fail_alloc_section:
  Rtos_Free(pMeta);
  return NULL;
}

AL_TStreamMetaData* AL_StreamMetaData_Clone(AL_TStreamMetaData* pMeta)
{
  return AL_StreamMetaData_Create(pMeta->uMaxNumSection, pMeta->uMaxSize);
}

