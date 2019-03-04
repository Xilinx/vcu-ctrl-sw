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
   \defgroup Encoder

   The diagram below shows the usual usage of the encoder control software API
   \htmlonly
    <br/><object data="Encoder.svg">
   \endhtmlonly

   \defgroup Encoder_API API
   \ingroup Encoder

   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"
#include "lib_common_enc/Settings.h"
#include "lib_common_enc/EncRecBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_Scheduler TScheduler;
void AL_ISchedulerEnc_Destroy(TScheduler* pScheduler);

/*************************************************************************//*!
   \brief Handle to an Encoder object
*****************************************************************************/
typedef AL_HANDLE AL_HEncoder;

/*************************************************************************//*!
   \brief This callback is called when
   - a frame is encoded ((pStream != NULL) && (pSrc != NULL))
   - eos reached ((pStream == NULL) && (pSrc == NULL))
   - release stream buffer ((pStream != NULL) && (pSrc == NULL))
   - release source buffer ((pStream == NULL) && (pSrc != NULL))
   \param[out] pUserParam User parameter
   \param[out] pStream The stream buffer if any
   \param[out] pSrc The source buffer associated to the stream if any
   \param[in] iLayerID layer identifier
*****************************************************************************/
typedef struct
{
  void (* func)(void* pUserParam, AL_TBuffer* pStream, AL_TBuffer const* pSrc, int iLayerID);
  void* userParam;
}AL_CB_EndEncoding;

/*************************************************************************//*!
   \brief Creates a new instance of the encoder
   and returns a handle that can be used to access the object
   \param[out] hEnc handle to the new created encoder
   \param[in] pScheduler Pointer to the scheduler object.
   \param[in] pAlloc Pointer to a AL_TAllocator interface.
   \param[in] pSettings Pointer to AL_TEncSettings structure specifying the encoder
   parameters.
   \param[in] callback callback called when the encoding of a frame is finished
   \return errorcode specifying why this encoder couldn't be created
   \see AL_Encoder_Destroy
*****************************************************************************/
AL_ERR AL_Encoder_Create(AL_HEncoder* hEnc, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback);

/*************************************************************************//*!
   \brief Releases all allocated and/or owned ressources
   \param[in] hEnc Handle to Encoder object previously created with CreateEncoder
   \see AL_Encoder_Create
*****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Informs the encoder that a scene change will shortly happen.
   \param[in] hEnc Handle to an encoder object
   \param[in] iAhead Number of frame until the scene change will happen.
   Allowed range is [0..31]
*****************************************************************************/
void AL_Encoder_NotifySceneChange(AL_HEncoder hEnc, int iAhead);

/*************************************************************************//*!
   \brief Informs the encoder that the next reference picture is a long term
   reference picture
   \param[in] hEnc Handle to an encoder object
*****************************************************************************/
void AL_Encoder_NotifyIsLongTerm(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Informs the encoder that a long term reference picture will be used
   \param[in] hEnc Handle to an encoder object
*****************************************************************************/
void AL_Encoder_NotifyUseLongTerm(AL_HEncoder hEnc);




/*************************************************************************//*!
   \brief When the encoder has been created with bEnableRecOutput set to
   true, the AL_Encoder_GetRecPicture function allows to retrieve the
   reconstructed frame picture in Display order.
   \param[in] hEnc Handle to an encoder object
   \param[in] pRecPic pointer to structure that receives the buffer information.
   \return true if a reconstructed buffer is available, false otherwise.
   \see AL_Encoder_ReleaseRecPicture
*****************************************************************************/
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic);

/*************************************************************************//*!
   \brief Release Reconstructed buffer previously obtains through
   AL_Encoder_GetRecPicture.
   \param[in] hEnc Handle to an encoder object
   \param[in] pRecPic Buffer to be released.
   \see AL_Encoder_GetRecPicture
*****************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic);

/*************************************************************************//*!
   \brief Pushes a stream buffer in the encoder stream buffer queue.
   This buffer will be used by the encoder to store the encoded bitstream.
   \param[in] hEnc Handle to an encoder object
   \param[in] pStream Pointer to the stream buffer given to the encoder.
   The buffer needs to have an associated AL_TStreamMetaData created with
   AL_StreamMetaData_Create(sectionNumber, uMaxSize) where uMaxSize shall be
   aligned on 32 bits and the sectionNumber should be set to AL_MAX_SECTION
   or more if the user wants to create his own sections (to add SEI inside
   the stream for example)
   \return return true if the buffer was successfully pushed. false if an error
   occured
*****************************************************************************/
bool AL_Encoder_PutStreamBuffer(AL_HEncoder hEnc, AL_TBuffer* pStream);

/*************************************************************************//*!
   \brief Pushes a frame buffer to the encoder.
   According to the GOP pattern, this frame buffer could or couldn't be encoded immediately.
   \param[in] hEnc  Handle to an encoder object
   \param[in] pFrame Pointer to the frame buffer to encode
   The pFrame buffer needs to have an associated AL_TSrcMetaData describing
   how the yuv is stored in memory. The memory of the buffer should not be altered
   while the encoder is using it. There are some restrictions associated to the
   source metadata.
   The chroma pitch has to be equal to the luma pitch and must be 32bits aligned
   and should be superior to the minimum supported pitch for the resolution
   (See AL_EncGetMinPitch()). The chroma offset shouldn't fall inside the luma block.
   The FourCC should be the same as the one from the channel (See AL_EncGetSrcFourCC())
   \param[in] pQpTable Pointer to an optional qp table used if the external qp table mode is enabled
   \return If the function succeeds the return value is nonzero (true)
   If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Encoder_Process(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable);

/*************************************************************************//*!
   \brief Add a SEI to the stream
   This function should be called after the encoder has encoded the bitstream.
   The maximum final size of the SEI in the stream can't exceed 2Ko.
   The sei payload does not need to be anti emulated, this will be done by the
   encoder.
   \param[in] hEnc Handle to the encoder
   \param[in] pStream The stream buffer, required to possess a stream metadata
   \param[in] isPrefix Discriminate between prefix and suffix SEI
   \param[in] iPayloadType SEI payload type. See Annex D.3 of ITU-T
   \param[in] pPayload Raw data of the SEI payload
   \param[in] iPayloadSize Size of the raw data payload
   \param[in] iTempId Temporal id of the raw data payload
   \return returns the section id
*****************************************************************************/
int AL_Encoder_AddSei(AL_HEncoder hEnc, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId);


/*************************************************************************//*!
   \brief Return an error code when an error has occured during encoding,
   otherwise the function returns AL_SUCCESS.
   \param[in] hEnc Handle to an encoder object
*****************************************************************************/
AL_ERR AL_Encoder_GetLastError(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Requests the encoder to insert a Keyframe and restart a new Gop.
   \param[in] hEnc Handle to an encoder object
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_RestartGop(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Changes the GopLength. If the on-going
   Gop is already longer than the new GopLength the encoder will restart a new
   Gop immediately. If the on-going GOP is shorter than the new GopLength,
   the encoder will restart the gop when the on-going one will reach the new
   GopLength
   \param[in] hEnc Handle to an encoder object
   \param[in] iGopLength New Gop Length
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetGopLength(AL_HEncoder hEnc, int iGopLength);

/*************************************************************************//*!
   \brief Changes the Number of consecutive B
   frame in-between 2 I/P frames.
   \param[in] hEnc Handle to an encoder object
   \param[in] iNumB the new number of B frames
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetGopNumB(AL_HEncoder hEnc, int iNumB);

/*************************************************************************//*!
   \brief Changes the target bitrate
   \param[in] hEnc Handle to an encoder object
   \param[in] iBitRate New target bitrate in kbps
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetBitRate(AL_HEncoder hEnc, int iBitRate);

/*************************************************************************//*!
   \brief Changes the encoding frame rate
   \param[in] hEnc Handle to an encoder object
   \param[in] uFrameRate the new frame rate
   \param[in] uClkRatio the ClkRatio
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
   \note Fps = iFrameRate * 1000 / iClkRatio. for example uFrameRate = 60 and
   uClkRatio = 1001 gives 59.94 fps
*****************************************************************************/
bool AL_Encoder_SetFrameRate(AL_HEncoder hEnc, uint16_t uFrameRate, uint16_t uClkRatio);

/*************************************************************************//*!
   \brief Changes the quantization parameter for the next pushed frame
   \param[in] hEnc Handle to an encoder object
   \param[in] iQP The new quantization parameter
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQP(AL_HEncoder hEnc, int16_t iQP);

/*************************************************************************//*!
   \brief Changes the resolution of the input frames to encode from the next
   pushed frame
   \param[in] hEnc Handle to an encoder object
   \param[in] tDim The new dimension of pushed frames
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetInputResolution(AL_HEncoder hEnc, AL_TDimension tDim);



#ifdef __cplusplus
}
#endif

/*@}*/

