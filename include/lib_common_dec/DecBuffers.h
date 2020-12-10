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
