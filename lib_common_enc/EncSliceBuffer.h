/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "lib_common/BufCommonInternal.h"
#include "lib_common_enc/EncRecBuffer.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferPixMapMeta.h"
#include "EncEPBuffer.h"

/*************************************************************************//*!
   \brief Slice Buffers: Set of buffers needed to encode a slice
*****************************************************************************/
typedef struct t_Span
{
  uint8_t* pBuf;
  uint32_t uSize;
}TSpan;

typedef struct t_SliceBuffersEnc
{
  AL_TBuffer* pSrc; /*!< Pointer to Source frame buffer. */

  TSpan RefA[AL_PLANE_MAX_ENUM]; /*!< Pointer to frame buffers used as first reference picture.
                                    (Required only for P Slice encoding; Can be set to
                                    NULL for I Slice encoding) */
  TSpan RefB[AL_PLANE_MAX_ENUM]; /*!< Pointer to frame buffers used as second reference picture.
                                    (Required only for B slice or P Slice with 2 reference
                                    pictures. Can be set to NULL if not used) */
  TSpan Rec[AL_PLANE_MAX_ENUM]; /*!< Pointer to frame buffers that receives reconstructed slice. */
  TBufferMV Coloc; /*!< Pointer to co-located buffer. */
  TBufferMV MV; /*!< Pointer to MV buffer that contains valid POC List section
                   and that receives Motion Vectors of the encoded slice. */

  TBuffer CompData; /*!< Pointer to the intermediate compressed buffer */
  TBuffer CompMap; /*!< Pointer to the intermediate compressed mapping */

  TBuffer* pWPP; /*!< Pointer to Wavefront size buffer */
  TBufferEP* pEP1; /*!< Pointer to the lambdas & SclMtx buffer */
  TBufferEP* pEP2; /*!< Pointer to the QP table buffer */

  TBufferEP Hwrc[AL_ENC_NUM_CORES]; /*!< Pointer to the HwRcCtx buffers for each core */

  TCircBuffer Stream;
}TSliceBuffersEnc;

