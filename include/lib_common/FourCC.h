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

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"

/*************************************************************************//*!
   \brief FOURCC identifer type
*****************************************************************************/
typedef uint32_t TFourCC;

#define FOURCC(A) ((TFourCC)(((uint32_t)((# A)[0])) \
                             | ((uint32_t)((# A)[1]) << 8) \
                             | ((uint32_t)((# A)[2]) << 16) \
                             | ((uint32_t)((# A)[3]) << 24)))

/***************************************************************************/

typedef struct AL_t_PicFormat
{
  AL_EChromaMode eChromaMode;
  uint8_t uBitDepth;
  AL_EFbStorageMode eStorageMode;
}AL_TPicFormat;

/*************************************************************************//*!
   \brief returns the ChromaMode identifier according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChomaMode according to the tFourCC parameter
*****************************************************************************/
AL_EChromaMode AL_GetChromaMode(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns the bitDepth according to the tFourCC parameter
   \param[in] tFourCC FourCC format of the current picture
   \return return the bitDepth according to the tFourCC parameter
*****************************************************************************/
uint8_t AL_GetBitDepth(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns the chroma subsampling according to the tFourCC parameter
   \param[in] tFourCC fourCC format of the current picture
   \param[out] sx subsampling x
   \param[out] sy subsampling y
*****************************************************************************/
void AL_GetSubsampling(TFourCC tFourCC, int* sx, int* sy);

/*************************************************************************//*!
   \brief returns true if YUV format specified by tFourCC is 10bit packed
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChomaMode according to the tFourCC parameter
*****************************************************************************/
bool AL_Is10bitPacked(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns true if YUV format specified by tFourCC is semiplanar
   \param[in] tFourCC FourCC format of the current picture
   \return return the ChomaMode according to the tFourCC parameter
*****************************************************************************/
bool AL_IsSemiPlanar(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns true if YUV format specified by tFourCC is tiled
   \param[in] tFourCC FourCC format of the current picture
   \return true if YUV is tiled according to the tFourCC parameter
*****************************************************************************/
bool AL_IsTiled(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns the storage buffer format specified by tFourCC
   \param[in] tFourCC FourCC format of the current picture
   \return the storage buffer format specified by tFourCC
*****************************************************************************/
AL_EFbStorageMode GetStorageMode(TFourCC tFourCC);

/*************************************************************************//*!
   \brief returns the FOURCC identifier used as native YUV source format
   according to the encoding ChromaMode and bitdepth
   \param[in] picFmt source picture format
   \return return the corresponding TFourCC format
*****************************************************************************/
TFourCC AL_GetSrcFourCC(AL_TPicFormat const picFmt);

/*@}*/

