/******************************************************************************
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "SkippedPicture.h"
/*************************************************************************//*!
   \brief The GenerateSkippedPicture function generate Slice data for skipped
   picture.
   \param[out] pSkipPict Pointer to TSkippedPicture that receives the skipped
   picture slice data
   \param[in] iWidth Encoded picture width
   \param[in] iHeight Encoded picture height
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uLog2MinCuSize Min size of a coding unit (log2)
   \param[in] iTileColumns Number of tile columns in the picture
   \param[in] iTileRows Number of tile rows in the picture
   \param[in] pTileWidths Array with the tile column widths
   \param[in] pTileHeights Array of the tile row heights
   \param[in] bSliceSplit Specifies if we want to generate multiple skipped-slices,
   one for each tile row, or if we want only one skipped-slice for the whole frame
*****************************************************************************/
extern bool AL_HEVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iWidth, int iHeight, uint8_t uLog2MaxCuSize, uint8_t uLog2MinCuSize, int iTileColumns, int iTileRows, uint16_t* pTileWidths, uint16_t* pTileHeights, bool bSliceSplit);

/****************************************************************************/

/*@}*/

