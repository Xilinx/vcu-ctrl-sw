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

#include "lib_common/BufferStreamMeta.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_assert/al_assert.h"

static bool StreamMeta_Destroy(AL_TMetaData* pMeta)
{
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)pMeta;
  Rtos_Free(pStreamMeta->pSections);
  Rtos_Free(pMeta);
  return true;
}

AL_TStreamMetaData* AL_StreamMetaData_Clone(AL_TStreamMetaData* pMeta)
{
  AL_TStreamMetaData* pNewMeta = AL_StreamMetaData_Create(pMeta->uMaxNumSection);
  pNewMeta->uTemporalID = pMeta->uTemporalID;
  pNewMeta->uMaxNumSection = pMeta->uMaxNumSection;
  pNewMeta->uNumSection = pMeta->uNumSection;

  for(uint16_t i = 0; i < pMeta->uNumSection; ++i)
    pNewMeta->pSections[i] = pMeta->pSections[i];

  return pNewMeta;
}

AL_TMetaData* StreamMeta_Clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_StreamMetaData_Clone((AL_TStreamMetaData*)pMeta);
}

AL_TStreamMetaData* AL_StreamMetaData_Create(uint16_t uMaxNumSection)
{
  if(uMaxNumSection == 0)
    return NULL;

  AL_TStreamMetaData* pMeta = (AL_TStreamMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->tMeta.eType = AL_META_TYPE_STREAM;
  pMeta->tMeta.MetaDestroy = StreamMeta_Destroy;
  pMeta->tMeta.MetaClone = StreamMeta_Clone;

  pMeta->uTemporalID = 0;
  pMeta->uMaxNumSection = uMaxNumSection;
  pMeta->uNumSection = 0;

  pMeta->pSections = (AL_TStreamSection*)Rtos_Malloc(sizeof(AL_TStreamSection) * uMaxNumSection);

  if(!pMeta->pSections)
    goto fail_alloc_section;

  return pMeta;

  fail_alloc_section:
  Rtos_Free(pMeta);
  return NULL;
}

static void SetSection(AL_TStreamSection* pSections, uint16_t uSectionID, uint32_t uOffset, uint32_t uLength, AL_ESectionFlags eFlags)
{
  pSections[uSectionID].uOffset = uOffset;
  pSections[uSectionID].uLength = uLength;
  pSections[uSectionID].eFlags = eFlags;
}

int AL_StreamMetaData_AddSection(AL_TStreamMetaData* pMetaData, uint32_t uOffset, uint32_t uLength, AL_ESectionFlags eFlags)
{
  if(!pMetaData)
    return -1;

  uint16_t uSectionID = pMetaData->uNumSection;

  if(uSectionID >= pMetaData->uMaxNumSection)
    return -1;

  SetSection(pMetaData->pSections, uSectionID, uOffset, uLength, eFlags);
  ++pMetaData->uNumSection;

  return uSectionID;
}

/****************************************************************************/
void AL_StreamMetaData_ChangeSection(AL_TStreamMetaData* pMetaData, uint16_t uSectionID, uint32_t uOffset, uint32_t uLength)
{
  AL_Assert(pMetaData && uSectionID < pMetaData->uNumSection);

  pMetaData->pSections[uSectionID].uOffset = uOffset;
  pMetaData->pSections[uSectionID].uLength = uLength;
}

/****************************************************************************/
void AL_StreamMetaData_SetSectionFlags(AL_TStreamMetaData* pMetaData, uint16_t uSectionID, AL_ESectionFlags eFlags)
{
  AL_Assert(pMetaData && uSectionID < pMetaData->uNumSection);
  pMetaData->pSections[uSectionID].eFlags = eFlags;
}

/****************************************************************************/
void AL_StreamMetaData_ClearAllSections(AL_TStreamMetaData* pMetaData)
{
  AL_Assert(pMetaData);
  pMetaData->uNumSection = 0;
}

/****************************************************************************/
int AL_StreamMetaData_GetLastSectionOfFlag(AL_TStreamMetaData* pMetaData, uint32_t flag)
{
  AL_Assert(pMetaData);
  AL_TStreamSection* pSections = pMetaData->pSections;
  int flagSectionId = pMetaData->uNumSection - 1;

  while(flagSectionId >= 0)
  {
    if(pSections[flagSectionId].eFlags & flag)
      break;
    --flagSectionId;
  }

  /* flagSectionId == -1 if we didn't find any flag sections */

  return flagSectionId;
}

static int InsertSectionAtId(AL_TStreamMetaData* pMetaData, uint16_t uTargetID, uint32_t uOffset, uint32_t uLength, AL_ESectionFlags eFlags)
{
  if(!pMetaData)
    return -1;

  uint16_t uNumSection = pMetaData->uNumSection;
  AL_TStreamSection* pSections = pMetaData->pSections;

  if(uNumSection >= pMetaData->uMaxNumSection)
    return -1;

  for(int i = uNumSection - 1; i >= (int)uTargetID; --i)
  {
    AL_TStreamSection* cur = &pSections[i];
    SetSection(pSections, i + 1, cur->uOffset, cur->uLength, cur->eFlags);
  }

  SetSection(pSections, uTargetID, uOffset, uLength, eFlags);
  ++pMetaData->uNumSection;

  return uTargetID;
}

static int AddPrefixSei(AL_TStreamMetaData* pMetaData, uint32_t uOffset, uint32_t uLength)
{
  // the prefix sei needs to be inserted after a config section if it exists
  int iLastSEIPrefixSectionID = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_SEI_PREFIX_FLAG);

  if(iLastSEIPrefixSectionID == -1)
    iLastSEIPrefixSectionID = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_CONFIG_FLAG);

  int iSEIPrefixSectionID = iLastSEIPrefixSectionID + 1;

  return InsertSectionAtId(pMetaData, iSEIPrefixSectionID, uOffset, uLength, AL_SECTION_SEI_PREFIX_FLAG);
}

int AL_StreamMetaData_AddSeiSection(AL_TStreamMetaData* pMetaData, bool isPrefix, uint32_t uOffset, uint32_t uLength)
{
  if(isPrefix)
    return AddPrefixSei(pMetaData, uOffset, uLength);

  return AL_StreamMetaData_AddSection(pMetaData, uOffset, uLength, AL_SECTION_NO_FLAG);
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

