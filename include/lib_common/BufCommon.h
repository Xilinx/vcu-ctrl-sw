// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/FourCC.h"

/*************************************************************************//*!
   \brief If the framebuffer is stored in raster, the pitch represents the number of bytes
   to go to the next line, so you get one line.
   If the framebuffer is stored in tiles, the pitch represents the number of bytes
   to go to the next line of tile. As a tile height is superior to one line,
   you have to skip multiple lines to go to the next line of tile.
   \param[in] eFrameBufferStorageMode how the framebuffer is stored in memory
   \return Number of lines in the pitch
*****************************************************************************/
int AL_GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode);

/*************************************************************************//*!
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaPitch Pitch of luma plane
   \return pitch >= 0 of a chroma plane on success, pitch < 0 on failure
*****************************************************************************/
int AL_GetChromaPitch(TFourCC tFourCC, int iLumaPitch);

/*************************************************************************//*!
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaWidth Width of luma plane
   \return width >= 0 of a chroma plane on success, width < 0 on failure
*****************************************************************************/
int AL_GetChromaWidth(TFourCC tFourCC, int iLumaWidth);

/*************************************************************************//*!
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaHeight Height of luma plane
   \return height >= 0 of a chroma plane on success; height < 0 on failure
*****************************************************************************/
int AL_GetChromaHeight(TFourCC tFourCC, int iLumaHeight);

/****************************************************************************/
/* Useful for traces */
void AL_CleanupMemory(void* pDst, size_t uSize);
extern int AL_CLEAN_BUFFERS;

/*@}*/

