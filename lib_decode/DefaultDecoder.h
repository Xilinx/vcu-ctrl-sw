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
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_decode/lib_decode.h"
#include "I_DecoderCtx.h"
#include "I_Decoder.h"

typedef struct
{
  const AL_TDecoderVtable* vtable;
  AL_TDecCtx ctx;
}AL_TDefaultDecoder;

AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*************************************************************************//*!
   \brief This function performs the decoding of one AU
   \param[in] pAbsDec decoder handle
   \param[in] pBufStream circular buffer containing input bitstream to decode
   \param[out] pEndOfFrame signal if we encountered an end of frame
   \return If the function returns true (true)
         Otherwise (false)
*****************************************************************************/
AL_ERR AL_Default_Decoder_TryDecodeOneAU(AL_TDecoder* pAbsDec, TCircBuffer* pBufStream, bool* pEndOfFrame);

/*************************************************************************//*!
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current frame decoded status
*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus* pStatus);

/*************************************************************************//*!
   \brief This function allocate memory blocks usable by the decoder
   \param[in]  pCtx decoder context
   \param[out] pMD  Pointer to TMemDesc structure that receives allocated
                  memory informations
   \param[in] uSize Number of bytes to allocate
   \param[in] name name of the buffer for debug purpose
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Default_Decoder_Alloc(AL_TDecCtx* pCtx, TMemDesc* pMD, uint32_t uSize, char const* name);

/*************************************************************************//*!
   \brief This function allocate comp memory blocks used by the decoder
   \param[in] pCtx decoder context
   \param[in] iWPSize Size of the weighted pred buffer
   \param[in] iSPSize Size of the slice param buffer
   \param[in] iCompDataSize Size of the comp data buffer
   \param[in] iCompMapSize Size of the comp map buffer
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Default_Decoder_AllocPool(AL_TDecCtx* pCtx, int iWPSize, int iSPSize, int iCompDataSize, int iCompMapSize);

/*************************************************************************//*!
   \brief This function allocate comp memory blocks used by the decoder
   \param[in] pCtx decoder context
   \param[in] iMVSize Size of the motion vector data buffer
   \param[in] iPOCSize Size of the poc buffer
   \param[in] iNum Number of buffers
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Default_Decoder_AllocMv(AL_TDecCtx* pCtx, int iMVSize, int iPOCSize, int iNum);

/*************************************************************************//*!
   \brief This function allocate comp memory blocks used by the decoder
   \param[in] pCtx decoder context
   \param[in] eError Error to set
*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError);

/*@}*/

