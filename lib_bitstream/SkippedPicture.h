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

#include "lib_rtos/types.h"

#define AL_MAX_TILE_ROWS 22 // see table A.1 of the HEVC specification

/*************************************************************************//*!
   \brief This structure is designed to store slice data information of
   skipped picture.
   \see GenerateSkippedPicture
*****************************************************************************/
typedef struct AL_t_SkippedPicture
{
  AL_HANDLE hBuf; /*!< Handle of the skipped picture buffer */
  uint8_t* pData; /*!< Data pointer from hBuf for storing precomputed skipped picture bitstream */
  int iBufSize; /*!< Size (in byte of pBuffer */

  int iNumBits; /*!< Number of bits used by the skipped picture */
  int iNumBins; /*!< Number of bins used by the skipped picture */

  int iNumTiles; /*!< Number of tile in the skipped picture */
  uint32_t uTileSizes[AL_ENC_NUM_CORES * AL_MAX_TILE_ROWS]; /*!< Tile size in Bytes */
}AL_TSkippedPicture;

/*****************************************************************************/
#include "HEVC_SkippedPict.h"
#include "AVC_SkippedPict.h"

