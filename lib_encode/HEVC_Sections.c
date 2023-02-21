// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "HEVC_Sections.h"
#include "lib_common/Nuts.h"
#include "lib_bitstream/HEVC_RbspEncod.h"

static AL_TNalHeader GetNalHeaderHevc(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId)
{
  (void)uNalRefIdc;
  AL_TNalHeader nh;
  nh.size = 2;
  nh.bytes[0] = ((uLayerId & 0x20) >> 5) | ((uNUT & 0x3F) << 1);
  nh.bytes[1] = (uTempId + 1) | ((uLayerId & 0x1F) << 3);
  return nh;
}

AL_TNuts CreateHevcNuts(void)
{
  AL_TNuts nuts =
  {
    &GetNalHeaderHevc,
    AL_HEVC_NUT_SPS,
    AL_HEVC_NUT_PPS,
    AL_HEVC_NUT_VPS,
    AL_HEVC_NUT_AUD,
    AL_HEVC_NUT_FD,
    AL_HEVC_NUT_PREFIX_SEI,
    AL_HEVC_NUT_SUFFIX_SEI,
    0,
    0,
  };
  return nuts;
}

void HEVC_GenerateSections(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayerID, int iPicID, bool bMustWritePPS, bool bMustWriteAUD)
{
  AL_TNuts nuts = CreateHevcNuts();
  AL_TNalsData nalsData = AL_ExtractNalsData(pCtx, iLayerID, iPicID);
  nalsData.bMustWritePPS = bMustWritePPS;
  nalsData.bMustWriteAud = bMustWriteAUD;
  AL_TEncChanParam const* pChannel = &pCtx->pSettings->tChParam[0];
  GenerateSections(AL_GetHevcRbspWriter(), nuts, &nalsData, pStream, pPicStatus, iLayerID, pChannel->uNumSlices, pChannel->bSubframeLatency, false);
}

