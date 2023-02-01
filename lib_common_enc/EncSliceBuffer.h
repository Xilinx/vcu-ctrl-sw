/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#pragma once

#include "lib_common/BufCommonInternal.h"
#include "lib_common_enc/EncRecBuffer.h"
#include "lib_common/BufferAPI.h"
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

