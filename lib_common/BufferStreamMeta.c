/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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
#include <assert.h>

static bool StreamMeta_Destroy(AL_TMetaData* pMeta)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)pMeta;
  Rtos_Free(pStreamMeta->pSections);
  Rtos_Free(pMeta);
  return true;
}

AL_TStreamMetaData* AL_StreamMetaData_Create(uint16_t uMaxNumSection)
{
  AL_TStreamMetaData* pMeta;

  if(uMaxNumSection == 0)
    return NULL;

  pMeta = Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_STREAM;
  pMeta->tMeta.MetaDestroy = StreamMeta_Destroy;

  pMeta->uMaxNumSection = uMaxNumSection;
  pMeta->uNumSection = 0;

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
  return AL_StreamMetaData_Create(pMeta->uMaxNumSection);
}

static void SetSection(AL_TStreamSection* pSections, uint16_t uSectionID, uint32_t uOffset, uint32_t uLength, uint32_t uFlags)
{
  pSections[uSectionID].uOffset = uOffset;
  pSections[uSectionID].uLength = uLength;
  pSections[uSectionID].uFlags = uFlags;
}

int AL_StreamMetaData_AddSection(AL_TStreamMetaData* pMetaData, uint32_t uOffset, uint32_t uLength, uint32_t uFlags)
{
  uint16_t uSectionID = pMetaData->uNumSection;

  if(!pMetaData || uSectionID >= pMetaData->uMaxNumSection)
    return -1;

  SetSection(pMetaData->pSections, uSectionID, uOffset, uLength, uFlags);
  ++pMetaData->uNumSection;

  return uSectionID;
}

/****************************************************************************/
void AL_StreamMetaData_ChangeSection(AL_TStreamMetaData* pMetaData, uint16_t uSectionID, uint32_t uOffset, uint32_t uLength)
{
  assert(pMetaData && uSectionID < pMetaData->uNumSection);

  pMetaData->pSections[uSectionID].uOffset = uOffset;
  pMetaData->pSections[uSectionID].uLength = uLength;
}

/****************************************************************************/
void AL_StreamMetaData_SetSectionFlags(AL_TStreamMetaData* pMetaData, uint16_t uSectionID, uint32_t uFlags)
{
  assert(pMetaData && uSectionID < pMetaData->uNumSection);
  pMetaData->pSections[uSectionID].uFlags = uFlags;
}

/****************************************************************************/
void AL_StreamMetaData_ClearAllSections(AL_TStreamMetaData* pMetaData)
{
  assert(pMetaData);
  pMetaData->uNumSection = 0;
}

static int FindLastConfigSection(AL_TStreamMetaData* pMetaData)
{
  AL_TStreamSection* pSections = pMetaData->pSections;
  int configSectionId = pMetaData->uNumSection - 1;

  while(configSectionId >= 0)
  {
    if(pSections[configSectionId].uFlags & SECTION_CONFIG_FLAG)
      break;
    --configSectionId;
  }

  /* configSectionId == -1 if we didn't find any config sections */

  return configSectionId;
}

static int InsertSectionAtId(AL_TStreamMetaData* pMetaData, uint16_t targetId, uint32_t uOffset, uint32_t uLength, uint32_t uFlags)
{
  uint16_t uNumSection = pMetaData->uNumSection;
  AL_TStreamSection* pSections = pMetaData->pSections;

  if(!pMetaData || uNumSection >= pMetaData->uMaxNumSection)
    return -1;

  for(int i = uNumSection - 1; i >= (int)targetId; --i)
  {
    AL_TStreamSection* cur = &pSections[i];
    SetSection(pSections, i + 1, cur->uOffset, cur->uLength, cur->uFlags);
  }

  SetSection(pSections, targetId, uOffset, uLength, uFlags);
  ++pMetaData->uNumSection;

  return targetId;
}

int AddPrefixSei(AL_TStreamMetaData* pMetaData, uint32_t uOffset, uint32_t uLength)
{
  // the prefix sei needs to be inserted after a config section if it exists
  int seiSectionId = FindLastConfigSection(pMetaData) + 1;
  return InsertSectionAtId(pMetaData, seiSectionId, uOffset, uLength, 0);
}

int AL_StreamMetaData_AddSeiSection(AL_TStreamMetaData* pMetaData, bool isPrefix, uint32_t uOffset, uint32_t uLength)
{
  if(isPrefix)
    return AddPrefixSei(pMetaData, uOffset, uLength);

  return AL_StreamMetaData_AddSection(pMetaData, uOffset, uLength, 0);
}

uint32_t AL_StreamMetaData_GetUnusedStreamPart(AL_TStreamMetaData* pMetaData)
{
  AL_TStreamSection* pSections = pMetaData->pSections;
  uint16_t uNumSection = pMetaData->uNumSection;

  uint32_t maxOffset = 0;

  for(uint16_t i = 0; i < uNumSection; ++i)
  {
    uint32_t curOffset = pSections[i].uOffset + pSections[i].uLength;

    if(curOffset > maxOffset)
      maxOffset = curOffset;
  }

  return maxOffset;
}

