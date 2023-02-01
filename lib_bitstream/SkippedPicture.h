/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

