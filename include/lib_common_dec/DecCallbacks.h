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

#pragma once

#include "lib_common_dec/DecInfo.h"
#include "lib_common/BufferAPI.h"

/*************************************************************************//*!
   \brief Parsing callback definition.
   It is called every time an input buffer as been parsed by the hardware
   If eInputMode is AL_DEC_UNSPLIT_INPUT, pParsedFrame should not have AL_THandleMetaData
   The iParsingID corresponds to the id in the AL_HandleMetaData associated with
   the buffer.
*****************************************************************************/
typedef struct
{
  void (* func)(AL_TBuffer* pParsedFrame, void* pUserParam, int iParsingID);
  void* userParam;
}AL_CB_EndParsing;

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
