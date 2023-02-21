// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Planes.h"
#include "lib_common/FourCC.h"
#include "lib_common/BufferMeta.h"

/*************************************************************************//*!
   \brief Give the size of a reconstructed picture buffer
   Restriction: The strideHeight is supposed to be the minimum stride height
   \param[in] tDim dimensions of the picture
   \param[in] iPitch luma pitch in bytes of the picture
   \param[in] eChromaMode chroma mode of the picture
   \param[in] bFrameBufferCompression will the frame buffer be compressed
   \param[in] eFbStorage frame buffer storage mode
   \return return the size of the reconstructed picture buffer
*****************************************************************************/
int AL_DecGetAllocSize_Frame(AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, bool bFrameBufferCompression, AL_EFbStorageMode eFbStorage);

/*************************************************************************//*!
   \brief Give the size of one pixel component of a reconstructed picture buffer
   \param[in] eFbStorage frame buffer storage modes
   \param[in] tDim dimensions of the picture
   \param[in] iPitch component pitch in bytes of the picture
   \param[in] eChromaMode Chroma Mode
   \param[in] ePlaneId The pixel plane type. Must not be a map plane.
   \return return the size of the pixel component of the reconstruct
*****************************************************************************/
int AL_DecGetAllocSize_Frame_PixPlane(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Create the AL_TMetaData associated to the reconstruct buffers
   \param[in] tDim Frame dimension (width, height) in pixel
   \param[in] tFourCC FourCC of the frame buffer
   \param[in] iPitch Pitch of the frame buffer
   \return the AL_TMetaData
*****************************************************************************/
AL_TMetaData* AL_CreateRecBufMetaData(AL_TDimension tDim, int iPitch, TFourCC tFourCC);

/*@}*/

AL_DEPRECATED("Renamed. Use AL_Decoder_GetMinPitch. Will be deleted in 0.9")
uint32_t AL_Decoder_RoundPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);
AL_DEPRECATED("Renamed. Use AL_Decoder_GetMinStrideHeight. Will be deleted in 0.9")
uint32_t AL_Decoder_RoundHeight(uint32_t uHeight);
// AL_DEPRECATED("Use AL_DecGetAllocSize_Frame. This function doesn't take the stride of the allocated buffer in consideration. Will be deleted in 0.9")
int AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFbStorage);
AL_DEPRECATED("Use AL_DecGetAllocSize_Frame_PixPlane.")
int AL_DecGetAllocSize_Frame_Y(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch);
AL_DEPRECATED("Use AL_DecGetAllocSize_Frame_PixPlane.")
int AL_DecGetAllocSize_Frame_UV(AL_EFbStorageMode eFbStorage, AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode);
