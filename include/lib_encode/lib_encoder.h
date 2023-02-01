/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
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

/**************************************************************************//*!
   \defgroup Encoder Encoder

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

/*************************************************************************//*!
    \brief Virtual interface used to access the scheduler of the Encoder IP.
    If you want to create multiple channels in the same process that access the same IP, the AL_IEncScheduler should be shared between them.
    \see AL_SchedulerCpu_Create and AL_SchedulerMcu_Create if available to get concrete implementations of this interface.
*****************************************************************************/
typedef struct AL_i_EncScheduler AL_IEncScheduler;
extern void AL_IEncScheduler_Destroy(AL_IEncScheduler* pScheduler);

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
   \brief Select control software architecture
*****************************************************************************/
typedef enum
{
  AL_LIB_ENCODER_ARCH_HOST,
}AL_ELibEncoderArch;

/*************************************************************************//*!
   \brief Information on created encoder
*****************************************************************************/
typedef struct
{
  uint8_t uNumCore; /*< number of cores used for encoding */
}AL_TEncoderInfo;

/*************************************************************************//*!
   \brief Initialize encoder library
   \param[in] eArch  encoder library arch to use
   \return error code specifying why library initialization has failed
*****************************************************************************/
AL_ERR AL_Lib_Encoder_Init(AL_ELibEncoderArch eArch);

/*************************************************************************//*!
   \brief Deinitialize encoder library
*****************************************************************************/
void AL_Lib_Encoder_DeInit(void);

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
AL_ERR AL_Encoder_Create(AL_HEncoder* hEnc, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback);

/*************************************************************************//*!
   \brief Releases all allocated and/or owned resources
   \param[in] hEnc Handle to Encoder object previously created with CreateEncoder
   \see AL_Encoder_Create
*****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Get information on created encoder
   \param[in] hEnc Handle to an encoder object
   \param[out] pEncInfo pEncInfo pointer to structure filled with encoder info
   \return true if succeed, false otherwise
*****************************************************************************/
bool AL_Encoder_GetInfo(AL_HEncoder hEnc, AL_TEncoderInfo* pEncInfo);

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
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic);

/*************************************************************************//*!
   \brief Release Reconstructed buffer previously obtains through
   AL_Encoder_GetRecPicture.
   \param[in] hEnc Handle to an encoder object
   \param[in] pRecPic Buffer to be released.
   \see AL_Encoder_GetRecPicture
*****************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic);

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
   The pFrame buffer needs to have an associated AL_TPixMapMetaData describing
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
   \brief Requests the encoder to change the cost mode flag.
   \param[in] hEnc Handle to an encoder object
   \param[in] costMode True to enable cost mode, False to disable cost mode
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetCostMode(AL_HEncoder hEnc, bool costMode);

/*************************************************************************//*!
   \brief Changes the max picture size set by the rate control
   \param[in] pEnc Pointer on an encoder object
   \param[in] uMaxPictureSize The new maximum picture size
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetMaxPictureSize(AL_HEncoder hEnc, uint32_t uMaxPictureSize);

/*************************************************************************//*!
   \brief Changes the max picture size set by the rate control
   \param[in] pEnc Pointer on an encoder object
   \param[in] uMaxPictureSize The new maximum picture size
   \param[in] sliceType The slice type used for the max picture size
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetMaxPictureSizePerFrameType(AL_HEncoder hEnc, uint32_t uMaxPictureSize, AL_ESliceType sliceType);

/*************************************************************************//*!
   \brief Requests the encoder to insert a Keyframe and restart a new Gop.
   \param[in] hEnc Handle to an encoder object
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_RestartGop(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief Requests the encoder to start a new pass of Gradual Decoding
   Refresh.
   \param[in] hEnc Handle to an encoder object
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_RestartGopRecoveryPoint(AL_HEncoder hEnc);

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
   \brief Changes the IDR frequency. If the new frequency is shorter than the
   number of frames already encoded since the last IDR, insert and IDR as soon
   as possible. Otherwise, the next IDR is inserted when the new IDR frequency
   is reached.
   \param[in] hEnc Handle to an encoder object
   \param[in] iFreqIDR The new IDR frequency (number of frames between two
   IDR pictures, -1 to disable IDRs). Might be rounded to a GOPLength multiple.
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetFreqIDR(AL_HEncoder hEnc, int iFreqIDR);

/*************************************************************************//*!
   \brief Changes the target bitrate
   \param[in] hEnc Handle to an encoder object
   \param[in] iBitRate New target bitrate in kbps
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetBitRate(AL_HEncoder hEnc, int iBitRate);

/*************************************************************************//*!
   \brief Changes the max bitrate
   \param[in] hEnc Handle to an encoder object
   \param[in] iTargetBitRate New target bitrate in kbps
   \param[in] iMaxBitRate New maximum bitrate in kbps
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetMaxBitRate(AL_HEncoder hEnc, int iTargetBitRate, int iMaxBitRate);

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
   \brief Changes the bounds of the QP set by the rate control
   \param[in] hEnc Handle to an encoder object
   \param[in] iMinQP The new QP lower bound
   \param[in] iMaxQP The new QP upper bound
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQPBounds(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP);

/*************************************************************************//*!
   \brief Changes the bounds of the QP set by the rate control for a slice type
   \param[in] hEnc Handle to an encoder object
   \param[in] iMinQP The new QP lower bound
   \param[in] iMaxQP The new QP upper bound
   \param[in] sliceType The slice type on which apply the new QPBounds
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQPBoundsPerFrameType(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType);

/*************************************************************************//*!
   \brief Changes the QP delta between I frames and P frames
   \param[in] hEnc Handle to an encoder object
   \param[in] uIPDelta The new QP IP delta
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQPIPDelta(AL_HEncoder hEnc, int16_t uIPDelta);

/*************************************************************************//*!
   \brief Changes the QP delta between P frames and B frames
   \param[in] hEnc Handle to an encoder object
   \param[in] uPBDelta The new QP PB delta
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQPPBDelta(AL_HEncoder hEnc, int16_t uPBDelta);

/*************************************************************************//*!
   \brief Changes the resolution of the input frames to encode from the next
   pushed frame
   \param[in] hEnc Handle to an encoder object
   \param[in] tDim The new dimension of pushed frames
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetInputResolution(AL_HEncoder hEnc, AL_TDimension tDim);

/*************************************************************************//*!
   \brief Changes the loop filter beta offset
   \param[in] hEnc Handle to an encoder object
   \param[in] iBetaOffset The new loop filter beta offset
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetLoopFilterBetaOffset(AL_HEncoder hEnc, int8_t iBetaOffset);

/*************************************************************************//*!
   \brief Changes the loop filter TC offset
   \param[in] hEnc Handle to an encoder object
   \param[in] iTcOffset The new loop filter TC offset
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetLoopFilterTcOffset(AL_HEncoder hEnc, int8_t iTcOffset);

/*************************************************************************//*!
   \brief Changes chroma offsets. change will be applied for current picture
   and for following pictures in display order.
   \param[in] hEnc Handle to an encoder object
   \param[in] iQp1Offset The new Cb chroma offset for avc/hevc/vvc. The new Dc
   chroma offset for av1.
   \param[in] iQp2Offset The new Cr chroma offset for avc/hevc/vvc. The new Ac
   chroma offset for av1.
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetQPChromaOffsets(AL_HEncoder hEnc, int8_t iQp1Offset, int8_t iQp2Offset);

/*************************************************************************//*!
   \brief Enable or Disable AutoQP control
   \param[in] hEnc Handle to an encoder object
   \param[in] useAutoQP The boolean to activate the use of AUTO_QP
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetAutoQP(AL_HEncoder hEnc, bool useAutoQP);

/*************************************************************************//*!
   \brief Specify HDR SEIs to insert in the bitstream
   \param[in] hEnc Handle to an encoder object
   \param[in] pHDRSEIs pointer to the HDR related SEIs
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Encoder_SetHDRSEIs(AL_HEncoder hEnc, AL_THDRSEIs* pHDRSEIs);

/*@}*/

