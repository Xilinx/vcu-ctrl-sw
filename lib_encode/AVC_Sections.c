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

#include "AVC_Sections.h"
#include "lib_bitstream/AVC_RbspEncod.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/Nuts.h"
#include "lib_common/Utils.h"
#include "lib_assert/al_assert.h"

AL_TNalHeader GetNalHeaderAvc(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId)
{
  (void)uTempId, (void)uLayerId;
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
    AL_AVC_NUT_AUD,
    AL_AVC_NUT_FD,
    AL_AVC_NUT_PREFIX_SEI,
    /* sei suffix do not really exist in AVC. use a prefix nut */
    AL_AVC_NUT_PREFIX_SEI,
  };
  return nuts;
}

static int getSectionSize(AL_TStreamMetaData* pMetaData, AL_ESectionFlags eFlags)
{
  int iSize = 0;

  for(int iSection = 0; iSection < pMetaData->uNumSection; iSection++)
  {
    AL_TStreamSection* pSection = &pMetaData->pSections[iSection];

    if(pSection->eFlags & eFlags)
      iSize += pSection->uLength;
  }

  return iSize;
}

static void padConfig(AL_TBuffer* pStream)
{
  int const iChunk = 512;
  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int iConfigSize = getSectionSize(pMetaData, AL_SECTION_CONFIG_FLAG);

  if(iConfigSize >= iChunk)
    return;

  int iLastConfigSection = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_CONFIG_FLAG);

  if(iLastConfigSection < 0)
    return;

  AL_TStreamSection* pLastConfigSection = &pMetaData->pSections[iLastConfigSection];
  int iPaddingSize = iChunk - iConfigSize;
  Rtos_Memset(AL_Buffer_GetData(pStream) + pLastConfigSection->uOffset + pLastConfigSection->uLength, 0x00, iPaddingSize);
  pLastConfigSection->uLength += iPaddingSize;
}

static void padSeiPrefix(AL_TBuffer* pStream, AL_TEncChanParam const* pChannel)
{
  int const iChunk = 512;
  int const iSeiMandatorySize = (pChannel->uEncHeight <= 720) ? iChunk * 10 : iChunk * 18;
  AL_Assert(iSeiMandatorySize <= AL_ENC_MAX_SEI_SIZE);

  AL_TStreamMetaData* pMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int iSeiSize = getSectionSize(pMetaData, AL_SECTION_SEI_PREFIX_FLAG);

  if(iSeiSize >= iSeiMandatorySize)
    return;

  int iPaddingSize = iSeiMandatorySize - iSeiSize;

  int iLastSeiSection = AL_StreamMetaData_GetLastSectionOfFlag(pMetaData, AL_SECTION_SEI_PREFIX_FLAG);

  if(iLastSeiSection < 0)
  {
    int iOffset = AL_ENC_MAX_CONFIG_HEADER_SIZE + 1;
    Rtos_Memset(AL_Buffer_GetData(pStream) + iOffset, 0x00, iPaddingSize);
    AL_StreamMetaData_AddSeiSection(pMetaData, true, iOffset, iPaddingSize);
    return;
  }

  AL_TStreamSection* pLastSeiSection = &pMetaData->pSections[iLastSeiSection];
  Rtos_Memset(AL_Buffer_GetData(pStream) + pLastSeiSection->uOffset + pLastSeiSection->uLength, 0x00, iPaddingSize);
  pLastSeiSection->uLength += iPaddingSize;
}

static void padCodedSliceData(int iCodedSliceSize, AL_TBuffer* pStream, AL_TEncChanParam const* pChannel)
{
  AL_TStreamMetaData* pStreamMetaData = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  int iTotalSize = 0;

  for(int iSection = 0; iSection < pStreamMetaData->uNumSection; iSection++)
    iTotalSize += pStreamMetaData->pSections[iSection].uLength;

  int const iChunk = 512;
  int const iSeiMandatorySize = (pChannel->uEncHeight <= 720) ? iChunk * 10 : iChunk * 18;
  int const iConfigMandatorySize = 512;
  int iPadding = (iCodedSliceSize + iSeiMandatorySize + iConfigMandatorySize) - iTotalSize;

  if(iPadding > 0)
  {
    int iLastDataSection = AL_StreamMetaData_GetLastSectionOfFlag(pStreamMetaData, AL_SECTION_END_FRAME_FLAG) - 1;
    int iOffset = pStreamMetaData->pSections[iLastDataSection].uOffset + pStreamMetaData->pSections[iLastDataSection].uLength + 1;
    AL_Assert((size_t)iOffset + iPadding < AL_Buffer_GetSize(pStream));
    Rtos_Memset(AL_Buffer_GetData(pStream) + iOffset, 0x00, iPadding);
    pStreamMetaData->pSections[iLastDataSection].uLength += iPadding;
  }
}

void AVC_GenerateSections(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iPicID, bool bMustWritePPS)
{
  AL_TNuts nuts = CreateAvcNuts();
  AL_TNalsData nalsData = AL_ExtractNalsData(pCtx, 0, iPicID);
  nalsData.bMustWritePPS = bMustWritePPS;
  AL_TEncChanParam const* pChannel = &pCtx->pSettings->tChParam[0];
  bool bForceSEIRecoveryPointOnIDR = false;

  if(AL_IS_XAVC(pChannel->eProfile) && !AL_IS_INTRA_PROFILE(pChannel->eProfile))
    bForceSEIRecoveryPointOnIDR = true;
  GenerateSections(AL_GetAvcRbspWriter(), nuts, &nalsData, pStream, pPicStatus, 0, pChannel->uNumSlices, pChannel->bSubframeLatency, bForceSEIRecoveryPointOnIDR);

  if(AL_IS_XAVC_CBG(pChannel->eProfile) && AL_IS_INTRA_PROFILE(pChannel->eProfile))
  {
    padConfig(pStream);

    if(pPicStatus->bIsFirstSlice)
      padSeiPrefix(pStream, pChannel);

    if(pPicStatus->bIsLastSlice)
      padCodedSliceData(BitsToBytes(pChannel->tRCParam.pMaxPictureSize[AL_SLICE_I]), pStream, pChannel);
  }
}

