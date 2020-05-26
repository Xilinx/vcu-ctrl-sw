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

#include "lib_decode/I_DecScheduler.h"
#include "lib_common/IDriver.h"

#if  __linux__

#include <pthread.h>
#include <stdio.h>
#include <string.h> // strerrno, strlen, strcpy
#include <errno.h>
#include "lib_rtos/types.h" // static_assert

#include "allegro_ioctl_mcu_dec.h"
#include "lib_common/List.h"
#include "lib_common/Error.h"
#include "lib_assert/al_assert.h"

#define DCACHE_OFFSET 0x80000000

typedef struct
{
  pthread_cond_t Cond;
  pthread_mutex_t Lock;
}AL_WaitQueue;

static void AL_WakeUp(AL_WaitQueue* pQueue)
{
  pthread_mutex_lock(&pQueue->Lock);
  int ret = pthread_cond_broadcast(&pQueue->Cond);
  AL_Assert(ret == 0);
  pthread_mutex_unlock(&pQueue->Lock);
}

static void AL_WaitEvent(AL_WaitQueue* pQueue, bool (* isReady)(void*), void* pParam)
{
  pthread_mutex_lock(&pQueue->Lock);

  while(!isReady(pParam))
    pthread_cond_wait(&pQueue->Cond, &pQueue->Lock);

  pthread_mutex_unlock(&pQueue->Lock);
}

static void AL_WaitQueue_Init(AL_WaitQueue* pQueue)
{
  pthread_mutex_init(&pQueue->Lock, NULL);
  pthread_cond_init(&pQueue->Cond, NULL);
}

static void AL_WaitQueue_Deinit(AL_WaitQueue* pQueue)
{
  pthread_mutex_destroy(&pQueue->Lock);
  pthread_cond_destroy(&pQueue->Cond);
}

typedef struct
{
  int fd;
  AL_THREAD thread;
  bool bBeingDestroyed;
  AL_TDriver* driver;
  AL_TDecScheduler_CB_EndParsing endParsingCB;
  AL_TDecScheduler_CB_EndDecoding endDecodingCB;
}Channel;

typedef struct
{
  int fd;
  AL_TDecScheduler_CB_EndStartCode endStartCodeCB;
  bool bEnded;
  AL_TDriver* driver;
}SCMsg;

typedef struct AL_t_Event
{
  void* pPriv;
  AL_ListHead List;
}AL_Event;

typedef struct AL_t_EventQueue
{
  AL_WaitQueue Queue;
  AL_ListHead List;
  pthread_mutex_t Lock;
}AL_EventQueue;

typedef struct
{
  AL_THREAD thread;
  AL_EventQueue queue;
}StartCodeChannel;

struct DecSchedulerMcuCtx
{
  const AL_IDecSchedulerVtable* vtable;
  AL_TDriver* driver;
  char* deviceFile;
};

static void AL_EventQueue_Init(AL_EventQueue* pEventQueue)
{
  AL_WaitQueue_Init(&pEventQueue->Queue);
  AL_ListHeadInit(&pEventQueue->List);
  pthread_mutex_init(&pEventQueue->Lock, NULL);
}

static void AL_EventQueue_Deinit(AL_EventQueue* pEventQueue)
{
  pthread_mutex_destroy(&pEventQueue->Lock);
  AL_WaitQueue_Deinit(&pEventQueue->Queue);
}

static void AL_EventQueue_Push(AL_EventQueue* pEventQueue, AL_Event* pEvent)
{
  pthread_mutex_lock(&pEventQueue->Lock);
  AL_ListAddTail(&pEvent->List, &pEventQueue->List);
  pthread_mutex_unlock(&pEventQueue->Lock);

  AL_WakeUp(&pEventQueue->Queue);
}

/* for one Reader */
static void AL_EventQueue_Fetch(AL_EventQueue* EventQueue, AL_Event** pEvent, bool (* isReady)(void*), void* pParam)
{
  AL_WaitEvent(&EventQueue->Queue, isReady, pParam);

  /* get msg */
  pthread_mutex_lock(&EventQueue->Lock);
  *pEvent = AL_ListFirstEntry(&EventQueue->List, AL_Event, List);
  AL_ListDel(&(*pEvent)->List);
  pthread_mutex_unlock(&EventQueue->Lock);
}

void setPictParam(struct al5_params* msg, AL_TDecPicParam* pPictParam)
{
  static_assert(sizeof(*pPictParam) <= sizeof(msg->opaque), "Driver pict_param struct is too small");
  msg->size = sizeof(*pPictParam);
  Rtos_Memcpy(msg->opaque, pPictParam, msg->size);
}

void setPictBufferAddrs(struct al5_params* msg, AL_TDecPicBufferAddrs* pBufferAddrs)
{
  static_assert(sizeof(*pBufferAddrs) <= sizeof(msg->opaque), "Driver pict_buffer_addrs struct is too small");
  msg->size = sizeof(*pBufferAddrs);
  Rtos_Memcpy(msg->opaque, pBufferAddrs, msg->size);
}

/* Fill msg with pChParam */
static void setChannelMsg(struct al5_params* msg, const TMemDesc* pMDChParams)
{
  uint32_t uVirtAddr;
  static_assert(sizeof(uVirtAddr) <= sizeof(msg->opaque), "Driver channel_param struct is too small");
  msg->size = sizeof(uVirtAddr);

  uVirtAddr = pMDChParams->uPhysicalAddr + DCACHE_OFFSET;
  Rtos_Memcpy(msg->opaque, &uVirtAddr, msg->size);
}

static bool getStatusMsg(Channel* chan, struct al5_params* msg)
{
  bool bRet = AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_WAIT_FOR_STATUS, msg) == DRIVER_SUCCESS;

  if(!bRet && !chan->bBeingDestroyed)
    perror("Failed to get decode status");

  return bRet;
}

static bool getScStatusMsg(SCMsg* pMsg, struct al5_scstatus* StatusMsg)
{
  return AL_Driver_PostMessage(pMsg->driver, pMsg->fd, AL_MCU_WAIT_FOR_START_CODE, StatusMsg) == DRIVER_SUCCESS;
}

static void processStatusMsg(Channel* channel, struct al5_params* msg)
{
  uint32_t const DEC_1 = 1;

  if(msg->opaque[0] == DEC_1)
  {
    uint32_t uFrameID;
    uint32_t uParsingID;
    int iOffset = (sizeof(DEC_1) / sizeof(*msg->opaque));
    Rtos_Memcpy(&uFrameID, msg->opaque + iOffset, msg->size - iOffset);
    Rtos_Memcpy(&uParsingID, msg->opaque + iOffset + 4, msg->size - iOffset - 4);

    if(channel->endParsingCB.func)
      channel->endParsingCB.func(channel->endParsingCB.userParam, uFrameID, uParsingID);
    return;
  }

  uint32_t const DEC_2 = 2;

  if(msg->opaque[0] == DEC_2)
  {
    AL_TDecPicStatus status;
    int iOffset = (sizeof(DEC_2) / sizeof(*msg->opaque));
    Rtos_Memcpy(&status, msg->opaque + iOffset, msg->size - iOffset);

    if(channel->endDecodingCB.func)
      channel->endDecodingCB.func(channel->endDecodingCB.userParam, &status);
    return;
  }
  AL_Assert(0);
}

static void* NotificationThread(void* p)
{
  Rtos_SetCurrentThreadName("dec-status-it");
  Channel* chan = p;

  while(true)
  {
    struct al5_params msg = { 0 };

    if(!getStatusMsg(chan, &msg))
      break;

    processStatusMsg(chan, &msg);
  }

  return NULL;
}

static void setScStatus(AL_TScStatus* status, struct al5_scstatus* msg)
{
  status->uNumSC = msg->num_sc;
  status->uNumBytes = msg->num_bytes;
}

static void processScStatusMsg(SCMsg* pMsg, struct al5_scstatus* StatusMsg)
{
  AL_TScStatus status;

  setScStatus(&status, StatusMsg);
  pMsg->endStartCodeCB.func(pMsg->endStartCodeCB.userParam, &status);
}

/* One reader, no race condition */
bool isSCReady(void* p)
{
  AL_EventQueue* pCtx = p;
  return !AL_ListEmpty(&pCtx->List);
}

static void* ScNotificationThread(void* p)
{
  Rtos_SetCurrentThreadName("dec-scd-it");
  AL_EventQueue* pEventQueue = (AL_EventQueue*)p;
  struct al5_scstatus StatusMsg = { 0 };

  while(true)
  {
    AL_Event* pEvent;
    AL_EventQueue_Fetch(pEventQueue, &pEvent, isSCReady, pEventQueue);
    SCMsg* pMsg = pEvent->pPriv;
    Rtos_Free(pEvent);

    if(pMsg->bEnded)
    {
      Rtos_Free(pMsg);
      break;
    }

    if(getScStatusMsg(pMsg, &StatusMsg))
      processScStatusMsg(pMsg, &StatusMsg);

    AL_Driver_Close(pMsg->driver, pMsg->fd);
    Rtos_Free(pMsg);
  }

  return NULL;
}

static void API_Destroy(AL_IDecScheduler* pScheduler)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;
  Rtos_Free(scheduler->deviceFile);
  Rtos_Free(scheduler);
}

static AL_ERR API_CreateStartCodeChannel(AL_HANDLE* hStartCodeChannel, AL_IDecScheduler* pScheduler)
{
  (void)pScheduler;
  AL_ERR errorCode = AL_ERROR;

  StartCodeChannel* scChan = Rtos_Malloc(sizeof(*scChan));

  if(!scChan)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  AL_EventQueue_Init(&scChan->queue);
  scChan->thread = Rtos_CreateThread(&ScNotificationThread, &scChan->queue);

  if(!scChan->thread)
  {
    perror("Couldn't create thread");
    goto fail;
  }

  *hStartCodeChannel = (AL_HANDLE)scChan;

  return AL_SUCCESS;

  fail:
  Rtos_Free(scChan);
  channel_creation_fail:
  *hStartCodeChannel = NULL;
  return errorCode;
}

static AL_ERR API_DestroyStartCodeChannel(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;

  AL_ERR errorCode = AL_ERROR;
  StartCodeChannel* scChan = (StartCodeChannel*)hStartCodeChannel;

  /* if the start code channel doesn't exist, nothing to do, return immediately. */
  if(!scChan)
    return AL_SUCCESS;

  AL_EventQueue* pEventQueue = &scChan->queue;

  AL_Event* pEvent = Rtos_Malloc(sizeof(*pEvent));

  if(!pEvent)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail_event;
  }

  SCMsg* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(!pMsg)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail_msg;
  }

  pMsg->bEnded = true;
  pMsg->driver = scheduler->driver;
  pEvent->pPriv = pMsg;
  AL_EventQueue_Push(pEventQueue, pEvent);

  if(!Rtos_JoinThread(scChan->thread))
    goto fail_join;

  Rtos_DeleteThread(scChan->thread);
  AL_EventQueue_Deinit(pEventQueue);
  Rtos_Free(scChan);

  return AL_SUCCESS;

  fail_join:
  AL_EventQueue_Deinit(pEventQueue);
  fail_msg:
  Rtos_Free(pEvent);
  fail_event:
  return errorCode;
}

static AL_ERR API_DestroyChannel(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel)
{
  (void)pScheduler;
  Channel* chan = (Channel*)hChannel;

  /* if the channel doesn't exist, nothing to do, return immediately. */
  if(!chan)
    return AL_SUCCESS;

  chan->bBeingDestroyed = true;
  AL_ERR errorCode = AL_ERROR;

  if(AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_DESTROY_CHANNEL, NULL) != DRIVER_SUCCESS)
  {
    perror("Failed to destroy channel");
    goto exit;
  }

  Rtos_JoinThread(chan->thread);
  Rtos_DeleteThread(chan->thread);

  errorCode = AL_SUCCESS;

  exit:
  AL_Driver_Close(chan->driver, chan->fd);
  Rtos_Free(chan);
  return errorCode;
}

static AL_ERR API_CreateChannel(AL_HANDLE* hChannel, AL_IDecScheduler* pScheduler, TMemDesc* pMDChParams, AL_TDecScheduler_CB_EndParsing endParsingCallback, AL_TDecScheduler_CB_EndDecoding endDecodingCallback)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;
  AL_ERR errorCode = AL_ERROR;

  Channel* chan = Rtos_Malloc(sizeof(*chan));

  if(!chan)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  chan->bBeingDestroyed = false;
  chan->endParsingCB = endParsingCallback;
  chan->endDecodingCB = endDecodingCallback;
  chan->driver = scheduler->driver;
  chan->fd = AL_Driver_Open(chan->driver, scheduler->deviceFile);

  if(chan->fd < 0)
  {
    fprintf(stderr, "Couldn't open device file %s while creating channel; %s\n", scheduler->deviceFile, strerror(errno));
    goto fail_open;
  }

  struct al5_channel_config msg = { 0 };
  setChannelMsg(&msg.param, pMDChParams);

  AL_EDriverError errdrv = AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_CONFIG_CHANNEL, &msg);

  if(errdrv != DRIVER_SUCCESS)
  {
    if(errdrv == DRIVER_ERROR_NO_MEMORY)
      errorCode = AL_ERR_NO_MEMORY;

    /* the ioctl might not have been called at all,
     * so the error_code might no be set. leave it to AL_ERROR in this case */
    if(errdrv == DRIVER_ERROR_CHANNEL && msg.status.error_code)
      errorCode = msg.status.error_code;
    goto fail_open;
  }

  AL_Assert(msg.status.error_code == 0);

  chan->thread = Rtos_CreateThread(&NotificationThread, chan);

  if(!chan->thread)
    goto fail_open;

  *hChannel = (AL_HANDLE)chan;
  return AL_SUCCESS;

  fail_open:

  if(chan->fd >= 0)
    AL_Driver_Close(chan->driver, chan->fd);

  Rtos_Free(chan);
  channel_creation_fail:
  return errorCode;
}

static void setStartCodeParams(struct al5_params* msg, AL_TScParam* pScParam)
{
  static_assert(sizeof(*pScParam) <= sizeof(msg->opaque), "Driver sc_param struct is too small");
  msg->size = sizeof(*pScParam);
  Rtos_Memcpy(msg->opaque, pScParam, sizeof(*pScParam));
}

static void setStartCodeAddresses(struct al5_params* msg, AL_TScBufferAddrs* pBufAddrs)
{
  static_assert(sizeof(*pBufAddrs) <= sizeof(msg->opaque), "Driver sc_addresses struct is too small");
  msg->size = sizeof(*pBufAddrs);
  Rtos_Memcpy(msg->opaque, pBufAddrs, sizeof(*pBufAddrs));
}

static void setSearchStartCodeMsg(struct al5_search_sc_msg* search_msg, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufAddrs)
{
  setStartCodeParams(&search_msg->param, pScParam);
  setStartCodeAddresses(&search_msg->buffer_addrs, pBufAddrs);
}

static void API_SearchSC(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufAddrs, AL_TDecScheduler_CB_EndStartCode endStartCodeCB)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;
  StartCodeChannel* scChan = (StartCodeChannel*)hStartCodeChannel;
  AL_EventQueue* pEventQueue = &scChan->queue;
  AL_Event* pEvent = Rtos_Malloc(sizeof(*pEvent));

  if(!pEvent)
    goto fail_event;

  SCMsg* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(!pMsg)
    goto fail_msg;

  pMsg->bEnded = false;
  pMsg->endStartCodeCB = endStartCodeCB;
  pMsg->driver = scheduler->driver;
  pMsg->fd = AL_Driver_Open(pMsg->driver, scheduler->deviceFile);

  if(pMsg->fd < 0)
  {
    fprintf(stderr, "Cannot open device file %s: %s\n", scheduler->deviceFile, strerror(errno));
    goto fail_open;
  }

  struct al5_search_sc_msg search_msg = { 0 };
  setSearchStartCodeMsg(&search_msg, pScParam, pBufAddrs);

  if(AL_Driver_PostMessage(scheduler->driver, pMsg->fd, AL_MCU_SEARCH_START_CODE, &search_msg) != DRIVER_SUCCESS)
  {
    perror("Failed to search start code");
    goto fail_open;
  }

  pEvent->pPriv = pMsg;
  AL_EventQueue_Push(pEventQueue, pEvent);

  return;

  fail_open:
  AL_Driver_Close(pMsg->driver, pMsg->fd);
  Rtos_Free(pMsg);
  fail_msg:
  Rtos_Free(pEvent);
  fail_event:
  return;
}

static void prepareDecodeMessage(struct al5_decode_msg* msg, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  setPictParam(&msg->params, pPictParam);
  setPictBufferAddrs(&msg->addresses, pPictAddrs);

  msg->slice_param_v = hSliceParam->uPhysicalAddr + DCACHE_OFFSET;
}

// TODO return error
static void API_DecodeOneFrame(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;
  Channel* chan = (Channel*)hChannel;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);

  if(AL_Driver_PostMessage(scheduler->driver, chan->fd, AL_MCU_DECODE_ONE_FRM, &msg) != DRIVER_SUCCESS)
    perror("Failed to decode");
}

static void API_DecodeOneSlice(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  struct DecSchedulerMcuCtx* scheduler = (struct DecSchedulerMcuCtx*)pScheduler;
  Channel* chan = (Channel*)hChannel;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);

  if(AL_Driver_PostMessage(scheduler->driver, chan->fd, AL_MCU_DECODE_ONE_SLICE, &msg) != DRIVER_SUCCESS)
    perror("Failed to decode");
}

static const AL_IDecSchedulerVtable DecSchedulerMcuVtable =
{
  API_Destroy,
  API_CreateStartCodeChannel,
  API_CreateChannel,
  API_DestroyStartCodeChannel,
  API_DestroyChannel,
  API_SearchSC,
  API_DecodeOneFrame,
  API_DecodeOneSlice,
};

/* Initialisation cannot be handled by control software with mcu.
 * With several channels it would be unacceptable to let any reinit the scheduler.
 * We init the scheduler once in the firmware at startup. */

AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_TDriver* driver, char const* deviceFile)
{
  struct DecSchedulerMcuCtx* scheduler = Rtos_Malloc(sizeof(*scheduler));

  if(!scheduler)
    return NULL;

  scheduler->vtable = &DecSchedulerMcuVtable;
  scheduler->driver = driver;

  scheduler->deviceFile = Rtos_Malloc((strlen(deviceFile) + 1) * sizeof(char));

  if(!scheduler->deviceFile)
  {
    Rtos_Free(scheduler);
    return NULL;
  }

  strcpy(scheduler->deviceFile, deviceFile);

  return (AL_IDecScheduler*)scheduler;
}

#else

AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_TDriver* driver, char const* deviceFile)
{
  (void)driver, (void)deviceFile;
  return NULL;
}

#endif
/*@}*/
