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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Encoded slice status
*****************************************************************************/
typedef struct AL_t_SliceStatus
{
  bool bLcuOverflow; /*!< True when number of bit used to encode one LCU exceed */
  bool bBufOverflow; /*!< True when number of bit used to encode one slice exceed the buffer available size*/

  uint32_t uNumLCUs; /*!< Total number of encoded LCU */

  int32_t iNumBytes; /*!< Number of bytes in the stream */
  uint32_t uNumBins; /*!< Number of CABAC bin */
  uint32_t uFrmTagSize; /*!< Number of bytes in the frame compressed header (VP9 only) */
  uint32_t uNumBitsRes; /*!< Number of bits used for residual encoding */
  uint32_t uNum4x4WithRes; /*!< Number of 4x4 blocks with non-zero residuals */
  uint32_t uNumIntra; /*!< Number of 8x8 blocks coded with intra mode */
  uint32_t uNumSkip; /*!< Number of 8x8 blocks coded with skip mode */
  uint32_t uNumCU8x8; /*!< Number of 8x8 CUs */
  uint32_t uNumCU16x16; /*!< Number of 16x16 CUs */
  uint32_t uNumCU32x32; /*!< Number of 32x32 CUs */
  uint32_t uNumCU64x64; /*!< Number of 64x64 CUs */
  uint32_t uSumCplx; /*!< Sum of LCU complexity */
  int32_t iSumQP; /*!< Sum of QP value used to encode each block unit */
  int16_t iSliceQP;  /*!< Slice QP value */
  int16_t iMinQP; /*!< Minimum QP value */
  int16_t iMaxQP; /*!< Maximum QP value */
  uint16_t uNumSlices; /*!< Number of slices */
  int32_t iEstimNumBytes; /*!< Estimated Number of bytes in the stream (AVC multi-core only) */

  uint32_t SyntaxElements;
}AL_TEncSliceStatus;

/*@}*/

