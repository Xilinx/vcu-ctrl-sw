// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \defgroup Decoder Decoder

   The diagram below shows the usual usage of the decoder control software API
   \htmlonly
    <br/><object data="Decoder.svg"/>
   \endhtmlonly

   \defgroup Decoder_API API
   \ingroup Decoder

   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"

#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"
#include "lib_common/PicFormat.h"

#include "lib_common_dec/DecCallbacks.h"
#include "lib_common_dec/DecoderArch.h"
#include "lib_decode/DecSettings.h"
#include "lib_decode/I_DecSchedulerInfo.h"

/*****************************************************************************
    \brief Virtual interface used to access the scheduler of the Decoder IP.
    If you want to create multiple channels in the same process that access the same IP, the AL_IDecScheduler should be shared between them.
    \see AL_DecSchedulerCpu_Create and AL_DecSchedulerMcu_Create if available to get concrete implementations of this interface.
*****************************************************************************/
typedef struct AL_IDecScheduler AL_IDecScheduler;

/*****************************************************************************
   \brief De-initializes the scheduler
*****************************************************************************/
void AL_IDecScheduler_Destroy(AL_IDecScheduler* pThis);

/*****************************************************************************
   \brief Scheduler getter
*****************************************************************************/
void AL_IDecScheduler_Get(AL_IDecScheduler const* pThis, AL_EIDecSchedulerInfo eInfo, void* pParam);

/*****************************************************************************
   \brief Scheduler setter
*****************************************************************************/
void AL_IDecScheduler_Set(AL_IDecScheduler* pThis, AL_EIDecSchedulerInfo eInfo, void const* pParam);

typedef enum AL_EDecHandleState
{
  AL_DEC_HANDLE_STATE_PROCESSING,
  AL_DEC_HANDLE_STATE_PROCESSED,
  AL_DEC_HANDLE_STATE_CONCEALING,
  AL_DEC_HANDLE_STATE_CONCEALED,
  AL_DEC_HANDLE_STATE_MAX_ENUM, /* sentinel */
}AL_EDecHandleState;

typedef struct AL_TDecMetaHandle
{
  AL_EDecHandleState eState;
  AL_TBuffer* pHandle;
}AL_TDecMetaHandle;

/*****************************************************************************
   \brief Flags characterizing stream buffers pushed for decoding
*****************************************************************************/
typedef enum AL_EStreamBufFlags
{
  AL_STREAM_BUF_FLAG_UNKNOWN = 0x0, /*!< Stream buffer content is unknown */
  AL_STREAM_BUF_FLAG_ENDOFSLICE = 0x2, /*!< The stream buffer ends a slice */
  AL_STREAM_BUF_FLAG_ENDOFFRAME = 0x4 /*!< The stream buffer ends a frame */
}AL_EStreamBufFlags;

/*****************************************************************************
   \brief Handle to the decoder object.
   \see AL_Decoder_Create
   \see AL_Decoder_Destroy
*****************************************************************************/
typedef AL_HANDLE AL_HDecoder;

/*****************************************************************************
   \brief Decoder callbacks
*****************************************************************************/
typedef struct AL_TDecCallBacks
{
  AL_CB_EndParsing endParsingCB; /*!< Called when an input buffer is parsed */
  AL_CB_EndDecoding endDecodingCB; /*!< Called when a frame is decoded */
  AL_CB_Display displayCB; /*!< Called when a buffer is ready to be displayed */
  AL_CB_ResolutionFound resolutionFoundCB; /*!< Called when a resolution change occurs */
  AL_CB_ParsedSei parsedSeiCB; /*!< Called when a SEI is parsed */
  AL_CB_Error errorCB; /*!< Called when an error is encountered */
}AL_TDecCallBacks;

/*****************************************************************************
   \brief Initialize decoder library
   \param[in] eArch  decoder library arch to use
   \return error code specifying why library initialization has failed
*****************************************************************************/
AL_ERR AL_Lib_Decoder_Init(AL_ELibDecoderArch eArch);

/*****************************************************************************
   \brief Deinitialize decoder library
*****************************************************************************/
void AL_Lib_Decoder_DeInit(void);

/*****************************************************************************
   \brief Creates a new instance of the Decoder
   \param[out] hDec           handle to the created decoder
   \param[in] pScheduler      Pointer to a dec scheduler structure.
   \param[in] pAllocator      Pointer to an allocator handle
   \param[in] pSettings       Pointer to the decoder settings
   \param[in] pCB             Pointer to the decoder callbacks
   \return error code specifying why this decoder couldn't be created
   \see AL_Decoder_Destroy
*****************************************************************************/
AL_ERR AL_Decoder_Create(AL_HDecoder* hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*****************************************************************************
   \brief Releases all allocated and/or owned resources
   \param[in] hDec Handle to Decoder object previously created with AL_Decoder_Create
   \see AL_Decoder_Create
*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec);

/****************************************************************************/
/* internal. Used for traces */
void AL_Decoder_SetParam(AL_HDecoder hDec, const char* sPrefix, int32_t iFrmID, int32_t iNumFrm, bool bShouldPrintFrameDelimiter, bool bOutputTraceOnly);

/*****************************************************************************
   \brief Pushes a buffer to the decoder queue. It will be decoded when possible
   \param[in] hDec Handle to a decoder object.
   \param[in] pBuf Pointer to the encoded bitstream buffer
   \param[in] uSize Size in bytes of actual data in pBuf
   \param[in] uFlags Flags characterizing the stream buffer
   \return return true if buffer has been successfully pushed, false otherwise
*****************************************************************************/
bool AL_Decoder_PushStreamBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags);

/*****************************************************************************
   \brief Flushes the decoding request stack when the stream parsing is finished.
   \param[in]  hDec Handle to a decoder object.
*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec);

/*****************************************************************************
   \brief Puts the display buffer inside the decoder internal bufpool
   It is used to give the decoder buffers where it will output the decoded pictures
   \param[in] hDec   Handle to a decoder object.
   \param[in] pDisplay Pointer to the decoded picture buffer
   \return return true if buffer has been successfully added, false otherwise
*****************************************************************************/
bool AL_Decoder_PutDisplayPicture(AL_HDecoder hDec, AL_TBuffer* pDisplay);

/*****************************************************************************
   \brief Sets the decoder output settings to be applied on the reconstructed yuv
   \param[in] hDec   Handle to a decoder object.
   \param[in] pDecOutputSettings Pointer to the output settings
   \return return true if output settings are successfully added or already added,
    false otherwise
*****************************************************************************/
bool AL_Decoder_ConfigureOutputSettings(AL_HDecoder hDec, AL_TDecOutputSettings const* pDecOutputSettings);

/*****************************************************************************
   \brief Retrieves the codec of the specified decoder instance
   \param[in] hDec   Handle to a decoder object.
*****************************************************************************/
AL_ECodec AL_Decoder_GetCodec(AL_HDecoder hDec);

/*****************************************************************************
   \brief Retrieves the maximum bitdepth allowed by the stream profile
   \param[in] hDec Handle to a decoder object.
   \return return the maximum bidepth allowed byu the current stream profile
*****************************************************************************/
int32_t AL_Decoder_GetMaxBD(AL_HDecoder hDec);

/*****************************************************************************
   \brief Retrieves the last decoder error state
   \param[in]  hDec  Handle to a decoder object.
   \return return the current error status
*****************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec);

/*****************************************************************************
   \brief Retrieves the error status related to a specific frame
   \param[in]  hDec  Handle to a decoder object.
   \param[in]  pBuf  Pointer to the decoded picture buffer for which to get the error status
   \return return the frame error status
*****************************************************************************/
AL_ERR AL_Decoder_GetFrameError(AL_HDecoder hDec, AL_TBuffer const* pBuf);

/*****************************************************************************
   \brief Preallocates internal buffers.
   This is only usable if Stream settings are set. Calling this function without
   the stream settings will assert.
   Internal buffers of the decoder are normally allocated after determining
   what kind of stream is being decoded. If you know the stream settings
   in advance, you can override this behavior by giving these information at
   the decoder creation and calling AL_Decoder_PreallocateBuffers.
   Giving wrong information about the stream will lead to undefined behaviour.
   Calling this function will reduce the memory footprint of the decoder and
   will remove the latency of the allocation of the internal buffer at runtime.
   \warning This is a blocking function waiting for resolutionFoundCB to end
   \param[in]  hDec  Handle to a decoder object.
   \return return false if the allocation failed. True otherwise
*****************************************************************************/
bool AL_Decoder_PreallocateBuffers(AL_HDecoder hDec);

/*****************************************************************************
   \brief Give the minimum pitch (aka stride) supported by the decoder for its decoded buffers
   \param[in] iWidth width of the reconstructed buffers in pixels
   \param[in] pPicFormat picture format of the buffer
   \return Minimum pitch required
*****************************************************************************/
int32_t AL_Decoder_GetMinPitch(int32_t iWidth, AL_TPicFormat const* pPicFormat);

/*****************************************************************************
   \brief Give the minimum stride height supported by the decoder for its decoded buffers
   Restriction: The decoder still only supports a stride height set to AL_Decoder_GetMinStrideHeight.
   all other strideHeight will be ignored
   \param[in] iHeight height of the picture in pixels
   \param[in] pPicFormat picture format of the buffer
   \return Minimum stride height required
*****************************************************************************/
int32_t AL_Decoder_GetMinStrideHeight(int32_t iHeight, AL_TPicFormat const* pPicFormat);

/*!@}*/
