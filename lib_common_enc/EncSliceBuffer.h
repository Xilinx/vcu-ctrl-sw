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

#pragma once

#include "lib_common/BufCommon.h"
#include "lib_common_enc/EncRecBuffer.h"
#include "EncEPBuffer.h"

/*************************************************************************//*!
   \brief Slice Buffers : Set of buffers needed to encode a slice
*****************************************************************************/
typedef struct t_SliceBuffersEnc
{
  TBufferYuv Src; /*!< Pointer to Source frame buffer. */
  TBufferRec RefA; /*!< Pointer to frame buffer used as first reference picture.
                      (Required only for P Slice encoding; Can be set to
                      NULL for I Slice encoding) */
  TBufferRec RefB; /*!< Pointer to frame buffer used as second reference picture.
                      (Required only for B slice or P Slice with 2 reference
                      pictures. Can be set to NULL if not used) */
  TBufferRec Rec; /*!< Pointer to frame buffer that receives reconstructed slice. */
  TBufferMV Coloc; /*!< Pointer to co-located buffer. */
  TBufferMV MV; /*!< Pointer to MV buffer that contains valid POC List section
                   and that receives Motion Vectors of the encoded slice. */
  TBuffer CompData; /*!< Pointer to the intermediate compressed buffer */
  TBuffer CompMap; /*!< Pointer to the intermediate compressed mapping */

  TBuffer StrPart; /*!< Buffer receiving the encoded nal units map */

  TBuffer* pWPP; /*!< Pointer to Wavefront size buffer */
  TBufferEP* pEP1; /*!< Pointer to the lambdas & SclMtx buffer */
  TBufferEP* pEP2; /*!< Pointer to the QP table buffer */
  TBufferEP* pEP3; /*!< Pointer to the HwRcCtx buffer */

  TCircBuffer* pStream;  /*!< Pointer to the bitstream buffer */
}TSliceBuffersEnc;

/*************************************************************************//*!
   \brief Slice buffer information. This structure is a kind of shortcut to
   the full Frame buffer
*****************************************************************************/
typedef struct t_SliceInfoYuv
{
  int iMBWidth; /*!< Slice width in Macroblock unit*/
  int iMBHeight; /*!< Slice height in Macroblock unit*/
  AL_EChromaMode eChromaMode; /*!< Chroma sampling mode */

  uint8_t* pY; /*!< Shortcut pointer to the Luma plane */
  uint8_t* pU; /*!< Shortcut pointer to the Chroma U plane */
  uint8_t* pV; /*!< Shortcut pointer to the Chroma V plane */
}TSliceInfoYuv;

