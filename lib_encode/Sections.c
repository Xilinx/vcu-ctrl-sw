/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include <stdio.h>

#include "lib_common/SeiInternal.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Utils.h"

#include "lib_bitstream/AVC_RbspEncod.h"
#include "lib_bitstream/HEVC_RbspEncod.h"

#include "lib_assert/al_assert.h"

static void AddSection(AL_TStreamMetaData* pMeta, uint32_t uOffset, uint32_t uLength, uint32_t uFlags)
{
  int iSection = AL_StreamMetaData_AddSection(pMeta, uOffset, uLength, uFlags);
  AL_Assert(iSection >= 0);
}

static int getBytesOffset(AL_TBitStreamLite* pStream)
{
  return AL_BitStreamLite_GetBitsCount(pStream) / 8;
}

static int writeNalInBuffer(IRbspWriter* writer, uint8_t* buffer, int bufSize, AL_TNalUnit* nal)
{
  AL_TBitStreamLite bitstream;
  AL_BitStreamLite_Init(&bitstream, buffer, bufSize);

  nal->Write(writer, &bitstream, nal->param, nal->layerId);

  if(bitstream.isOverflow)
    return -1;

  return AL_BitStreamLite_GetBitsCount(&bitstream);
}

static int WriteNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int bitstreamSize, AL_TNalUnit* nal, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  uint8_t tmpBuffer[AL_ENC_MAX_HEADER_SIZE];

  int sizeInBits = writeNalInBuffer(writer, tmpBuffer, bitstreamSize, nal);

  if(sizeInBits < 0)
    return -1;

  int start = getBytesOffset(bitstream);
  FlushNAL(writer, bitstream, nal->nut, &nal->header, tmpBuffer, sizeInBits, eStartCodeBytesAligned);
  int end = getBytesOffset(bitstream);

  if(bitstream->isOverflow)
    return -1;
  return end - start;
}

static void GenerateNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int bitstreamSize, AL_TNalUnit* nal, AL_TStreamMetaData* pMeta, uint32_t uFlags, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  int start = getBytesOffset(bitstream);
  int size = WriteNal(writer, bitstream, bitstreamSize, nal, eStartCodeBytesAligned);
  /* we should always be able to write the configuration nals as we reserved
   * enough space for them */
  AL_Assert(size >= 0);
  AddSection(pMeta, start, size, uFlags);
}

static void GenerateConfigNalUnits(IRbspWriter* writer, AL_TNalUnit* nals, uint32_t* nalsFlags, int nalsCount, AL_TBuffer* pStream, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  AL_TBitStreamLite bitstream;
  int bitstreamSize = AL_ENC_MAX_CONFIG_HEADER_SIZE;
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream), bitstreamSize);
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  for(int i = 0; i < nalsCount; i++)
    GenerateNal(writer, &bitstream, bitstreamSize, &nals[i], pMetaData, nalsFlags[i], eStartCodeBytesAligned);
}

static AL_TSeiPrefixAPSCtx createSeiPrefixAPSCtx(AL_TSps* sps, AL_THevcVps* vps)
{
  AL_TSeiPrefixAPSCtx ctx = { sps, vps };
  return ctx;
}

static AL_TSeiPrefixCtx createSeiPrefixCtx(const AL_TNalsData* pNalsData, AL_TEncPicStatus const* pPicStatus, uint32_t uFlags)
{
  AL_TSeiPrefixCtx ctx =
  {
    pNalsData->sps,
    pNalsData->seiData.initialCpbRemovalDelay,
    pNalsData->seiData.cpbRemovalDelay,
    uFlags,
    pPicStatus,
    pNalsData->seiData.pHDRSEIs,
  };
  return ctx;
}

static AL_TSeiPrefixUDUCtx createSeiPrefixUDUCtx(int8_t iNumSlices)
{
  AL_TSeiPrefixUDUCtx ctx;
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

static uint32_t generateSeiFlags(AL_TEncPicStatus const* pPicStatus, bool bForceSEIRecoveryPointOnIDR)
{
  uint32_t uFlags = AL_SEI_PT;

  if(pPicStatus->eType == AL_SLICE_I)
  {
    uFlags |= AL_SEI_BP;

    bool bShouldUseSEIRecoveryPoint = (!pPicStatus->bIsIDR || bForceSEIRecoveryPointOnIDR);

    if(bShouldUseSEIRecoveryPoint)
      uFlags |= AL_SEI_RP;
  }
  else if(pPicStatus->iRecoveryCnt)
    uFlags |= AL_SEI_RP;

  return uFlags;
}

static uint32_t generateHDRSeiFlags(bool bWriteSPS, bool bMustWriteDynHDR)
{
  uint32_t uFlags = 0;

  if(bWriteSPS)
    uFlags |= AL_SEI_MDCV | AL_SEI_CLL | AL_SEI_ATC;

  uFlags |= AL_SEI_ST2094_10;

  if(bWriteSPS || bMustWriteDynHDR)
    uFlags |= AL_SEI_ST2094_40;

  return uFlags;
}

void GenerateSections(IRbspWriter* writer, AL_TNuts nuts, AL_TNalsData const* pNalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayerID, int iNumSlices, bool bSubframeLatency, bool bForceSEIRecoveryPointOnIDR)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  if(pPicStatus->bIsFirstSlice)
  {
    AL_TNalUnit nals[9];
    uint32_t nalsFlags[9];
    int nalsCount = 0;

    if(pNalsData->bMustWriteAud)
    {
      nals[nalsCount] = AL_CreateAud(nuts.audNut, pPicStatus->eType, pPicStatus->uTempId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bRandomAccessPoint = pPicStatus->bIsIDR || pPicStatus->iRecoveryCnt;

    bool bWriteVPS = bRandomAccessPoint && (iLayerID == 0) && writer->WriteVPS;

    if(bWriteVPS)
    {
      nals[nalsCount] = AL_CreateVps(pNalsData->vps, pPicStatus->uTempId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bWriteSPS = bRandomAccessPoint;

    if(bWriteSPS)
    {
      nals[nalsCount] = AL_CreateSps(nuts.spsNut, pNalsData->sps, iLayerID, pPicStatus->uTempId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bWritePPS = bRandomAccessPoint || pNalsData->bMustWritePPS;

    if(bWritePPS)
    {
      nals[nalsCount] = AL_CreatePps(nuts.ppsNut, pNalsData->pps, iLayerID, pPicStatus->uTempId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    AL_TSeiPrefixAPSCtx seiPrefixAPSCtx;
    AL_TSeiPrefixCtx seiPrefixCtx;

    if(AL_HAS_SEI_PREFIX(pNalsData->seiFlags))
    {
      AL_Assert(pNalsData->seiFlags != AL_SEI_NONE);

      uint32_t uFlags = generateSeiFlags(pPicStatus, bForceSEIRecoveryPointOnIDR);
      uFlags |= generateHDRSeiFlags(bWriteSPS, pNalsData->bMustWriteDynHDR);
      uFlags &= pNalsData->seiFlags;

      bool bIsBufferingPeriodOrPictureTiming = ((uFlags & (AL_SEI_BP | AL_SEI_PT)) != 0);

      if(bIsBufferingPeriodOrPictureTiming && writer->WriteSEI_ActiveParameterSets)
      {
        seiPrefixAPSCtx = createSeiPrefixAPSCtx(pNalsData->sps, pNalsData->vps);
        nals[nalsCount] = AL_CreateSeiPrefixAPS(&seiPrefixAPSCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
        nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
      }

      if(uFlags)
      {
        seiPrefixCtx = createSeiPrefixCtx(pNalsData, pPicStatus, uFlags);
        nals[nalsCount] = AL_CreateSeiPrefix(&seiPrefixCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
        nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
      }
    }

    AL_TSeiPrefixUDUCtx seiPrefixUDUCtx;

    if(bSubframeLatency)
    {
      seiPrefixUDUCtx = createSeiPrefixUDUCtx(iNumSlices);
      nals[nalsCount] = AL_CreateSeiPrefixUDU(&seiPrefixUDUCtx, nuts.seiPrefixNut, pPicStatus->uTempId);
      nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
    }

    for(int i = 0; i < nalsCount; i++)
      nals[i].header = nuts.GetNalHeader(nals[i].nut, nals[i].nalRefIdc, nals[i].layerId, nals[i].tempId);

    GenerateConfigNalUnits(writer, nals, nalsFlags, nalsCount, pStream, pNalsData->eStartCodeBytesAligned);
  }

  AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(AL_Buffer_GetData(pStream) + pPicStatus->uStreamPartOffset);

  for(int iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
    AddSection(pMetaData, pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize, 0);

  int offset = getOffsetAfterLastSection(pMetaData);
  AL_TBitStreamLite bs;
  AL_BitStreamLite_Init(&bs, AL_Buffer_GetData(pStream), AL_Buffer_GetSize(pStream));
  AL_BitStreamLite_SkipBits(&bs, BytesToBits(offset));

  bool shouldWriteFiller = pNalsData->fillerCtrlMode && pPicStatus->iFiller;

  if(bSubframeLatency)
    shouldWriteFiller |= pPicStatus->bIsLastSlice;

  if(shouldWriteFiller)
  {
    bool bDontFill = (pNalsData->fillerCtrlMode == AL_FILLER_APP);

    int iBookmark = AL_BitStreamLite_GetBitsCount(&bs);
    AL_TNalHeader header = nuts.GetNalHeader(nuts.fdNut, 0, 0, pPicStatus->uTempId);
    WriteFillerData(writer, &bs, nuts.fdNut, &header, pPicStatus->iFiller, bDontFill, pNalsData->eStartCodeBytesAligned);
    int iWritten = (AL_BitStreamLite_GetBitsCount(&bs) - iBookmark) / 8;

    if(iWritten < pPicStatus->iFiller)
      Rtos_Log(AL_LOG_CRITICAL, "[WARNING] Filler data (%i) doesn't fit in the current buffer. Clip it to %i !\n", pPicStatus->iFiller, iWritten);

    AddSection(pMetaData, offset, iWritten, bDontFill ? AL_SECTION_APP_FILLER_FLAG : AL_SECTION_FILLER_FLAG);
  }

  if(pPicStatus->bIsLastSlice)
    AddSection(pMetaData, 0, 0, AL_SECTION_END_FRAME_FLAG);

  if(pPicStatus->bIsIDR)
    AddFlagsToAllSections(pMetaData, AL_SECTION_SYNC_FLAG);
}

static AL_TSeiExternalCtx createExternalSeiCtx(uint8_t* pPayload, int iPayloadType, int iPayloadSize)
{
  AL_TSeiExternalCtx ctx;
  ctx.pPayload = pPayload;
  ctx.iPayloadType = iPayloadType;
  ctx.iPayloadSize = iPayloadSize;
  return ctx;
}

#include "lib_common/Utils.h" // For Min

static int createExternalSei(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, uint32_t uOffset, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  (void)eCodec;

  AL_TSeiExternalCtx ctx = createExternalSeiCtx(pPayload, iPayloadType, iPayloadSize);
  AL_TNalUnit nal = AL_CreateExternalSei(&ctx, isPrefix ? nuts.seiPrefixNut : nuts.seiSuffixNut, iTempId);
  nal.header = nuts.GetNalHeader(nal.nut, nal.nalRefIdc, nal.layerId, nal.tempId);

  AL_TBitStreamLite bitstream;
  int bitstreamSize = Min(AL_ENC_MAX_SEI_SIZE, AL_Buffer_GetSize(pStream));
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream) + uOffset, bitstreamSize);

  IRbspWriter* pWriter = NULL;

  if(eCodec == AL_CODEC_AVC)
    pWriter = AL_GetAvcRbspWriter();

  if(eCodec == AL_CODEC_HEVC)
    pWriter = AL_GetHevcRbspWriter();

  return WriteNal(pWriter, &bitstream, bitstreamSize, &nal, eStartCodeBytesAligned);
}

uint32_t getUserSeiPrefixOffset(AL_TStreamMetaData* pStreamMeta)
{
  int iSEIPrefixSectionID = AL_StreamMetaData_GetLastSectionOfFlag(pStreamMeta, AL_SECTION_SEI_PREFIX_FLAG);

  if(iSEIPrefixSectionID == -1)
    return AL_ENC_MAX_HEADER_SIZE - AL_ENC_MAX_SEI_SIZE;

  AL_TStreamSection lastSeiPrefixSection = pStreamMeta->pSections[iSEIPrefixSectionID];
  return lastSeiPrefixSection.uOffset + lastSeiPrefixSection.uLength;
}

int AL_WriteSeiSection(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  AL_Assert(pMetaData);
  AL_Assert(iPayloadType >= 0);

  uint32_t uOffset = isPrefix ? getUserSeiPrefixOffset(pMetaData) : AL_StreamMetaData_GetUnusedStreamPart(pMetaData);

  int iTotalSize = createExternalSei(eCodec, nuts, pStream, uOffset, isPrefix, iPayloadType, pPayload, iPayloadSize, iTempId, eStartCodeBytesAligned);

  if(iTotalSize < 0)
    return -1;

  int sectionId = AL_StreamMetaData_AddSeiSection(pMetaData, isPrefix, uOffset, iTotalSize);

  if(sectionId < 0)
    return -1;

  return sectionId;
}

