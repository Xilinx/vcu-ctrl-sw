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

#include "lib_encode/IScheduler.h"
#include "lib_encode/SchedulerMcu.h"
#include "lib_encode/ISchedulerCommon.h"
#include "lib_common/IDriver.h"

#include "lib_rtos/lib_rtos.h"
#include "lib_fpga/DmaAlloc.h"
#include "lib_common/Error.h"

typedef struct al_t_SchedulerMcu
{
  const TSchedulerVtable* vtable;
  AL_TAllocator* allocator;
  AL_TDriver* driver;
}AL_TSchedulerMcu;

typedef struct
{
  AL_TISchedulerCallBacks CBs;
  AL_TCommonChannelInfo info;
  AL_TDriver* driver;
  int fd;
  AL_THREAD thread;
  int32_t shouldContinue;
  bool outputRec;
}Channel;

#if __linux__

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h> // for close

#include "lib_fpga/DmaAllocLinux.h"
#include "allegro_ioctl_mcu_enc.h"
#include "DriverDataConversions.h"

static const char* deviceFile = "/dev/allegroIP";
static void* WaitForStatus(void* p);

static void setChannelFeedback(AL_TEncChanParam* pChParam, struct al5_channel_status* msg)
{
  pChParam->eEncOptions = msg->options;
  pChParam->eEncTools = msg->tools;
  pChParam->uNumCore = msg->num_core;
  pChParam->uPpsParam = msg->pps_param;
}

static void setCallbacks(Channel* chan, AL_TISchedulerCallBacks* pCBs)
{
  chan->CBs.pEndEncodingCBParam = pCBs->pEndEncodingCBParam;
  chan->CBs.pfnEndEncodingCallBack = pCBs->pfnEndEncodingCallBack;
}

static AL_ERR createChannel(AL_HANDLE* hChannel, TScheduler* pScheduler, AL_TEncChanParam* pChParam, TMemDesc* pEP1, AL_TISchedulerCallBacks* pCBs)
{
  AL_ERR errorCode = AL_ERROR;
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;

  Channel* chan = Rtos_Malloc(sizeof(*chan));

  if(!chan)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  Rtos_Memset(chan, 0, sizeof(*chan));

  chan->driver = schedulerMcu->driver;
  chan->fd = AL_Driver_Open(chan->driver, deviceFile);

  if(chan->fd < 0)
  {
    perror("Can't open driver");
    goto driver_open_fail;
  }

  struct al5_channel_config msg = { 0 };
  setChannelParam(&msg.param, pChParam, pEP1);
  chan->outputRec = pChParam->eEncOptions & AL_OPT_FORCE_REC;

  AL_EDriverError errdrv = AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_CONFIG_CHANNEL, &msg);

  if(errdrv != DRIVER_SUCCESS)
  {
    if(errdrv == DRIVER_ERROR_NO_MEMORY)
      errorCode = AL_ERR_NO_MEMORY;

    /* the ioctl might not have been called at all,
     * so the error_code might no be set. leave it to AL_ERROR in this case */
    if((errdrv == DRIVER_ERROR_CHANNEL) && (msg.status.error_code != 0))
      errorCode = msg.status.error_code;

    goto fail;
  }

  assert(msg.status.error_code == 0);

  setChannelFeedback(pChParam, &msg.status);
  setCallbacks(chan, pCBs);
  chan->shouldContinue = 1;
  chan->thread = Rtos_CreateThread(&WaitForStatus, chan);

  if(!chan->thread)
    goto fail;

  SetChannelInfo(&chan->info, pChParam);
  *hChannel = (AL_HANDLE)chan;
  return AL_SUCCESS;

  fail:
  AL_Driver_Close(schedulerMcu->driver, chan->fd);
  driver_open_fail:
  Rtos_Free(chan);
  channel_creation_fail:
  *hChannel = AL_INVALID_CHANNEL;
  return errorCode;
}

static void createEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* pEncInfo, AL_TEncRequestInfo* pReqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  if(!pEncInfo || !pReqInfo || !pBuffersAddrs)
  {
    msg->params.size = 0;
    msg->addresses.size = 0;
  }
  else
  {
    if(pBuffersAddrs->pEP2)
      pBuffersAddrs->pEP2_v = pBuffersAddrs->pEP2 + DCACHE_OFFSET;
    else
      pBuffersAddrs->pEP2_v = 0;
    setEncodeMsg(msg, pEncInfo, pReqInfo, pBuffersAddrs);
  }
}

static bool encodeOneFrame(TScheduler* pScheduler, AL_HANDLE hChannel, AL_TEncInfo* pEncInfo, AL_TEncRequestInfo* pReqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;
  struct al5_encode_msg msg = { 0 };
  createEncodeMsg(&msg, pEncInfo, pReqInfo, pBuffersAddrs);
  return AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_ENCODE_ONE_FRM, &msg) == DRIVER_SUCCESS;
}

static bool destroyChannel(TScheduler* pScheduler, AL_HANDLE hChannel)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;

  if(!chan)
    return false;

  Rtos_AtomicDecrement(&chan->shouldContinue);

  AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_DESTROY_CHANNEL, NULL);

  if(!Rtos_JoinThread(chan->thread))
    return false;
  Rtos_DeleteThread(chan->thread);

  AL_Driver_Close(schedulerMcu->driver, chan->fd);

  Rtos_Free(chan);

  return true;
}

static bool getRecPicture(TScheduler* pScheduler, AL_HANDLE hChannel, TRecPic* pRecPic)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;
  struct al5_reconstructed_info msg = { 0 };

  if(!chan->outputRec)
    return false;

  if(AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_GET_REC_PICTURE, &msg) != DRIVER_SUCCESS)
    return false;

  AL_TAllocator* pAllocator = schedulerMcu->allocator;
  AL_HANDLE hRecBuf = AL_LinuxDmaAllocator_ImportFromFd((AL_TLinuxDmaAllocator*)pAllocator, msg.fd);

  if(!hRecBuf)
    return false;

  AL_TReconstructedInfo recInfo;
  recInfo.uID = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pAllocator, hRecBuf);
  recInfo.ePicStruct = msg.pic_struct;
  recInfo.iPOC = msg.poc;

  recInfo.tPicDim.iWidth = msg.width;
  recInfo.tPicDim.iHeight = msg.height;

  SetRecPic(pRecPic, pAllocator, hRecBuf, &chan->info, &recInfo);

  return true;
}

static bool releaseRecPicture(TScheduler* pScheduler, AL_HANDLE hChannel, TRecPic* pRecPic)
{
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = hChannel;

  if(!pRecPic->pBuf || !chan->outputRec)
    return false;

  AL_HANDLE hRecBuf = pRecPic->pBuf->hBuf;
  AL_TAllocator* pAllocator = schedulerMcu->allocator;
  __u32 fd = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pAllocator, hRecBuf);

  if(AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_RELEASE_REC_PICTURE, &fd) != DRIVER_SUCCESS)
    return false;

  AL_Allocator_Free(pAllocator, hRecBuf);
  close(fd);

  return true;
}

static bool getStatusMsg(Channel* chan, struct al5_params* msg)
{
  return AL_Driver_PostMessage(chan->driver, chan->fd, AL_MCU_WAIT_FOR_STATUS, msg) == DRIVER_SUCCESS;
}

static void processStatusMsg(Channel* chan, struct al5_params* msg)
{
  assert(msg->size >= sizeof(AL_PTR64));
  AL_PTR64 streamBufferPtr;
  Rtos_Memcpy(&streamBufferPtr, msg->opaque_params, sizeof(AL_PTR64));
  AL_TEncPicStatus* pStatus = NULL;
  AL_TEncPicStatus status;

  if(msg->size > sizeof(AL_PTR64))
  {
    assert(msg->size == sizeof(AL_PTR64) + sizeof(AL_TEncPicStatus));
    Rtos_Memcpy(&status, (char*)msg->opaque_params + sizeof(AL_PTR64), sizeof(status));
    pStatus = &status;
  }

  chan->CBs.pfnEndEncodingCallBack(chan->CBs.pEndEncodingCBParam, pStatus, streamBufferPtr);
}

static void* WaitForStatus(void* p)
{
  Channel* chan = p;
  struct al5_params msg = { 0 };

  while(true)
  {
    if(Rtos_AtomicDecrement(&chan->shouldContinue) < 0)
      break;
    Rtos_AtomicIncrement(&chan->shouldContinue);

    if(getStatusMsg(chan, &msg))
      processStatusMsg(chan, &msg);
  }

  return 0;
}

static void destroy(TScheduler* pScheduler)
{
  Rtos_Free((AL_TSchedulerMcu*)pScheduler);
}

static __u32 getFd(AL_TBuffer* b)
{
  return (__u32)AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)b->pAllocator, b->hBuf);
}

static void createPutStreamMsg(struct al5_buffer* msg, AL_TBuffer* streamBuffer, AL_64U streamUserPtr, uint32_t uOffset)
{
  Rtos_Memset(msg, 0, sizeof(*msg));
  msg->handle = getFd(streamBuffer);
  msg->offset = uOffset;
  msg->stream_buffer_ptr = streamUserPtr;
  msg->size = streamBuffer->zSize;
}

static void putStreamBuffer(TScheduler* pScheduler, AL_HANDLE hChannel, AL_TBuffer* streamBuffer, AL_64U streamUserPtr, uint32_t uOffset)
{
  assert(streamBuffer);
  AL_TSchedulerMcu* schedulerMcu = (AL_TSchedulerMcu*)pScheduler;
  Channel* chan = (Channel*)hChannel;
  struct al5_buffer driverBuffer;
  createPutStreamMsg(&driverBuffer, streamBuffer, streamUserPtr, uOffset);
  AL_Driver_PostMessage(schedulerMcu->driver, chan->fd, AL_MCU_PUT_STREAM_BUFFER, &driverBuffer);
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

TScheduler* AL_SchedulerMcu_Create(AL_TDriver* driver, AL_TAllocator* pDmaAllocator)
{
  AL_TSchedulerMcu* scheduler = Rtos_Malloc(sizeof(*scheduler));

  if(!scheduler)
    return NULL;
  scheduler->vtable = &McuSchedulerVtable;
  scheduler->driver = driver;
  scheduler->allocator = pDmaAllocator;
  return (TScheduler*)scheduler;
}

#else

TScheduler* AL_SchedulerMcu_Create(AL_TDriver* driver, AL_TAllocator* pDmaAllocator)
{
  (void)driver, (void)pDmaAllocator;
  return NULL;
}

#endif

