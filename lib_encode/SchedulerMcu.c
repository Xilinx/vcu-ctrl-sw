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

#include "lib_encode/IScheduler.h"
#include "lib_encode/driverInterface.h"
#include "lib_encode/SchedulerMcu.h"
#include "lib_encode/ISchedulerCommon.h"

#include <stdlib.h>
#include <string.h>

#include "lib_rtos/lib_rtos.h"
#include "lib_fpga/DmaAlloc.h"

typedef struct al_t_SchedulerMcu
{
  const TSchedulerVtable* vtable;
  AL_TAllocator* allocator;
  Driver* driver;
}AL_TSchedulerMcu;

typedef struct
{
  AL_TISchedulerCallBacks CBs;
  AL_TCommonChannelInfo info;
  Driver* driver;
  int fd;
  AL_THREAD thread;
  bool shouldContinue;
}Channel;

bool AL_SchedulerMcu_Destroy(AL_TSchedulerMcu* schedulerMcu);

#if __linux__

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "lib_fpga/DmaAllocLinux.h"
#include "allegro_ioctl_mcu_enc.h"
#include "driverDataConversions.h"

#define DCACHE_OFFSET 0x80000000

static void updateChannelParam(AL_TEncChanParam* pChParam, struct al5_channel_status* msg);
static void setCallbacks(Channel* chan, AL_TISchedulerCallBacks* pCBs);
static bool getStatusMsg(Channel* chan, struct al5_params* msg);
static void processStatusMsg(Channel* chan, struct al5_params* msg, AL_TEncPicStatus* status);
static void* WaitForStatus(void* p);

static AL_HANDLE createChannel(TScheduler* pScheduler, AL_TEncChanParam* pChParam, AL_PADDR pEP1, AL_TISchedulerCallBacks* pCBs)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;

  Channel* chan = calloc(1, sizeof(Channel));

  if(!chan)
    goto fail;

  chan->driver = schedulerMcu->driver;
  chan->fd = AL_Driver_Open(chan->driver);

  if(chan->fd < 0)
  {
    perror("Can't open driver");
    goto fail;
  }

  struct al5_config_channel msg = { 0 };
  setConfigChannelMsg(&msg, pChParam, pEP1);
  bool bRet = AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_CONFIG_CHANNEL, &msg);

  if(!bRet)
  {
    perror("Failed to create channel");
    printf("error: %d\n", msg.status.error_code);
    goto fail;
  }

  assert(msg.status.error_code == 0);
  updateChannelParam(pChParam, &msg.status);

  setCallbacks(chan, pCBs);

  chan->shouldContinue = true;

  chan->thread = Rtos_CreateThread(&WaitForStatus, chan);

  if(!chan->thread)
    goto fail;

  SetChannelInfo(&chan->info, pChParam);

  return chan;

  fail:

  if(chan && chan->fd >= 0)
    AL_Driver_Close(schedulerMcu->driver, chan->fd);
  free(chan);
  return AL_INVALID_CHANNEL;
}

static bool encodeOneFrame(TScheduler* pScheduler, AL_HANDLE hChannel, AL_TEncInfo* pEncInfo, AL_TEncRequestInfo* pReqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;
  struct al5_encode_msg msg = { 0 };

  if(!pEncInfo || !pReqInfo || !pBuffersAddrs)
  {
    msg.params.size = 0;
    msg.addresses.size = 0;
  }
  else
  {
    pBuffersAddrs->pEP2_v = pBuffersAddrs->pEP2 + DCACHE_OFFSET;
    setEncodeMsg(&msg, pEncInfo, pReqInfo, pBuffersAddrs);
  }

  return AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_ENCODE_ONE_FRM, &msg);
}

static bool destroyChannel(TScheduler* pScheduler, AL_HANDLE hChannel)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;

  chan->shouldContinue = false;

  AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_DESTROY_CHANNEL, NULL);

  if(!Rtos_JoinThread(chan->thread))
    return false;
  Rtos_DeleteThread(chan->thread);

  AL_Driver_Close(schedulerMcu->driver, chan->fd);

  free(chan);

  return true;
}

static bool getRecPicture(TScheduler* pScheduler, AL_HANDLE hChannel, TRecPic* pRecPic)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;
  struct al5_reconstructed_info msg = { 0 };
  bool isSuccess = AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_GET_REC_PICTURE, &msg);

  if(!isSuccess)
    return false;

  AL_TAllocator* pAllocator = schedulerMcu->allocator;
  AL_HANDLE hRecBuf = AL_LinuxDmaAllocator_ImportFromFd((AL_TLinuxDmaAllocator*)pAllocator, msg.fd);

  if(!hRecBuf)
    return false;

  AL_TReconstructedInfo recInfo;
  recInfo.uID = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pAllocator, hRecBuf);
  recInfo.ePicStruct = msg.pic_struct;
  recInfo.iPOC = msg.poc;

  SetRecPic(pRecPic, pAllocator, hRecBuf, &chan->info, &recInfo);

  return true;
}

static bool releaseRecPicture(TScheduler* pScheduler, AL_HANDLE hChannel, TRecPic* pRecPic)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;
  AL_HANDLE hRecBuf = pRecPic->tBuf.tMD.hAllocBuf;

  if(!hRecBuf)
    return false;

  AL_TAllocator* pAllocator = schedulerMcu->allocator;
  __u32 fd = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pAllocator, hRecBuf);

  if(!AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_RELEASE_REC_PICTURE, &fd))
    return false;
  AL_Allocator_Free(pAllocator, hRecBuf);
  return true;
}

static void updateChannelParam(AL_TEncChanParam* pChParam, struct al5_channel_status* msg)
{
  pChParam->eOptions = msg->options;
  pChParam->uNumCore = msg->num_core;
  pChParam->uPpsParam = msg->pps_param;
}

static void setCallbacks(Channel* chan, AL_TISchedulerCallBacks* pCBs)
{
  chan->CBs.pEndEncodingCBParam = pCBs->pEndEncodingCBParam;
  chan->CBs.pfnEndEncodingCallBack = pCBs->pfnEndEncodingCallBack;
}

static bool getStatusMsg(Channel* chan, struct al5_params* msg)
{
  return AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_WAIT_FOR_STATUS, msg);
}

static void processStatusMsg(Channel* chan, struct al5_params* msg, AL_TEncPicStatus* status)
{
  assert(msg->size == 8 + sizeof(*status));
  AL_PTR64 streamBufferPtr;
  memcpy(&streamBufferPtr, msg->opaque_params, 8);
  memcpy(status, msg->opaque_params + 2, sizeof(*status));

  chan->CBs.pfnEndEncodingCallBack(chan->CBs.pEndEncodingCBParam, status, (AL_TBuffer*)(uintptr_t)streamBufferPtr);
}

static void* WaitForStatus(void* p)
{
  Channel* chan = p;
  struct al5_params msg = { 0 };
  AL_TEncPicStatus status;

  while(true)
  {
    if(!chan->shouldContinue)
      break;

    if(getStatusMsg(chan, &msg))
      processStatusMsg(chan, &msg, &status);
  }

  return 0;
}

static bool destroy(TScheduler* pScheduler)
{
  return AL_SchedulerMcu_Destroy((AL_TSchedulerMcu*)pScheduler);
}

static __u32 getFd(AL_TBuffer* b)
{
  return (__u32)AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)b->pAllocator, b->hBuf);
}

static bool putStreamBuffer(TScheduler* pScheduler, AL_HANDLE hChannel, AL_TBuffer* streamBuffer, uint32_t uOffset)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = (Channel*)hChannel;
  struct al5_buffer driverBuffer;
  Rtos_Memset(&driverBuffer, 0, sizeof(driverBuffer));

  if(streamBuffer == NULL)
    return false;

  driverBuffer.handle = getFd(streamBuffer);
  driverBuffer.offset = uOffset;
  driverBuffer.stream_buffer_ptr = (AL_64U)(uintptr_t)streamBuffer;
  driverBuffer.size = AL_Buffer_GetSizeData(streamBuffer);

  return AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_PUT_STREAM_BUFFER, &driverBuffer);
}


static const TSchedulerVtable McuSchedulerVtable =
{
  &destroy,
  &createChannel,
  &destroyChannel,
  &encodeOneFrame,
  &putStreamBuffer,
  &getRecPicture,
  &releaseRecPicture,
};

TScheduler* AL_SchedulerMcu_Create(Driver* driver, AL_TAllocator* pDmaAllocator)
{
  AL_TSchedulerMcu* scheduler = Rtos_Malloc(sizeof(*scheduler));

  if(!scheduler)
    return NULL;
  scheduler->vtable = &McuSchedulerVtable;
  scheduler->driver = driver;
  scheduler->allocator = pDmaAllocator;
  return (TScheduler*)scheduler;
}

bool AL_SchedulerMcu_Destroy(AL_TSchedulerMcu* schedulerMcu)
{
  free(schedulerMcu);

  return true;
}

#else

TScheduler* AL_SchedulerMcu_Create(Driver* driver, AL_TAllocator* pDmaAllocator)
{
  (void)driver;
  (void)pDmaAllocator;
  return NULL;
}

#endif

