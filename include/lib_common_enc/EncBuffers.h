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
   \addtogroup Encoder_Buffers Buffer size
   \ingroup Encoder

   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

#include "lib_common/BufCommon.h"
#include "lib_common/BufferPixMapMeta.h"

#include "lib_common_enc/EncChanParam.h"
#include "lib_common_enc/QPTable.h"

// Encoder Parameter Buf 2 Flag,  Size, Offset
static const AL_TBufInfo EP2_BUF_QP_CTRL =
{
  1, 48, 0
}; // only 20 bytes used
static const AL_TBufInfo EP2_BUF_SEG_CTRL =
{
  2, AL_QPTABLE_SEGMENTS_SIZE, 48
};
static const AL_TBufInfo EP2_BUF_QP_BY_MB =
{
  4, 0, 48 + AL_QPTABLE_SEGMENTS_SIZE
}; // no fixed size

/*************************************************************************//*!
   \brief Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeEP2(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize);

// AL_DEPRECATED("Doesn't support pitch different of AL_EncGetMinPitch. Use AL_GetAllocSizeSrc(). Will be removed in 0.9")
uint32_t AL_GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt);

/*************************************************************************//*!
   \brief Retrieves the size of a Source YUV frame buffer
   \param[in] tDim Frame size in pixels
   \param[in] eChromaMode Chroma Mode
   \param[in] eSrcFmt Source format used by the HW IP
   \param[in] iPitch Pitch / stride of the source frame buffer
   \param[in] iStrideHeight The height used for buffer allocation. Might be
   greater than the frame height when frame-height is non 8-multiple, or to
   customize offset between luma and chroma.
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSizeSrc(AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight);

/*************************************************************************//*!
   \brief Retrieves the size of one pixel component of a YUV frame buffer
   \param[in] eSrcFmt Source format used by the HW IP
   \param[in] iPitch Pitch / stride of the source frame buffer
   \param[in] iStrideHeight The height used for buffer allocation
   \param[in] eChromaMode Chroma Mode
   \param[in] ePlaneId The pixel plane type. Must not be a map plane.
   \return maximum size (in bytes) needed for the component
*****************************************************************************/
uint32_t AL_GetAllocSizeSrc_PixPlane(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight, AL_EChromaMode eChromaMode, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Retrieves the minimal pitch value supported by the ip depending
   on the source format
   \param[in] iWidth Frame width in pixel unit
   \param[in] uBitDepth YUV bit-depth
   \param[in] eStorageMode Source Storage Mode
   \return pitch value in bytes
*****************************************************************************/
int AL_EncGetMinPitch(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode);

/*************************************************************************//*!
   \brief Retrieves the Source frame buffer storage mode depending on Source mode
   \param[in] eSrcMode Source Mode
   \return Source Storage Mode
*****************************************************************************/
AL_EFbStorageMode AL_GetSrcStorageMode(AL_ESrcMode eSrcMode);

/*************************************************************************//*!
   \brief Check if the Source frame buffer is compressed depending on Source mode
   \param[in] eSrcMode Source Mode
   \return Source Storage Mode
*****************************************************************************/
bool AL_IsSrcCompressed(AL_ESrcMode eSrcMode);

AL_DEPRECATED("Renamed as AL_EncGetMinPitch, Will be removed in 0.9")
int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode);

AL_DEPRECATED("Use AL_GetAllocSizeSrc_PixPlane.")
uint32_t AL_GetAllocSizeSrc_Y(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight);
AL_DEPRECATED("Use AL_GetAllocSizeSrc_PixPlane.")
uint32_t AL_GetAllocSizeSrc_UV(AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight, AL_EChromaMode eChromaMode);

/*@}*/

