/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#pragma once
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferCircMeta.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufConst.h"

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

#include <assert.h>
static AL_INLINE void CircBuffer_ConsumeUpToOffset(AL_TBuffer* stream, int32_t iNewOffset)
{
  AL_TCircMetaData* pCircMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(stream, AL_META_TYPE_CIRCULAR);

  if(iNewOffset < pCircMeta->iOffset)
    pCircMeta->iAvailSize -= iNewOffset + stream->zSize - pCircMeta->iOffset;
  else
    pCircMeta->iAvailSize -= iNewOffset - pCircMeta->iOffset;
  pCircMeta->iOffset = iNewOffset;

  assert(pCircMeta->iAvailSize >= 0);
}

static AL_INLINE void CircBuffer_Init(TCircBuffer* pBuf)
{
  pBuf->iOffset = 0;
  pBuf->iAvailSize = 0;
}

int32_t ComputeRndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode, int iAlignment);

