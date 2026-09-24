// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufCommonInternal.h"
#include "lib_common/BufConst.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Index.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/DecOutputSettings.h"

#define SIZE_PIXEL sizeof(uint16_t)

// Limitation on old decoder ips
#define AL_MAX_HEIGHT 8192
#define SIZE_LCU_INFO 16   /*!< LCU compressed size + LCU offset                  */
#define ITU_JPEG_SCD_SIZE (16 * sizeof(AL_TStartCode)) /*!< size of start code detector output                */
#define MAX_NAL_UNIT 4096/*!< Maximum nal unit within an access unit supported  */
#define NON_VCL_NAL_SIZE 5248 /*!< Init size of the Deanti-emulated buffer used by the software to parse the high level syntax data  */

#define WP_ONE_SET_SIZE 8
#define WP_SLICE_SIZE (16 * 2 * WP_ONE_SET_SIZE) // WP coefficient per slice (num_ref_idx * number_of_list * 16 bytes = 16 * 2 * 16)

static const int32_t HEVC_LCU_CMP_SIZE[4] =
{
  832, 1088, 1344, 1856
}; /*!< HEVC LCU compressed max size*/
static const int32_t AVC_LCU_CMP_SIZE[4] =
{
  800, 1120, 1408, 2016
}; /*!< AVC  LCU compressed max size*/

#define MAX_NB_REF_PLANE 2

static const int32_t SCLST_SIZE_DEC = 1024;

/*****************************************************************************
   \brief Buffer with Poc list content
*****************************************************************************/
typedef TBuffer TBufferPOC;

static const int32_t POCBUFF_PL_SIZE = 96;  // POC List size
static const int32_t POCBUFF_LONG_TERM_OFFSET = 64;  // Long term flag List
static const int32_t POCBUFF_AVAILABLE_REF_OFFSET = 66;  // Available Ref flag List
static const int32_t POCBUFF_SUBPIC_OFFSET = 68;  // Frame with subpicture flag List

/*****************************************************************************
   \brief List of references frame buffer
*****************************************************************************/
typedef struct TBufferRef
{
  AL_TIndex tNodeID;
}TBufferRef, TBufferListRef[2][AL_MAX_REF + 1];

/*****************************************************************************
   \brief Offsets to the data in the reference frame list
*****************************************************************************/
typedef struct TRefListOffsets
{
  uint32_t uColocPocOffset;
  uint32_t uColocMVOffset;
  uint32_t uMapOffset;
  uint32_t uDimensionOffset;
}TRefListOffsets;

/*****************************************************************************
   \brief Retrieves the size of a HEVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
int32_t AL_GetAllocSize_HevcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode);

/*****************************************************************************
   \brief Retrieves the size of a AVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
int32_t AL_GetAllocSize_AvcCompData(AL_TDimension tDim, AL_EChromaMode eChromaMode);

/*****************************************************************************
   \brief Retrieves the size of the compressed map buffer (LCU Offset & size)
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return maximum size (in bytes) needed for the compressed map buffer
*****************************************************************************/
int32_t AL_GetAllocSize_DecCompMap(AL_TDimension tDim);

/*****************************************************************************
   \brief Retrieves the size of a HEVC motion vector buffer
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return the size (in bytes) needed for the collocated frame buffer
*****************************************************************************/
int32_t AL_GetAllocSize_HevcMV(AL_TDimension tDim);

/*****************************************************************************
   \brief Retrieves the size of a AVC motion vector buffer
   \param[in] tDim  Frame dimension (width, height) in pixel
   \return the size (in bytes) needed for the collocated frame buffer
*****************************************************************************/
int32_t AL_GetAllocSize_AvcMV(AL_TDimension tDim);

/*****************************************************************************
   \brief Retrieves the size of the output frame buffer, using the
   minimum pitch value.
   \param[in] tDim Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma mode
   \param[in] uBitDepth Bit Depth
   \param[in] bFrameBufferCompression Specifies if compression is needed
   \param[in] eFrameBufferStorageMode Storage Mode of the frame buffer
   \return the size (in bytes) needed for the output frame buffer
*****************************************************************************/
int32_t AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFrameBufferStorageMode);

uint32_t AL_GetRefListOffsets(TRefListOffsets* pOffsets, AL_ECodec eCodec, AL_EChromaMode eChromaMode, uint8_t uMaxRef, uint8_t uAddrSizeInBytes);

int32_t AL_DecGetLumaPixPlanePitch(int32_t iWidth, AL_TPicFormat const* pPicFormat);
int32_t AL_DecGetPixPlaneHeight(int32_t iHeight, AL_TPicFormat const* pPicFormat);

/*****************************************************************************
   \brief  Frame Buffer parameters that are used to create a AL_TPixMapBuffer
*****************************************************************************/
typedef struct AL_TDecFrameBufferParams
{
  AL_TDimension tDim;
  AL_TPicFormat tPicFormat;
}AL_TDecFrameBufferParams;

/*****************************************************************************
   \brief Creates A PixMapBuffer with the given parameters

   \param[in] pFrameBufferParams Parameters to allocate the buffer
   \return The allocated buffer
*****************************************************************************/
AL_TBuffer* AL_DecCreateFrameBuffer(AL_TDecFrameBufferParams const* pFrameBufferParams, AL_TAllocator* pAllocator, PFN_RefCount_CallBack pRefCountCallback, void* pUserParam);
