// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_bitstream/IRbspWriter.h"
#include "IP_Stream.h"

typedef struct t_NalUnit
{
  void (* Write)(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId);
  void const* param;
  int nut;
  int nalRefIdc;
  int layerId;
  int tempId;
  AL_TNalHeader header;
}AL_TNalUnit;

AL_TNalUnit AL_CreateAud(int nut, AL_TAud* aud, int tempId);
AL_TNalUnit AL_CreateSps(int nut, AL_TSps* sps, int layerId, int tempId);
AL_TNalUnit AL_CreatePps(int nut, AL_TPps* pps, int layerId, int tempId);
AL_TNalUnit AL_CreateVps(int nut, AL_TVps* vps, int tempId);

#include "lib_common_enc/EncPicInfo.h"
typedef struct t_SeiPrefixAPSCtx
{
  AL_TSps* sps;
  AL_THevcVps* vps;
}AL_TSeiPrefixAPSCtx;

AL_TNalUnit AL_CreateSeiPrefixAPS(AL_TSeiPrefixAPSCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixCtx
{
  AL_TSps* sps;
  int cpbInitialRemovalDelay;
  int cpbRemovalDelay;
  uint32_t uFlags;
  AL_TEncPicStatus const* pPicStatus;
  AL_THDRSEIs* pHDRSEIs;
}AL_TSeiPrefixCtx;

AL_TNalUnit AL_CreateSeiPrefix(AL_TSeiPrefixCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixUDUCtx
{
  uint8_t uuid[16];
  int8_t numSlices;
}AL_TSeiPrefixUDUCtx;

AL_TNalUnit AL_CreateSeiPrefixUDU(AL_TSeiPrefixUDUCtx* ctx, int nut, int tempId);

typedef struct t_SeiExternalCtx
{
  uint8_t* pPayload;
  int iPayloadType;
  int iPayloadSize;
}AL_TSeiExternalCtx;

AL_TNalUnit AL_CreateExternalSei(AL_TSeiExternalCtx* ctx, int nut, int tempId);

