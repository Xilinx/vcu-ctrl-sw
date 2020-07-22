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

#pragma once
#include "lib_common_enc/EncBuffers.h"
#include "lib_common/BufCommonInternal.h"
#include "EncEPBuffer.h"
#include "lib_common_enc/Lambdas.h"
#define AL_MAX_LAWINDOWSIZE 0
#define ENC_MAX_CMD (AL_MAX_NUM_B_PICT + 3 + AL_MAX_LAWINDOWSIZE)

/*************************************************************************//*!
   \brief Retrieves the maximum size of one NAL unit
   \param[in] uWidth Frame Width in pixel
   \param[in] uHeight Frame Height in pixel
   \param[in] uMaxCuSize  Maximum Size of a Coding Unit
   \return maximum size of one NAL unit
*****************************************************************************/
uint32_t GetMaxLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uMaxCuSize);

static const AL_TBufInfo EP1_BUF_SCL_LST =
{
  16, 25344, 256
};

/* see ChooseLda.h for EP1_BUF_LAMBDAS */
// Encoder Parameter Buf 1 Flag,  Size, Offset

static const AL_TBufInfo EP3_BUF_RC_TABLE1 =
{
  1, 512, 0
};
static const AL_TBufInfo EP3_BUF_RC_TABLE2 =
{
  2, 512, 512
};
static const AL_TBufInfo EP3_BUF_RC_CTX =
{
  4, 4096, 1024
}; // no fixed size with max = 4096
static const AL_TBufInfo EP3_BUF_RC_LVL =
{
  8, 32, 5120
};

/*************************************************************************//*!
   \brief  Retrieves the size of a QP parameters buffer 1 (Lda + SclMtx)
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeEP1(void);

/*************************************************************************//*!
   \brief  Retrieves the size of a Encoder parameters buffer 3 (HW RateCtrl) for one core
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeEP3PerCore(void);

/*************************************************************************//*!
   \brief  Retrieves the size of a Encoder parameters buffer 3 (HW RateCtrl)
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeEP3(void);

static const size_t MVBUFF_PL_OFFSET[2] =
{
  0, 64
}; // POC List
static const size_t MVBUFF_LONG_TERM_OFFSET = 128; // Long term flag List
static const size_t MVBUFF_USED_POC = 132; // Used POCs
static const size_t MVBUFF_MV_OFFSET = 256; // Motion Vectors

/*************************************************************************//*!
   \brief Retrieves the size of a Reference YUV frame buffer
   \param[in] tDim Frame dimensions
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] eEncOption Encoding option flags
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bComp);

/*************************************************************************//*!
   \brief Retrieves the size of a compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim Frame dimensions
   \param[in] uLCUSize Max size of a coding unit
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] bUseEnt Do we use entropy compression
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_CompData(AL_TDimension tDim, uint8_t uLcuSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt);

/*************************************************************************//*!
   \brief Retrieves the offset of the current LCU Hdr_MVDs words
   \param[in] tDim Frame dimensions
   \param[in] uLCUSize Max size of a coding unit
   \param[in] uNumCore number of core used by the channel
   \param[in] bUseEnt Do we use entropy compression
   \return maximum size (in bytes) needed for the LCU Info buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_EncCompMap(AL_TDimension tDim, uint8_t uLcuSize, uint8_t uNumCore, bool bUseEnt);

/*************************************************************************//*!
   \brief Retrieves the size of a colocated frame buffer
   \param[in] tDim Frame dimensions
   \param[in] uLCUSize Max size of a coding unit
   \param[in] Codec Flag which specifies the codec used
   \return the size (in bytes) needed for the colocated frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_MV(AL_TDimension tDim, uint8_t uLcuSize, AL_ECodec Codec);

/*************************************************************************//*!
   \brief Retrieves the size of an entry_points size buffer
   \param[in] iLCUHeight Frame Height in pixel
   \param[in] iNumSlices Number of slices within the frame
   \param[in] uNumCore Number of used core
   \return the size (in bytes) needed for the entry_points size buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_WPP(int iLCUHeight, int iNumSlices, uint8_t uNumCore);

uint32_t AL_GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uMaxCuSize);

/*************************************************************************//*!
   \brief Retrieves the size of a stream part size buffer
   \param[in] iLCUHeight Frame Height in pixel
   \param[in] iNumSlices Number of slices within the frame
   \param[in] iSliceSize Number of byte per slice or zero if not used
   \return the size (in bytes) needed for the entry_points size buffer
*****************************************************************************/
uint32_t GetAllocSize_StreamPart(int iLCUHeight, int iNumSlices, int iSliceSize);

