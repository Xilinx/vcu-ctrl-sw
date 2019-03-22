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
#include "lib_bitstream/lib_bitstream.h"
#include "IP_Stream.h"
#include "SourceBufferChecker.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/PictureInfo.h"
#include "lib_common_enc/EncSliceStatus.h"
#include "lib_common_enc/EncSliceBuffer.h"
#include "lib_common_enc/EncSize.h"
#include "lib_encode/lib_encoder.h"
#include "lib_common/Fifo.h"



#define MAX_NAL_IDS 15

typedef struct t_Scheduler TScheduler;

/*************************************************************************//*!
   \brief Frame encoding info structure
*****************************************************************************/
typedef struct AL_t_FrameInfo
{
  AL_TEncInfo tEncInfo;
  AL_TBuffer* pQpTable;
  bool bResolutionChanged;
  uint8_t uNewNalsId;
}AL_TFrameInfo;


typedef AL_TEncSliceStatus TStreamInfo;

typedef struct AL_t_EncCtx AL_TEncCtx;

typedef struct
{
  bool (* shouldReleaseSource)(AL_TEncPicStatus* pPicStatus);
  void (* preprocessEp1)(AL_TEncCtx* pCtx, TBufferEP* pEP1);
  void (* configureChannel)(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TEncSettings const* pSettings);
  void (* generateNals)(AL_TEncCtx* pCtx, int iLayerID, bool bWriteVps);
  void (* updateHlsAndWriteSections)(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, bool bResolutionChanged, uint8_t uNalID, AL_TBuffer* pStream, int iLayerID);
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

  AL_TSrcBufferChecker srcBufferChecker;
  AL_TEncRequestInfo currentRequestInfo;
  TBufferEP tBufEP1;

  int iCurStreamSent;
  int iCurStreamRecv;
  AL_TBuffer* StreamSent[AL_MAX_STREAM_BUFFER];

  AL_TCbUserParam callback_user_param;
  AL_CB_EndEncoding callback;
}AL_TLayerCtx;

typedef struct
{
  AL_TFrameInfo* pFI;
  AL_TBuffer* pSrc;
}AL_TFrameCtx;

/*************************************************************************//*!
   \brief Encoder Context structure
*****************************************************************************/
typedef struct AL_t_EncCtx
{
  HighLevelEncoder encoder;
  AL_TEncSettings Settings;
  AL_TLayerCtx tLayerCtx[MAX_NUM_LAYER];

  AL_TDimension nalResolutionsPerID[MAX_NAL_IDS];
  AL_THevcVps vps;

  TStreamInfo StreamInfo;
  bool bEndOfStreamReceived[MAX_NUM_LAYER];
  int iLastIdrId;

  AL_SeiData seiData;

  int iMaxNumRef;

  int iFrameCountDone;
  AL_ERR eError;
  int iNumLCU;
  int iMinQP;
  int iMaxQP;

  /* O(1) access to a frame info */
  AL_TFrameInfo Pool[MAX_NUM_LAYER * ENC_MAX_CMD];
  AL_TFifo iPoolIds;
  int iCurPool;

  /* O(n) as you need to search for the source inside */
  AL_TFrameCtx SourceSent[AL_MAX_SOURCE_BUFFER];


  AL_MUTEX Mutex;
  AL_SEMAPHORE PendingEncodings; // tracks the count of jobs sent to the scheduler

  TScheduler* pScheduler;

  int iInitialNumB;
  uint16_t uInitialFrameRate;
}AL_TEncCtx;

NalsData AL_ExtractNalsData(AL_TEncCtx* pCtx, int iLayerID);

/*@}*/

