// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup FourCC
   !@{
   \file
 *****************************************************************************/
#pragma once
#include "lib_common/FourCC.h"

/*****************************************************************************
   \brief Returns the FOURCC identifier of the decoded frame buffer generated
   by the decoder according to the chroma mode, the bitdepth of the stream
   and the storage mode of the ip
   \param[in] pPicFormat Pointer of the picture format
   \return return the corresponding TFourCC format, FOURCC(NULL) if unsupported
*****************************************************************************/
TFourCC AL_GetDecFourCC(AL_TPicFormat const* pPicFormat);

/*****************************************************************************
   \brief Returns the Chroma ordering format used by the decoder to store
   the specified ChromaMode
   \param[in] eChromaMode decoded picture chroma mode
   \return Returns the corresponding plane mode
*****************************************************************************/
AL_EPlaneMode AL_ChromaModeToPlaneMode(AL_EChromaMode eChromaMode);

/*****************************************************************************
   \brief Returns a TPicFormat according to the given parameters
   \param[in] eChromaMode picture chroma mode
   \param[in] uBitDepth picture bit depth
   \param[in] eStorageMode picture storage mode
   \param[in] bIsCompressed true if picture is compressed, false otherwise
   \param[in] ePlaneMode picture plane mode
   \return Returns the corresponding TPicFormat
*****************************************************************************/
AL_TPicFormat AL_GetDecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed, AL_EPlaneMode ePlaneMode);

/*!@}*/
