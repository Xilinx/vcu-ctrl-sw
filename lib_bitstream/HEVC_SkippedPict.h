// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

