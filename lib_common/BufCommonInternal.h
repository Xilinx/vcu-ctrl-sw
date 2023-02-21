// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferCircMeta.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufConst.h"
#include "lib_common/MemDesc.h"
#include "lib_assert/al_assert.h"

/*************************************************************************//*!
   \brief Generic Buffer
*****************************************************************************/
typedef struct AL_t_Buffer
{
  TMemDesc tMD; /*!< Memory descriptor associated to the buffer */
}TBuffer;

/*************************************************************************//*!
   \brief Buffer with Motion Vectors content
*****************************************************************************/
typedef TBuffer TBufferMV;

/*************************************************************************//*!
   \brief Circular Buffer
*****************************************************************************/
typedef struct t_CircBuffer
{
  TMemDesc tMD; /*!< Memory descriptor associated to the buffer */

  int32_t iOffset; /*!< Initial Offset in Circular Buffer */
  int32_t iAvailSize; /*!< Avail Space in Circular Buffer */
}TCircBuffer;

static inline void CircBuffer_ConsumeUpToOffset(AL_TBuffer* stream, int32_t iNewOffset)
{
  AL_TCircMetaData* pCircMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(stream, AL_META_TYPE_CIRCULAR);

  if(iNewOffset < pCircMeta->iOffset)
    pCircMeta->iAvailSize -= iNewOffset + AL_Buffer_GetSize(stream) - pCircMeta->iOffset;
  else
    pCircMeta->iAvailSize -= iNewOffset - pCircMeta->iOffset;
  pCircMeta->iOffset = iNewOffset;

  AL_Assert(pCircMeta->iAvailSize >= 0);
}

static inline void CircBuffer_Init(TCircBuffer* pBuf)
{
  pBuf->iOffset = 0;
  pBuf->iAvailSize = 0;
}

int32_t ComputeRndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode, int iAlignment);

