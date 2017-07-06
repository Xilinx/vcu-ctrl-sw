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

#include "lib_decode/lib_decode.h"
#include "BufferFeeder.h"
#include "I_Decoder.h"

AL_HDecoder AL_CreateDefaultDecoder(AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*****************************************************************************/
AL_HDecoder AL_Decoder_Create(AL_TIDecChannel* pDecChannel, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  return AL_CreateDefaultDecoder(pDecChannel, pAllocator, pSettings, pCB);
}

/*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnDecoderDestroy(pDec);
}

/*****************************************************************************/
void AL_Decoder_SetParam(AL_HDecoder hDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnSetParam(pDec, bConceal, bUseBoard, iFrmID, iNumFrm);
}

/*****************************************************************************/
AL_ERR AL_Decoder_TryDecodeOneAU(AL_HDecoder hDec, TCircBuffer* pBufStream)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->vtable->pfnTryDecodeOneAU(pDec, pBufStream);
}

/*****************************************************************************/
bool AL_Decoder_PushBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize, AL_EBufMode eMode)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->vtable->pfnPushBuffer(pDec, pBuf, uSize, eMode);
}

/*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnFlush(pDec);
}

/*****************************************************************************/
void AL_Decoder_ReleaseDecPict(AL_HDecoder hDec, AL_TBuffer* pDecPict)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnReleaseDecPict(pDec, pDecPict);
}

void AL_Decoder_PutDecPict(AL_HDecoder hDec, AL_TBuffer* pDecPict)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnPutDecPict(pDec, pDecPict);
}

/*************************************************************************/
int AL_Decoder_GetStrOffset(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->vtable->pfnGetStrOffset(pDec);
}

/*************************************************************************/
int AL_Decoder_GetMaxBD(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->vtable->pfnGetMaxBD(pDec);
}

/*************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return pDec->vtable->pfnGetLastError(pDec);
}

/*****************************************************************************/
void AL_Decoder_InternalFlush(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  pDec->vtable->pfnInternalFlush(pDec);
}

