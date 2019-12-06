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

#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_decode/DefaultDecoder.h"

AL_ERR CreateAvcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);
AL_ERR CreateHevcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB);

/*****************************************************************************/
AL_ERR AL_Decoder_Create(AL_HDecoder* hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  if(!pSettings || !pCB || !pAllocator || !pScheduler || !hDec)
    return AL_ERROR;

  if(pSettings->eCodec == AL_CODEC_AVC)
    return CreateAvcDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  if(pSettings->eCodec == AL_CODEC_HEVC)
    return CreateHevcDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  return AL_ERROR;
}

/*****************************************************************************/
void AL_Decoder_Destroy(AL_HDecoder hDec)
{
  AL_Default_Decoder_Destroy((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_SetParam(AL_HDecoder hDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool shouldPrintFrameDelimiter)
{
  AL_Default_Decoder_SetParam((AL_TDecoder*)hDec, bConceal, bUseBoard, iFrmID, iNumFrm, bForceCleanBuffers, shouldPrintFrameDelimiter);
}

/*****************************************************************************/
bool AL_Decoder_PushBuffer(AL_HDecoder hDec, AL_TBuffer* pBuf, size_t uSize)
{
  return AL_Default_Decoder_PushBuffer((AL_TDecoder*)hDec, pBuf, uSize);
}

/*****************************************************************************/
void AL_Decoder_Flush(AL_HDecoder hDec)
{
  AL_Default_Decoder_Flush((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_PutDisplayPicture(AL_HDecoder hDec, AL_TBuffer* pDisplay)
{
  AL_Default_Decoder_PutDecPict((AL_TDecoder*)hDec, pDisplay);
}

/*****************************************************************************/
int AL_Decoder_GetMaxBD(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetMaxBD((AL_TDecoder*)hDec);
}

/*****************************************************************************/
AL_ERR AL_Decoder_GetLastError(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetLastError((AL_TDecoder*)hDec);
}

/*****************************************************************************/
AL_ERR AL_Decoder_GetFrameError(AL_HDecoder hDec, AL_TBuffer* pBuf)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return AL_Default_Decoder_GetFrameError(pDec, pBuf);
}

/*****************************************************************************/
bool AL_Decoder_PreallocateBuffers(AL_HDecoder hDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)hDec;
  return AL_Default_Decoder_PreallocateBuffers(pDec);
}

int32_t RndPitch(int32_t iWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode);
int32_t RndHeight(int32_t iHeight);

uint32_t AL_Decoder_GetMinPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  return RndPitch(uWidth, uBitDepth, eFrameBufferStorageMode);
}

uint32_t AL_Decoder_GetMinStrideHeight(uint32_t uHeight)
{
  return RndHeight(uHeight);
}

uint32_t AL_Decoder_RoundPitch(uint32_t uWidth, uint8_t uBitDepth, AL_EFbStorageMode eFrameBufferStorageMode)
{
  return AL_Decoder_GetMinPitch(uWidth, uBitDepth, eFrameBufferStorageMode);
}

uint32_t AL_Decoder_RoundHeight(uint32_t uHeight)
{
  return AL_Decoder_GetMinStrideHeight(uHeight);
}

/*****************************************************************************/
UNIT_ERROR AL_Decoder_TryDecodeOneUnit(AL_HDecoder hDec, AL_TBuffer* pBufStream)
{
  return AL_Default_Decoder_TryDecodeOneUnit((AL_TDecoder*)hDec, pBufStream);
}

/*****************************************************************************/
int AL_Decoder_GetStrOffset(AL_HDecoder hDec)
{
  return AL_Default_Decoder_GetStrOffset((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_InternalFlush(AL_HDecoder hDec)
{
  AL_Default_Decoder_InternalFlush((AL_TDecoder*)hDec);
}

/*****************************************************************************/
void AL_Decoder_FlushInput(AL_HDecoder hDec)
{
  AL_Default_Decoder_FlushInput((AL_TDecoder*)hDec);
}

