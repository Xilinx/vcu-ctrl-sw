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

#include "Sections.h"
#include "NalWriters.h"

#include <assert.h>

static int getBytesOffset(AL_TBitStreamLite* pStream)
{
  return AL_BitStreamLite_GetBitsCount(pStream) / 8;
}

static int writeNalInBuffer(IRbspWriter* writer, uint8_t* buffer, AL_NalUnit* nal)
{
  AL_TBitStreamLite bitstream;
  AL_BitStreamLite_Init(&bitstream, buffer);

  nal->Write(writer, &bitstream, nal->param);

  return AL_BitStreamLite_GetBitsCount(&bitstream);
}

static void GenerateNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, AL_NalUnit* nal, AL_TStreamMetaData* pMeta, uint32_t uFlags)
{
  uint8_t tmpBuffer[ENC_MAX_HEADER_SIZE];

  int sizeInBits = writeNalInBuffer(writer, tmpBuffer, nal);

  int start = getBytesOffset(bitstream);
  FlushNAL(bitstream, nal->nut, nal->header, tmpBuffer, sizeInBits);
  int end = getBytesOffset(bitstream);

  AL_StreamMetaData_AddSection(pMeta, start, end - start, uFlags);
}

static void GenerateNalUnits(IRbspWriter* writer, AL_NalUnit* nals, int nalsCount, AL_TBuffer* pStream, uint32_t uFlags)
{
  AL_TBitStreamLite bitstream;
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream));
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  for(int i = 0; i < nalsCount; i++)
    GenerateNal(writer, &bitstream, &nals[i], pMetaData, uFlags);
}

static SeiCtx createSeiCtx(AL_TSps* sps, AL_TPps* pps, AL_THevcVps* vps, int initialCpbRemovalDelay, int cpbRemovalDelay, AL_TEncPicStatus const* pPicStatus)
{
  uint32_t uFlags = SEI_PT;

  if(pPicStatus->eType == SLICE_I)
  {
    uFlags |= SEI_BP;

    if(!pPicStatus->bIsIDR)
      uFlags |= SEI_RP;
  }
  SeiCtx ctx = { sps, pps, vps, initialCpbRemovalDelay, cpbRemovalDelay, uFlags, pPicStatus };
  return ctx;
}

void GenerateSections(IRbspWriter* writer, Nuts nuts, const NalsData* nalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  uint32_t flags = SECTION_COMPLETE_FLAG;

  if(pPicStatus->bIsIDR)
    flags |= SECTION_SYNC_FLAG;

  if(pPicStatus->bIsFirstSlice)
  {
    AL_NalUnit nals[5];
    int nalsCount = 0;

    if(nalsData->shouldWriteAud)
      nals[nalsCount++] = AL_CreateAud(nuts.audNut, pPicStatus->eType);

    if(pPicStatus->bIsIDR)
    {
      if(writer->WriteVPS)
        nals[nalsCount++] = AL_CreateVps(nalsData->vps);

      nals[nalsCount++] = AL_CreateSps(nuts.spsNut, nalsData->sps);
    }

    if(pPicStatus->eType == SLICE_I)
      nals[nalsCount++] = AL_CreatePps(nuts.ppsNut, nalsData->pps);

    SeiCtx ctx;

    if(nalsData->seiData)
    {
      ctx = createSeiCtx(nalsData->sps, nalsData->pps, nalsData->vps, nalsData->seiData->initialCpbRemovalDelay, nalsData->seiData->cpbRemovalDelay, pPicStatus);
      nals[nalsCount++] = AL_CreateSei(&ctx, nuts.seiNut);
    }

    for(int i = 0; i < nalsCount; i++)
      nals[i].header = nuts.GetNalHeader(nals[i].nut, nals[i].idc);

    GenerateNalUnits(writer, nals, nalsCount, pStream, flags | SECTION_CONFIG_FLAG);
  }

  AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(AL_Buffer_GetData(pStream) + pPicStatus->uStreamPartOffset);

  for(int iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
    AL_StreamMetaData_AddSection(pMetaData, pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize, flags);

  if(nalsData->shouldWriteFillerData && pPicStatus->iFiller)
  {
    AL_TStreamPart lastPart = pStreamParts[pPicStatus->iNumParts - 1];
    uint32_t uOffset = lastPart.uOffset + lastPart.uSize;
    AL_TBitStreamLite bs;
    AL_BitStreamLite_Init(&bs, AL_Buffer_GetData(pStream) + uOffset);
    WriteFillerData(&bs, nuts.fdNut, nuts.GetNalHeader(nuts.fdNut, 0), pPicStatus->iFiller);
    AL_StreamMetaData_AddSection(pMetaData, uOffset, pPicStatus->iFiller, flags);
  }

  if(pPicStatus->bIsLastSlice)
    AL_StreamMetaData_AddSection(pMetaData, 0, 0, flags | SECTION_END_FRAME_FLAG);
}

