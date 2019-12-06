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

#include "Encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_encode/I_EncScheduler.h"
#include "lib_encode/Com_Encoder.h"
#include "lib_encode/lib_encoder.h"
#include "IP_EncoderCtx.h"

void AL_CreateHevcEncoder(HighLevelEncoder* pCtx);
void AL_CreateAvcEncoder(HighLevelEncoder* pCtx);

/****************************************************************************/
AL_ERR AL_Encoder_Create(AL_HEncoder* hEnc, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback)
{
  if(!pSettings)
    return AL_ERROR;

  AL_TEncoder* pEncoder = AL_Common_Encoder_Create(pAlloc);
  *hEnc = (AL_HEncoder)pEncoder;

  if(!pEncoder)
    return AL_ERR_NO_MEMORY;

  AL_TEncCtx* pCtx = pEncoder->pCtx;

  if(AL_IS_HEVC(pSettings->tChParam[0].eProfile))
    AL_CreateHevcEncoder(&pCtx->encoder);

  if(AL_IS_AVC(pSettings->tChParam[0].eProfile))
    AL_CreateAvcEncoder(&pCtx->encoder);

  AL_ERR errorCode = AL_Common_Encoder_CreateChannel(pEncoder, pScheduler, pAlloc, pSettings);

  if(AL_IS_ERROR_CODE(errorCode))
    goto fail;

  if(callback.func)
    pCtx->tLayerCtx[0].callback = callback;

  return errorCode;

  fail:
  AL_Common_Encoder_Destroy(pEncoder);
  *hEnc = NULL;
  return errorCode;
}

/****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_Destroy(pEnc);
}

/****************************************************************************/
void AL_Encoder_NotifySceneChange(AL_HEncoder hEnc, int iAhead)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifySceneChange(pEnc, iAhead);
}

/****************************************************************************/
void AL_Encoder_NotifyIsLongTerm(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifyIsLongTerm(pEnc);
}

/****************************************************************************/
void AL_Encoder_NotifyUseLongTerm(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifyUseLongTerm(pEnc);
}

/****************************************************************************/
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  return AL_Common_Encoder_GetRecPicture(pEnc, pRecPic, 0);
}

/****************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_ReleaseRecPicture(pEnc, pRecPic, 0);
}

/****************************************************************************/
bool AL_Encoder_PutStreamBuffer(AL_HEncoder hEnc, AL_TBuffer* pStream)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_PutStreamBuffer(pEnc, pStream, 0);
}

/****************************************************************************/
bool AL_Encoder_Process(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_Process(pEnc, pFrame, pQpTable, 0);
}

/****************************************************************************/
AL_ERR AL_Encoder_GetLastError(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_GetLastError(pEnc);
}

/****************************************************************************/
bool AL_Encoder_RestartGop(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_RestartGop(pEnc);
}

/****************************************************************************/
bool AL_Encoder_SetGopLength(AL_HEncoder hEnc, int iGopLength)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetGopLength(pEnc, iGopLength);
}

/****************************************************************************/
bool AL_Encoder_SetGopNumB(AL_HEncoder hEnc, int iNumB)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetGopNumB(pEnc, iNumB);
}

/****************************************************************************/
bool AL_Encoder_SetFreqIDR(AL_HEncoder hEnc, int iFreqIDR)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetFreqIDR(pEnc, iFreqIDR);
}

/****************************************************************************/
bool AL_Encoder_SetBitRate(AL_HEncoder hEnc, int iBitRate)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetBitRate(pEnc, iBitRate, 0);
}

/****************************************************************************/
bool AL_Encoder_SetFrameRate(AL_HEncoder hEnc, uint16_t uFrameRate, uint16_t uClkRatio)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetFrameRate(pEnc, uFrameRate, uClkRatio);
}

/****************************************************************************/
bool AL_Encoder_SetQP(AL_HEncoder hEnc, int16_t iQP)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQP(pEnc, iQP);
}

/****************************************************************************/
bool AL_Encoder_SetQPBounds(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPBounds(pEnc, iMinQP, iMaxQP);
}

/****************************************************************************/
bool AL_Encoder_SetQPIPDelta(AL_HEncoder hEnc, int16_t uIPDelta)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPIPDelta(pEnc, uIPDelta);
}

/****************************************************************************/
bool AL_Encoder_SetQPPBDelta(AL_HEncoder hEnc, int16_t uPBDelta)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetQPPBDelta(pEnc, uPBDelta);
}

/****************************************************************************/
bool AL_Encoder_SetInputResolution(AL_HEncoder hEnc, AL_TDimension tDim)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetInputResolution(pEnc, tDim);
}

/****************************************************************************/
bool AL_Encoder_SetLoopFilterBetaOffset(AL_HEncoder hEnc, int8_t iBetaOffset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetLoopFilterBetaOffset(pEnc, iBetaOffset);
}

/****************************************************************************/
bool AL_Encoder_SetLoopFilterTcOffset(AL_HEncoder hEnc, int8_t iTcOffset)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetLoopFilterTcOffset(pEnc, iTcOffset);
}

/****************************************************************************/
bool AL_Encoder_SetHDRSEIs(AL_HEncoder hEnc, AL_THDRSEIs* pHDRSEIs)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_SetHDRSEIs(pEnc, pHDRSEIs);
}

