// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

#define AL_MAX_TILE_ROWS 22 // see table A.1 of the HEVC specification

/*************************************************************************//*!
   \brief Store information on skipped slice data.
*****************************************************************************/
typedef struct AL_t_SkippedSlice
{
  uint32_t uOffset; /*!< Offset to slice data in the skipped picture buffer */
  uint32_t uSize; /*!< Non-anti-emulated slice data size in bytes */
  uint16_t uNumTiles; /*!< Number of tiles in the slice */
}AL_TSkippedSlice;

/*************************************************************************//*!
   \brief This structure is designed to store data information of a skipped picture.
   \see GenerateSkippedPicture
*****************************************************************************/
typedef struct AL_t_SkippedPicture
{
  AL_HANDLE hBuf; /*!< Handle of the skipped picture buffer */
  uint8_t* pData; /*!< Data pointer from hBuf for storing precomputed skipped picture bitstream */
  int iBufSize; /*!< Size in byte of hBuf */

  int iNumSlices;
  AL_TSkippedSlice tSkippedSlice[AL_MAX_TILE_ROWS];
  uint32_t uTileSizes[AL_ENC_NUM_CORES * AL_MAX_TILE_ROWS]; /*!< Anti-emulated tile size in bytes */

  int iNumBins; /*!< Number of bins used by the skipped picture */
}AL_TSkippedPicture;

/*****************************************************************************/
#include "HEVC_SkippedPict.h"
#include "AVC_SkippedPict.h"

