/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
#include "lib_common/FourCC.h"

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common_dec/DecSynchro.h"

typedef struct AL_t_IDecChannel AL_TIDecChannel;

/*************************************************************************//*!
   \brief Decoded callback definition.
   It is called every time a frame is decoded
   A null frame indicates an error occured
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pDecodedFrame, void* pUserParam);
  void* userParam;
}AL_CB_EndDecoding;

/*************************************************************************//*!
   \brief Display callback definition.
   It is called every time a frame can be displayed
   a null frame indicates the end of the stream
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pDisplayedFrame, AL_TInfoDecode* pInfo, void* pUserParam);
  void* userParam;
}AL_CB_Display;

/*************************************************************************//*!
   \brief Resolution change callback definition.
   It is called only once when the first decoding process occurs.
   The decoder doesn't support a change of resolution inside a stream
   Callback must return an error code that can be different from AL_SUCCESS
   in case of memory allocation error
*****************************************************************************/
typedef struct
{
  AL_ERR (* func)(int BufferNumber, int BufferSize, AL_TStreamSettings const* pSettings, AL_TCropInfo const* pCropInfo, void* pUserParam);
  void* userParam;
}AL_CB_ResolutionFound;

/*************************************************************************//*!
   \brief Parsed SEI callback definition.
   It is called when a SEI is parsed
   Antiemulation has already been removed by the decoder from the payload.
   See Annex D.3 of ITU-T for the sei_payload syntax
*****************************************************************************/
typedef struct
{
  void (* func)(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, void* pUserParam);
  void* userParam;
}AL_CB_ParsedSei;

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
  int iBitDepth;        /*!< Output bitDepth */
  uint8_t uNumCore;     /*!< Number of hevc decoder core used for the decoding */
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
}AL_TDecSettings;

/*************************************************************************//*!
   \brief Decoder callbacks
*****************************************************************************/
typedef struct
{
  AL_CB_EndDecoding endDecodingCB; /*!< Called when a frame is decoded */
  AL_CB_Display displayCB; /*!< Called when a buffer is ready to be displayed */
  AL_CB_ResolutionFound resolutionFoundCB; /*!< Called when a resolution change occurs */
  AL_CB_ParsedSei parsedSeiCB; /*!< Called when a SEI is parsed */
}AL_TDecCallBacks;

/*************************************************************************//*!
   \brief Creates a new instance of the Decoder
   \param[out] hDec           handle to the created decoder
   \param[in] pDecChannel     Pointer to an dec channel structure.
   \param[in] pAllocator      Pointer to an allocator handle
   \param[in] pSettings       Pointer to the decoder settings
   \param[in] pCB             Pointer to the decoder callbacks
   \return error code specifying why this decoder couldn't be created
   \see AL_Decoder_Destroy
*****************************************************************************/
AL_ERR AL_Decoder_Create(AL_HDecoder* hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*************************************************************************//*!
   \brief Releases all allocated and/or owned ressources
   \param[in] hDec Handle to Decoder object previously created with AL_Decoder_Create
   \see AL_Decoder_Create
*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec);

/****************************************************************************/
/* internal. Used for traces */
void AL_Decoder_SetParam(AL_HDecoder hDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm, bool bForceCleanBuffers);

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

/*************************************************************************//*!
   \brief Give the size of a reconstructed picture buffer
   Restriction: The strideHeight is supposed to be the minimum stride height
   \param[in] tDim dimensions of the picture
   \param[in] iPitch luma pitch in bytes of the picture
   \param[in] eChromaMode chroma mode of the picture
   \param[in] bFrameBufferCompression will the frame buffer be compressed
   \param[in] eFbStorage frame buffer storage mode
*****************************************************************************/
int AL_DecGetAllocSize_Frame(AL_TDimension tDim, int iPitch, AL_EChromaMode eChromaMode, bool bFrameBufferCompression, AL_EFbStorageMode eFbStorage);
/*@}*/

AL_DEPRECATED("Renamed. Use AL_Decoder_GetMinPitch. Will be deleted in 0.9")
uint32_t AL_Decoder_RoundPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);
AL_DEPRECATED("Renamed. Use AL_Decoder_GetMinStrideHeight. Will be deleted in 0.9")
uint32_t AL_Decoder_RoundHeight(uint32_t uHeight);
// AL_DEPRECATED("Use AL_DecGetAllocSize_Frame. This function doesn't take the stride of the allocated buffer in consideration. Will be deleted in 0.9")
int AL_GetAllocSize_Frame(AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFbStorage);

