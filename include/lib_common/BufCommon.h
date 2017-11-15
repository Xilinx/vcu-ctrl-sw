/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/MemDesc.h"
#include "lib_common/FourCC.h"
#include "lib_common/OffsetYC.h"

#define MAX_REF 16 /*!< max number of frame buffer */

#define AL_MAX_COLUMNS_TILE 20 // warning : true only for main profile (4K = 10; 8K = 20)
#define AL_MAX_ROWS_TILE 22  // warning : true only for main profile (4K = 11; 8K = 22)

#define AL_MAX_NUM_TILE ((AL_MAX_COLUMNS_TILE)*(AL_MAX_ROWS_TILE))
#define AL_MAX_NUM_WPP 528// max line number : sqrt(MaxLumaSample size * 8) / 32. (4K = 264; 8K = 528);

#define AL_MAX_ENTRY_POINT (((AL_MAX_NUM_TILE) > (AL_MAX_NUM_WPP)) ? (AL_MAX_NUM_TILE) : (AL_MAX_NUM_WPP))

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

  int32_t uOffset; /*!< Initial Offset in Circular Buffer */
  int32_t uAvailSize; /*!< Avail Space in Circular Buffer */
}TCircBuffer;

#include <assert.h>
static AL_INLINE void CircBuffer_ConsumeUpToOffset(TCircBuffer* stream, int32_t uNewOffset)
{
  if(uNewOffset < stream->uOffset)
    stream->uAvailSize -= uNewOffset + stream->tMD.uSize - stream->uOffset;
  else
    stream->uAvailSize -= uNewOffset - stream->uOffset;
  stream->uOffset = uNewOffset;

  assert(stream->uAvailSize >= 0);
}

static AL_INLINE void CircBuffer_Init(TCircBuffer* pBuf)
{
  pBuf->uOffset = 0;
  pBuf->uAvailSize = 0;
}

typedef struct AL_t_OffsetYC AL_TOffsetYC;
/*************************************************************************//*!
   \brief Frame buffer stored as IYUV planar format (also called I420)
*****************************************************************************/
typedef struct t_BufferYuv
{
  TMemDesc tMD; /*!< Memory descriptor associated to the buffer */

  int iWidth; /*!< Width in pixel of the frame */
  int iHeight; /*!< Height in pixel of the frame */

  int iPitchY; /*!< offset in bytes between a Luma pixel and the Luma
                     pixel on the next line with same horizontal position*/
  int iPitchC; /*!< offset in bytes between a chroma pixel and the chroma
                     pixel on the next line with same horizontal position*/
  AL_TOffsetYC tOffsetYC; /*< offset for luma and chroma addresses */

  TFourCC tFourCC; /*!< FOURCC identifier */
}TBufferYuv;

/****************************************************************************/
int GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode);
int32_t ComputeRndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode, int iAlignment);

/****************************************************************************/
void AL_CleanupMemory(void* pDst, size_t uSize);

extern int AL_CLEAN_BUFFERS;

