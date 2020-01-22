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
   \addtogroup SliceConsts
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "Profiles.h"

/*************************************************************************//*!
   \brief Identifies the slice coding type
*****************************************************************************/
typedef enum e_SliceType
{
  AL_SLICE_SI = 4, /*!< AVC SI Slice */
  AL_SLICE_SP = 3, /*!< AVC SP Slice */
  AL_SLICE_GOLDEN = 3, /*!< Golden Slice */
  AL_SLICE_I = 2,  /*!< I Slice (can contain I blocks) */
  AL_SLICE_P = 1,  /*!< P Slice (can contain I and P blocks) */
  AL_SLICE_B = 0,  /*!< B Slice (can contain I, P and B blocks) */
  AL_SLICE_CONCEAL = 6, /*!< Conceal Slice (slice was concealed) */
  AL_SLICE_SKIP = 7, /*!< Skip Slice */
  AL_SLICE_REPEAT = 8, /*!< VP9 Repeat Slice (repeats the content of its reference) */
  AL_SLICE_MAX_ENUM, /* sentinel */
}AL_ESliceType;

/*************************************************************************//*!
   \brief Indentifies pic_struct (subset of table D-2)
*****************************************************************************/
typedef enum e_PicStruct
{
  AL_PS_FRM = 0,
  AL_PS_TOP_FLD = 1,
  AL_PS_BOT_FLD = 2,
  AL_PS_TOP_BOT = 3,
  AL_PS_BOT_TOP = 4,
  AL_PS_TOP_BOT_TOP = 5,
  AL_PS_BOT_TOP_BOT = 6,
  AL_PS_FRM_x2 = 7,
  AL_PS_FRM_x3 = 8,
  AL_PS_TOP_FLD_WITH_PREV_BOT = 9,
  AL_PS_BOT_FLD_WITH_PREV_TOP = 10,
  AL_PS_TOP_FLD_WITH_NEXT_BOT = 11,
  AL_PS_BOT_FLD_WITH_NEXT_TOP = 12,
  AL_PS_MAX_ENUM, /* sentinel */
}AL_EPicStruct;

/*************************************************************************//*!
   \brief chroma_format_idc
*****************************************************************************/
typedef enum e_ChromaMode
{
  AL_CHROMA_MONO, /*!< Monochrome */
  AL_CHROMA_4_0_0 = AL_CHROMA_MONO, /*!< 4:0:0 = Monochrome */
  AL_CHROMA_4_2_0, /*!< 4:2:0 chroma sampling */
  AL_CHROMA_4_2_2, /*!< 4:2:2 chroma sampling */
  AL_CHROMA_4_4_4, /*!< 4:4:4 chroma sampling : Not supported */
  AL_CHROMA_MAX_ENUM, /* sentinel */
}AL_EChromaMode;

/*************************************************************************//*!
   \brief identifies the entropy coding method
*****************************************************************************/
typedef enum e_EntropyMode
{
  AL_MODE_CAVLC,
  AL_MODE_CABAC,
  AL_MODE_MAX_ENUM, /* sentinel */
}AL_EEntropyMode;

/*************************************************************************//*!
   \brief Weighted Pred Mode
*****************************************************************************/
typedef enum e_WPMode
{
  AL_WP_DEFAULT,
  AL_WP_EXPLICIT,
  AL_WP_IMPLICIT,
  AL_WP_MAX_ENUM, /* sentinel */
}AL_EWPMode;

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
   \brief Decoded buffer output mode
*****************************************************************************/
typedef enum AL_e_BufferOutputMode
{
  AL_OUTPUT_INTERNAL, // Output reconstructed buffers stored as in the encoder
  AL_OUTPUT_MAX_ENUM, /* sentinel */
}AL_EBufferOutputMode;

/*************************************************************************//*!
   \brief Struct for dimension
*****************************************************************************/
typedef struct
{
  int32_t iWidth;
  int32_t iHeight;
}AL_TDimension;

/*************************************************************************//*!
   \brief Struct for offsets
*****************************************************************************/
typedef struct
{
  int32_t iX;
  int32_t iY;
}AL_TOffset;

#define AL_MAX_SLICES_SUBFRAME 32

/*************************************************************************//*!
   \brief Filler Data Control Mode
*****************************************************************************/
typedef enum e_FillerCtrlMode
{
  AL_FILLER_DISABLE,
  AL_FILLER_ENC, /*!< 0xFF data filled by encoder it-self */
  AL_FILLER_APP, /*!< 0xFF data filled by the application layer */
}AL_EFillerCtrlMode;

/*@}*/

