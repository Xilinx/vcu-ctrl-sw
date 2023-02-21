// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "IP_Stream.h"
#include "lib_bitstream/IRbspWriter.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SEI.h"
#include "lib_common/AUD.h"
#include "lib_common/HDR.h"

typedef struct t_nuts
{
  AL_TNalHeader (* GetNalHeader)(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId);
  int spsNut;
  int ppsNut;
  int vpsNut;
  int audNut;
  int fdNut;
  int seiPrefixNut;
  int seiSuffixNut;
  int phNut;
  int apsNut;
}AL_TNuts;

typedef struct
{
  int initialCpbRemovalDelay;
  int cpbRemovalDelay;
  AL_THDRSEIs* pHDRSEIs;
}AL_TSeiData;

typedef struct
{
  AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned;
  AL_TVps* vps;
  AL_TSps* sps;
  AL_TPps* pps;
  AL_TAud* aud;

  bool bMustWriteAud;
  AL_EFillerCtrlMode fillerCtrlMode;
  bool bMustWritePPS;
  bool bMustWriteDynHDR;
  AL_ESeiFlag seiFlags;
  AL_TSeiData seiData;
}AL_TNalsData;

void GenerateSections(IRbspWriter* writer, AL_TNuts Nuts, AL_TNalsData const* pNalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayerID, int iNumSlices, bool bSubframeLatency, bool bForceSEIRecoveryPointOnIDR);
int AL_WriteSeiSection(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);
bool AL_CreateNuts(AL_TNuts* nuts, AL_EProfile eProfile);

