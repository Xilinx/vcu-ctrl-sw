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

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/FourCC.h"

/*************************************************************************//*!
   If the framebuffer is stored in raster, the pitch represents the number of bytes
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
   \return pitch of a chroma plane
*****************************************************************************/
int AL_GetChromaPitch(TFourCC tFourCC, int iLumaPitch);

/*************************************************************************//*!
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaWidth Width of luma plane
   \return width of a chroma plane
*****************************************************************************/
int AL_GetChromaWidth(TFourCC tFourCC, int iLumaWidth);

/*************************************************************************//*!
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaHeight Height of luma plane
   \return height of a chroma plane
*****************************************************************************/
int AL_GetChromaHeight(TFourCC tFourCC, int iLumaHeight);

/****************************************************************************/
/* Useful for traces */
void AL_CleanupMemory(void* pDst, size_t uSize);
extern int AL_CLEAN_BUFFERS;

/*@}*/

