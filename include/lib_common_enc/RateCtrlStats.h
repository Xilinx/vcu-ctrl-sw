// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

/*************************************************************************//*!
   \brief Statistics useful for rate-control algorithms
*****************************************************************************/
typedef struct al_t_RateCtrl_Statistics
{
  uint32_t uNumLCUs; /*!< Total number of encoded LCU */
  uint32_t uNumBytes; /*!< Number of bytes in the stream or Estimated Number of bytes in the stream (AVC multi-core only) */
  uint32_t uNumBins; /*!< Number of CABAC bin */
  uint32_t uNumIntra; /*!< Number of 8x8 blocks coded with intra mode */
  uint32_t uNumSkip; /*!< Number of 8x8 blocks coded with skip mode */
  uint32_t uNumCU8x8; /*!< Number of 8x8 CUs */
  uint32_t uNumCU16x16; /*!< Number of 16x16 CUs */
  uint32_t uNumCU32x32; /*!< Number of 32x32 CUs */
  int32_t uSumQP; /*!< Sum of QP value used to encode each block unit */
  int16_t uMinQP; /*!< Minimum QP value */
  int16_t uMaxQP; /*!< Maximum QP value */
}AL_RateCtrl_Statistics;

/*@}*/
