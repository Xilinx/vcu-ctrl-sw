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
#include "lib_common/BufCommonInternal.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/FourCC.h"

#define SIZE_PIXEL sizeof(uint16_t)

#define AL_MAX_HEIGHT 8192
#define SIZE_LCU_INFO 16   /*!< LCU compressed size + LCU offset                  */
#define SCD_SIZE 128 /*!< size of start code detector output                */

#define MAX_NAL_UNIT 4096/*!< Maximum nal unit within an access unit supported  */
#define NON_VCL_NAL_SIZE 2048/*!< Init size of the Deanti-emulated buffer used by the software to parse the high level syntax data  */
#define WP_SLICE_SIZE 256

static const int HEVC_LCU_CMP_SIZE[4] =
{
  832, 1088, 1344, 1856
}; /*!< HEVC LCU compressed max size*/
static const int AVC_LCU_CMP_SIZE[4] =
{
  800, 1120, 1408, 2016
}; /*!< AVC  LCU compressed max size*/

static const int POCOL_LIST_OFFSET = 32; // in number of list entry
static const int MVCOL_LIST_OFFSET = 48; // in number of list entry
static const int FBC_LIST_OFFSET = 64; // in number of list entry
static const int REF_LIST_SIZE = 96; // in number of list entry

static const int SCLST_SIZE_DEC = 12288;

/*************************************************************************//*!
   \brief Buffer with Poc list content
*****************************************************************************/
typedef TBuffer TBufferPOC;

static const int POCBUFF_PL_SIZE = 96;  // POC List size
static const int POCBUFF_LONG_TERM_OFFSET = 64;  // Long term flag List

/*************************************************************************//*!
   \brief List of references frame buffer
*****************************************************************************/
typedef struct t_BufferRef
{
  AL_TBuffer RefBuf; // adress of the corresponding frame buffer
  uint8_t uNodeID;
}TBufferRef, TBufferListRef[2][MAX_REF + 1];

/*************************************************************************//*!
   \brief Retrieves the number of LCU in the frame
   \param[in] tDim  Frame dimension (width, height) in pixel
   \param[in] uLCUSize    Max Size of a Coding Unit
   \return number of LCU in the frame
*****************************************************************************/
int AL_GetNumLCU(AL_TDimension tDim, uint8_t uLCUSize);

/*************************************************************************//*!
   \brief Retrieves the size of a HEVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
int AL_GetAllocSize_HevcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode);

/*************************************************************************//*!
   \brief Retrieves the size of a AVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
int AL_GetAllocSize_AvcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode);

/*************************************************************************//*!
   \brief Retrieves the size of the compressed map buffer (LCU Offset & size)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return maximum size (in bytes) needed for the compressed map buffer
*****************************************************************************/
int AL_GetAllocSize_DecCompMap(AL_TDimension tDim);

/*************************************************************************//*!
   \brief Retrieves the size of a HEVC motion vector buffer
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return the size (in bytes) needed for the colocated frame buffer
*****************************************************************************/
int AL_GetAllocSize_HevcMV(AL_TDimension tDim);

/*************************************************************************//*!
   \brief Retrieves the size of a AVC motion vector buffer
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return the size (in bytes) needed for the colocated frame buffer
*****************************************************************************/
int AL_GetAllocSize_AvcMV(AL_TDimension tDim);

/*************************************************************************//*!
   \brief Retrieves the size of the output frame buffer, using the
   minimum pitch value.
   \param[in] tDim Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma mode
   \param[in] uBitDepth Bit Depth
   \param[in] bFrameBufferCompression Specifies if compression is needed
   \param[in] eFrameBufferStorageMode Storage Mode of the frame buffer
   \return the size (in bytes) needed for the output frame buffer
*****************************************************************************/
int AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFrameBufferStorageMode);

/*************************************************************************//*!
   \brief Create the AL_TMetaData associated to the reconstruct buffers
   \param[in] tDim Frame dimension (width, height) in pixel
   \param[in] tFourCC FourCC of the frame buffer
   \param[in] iPitch Pitch of the frame buffer
   \return the AL_TMetaData
*****************************************************************************/
AL_TMetaData* AL_CreateRecBufMetaData(AL_TDimension tDim, int iPitch, TFourCC tFourCC);

/*@}*/

