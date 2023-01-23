/******************************************************************************
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/
#pragma once

#include "Sections.h"
#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "SourceBufferChecker.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/EncSliceStatus.h"
#include "lib_common_enc/EncSliceBuffer.h"
#include "lib_common_enc/EncSize.h"
#include "lib_common/Fifo.h"
#include "lib_encode/EncUtils.h"

typedef struct AL_i_EncScheduler AL_IEncScheduler;

/*************************************************************************//*!
   \brief Structure containing infos for non-vcl nals generation
*****************************************************************************/
typedef struct AL_t_HLSInfo
{
  uint8_t uSpsId;
  uint8_t uPpsId;
  bool bLFOffsetChanged;
  int8_t iLFBetaOffset;
  int8_t iLFTcOffset;
  int8_t iCbPicQpOffset;
  int8_t iCrPicQpOffset;
  int16_t uFrameRate;
  int16_t uClkRatio;
  bool bHDRChanged;
  int8_t iHDRID;
}AL_HLSInfo;

/*************************************************************************//*!
   \brief Frame encoding info structure
*****************************************************************************/
typedef struct AL_t_FrameInfo
{
  AL_TEncInfo tEncInfo;
  AL_TBuffer* pQpTable;
  AL_HLSInfo tHLSUpdateInfo;
}AL_TFrameInfo;

typedef AL_TEncSliceStatus TStreamInfo;

typedef struct AL_t_EncCtx AL_TEncCtx;

typedef struct
{
  bool (* shouldReleaseSource)(AL_TEncPicStatus* pPicStatus);
  void (* preprocessEp1)(AL_TEncCtx* pCtx, TBufferEP* pEP1);
  void (* configureChannel)(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TEncSettings const* pSettings);
  void (* generateNals)(AL_TEncCtx* pCtx, int iLayerID, bool bWriteVps);
  void (* updateHlsAndWriteSections)(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream, int iLayerID, int iPicID);
}HighLevelEncoder;

typedef struct
{
  AL_TEncCtx* pCtx;
  int iLayerID;
}AL_TCbUserParam;

typedef struct
{
  AL_HANDLE hChannel;

  AL_TSps sps;
  AL_TPps pps;
  AL_TAud aud;

  AL_TSrcBufferChecker srcBufferChecker;
  AL_TEncRequestInfo currentRequestInfo;
  TBufferEP tBufEP1;

  int iCurStreamSent;
  int iCurStreamRecv;
  AL_TBuffer* StreamSent[AL_MAX_STREAM_BUFFER];

  TMemDesc tMDChParam;

  AL_TCbUserParam callback_user_param;
  AL_CB_EndEncoding tEndEncodingCallback;
}AL_TLayerCtx;

typedef struct
{
  AL_TFrameInfo* pFI;
  AL_TBuffer* pSrc;
}AL_TFrameCtx;

/*************************************************************************//*!
   \brief Pool of FrameInfos
*****************************************************************************/
#define INVALID_POOL_ID -1
typedef struct AL_t_IDPool
{
  AL_TFifo tFreeIDs;
  int iCurID;
}AL_TIDPool;

typedef struct AL_t_FrameInfoPool
{
  /* O(1) access to a frame info */
  AL_TIDPool tIDPool;
  AL_TFrameInfo FrameInfos[MAX_NUM_LAYER * ENC_MAX_CMD];
}AL_TFrameInfoPool;

/*************************************************************************//*!
   \brief Pool of HDR SEIs
*****************************************************************************/
typedef struct AL_t_HDRPool
{
  AL_TIDPool tIDPool;
  AL_THDRSEIs HDRSEIs[ENC_MAX_CMD];
  uint8_t uRefCount[ENC_MAX_CMD];
  bool bHDRChanged;
}AL_THDRPool;

/*************************************************************************//*!
   \brief Encoder Context structure
*****************************************************************************/
typedef struct AL_t_EncCtx
{
  HighLevelEncoder encoder;

  AL_TEncSettings* pSettings;

  AL_TLayerCtx tLayerCtx[MAX_NUM_LAYER];

  AL_THeadersCtx tHeadersCtx[MAX_NUM_LAYER];
  AL_TVps vps;
  bool bEndOfStreamReceived[MAX_NUM_LAYER];

  int initialCpbRemovalDelay;
  int cpbRemovalDelay;

  int iMaxNumRef;

  int iFrameCountDone;
  AL_ERR eError;

  AL_TFrameInfoPool tFrameInfoPool;

  /* O(n) as you need to search for the source inside */
  AL_TFrameCtx SourceSent[AL_MAX_SOURCE_BUFFER];

  AL_THDRPool tHDRPool;

  AL_MUTEX Mutex;

  AL_SEMAPHORE PendingEncodings; // tracks the count of jobs sent to the scheduler

  AL_IEncScheduler* pScheduler;

  int iInitialNumB;
  uint16_t uInitialFrameRate;

  TMemDesc tMDSettings;
}AL_TEncCtx;

AL_HLSInfo* AL_GetHLSInfo(AL_TEncCtx* pCtx, int iPicID);
AL_TNalsData AL_ExtractNalsData(AL_TEncCtx* pCtx, int iLayerID, int iPicID);

/*@}*/

