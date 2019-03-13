/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
   \addtogroup Encoder_Buffers Buffer size
   \ingroup Encoder

   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

#include "lib_common/versions.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/BufCommon.h"

#include "lib_common_enc/EncChanParam.h"

// EP2 masks
#define FLAG_INTRA_ONLY 0x40

#define MASK_QP 0x3F
#define MASK_FORCE_INTRA 0x40
#define MASK_FORCE_MV0 0x80
#define MASK_FORCE 0xC0


// Encoder Parameter Buf 2 Flag,  Size, Offset
static const AL_TBufInfo EP2_BUF_QP_CTRL =
{
  1, 48, 0
}; // only 20 bytes used
static const AL_TBufInfo EP2_BUF_SEG_CTRL =
{
  2, 16, 48
};
static const AL_TBufInfo EP2_BUF_QP_BY_MB =
{
  4, 0, 64
}; // no fixed size


/*************************************************************************//*!
   \brief Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeEP2(AL_TDimension tDim, AL_ECodec eCodec);

// AL_DEPRECATED("Doesn't support pitch different of AL_EncGetMinPitch. Use AL_GetAllocSizeSrc(). Will be removed in 0.9")
uint32_t AL_GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt);

/*************************************************************************//*!
   \brief Retrieves the size of a Source YUV frame buffer
   \param[in] tDim Frame size in pixels
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] eSrcFmt Source format used by the HW IP
   \param[in] iPitch pitch / stride of the source frame buffer
   \param[in] iStrideHeight the offset to the chroma in line of pixels
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSizeSrc(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt, int iPitch, int iStrideHeight);

/*************************************************************************//*!
   \brief Retrieves the minimal pitch value supported by the ip depending
   on the source format
   \param[in] iWidth Frame width in pixel unit
   \param[in] uBitDepth YUV bit-depth
   \param[in] eStorageMode Source Storage Mode
   \return pitch value in bytes
*****************************************************************************/
int AL_EncGetMinPitch(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode);

AL_DEPRECATED("Renamed as AL_EncGetMinPitch, Will be removed in 0.9")
int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode);

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



/*@}*/

