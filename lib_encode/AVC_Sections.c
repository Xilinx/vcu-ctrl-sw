// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "AVC_Sections.h"

#include "lib_bitstream/AVC_RbspEncod.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Nuts.h"
#include "lib_common/Arithmetics.h"

AL_TNalHeader GetNalHeaderAvc(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTemporalId)
{
  (void)uTemporalId, (void)uLayerId;
  AL_TNalHeader nh;
  nh.size = 1;
  nh.bytes[0] = ((uNalRefIdc & 0x03) << 5) | (uNUT & 0x1F);
  return nh;
}

AL_TNuts CreateAvcNuts(void)
{
  AL_TNuts nuts =
  {
    &GetNalHeaderAvc,
    AL_AVC_NUT_SPS,
    AL_AVC_NUT_PPS,
    0, /* VPS does not exist in AVC */
    AL_AVC_NUT_AUD,
    AL_AVC_NUT_FD,
    AL_AVC_NUT_PREFIX_SEI,
    /* sei suffix do not really exist in AVC. use a prefix nut */
    AL_AVC_NUT_PREFIX_SEI,
    0,
    0,
  };
  return nuts;
}

static int32_t getSectionSize(AL_TStreamMetaData* pMetaData, AL_ESectionFlags eFlags)
{
  int32_t iSize = 0;

  for(int32_t iSection = 0; iSection < pMetaData->uNumSection; iSection++)
  {
    AL_TStreamSection* pSection = &pMetaData->pSections[iSection];

    if(pSection->eFlags & eFlags)
      iSize += pSection->uLength;
  }

  return iSize;
}

static void padConfig(AL_TBuffer* pStream)
{
  int32_t const iChunk = 512;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int32_t iConfigSize = getSectionSize(pMetaData, AL_SECTION_CONFIG_FLAG);

  if(iConfigSize >= iChunk)
    return;

  int32_t iLastConfigSection = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_CONFIG_FLAG);

  if(iLastConfigSection < 0)
    return;

  AL_TStreamSection* pLastConfigSection = &pMetaData->pSections[iLastConfigSection];
  int32_t iPaddingSize = iChunk - iConfigSize;
  Rtos_Memset(AL_Buffer_GetData(pStream) + pLastConfigSection->uOffset + pLastConfigSection->uLength, 0x00, iPaddingSize);
  pLastConfigSection->uLength += iPaddingSize;
}

static void padSeiPrefix(AL_TBuffer* pStream, AL_TEncChanParam const* pChannel)
{
  int32_t const iChunk = 512;
  int32_t const iSeiMandatorySize = (pChannel->uEncHeight <= 720) ? iChunk * 10 : iChunk * 18;
  Rtos_Assert(iSeiMandatorySize <= AL_ENC_MAX_SEI_SIZE);

  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int32_t iSeiSize = getSectionSize(pMetaData, AL_SECTION_SEI_PREFIX_FLAG);

  if(iSeiSize >= iSeiMandatorySize)
    return;

  int32_t iPaddingSize = iSeiMandatorySize - iSeiSize;

  int32_t iLastSeiSection = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_SEI_PREFIX_FLAG);

  if(iLastSeiSection < 0)
  {
    int32_t iOffset = AL_ENC_MAX_CONFIG_HEADER_SIZE + 1;
    Rtos_Memset(AL_Buffer_GetData(pStream) + iOffset, 0x00, iPaddingSize);
    AL_StreamMetaData_AddSeiSection(pMetaData, true, iOffset, iPaddingSize);
    return;
  }

  AL_TStreamSection* pLastSeiSection = &pMetaData->pSections[iLastSeiSection];
  Rtos_Memset(AL_Buffer_GetData(pStream) + pLastSeiSection->uOffset + pLastSeiSection->uLength, 0x00, iPaddingSize);
  pLastSeiSection->uLength += iPaddingSize;
}

static void padCodedSliceData(int32_t iCodedSliceSize, AL_TBuffer* pStream, AL_TEncChanParam const* pChannel)
{
  AL_TStreamMetaData* pStreamMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int32_t iTotalSize = 0;

  for(int32_t iSection = 0; iSection < pStreamMetaData->uNumSection; iSection++)
    iTotalSize += pStreamMetaData->pSections[iSection].uLength;

  int32_t const iChunk = 512;
  int32_t const iSeiMandatorySize = (pChannel->uEncHeight <= 720) ? iChunk * 10 : iChunk * 18;
  int32_t const iConfigMandatorySize = 512;
  int32_t iPadding = (iCodedSliceSize + iSeiMandatorySize + iConfigMandatorySize) - iTotalSize;

  if(iPadding > 0)
  {
    int32_t iLastDataSection = AL_StreamMetaData_GetLastSectionOfFlag(pStreamMetaData, AL_SECTION_END_FRAME_FLAG) - 1;
    int32_t iOffset = pStreamMetaData->pSections[iLastDataSection].uOffset + pStreamMetaData->pSections[iLastDataSection].uLength + 1;
    Rtos_Assert((size_t)iOffset + iPadding < AL_Buffer_GetSize(pStream));
    Rtos_Memset(AL_Buffer_GetData(pStream) + iOffset, 0x00, iPadding);
    pStreamMetaData->pSections[iLastDataSection].uLength += iPadding;
  }
}

void AVC_GenerateSections(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int32_t iPicID, bool bMustWritePPS, bool bMustWriteAUD)
{
  AL_TNuts nuts = CreateAvcNuts();
  AL_TNalsData nalsData = AL_ExtractNalsData(pCtx, 0, iPicID);
  nalsData.bMustWritePPS = bMustWritePPS;
  nalsData.bMustWriteAud = bMustWriteAUD;
  AL_TEncChanParam const* pChannel = &pCtx->pSettings->tChParam[0];
  bool bForceSEIRecoveryPointOnIDR = false;

  if(AL_IS_XAVC(pChannel->eProfile) && !AL_IS_INTRA_PROFILE(pChannel->eProfile))
    bForceSEIRecoveryPointOnIDR = true;

  bool bSubframeLatency = false;
  bSubframeLatency = pChannel->bSubframeLatency;

  GenerateSections(AL_GetAvcRbspWriter(), nuts, &nalsData, pStream, pPicStatus, 0, pChannel->uNumSlices, bSubframeLatency, bForceSEIRecoveryPointOnIDR);

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    padConfig(pStream);

    if(pPicStatus->bIsFirstSlice)
      padSeiPrefix(pStream, pChannel);

    if(pPicStatus->bIsLastSlice)
      padCodedSliceData(BitsToBytes(pChannel->tRCParam.pMaxPictureSize[AL_SLICE_I]), pStream, pChannel);
  }
}
