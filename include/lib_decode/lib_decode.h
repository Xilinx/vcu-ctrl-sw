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

/**************************************************************************//*!
   \defgroup Decoder

   The diagram below shows the usual usage of the encoder control software API
   \htmlonly
    <br/><object data="Decoder.svg"/>
   \endhtmlonly

   \defgroup Decoder_API API
   \ingroup Decoder

   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"

#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"
#include "lib_common/Planes.h"

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecCallbacks.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common_dec/DecSynchro.h"

/*************************************************************************//*!
    \brief Virtual interface used to access the scheduler of the Decoder IP.
    If you want to create multiple channels in the same process that access the same IP, the AL_IDecScheduler should be shared between them.
    \see AL_DecSchedulerCpu_Create and AL_DecSchedulerMcu_Create if available to get concrete implementations of this interface.
*****************************************************************************/
typedef struct AL_i_DecScheduler AL_IDecScheduler;
void AL_IDecScheduler_Destroy(AL_IDecScheduler* pThis);

typedef enum
{
  AL_DEC_HANDLE_STATE_PROCESSING,
  AL_DEC_HANDLE_STATE_PROCESSED,
  AL_DEC_HANDLE_STATE_MAX_ENUM, /* sentinel */
}AL_EDecHandleState;

typedef enum
{
  AL_DEC_UNSPLIT_INPUT, /*!<The input is fed to the decoder without delimitations and the decoder find the decoding unit in the data by himself.*/
  AL_DEC_SPLIT_INPUT, /*!<The input is fed to the decoder with buffers containing one decoding unit each. */
}AL_EDecInputMode;

typedef struct
{
  AL_EDecHandleState eState;
  AL_TBuffer* pHandle;
}AL_TDecMetaHandle;

/*************************************************************************//*!
   \brief Handle to the decoder object.
   \see AL_Decoder_Create
   \see AL_Decoder_Destroy
*****************************************************************************/
typedef AL_HANDLE AL_HDecoder;

/*************************************************************************//*!
   \brief Decoder Settings
   \ingroup Decoder_Settings
*****************************************************************************/
typedef struct
{
  int iStackSize;       /*!< Size of the command stack handled by the decoder */
  int iStreamBufSize;   /*!< Size of the internal circular stream buffer (0 = default) */
  uint8_t uNumCore;     /*!< Number of core used for the decoding */
  uint32_t uFrameRate;  /*!< Frame rate value used if syntax element isn't present */
  uint32_t uClkRatio;   /*!< Clock ratio value used if syntax element isn't present */
  AL_ECodec eCodec;     /*!< Specify which codec is used */
  bool bParallelWPP;    /*!< Should wavefront parallelization processing be used (might be ignored if not available) */
  uint8_t uDDRWidth;    /*!< Width of the DDR uses by the decoder */
  bool bDisableCache;   /*!< Should the decoder disable the cache */
  bool bLowLat;         /*!< Should low latency decoding be used */
  bool bForceFrameRate; /*!< Should stream frame rate be ignored and replaced by user defined one */
  bool bFrameBufferCompression; /*!< Should internal frame buffer compression be used */
  AL_EFbStorageMode eFBStorageMode; /*!< Specifies the storage mode the decoder should use for the frame buffers*/
  AL_EDecUnit eDecUnit; /*!< Should subframe latency mode be used */
  AL_EDpbMode eDpbMode; /*!< Should low ref mode be used */
  AL_TStreamSettings tStream; /*!< Stream's settings. These need to be set if you want to preallocate the buffer. memset to 0 otherwise */
  AL_EBufferOutputMode eBufferOutputMode; /*!< Reconstructed buffers output mode */
  bool bUseIFramesAsSyncPoint; /*!< Allow decoder to sync on I frames if configurations' nals are presents */
  bool bUseEarlyCallback; /*< Lowlat phase 2. This only makes sense with special support for hw synchro */
  AL_EDecInputMode eInputMode; /* Send stream data by decoding unit or feed the library enough data and let it find the units. */
}AL_TDecSettings;

/*************************************************************************//*!
   \brief Decoder callbacks
*****************************************************************************/
typedef struct
{
  AL_CB_EndParsing endParsingCB; /*!< Called when an input buffer is parsed */
  AL_CB_EndDecoding endDecodingCB; /*!< Called when a frame is decoded */
  AL_CB_Display displayCB; /*!< Called when a buffer is ready to be displayed */
  AL_CB_ResolutionFound resolutionFoundCB; /*!< Called when a resolution change occurs */
  AL_CB_ParsedSei parsedSeiCB; /*!< Called when a SEI is parsed */
}AL_TDecCallBacks;

/*************************************************************************//*!
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

/*************************************************************************//*!
   \brief Releases all allocated and/or owned ressources
   \param[in] hDec Handle to Decoder object previously created with AL_Decoder_Create
   \see AL_Decoder_Create
*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec);

/****************************************************************************/
/* internal. Used for traces */
void AL_Decoder_SetParam(AL_HDecoder hDec, const char* sPrefix, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool bShouldPrintFrameDelimiter);

/*************************************************************************//*!
   \brief Pushes a buffer to the decoder queue. It will be decoded when possible
   \param[in] hDec Handle to a decoder object.
   \param[in] pBuf Pointer to the encoded bitstream buffer
   \param[in] uSize Size in bytes of actual data in pBuf
   \return return the current error status
*****************************************************************************/
bool AL_Decoder_PushBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize);

/*************************************************************************//*!
   \brief Flushes the decoding request stack when the stream parsing is finished.
   \param[in]  hDec Handle to a decoder object.
*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Puts the display buffer inside the decoder internal bufpool
   It is used to give the decoder buffers where it will output the decoded pictures
   \param[in] hDec   Handle to a decoder object.
   \param[in] pDisplay Pointer to the decoded picture buffer
*****************************************************************************/
void AL_Decoder_PutDisplayPicture(AL_HDecoder hDec, AL_TBuffer* pDisplay);

/*************************************************************************//*!
   \brief Retrieves the codec of the specified decoder instance
   \param[in] hDec   Handle to a decoder object.
*****************************************************************************/
AL_ECodec AL_Decoder_GetCodec(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Retrieves the maximum bitdepth allowed by the stream profile
   \param[in] hDec Handle to a decoder object.
   \return return the maximum bidepth allowed byu the current stream profile
*****************************************************************************/
int AL_Decoder_GetMaxBD(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Retrieves the last decoder error state
   \param[in]  hDec  Handle to a decoder object.
   \return return the current error status
*****************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Retrieves the error status related to a specific frame
   \param[in]  hDec  Handle to a decoder object.
   \param[in]  pBuf  Pointer to the decoded picture buffer for which to get the error status
   \return return the frame error status
*****************************************************************************/
AL_ERR AL_Decoder_GetFrameError(AL_HDecoder hDec, AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Preallocates internal buffers.
   This is only usable if Stream settings are set. Calling this function without
   the stream settings will assert.
   Internal buffers of the decoder are normally allocated after determining
   what kind of stream is being decoded. If you know the stream settings
   in advance, you can override this behavior by giving these informations at
   the decoder creation and calling AL_Decoder_PreallocateBuffers.
   Giving wrong information about the stream will lead to undefined behaviour.
   Calling this function will reduce the memory footprint of the decoder and
   will remove the latency of the allocation of the internal buffer at runtime.
   \param[in]  hDec  Handle to a decoder object.
   \return return false if the allocation failed. True otherwise
*****************************************************************************/
bool AL_Decoder_PreallocateBuffers(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Give the minimum stride supported by the decoder for its reconstructed buffers
   \param[in] uWidth width of the reconstructed buffers in pixels
   \param[in] uBitDepth of the stream (8 or 10)
   \param[in] eFrameBufferStorageMode frame buffer storage mode
*****************************************************************************/
uint32_t AL_Decoder_GetMinPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);

/*************************************************************************//*!
   \brief Give the minimum stride height supported by the decoder
   Restriction: The decoder still only supports a stride height set to AL_Decoder_GetMinStrideHeight.
   all other strideHeight will be ignored
   \param[in] uHeight height of the picture in pixels
*****************************************************************************/
uint32_t AL_Decoder_GetMinStrideHeight(uint32_t uHeight);

