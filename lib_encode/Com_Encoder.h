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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/

#pragma once

#include "Encoder.h"
#include "IP_EncoderCtx.h"

#include "lib_encode/I_EncScheduler.h"
#include "lib_common/HDR.h"

/*************************************************************************//*!
   \brief Creates an encoder object
   \return Pointer to the encoder created if succeeded, NULL otherwise
*****************************************************************************/
AL_TEncCtx* AL_Common_Encoder_Create(AL_TAllocator* pAlloc);

/*************************************************************************//*!
   \brief Destroy an encoder object
   \param[in] pEnc Pointer on the encoder context object to destroy
*****************************************************************************/
void AL_Common_Encoder_Destroy(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief Creates an encoding channel
   \param[in] pEnc Pointer on an encoder object
   \param[in] pScheduler Pointer to the scheduler object
   \param[in] pAlloc Pointer to a AL_TAllocator interface.
   \param[in] pSettings Pointer to AL_TEncSettings structure specifying the encoder
   parameters.
   \return Channel creation result
*****************************************************************************/
AL_ERR AL_Common_Encoder_CreateChannel(AL_TEncCtx* pCtx, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings);

/*************************************************************************//*!
   \brief Get information on created encoder
   \param[in] hEnc Pointer on an encoder object
   \param[out] pEncInfo pEncInfo pointer to structure filled with encoder info
   \return true if succeed, false otherwise
*****************************************************************************/
bool AL_Common_Encoder_GetInfo(AL_TEncCtx* pCtx, AL_TEncoderInfo* pEncInfo);

/*************************************************************************//*!
   \brief The Encoder_NotifySceneChange function informs the encoder that a scene
   change will shortly happen.
   \param[in] pEnc Handle to an encoder object
   \param[in] iAhead Number of frame until the scene change will happen.
   Allowed range is [0..31]
*****************************************************************************/
void AL_Common_Encoder_NotifySceneChange(AL_TEncCtx* pCtx, int iAhead);

/*************************************************************************//*!
   \brief The Encoder_NotifyIsLongTerm function informs the encoder that the
   next reference picture is a long term reference picture
   \param[in] pEnc Handle to an encoder object
*****************************************************************************/
void AL_Common_Encoder_NotifyIsLongTerm(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief The Encoder_NotifyUseLongTerm function informs the encoder that a long
   term reference picture will be used
   \param[in] pEnc Handle to an encoder object
*****************************************************************************/
void AL_Common_Encoder_NotifyUseLongTerm(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief The Encoder_PutStreamBuffer function push a stream buffer to be filled
   \param[in] pEnc Handle to an encoder object
   \param[in] pStream The stream buffer to be filled
   \param[in] iLayerID Current layer identifier
*****************************************************************************/
bool AL_Common_Encoder_PutStreamBuffer(AL_TEncCtx* pCtx, AL_TBuffer* pStream, int iLayerID);

/***************************************************************************/
bool AL_Common_Encoder_GetRecPicture(AL_TEncCtx* pCtx, AL_TRecPic* pRecPic, int iLayerID);

/***************************************************************************/
bool AL_Common_Encoder_ReleaseRecPicture(AL_TEncCtx* pCtx, AL_TRecPic* pRecPic, int iLayerID);

/*************************************************************************//*!
   \brief The Encoder_Process function allows to push a frame buffer to the
   encoder. According to the GOP pattern, this frame buffer
   could or couldn't be encoded immediately.
   \param[in] pEnc Handle to an encoder object
   \param[in] pFrame Pointer to the frame buffer to encode
   \param[in] pQPTable Pointer to an optional qp table used if the external qp table mode is enabled
   \param[in] iLayerID Current layer identifier
   \warning The AL_TBuffer struct pointed to by pFrame shall not be altered.
   \return If the function succeeds the return value is nonzero (true)
   If the function fails the return value is zero (false)
   \see AL_Encoder_PutStreamBuffer
*****************************************************************************/
bool AL_Common_Encoder_Process(AL_TEncCtx* pCtx, AL_TBuffer* pFrame, AL_TBuffer* pQPTable, int iLayerID);

/*************************************************************************//*!
   \brief Add a SEI to the stream
   \param[in] pEnc Pointer on an encoder context object
   \param[in] pStream The stream buffer, required to possess a stream metadata
   \param[in] isPrefix Discriminate between prefix and suffix SEI
   \param[in] iPayloadType SEI payload type. See Annex D.3 of ITU-T
   \param[in] pPayload Raw data of the SEI payload
   \param[in] iPayloadSize Size of the raw data payload
   \param[in] iTempId Temporal id of the raw data payload
   \return returns the section id
*****************************************************************************/
int AL_Common_Encoder_AddSei(AL_TEncCtx* pCtx, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId);

/*************************************************************************//*!
   \brief The Encoder_GetLastError function return the last error if any
   \param[in] pEnc Pointer on an encoder context object
   \return if an error occurs the function returns the corresponding error code,
   otherwise, the function returns AL_SUCCESS.
*****************************************************************************/
AL_ERR AL_Common_Encoder_GetLastError(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief The Encoder_WaitReadiness function wait until the encoder context object is available
   \param[in] pCtx Pointer on an encoder context object
   \return If the function succeeds the return value is nonzero (true)
   If the function fails the return value is zero (false)
*****************************************************************************/
void AL_Common_Encoder_WaitReadiness(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief Requests the encoder to change the cost mode flag.
   \param[in] hEnc Handle to an encoder object
   \param[in] costMode True to enable cost mode, False to disable cost mode
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetCostMode(AL_TEncCtx* pCtx, bool costMode);

/*************************************************************************//*!
   \brief Changes the max picture size set by the rate control
   \param[in] pEnc Pointer on an encoder object
   \param[in] uMaxPictureSize The new maximum picture size
   \param[in] sliceType The slice type used for the max picture size
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetMaxPictureSize(AL_TEncCtx* pCtx, uint32_t uMaxPictureSize, AL_ESliceType sliceType);

/*************************************************************************//*!
   \brief The AL_Encoder_RestartGop requests the encoder to insert a Keyframe
   and restart a new Gop.
   \param[in] pEnc Pointer on an encoder object
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_RestartGop(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief The AL_Encoder_RestartGopRecoveryPoint requests the encoder
   to start a new pass of Gradual Decoding Refresh
   \param[in] pEnc Pointer on an encoder object
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_RestartGopRecoveryPoint(AL_TEncCtx* pCtx);

/*************************************************************************//*!
   \brief The AL_Encoder_SetGopLength changes the GopLength. If the on-going
   Gop is already longer than the new GopLength the encoder will restart a new
   Gop immediately. If the on-going GOP is shorter than the new GopLength,
   the encoder will restart the gop when the on-going one will reach the new
   GopLength
   \param[in] pEnc Pointer on an encoder object
   \param[in] iGopLength New Gop Length
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetGopLength(AL_TEncCtx* pCtx, int iGopLength);

/*************************************************************************//*!
   \brief The AL_Encoder_SetGopNumB changes the Number of consecutive B
   frame in-between 2 I/P frames.
   \param[in] pEnc Pointer on an encoder object
   \param[in] iNumB the new number of B frames
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetGopNumB(AL_TEncCtx* pCtx, int iNumB);

/*************************************************************************//*!
   \brief Changes the IDR frequency. If the new frequency is shorter than the
   number of frames already encoded since the last IDR, insert and IDR as soon
   as possible. Otherwise, the next IDR is inserted when the new IDR frequency
   is reached.
   \param[in] pEnc Pointer on an encoder object
   \param[in] iFreqIDR The new IDR frequency (number of frames between two
   IDR pictures, -1 to disable IDRs). Might be rounded to a GOP length multiple.
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetFreqIDR(AL_TEncCtx* pCtx, int iFreqIDR);

/*************************************************************************//*!
   \brief The AL_Encoder_SetBitRate changes the target bitrate
   \param[in] pEnc Pointer on an encoder object
   \param[in] iBitRate New Target bitrate
   \param[in] iLayerID layer identifier of the channel which will have his bitrate changed
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetBitRate(AL_TEncCtx* pCtx, int iBitRate, int iLayerID);

/*************************************************************************//*!
   \brief The AL_Encoder_SetMaxBitRate changes the max bitrate
   \param[in] pEnc Pointer on an encoder object
   \param[in] iTargetBitRate New Target bitrate
   \param[in] iMaxBitRate New Max bitrate
   \param[in] iLayerID layer identifier of the channel which will have his bitrate changed
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetMaxBitRate(AL_TEncCtx* pCtx, int iTargetBitRate, int iMaxBitRate, int iLayerID);

/*************************************************************************//*!
   \brief The AL_Encoder_SetFrameRate changes the encoding frame rate
   \param[in] pEnc Pointer on an encoder object
   \param[in] uFrameRate the new frame rate
   \param[in] uClkRatio the ClkRatio
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
   \note Fps = uFrameRate * 1000 / uClkRatio. for example uFrameRate = 60 and
   uClkRatio = 1001 gives 59.94 fps
*****************************************************************************/
bool AL_Common_Encoder_SetFrameRate(AL_TEncCtx* pCtx, uint16_t uFrameRate, uint16_t uClkRatio);

/*************************************************************************//*!
   \brief Changes the quantization parameter for the next pushed frame
   \param[in] pEnc Pointer on an encoder object
   \param[in] iQP The new quantization parameter
   \return true on success, false on error : call AL_Common_Encoder_GetLastError
   to retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetQP(AL_TEncCtx* pCtx, int16_t iQP);

/*************************************************************************//*!
   \brief Changes the bounds of the QP set by the rate control
   \param[in] pEnc Pointer on an encoder object
   \param[in] iMinQP The new QP lower bound
   \param[in] iMaxQP The new QP upper bound
   \param[in] sliceType The slice type used for the bounds
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetQPBounds(AL_TEncCtx* pCtx, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType);

/*************************************************************************//*!
   \brief Changes the QP delta between I frames and P frames
   \param[in] pEnc Pointer on an encoder object
   \param[in] uIPDelta The new QP IP delta
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetQPIPDelta(AL_TEncCtx* pCtx, int16_t uIPDelta);

/*************************************************************************//*!
   \brief Changes the QP delta between P frames and B frames
   \param[in] pEnc Pointer on an encoder object
   \param[in] uPBDelta The new QP PB delta
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetQPPBDelta(AL_TEncCtx* pCtx, int16_t uPBDelta);

/*************************************************************************//*!
   \brief Changes the resolution of the input frames to encode from the next
   pushed frame
   \param[in] pEnc Pointer on an encoder object
   \param[in] tDim The new dimension of pushed frames
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetInputResolution(AL_TEncCtx* pCtx, AL_TDimension tDim);

/*************************************************************************//*!
   \brief Changes the loop filter beta offset
   \param[in] pEnc Pointer on an encoder object
   \param[in] iBetaOffset The new loop filter beta offset
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetLoopFilterBetaOffset(AL_TEncCtx* pCtx, int8_t iBetaOffset);

/*************************************************************************//*!
   \brief Changes the loop filter TC offset
   \param[in] pEnc Pointer on an encoder object
   \param[in] iTcOffset The new loop filter TC offset
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetLoopFilterTcOffset(AL_TEncCtx* pCtx, int8_t iTcOffset);

/*************************************************************************//*!
   \brief Changes Qp chroma offset
   \param[in] pEnc Pointer on an encoder object
   \param[in] iQp1Offset The new Cb chroma offset for avc/hevc/vvc. The new Dc
   chroma offset for av1.
   \param[in] iQp2Offset The new Cr chroma offset for avc/hevc/vvc. The new Ac
   chroma offset for av1.
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetQPChromaOffsets(AL_TEncCtx* pCtx, int8_t iQp1Offset, int8_t iQp2Offset);

/*************************************************************************//*!
   \brief Enable or Disable AutoQP control
   \param[in] hEnc Handle to an encoder object
   \param[in] useAutoQP The boolean to activate the use of AUTO_QP
   \return true on success, false on error : call AL_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetAutoQP(AL_TEncCtx* pCtx, bool useAutoQP);

/*************************************************************************//*!
   \brief Specify HDR SEIs to insert in the bitstream
   \param[in] pEnc  Pointer on an encoder object
   \param[in] pHDRSettings pointer to the HDR related SEIs
   \return true on success, false on error : call AL_Common_Encoder_GetLastError to
   retrieve the error code
*****************************************************************************/
bool AL_Common_Encoder_SetHDRSEIs(AL_TEncCtx* pCtx, AL_THDRSEIs* pHDRSEIs);

/*************************************************************************//*!
   \brief The SetME function initializes the motion estimation parameters
   \param[in]  iHrzRange_P Horizontal range for P Slice
   \param[in]  iVrtRange_P Vertical range for P Slice
   \param[in]  iHrzRange_B Horizontal range for B Slice
   \param[in]  iVrtRange_B Vertical range for B Slice
   \param[out] pChParam Pointer to the structure receiving the motion estimation parameters
*****************************************************************************/
void AL_Common_Encoder_SetME(int iHrzRange_P, int iVrtRange_P, int iHrzRange_B, int iVrtRange_B, AL_TEncChanParam* pChParam);

/*************************************************************************//*!
   \brief The AL_Common_Encoder_ComputeRCParam function initializes the rate control basic parameters
   \param[in]  iCbOffset U plane qp offset
   \param[in]  iCrOffset V plane qp offset
   \param[in]  iIntraOnlyOff QP offset to apply in intra only case
   \param[out] pChParam Pointer to the structure receiving the rate control parameters
*****************************************************************************/
void AL_Common_Encoder_ComputeRCParam(int iCbOffset, int iCrOffset, int iIntraOnlyOff, AL_TEncChanParam* pChParam);

/*************************************************************************//*!
   \brief The AL_Common_Encoder_GetInitialQP function compute the initial QP value when using rate controller
   \param[in]  iBitPerPixel number of coded bits per pixel : computed using BitRate and FrameRate
   \param[in]  eProfile encoding profile used
   \return returns the initial QP value
*****************************************************************************/
uint8_t AL_Common_Encoder_GetInitialQP(uint32_t iBitPerPixel, AL_EProfile eProfile);

#if AL_ENABLE_ENC_WATCHDOG
void AL_Common_Encoder_SetWatchdogCB(AL_TEncCtx* pCtx, const AL_TEncSettings* pSettings);
#endif

void AL_Common_Encoder_SetHlsParam(AL_TEncChanParam* pChParam);
bool AL_Common_Encoder_IsInitialQpProvided(AL_TEncChanParam* pChParam);

/*@}*/

