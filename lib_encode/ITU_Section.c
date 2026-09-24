// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "ITU_Section.h"
#include "NalWriters.h"

#include "lib_bitstream/BitStreamLite.h"
#include "lib_common/SEI.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/StreamSection.h"

#include "lib_common/Arithmetics.h"
#include "lib_rtos/lib_rtos.h"

#include "lib_bitstream/AVC_RbspEncod.h"
#include "lib_encode/AVC_Sections.h"
#include "lib_bitstream/HEVC_RbspEncod.h"
#include "lib_encode/HEVC_Sections.h"

static int32_t getBytesOffset(AL_TBitStreamLite* pStream)
{
  return AL_BitStreamLite_GetBitsCount(pStream) / 8;
}

static int32_t writeNalInBuffer(IRbspWriter* writer, uint8_t* buffer, int32_t bufSize, AL_TNalUnit* nal)
{
  AL_TBitStreamLite bitstream;
  AL_BitStreamLite_Init(&bitstream, buffer, bufSize);

  nal->Write(writer, &bitstream, nal->param, nal->layerId);

  if(bitstream.isOverflow)
    return -1;

  return AL_BitStreamLite_GetBitsCount(&bitstream);
}

static int32_t WriteNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int32_t bitstreamSize, AL_TNalUnit* nal, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  uint8_t tmpBuffer[AL_ENC_MAX_HEADER_SIZE];
  uint8_t* pInitialStreamData = AL_BitStreamLite_GetCurData(bitstream);
  int32_t sizeInBits = writeNalInBuffer(writer, tmpBuffer, bitstreamSize, nal);

  if(sizeInBits < 0)
    return -1;

  int32_t start = getBytesOffset(bitstream);
  FlushNAL(writer, bitstream, nal->nut, &nal->header, tmpBuffer, sizeInBits, eStartCodeBytesAligned);
  int32_t end = getBytesOffset(bitstream);

  if(bitstream->isOverflow)
    return -1;

  int32_t size = end - start;
  Rtos_FlushCacheMemory(pInitialStreamData, size);

  return size;
}

static void GenerateNal(IRbspWriter* writer, AL_TBitStreamLite* bitstream, int32_t bitstreamSize, AL_TNalUnit* nal, AL_TStreamMetaData* pMeta, AL_ESectionFlags eFlags, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  int32_t start = getBytesOffset(bitstream);
  int32_t size = WriteNal(writer, bitstream, bitstreamSize, nal, eStartCodeBytesAligned);
  /* we should always be able to write the configuration nals as we reserved
   * enough space for them */
  Rtos_Assert(size >= 0);
  AL_StreamMetaData_AddSection(pMeta, start, size, eFlags);
}

static void GenerateConfigNalUnits(IRbspWriter* writer, AL_TNalUnit* nals, AL_ESectionFlags* nalsFlags, int32_t nalsCount, AL_TBuffer* pStream, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  AL_TBitStreamLite bitstream;
  int32_t bitstreamSize = AL_ENC_MAX_CONFIG_HEADER_SIZE;
  AL_BitStreamLite_Init(&bitstream, AL_Buffer_GetData(pStream), bitstreamSize);
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  for(int32_t i = 0; i < nalsCount; i++)
    GenerateNal(writer, &bitstream, bitstreamSize, &nals[i], pMetaData, nalsFlags[i], eStartCodeBytesAligned);
}

static AL_TSeiPrefixAPSCtx createSeiPrefixAPSCtx(AL_TSps* sps, AL_TVps* vps)
{
  AL_TSeiPrefixAPSCtx ctx = { sps, (AL_THevcVps*)vps };
  return ctx;
}

static AL_TSeiPrefixCtx createSeiPrefixCtx(const AL_TNalsData* pNalsData, AL_TEncPicStatus const* pPicStatus, AL_ESeiFlag eFlags)
{
  AL_TSeiPrefixCtx ctx =
  {
    pNalsData->sps,
    pNalsData->seiData.initialCpbRemovalDelay,
    pNalsData->seiData.cpbRemovalDelay,
    eFlags,
    pPicStatus,
    pNalsData->seiData.pHDRSEIs,
  };
  return ctx;
}

static AL_TAllegroNumSlicesSeiCtx createAllegroNumSlicesSeiCtx(int8_t iNumSlices)
{
  AL_TAllegroNumSlicesSeiCtx ctx;
  ctx.numSlices = iNumSlices;
  return ctx;
}

static int32_t getOffsetAfterLastSection(AL_TStreamMetaData* pMeta)
{
  if(pMeta->uNumSection == 0)
    return 0;

  AL_TStreamSection lastSection = pMeta->pSections[pMeta->uNumSection - 1];
  return lastSection.uOffset + lastSection.uLength;
}

static AL_ESeiFlag generateSeiFlags(AL_TEncPicStatus const* pPicStatus, bool bForceSEIRecoveryPointOnIDR)
{
  AL_ESeiFlag eFlags = AL_SEI_PT;

  if(pPicStatus->eType == AL_SLICE_I)
  {
    eFlags |= AL_SEI_BP;

    bool bShouldUseSEIRecoveryPoint = (!pPicStatus->bIsIDR || bForceSEIRecoveryPointOnIDR);

    if(bShouldUseSEIRecoveryPoint)
      eFlags |= AL_SEI_RP;
  }
  else if(pPicStatus->iRecoveryCnt != 0)
    eFlags |= AL_SEI_RP;

  return eFlags;
}

static AL_ESeiFlag generateHDRSeiFlags(bool bWriteSPS, bool bMustWriteDynHDR)
{
  AL_ESeiFlag eFlags = AL_SEI_NONE;

  if(bWriteSPS)
    eFlags |= AL_SEI_MDCV | AL_SEI_CLL | AL_SEI_ATC;

  eFlags |= AL_SEI_ST2094_10;

  if(bWriteSPS || bMustWriteDynHDR)
    eFlags |= AL_SEI_ST2094_40;

  return eFlags;
}

void GenerateSections(IRbspWriter* writer, AL_TNuts nuts, AL_TNalsData const* pNalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int32_t iLayerID, int32_t iNumSlices, bool bSubframeLatency, bool bForceSEIRecoveryPointOnIDR)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);

  if(pPicStatus->bIsFirstSlice)
  {
    AL_TNalUnit nals[11];
    AL_ESectionFlags nalsFlags[11];
    int32_t nalsCount = 0;

    if(pNalsData->bMustWriteAud)
    {
      nals[nalsCount] = AL_CreateAud(nuts.audNut, pNalsData->aud, pPicStatus->uTemporalId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bRandomAccessPoint = pPicStatus->bIsIDR || (pPicStatus->iRecoveryCnt != 0);

    bool bWriteVPS = bRandomAccessPoint && (iLayerID == 0) && writer->WriteVPS;

    if(bWriteVPS)
    {
      nals[nalsCount] = AL_CreateVps(nuts.vpsNut, pNalsData->vps, pPicStatus->uTemporalId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bWriteSPS = bRandomAccessPoint;

    if(bWriteSPS)
    {
      nals[nalsCount] = AL_CreateSps(nuts.spsNut, pNalsData->sps, iLayerID, pPicStatus->uTemporalId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    bool bWritePPS = bRandomAccessPoint || pNalsData->bMustWritePPS;

    if(bWritePPS)
    {
      nals[nalsCount] = AL_CreatePps(nuts.ppsNut, pNalsData->pps, iLayerID, pPicStatus->uTemporalId);
      nalsFlags[nalsCount++] = AL_SECTION_CONFIG_FLAG;
    }

    AL_TSeiPrefixAPSCtx seiPrefixAPSCtx;
    AL_TSeiPrefixCtx seiPrefixCtx;

    if(AL_HAS_SEI_PREFIX(pNalsData->seiFlags))
    {
      Rtos_Assert(pNalsData->seiFlags != AL_SEI_NONE);

      AL_ESeiFlag eFlags = generateSeiFlags(pPicStatus, bForceSEIRecoveryPointOnIDR);
      eFlags |= generateHDRSeiFlags(bWriteSPS, pNalsData->bMustWriteDynHDR);
      eFlags &= pNalsData->seiFlags;

      bool bIsBufferingPeriodOrPictureTiming = ((eFlags & (AL_SEI_BP | AL_SEI_PT)) != 0);

      if(bIsBufferingPeriodOrPictureTiming && writer->WriteSEI_ActiveParameterSets)
      {
        seiPrefixAPSCtx = createSeiPrefixAPSCtx(pNalsData->sps, pNalsData->vps);
        nals[nalsCount] = AL_CreateSeiPrefixAPS(&seiPrefixAPSCtx, nuts.seiPrefixNut, iLayerID, pPicStatus->uTemporalId);
        nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
      }

      if(eFlags != AL_SEI_NONE)
      {
        seiPrefixCtx = createSeiPrefixCtx(pNalsData, pPicStatus, eFlags);
        nals[nalsCount] = AL_CreateSeiPrefix(&seiPrefixCtx, nuts.seiPrefixNut, iLayerID, pPicStatus->uTemporalId);
        nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
      }
    }

    AL_TAllegroNumSlicesSeiCtx tAllegroNumSlicesSeiCtx;

    if(bSubframeLatency)
    {
      tAllegroNumSlicesSeiCtx = createAllegroNumSlicesSeiCtx(iNumSlices);
      nals[nalsCount] = AL_CreateAllegroNumSlicesSei(&tAllegroNumSlicesSeiCtx, nuts.seiPrefixNut, iLayerID, pPicStatus->uTemporalId);
      nalsFlags[nalsCount++] = AL_SECTION_SEI_PREFIX_FLAG;
    }

    for(int32_t i = 0; i < nalsCount; i++)
      nals[i].header = nuts.GetNalHeader(nals[i].nut, nals[i].nalRefIdc, nals[i].layerId, nals[i].tempId);

    GenerateConfigNalUnits(writer, nals, nalsFlags, nalsCount, pStream, pNalsData->eStartCodeBytesAligned);
  }

  AL_TStreamPart* pStreamParts = (AL_TStreamPart*)(AL_Buffer_GetData(pStream) + pPicStatus->uStreamPartOffset);

  for(int32_t iPart = 0; iPart < pPicStatus->iNumParts; ++iPart)
  {
    AL_ESectionFlags eSectionFlags = AL_SECTION_NO_FLAG;
    AL_StreamMetaData_AddSection(pMetaData, pStreamParts[iPart].uOffset, pStreamParts[iPart].uSize, eSectionFlags);
  }

  int32_t offset = getOffsetAfterLastSection(pMetaData);
  AL_TBitStreamLite bs;
  AL_BitStreamLite_Init(&bs, AL_Buffer_GetData(pStream), AL_Buffer_GetSize(pStream));
  AL_BitStreamLite_SkipBits(&bs, BytesToBits(offset));

  bool shouldWriteFiller = pNalsData->fillerCtrlMode && pPicStatus->iFiller;

  if(bSubframeLatency)
    shouldWriteFiller |= pPicStatus->bIsLastSlice;

  if(shouldWriteFiller)
  {
    AL_TNalHeader header = nuts.GetNalHeader(nuts.fdNut, 0, 0, pPicStatus->uTemporalId);
    int32_t const iMaxStartCodeSize = (pNalsData->eStartCodeBytesAligned == AL_START_CODE_4_BYTES) ? 4 : 3;
    int32_t const iFDAtLeastOneWord = 1; /* 0xFF */
    int32_t const iFDLastWord = 1; /* 0x80 */
    AL_64U const iMinSpaceRequired = header.size + iMaxStartCodeSize + iFDAtLeastOneWord + iFDLastWord;
    bool bNotEnoughSpaceLeft = (BitsToBytes(bs.iMaxBits - AL_BitStreamLite_GetBitsCount(&bs))) < iMinSpaceRequired;

    if(bNotEnoughSpaceLeft)
      Rtos_Log(AL_LOG_CRITICAL, "[WARNING] Filler data (%i) doesn't fit in the current buffer. Don't add them", pPicStatus->iFiller);
    else
    {
      bool bDontFill = (pNalsData->fillerCtrlMode == AL_FILLER_APP);
      int32_t iBookmark = AL_BitStreamLite_GetBitsCount(&bs);
      /* iFiller should be at least iMinSpaceRequired */
      int32_t iFiller = Clip3(pPicStatus->iFiller, iMinSpaceRequired, pPicStatus->iFiller);
      WriteFillerData(writer, &bs, nuts.fdNut, &header, iFiller, bDontFill, pNalsData->eStartCodeBytesAligned);
      int32_t iWritten = BitsToBytes(AL_BitStreamLite_GetBitsCount(&bs) - iBookmark);

      if(iWritten < pPicStatus->iFiller)
        Rtos_Log(AL_LOG_CRITICAL, "[WARNING] Filler data (%i) doesn't fit in the current buffer. Clip it to %i !\n", pPicStatus->iFiller, iWritten);

      AL_StreamMetaData_AddSection(pMetaData, offset, iWritten, bDontFill ? AL_SECTION_APP_FILLER_FLAG : AL_SECTION_FILLER_FLAG);
    }
  }

  if(pPicStatus->bIsLastSlice)
    AL_StreamMetaData_AddSection(pMetaData, 0, 0, AL_SECTION_END_FRAME_FLAG);

  if(pPicStatus->bIsIDR)
    AddFlagsToAllSections(pMetaData, AL_SECTION_SYNC_FLAG);
}

static AL_TSeiExternalCtx createExternalSeiCtx(uint8_t* pPayload, int32_t iPayloadType, int32_t iPayloadSize)
{
  AL_TSeiExternalCtx ctx;
  ctx.pPayload = pPayload;
  ctx.iPayloadType = iPayloadType;
  ctx.iPayloadSize = iPayloadSize;
  return ctx;
}

static int32_t createExternalSei(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, uint32_t uOffset, bool isPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, int32_t iLayerId, int32_t iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  (void)eCodec;

  AL_TSeiExternalCtx ctx = createExternalSeiCtx(pPayload, iPayloadType, iPayloadSize);
  AL_TNalUnit nal = AL_CreateExternalSei(&ctx, isPrefix ? nuts.seiPrefixNut : nuts.seiSuffixNut, iLayerId, iTempId);
  nal.header = nuts.GetNalHeader(nal.nut, nal.nalRefIdc, nal.layerId, nal.tempId);

  AL_TBitStreamLite bitstream;
  int32_t bitstreamSize = Min(AL_ENC_MAX_SEI_SIZE, AL_Buffer_GetSize(pStream));
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
  int32_t iSEIPrefixSectionID = AL_StreamMetaData_GetLastSectionOfFlag(pStreamMeta, AL_SECTION_SEI_PREFIX_FLAG);

  if(iSEIPrefixSectionID == -1)
    return AL_ENC_MAX_HEADER_SIZE - AL_ENC_MAX_SEI_SIZE;

  AL_TStreamSection lastSeiPrefixSection = pStreamMeta->pSections[iSEIPrefixSectionID];
  return lastSeiPrefixSection.uOffset + lastSeiPrefixSection.uLength;
}

int32_t AL_WriteSeiSection(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, bool isPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, int32_t iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  Rtos_Assert(pMetaData);
  Rtos_Assert(iPayloadType >= 0);

  uint32_t uOffset = isPrefix ? getUserSeiPrefixOffset(pMetaData) : AL_StreamMetaData_GetUnusedStreamPart(pMetaData);

  int32_t iTotalSize = createExternalSei(eCodec, nuts, pStream, uOffset, isPrefix, iPayloadType, pPayload, iPayloadSize, 0, iTempId, eStartCodeBytesAligned);

  if(iTotalSize < 0)
    return -1;

  int32_t sectionId = AL_StreamMetaData_AddSeiSection(pMetaData, isPrefix, uOffset, iTotalSize);

  if(sectionId < 0)
    return -1;

  return sectionId;
}

bool AL_CreateNuts(AL_TNuts* nuts, AL_EProfile eProfile)
{
  (void)nuts, (void)eProfile;

  if(AL_IS_AVC(eProfile))
  {
    *nuts = CreateAvcNuts();
    return true;
  }

  if(AL_IS_HEVC(eProfile))
  {
    *nuts = CreateHevcNuts();
    return true;
  }
  return false;
}
