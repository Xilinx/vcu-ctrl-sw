/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "lib_rtos/types.h"

#include "lib_common/MemDesc.h"

#include "lib_common_dec/StartCodeParam.h"
#include "lib_common_dec/DecChanParam.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecPicParam.h"

/****************************************************************************/
typedef struct
{
  void (* func)(void* pUserParam, int iFrameID, int iSliceID);
  void* userParam;
}AL_TDecScheduler_CB_EndParsing;

/****************************************************************************/
typedef struct
{
  void (* func)(void* pUserParam, AL_TDecPicStatus* pPicStatus);
  void* userParam;
}AL_TDecScheduler_CB_EndDecoding;

typedef struct
{
  void (* func)(void* pUserParam, AL_TScStatus* pScdStatus);
  void* userParam;
}AL_TDecScheduler_CB_EndStartCode;

typedef struct
{
  void (* func)(void* pUserParam);
  void* userParam;
}AL_TDecScheduler_CB_DestroyChannel;

/****************************************************************************/
typedef struct AL_i_DecSchedulerVtable AL_IDecSchedulerVtable;

typedef struct AL_i_DecScheduler
{
  const AL_IDecSchedulerVtable* vtable;
}AL_IDecScheduler;

typedef struct AL_i_DecSchedulerVtable
{
  void (* Destroy)(AL_IDecScheduler* pScheduler);

  AL_ERR (* CreateStartCodeChannel)(AL_HANDLE* hStartCodeChannel, AL_IDecScheduler* pScheduler);
  AL_ERR (* CreateChannel)(AL_HANDLE* hChannel, AL_IDecScheduler* pScheduler, TMemDesc* pMDChParams, AL_TDecScheduler_CB_EndParsing endParsingCallback, AL_TDecScheduler_CB_EndDecoding endDecodingCallback);
  AL_ERR (* DestroyStartCodeChannel)(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel);
  AL_ERR (* DestroyChannel)(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel);

  void (* SearchSC)(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufferAddrs, AL_TDecScheduler_CB_EndStartCode callback);
  void (* DecodeOneFrame)(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* pSliceParams);
  void (* DecodeOneSlice)(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* pSliceParams);

}AL_IDecSchedulerVtable;

/*************************************************************************//*!
   \brief De-initializes the scheduler
   \return return true
*****************************************************************************/
void AL_IDecScheduler_Destroy(AL_IDecScheduler* pThis);

/*************************************************************************//*!
   \brief Channel creation
   \param[out] hChannel Handle to the created channel in case of success
   \param[in] pThis Decoder scheduler interface
   \param[in] pMDChParams Pointer to the memory descriptor containing the channel
   parameters
   \param[in] endParsingCallback Callback structure
   \param[in] endDecodingCallback Callback structure
   \return error code explaining why the channel creation failed
   or AL_SUCCESS (See include/lib_common/Error.h)
*****************************************************************************/
static AL_INLINE
AL_ERR AL_IDecScheduler_CreateChannel(AL_HANDLE* hChannel, AL_IDecScheduler* pThis, TMemDesc* pMDChParams, AL_TDecScheduler_CB_EndParsing endParsingCallback, AL_TDecScheduler_CB_EndDecoding endDecodingCallback)
{
  return pThis->vtable->CreateChannel(hChannel, pThis, pMDChParams, endParsingCallback, endDecodingCallback);
}

/*************************************************************************//*!
   \brief Startcode Channel creation
   \param[out] hChannel Handle to the created startcode channel in case of success
   \param[in] pThis Decoder scheduler interface
   \return error code explaining why the startcode channel creation failed
   or AL_SUCCESS (See include/lib_common/Error.h)
*****************************************************************************/
static AL_INLINE
AL_ERR AL_IDecScheduler_CreateStartCodeChannel(AL_HANDLE* hStartCodeChannel, AL_IDecScheduler* pThis)
{
  return pThis->vtable->CreateStartCodeChannel(hStartCodeChannel, pThis);
}

/*************************************************************************//*!
   \brief Starcode Channel destruction
   Destroying a NULL handle returns AL_SUCCESS and does nothing.
   \param[in] pThis Decoder scheduler interface
   \param[in] hChannel Handle to the startcode channel we want to destroy
   \return error code explaining why the startcode channel creation failed
   or AL_SUCCESS (See include/lib_common/Error.h)
*****************************************************************************/
static AL_INLINE
AL_ERR AL_IDecScheduler_DestroyStartCodeChannel(AL_IDecScheduler* pThis, AL_HANDLE hStartCodeChannel)
{
  return pThis->vtable->DestroyStartCodeChannel(pThis, hStartCodeChannel);
}

/*************************************************************************//*!
   \brief Channel destruction
   Destroying a NULL handle returns AL_SUCCESS and does nothing.
   \param[in] pThis Decoder scheduler interface
   \param[in] hChannel Handle to the channel we want to destroy
   \return error code explaining why the startcode channel creation failed
   or AL_SUCCESS (See include/lib_common/Error.h)
*****************************************************************************/
static AL_INLINE
AL_ERR AL_IDecScheduler_DestroyChannel(AL_IDecScheduler* pThis, AL_HANDLE hChannel)
{
  return pThis->vtable->DestroyChannel(pThis, hChannel);
}

/*************************************************************************//*!
   \brief Asks the scheduler to process a start code detection
   \param[in] pThis Decoder scheduler interface
   \param[in] pScParam Pointer to the start code detector parameters
   \param[in] pBufferAddrs Pointer to the start code detectors buffers
   \param[in] callback Start code callback structure
   \return return true
*****************************************************************************/
static AL_INLINE
void AL_IDecScheduler_SearchSC(AL_IDecScheduler* pThis, AL_HANDLE hStartCodeChannel, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufferAddrs, AL_TDecScheduler_CB_EndStartCode callback)
{
  pThis->vtable->SearchSC(pThis, hStartCodeChannel, pScParam, pBufferAddrs, callback);
}

/*************************************************************************//*!
   \brief Asks the scheduler to process a frame decoding
   \param[in] pThis Decoder scheduler interface
   \param[in] pPictParam  Pointer to the picture parameters structure
   \param[in] pSliceParam Pointer to the slice parameters list structure
   \return return true if the decoding launch is successfull
              false otherwise
*****************************************************************************/
static AL_INLINE
void AL_IDecScheduler_DecodeOneFrame(AL_IDecScheduler* pThis, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* pSliceParams)
{
  pThis->vtable->DecodeOneFrame(pThis, hChannel, pPictParam, pPictAddrs, pSliceParams);
}

/*************************************************************************//*!
   \brief Asks the scheduler to process a frame decoding
   \param[in] pThis Decoder scheduler interface
   \param[in] pPictParam  Pointer to the picture parameters structure
   \param[in] pSliceParam Pointer to the slice parameters list structure
   \return return true if the decoding launch is successfull
              false otherwise
*****************************************************************************/
static AL_INLINE
void AL_IDecScheduler_DecodeOneSlice(AL_IDecScheduler* pThis, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* pSliceParams)
{
  pThis->vtable->DecodeOneSlice(pThis, hChannel, pPictParam, pPictAddrs, pSliceParams);
}

/*@}*/
