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
   \defgroup lib_encode lib_encode
   @{
   \file
 *****************************************************************************/

#pragma once

#include "IScheduler.h"
#include "lib_common/BufferAccess.h"
#include "lib_common/BufferAPI.h"
#include "lib_common_enc/Settings.h"
#include "lib_common_enc/EncRecBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AL_HANDLE AL_HEncoder;

typedef struct
{
  void (* func)(void* pUserParam, AL_TBuffer* pStream, AL_TBuffer const* pSrc);
  void* userParam;
}AL_CB_EndEncoding;

/*************************************************************************//*!
   \brief The AL_Encoder_Create function creates a new instance of the encoder
   and returns a handle that can be used to access the object
   \param[in] pScheduler Pointer to the scheduler object.
   \param[in] pAlloc Pointer to a AL_TAllocator interface.
   \param[in] pSettings Pointer to AL_TEncSettings structure specifying the encoder
   parameters.
   \param[in] callback callback called when the encoding of a frame is finished
   \return If the function succeeds the return value is an handle to the new
   created encoder
   If the function fails the return value is NULL
   \see AL_Encoder_Destroy
*****************************************************************************/
AL_HEncoder AL_Encoder_Create(TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback);

/*************************************************************************//*!
   \brief The AL_Encoder_Destroy function releases all allocated ressources
   \param[in] hEnc Handle to Encoder object previously created with CreateEncoder
   \see AL_Encoder_Create
*****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc);

/*************************************************************************//*!
   \brief The EncoderNotifySceneChange function informs the encoder that a scene
   change will shortly happen.
   \param[in] hEnc Handle to an encoder object
   \param[in] iAhead Number of frame until the scene change will happen.
   Allowed range is [0..31]
*****************************************************************************/
void AL_Encoder_NotifySceneChange(AL_HEncoder hEnc, int iAhead);

/*************************************************************************//*!
   \brief The Encoder_NotifyLongTerm function informs the encoder that a long
   term reference picture will be used
   \param[in] hEnc Handle to an encoder object
*****************************************************************************/
void AL_Encoder_NotifyLongTerm(AL_HEncoder hEnc);

/***************************************************************************/
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic);

/***************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic);

/*************************************************************************//*!
   \brief The AL_Encoder_PutStreamBuffer function allows to push a stream buffer
   in the encoder stream buffer queue. This buffer will be used by the encoder to
   store the encoded bitstream.
   \param[in] hEnc Handle to an encoder object
   \param[in] pStream Pointer to the stream buffer given to the encoder
   \return return true if the buffer was successfully pushed. false if an error
   occured
 ***************************************************************************/
bool AL_Encoder_PutStreamBuffer(AL_HEncoder hEnc, AL_TBuffer* pStream);

/*************************************************************************//*!
   \brief The AL_Encoder_Process function allows to push a frame buffer to the
   H264 encoder. According to the GOP pattern, this frame buffer
   could or couldn't be encoded immediately.
   \param[in] hEnc hEnc  Handle to an encoder object
   \param[in] pFrame Pointer to the frame buffer to encode
   \warning The pData member of each AL_TBuffer struct pointed to by pFrame
   shall not be altered.
   \param[in] pQpTable Pointer to an optional qp table used if the external qp table mode is enabled
   \return If the function succeeds the return value is nonzero (true)
   If the function fails the return value is zero (false)
   \see AL_Encoder_ReleaseFrameBuffer
*****************************************************************************/
bool AL_Encoder_Process(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable);

/*************************************************************************//*!
   \brief The AL_Encoder_GetLastError function return an error code when an
   error has occured during encoding, otherwise the function
   returns AL_SUCCESS.
   \param[in] hEnc Handle to an encoder object
*****************************************************************************/
AL_ERR AL_Encoder_GetLastError(AL_HEncoder hEnc);


#ifdef __cplusplus
}
#endif

/*@}*/

