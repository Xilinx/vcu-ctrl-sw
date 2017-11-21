/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
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
static const TBufInfo EP2_BUF_QP_CTRL =
{
  1, 48, 0
}; // only 20 bytes used
static const TBufInfo EP2_BUF_SEG_CTRL =
{
  2, 16, 48
};
static const TBufInfo EP2_BUF_QP_BY_MB =
{
  4, 0, 64
}; // no fixed size

/*************************************************************************//*!
   \brief  Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] uMaxCuSize Maximum Size of a Coding Unit
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t GetAllocSizeEP2(AL_TDimension tDim, uint8_t uMaxCuSize);

/*************************************************************************//*!
   \brief Retrieves the size of a Source YUV frame buffer
   \param[in] tDim Frame size in pixels
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] eStorageMode Source Storage Mode
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_EFbStorageMode eStorageMode);

/*************************************************************************//*!
   \brief Retrieves the size of a Source YUV frame buffer
   \param[in] tDim Frame size in pixels
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] eSrcFmt Source format used by the HW IP
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t GetAllocSize_Src(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt);

/*************************************************************************//*!
   \brief Retrieves the Pitch value depending on Source format
   \param[in] iWidth Frame width in pixel unit
   \param[in] uBitDepth YUV bit-depth
   \param[in] eStorageMode Source Storage Mode
   \return pitch value in bytes
*****************************************************************************/
int AL_CalculatePitchValue(int iWidth, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode);

/*************************************************************************//*!
   \brief Retrieves the Source frame buffer storage mode depending on Source mode
   \param[in] eSrcMode Source Mode
   \return Source Storage Mode
*****************************************************************************/
AL_EFbStorageMode AL_GetSrcStorageMode(AL_ESrcMode eSrcMode);


/*@}*/

