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

#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

#include "lib_common/SliceConsts.h"
#include "lib_common_dec/DecDpbMode.h"

/*************************************************************************//*!
   \brief Stream's settings
 *************************************************************************/
typedef struct
{
  AL_TDimension tDim; // !< Stream's dimension (width / height)
  AL_EChromaMode eChroma; // !< Stream's chroma mode (400/420/422/444)
  int iBitDepth; // !< Stream's bit depth
  int iLevel; // !< Stream's level
  int iProfileIdc; // !< Stream's profile idc
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
  bool bChroma;
  AL_TDimension tDim;
  uint8_t uBitDepthY;
  uint8_t uBitDepthC;
  AL_TCropInfo tCrop;
  AL_EFbStorageMode eFbStorageMode;
  uint32_t uCRC;
}AL_TInfoDecode;

/******************************************************************************/
bool AL_NeedsCropping(AL_TCropInfo* pInfo);
/******************************************************************************/
int AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack, AL_EDpbMode eMode);
/******************************************************************************/
int AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack, AL_EDpbMode eMode);

