// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_decode/lib_decode.h"
#include "DecoderCommon.h"
#include "lib_common_dec/InternalError.h"
#include "lib_common_dec/DecInfo.h"
#include "lib_common/Index.h"
#include "lib_rtos/types.h"

typedef struct
{
  AL_TDecCtx ctx;
}AL_TDecoder;

AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);
AL_ERR AL_CreateSubLayerDecoder(AL_TDecCtx* pBaseLayerCtx, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*****************************************************************************
   \brief This function performs the decoding of one unit
   \param[in] pAbsDec decoder handle
   \param[in] pBufStream buffer containing input bitstream to decode
   \return if the function succeeds the return value is ERR_UNIT_NONE
*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_TryDecodeOneUnit(AL_TDecoder* pAbsDec, AL_TBuffer* pBufStream);

UNIT_ERROR AL_Default_Decoder_DecodeOneUnit(AL_TDecoder* pAbsDec, AL_TCircBuffer* pStream, AL_TNal* pNals, int32_t iNalCount, int32_t iLastVclNalInAU);

/*****************************************************************************
   \brief This function signal that a buffer as been fully parsed
   \param[in] pUserParam filled with the decoder context
   \param[in] tFrameID frame id for the picture manager
   \param[in] tParsingID stream input id in the split input case.
*****************************************************************************/
void AL_Default_Decoder_EndParsing(void* pUserParam, AL_TIndex tFrameID, AL_TIndex tParsingID);

/*****************************************************************************
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current frame decoded status
*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus const* pStatus);

/*****************************************************************************
   \brief This function signal that a buffer as been fully parsed and force
          its release. Function meant to be used for buffers that haven't
          been freed through a endparsing nor enddecoding call.
   \param[in] pUserParam filled with the decoder context
   \param[in] pBufStream buffer containing input bitstream to decode
*****************************************************************************/
void AL_Default_Decoder_ReleaseStreamBuffer(void* pUserParam, AL_TBuffer* pBufStream);

/*****************************************************************************
   \brief This function sets an error
   \param[in] pCtx decoder context
   \param[in] eError Error to set
   \param[in] iFrameID Id of the erroneous frame, -1 if error is not frame-related
   \param[in] bTriggerCB Specifies if we must trigger the error CB
*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError, int32_t iFrameID, bool bTriggerCB);

/*****************************************************************************
   \brief This function indicates the storage mode of displayed reconstructed frames
   \param[in] pCtx decoder context
   \param[in] iBitDepth stream bitdepth
   \param[out] pEnableCompression indicates if FBC in enabled for output frames
   \return the output storage mode
*****************************************************************************/
AL_EFbStorageMode AL_Default_Decoder_GetDisplayStorageMode(AL_TDecCtx const* pCtx, int32_t iBitDepth, bool* pEnableCompression);

/*****************************************************************************
   \brief This function indicates if there has an ongoing frame
   \param[in] pCtx decoder context
*****************************************************************************/
bool AL_Default_Decoder_HasOngoingFrame(AL_TDecCtx* pCtx);

bool AL_Default_Decoder_CreateChannel(AL_TDecCtx* pCtx, void (* pfnEndParsing)(void*, AL_TIndex, AL_TIndex), void (* pfnEndDecoding)(void*, AL_TDecPicStatus const*));
void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec);
void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, const char* sPrefix, int32_t iFrmID, int32_t iNumFrm, bool bShouldPrintFrameDelimiter, bool bOutputTraceOnly);
bool AL_Default_Decoder_PushStreamBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags);
bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize);
void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec);
void AL_Default_Decoder_ForceStop(AL_TDecoder* pAbsDec);
bool AL_Default_Decoder_ConfigureOutputSettings(AL_TDecoder* pAbsDec, AL_TDecOutputSettings const* pDecOutputSettings);
bool AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, uint8_t uLayerID, AL_TBuffer* pDecPict);
int32_t AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec);
AL_ERR AL_Default_Decoder_GetLastError(AL_TDecoder* pAbsDec);
AL_ERR AL_Default_Decoder_GetFrameError(AL_TDecoder* pAbsDec, AL_TBuffer const* pBuf);
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec);

int32_t AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec);
int32_t AL_Default_Decoder_SkipParsedNals(AL_TDecoder* pAbsDec);
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec);
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec);

/*!@}*/
