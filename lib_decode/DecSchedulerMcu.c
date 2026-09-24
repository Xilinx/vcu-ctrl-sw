// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_decode/I_DecScheduler.h"
#include "lib_common/I_Communication.h"

#if defined(__linux__)

#include "lib_rtos/assert.h"
#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "DecDriverDataConversions.h"

#include "lib_common/List.h"
#include "lib_common/Error.h"
#include "lib_common_dec/SchedulerInfo.h"
#include "lib_decode/DecSchedulerCommon.h"

typedef struct
{
  AL_HANDLE hCommunicationLine;
  AL_THREAD thread;
  bool bBeingDestroyed;
  AL_ICommunication* pCommunication;
  int32_t iChannelID; // for debug purposes
  AL_TDecScheduler_ChannelParams tSchedParams;
}Channel;

typedef struct AL_TStartCodeMessage
{
  AL_HANDLE hCommunicationLine;
  AL_TDecScheduler_CB_EndStartCode endStartCodeCB;
  bool bEnded;
  AL_ICommunication* pCommunication;
}AL_TStartCodeMessage;

typedef struct AL_TEvent
{
  void* pPrivate;
  AL_ListHead tList;
}AL_TEvent;

typedef struct AL_TEventQueue
{
  AL_SEMAPHORE hEvents;
  AL_ListHead tList;
  AL_MUTEX hLock;
}AL_TEventQueue;

typedef struct AL_TStartCodeChannel
{
  AL_THREAD thread;
  AL_TEventQueue queue;
  AL_HANDLE hCommunicationLine;
}AL_TStartCodeChannel;

typedef struct
{
  AL_IDecSchedulerVtable const* vtable;
  AL_ICommunication* pCommunication;
}AL_TDecSchedulerMicroblaze;

static bool AL_TEventQueue_Init(AL_TEventQueue* pEventQueue)
{
  pEventQueue->hEvents = Rtos_CreateSemaphore(0);

  if(pEventQueue->hEvents == NULL)
    return false;

  AL_ListHeadInit(&pEventQueue->tList);

  pEventQueue->hLock = Rtos_CreateMutex();

  if(pEventQueue->hLock == NULL)
  {
    Rtos_DeleteSemaphore(pEventQueue->hEvents);
    return false;
  }

  return true;
}

static void AL_TEventQueue_Deinit(AL_TEventQueue* pEventQueue)
{
  Rtos_DeleteMutex(pEventQueue->hLock);
  Rtos_DeleteSemaphore(pEventQueue->hEvents);
}

static void AL_TEventQueue_Push(AL_TEventQueue* pEventQueue, AL_TEvent* pEvent)
{
  Rtos_GetMutex(pEventQueue->hLock);
  AL_ListAddTail(&pEvent->tList, &pEventQueue->tList);
  Rtos_ReleaseMutex(pEventQueue->hLock);

  Rtos_ReleaseSemaphore(pEventQueue->hEvents);
}

/* for one Reader */
static void AL_TEventQueue_Fetch(AL_TEventQueue* EventQueue, AL_TEvent** pEvent)
{
  Rtos_GetSemaphore(EventQueue->hEvents, AL_WAIT_FOREVER);

  /* get msg */
  Rtos_GetMutex(EventQueue->hLock);
  *pEvent = AL_ListFirstEntry(&EventQueue->tList, AL_TEvent, tList);
  AL_ListDel(&(*pEvent)->tList);
  Rtos_ReleaseMutex(EventQueue->hLock);
}

static void processStatusMsg(Channel const* channel, struct al5_params* msg)
{
  Rtos_InvalidateCacheMemory(msg, sizeof(msg->size));
  Rtos_InvalidateCacheMemory(msg, msg->size);
  uint32_t const DEC_1 = 1;

  if(msg->opaque[0] == DEC_1)
  {
    uint32_t uFrameID;
    uint32_t uParsingID;
    int32_t iOffset = sizeof(DEC_1);
    Rtos_Assert(msg->size >= iOffset + sizeof(uFrameID) + sizeof(uParsingID));
    Rtos_Memcpy(&uFrameID, (uint8_t*)msg->opaque + iOffset, sizeof(uFrameID));
    Rtos_Memcpy(&uParsingID, (uint8_t*)msg->opaque + iOffset + sizeof(uFrameID), sizeof(uParsingID));

    if(channel->tSchedParams.endParsingCallback.func)
      channel->tSchedParams.endParsingCallback.func(channel->tSchedParams.endParsingCallback.userParam, uFrameID, uParsingID);
    return;
  }

  uint32_t const DEC_2 = 2;

  if(msg->opaque[0] == DEC_2)
  {
    AL_TDecPicStatus status;
    int32_t iOffset = sizeof(DEC_2);
    Rtos_Assert(msg->size >= iOffset + sizeof(status));
    Rtos_Memcpy(&status, (uint8_t*)msg->opaque + iOffset, sizeof(status));

    if(channel->tSchedParams.endDecodingCallback.func)
      channel->tSchedParams.endDecodingCallback.func(channel->tSchedParams.endDecodingCallback.userParam, &status);
    return;
  }
  Rtos_Assert(false);
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

    AL_ECommunicationError err = AL_ICommunication_PostBlockingMessage(channel->pCommunication, channel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_POLL, &ctx);

    if(err != AL_COMMUNICATION_SUCCESS)
      continue;

    if(ctx.revents & AL_POLLIN)
    {
      err = AL_ICommunication_PostNonBlockingMessage(channel->pCommunication, channel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_WAIT_FOR_STATUS, &msg);

      if(err == AL_COMMUNICATION_SUCCESS)
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

static void processStartCodeStatusMsg(AL_TStartCodeMessage const* pMsg, struct al5_scstatus* pStatusMsg)
{
  Rtos_InvalidateCacheMemory(pStatusMsg, sizeof(*pStatusMsg));
  AL_TStartCodeStatus tStatus;
  tStatus.uNumSC = pStatusMsg->num_sc;
  tStatus.uNumBytes = pStatusMsg->num_bytes;
  pMsg->endStartCodeCB.func(pMsg->endStartCodeCB.userParam, &tStatus);
}

static void* StartCodeNotificationThread(void* p)
{
  Rtos_SetCurrentThreadName("dec-scd-it");
  AL_TEventQueue* pEventQueue = (AL_TEventQueue*)p;
  struct al5_scstatus StatusMsg = { 0 };
  Rtos_PollCtx ctx;
  /* Wait for start code events forever in poll */
  ctx.timeout = -1;
  ctx.events = AL_POLLIN;

  while(true)
  {
    AL_TEvent* pEvent;
    AL_TEventQueue_Fetch(pEventQueue, &pEvent);
    AL_TStartCodeMessage* pMsg = pEvent->pPrivate;
    Rtos_Free(pEvent);

    if(pMsg->bEnded)
    {
      Rtos_Free(pMsg);
      break;
    }

    ctx.revents = 0;

    AL_ECommunicationError err = AL_ICommunication_PostBlockingMessage(pMsg->pCommunication, pMsg->hCommunicationLine, AL_COMMUNICATION_MESSAGE_POLL, &ctx);

    if(err != AL_COMMUNICATION_SUCCESS)
      continue;

    if(ctx.revents & AL_POLLIN)
    {
      err = AL_ICommunication_PostNonBlockingMessage(pMsg->pCommunication, pMsg->hCommunicationLine, AL_COMMUNICATION_MESSAGE_WAIT_FOR_START_CODE, &StatusMsg);

      if(err == AL_COMMUNICATION_SUCCESS)
        processStartCodeStatusMsg(pMsg, &StatusMsg);
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
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  Rtos_Free(scheduler);
}

static AL_ERR API_CreateStartCodeChannel(AL_HANDLE* hStartCodeChannel, AL_IDecScheduler* pScheduler)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  AL_ERR errorCode = AL_ERROR;
  AL_TStartCodeChannel* pStartCodeChannel = Rtos_Malloc(sizeof(*pStartCodeChannel));

  if(pStartCodeChannel == NULL)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_create_fail_alloc;
  }

  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  pStartCodeChannel->hCommunicationLine = AL_ICommunication_Open(scheduler->pCommunication, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to open communication: %s\n", AL_ECommunicationError_ToString(eError));
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_create_fail_open;
  }

  if(!AL_TEventQueue_Init(&pStartCodeChannel->queue))
    goto channel_create_fail_event_queue_init;

  pStartCodeChannel->thread = Rtos_CreateThread(&StartCodeNotificationThread, &pStartCodeChannel->queue);

  if(pStartCodeChannel->thread == NULL)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't create thread");
    goto channel_create_fail_thread;
  }

  *hStartCodeChannel = (AL_HANDLE)pStartCodeChannel;

  return AL_SUCCESS;

  channel_create_fail_thread:
  AL_TEventQueue_Deinit(&pStartCodeChannel->queue);
  channel_create_fail_event_queue_init:
  AL_ICommunication_Close(scheduler->pCommunication, pStartCodeChannel->hCommunicationLine);
  channel_create_fail_open:
  Rtos_Free(pStartCodeChannel);
  channel_create_fail_alloc:
  *hStartCodeChannel = NULL;
  return errorCode;
}

static AL_ERR API_DestroyStartCodeChannel(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  AL_ERR errorCode = AL_ERROR;
  AL_TStartCodeChannel* pStartCodeChannel = (AL_TStartCodeChannel*)hStartCodeChannel;

  /* if the start code channel doesn't exist, nothing to do, return immediately. */
  if(pStartCodeChannel == NULL)
    return AL_SUCCESS;

  AL_TEventQueue* pEventQueue = &pStartCodeChannel->queue;

  AL_TEvent* pEvent = Rtos_Malloc(sizeof(*pEvent));

  if(pEvent == NULL)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail_event;
  }

  AL_TStartCodeMessage* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(pMsg == NULL)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto fail_msg;
  }

  pMsg->bEnded = true;
  pMsg->pCommunication = scheduler->pCommunication;
  pEvent->pPrivate = pMsg;
  AL_TEventQueue_Push(pEventQueue, pEvent);

  if(!Rtos_JoinThread(pStartCodeChannel->thread))
    goto fail_join;

  Rtos_DeleteThread(pStartCodeChannel->thread);
  AL_TEventQueue_Deinit(pEventQueue);
  AL_ICommunication_Close(scheduler->pCommunication, pStartCodeChannel->hCommunicationLine);
  Rtos_Free(pStartCodeChannel);

  return AL_SUCCESS;

  fail_join:
  AL_TEventQueue_Deinit(pEventQueue);
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
  if(pChannel == NULL)
    return AL_SUCCESS;

  pChannel->bBeingDestroyed = true;
  AL_ERR errorCode = AL_ERROR;

  AL_ECommunicationError const error = AL_ICommunication_PostBlockingMessage(pChannel->pCommunication, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_DESTROY_CHANNEL, NULL);

  if(error != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to destroy channel (error code: %d)\n", error);
    goto exit;
  }

  Rtos_JoinThread(pChannel->thread);
  Rtos_DeleteThread(pChannel->thread);

  errorCode = AL_SUCCESS;

  exit:
  AL_ICommunication_Close(pChannel->pCommunication, pChannel->hCommunicationLine);
  Rtos_Free(pChannel);
  return errorCode;
}

static AL_ERR API_CreateChannel(AL_HANDLE* hChannel, AL_IDecScheduler* pScheduler, AL_TDecScheduler_ChannelParams* pSchedParams)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  AL_ERR errorCode = AL_ERROR;

  Channel* pChannel = Rtos_Malloc(sizeof(*pChannel));

  if(pChannel == NULL)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto create_channel_fail_alloc;
  }

  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  pChannel->hCommunicationLine = AL_ICommunication_Open(scheduler->pCommunication, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open communication while creating channel: %s\n", AL_ECommunicationError_ToString(eError));
    goto create_channel_fail_open;
  }

  pChannel->pCommunication = scheduler->pCommunication;
  pChannel->bBeingDestroyed = false;
  pChannel->iChannelID = -1;
  pChannel->tSchedParams = *pSchedParams;

  struct al5_channel_config msg = { 0 };
  setChannelMsg(&msg.param, pChannel->tSchedParams.pMDChParams);

  AL_ECommunicationError errdrv = AL_ICommunication_PostBlockingMessage(pChannel->pCommunication, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_CONFIG_CHANNEL, &msg);

  if(errdrv != AL_COMMUNICATION_SUCCESS)
  {
    if(errdrv == AL_COMMUNICATION_ERROR_NO_MEMORY)
      errorCode = AL_ERR_NO_MEMORY;

    /* the ioctl might not have been called at all,
     * so the error_code might no be set. leave it to AL_ERROR in this case */
    if(errdrv == AL_COMMUNICATION_ERROR_CHANNEL && msg.status.error_code)
      errorCode = msg.status.error_code;
    goto create_channel_fail_post;
  }

  Rtos_InvalidateCacheMemory(&msg, sizeof(msg));
  Rtos_Assert(msg.status.error_code == AL_SUCCESS);

  /* Retrieve channel id if it was given to us using the param as an output (debug) */
  if(msg.param.size == sizeof(pChannel->iChannelID))
    pChannel->iChannelID = msg.param.opaque[0];

  pChannel->thread = Rtos_CreateThread(&NotificationThread, pChannel);

  if(pChannel->thread == NULL)
    goto create_channel_fail_thread;

  *hChannel = (AL_HANDLE)pChannel;
  return AL_SUCCESS;

  create_channel_fail_thread:
  create_channel_fail_post:
  AL_ICommunication_Close(pChannel->pCommunication, pChannel->hCommunicationLine);
  create_channel_fail_open:
  Rtos_Free(pChannel);
  create_channel_fail_alloc:
  return errorCode;
}

static void setStartCodeParams(struct al5_params* msg, AL_TStartCodeParam const* pStartCodeParam)
{
  Rtos_StaticAssert(sizeof(*pStartCodeParam) <= sizeof(msg->opaque), "Driver sc_param struct is too small");
  msg->size = sizeof(*pStartCodeParam);
  Rtos_Memcpy(msg->opaque, pStartCodeParam, sizeof(*pStartCodeParam));
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

static void setStartCodeAddresses(struct al5_params* msg, AL_TStartCodeBufferAddrs const* pBufAddrs)
{
  Rtos_StaticAssert(sizeof(*pBufAddrs) <= sizeof(msg->opaque), "Driver sc_addresses struct is too small");
  msg->size = sizeof(*pBufAddrs);
  Rtos_Memcpy(msg->opaque, pBufAddrs, sizeof(*pBufAddrs));
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

static void setSearchStartCodeMsg(struct al5_search_sc_msg* search_msg, AL_TStartCodeParam const* pStartCodeParam, AL_TStartCodeBufferAddrs const* pBufAddrs)
{
  setStartCodeParams(&search_msg->param, pStartCodeParam);
  setStartCodeAddresses(&search_msg->buffer_addrs, pBufAddrs);
}

static void API_SearchSC(AL_IDecScheduler* pScheduler, AL_HANDLE hStartCodeChannel, AL_TStartCodeParam* pStartCodeParam, AL_TStartCodeBufferAddrs* pBufAddrs, AL_TDecScheduler_CB_EndStartCode endStartCodeCB)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  AL_TStartCodeChannel* pStartCodeChannel = (AL_TStartCodeChannel*)hStartCodeChannel;
  AL_TEventQueue* pEventQueue = &pStartCodeChannel->queue;
  AL_TEvent* pEvent = Rtos_Malloc(sizeof(*pEvent));

  if(pEvent == NULL)
    goto fail_event;

  AL_TStartCodeMessage* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(pMsg == NULL)
    goto fail_msg;

  pMsg->bEnded = false;
  pMsg->endStartCodeCB = endStartCodeCB;
  pMsg->pCommunication = scheduler->pCommunication;
  pMsg->hCommunicationLine = pStartCodeChannel->hCommunicationLine;

  struct al5_search_sc_msg search_msg = { 0 };
  setSearchStartCodeMsg(&search_msg, pStartCodeParam, pBufAddrs);

  AL_ECommunicationError error = AL_ICommunication_PostBlockingMessage(scheduler->pCommunication, pMsg->hCommunicationLine, AL_COMMUNICATION_MESSAGE_SEARCH_START_CODE, &search_msg);

  if(error != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to search start code (error code: %d)\n", error);
    goto fail_open;
  }

  pEvent->pPrivate = pMsg;
  AL_TEventQueue_Push(pEventQueue, pEvent);

  return;

  fail_open:
  AL_ICommunication_Close(pMsg->pCommunication, pMsg->hCommunicationLine);
  Rtos_Free(pMsg);
  fail_msg:
  Rtos_Free(pEvent);
  fail_event:
  return;
}

// TODO return error
static void API_DecodeOneFrame(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecBuffers* pPictBuffers, AL_TMemDesc* hSliceParam)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  Channel* chan = (Channel*)hChannel;
  AL_TDecChanParam* pChanParam = (AL_TDecChanParam*)chan->tSchedParams.pMDChParams->pVirtualAddr;

  struct al5_decode_msg msg = { 0 };
  AL_TDecBufferAddrs tBufAddrs;
  AL_SetBufferAddrs(&tBufAddrs, pPictBuffers, pChanParam);
  prepareDecodeMessage(&msg, pPictParam, &tBufAddrs, hSliceParam);
  AL_ECommunicationError error = AL_ICommunication_PostBlockingMessage(scheduler->pCommunication, chan->hCommunicationLine, AL_COMMUNICATION_MESSAGE_DECODE_ONE_FRAME, &msg);

  if(error != AL_COMMUNICATION_SUCCESS)
    Rtos_Log(AL_LOG_ERROR, "Failed to decode one frame (error code: %d)\n", error);
}

static void API_DecodeOneSlice(AL_IDecScheduler* pScheduler, AL_HANDLE hChannel, AL_TDecPicParam* pPictParam, AL_TDecBuffers* pPictBuffers, AL_TMemDesc* hSliceParam)
{
  AL_TDecSchedulerMicroblaze* scheduler = (AL_TDecSchedulerMicroblaze*)pScheduler;
  Channel* chan = (Channel*)hChannel;
  AL_TDecChanParam* pChanParam = (AL_TDecChanParam*)chan->tSchedParams.pMDChParams->pVirtualAddr;

  struct al5_decode_msg msg = { 0 };
  AL_TDecBufferAddrs tBufAddrs;
  AL_SetBufferAddrs(&tBufAddrs, pPictBuffers, pChanParam);
  prepareDecodeMessage(&msg, pPictParam, &tBufAddrs, hSliceParam);
  AL_ECommunicationError const error = AL_ICommunication_PostBlockingMessage(scheduler->pCommunication, chan->hCommunicationLine, AL_COMMUNICATION_MESSAGE_DECODE_ONE_SLICE, &msg);

  if(error != AL_COMMUNICATION_SUCCESS)
    Rtos_Log(AL_LOG_ERROR, "Failed to decode one slice (error code: %d)\n", error);
}

static void GetSchedulerCoreInfo(AL_TDecSchedulerMicroblaze const* pThis, AL_TIDecSchedulerCore* pCore)
{
  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  AL_HANDLE hCommunicationLine = AL_ICommunication_Open(pThis->pCommunication, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open the communication while getting core info: %s\n", AL_ECommunicationError_ToString(eError));
    return;
  }

  struct al5_params msg;
  AL_EDecSchedulerInfo eInfo = AL_DEC_SCHEDULER_CORE;
  msg.opaque[0] = eInfo;
  Rtos_Memcpy(&msg.opaque[sizeof(eInfo) / sizeof(*msg.opaque)], pCore, sizeof(*pCore));

  Rtos_StaticAssert(sizeof(eInfo) + sizeof(*pCore) <= sizeof(msg.opaque), "Driver core structure struct is too small");
  msg.size = sizeof(eInfo) + sizeof(*pCore);

  AL_ECommunicationError const error = AL_ICommunication_PostBlockingMessage(pThis->pCommunication, hCommunicationLine, AL_COMMUNICATION_MESSAGE_GET, &msg);

  if(error != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to get parameter '%s', (error code: '%d')\n", ToStringIDecSchedulerInfo(AL_IDECSCHEDULER_CORE), error);
    AL_ICommunication_Close(pThis->pCommunication, hCommunicationLine);
    return;
  }

  Rtos_InvalidateCacheMemory(&msg.opaque[1], sizeof(*pCore));
  Rtos_Memcpy(pCore, &msg.opaque[1], sizeof(*pCore));

  AL_ICommunication_Close(pThis->pCommunication, hCommunicationLine);
}

/******************************************************************************/
static void GetSchedulerVersion(AL_TDecSchedulerMicroblaze const* pThis, AL_TIDecSchedulerVersion* pVersion)
{
  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  AL_HANDLE hCommunicationLine = AL_ICommunication_Open(pThis->pCommunication, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open the communication while getting scheduler version: %s\n", AL_ECommunicationError_ToString(eError));
    return;
  }

  struct al5_params msg;
  AL_EDecSchedulerInfo eInfo = AL_DEC_SCHEDULER_VERSION;
  msg.opaque[0] = eInfo;
  Rtos_Memcpy(&msg.opaque[sizeof(eInfo) / sizeof(*msg.opaque)], pVersion, sizeof(*pVersion));

  Rtos_StaticAssert(sizeof(eInfo) + sizeof(*pVersion) <= sizeof(msg.opaque), "Driver version structure struct is too small");
  msg.size = sizeof(eInfo) + sizeof(*pVersion);

  AL_ECommunicationError const error = AL_ICommunication_PostBlockingMessage(pThis->pCommunication, hCommunicationLine, AL_COMMUNICATION_MESSAGE_GET, &msg);

  if(error != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to get parameter '%s', (error code: '%d')\n", ToStringIDecSchedulerInfo(AL_IDECSCHEDULER_VERSION), error);
    AL_ICommunication_Close(pThis->pCommunication, hCommunicationLine);
    return;
  }

  Rtos_InvalidateCacheMemory(&msg.opaque[1], sizeof(*pVersion));
  Rtos_Memcpy(pVersion, &msg.opaque[1], sizeof(*pVersion));

  AL_ICommunication_Close(pThis->pCommunication, hCommunicationLine);
}

static void API_Get(AL_IDecScheduler const* pScheduler, AL_EIDecSchedulerInfo info, void* pParam)
{
  AL_TDecSchedulerMicroblaze const* pThis = (AL_TDecSchedulerMicroblaze const*)pScheduler;
  switch(info)
  {
  case AL_IDECSCHEDULER_VERSION:
  {
    GetSchedulerVersion(pThis, (AL_TIDecSchedulerVersion*)pParam);
    return;
  }
  case AL_IDECSCHEDULER_CORE:
  {
    GetSchedulerCoreInfo(pThis, (AL_TIDecSchedulerCore*)pParam);
    return;
  }
  default: return;
  }

  return;
}

static void API_Set(AL_IDecScheduler* pScheduler, AL_EIDecSchedulerInfo info, void const* pParam)
{
  (void)pParam;
  AL_TDecSchedulerMicroblaze* pThis = (AL_TDecSchedulerMicroblaze*)pScheduler;
  (void)pThis;
  switch(info)
  {
  default: return;
  }

  return;
}

/******************************************************************************/
static void API_InternalSet(AL_IDecScheduler* pScheduler, AL_EIDecSchedulerInternalInfo eInfo, void const* pParam)
{
  (void)pScheduler;
  (void)pParam;
  switch(eInfo)
  {
  case AL_IDECSCHEDULERINTERNAL_MAX_ENUM:
  default:
    return;
  }
}

static AL_IDecSchedulerVtable const DecSchedulerMcuVtable =
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
  API_InternalSet,
};

/* Initialisation cannot be handled by control software with mcu.
 * With several channels it would be unacceptable to let any reinit the scheduler.
 * We init the scheduler once in the firmware at startup. */

AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_ICommunication* pCommunication)
{
  if(pCommunication == NULL)
    return NULL;

  AL_TDecSchedulerMicroblaze* scheduler = Rtos_Malloc(sizeof(*scheduler));

  if(scheduler == NULL)
    return NULL;

  scheduler->vtable = &DecSchedulerMcuVtable;
  scheduler->pCommunication = pCommunication;

  return (AL_IDecScheduler*)scheduler;
}

#else

AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_ICommunication* pCommunication)
{
  (void)pCommunication;
  return NULL;
}

#endif
/*!@}*/
