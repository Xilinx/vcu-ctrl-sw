/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
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
