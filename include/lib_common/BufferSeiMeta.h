/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

