/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "SkippedPicture.h"
/*************************************************************************//*!
   \brief The GenerateSkippedPicture function generate Slice data for skipped
   picture.
   \param[out] pSkipPict Pointer to TSkippedPicture that receives the skipped
   picture slice data
   \param[in] iWidth Encoded picture width
   \param[in] iHeight Encoded picture height
   \param[in] uMaxCuSize Maximum coding unit Size
   \param[in] uMinCuSize Minimum coding unit Size
   \param[in] iTileColumns Number of tile columns in the picture
   \param[in] iTileRows Number of tile rows in the picture
   \param[in] pTileWidths Array with the tile column widths
   \param[in] pTileHeights Array of the tile row heights
*****************************************************************************/
extern bool AL_HEVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iWidth, int iHeight, uint8_t uMaxCuSize, uint8_t uMinCuSize, int iTileColumns, int iTileRows, uint16_t* pTileWidths, uint16_t* pTileHeights);

/****************************************************************************/

/*@}*/

