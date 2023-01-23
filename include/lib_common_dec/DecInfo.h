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
   \defgroup Decoder_Settings Settings
   \ingroup Decoder
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common/VideoMode.h"

/*************************************************************************//*!
   \brief Stream's settings
 *************************************************************************/
typedef struct
{
  AL_TDimension tDim; /*!< Stream's dimension (width / height) */
  AL_EChromaMode eChroma; /*!< Stream's chroma mode (400/420/422/444) */
  int iBitDepth; /*!< Stream's bit depth */
  int iLevel; /*!< Stream's level */
  AL_EProfile eProfile; /*!< Stream's profile */
  AL_ESequenceMode eSequenceMode; /*!< Stream's sequence mode */
  bool bDecodeIntraOnly;  /*!< Should the decoder process only I frames  */
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
  AL_TDimension tDim; /*!< Dimensions of the current framebuffer */
  AL_EChromaMode eChromaMode; /*!< Chroma sub-sampling mode of the current frame*/
  uint8_t uBitDepthY; /*!< Luma bitdepth of the current framebuffer */
  uint8_t uBitDepthC; /*!< Chroma bitdepth of the current framebuffer */
  AL_TCropInfo tCrop; /*!< Crop information of the current framebuffer */
  AL_EFbStorageMode eFbStorageMode; /*! frame buffer storage mode */
  AL_EPicStruct ePicStruct; /*!< structure (frame/field, top/Bottom) of the current framebuffer */
  uint32_t uCRC; /*!< framebuffer data crc */
  AL_TPosition tPos; /*!< Position of the top left decoded pixel */
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
   \param[in] pStreamSettings Settings describing the stream to decode
   \param[in] iStack Number of requests that should be stacked inside the decoder
   at the same time (affects performances)
   \return Returns the minimum number of output buffers required to decode
   the AVC stream in the specified dpb mode
 ***************************************************************************/
int AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int iStack);

/*************************************************************************//*!
   \brief Returns the minimum number of output buffers required to decode
   the HEVC stream in the specified dpb mode
   \param[in] pStreamSettings Settings describing the stream to decode
   \param[in] iStack Number of requests that should be stacked inside the decoder
   at the same time (affects performances)
   \return Returns the minimum number of output buffers required to decode
   the HEVC stream in the specified dpb mode
 ***************************************************************************/
int AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int iStack);

/*@}*/

