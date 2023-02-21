// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

/****************************************************************************/
typedef struct AL_i_EncArchVtable AL_IEncArchVtable;

typedef struct AL_i_EncArch
{
  const AL_IEncArchVtable* vtable;
}AL_IEncArch;

typedef struct AL_i_EncArchVtable
{
  void (* Deinit)(void);
  AL_ERR (* EncoderCreate)(AL_HEncoder* hEnc, AL_IEncScheduler* pScheduler, AL_TAllocator* pAlloc, AL_TEncSettings const* pSettings, AL_CB_EndEncoding callback);
  void (* EncoderDestroy)(AL_HEncoder hEnc);
  bool (* EncoderGetInfo)(AL_HEncoder hEnc, AL_TEncoderInfo* pEncInfo);
  void (* EncoderNotifySceneChange)(AL_HEncoder hEnc, int iAhead);
  void (* EncoderNotifyIsLongTerm)(AL_HEncoder hEnc);
  void (* EncoderNotifyUseLongTerm)(AL_HEncoder hEnc);
  bool (* EncoderGetRecPicture)(AL_HEncoder hEnc, AL_TRecPic* pRecPic);
  void (* EncoderReleaseRecPicture)(AL_HEncoder hEnc, AL_TRecPic* pRecPic);
  bool (* EncoderPutStreamBuffer)(AL_HEncoder hEnc, AL_TBuffer* pStream);
  bool (* EncoderProcess)(AL_HEncoder hEnc, AL_TBuffer* pFrame, AL_TBuffer* pQpTable);
  int (* EncoderAddSei)(AL_HEncoder hEnc, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId);
  AL_ERR (* EncoderGetLastError)(AL_HEncoder hEnc);
  bool (* EncoderSetCostMode)(AL_HEncoder hEnc, bool costMode);
  bool (* EncoderSetMaxPictureSize)(AL_HEncoder hEnc, uint32_t uMaxPictureSize);
  bool (* EncoderSetMaxPictureSizePerFrameType)(AL_HEncoder hEnc, uint32_t uMaxPictureSize, AL_ESliceType sliceType);
  bool (* EncoderRestartGop)(AL_HEncoder hEnc);
  bool (* EncoderRestartGopRecoveryPoint)(AL_HEncoder hEnc);
  bool (* EncoderSetGopLength)(AL_HEncoder hEnc, int iGopLength);
  bool (* EncoderSetGopNumB)(AL_HEncoder hEnc, int iNumB);
  bool (* EncoderSetFreqIDR)(AL_HEncoder hEnc, int iFreqIDR);
  bool (* EncoderSetBitRate)(AL_HEncoder hEnc, int iBitRate);
  bool (* EncoderSetMaxBitRate)(AL_HEncoder hEnc, int iTargetBitRate, int iMaxBitRate);
  bool (* EncoderSetFrameRate)(AL_HEncoder hEnc, uint16_t uFrameRate, uint16_t uClkRatio);
  bool (* EncoderSetQP)(AL_HEncoder hEnc, int16_t iQP);
  bool (* EncoderSetQPOffset)(AL_HEncoder hEnc, int16_t iQPOffset);
  bool (* EncoderSetQPBounds)(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP);
  bool (* EncoderSetQPBoundsPerFrameType)(AL_HEncoder hEnc, int16_t iMinQP, int16_t iMaxQP, AL_ESliceType sliceType);
  bool (* EncoderSetQPIPDelta)(AL_HEncoder hEnc, int16_t uIPDelta);
  bool (* EncoderSetQPPBDelta)(AL_HEncoder hEnc, int16_t uPBDelta);
  bool (* EncoderSetInputResolution)(AL_HEncoder hEnc, AL_TDimension tDim);
  bool (* EncoderSetLoopFilterBetaOffset)(AL_HEncoder hEnc, int8_t iBetaOffset);
  bool (* EncoderSetLoopFilterTcOffset)(AL_HEncoder hEnc, int8_t iTcOffset);
  bool (* EncoderSetQPChromaOffsets)(AL_HEncoder hEnc, int8_t iQp1Offset, int8_t iQp2Offset);
  bool (* EncoderSetAutoQP)(AL_HEncoder hEnc, bool useAutoQP);
  bool (* EncoderSetHDRSEIs)(AL_HEncoder hEnc, AL_THDRSEIs* pHDRSEIs);
}AL_IEncArchVtable;

/*@}*/
