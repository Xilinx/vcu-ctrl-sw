/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/FourCC.h"
#include "lib_common/OffsetYC.h"
#include "lib_common/Pitches.h"

/*************************************************************************//*!
   \brief Useful information related to the framebuffer containing the picture
*****************************************************************************/
typedef struct AL_t_SrcMetaData
{
  AL_TMetaData tMeta;
  AL_TDimension tDim; /*!< Dimension in pixel of the frame */
  AL_TPitches tPitches; /*!< Luma & chroma pitches size */
  AL_TOffsetYC tOffsetYC; /*!< Luma & chroma offset addresses */
  TFourCC tFourCC; /*!< FOURCC identifier */
}AL_TSrcMetaData;

/*************************************************************************//*!
   \brief Create a source metadata.
   \param[in] tDim Dimension of the the picture (width and height in pixels)
   \param[in] tPitches Luma and chroma pitches in bytes
   \param[in] tOffsetYC Offset to the beginning of the luma and of the chroma in bytes
   \param[in] tFourCC FourCC of the framebuffer
   \return Returns NULL in case of allocation failure. Returns a pointer
   to the metadata in case of success.
*****************************************************************************/
AL_TSrcMetaData* AL_SrcMetaData_Create(AL_TDimension tDim, AL_TPitches tPitches, AL_TOffsetYC tOffsetYC, TFourCC tFourCC);
AL_TSrcMetaData* AL_SrcMetaData_Clone(AL_TSrcMetaData* pMeta);
int AL_SrcMetaData_GetOffsetC(AL_TSrcMetaData* pMeta);

/*************************************************************************//*!
   \brief Get the size of the luma inside the picture
   \param[in] pMeta A pointer the the source metadata
   \return Returns size of the luma region
*****************************************************************************/
int AL_SrcMetaData_GetLumaSize(AL_TSrcMetaData* pMeta);

/*************************************************************************//*!
   \brief Get the size of the chroma inside the picture
   \param[in] pMeta A pointer the the source metadata
   \return Returns size of the chroma region
*****************************************************************************/
int AL_SrcMetaData_GetChromaSize(AL_TSrcMetaData* pMeta);

/*@*/

