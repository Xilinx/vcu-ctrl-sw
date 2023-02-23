// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
  int const ret = pthread_cond_broadcast(&pQueue->Cond);
  (void)ret;
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
  int iChannelID; // for debug purposes
}Channel;

typedef struct
{
  int fd;
  AL_TDecScheduler_CB_EndStartCode endStartCodeCB;
  bool bEnded;
  AL_TDriver* driver;
}SCMsg;

typedef struct
{
  void* pPriv;
  AL_ListHead List;
}AL_Event;

typedef struct
{
  AL_WaitQueue Queue;
  AL_ListHead List;
  pthread_mutex_t Lock;
}AL_EventQueue;

typedef struct
{
  AL_THREAD thread;
  AL_EventQueue queue;
  int fd;
}StartCodeChannel;

typedef struct
{
  AL_IDecSchedulerVtable const* vtable;
  AL_TDriver* driver;
  char* deviceFile;
}DecSchedulerMcuCtx;

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
static void setChannelMsg(struct al5_params* msg, TMemDesc const* pMDChParams)
{
  uint32_t uVirtAddr;
  static_assert(sizeof(uVirtAddr) <= sizeof(msg->opaque), "Driver channel_param struct is too small");
  msg->size = sizeof(uVirtAddr);

  uVirtAddr = pMDChParams->uPhysicalAddr + DCACHE_OFFSET;
  Rtos_Memcpy(msg->opaque, &uVirtAddr, msg->size);
}

static void processStatusMsg(Channel const* channel, struct al5_params const* msg)
{
  uint32_t const DEC_1 = 1;

  if(msg->opaque[0] == DEC_1)
  {
    uint32_t uFrameID;
    uint32_t uParsingID;
    int iOffset = sizeof(DEC_1);
    AL_Assert(msg->size >= iOffset + sizeof(uFrameID) + sizeof(uParsingID));
    Rtos_Memcpy(&uFrameID, (uint8_t*)msg->opaque + iOffset, sizeof(uFrameID));
    Rtos_Memcpy(&uParsingID, (uint8_t*)msg->opaque + iOffset + sizeof(uFrameID), sizeof(uParsingID));

    if(channel->endParsingCB.func)
      channel->endParsingCB.func(channel->endParsingCB.userParam, uFrameID, uParsingID);
    return;
  }

  uint32_t const DEC_2 = 2;

  if(msg->opaque[0] == DEC_2)
  {
    AL_TDecPicStatus status;
    int iOffset = sizeof(DEC_2);
    AL_Assert(msg->size >= iOffset + sizeof(status));
    Rtos_Memcpy(&status, (uint8_t*)msg->opaque + iOffset, sizeof(status));

    if(channel->endDecodingCB.func)
      channel->endDecodingCB.func(channel->endDecodingCB.userParam, &status);
    return;
  }
  AL_Assert(0);
}

static void* NotificationThread(void* p)
{
  Rtos_SetCurrentThreadName("dec-status-it");
  Channel const* channel = p;
  struct al5_params msg = { 0 };
  Rtos_PollCtx ctx;
  /* Wait for wait for status events forever in poll */
  ctx.timeout = -1;
  ctx.events = AL_POLLIN;

  while(true)
  {
    ctx.revents = 0;

    AL_EDriverError err = AL_Driver_PostMessage(channel->driver, channel->fd, AL_POLL_MSG, &ctx);

    if(err != DRIVER_SUCCESS)
      continue;

    if(ctx.revents & AL_POLLIN)
    {
      bool const blocking = true;
      err = AL_Driver_PostMessage2(channel->driver, channel->fd, AL_MCU_WAIT_FOR_STATUS, &msg, !blocking);

      if(err == DRIVER_SUCCESS)
        processStatusMsg(channel, &msg);
      else
        Rtos_Log(AL_LOG_ERROR, "Failed to get decode status (error code: %d)\n", err);
    }

    /* If the polling finds an end of operation, it means that the channel was destroyed and we can stop waiting for decoding results. */
    if(ctx.revents & AL_POLLHUP)
      break;
  }

  return NULL;
}

static void setScStatus(AL_TScStatus* status, struct al5_scstatus const* msg)
{
  status->uNumSC = msg->num_sc;
  status->uNumBytes = msg->num_bytes;
}

static void processScStatusMsg(SCMsg const* pMsg, struct al5_scstatus const* StatusMsg)
{
  AL_TScStatus status;

  setScStatus(&status, StatusMsg);
  pMsg->endStartCodeCB.func(pMsg->endStartCodeCB.userParam, &status);
}

/* One reader, no race condition */
static bool isSCReady(void* p)
{
  AL_EventQueue* pCtx = p;
  pthread_mutex_lock(&pCtx->Lock);
  bool isReady = !AL_ListEmpty(&pCtx->List);
  pthread_mutex_unlock(&pCtx->Lock);
  return isReady;
}

static void* ScNotificationThread(void* p)
{
  Rtos_SetCurrentThreadName("dec-scd-it");
  AL_EventQueue* pEventQueue = (AL_EventQueue*)p;
  struct al5_scstatus StatusMsg = { 0 };
  Rtos_PollCtx ctx;
  /* Wait for start code events forever in poll */
  ctx.timeout = -1;
  ctx.events = AL_POLLIN;

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

    ctx.revents = 0;

    AL_EDriverError err = AL_Driver_PostMessage(pMsg->driver, pMsg->fd, AL_POLL_MSG, &ctx);

    if(err != DRIVER_SUCCESS)
      continue;

    if(ctx.revents & AL_POLLIN)
    {
      bool const blocking = true;
      err = AL_Driver_PostMessage2(pMsg->driver, pMsg->fd, AL_MCU_WAIT_FOR_START_CODE, &StatusMsg, !blocking);

      if(err == DRIVER_SUCCESS)
        processScStatusMsg(pMsg, &StatusMsg);
      else
        Rtos_Log(AL_LOG_ERROR, "Failed to get start code status (error code: %d)\n", err);
    }

    if(ctx.revents & AL_POLLHUP)
    {
      Rtos_Log(AL_LOG_ERROR, "Unexpected End of Operation on start code status: (error_code: %d\n", err);
      break;
    }

    Rtos_Free(pMsg);
  }

  return NULL;
}

static void API_Destroy(AL_IDecScheduler* pScheduler)
{
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
  Rtos_Free(scheduler->deviceFile);
  Rtos_Free(scheduler);
}

static AL_ERR API_CreateStartCodeChannel(AL_HANDLE* hStartCodeChannel, AL_IDecScheduler* pScheduler)
{
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
  AL_ERR errorCode = AL_ERROR;

  StartCodeChannel* scChan = Rtos_Malloc(sizeof(*scChan));

  if(!scChan)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  scChan->fd = AL_Driver_Open(scheduler->driver, scheduler->deviceFile);

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
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;

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
  AL_Driver_Close(scheduler->driver, scChan->fd);
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
  Channel* pChannel = (Channel*)hChannel;

  /* if the channel doesn't exist, nothing to do, return immediately. */
  if(!pChannel)
    return AL_SUCCESS;

  pChannel->bBeingDestroyed = true;
  AL_ERR errorCode = AL_ERROR;

  AL_EDriverError const error = AL_Driver_PostMessage(pChannel->driver, pChannel->fd, AL_MCU_DESTROY_CHANNEL, NULL);

  if(error != DRIVER_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to destroy channel (error code: %d)\n", error);
    goto exit;
  }

  Rtos_JoinThread(pChannel->thread);
  Rtos_DeleteThread(pChannel->thread);

  errorCode = AL_SUCCESS;

  exit:
  AL_Driver_Close(pChannel->driver, pChannel->fd);
  Rtos_Free(pChannel);
  return errorCode;
}

static AL_ERR API_CreateChannel(AL_HANDLE* hChannel, AL_IDecScheduler* pScheduler, TMemDesc* pMDChParams, AL_TDecScheduler_CB_EndParsing endParsingCallback, AL_TDecScheduler_CB_EndDecoding endDecodingCallback)
{
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
  AL_ERR errorCode = AL_ERROR;

  Channel* pChannel = Rtos_Malloc(sizeof(*pChannel));

  if(!pChannel)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  pChannel->bBeingDestroyed = false;
  pChannel->iChannelID = -1;
  pChannel->endParsingCB = endParsingCallback;
  pChannel->endDecodingCB = endDecodingCallback;
  pChannel->driver = scheduler->driver;
  pChannel->fd = AL_Driver_Open(pChannel->driver, scheduler->deviceFile);

  if(pChannel->fd < 0)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open device file %s while creating channel: %s\n", scheduler->deviceFile, strerror(errno));
    goto fail_open;
  }

  struct al5_channel_config msg = { 0 };
  setChannelMsg(&msg.param, pMDChParams);

  AL_EDriverError errdrv = AL_Driver_PostMessage(pChannel->driver, pChannel->fd, AL_MCU_CONFIG_CHANNEL, &msg);

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

  AL_Assert(msg.status.error_code == AL_SUCCESS);

  /* Retrieve channel id if it was given to us using the param as an output (debug) */
  if(msg.param.size == 4)
    pChannel->iChannelID = msg.param.opaque[0];

  pChannel->thread = Rtos_CreateThread(&NotificationThread, pChannel);

  if(!pChannel->thread)
    goto fail_open;

  *hChannel = (AL_HANDLE)pChannel;
  return AL_SUCCESS;

  fail_open:

  if(pChannel->fd >= 0)
    AL_Driver_Close(pChannel->driver, pChannel->fd);

  Rtos_Free(pChannel);
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
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
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
  pMsg->fd = scChan->fd;

  if(pMsg->fd < 0)
  {
    Rtos_Log(AL_LOG_ERROR, "Cannot open device file %s: %s\n", scheduler->deviceFile, strerror(errno));
    goto fail_open;
  }

  struct al5_search_sc_msg search_msg = { 0 };
  setSearchStartCodeMsg(&search_msg, pScParam, pBufAddrs);

  AL_EDriverError error = AL_Driver_PostMessage(scheduler->driver, pMsg->fd, AL_MCU_SEARCH_START_CODE, &search_msg);

  if(error != DRIVER_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to search start code (error code: %d)\n", error);
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
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
  Channel* chan = (Channel*)hChannel;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);
  AL_EDriverError error = AL_Driver_PostMessage(scheduler->driver, chan->fd, AL_MCU_DECODE_ONE_FRM, &msg);

  if(error != DRIVER_SUCCESS)
    Rtos_Log(AL_LOG_ERROR, "Failed to decode one frame (error code: %d)\n", error);
}

static void API_DecodeOneSlice(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  DecSchedulerMcuCtx* scheduler = (DecSchedulerMcuCtx*)pScheduler;
  Channel* chan = (Channel*)hChannel;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);

  AL_EDriverError const error = AL_Driver_PostMessage(scheduler->driver, chan->fd, AL_MCU_DECODE_ONE_SLICE, &msg);

  if(error != DRIVER_SUCCESS)
    Rtos_Log(AL_LOG_ERROR, "Failed to decode one slice (error code: %d)\n", error);
}

static void GetSchedulerCoreInfo(DecSchedulerMcuCtx const* pThis, AL_TISchedulerCore* pCore)
{
  int const fd = AL_Driver_Open(pThis->driver, pThis->deviceFile);

  if(fd < 0)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open device file '%s' while creating channel: '%s'\n", pThis->deviceFile, strerror(errno));
    return;
  }

  struct al5_params msg;
  AL_EIDecSchedulerInfo eInfo = AL_ISCHEDULER_CORE;
  msg.opaque[0] = eInfo;
  memcpy(&msg.opaque[sizeof(eInfo) / sizeof(*msg.opaque)], pCore, sizeof(*pCore));

  static_assert(sizeof(eInfo) + sizeof(*pCore) <= sizeof(msg.opaque), "Driver core structure struct is too small");
  msg.size = sizeof(eInfo) + sizeof(*pCore);

  AL_EDriverError const error = AL_Driver_PostMessage(pThis->driver, fd, AL_MCU_GET, &msg);

  if(error != DRIVER_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to get parameter '%s', (error code: '%d')\n", ToStringIDecSchedulerInfo(AL_ISCHEDULER_CORE), error);
    AL_Driver_Close(pThis->driver, fd);
    return;
  }

  memcpy(pCore, &msg.opaque[1], sizeof(*pCore));

  AL_Driver_Close(pThis->driver, fd);
}

static void API_Get(AL_IDecScheduler const* pScheduler, AL_EIDecSchedulerInfo info, void* pParam)
{
  DecSchedulerMcuCtx const* pThis = (DecSchedulerMcuCtx const*)pScheduler;
  switch(info)
  {
  case AL_ISCHEDULER_CORE:
  {
    GetSchedulerCoreInfo(pThis, (AL_TISchedulerCore*)pParam);
    return;
  }
  default: return;
  }

  return;
}

static void API_Set(AL_IDecScheduler* pScheduler, AL_EIDecSchedulerInfo info, void const* pParam)
{
  (void)pParam;
  DecSchedulerMcuCtx* pThis = (DecSchedulerMcuCtx*)pScheduler;
  (void)pThis;
  switch(info)
  {
  default: return;
  }

  return;
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
  API_Get,
  API_Set,
};

/* Initialisation cannot be handled by control software with mcu.
 * With several channels it would be unacceptable to let any reinit the scheduler.
 * We init the scheduler once in the firmware at startup. */

AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_TDriver* driver, char const* deviceFile)
{
  DecSchedulerMcuCtx* scheduler = Rtos_Malloc(sizeof(*scheduler));

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
