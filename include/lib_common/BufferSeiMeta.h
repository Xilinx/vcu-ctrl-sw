// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"

/*************************************************************************//*!
   \brief Sei messages
*****************************************************************************/
typedef struct AL_t_SeiMessage
{
  bool bPrefix;
  uint32_t type;
  uint8_t* pData;
  uint32_t size;
}AL_TSeiMessage;

typedef struct AL_t_SeiMetaData
{
  AL_TMetaData tMeta;
  uint8_t numPayload;
  uint8_t maxPayload;
  AL_TSeiMessage* payload;
  uint8_t* pBuf;
  uint32_t maxBufSize;
}AL_TSeiMetaData;

/*************************************************************************//*!
   \brief Create a sei metadata.
*****************************************************************************/
AL_TSeiMetaData* AL_SeiMetaData_Create(uint8_t uMaxPayload, uint32_t uMaxBufSize);
AL_TSeiMetaData* AL_SeiMetaData_Clone(AL_TSeiMetaData* pMeta);
bool AL_SeiMetaData_AddPayload(AL_TSeiMetaData* pMeta, AL_TSeiMessage payload);
uint8_t* AL_SeiMetaData_GetBuffer(AL_TSeiMetaData* pMeta);
void AL_SeiMetaData_Reset(AL_TSeiMetaData* pMeta);

/*@}*/

