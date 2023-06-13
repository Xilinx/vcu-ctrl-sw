// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
   \brief Struct for position
*****************************************************************************/
typedef struct
{
  int32_t iX;
  int32_t iY;
}AL_TPosition;

/*************************************************************************//*!
   \brief Struct for window
*****************************************************************************/
typedef struct
{
  AL_TPosition tPos;
  AL_TDimension tDim;
}AL_TWindow;

/*************************************************************************//*!
   \brief Cropping Info on the YUV reconstructed
 *************************************************************************/
typedef struct t_CropInfo
{
  bool bCropping;         /*!< Cropping information present flag    */
  uint32_t uCropOffsetLeft;   /*!< Left   offset of the cropping window */
  uint32_t uCropOffsetRight;  /*!< Right  offset of the cropping window */
  uint32_t uCropOffsetTop;    /*!< Top    offset of the cropping window */
  uint32_t uCropOffsetBottom; /*!< Bottom offset of the cropping window */
}AL_TCropInfo;

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
  AL_C_ORDER_PACKED,
  AL_C_ORDER_MAX_ENUM, /* sentinel */
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

/*************************************************************************//*!
   \brief Output type
 *************************************************************************/
typedef enum AL_e_OutputType
{
  AL_OUTPUT_PRIMARY,
  AL_OUTPUT_MAIN,
  AL_OUTPUT_POSTPROC,
  AL_OUTPUT_MAX_ENUM,
}AL_EOutputType;

/****************************************************************************/
static inline AL_EChromaOrder GetChromaOrder(AL_EChromaMode eChromaMode)
{
  return eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA :
         (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR);
}

/*@}*/
