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

#include "lib_decode/I_DecChannel.h"

#if __linux__

#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "allegro_ioctl_mcu_dec.h"
#include "lib_common/List.h"

#define DCACHE_OFFSET 0x80000000

/****************************************************************************/
const char* deviceFile = "/dev/allegroDecodeIP";

/****************************************************************************/

typedef struct
{
  pthread_cond_t Cond;
  pthread_mutex_t Lock;
}AL_WaitQueue;

static
int AL_WakeUp(AL_WaitQueue* pQueue)
{
  pthread_mutex_lock(&pQueue->Lock);
  int ret = pthread_cond_broadcast(&pQueue->Cond);
  pthread_mutex_unlock(&pQueue->Lock);

  return ret;
}

static
int AL_WaitEvent(AL_WaitQueue* pQueue, bool (* isReady)(void*), void* pParam)
{
  pthread_mutex_lock(&pQueue->Lock);

  while(!isReady(pParam))
    pthread_cond_wait(&pQueue->Cond, &pQueue->Lock);

  pthread_mutex_unlock(&pQueue->Lock);

  return 0;
}

static
void AL_WaitQueue_Init(AL_WaitQueue* pQueue)
{
  pthread_mutex_init(&pQueue->Lock, NULL);
  pthread_cond_init(&pQueue->Cond, NULL);
}

static
void AL_WaitQueue_Deinit(AL_WaitQueue* pQueue)
{
  pthread_mutex_destroy(&pQueue->Lock);
  pthread_cond_destroy(&pQueue->Cond);
}

/****************************************************************************/
typedef struct
{
  int fd;
  AL_THREAD thread;
  bool bBeingDestroyed;

  AL_CB_EndFrameDecoding endFrameDecodingCB;
}Channel;

typedef struct
{
  int fd;
  AL_CB_EndStartCode endStartCodeCB;
  bool bEnded;
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
  AL_EventQueue EventQueue;
}StartCodeEventQueue;

struct DecChanMcuCtx
{
  const AL_TIDecChannelVtable* vtable;
  AL_THREAD pSCThread;
  StartCodeEventQueue SCQueue;
  Channel chan;
  bool chanIsConfigured;
};

int AL_EventQueue_Init(AL_EventQueue* pEventQueue)
{
  AL_WaitQueue_Init(&pEventQueue->Queue);
  AL_ListHeadInit(&pEventQueue->List);
  pthread_mutex_init(&pEventQueue->Lock, NULL);
  return 0;
}

int AL_EventQueue_Deinit(AL_EventQueue* pEventQueue)
{
  pthread_mutex_destroy(&pEventQueue->Lock);
  AL_WaitQueue_Deinit(&pEventQueue->Queue);
  return 0;
}

int AL_EventQueue_Push(AL_EventQueue* pEventQueue, AL_Event* pEvent)
{
  pthread_mutex_lock(&pEventQueue->Lock);
  AL_ListAddTail(&pEvent->List, &pEventQueue->List);
  pthread_mutex_unlock(&pEventQueue->Lock);

  AL_WakeUp(&pEventQueue->Queue);
  return 0;
}

/* for one Reader */
int AL_EventQueue_Fetch(AL_EventQueue* EventQueue, AL_Event** pEvent, bool (* isReady)(void*), void* pParam)
{
  AL_WaitEvent(&EventQueue->Queue, isReady, pParam);

  /* get msg */
  pthread_mutex_lock(&EventQueue->Lock);
  *pEvent = AL_ListFirstEntry(&EventQueue->List, AL_Event, List);
  AL_ListDel(&(*pEvent)->List);
  pthread_mutex_unlock(&EventQueue->Lock);

  return 0;
}

void setPictParam(struct al5_params* msg, AL_TDecPicParam* pPictParam)
{
  static_assert(sizeof(*pPictParam) <= sizeof(msg->opaque), "Driver pict_param struct is too small");
  msg->size = sizeof(*pPictParam);
  memcpy(msg->opaque, pPictParam, msg->size);
}

void setPictBufferAddrs(struct al5_params* msg, AL_TDecPicBufferAddrs* pBufferAddrs)
{
  static_assert(sizeof(*pBufferAddrs) <= sizeof(msg->opaque), "Driver pict_buffer_addrs struct is too small");
  msg->size = sizeof(*pBufferAddrs);
  memcpy(msg->opaque, pBufferAddrs, msg->size);
}

static bool PostMessage(int fd, int iMsgID, void* pData)
{
  /* must keep the errno from ioctl */
  int iRet;
  bool bDone = false;

  while(!bDone)
  {
    iRet = ioctl(fd, iMsgID, pData);

    /* if a timeout occured, retry */
    if(iRet < 0 && errno == EAGAIN)
      continue;
    bDone = true;
  }

  return iRet >= 0;
}

/* Fill msg with pChParam */
static
void setChannelMsg(struct al5_params* msg, const AL_TDecChanParam* pChParam)
{
  static_assert(sizeof(*pChParam) <= sizeof(msg->opaque), "Driver channel_param struct is too small");
  msg->size = sizeof(*pChParam);
  memcpy(msg->opaque, pChParam, msg->size);
}

static
bool getStatusMsg(Channel* chan, struct al5_params* msg)
{
  bool bRet = PostMessage(chan->fd, AL_MCU_WAIT_FOR_STATUS, msg);

  if(!bRet && !chan->bBeingDestroyed)
    perror("Failed to get decode status");

  return bRet;
}

static
bool getScStatusMsg(SCMsg* pMsg, struct al5_scstatus* StatusMsg)
{
  return PostMessage(pMsg->fd, AL_MCU_WAIT_FOR_START_CODE, StatusMsg);
}

static void processStatusMsg(Channel* chan, struct al5_params* msg)
{
  AL_TDecPicStatus status;
  memcpy(&status, msg->opaque, msg->size);
  chan->endFrameDecodingCB.func(chan->endFrameDecodingCB.userParam, &status);
}

static void* NotificationThread(void* p)
{
  Channel* chan = p;

  for(;;)
  {
    struct al5_params msg = { 0 };
    bool bRet = getStatusMsg(chan, &msg);

    if(!bRet)
      break;

    processStatusMsg(chan, &msg);
  }

  return 0;
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
  StartCodeEventQueue* pSCQueue = p;
  AL_EventQueue* pEventQueue = &pSCQueue->EventQueue;
  struct al5_scstatus StatusMsg = { 0 };

  while(true)
  {
    AL_Event* pEvent;
    AL_EventQueue_Fetch(pEventQueue, &pEvent, isSCReady, pEventQueue);
    SCMsg* pMsg = pEvent->pPriv;

    if(pMsg->bEnded)
    {
      Rtos_Free(pMsg);
      break;
    }

    bool bRet = getScStatusMsg(pMsg, &StatusMsg);

    if(bRet)
      processScStatusMsg(pMsg, &StatusMsg);

    close(pMsg->fd);
    Rtos_Free(pMsg);
  }

  return NULL;
}

/* Update some values of pChParam set by MCU */
static void getParamUpdateByMcu(const struct al5_channel_status* msg, AL_TDecChanParam* pChParam)
{
  pChParam->uNumCore = msg->num_core;
}

/****************************************************************************/
/** Initialisation cannot be handled by control software with mcu.
 *  With several channels it would be unacceptable to let any reinit scheduler.
 *
 * Scheduler initialisation is done once, on mcu start.
 */
static bool DecChannelMcu_Init(struct DecChanMcuCtx* decChanMcu)
{
  StartCodeEventQueue* SCQueue = &decChanMcu->SCQueue;
  decChanMcu->chanIsConfigured = false;

  AL_EventQueue_Init(&SCQueue->EventQueue);

  decChanMcu->pSCThread = Rtos_CreateThread(&ScNotificationThread, SCQueue);

  if(!decChanMcu->pSCThread)
  {
    perror("Couldn't create thread");
    return false;
  }

  return true;
}

/****************************************************************************/
static bool DecChannelMcu_DestroyChannel(Channel* chan)
{
  chan->bBeingDestroyed = true;

  bool bRet = PostMessage(chan->fd, AL_MCU_DESTROY_CHANNEL, NULL);

  if(!bRet)
  {
    perror("Failed to destroy channel");
    goto exit;
  }

  Rtos_JoinThread(chan->thread);
  Rtos_DeleteThread(chan->thread);

  exit:
  close(chan->fd);

  return bRet;
}

/****************************************************************************/
static void DecChannelMcu_Destroy(AL_TIDecChannel* pDecChannel)
{
  struct DecChanMcuCtx* decChanMcu = (struct DecChanMcuCtx*)pDecChannel;
  StartCodeEventQueue* SCQueue = &decChanMcu->SCQueue;

  AL_Event* pEvent = Rtos_Malloc(sizeof(*pEvent));

  if(!pEvent)
    goto fail_event;
  SCMsg* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(!pMsg)
    goto fail_msg;

  pMsg->bEnded = true;
  pEvent->pPriv = pMsg;
  AL_EventQueue_Push(&SCQueue->EventQueue, pEvent);

  if(decChanMcu->chanIsConfigured)
    DecChannelMcu_DestroyChannel(&decChanMcu->chan);

  if(!Rtos_JoinThread(decChanMcu->pSCThread))
    goto fail_join;

  Rtos_DeleteThread(decChanMcu->pSCThread);

  AL_EventQueue_Deinit(&SCQueue->EventQueue);

  return;

  fail_join:
  AL_EventQueue_Deinit(&SCQueue->EventQueue);
  fail_msg:
  Rtos_Free(pEvent);
  fail_event:
  return;
}

/****************************************************************************/
static bool DecChannelMcu_ConfigChannel(AL_TIDecChannel* pDecChannel, AL_TDecChanParam* pChParam, AL_TIDecChannelCallbacks* pCBs)
{
  struct DecChanMcuCtx* decChanMcu = (struct DecChanMcuCtx*)pDecChannel;
  struct al5_channel_config msg = { 0 };
  bool bRet;

  Channel* chan = &decChanMcu->chan;

  chan->bBeingDestroyed = false;
  chan->endFrameDecodingCB = pCBs->endFrameDecodingCB;

  chan->fd = open(deviceFile, O_RDWR);

  if(chan->fd < 0)
  {
    printf("Cannot open device file %s: %s\n", deviceFile, strerror(errno));
    goto fail_open;
  }

  setChannelMsg(&msg.param, pChParam);
  bRet = PostMessage(chan->fd, AL_MCU_CONFIG_CHANNEL, &msg);

  if(!bRet)
  {
    perror("Failed to config channel");
    goto fail_open;
  }

  getParamUpdateByMcu(&msg.status, pChParam);

  chan->thread = Rtos_CreateThread(&NotificationThread, chan);

  if(!chan->thread)
    goto fail_open;

  decChanMcu->chanIsConfigured = true;
  return true;

  fail_open:

  if(chan->fd >= 0)
    close(chan->fd);
  return false;
}

static void setStartCodeParams(struct al5_params* msg, AL_TScParam* pScParam)
{
  static_assert(sizeof(*pScParam) <= sizeof(msg->opaque), "Driver sc_param struct is too small");
  msg->size = sizeof(*pScParam);
  memcpy(msg->opaque, pScParam, sizeof(*pScParam));
}

static void setStartCodeAddresses(struct al5_params* msg, AL_TScBufferAddrs* pBufAddrs)
{
  static_assert(sizeof(*pBufAddrs) <= sizeof(msg->opaque), "Driver sc_addresses struct is too small");
  msg->size = sizeof(*pBufAddrs);
  memcpy(msg->opaque, pBufAddrs, sizeof(*pBufAddrs));
}

static void setSearchStartCodeMsg(struct al5_search_sc_msg* search_msg, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufAddrs)
{
  setStartCodeParams(&search_msg->param, pScParam);
  setStartCodeAddresses(&search_msg->buffer_addrs, pBufAddrs);
}

/****************************************************************************/
static void DecChannelMcu_SearchSC(AL_TIDecChannel* pDecChannel, AL_TScParam* pScParam, AL_TScBufferAddrs* pBufAddrs, AL_CB_EndStartCode endStartCodeCB)
{
  struct DecChanMcuCtx* decChanMcu = (struct DecChanMcuCtx*)pDecChannel;
  struct al5_search_sc_msg search_msg = { 0 };
  AL_EventQueue* pEventQueue = &decChanMcu->SCQueue.EventQueue;
  bool bRet;
  AL_Event* pEvent = Rtos_Malloc(sizeof(*pEvent));
  SCMsg* pMsg = Rtos_Malloc(sizeof(*pMsg));

  if(!pEvent)
    goto fail_event;

  if(!pMsg)
    goto fail_msg;

  pMsg->bEnded = false;
  pMsg->endStartCodeCB = endStartCodeCB;

  pMsg->fd = open(deviceFile, O_RDWR);

  if(pMsg->fd < 0)
  {
    perror("Cannot open device file");
    printf("%s\n", deviceFile);
    goto fail_open;
  }

  setSearchStartCodeMsg(&search_msg, pScParam, pBufAddrs);
  bRet = PostMessage(pMsg->fd, AL_MCU_SEARCH_START_CODE, &search_msg);

  if(!bRet)
  {
    perror("Failed to search start code");
    goto fail_open;
  }

  pEvent->pPriv = pMsg;
  AL_EventQueue_Push(pEventQueue, pEvent);

  return;

  fail_open:
  close(pMsg->fd);
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

/****************************************************************************/
// TODO return error
static void DecChannelMcu_DecodeOneFrame(AL_TIDecChannel* pDecChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  struct DecChanMcuCtx* decChanMcu = (struct DecChanMcuCtx*)pDecChannel;
  Channel* chan = &decChanMcu->chan;

  if(chan == AL_INVALID_CHANNEL || chan == AL_UNINITIALIZED_CHANNEL)
    return;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);

  if(!PostMessage(chan->fd, AL_MCU_DECODE_ONE_FRM, &msg))
    perror("Failed to decode");
}

/****************************************************************************/
static void DecChannelMcu_DecodeOneSlice(AL_TIDecChannel* pDecChannel, AL_TDecPicParam* pPictParam, AL_TDecPicBufferAddrs* pPictAddrs, TMemDesc* hSliceParam)
{
  struct DecChanMcuCtx* decChanMcu = (struct DecChanMcuCtx*)pDecChannel;
  Channel* chan = &decChanMcu->chan;

  if(chan == AL_INVALID_CHANNEL || chan == AL_UNINITIALIZED_CHANNEL)
    return;

  struct al5_decode_msg msg = { 0 };
  prepareDecodeMessage(&msg, pPictParam, pPictAddrs, hSliceParam);

  if(!PostMessage(chan->fd, AL_MCU_DECODE_ONE_SLICE, &msg))
    perror("Failed to decode");
}


/******************************************************************************/
static const AL_TIDecChannelVtable DecChannelMcu =
{
  DecChannelMcu_Destroy,
  DecChannelMcu_ConfigChannel,
  DecChannelMcu_SearchSC,
  DecChannelMcu_DecodeOneFrame,
  DecChannelMcu_DecodeOneSlice,
};

AL_TIDecChannel* AL_DecChannelMcu_Create()
{
  struct DecChanMcuCtx* decChannel = Rtos_Malloc(sizeof(*decChannel));

  if(!decChannel)
    return NULL;
  decChannel->vtable = &DecChannelMcu;
  DecChannelMcu_Init(decChannel);
  return (AL_TIDecChannel*)decChannel;
}

#else

AL_TIDecChannel* AL_DecChannelMcu_Create()
{
  return NULL;
}

#endif
/*@}*/

