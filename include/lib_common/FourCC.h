/******************************************************************************
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

/**************************************************************************//*!
   \addtogroup FourCC

   This regroups all the functions that can be used to know how the framebuffer
   is stored in memory

   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/PicFormat.h"

/*************************************************************************//*!
   \brief FOURCC identifier type
*****************************************************************************/
typedef uint32_t TFourCC;

#define FOURCC(A) ((TFourCC)(((uint32_t)((# A)[0])) \
                             | ((uint32_t)((# A)[1]) << 8) \
                             | ((uint32_t)((# A)[2]) << 16) \
                             | ((uint32_t)((# A)[3]) << 24)))

/*************************************************************************//*!
   \brief Returns the ChromaMode identifier according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChomaMode according to the tFourCC parameter
*****************************************************************************/
AL_EChromaMode AL_GetChromaMode(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns the ChromaOrder identifier according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChromaOrder according to the tFourCC parameter
*****************************************************************************/
AL_EChromaOrder AL_GetChromaOrder(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns the bitDepth according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return return the bitDepth according to the tFourCC parameter
*****************************************************************************/
uint8_t AL_GetBitDepth(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns the number of byte per color sample according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return number of bytes per color sample according to the tFourCC parameter
*****************************************************************************/
int AL_GetPixelSize(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns the chroma subsampling according to the tFourCC parameter
   \param[in] tFourCC fourCC format of the current picture
   \param[out] sx subsampling x
   \param[out] sy subsampling y
*****************************************************************************/
void AL_GetSubsampling(TFourCC tFourCC, int* sx, int* sy);

/*************************************************************************//*!
   \brief Returns true if YUV format specified by tFourCC is 10bit packed
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChomaMode according to the tFourCC parameter
*****************************************************************************/
bool AL_Is10bitPacked(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns true if YUV format specified by tFourCC is monochrome
   \param[in] tFourCC FourCC format of the current picture
   \return true if YUV is monochrome according to the tFourCC parameter
*****************************************************************************/
bool AL_IsMonochrome(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns true if YUV format specified by tFourCC is semiplanar
   \param[in] tFourCC FourCC format of the current picture
   \return true if YUV is semiplanar according to the tFourCC parameter
*****************************************************************************/
bool AL_IsSemiPlanar(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns true if YUV format specified by tFourCC is tiled
   \param[in] tFourCC FourCC format of the current picture
   \return true if YUV is tiled according to the tFourCC parameter
*****************************************************************************/
bool AL_IsTiled(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns true if YUV format specified by tFourCC is  non-compressed tile
   \param[in] tFourCC FourCC format of the current picture
   \return true if YUV is non compressed tiled according to the tFourCC parameter
*****************************************************************************/
bool AL_IsNonCompTiled(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns the storage buffer format specified by tFourCC
   \param[in] tFourCC FourCC format of the current picture
   \return the storage buffer format specified by tFourCC
*****************************************************************************/
AL_EFbStorageMode AL_GetStorageMode(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns true if tFourCC specifies that the yuv buffer is compressed
   \param[in] tFourCC FourCC format of the current picture
   \return return true tFourCC indicates buffer compression enabled
*****************************************************************************/
bool AL_IsCompressed(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Returns FourCC from AL_TPicFormat
   \param[in] tPicFormat the pict format
   \return return corresponding FourCC, 0 if picFormat does not exist
*****************************************************************************/
TFourCC AL_GetFourCC(AL_TPicFormat tPicFormat);

/*************************************************************************//*!
   \brief Returns pointer to AL_TPicFormat from FourCC
   \param[in] tFourCC the FourCC
   \param[out] tPicFormat corresponding PicFormat if FourCC exists, untouched otherwise
   \return return true if FourCC exists, false otherwise
*****************************************************************************/
bool AL_GetPicFormat(TFourCC tFourCC, AL_TPicFormat* tPicFormat);

/*@}*/

