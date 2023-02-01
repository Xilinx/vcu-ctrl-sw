/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#pragma once
#include "EncEPBuffer.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/Lambdas.h"
#define AL_MAX_LAWINDOWSIZE 0
#define ENC_MAX_CMD (AL_MAX_NUM_B_PICT + 3 + AL_MAX_LAWINDOWSIZE)

static const AL_TBufInfo EP1_BUF_SCL_LST =
{
  16, 25600, 256
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
uint32_t AL_GetAllocSizeEP1(AL_ECodec eCodec);

/*************************************************************************//*!
   \brief Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uQpLCUGranularity QP block granularity in LCU unit
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_GetAllocSizeFlexibleEP2(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, uint8_t uQpLCUGranularity);

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
   \param[in] uLCUSize Max size of a coding unit
   \param[in] eChromaMode Chroma Mode
   \param[in] eOptions Encoding option flags
   \param[in] uMVVRange extra buffer lines used for reconstructed buffering
   \return maximum size (in bytes) needed for the YUV frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_EncReference(AL_TDimension tDim, uint8_t uBitDepth, uint8_t uLCUSize, AL_EChromaMode eChromaMode, AL_EChEncOption eOptions, uint16_t uMVVRange);

/*************************************************************************//*!
   \brief Retrieves the size of a compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim Frame dimensions
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uBitDepth YUV bit-depth
   \param[in] eChromaMode Chroma Mode
   \param[in] bUseEnt Do we use entropy compression
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_CompData(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt);

/*************************************************************************//*!
   \brief Retrieves the offset of the current LCU Hdr_MVDs words
   \param[in] tDim Frame dimensions
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uNumCore number of core used by the channel
   \param[in] bUseEnt Do we use entropy compression
   \return maximum size (in bytes) needed for the LCU Info buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_EncCompMap(AL_TDimension tDim, uint8_t uLog2MaxCuSize, uint8_t uNumCore, bool bUseEnt);

/*************************************************************************//*!
   \brief Retrieves the size of a colocated frame buffer
   \param[in] tDim Frame dimensions
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] Codec Flag which specifies the codec used
   \return the size (in bytes) needed for the colocated frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_MV(AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec Codec);

/*************************************************************************//*!
   \brief Retrieves the size of an entry_points size buffer
   \param[in] iLCUPicHeight Frame Height in pixel
   \param[in] iNumSlices Number of slices within the frame
   \param[in] uNumCore Number of used core
   \return the size (in bytes) needed for the entry_points size buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_WPP(int iLCUPicHeight, int iNumSlices, uint8_t uNumCore);

uint32_t AL_GetAllocSize_SliceSize(uint32_t uWidth, uint32_t uHeight, uint32_t uNumSlices, uint32_t uLog2MaxCuSize);

/*************************************************************************//*!
   \brief Retrieves the size of a stream part size buffer
   \param[in] eProfile Encoding profile
   \param[in] iNumCores Number of cores used to encode the frame
   \param[in] iNumSlices Number of slices within the frame
   \param[in] bSliceSize True if frame is encode in slice size mode
   \return the size (in bytes) needed for the entry_points size buffer
*****************************************************************************/
uint32_t GetAllocSize_StreamPart(AL_EProfile eProfile, int iNumCores, int iNumSlices, bool bSliceSize);

/*************************************************************************//*!
   \brief Retrieves plane description of the reference buffer
   \param[in] pPlaneDesc Plane description to fill
   \param[in] tDim Frame dimensions
   \param[in] eChromaMode Chroma Mode
   \param[in] uBitDepth YUV bit-depth
   \param[in] bIsAvc AVC Codec
   \param[in] uLCUSize Max size of a coding unit
   \param[in] uMVVRange extra buffer lines used for reconstructed buffering
   \param[in] eOptions Encoding option flags
*****************************************************************************/
void AL_FillPlaneDesc_EncReference(AL_TPlaneDescription* pPlaneDesc, AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsAvc, uint8_t uLCUSize, uint16_t uMVVRange, AL_EChEncOption eOptions);

