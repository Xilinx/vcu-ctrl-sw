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
   \defgroup Decoder_Settings Settings
   \ingroup Decoder
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/VideoMode.h"
#include "lib_common_dec/DecDpbMode.h"

/*************************************************************************//*!
   \brief Stream's settings
 *************************************************************************/
typedef struct
{
  AL_TDimension tDim; /*!< Stream's dimension (width / height) */
  AL_EChromaMode eChroma; /*!< Stream's chroma mode (400/420/422/444) */
  int iBitDepth; /*!< Stream's bit depth */
  int iLevel; /*!< Stream's level */
  int iProfileIdc; /*!< Stream's profile idc */
  AL_ESequenceMode eSequenceMode; /*!< Stream's sequence mode */
}AL_TStreamSettings;

/*************************************************************************//*!
   \brief Cropping Info on the YUV reconstructed
 *************************************************************************/
typedef struct t_CropInfo
{
  bool bCropping;         /*!< Cropping information present flag    */
  uint32_t uCropOffsetLeft;   /*!< Left   offset of the cropping window */
  uint32_t uCropOffsetRight;  /*!< Rigth  offset of the cropping window */
  uint32_t uCropOffsetTop;    /*!< Top    offset of the cropping window */
  uint32_t uCropOffsetBottom; /*!< Bottom offset of the cropping window */
}AL_TCropInfo;

/*************************************************************************//*!
   \brief Info on stream decoding
 ***************************************************************************/
typedef struct t_InfoDecode
{
  bool bChroma; /*!< Does the current framebuffer hold a chroma component or not (monochrome)*/
  AL_TDimension tDim; /*!< Dimensions of the current framebuffer */
  uint8_t uBitDepthY; /*!< Luma bitdepth of the current framebuffer */
  uint8_t uBitDepthC; /*!< Chroma bitdepth of the current framebuffer */
  AL_TCropInfo tCrop; /*!< Crop information of the current framebuffer */
  AL_EFbStorageMode eFbStorageMode; /*! frame buffer storage mode */
  AL_EPicStruct ePicStruct; /*!< structure (frame/field, top/Bottom) of the current framebuffer */
  uint32_t uCRC; /*!< framebuffer data checksum */
}AL_TInfoDecode;

/*************************************************************************//*!
   \brief Specifies if the framebuffer bound to the crop info requires cropping
   \param[in] pInfo Pointer to the crop info
   \return Returns true if the framebuffer requires cropping. False otherwise.
 ***************************************************************************/
bool AL_NeedsCropping(AL_TCropInfo const* pInfo);

/*************************************************************************//*!
   \brief Returns the minimum number of output buffers required to decode
   the AVC stream in the specified dpb mode
   \param[in] tStreamSettings Settings describing the stream to decode
   \param[in] iStack Number of requests that should be stacked inside the decoder
   at the same time (affects performances)
   \return Returns the minimum number of output buffers required to decode
   the AVC stream in the specified dpb mode
 ***************************************************************************/
int AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack);

/*************************************************************************//*!
   \brief Returns the minimum number of output buffers required to decode
   the HEVC stream in the specified dpb mode
   \param[in] tStreamSettings Settings describing the stream to decode
   \param[in] iStack Number of requests that should be stacked inside the decoder
   at the same time (affects performances)
   \return Returns the minimum number of output buffers required to decode
   the HEVC stream in the specified dpb mode
 ***************************************************************************/
int AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack);

/*@}*/

