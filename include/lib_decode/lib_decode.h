/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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
   \defgroup lib_decode_hls lib_decode
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"

#include "lib_common/BufferAccess.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"

#include "I_DecChannel.h"

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
   a null frame indicates the end of the stream
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pDisplayedFrame, AL_TInfoDecode tInfo, void* pUserParam);
  void* userParam;
}AL_CB_Display;

/*************************************************************************//*!
   \brief Resolution change callback definition.
   It is called only once when the first decoding process occurs.
   The decoder doesn't support a change of resolution inside a stream
*****************************************************************************/
typedef struct
{
  void (* func)(int BufferNumber, int BufferSize, int iWidth, int iHeight, AL_TCropInfo tCropInfo, TFourCC tFourCC, void* pUserParam);
  void* userParam;
}AL_CB_ResolutionFound;

/*************************************************************************//*!
   \brief Handle to IP_Decoder.
   \see AL_Decoder_Create
   \see AL_Decoder_Destroy
*****************************************************************************/
typedef AL_HANDLE AL_HDecoder;

typedef struct
{
  int iStackSize;        // !< Size of the command stack handled by the decoder
  int iBitDepth;         // !< Output BitDepth
  uint8_t uNumCore;           // !< Number of hevc decoder core used for the decoding
  uint32_t uFrameRate;        // !< User Frame Rate value which is used if syntax element isn't present
  uint32_t uClkRatio;         // !< User Clock Ratio value which is used if syntax element isn't present
  bool bIsAvc;           // !< Flag specifying which codec is used : true : AVC | Al_FALSE : HEVC
  bool bParallelWPP;     // !< Flag specifying wavefront parallelization processing activation
  uint8_t uDDRWidth;        // !< Width of the DDR uses by the decoder
  bool bDisableCache;    // !< Flag specifying if the decoder disable cache
  bool bLowLat;          // !< LowLatency decoding activation flag
  bool bForceFrameRate;  // !< Indicate to ignore stream frame rate and use the one defined by user
  bool bFrameBufferCompression; // !< Flag specifying if internal frame buffer compression should be used
  AL_EFbStorageMode eFBStorageMode;
  AL_EDecUnit eDecUnit;     // !< SubFrame latency control activation flag
  AL_EDpbMode eDpbMode;     // !< Low ref mode control activation flag
}AL_TDecSettings;

typedef struct
{
  AL_CB_EndDecoding endDecodingCB; // !< Called when a frame is decoded
  AL_CB_Display displayCB; // !< Called when a buffer is ready to be displayed
  AL_CB_ResolutionFound resolutionFoundCB;// !< Called when a resolution change occurs
}AL_TDecCallBacks;

/*************************************************************************//*!
   \brief The AL_Decoder_Create function creates a new instance of the Decoder
   \param[out] hDec           handle to the new created decoder
   \param[in] pDecChannel     Pointer to a Scheduler structure.
   \param[in] pAllocator      Pointer to an allocator handle
   \param[in] pSettings       Pointer to the decoder settings
   \param[in] pCB             Pointer to the decoder callbacks
   \return errorcode specifying why this decoder couldn't be created
   \see AL_Decoder_Destroy
*****************************************************************************/
AL_ERR AL_Decoder_Create(AL_HDecoder* hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*************************************************************************//*!
   \brief The AL_Decoder_Destroy function releases all allocated ressources
   \param[in] hDec Handle to Decoder object previously created with AL_Decoder_Create
   \see AL_Decoder_Create
*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec);

/****************************************************************************/
void AL_Decoder_SetParam(AL_HDecoder hDec, bool bConceal, bool bUseBoard, int iNumTrace, int iNumberTrace);

/*************************************************************************//*!
   \brief Pushes a buffer to the decoder queue. It will be decoded when possible
   \param[in] hDec Handle to an decoder object.
   \param[in] pBuf Pointer to the encoded bitstream buffer
   \param[in] uSize Size in bytes of actual data in pBuf
   \param[in] eMode Blocking strategy.
   \return return the current error status
*****************************************************************************/
bool AL_Decoder_PushBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, AL_EBufMode eMode);

/*************************************************************************//*!
   \brief The AL_Decoder_Flush function allows to flush the decoding request stack when the stream parsing is finished.
   \param[in]  hDec Handle to an decoder object.
*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief The AL_Decoder_ReleaseDecPict function release the decoded picture buffer
   \param[in] hDec   Handle to an decoder object.
   \param[in] pDecPict Pointer to the decoded picture buffer
*****************************************************************************/
void AL_Decoder_ReleaseDecPict(AL_HDecoder hDec, AL_TBuffer* pDecPict);

/*************************************************************************//*!
   \brief The AL_Decoder_PutDecPict function put the decoded picture buffer inside the decoder internal bufpool
   It is used to give the decoder buffers where it will output the decoded pictures
   \param[in] hDec   Handle to an decoder object.
   \param[in] pDecPict Pointer to the decoded picture buffer
*****************************************************************************/
void AL_Decoder_PutDecPict(AL_HDecoder hDec, AL_TBuffer* pDecPict);

/*************************************************************************//*!
   \brief The AL_Decoder_GetMaxBD function retrieves the maximum bitdepth allowed by the stream profile
   \param[in] hDec Handle to an decoder object.
   \return return the maximum bidepth allowed byu the current stream profile
*****************************************************************************/
int AL_Decoder_GetMaxBD(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Retrieves the last decoder error state
   \param[in]  hDec  Handle to an decoder object.
   \return return the current error status
*****************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Force to stop the decoder as soon as possible. This is not a pause.
   \param[in]  hDec  Handle to an decoder object.
*****************************************************************************/
void AL_Decoder_ForceStop(AL_HDecoder hDec);

/*************************************************************************//*!
   \brief Force to flush decoder input context (e.g. pending SC)
   \param[in]  hDec  Handle to an decoder object.
*****************************************************************************/
void AL_Decoder_FlushInput(AL_HDecoder hDec);

/*@}*/

