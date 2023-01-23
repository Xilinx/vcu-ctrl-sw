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

