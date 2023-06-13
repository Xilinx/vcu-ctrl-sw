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

#include "lib_common/BufCommonInternal.h"
#include "lib_common/BufferAPI.h"
#include "lib_common_dec/DecBuffers.h"

#define SIZE_PIXEL sizeof(uint16_t)

// Limitation on old decoder ips
#define AL_MAX_HEIGHT 8192
#define SIZE_LCU_INFO 16   /*!< LCU compressed size + LCU offset                  */
#define SCD_SIZE 128 /*!< size of start code detector output                */

#define MAX_NAL_UNIT 4096/*!< Maximum nal unit within an access unit supported  */
#define NON_VCL_NAL_SIZE 5248 /*!< Init size of the Deanti-emulated buffer used by the software to parse the high level syntax data  */

#define WP_ONE_SET_SIZE 8
#define WP_SLICE_SIZE 16 * 2 * WP_ONE_SET_SIZE // WP coefficient per slice (num_ref_idx * number_of_list * 16 bytes = 16 * 2 * 16)

static const int HEVC_LCU_CMP_SIZE[4] =
{
  832, 1088, 1344, 1856
}; /*!< HEVC LCU compressed max size*/
static const int AVC_LCU_CMP_SIZE[4] =
{
  800, 1120, 1408, 2016
}; /*!< AVC  LCU compressed max size*/

#define MAX_NB_REF_PLANE 2

static const int SCLST_SIZE_DEC = 12288;

/*************************************************************************//*!
   \brief Buffer with Poc list content
*****************************************************************************/
typedef TBuffer TBufferPOC;

static const int POCBUFF_PL_SIZE = 96;  // POC List size
static const int POCBUFF_LONG_TERM_OFFSET = 64;  // Long term flag List
static const int POCBUFF_SUBPIC_OFFSET = 68;  // Frame with subpicture flag List

/*************************************************************************//*!
   \brief List of references frame buffer
*****************************************************************************/
typedef struct t_BufferRef
{
  AL_TBuffer RefBuf; // address of the corresponding frame buffer
  uint8_t uNodeID;
}TBufferRef, TBufferListRef[2][MAX_REF + 1];

/*************************************************************************//*!
   \brief Offsets to the data in the reference frame list
*****************************************************************************/
typedef struct t_RefListOffsets
{
  uint32_t uColocPocOffset;
  uint32_t uColocMVOffset;
  uint32_t uMapOffset;
}TRefListOffsets;

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
   \return the size (in bytes) needed for the collocated frame buffer
*****************************************************************************/
int AL_GetAllocSize_HevcMV(AL_TDimension tDim);

/*************************************************************************//*!
   \brief Retrieves the size of a AVC motion vector buffer
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return the size (in bytes) needed for the collocated frame buffer
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
   \brief Get offsets to the different data of the reference list buffer
   \param[out] pOffset the data offsets in the reference list buffer
   \param[in] eCodec Current codec
   \param[in] eChromaOrder Chroma order
   \param[in] uAddrSizeInBytes References list's buffer address size in bytes.
   \return the total size of the RefListBuffer
*****************************************************************************/
uint32_t AL_GetRefListOffsets(TRefListOffsets* pOffsets, AL_ECodec eCodec, AL_EChromaOrder eChromaOrder, uint8_t uAddrSizeInBytes);

int32_t RndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);
int32_t RndHeight(int32_t iHeight);

/*@}*/
