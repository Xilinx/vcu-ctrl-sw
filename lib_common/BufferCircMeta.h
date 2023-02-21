// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufferMeta.h"

typedef struct AL_t_CircMetaData
{
  AL_TMetaData tMeta;
  int32_t iOffset;
  int32_t iAvailSize;
  bool bLastBuffer;
}AL_TCircMetaData;

AL_TCircMetaData* AL_CircMetaData_Create(int32_t iOffset, int32_t iAvailSize, bool bLastBuffer);
AL_TCircMetaData* AL_CircMetaData_Clone(AL_TCircMetaData* pMeta);

