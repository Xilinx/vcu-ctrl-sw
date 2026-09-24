// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Buffers
   !@{
   \file
******************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/FourCC.h"

/*****************************************************************************
   \brief If the framebuffer is stored in raster, the pitch represents the number of bytes
   to go to the next line, so you get one line.
   If the framebuffer is stored in tiles, the pitch represents the number of bytes
   to go to the next line of tile. As a tile height is superior to one line,
   you have to skip multiple lines to go to the next line of tile.
   \param[in] eFrameBufferStorageMode how the framebuffer is stored in memory
   \return Number of lines in the pitch
*****************************************************************************/
int32_t AL_GetNumLinesInPitch(AL_EFbStorageMode eFrameBufferStorageMode, AL_EChromaMode eChromaMode, uint8_t uBitDepth);

/*****************************************************************************
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaPitch Pitch of luma plane
   \return pitch >= 0 of a chroma plane on success, pitch < 0 on failure
*****************************************************************************/
int32_t AL_GetChromaPitch(TFourCC tFourCC, int32_t iLumaPitch);

/*****************************************************************************
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaWidth Width of luma plane
   \return width >= 0 of a chroma plane on success, width < 0 on failure
*****************************************************************************/
int32_t AL_GetChromaWidth(TFourCC tFourCC, int32_t iLumaWidth);

/*****************************************************************************
   \param[in] tFourCC FourCC of a framebuffer
   \param[in] iLumaHeight Height of luma plane
   \return height >= 0 of a chroma plane on success; height < 0 on failure
*****************************************************************************/
int32_t AL_GetChromaHeight(TFourCC tFourCC, int32_t iLumaHeight);

/****************************************************************************/
/* Useful for traces */
extern int32_t AL_CLEAN_BUFFERS;

/*****************************************************************************
   \param[in] iWidth Width of luma plane
   \param[in] pPicFormat Picture format of the frame buffer
   \param[in] iPitchAlignment Final alignment of the pitch value
   \return Calculated pitch of the buffer
*****************************************************************************/
int32_t AL_GetLumaPixPlanePitch(int32_t iWidth, AL_TPicFormat const* pPicFormat, int32_t iPitchAlignment);

/*****************************************************************************
   \param[in] pPicFormat Picture format of the frame buffer
   \param[in] iLumaPitch Pitch of the corresponding Luma plane
   \return Calculated chroma pitch of the buffer
*****************************************************************************/
int32_t AL_GetChromaPixPlanePitch(AL_TPicFormat const* pPicFormat, int32_t iLumaPitch);

/*!@}*/
