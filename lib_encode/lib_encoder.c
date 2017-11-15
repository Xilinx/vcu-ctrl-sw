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

#include "Encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_encode/IScheduler.h"
#include "lib_encode/Com_Encoder.h"
#include "lib_encode/lib_encoder.h"
#include "IP_EncoderCtx.h"

/****************************************************************************/

AL_ERR AL_HEVC_Encoder_Create(AL_TEncCtx** hCtx, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings);
AL_ERR AL_AVC_Encoder_Create(AL_TEncCtx** hCtx, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings);

/****************************************************************************/
AL_ERR AL_Encoder_Create(AL_HEncoder* hEnc, TScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback)
{
  *hEnc = (AL_HEncoder)Rtos_Malloc(sizeof(AL_TEncoder));
  AL_TEncoder* pEncoder = (AL_TEncoder*)*hEnc;
  AL_ERR errorCode = AL_ERROR;

  if(!pEncoder)
    goto fail;

  Rtos_Memset(pEncoder, 0, sizeof(AL_TEncoder));


  if(AL_IS_HEVC(pSettings->tChParam.eProfile))
    errorCode = AL_HEVC_Encoder_Create(&pEncoder->pCtx, pScheduler, pAlloc, pSettings);

  if(AL_IS_AVC(pSettings->tChParam.eProfile))
    errorCode = AL_AVC_Encoder_Create(&pEncoder->pCtx, pScheduler, pAlloc, pSettings);

  if(!pEncoder->pCtx)
    goto fail;

  if(callback.func)
    pEncoder->pCtx->m_callback = callback;

  return AL_SUCCESS;

  fail:
  Rtos_Free(pEncoder);
  *hEnc = NULL;
  return errorCode;
}

/****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  AL_Common_Encoder_Destroy(pEnc);

  Rtos_Free(pEnc);
}

/****************************************************************************/
void AL_Encoder_NotifySceneChange(AL_HEncoder hEnc, int iAhead)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifySceneChange(pEnc, iAhead);
}

/****************************************************************************/
void AL_Encoder_NotifyLongTerm(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_NotifyLongTerm(pEnc);
}

/****************************************************************************/
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;

  return AL_Common_Encoder_GetRecPicture(pEnc, pRecPic);
}

/****************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, TRecPic* pRecPic)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  AL_Common_Encoder_ReleaseRecPicture(pEnc, pRecPic);
}

/****************************************************************************/
bool AL_Encoder_PutStreamBuffer(AL_HEncoder hEnc, AL_TBuffer* pStream)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_PutStreamBuffer(pEnc, pStream);
}

/****************************************************************************/
bool AL_Encoder_Process(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_Process(pEnc, pFrame, pQpTable);
}

/****************************************************************************/
AL_ERR AL_Encoder_GetLastError(AL_HEncoder hEnc)
{
  AL_TEncoder* pEnc = (AL_TEncoder*)hEnc;
  return AL_Common_Encoder_GetLastError(pEnc);
}


