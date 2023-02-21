// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include <vector>
extern "C"
{
#include "lib_common/FourCC.h"
}

/*************************************************************************//*!
   \brief Chroma sampling format for video source file
*****************************************************************************/
typedef enum e_FileFormat
{
  FILE_MONOCHROME, /*!< YUV file is monochrome and contains only luma samples*/
  FILE_YUV_4_2_0,  /*!< YUV file contains 4:2:0 chroma samples and is stored
                      in planar IYUV (also called I420) format */
}EFileFormat;

/*************************************************************************//*!
   \brief YUV File size and format information
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct t_YUVFileInfo
{
  int PictWidth;  /*!< Frame width in pixels */
  int PictHeight; /*!< Frame height in pixels */

  TFourCC FourCC; /*!< FOURCC identifying the file format */

  unsigned int FrameRate;  /*!< Frame by second */
}TYUVFileInfo;

/*@}*/
