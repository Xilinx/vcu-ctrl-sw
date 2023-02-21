// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

