/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include "Sections.h"
#include "NalWriters.h"

#include "lib_common/SEI.h"
#include "lib_common/StreamBuffer.h"

#include <assert.h>
#include <stdio.h>

static void AddSection(AL_TStreamMetaData* pMeta, uint32_t uOffset, uint32_t uLength, uint32_t uFlags)
{
  int iSection = AL_StreamMetaData_AddSection(pMeta, uOffset, uLength, uFlags);
  assert(iSection >= 0);
}

static int getBytesOffset(AL_TBitStreamLite* pStream)
{
  return AL_BitStreamLite_GetBitsCount(pStream) / 8;
}

static int writeNalInBuffer(IRbspWriter* writer, uint8_t* buffer, int bufSize, AL_NalUnit* nal)
{
  AL_TBitStreamLite bitstream;
  AL_BitStreamLite_Init(&bitstream, buffer, bufSize);

  nal->Write(writer, &bitstream, nal->param, nal->iLayerId);

  if(bitstream.isOverflow)
    return -1;

  return AL_BitStreamLite_GetBitsCount(&bitstream);
}

static int WriteNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int bitstreamSize, AL_NalUnit* nal)
{
  uint8_t tmpBuffer[ENC_MAX_HEADER_SIZE];

  int sizeInBits = writeNalInBuffer(writer, tmpBuffer, bitstreamSize, nal);

  if(sizeInBits < 0)
    return -1;

  int start = getBytesOffset(bitstream);
  FlushNAL(bitstream, nal->nut, nal->header, tmpBuffer, sizeInBits);
  int end = getBytesOffset(bitstream);

  if(bitstream->isOverflow)
    return -1;
  return end - start;
}

static void GenerateNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int bitstreamSize, AL_NalUnit* nal, AL_TStreamMetaData* pMeta, uint32_t uFlags)
{
  int start = getBytesOffset(bitstream);
  int size = WriteNal(writer, bitstream, bitstreamSize, nal);
  /* we should always be able to write the configuration nals as we reserved
   * enough space for them */
  assert(size >= 0);
  AddSection(pMeta, start, size, uFlags);
}

static void GenerateConfigNalUnits(IRbspWriter* writer, AL_NalUnit* nals, int nalsCount, AL_TBuffer* pStream)
{
  AL_TBitStreamLite bitstream;
  int bitstreamSize = ENC_MAX_CONFIG_HEADER_SIZE;
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream), bitstreamSize);
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  for(int i = 0; i < nalsCount; i++)
    GenerateNal(writer, &bitstream, bitstreamSize, &nals[i], pMetaData, SECTION_CONFIG_FLAG);
}

static SeiPrefixAPSCtx createSeiPrefixAPSCtx(AL_TSps* sps, AL_THevcVps* vps)
{
  SeiPrefixAPSCtx ctx = { sps, vps };
  return ctx;
}

static SeiPrefixCtx createSeiPrefixCtx(AL_TSps* sps, int initialCpbRemovalDelay, int cpbRemovalDelay, AL_TEncPicStatus const* pPicStatus, uint32_t uFlags)
{
  SeiPrefixCtx ctx = { sps, initialCpbRemovalDelay, cpbRemovalDelay, uFlags, pPicStatus };
  return ctx;
}

static SeiPrefixUDUCtx createSeiPrefixUDUCtx(int8_t iNumSlices)
{
  SeiPrefixUDUCtx ctx;
  int iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);
  Rtos_Memcpy(&ctx.uuid, SEI_PREFIX_USER_DATA_UNREGISTERED_UUID, iSize);
  Rtos_Memcpy(&ctx.numSlices, &iNumSlices, sizeof(iNumSlices));
  return ctx;
}


static int getOffsetAfterLastSection(AL_TStreamMetaData* pMeta)
{
  if(pMeta->uNumSection == 0)
    return 0;

  AL_TStreamSection lastSection = pMeta->pSections[pMeta->uNumSection - 1];
  return lastSection.uOffset + lastSection.uLength;
}

static uint32_t generateSeiFlags(AL_TEncPicStatus const* pPicStatus)
{
  uint32_t uFlags = SEI_PT;

  if(pPicStatus->eType == SLICE_I)
  {
    uFlags |= SEI_BP;

    if(!pPicStatus->bIsIDR)
      uFlags |= SEI_RP;
  }
  else if(pPicStatus->iRecoveryCnt)
    uFlags |= SEI_RP;

  return uFlags;
}

void GenerateSections(IRbspWriter* writer, Nuts nuts, const NalsData* nalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayersCount, int iNumSlices)
{
  (void)iNumSlices;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  if(pPicStatus->bIsFirstSlice)
  {
    AL_NalUnit nals[9];
    int nalsCount = 0;

    if(nalsData->shouldWriteAud)
      nals[nalsCount++] = AL_CreateAud(nuts.audNut, pPicStatus->eType, pPicStatus->uTempId);

    if(pPicStatus->bIsIDR || pPicStatus->iRecoveryCnt)
    {
      if(writer->WriteVPS)
        nals[nalsCount++] = AL_CreateVps(nalsData->vps, pPicStatus->uTempId);

      for(int i = 0; i < iLayersCount; i++)
        nals[nalsCount++] = AL_CreateSps(nuts.spsNut, nalsData->sps[i], i, pPicStatus->uTempId);
    }

    if(pPicStatus->eType == SLICE_I || pPicStatus->iRecoveryCnt)
    {
      for(int i = 0; i < iLayersCount; i++)
        nals[nalsCount++] = AL_CreatePps(nuts.ppsNut, nalsData->pps[i], i, pPicStatus->uTempId);
    }

    SeiPrefixAPSCtx seiPrefixAPSCtx;
    SeiPrefixCtx seiPrefixCtx;
    SeiPrefixUDUCtx seiPrefixUDUCtx;

    if(isPrefix(nalsData->seiFlags))
    {
      assert(nalsData != NULL);
      assert(nalsData->seiFlags != SEI_NONE);

      uint32_t const uFlags = generateSeiFlags(pPicStatus) & nalsData->seiFlags;

      if(uFlags & (SEI_BP | SEI_PT) && writer->WriteSEI_ActiveParameterSets)
      {
        seiPrefixAPSCtx = createSeiPrefixAPSCtx(nalsData->sps[0], nalsData->vps);
        nals[nalsCount++] = AL_CreateSeiPrefixAPS(&seiPrefixAPSCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
      }

      if(uFlags)
      {
        seiPrefixCtx = createSeiPrefixCtx(nalsData->sps[0], nalsData->seiData->initialCpbRemovalDelay, nalsData->seiData->cpbRemovalDelay, pPicStatus, uFlags);
        nals[nalsCount++] = AL_CreateSeiPrefix(&seiPrefixCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
      }


      if(nalsData->seiFlags & SEI_UDU)
      {
        seiPrefixUDUCtx = createSeiPrefixUDUCtx(iNumSlices);
        nals[nalsCount++] = AL_CreateSeiPrefixUDU(&seiPrefixUDUCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
      }
    }

    for(int i = 0; i < nalsCount; i++)
      nals[i].header = nuts.GetNalHeader(nals[i].nut, nals[i].idc, nals[i].tempId);

    GenerateConfigNalUnits(writer, nals, nalsCount, pStream);
  }

  AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(AL_Buffer_GetData(pStream) + pPicStatus->uStreamPartOffset);

  for(int iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
    AddSection(pMetaData, pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize, 0);

  int offset = getOffsetAfterLastSection(pMetaData);
  AL_TBitStreamLite bs;
  AL_BitStreamLite_Init(&bs, AL_Buffer_GetData(pStream), pStream->zSize);
  AL_BitStreamLite_SkipBits(&bs, offset * 8);

  bool shouldWriteFiller = nalsData->shouldWriteFillerData && pPicStatus->iFiller;

  if(nalsData->seiFlags & SEI_UDU)
    shouldWriteFiller |= pPicStatus->bIsLastSlice;

  if(shouldWriteFiller)
  {
    int iBookmark = AL_BitStreamLite_GetBitsCount(&bs);
    WriteFillerData(&bs, nuts.fdNut, nuts.GetNalHeader(nuts.fdNut, 0, pPicStatus->uTempId), pPicStatus->iFiller);
    int iWritten = (AL_BitStreamLite_GetBitsCount(&bs) - iBookmark) / 8;

    if(iWritten < pPicStatus->iFiller)
      printf("[WARNING] Filler data (%i) doesn't fit in the current buffer. Clip it to %i !\n", pPicStatus->iFiller, iWritten);
    AddSection(pMetaData, offset, iWritten, 0);
  }

  if(pPicStatus->bIsLastSlice)
    AddSection(pMetaData, 0, 0, SECTION_END_FRAME_FLAG);

  if(pPicStatus->bIsIDR)
    AddFlagsToAllSections(pMetaData, SECTION_SYNC_FLAG);
}

static SeiExternalCtx createExternalSeiCtx(uint8_t* pPayload, int iPayloadType, int iPayloadSize)
{
  SeiExternalCtx ctx;
  ctx.pPayload = pPayload;
  ctx.iPayloadType = iPayloadType;
  ctx.iPayloadSize = iPayloadSize;
  return ctx;
}

#include "lib_common/Utils.h" // For Min

static int createExternalSei(Nuts nuts, AL_TBuffer* pStream, uint32_t uOffset, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId)
{
  SeiExternalCtx ctx = createExternalSeiCtx(pPayload, iPayloadType, iPayloadSize);
  AL_NalUnit nal = AL_CreateExternalSei(&ctx, isPrefix ? nuts.seiPrefixNut : nuts.seiSuffixNut, iTempId);
  nal.header = nuts.GetNalHeader(nal.nut, nal.idc, nal.tempId);

  AL_TBitStreamLite bitstream;
  int bitstreamSize = Min(ENC_MAX_SEI_SIZE, pStream->zSize);
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream) + uOffset, bitstreamSize);
  return WriteNal(NULL, &bitstream, bitstreamSize, &nal);
}

uint32_t getUserSeiPrefixOffset(AL_TStreamMetaData* pStreamMeta)
{
  int iSEIPrefixSectionID = AL_StreamMetaData_GetLastSectionOfFlag(pStreamMeta, SECTION_SEI_PREFIX_FLAG);

  if(iSEIPrefixSectionID == -1)
    return ENC_MAX_HEADER_SIZE - ENC_MAX_SEI_SIZE;

  AL_TStreamSection lastSeiPrefixSection = pStreamMeta->pSections[iSEIPrefixSectionID];
  return lastSeiPrefixSection.uOffset + lastSeiPrefixSection.uLength;
}

int AL_WriteSeiSection(Nuts nuts, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  assert(pMetaData);
  assert(iPayloadType >= 0);

  uint32_t uOffset = isPrefix ? getUserSeiPrefixOffset(pMetaData) : AL_StreamMetaData_GetUnusedStreamPart(pMetaData);

  int iTotalSize = createExternalSei(nuts, pStream, uOffset, isPrefix, iPayloadType, pPayload, iPayloadSize, iTempId);

  if(iTotalSize < 0)
    return -1;

  int sectionId = AL_StreamMetaData_AddSeiSection(pMetaData, isPrefix, uOffset, iTotalSize);

  if(sectionId < 0)
    return -1;

  return sectionId;
}

