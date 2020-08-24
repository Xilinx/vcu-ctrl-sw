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

/**************************************************************************//*!
   \addtogroup PicFormat

   Describes the format of a YUV buffer

   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Struct for dimension
*****************************************************************************/
typedef struct
{
  int32_t iWidth;
  int32_t iHeight;
}AL_TDimension;

/*************************************************************************//*!
   \brief chroma_format_idc
*****************************************************************************/
typedef enum e_ChromaMode
{
  AL_CHROMA_MONO = 0, /*!< Monochrome */
  AL_CHROMA_4_0_0 = AL_CHROMA_MONO, /*!< 4:0:0 = Monochrome */
  AL_CHROMA_4_2_0 = 1, /*!< 4:2:0 chroma sampling */
  AL_CHROMA_4_2_2 = 2, /*!< 4:2:2 chroma sampling */
  AL_CHROMA_4_4_4 = 3, /*!< 4:4:4 chroma sampling */
  AL_CHROMA_MAX_ENUM, /* sentinel */
}AL_EChromaMode;

/*************************************************************************//*!
   \brief Internal frame buffer storage mode
*****************************************************************************/
typedef enum AL_e_FbStorageMode
{
  AL_FB_RASTER = 0,
  AL_FB_TILE_32x4 = 2,
  AL_FB_TILE_64x4 = 3,
  AL_FB_MAX_ENUM, /* sentinel */
}AL_EFbStorageMode;

/*************************************************************************//*!
   \brief Chroma order
*****************************************************************************/
typedef enum e_ChromaOrder
{
  AL_C_ORDER_NO_CHROMA,
  AL_C_ORDER_U_V,
  AL_C_ORDER_V_U,
  AL_C_ORDER_SEMIPLANAR,
  AL_C_ORDER_PACKED
}AL_EChromaOrder;

/*************************************************************************//*!
   \brief Describes the format of a YUV buffer
*****************************************************************************/
typedef struct AL_t_PicFormat
{
  AL_EChromaMode eChromaMode;
  uint8_t uBitDepth;
  AL_EFbStorageMode eStorageMode;
  AL_EChromaOrder eChromaOrder;
  bool bCompressed;
  bool b10bPacked;
}AL_TPicFormat;

/*@}*/
