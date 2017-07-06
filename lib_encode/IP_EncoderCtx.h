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
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_encode/lib_encoder.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_bitstream/lib_bitstream.h"
#include "IP_Stream.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/PictureInfo.h"
#include "lib_common_enc/EncSliceStatus.h"
#include "lib_common_enc/EncSliceBuffer.h"
#include "lib_encode/lib_encoder.h"


typedef struct t_Scheduler TScheduler;

#if ENABLE_RTOS_SYNC
#define AL_IP_ENCODER_BEGIN_MUTEX(pCtx) Rtos_GetMutex(pCtx->m_Mutex)
#define AL_IP_ENCODER_END_MUTEX(pCtx) Rtos_ReleaseMutex(pCtx->m_Mutex)
#define AL_IP_ENCODER_GET_SEM(pCtx, uTimeOut) Rtos_GetSemaphore(pCtx->m_Semaphore, uTimeOut)
#define AL_IP_ENCODER_RELEASE_SEM(pCtx) Rtos_ReleaseSemaphore(pCtx->m_Semaphore)
#else
#define AL_IP_ENCODER_BEGIN_MUTEX(pCtx)
#define AL_IP_ENCODER_END_MUTEX(pCtx)
#define AL_IP_ENCODER_GET_SEM(pCtx)
#define AL_IP_ENCODER_RELEASE_SEM(pCtx)
#endif

/****************************************************************************/
#define ENC_MAX_HEADER_SIZE (2 * 1024)
#define MAX_PPS_SIZE (1 * 1024)
#define ENC_MAX_REF_DEFAULT 3
#define ENC_MAX_REF_CUSTOM 5
#define ENC_MAX_SRC 2
#define ENC_MAX_VIEW 1
#define MAX_NUM_B 8

/*************************************************************************//*!
   \brief Frame encoding info structure
*****************************************************************************/
typedef struct AL_t_FrameInfo
{
  AL_TEncInfo tEncInfo;
  AL_TEncRequestInfo tRequestInfo;
  uint16_t uFirstSecID;
  AL_TEncPicBufAddrs tBufAddrs;
  AL_TBuffer* pQpTable;
}AL_TFrameInfo;


typedef AL_TEncSliceStatus TStreamInfo;

/*************************************************************************//*!
   \brief Encoder Context structure
*****************************************************************************/
struct AL_t_EncCtx
{
  AL_TEncSettings m_Settings;

  AL_HANDLE m_hChannel;

  AL_THevcVps m_VPS;
  union
  {
    struct
    {
      AL_THevcSps m_HevcSPS;
      AL_THevcPps m_HevcPPS;
    };
    struct
    {
      AL_TAvcSps m_AvcSPS;
      AL_TAvcPps m_AvcPPS;
    };
  };
  TStream m_Stream;
  TStreamInfo m_StreamInfo;

  int32_t m_uAltRefRecPOC;
  uint8_t m_uAltRefRecID;
  uint8_t m_uLastRecID;
  int m_iLastIdrId;

  uint32_t m_uInitialCpbRemovalDelay;
  uint32_t m_uCpbRemovalDelay;

  int m_iMaxNumRef;

  AL_TSkippedPicture m_pSkippedPicture;

  int m_iFrameCountSent;
  int m_iFrameCountDone;
  AL_ERR m_eError;
  int m_iNumLCU;
  int m_iMinQP;
  int m_iMaxQP;

  uint8_t* m_pHdrBuf;
  AL_TBitStreamLite m_BS;
  AL_TRbspEncoding m_RE;

  uint32_t uNumGroup;

  TBufferEP m_tBufEP1;

  AL_TFrameInfo m_Pool[ENC_MAX_CMD];
  int m_iCurPool;


#if ENABLE_RTOS_SYNC
  AL_MUTEX m_Mutex;
  AL_SEMAPHORE m_Semaphore;
#endif

  TScheduler* m_pScheduler;

  AL_CB_EndEncoding m_callback;

};

typedef struct AL_t_EncCtx AL_TEncCtx;

/*@}*/

