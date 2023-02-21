// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/FourCC.h"
#include "lib_common/Planes.h"
#include "lib_rtos/types.h"
#include "lib_common_enc/EncChanParam.h"
#include "lib_common_enc/EncRecBuffer.h"

typedef struct
{
  AL_TPicFormat tRecPicFormat;
  TFourCC RecFourCC;
  AL_TPlaneDescription tPlanesDesc[AL_MAX_BUFFER_PLANES];
  int iNbPlanes;
  uint32_t uRecPicSize;
  bool bIsAvc;
}AL_TCommonChannelInfo;

void SetChannelInfo(AL_TCommonChannelInfo* pChanInfo, const AL_TEncChanParam* pChParam);

void SetRecPic(AL_TRecPic* pRecPic, AL_TAllocator* pAllocator, AL_HANDLE hRecBuf, AL_TCommonChannelInfo* pChanInfo, AL_TReconstructedInfo* pRecInfo);

