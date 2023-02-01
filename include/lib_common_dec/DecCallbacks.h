/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#pragma once

#include "lib_common_dec/DecInfo.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"

/*************************************************************************//*!
   \brief Parsing callback definition.
   It is called every time an input buffer as been parsed by the hardware.
   Callback parameters are:
   - pParsedFrame: the frame parsed. If eInputMode is AL_DEC_UNSPLIT_INPUT,
                   pParsedFrame should not have AL_THandleMetaData
   - pUserParam: user context provided in the callback structure
   - iParsingID: the id in the AL_HandleMetaData associated with the buffer
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pParsedFrame, void* pUserParam, int iParsingID);
  void* userParam;
}AL_CB_EndParsing;

/*************************************************************************//*!
   \brief Decoded callback definition.
   It is called every time a frame is decoded. Callback parameters are:
   - pDecodedFrame: the decoded frame.
   - pUserParam: user context provided in the callback structure
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pDecodedFrame, void* pUserParam);
  void* userParam;
}AL_CB_EndDecoding;

/*************************************************************************//*!
   \brief Display callback definition.
   It is called every time a frame can be displayed. Callback parameters are:
   - pDisplayedFrame: the frame to display
   - pInfo: Info on the decoded frame. If pInfo is null, it means that the
            decoder is releasing the pDisplayed frame, and the frame must not
            be displayed. If both pDisplayedFrame and pInfo are null, it means
            that we reached the end of stream.
   - pUserParam: user context provided in the callback structure
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pDisplayedFrame, AL_TInfoDecode* pInfo, void* pUserParam);
  void* userParam;
}AL_CB_Display;

/*************************************************************************//*!
   \brief Resolution change callback definition.
   It is called for each frame resolution change (including the first frame
   resolution detection) and provides all information required to allocate
   frame buffers. Callback must return an error code that can be different
   from AL_SUCCESS in case of memory allocation error. Callback parameters are:
   - BufferNumber: number of frame buffers required to decode the stream. User
                   must ensure that at least as many buffers are pushed in
                   the decoder.
   - BufferSize: minimum size required for one frame buffer
   - pSettings: Info on the decoded stream
   - pCropInfo: Area that will have to be cropped from the decoded frames
   - pUserParam: user context provided in the callback structure
*****************************************************************************/
typedef struct
{
  AL_ERR (* func)(int BufferNumber, int BufferSize, AL_TStreamSettings const* pSettings, AL_TCropInfo const* pCropInfo, void* pUserParam);
  void* userParam;
}AL_CB_ResolutionFound;

/*************************************************************************//*!
   \brief Parsed SEI callback definition.
   It is called when a SEI is parsed. See Annex D.3 of ITU-T for the
   sei_payload syntax. Callback parameters are:
   - bIsPrefix: true if prefix SEI, false otherwise
   - iPayloadType: type of the payload
   - pPayload: payload data, for which antiemulation has already been removed
               by the decoder. This value can be NULL in case decoder failed to
               allocate memory for sei payload.
   - iPayloadSize: size of the payload data. This value can be zero in case
                   decoder failed to allocate memory for sei payload.
   - pUserParam: user context provided in the callback structure
*****************************************************************************/
typedef struct
{
  void (* func)(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, void* pUserParam);
  void* userParam;
}AL_CB_ParsedSei;

/*************************************************************************//*!
   \brief Decoding error callback definition.
   It is called when an error occurs during decoding. User might decide to
   continue decoding or destroy the decoder depending on error.
   - eError: error type
   - pUserParam: user context provided in the callback structure
*****************************************************************************/
typedef struct
{
  void (* func)(AL_ERR eError, void* pUserParam);
  void* userParam;
}AL_CB_Error;
