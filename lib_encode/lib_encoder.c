/******************************************************************************
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

#include "lib_encode/lib_encoder.h"
#include "lib_encode/LibEncoderHost.h"

/* Initialize with encHost so AL_Lib_Encoder_Init() call is optionnal for the
 * time being.
 */
static AL_IEncArch* pArch = &encHost;

/*****************************************************************************/
AL_ERR AL_Lib_Encoder_Init(AL_ELibEncoderArch eArch)
{
  switch(eArch)
  {
  case AL_LIB_ENCODER_ARCH_HOST:
    pArch = LibEncodeHostGet();
    return AL_SUCCESS;
  default:
    return AL_ERROR;
  }
}

/*****************************************************************************/
void AL_Lib_Encoder_DeInit()
{
  if(pArch)
    pArch->vtable->Deinit();
  pArch = NULL;
}

/*****************************************************************************/
AL_ERR AL_Encoder_Create(AL_HEncoder* hEnc, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->EncoderCreate(hEnc, pScheduler, pAlloc, pSettings, callback);
}

/*****************************************************************************/
void AL_Encoder_Destroy(AL_HEncoder hEnc)
{
  if(!pArch)
    return;

  pArch->vtable->EncoderDestroy(hEnc);
}

/*****************************************************************************/
bool AL_Encoder_GetInfo(AL_HEncoder hEnc, AL_TEncoderInfo* pEncInfo)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderGetInfo(hEnc, pEncInfo);
}

/*****************************************************************************/
void AL_Encoder_NotifySceneChange(AL_HEncoder hEnc, int iAhead)
{
  if(!pArch)
    return;

  pArch->vtable->EncoderNotifySceneChange(hEnc, iAhead);
}

/*****************************************************************************/
void AL_Encoder_NotifyIsLongTerm(AL_HEncoder hEnc)
{
  if(!pArch)
    return;

  pArch->vtable->EncoderNotifyIsLongTerm(hEnc);
}

/*****************************************************************************/
void AL_Encoder_NotifyUseLongTerm(AL_HEncoder hEnc)
{
  if(!pArch)
    return;

  pArch->vtable->EncoderNotifyUseLongTerm(hEnc);
}

/*****************************************************************************/
bool AL_Encoder_GetRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderGetRecPicture(hEnc, pRecPic);
}

/*****************************************************************************/
void AL_Encoder_ReleaseRecPicture(AL_HEncoder hEnc, AL_TRecPic* pRecPic)
{
  if(!pArch)
    return;

  pArch->vtable->EncoderReleaseRecPicture(hEnc, pRecPic);
}

/*****************************************************************************/
bool AL_Encoder_PutStreamBuffer(AL_HEncoder hEnc, AL_TBuffer* pStream)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderPutStreamBuffer(hEnc, pStream);
}

/*****************************************************************************/
bool AL_Encoder_Process(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderProcess(hEnc, pFrame, pQpTable);
}

/*****************************************************************************/
int AL_Encoder_AddSei(AL_HEncoder hEnc, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->EncoderAddSei(hEnc, pStream, isPrefix, iPayloadType, pPayload, iPayloadSize, iTempId);
}

/*****************************************************************************/
AL_ERR AL_Encoder_GetLastError(AL_HEncoder hEnc)
{
  if(!pArch)
    return AL_ERROR;

  return pArch->vtable->EncoderGetLastError(hEnc);
}

/*****************************************************************************/
bool AL_Encoder_SetCostMode(AL_HEncoder hEnc, bool costMode)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetCostMode(hEnc, costMode);
}

/*****************************************************************************/
bool AL_Encoder_SetMaxPictureSize(AL_HEncoder hEnc, uint32_t uMaxPictureSize)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetMaxPictureSize(hEnc, uMaxPictureSize);
}

/*****************************************************************************/
bool AL_Encoder_SetMaxPictureSizePerFrameType(AL_HEncoder hEnc, uint32_t uMaxPictureSize, AL_ESliceType sliceType)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetMaxPictureSizePerFrameType(hEnc, uMaxPictureSize, sliceType);
}

/*****************************************************************************/
bool AL_Encoder_RestartGop(AL_HEncoder hEnc)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderRestartGop(hEnc);
}

/*****************************************************************************/
bool AL_Encoder_RestartGopRecoveryPoint(AL_HEncoder hEnc)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderRestartGopRecoveryPoint(hEnc);
}

/*****************************************************************************/
bool AL_Encoder_SetGopLength(AL_HEncoder hEnc, int iGopLength)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetGopLength(hEnc, iGopLength);
}

/*****************************************************************************/
bool AL_Encoder_SetGopNumB(AL_HEncoder hEnc, int iNumB)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetGopNumB(hEnc, iNumB);
}

/*****************************************************************************/
bool AL_Encoder_SetFreqIDR(AL_HEncoder hEnc, int iFreqIDR)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetFreqIDR(hEnc, iFreqIDR);
}

/*****************************************************************************/
bool AL_Encoder_SetBitRate(AL_HEncoder hEnc, int iBitRate)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetBitRate(hEnc, iBitRate);
}

/*****************************************************************************/
bool AL_Encoder_SetMaxBitRate(AL_HEncoder hEnc, int iTargetBitRate, int iMaxBitRate)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetMaxBitRate(hEnc, iTargetBitRate, iMaxBitRate);
}

/*****************************************************************************/
bool AL_Encoder_SetFrameRate(AL_HEncoder hEnc, uint16_t uFrameRate, uint16_t uClkRatio)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetFrameRate(hEnc, uFrameRate, uClkRatio);
}

/*****************************************************************************/
bool AL_Encoder_SetQP(AL_HEncoder hEnc, int16_t iQP)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQP(hEnc, iQP);
}

/*****************************************************************************/
bool AL_Encoder_SetQPOffset(AL_HEncoder hEnc, int16_t iQPOffset)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPOffset(hEnc, iQPOffset);
}

/*****************************************************************************/
bool AL_Encoder_SetQPBounds(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPBounds(hEnc, iMinQP, iMaxQP);
}

/*****************************************************************************/
bool AL_Encoder_SetQPBoundsPerFrameType(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPBoundsPerFrameType(hEnc, iMinQP, iMaxQP, sliceType);
}

/*****************************************************************************/
bool AL_Encoder_SetQPIPDelta(AL_HEncoder hEnc, int16_t uIPDelta)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPIPDelta(hEnc, uIPDelta);
}

/*****************************************************************************/
bool AL_Encoder_SetQPPBDelta(AL_HEncoder hEnc, int16_t uPBDelta)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPPBDelta(hEnc, uPBDelta);
}

/*****************************************************************************/
bool AL_Encoder_SetInputResolution(AL_HEncoder hEnc, AL_TDimension tDim)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetInputResolution(hEnc, tDim);
}

/*****************************************************************************/
bool AL_Encoder_SetLoopFilterBetaOffset(AL_HEncoder hEnc, int8_t iBetaOffset)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetLoopFilterBetaOffset(hEnc, iBetaOffset);
}

/*****************************************************************************/
bool AL_Encoder_SetLoopFilterTcOffset(AL_HEncoder hEnc, int8_t iTcOffset)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetLoopFilterTcOffset(hEnc, iTcOffset);
}

/*****************************************************************************/
bool AL_Encoder_SetQPChromaOffsets(AL_HEncoder hEnc, int8_t iQp1Offset, int8_t iQp2Offset)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetQPChromaOffsets(hEnc, iQp1Offset, iQp2Offset);
}

/*****************************************************************************/
bool AL_Encoder_SetAutoQP(AL_HEncoder hEnc, bool useAutoQP)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetAutoQP(hEnc, useAutoQP);
}

/*****************************************************************************/
bool AL_Encoder_SetHDRSEIs(AL_HEncoder hEnc, AL_THDRSEIs* pHDRSEIs)
{
  if(!pArch)
    return false;

  return pArch->vtable->EncoderSetHDRSEIs(hEnc, pHDRSEIs);
}

