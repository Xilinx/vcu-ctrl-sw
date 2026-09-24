// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_encode/I_EncScheduler.h"
#include "lib_common/I_Communication.h"
#include "lib_common/Allocator.h"

#if defined(__linux__)

#include "lib_encode/I_EncSchedulerInfo.h"
#include "lib_encode/EncSchedulerMcu.h"
#include "lib_encode/EncSchedulerCommon.h"
#include "lib_common/Arithmetics.h"
#include "lib_common/Error.h"
#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_rtos/assert.h"
#include "lib_rtos/lib_rtos.h"
#include "EncDriverDataConversions.h"
#include "lib_fpga/DmaAllocLinux.h"

#include "lib_rtos/types.h"

#define GET_DMA_FD GET_DMA32_FD
#define al5_dma_info al5_dma32_info

#include "driver/microblaze/include/al_uapi_codec_ioctl.h"
#include "driver/microblaze/include/al_uapi_enc_ioctl.h"

#include <string.h> // strerrno
#include <unistd.h> // for close

typedef struct
{
  AL_IEncSchedulerVtable const* vtable;
  AL_TLinuxDmaAllocator* allocator;
  AL_ICommunication* driver;
}AL_TEncSchedulerMicroblaze;

typedef struct
{
  AL_TEncScheduler_CB_EndEncoding CBs;
  AL_TCommonChannelInfo info;
  AL_ICommunication* driver;
  AL_HANDLE hCommunicationLine;
  AL_THREAD thread;
  bool outputRec;
}AL_TEncChannelMicroblaze;

static void* WaitForStatus(void* p);

static void setCallbacks(AL_TEncChannelMicroblaze* pChannel, AL_TEncScheduler_CB_EndEncoding* pCBs)
{
  pChannel->CBs.func = pCBs->func;
  pChannel->CBs.userParam = pCBs->userParam;
}

static AL_ERR API_CreateChannel(AL_HANDLE* hChannel, AL_IEncScheduler* pIScheduler, AL_TMemDesc* pMDChParam, AL_TMemDesc* pEP1, AL_HANDLE hRcPluginDmaContext, AL_TEncScheduler_CB_EndEncoding* pCBs)
{
  (void)hRcPluginDmaContext;
  AL_ERR errorCode = AL_ERROR;
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = Rtos_Calloc(1, sizeof(*pChannel));

  if(NULL == pChannel)
  {
    errorCode = AL_ERR_NO_MEMORY;
    goto channel_creation_fail;
  }

  pChannel->driver = pScheduler->driver;

  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  pChannel->hCommunicationLine = AL_ICommunication_Open(pChannel->driver, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open communication while creating channel: %s\n", AL_ECommunicationError_ToString(eError));
    goto driver_open_fail;
  }

  AL_TEncChanParam* pChParam = ((AL_TEncChanParam*)pMDChParam->pVirtualAddr);
  struct al5_config_channel msg = { 0 };
  setChannelParam(&msg.param, pMDChParam, pEP1);

  msg.rc_plugin_fd = -1;

  if(hRcPluginDmaContext)
    msg.rc_plugin_fd = AL_LinuxDmaAllocator_GetFd(pScheduler->allocator, hRcPluginDmaContext);

  Rtos_FlushCacheMemory(&msg.rc_plugin_fd, sizeof(msg.rc_plugin_fd));

  AL_ECommunicationError errdrv = AL_ICommunication_PostBlockingMessage(pChannel->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_CONFIG_CHANNEL, &msg);

  if(errdrv != AL_COMMUNICATION_SUCCESS)
  {
    if(errdrv == AL_COMMUNICATION_ERROR_NO_MEMORY)
      errorCode = AL_ERR_NO_MEMORY;

    /* the ioctl might not have been called at all,
     * so the error_code might no be set. leave it to AL_ERROR in this case */
    if((errdrv == AL_COMMUNICATION_ERROR_CHANNEL) && (msg.status.error_code != 0))
      errorCode = msg.status.error_code;

    goto fail;
  }

  Rtos_InvalidateCacheMemory(&msg.status.error_code, sizeof(msg.status.error_code));

  if(AL_IS_ERROR_CODE(msg.status.error_code))
  {
    Rtos_Assert(!AL_IS_ERROR_CODE(msg.status.error_code));
    return AL_ERROR;
  }

  pChannel->outputRec = pChParam->eEncOptions & AL_OPT_FORCE_REC;
  setCallbacks(pChannel, pCBs);
  pChannel->thread = Rtos_CreateThread(&WaitForStatus, pChannel);

  if(!pChannel->thread)
    goto fail;

  /* We assume we configure the rec buffers the same way on MCU side */
  SetChannelInfo(&pChannel->info, (AL_TEncChanParam*)pMDChParam->pVirtualAddr);

  *hChannel = (AL_HANDLE)pChannel;
  return AL_SUCCESS;

  fail:
  AL_ICommunication_Close(pScheduler->driver, pChannel->hCommunicationLine);
  driver_open_fail:
  Rtos_Free(pChannel);
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
    return;
  }

  prepareQpTablesAddr(pBuffersAddrs);
  setEncodeMsg(msg, pEncInfo, pReqInfo, pBuffersAddrs);
}

static bool API_EncodeOneFrame(AL_IEncScheduler* pIScheduler, AL_HANDLE hChannel, AL_TEncInfo* pEncInfo, AL_TEncRequestInfo* pReqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = (AL_TEncChannelMicroblaze*)hChannel;
  struct al5_encode_msg msg = { 0 };
  createEncodeMsg(&msg, pEncInfo, pReqInfo, pBuffersAddrs);
  return AL_ICommunication_PostBlockingMessage(pScheduler->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_ENCODE_ONE_FRAME, &msg) == AL_COMMUNICATION_SUCCESS;
}

static bool API_DestroyChannel(AL_IEncScheduler* pIScheduler, AL_HANDLE hChannel)
{
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = (AL_TEncChannelMicroblaze*)hChannel;

  if(NULL == pChannel)
    return false;

  AL_ICommunication_PostBlockingMessage(pScheduler->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_DESTROY_CHANNEL, NULL);

  if(!Rtos_JoinThread(pChannel->thread))
    return false;
  Rtos_DeleteThread(pChannel->thread);

  AL_ICommunication_Close(pScheduler->driver, pChannel->hCommunicationLine);

  Rtos_Free(pChannel);

  return true;
}

static bool API_GetRecPicture(AL_IEncScheduler* pIScheduler, AL_HANDLE hChannel, AL_TRecPic* pRecPic)
{
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = (AL_TEncChannelMicroblaze*)hChannel;
  struct al5_reconstructed_info msg = { 0 };

  if(!pChannel->outputRec)
    return false;

  if(AL_ICommunication_PostBlockingMessage(pScheduler->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_GET_REC_PICTURE, &msg) != AL_COMMUNICATION_SUCCESS)
    return false;

  AL_TLinuxDmaAllocator* pAllocator = pScheduler->allocator;
  AL_HANDLE hRecBuf = AL_LinuxDmaAllocator_ImportFromFd(pAllocator, msg.fd);

  if(!hRecBuf)
    return false;

  AL_TReconstructedInfo recInfo;
  recInfo.tID = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pAllocator, hRecBuf);
  recInfo.ePicStruct = msg.pic_struct;
  recInfo.iPOC = msg.poc;

  recInfo.tPicDim.iWidth = msg.width;
  recInfo.tPicDim.iHeight = msg.height;

  SetRecPic(pRecPic, (AL_TAllocator*)pAllocator, hRecBuf, &pChannel->info, &recInfo);

  return true;
}

static bool API_ReleaseRecPicture(AL_IEncScheduler* pIScheduler, AL_HANDLE hChannel, AL_TRecPic* pRecPic)
{
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = hChannel;

  if(!pRecPic->pBuf || !pChannel->outputRec)
    return false;

  AL_HANDLE hRecBuf = pRecPic->pBuf->hBufs[0];
  AL_TLinuxDmaAllocator* pAllocator = pScheduler->allocator;
  int32_t fd = AL_LinuxDmaAllocator_GetFd(pAllocator, hRecBuf);

  if(AL_ICommunication_PostBlockingMessage(pScheduler->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_RELEASE_REC_PICTURE, &fd) != AL_COMMUNICATION_SUCCESS)
    return false;

  AL_Allocator_Free((AL_TAllocator*)pAllocator, hRecBuf);
  close(fd);

  return true;
}

static void processStatusMsg(AL_TEncChannelMicroblaze* pChannel, struct al5_params* msg)
{
  Rtos_InvalidateCacheMemory(msg, sizeof(msg->size));
  Rtos_InvalidateCacheMemory(msg, msg->size);
  Rtos_Assert(msg->size >= sizeof(AL_PTR64));
  AL_PTR64 streamBufferPtr;
  Rtos_Memcpy(&streamBufferPtr, msg->opaque, sizeof(AL_PTR64));
  AL_TEncPicStatus* pStatus = NULL;
  AL_TEncPicStatus status;

  if(msg->size > sizeof(AL_PTR64))
  {
    Rtos_Assert(msg->size == sizeof(AL_PTR64) + sizeof(AL_TEncPicStatus));
    Rtos_Memcpy(&status, (char*)msg->opaque + sizeof(AL_PTR64), UnsignedMin(sizeof(status), sizeof(msg->opaque) - sizeof(AL_PTR64)));
    pStatus = &status;
  }

  pChannel->CBs.func(pChannel->CBs.userParam, pStatus, streamBufferPtr);
}

static void* WaitForStatus(void* p)
{
  Rtos_SetCurrentThreadName("enc-status-it");
  AL_TEncChannelMicroblaze* pChannel = (AL_TEncChannelMicroblaze*)p;
  struct al5_params msg = { 0 };
  Rtos_PollCtx ctx;
  /* Wait for wait for status events forever in poll */
  ctx.timeout = -1;
  ctx.events = AL_POLLIN;

  while(true)
  {
    ctx.revents = 0;

    AL_ECommunicationError err = AL_ICommunication_PostBlockingMessage(pChannel->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_POLL, &ctx);

    if(err != AL_COMMUNICATION_SUCCESS)
      continue;

    if(ctx.revents & AL_POLLIN)
    {
      AL_ECommunicationError err = AL_ICommunication_PostNonBlockingMessage(pChannel->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_WAIT_FOR_STATUS, &msg);

      if(err == AL_COMMUNICATION_SUCCESS)
        processStatusMsg(pChannel, &msg);
      else
        Rtos_Log(AL_LOG_ERROR, "Failed to get encode status (error code: %d)\n", err);
    }

    /* If the polling finds an end of operation, it means that the channel was destroyed and we can stop waiting for encoding results. */
    if(ctx.revents & AL_POLLHUP)
      break;
  }

  return NULL;
}

static void API_Destroy(AL_IEncScheduler* pIScheduler)
{
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  Rtos_Free(pScheduler);
}

static int32_t getFd(AL_TBuffer const* pBuffer)
{
  return (int32_t)AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)pBuffer->pAllocator, pBuffer->hBufs[0]);
}

static void createPutStreamMsg(struct al5_buffer* msg, AL_TBuffer* streamBuffer, AL_64U streamUserPtr, uint32_t uOffset)
{
  Rtos_Memset(msg, 0, sizeof(*msg));
  msg->stream_buffer.handle = getFd(streamBuffer);
  msg->stream_buffer.offset = uOffset;
  msg->stream_buffer.stream_buffer_ptr = streamUserPtr;
  msg->stream_buffer.size = AL_Buffer_GetSize(streamBuffer);
  msg->external_mv_handle = -1;

  AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)AL_Buffer_GetMetaData(streamBuffer, AL_META_TYPE_RATECTRL);

  if(pMeta != NULL)
    msg->external_mv_handle = getFd(pMeta->pMVBuf);
}

static void API_PutStreamBuffer(AL_IEncScheduler* pIScheduler, AL_HANDLE hChannel, AL_TBuffer* streamBuffer, AL_64U streamUserPtr, uint32_t uOffset)
{
  Rtos_Assert(streamBuffer);
  AL_TEncSchedulerMicroblaze* pScheduler = (AL_TEncSchedulerMicroblaze*)pIScheduler;
  AL_TEncChannelMicroblaze* pChannel = (AL_TEncChannelMicroblaze*)hChannel;
  struct al5_buffer driverBuffer;
  createPutStreamMsg(&driverBuffer, streamBuffer, streamUserPtr, uOffset);
  AL_ICommunication_PostBlockingMessage(pScheduler->driver, pChannel->hCommunicationLine, AL_COMMUNICATION_MESSAGE_PUT_STREAM_BUFFER, &driverBuffer);
}

/******************************************************************************/
static void GetSchedulerVersion(AL_TEncSchedulerMicroblaze const* pScheduler, AL_TIEncSchedulerVersion* pVersion)
{
  AL_ECommunicationError eError = AL_COMMUNICATION_ERROR_UNKNOWN;
  AL_HANDLE hCommunicationLine = AL_ICommunication_Open(pScheduler->driver, &eError);

  if(eError != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Couldn't open communication while getting scheduler version: %s\n", AL_ECommunicationError_ToString(eError));
    return;
  }

  struct al5_params msg;
  AL_EIEncSchedulerInfo eInfo = AL_IENCSCHEDULER_VERSION;
  msg.opaque[0] = eInfo;
  Rtos_Memcpy(&msg.opaque[sizeof(eInfo) / sizeof(*msg.opaque)], pVersion, sizeof(*pVersion));

  Rtos_StaticAssert(sizeof(eInfo) + sizeof(*pVersion) <= sizeof(msg.opaque), "Driver version structure struct is too small");
  msg.size = sizeof(eInfo) + sizeof(*pVersion);

  AL_ECommunicationError const error = AL_ICommunication_PostBlockingMessage(pScheduler->driver, hCommunicationLine, AL_COMMUNICATION_MESSAGE_GET, &msg);

  if(error != AL_COMMUNICATION_SUCCESS)
  {
    Rtos_Log(AL_LOG_ERROR, "Failed to get parameter '%s', (error code: '%d')\n", ToStringIEncSchedulerInfo(AL_IENCSCHEDULER_VERSION), error);
    AL_ICommunication_Close(pScheduler->driver, hCommunicationLine);
    return;
  }

  Rtos_Memcpy(pVersion, &msg.opaque[1], sizeof(*pVersion));

  AL_ICommunication_Close(pScheduler->driver, hCommunicationLine);
}

static void API_Get(AL_IEncScheduler const* pScheduler, AL_EIEncSchedulerInfo info, void* pParam)
{
  AL_TEncSchedulerMicroblaze const* pSchedulerMcu = (AL_TEncSchedulerMicroblaze const*)pScheduler;
  switch(info)
  {
  case AL_IENCSCHEDULER_VERSION:
  {
    GetSchedulerVersion(pSchedulerMcu, (AL_TIEncSchedulerVersion*)pParam);
    return;
  }
  default: return;
  }

  return;
}

static void API_Set(AL_IEncScheduler* pScheduler, AL_EIEncSchedulerInfo info, void const* pParam)
{
  (void)pParam;
  AL_TEncSchedulerMicroblaze* pSchedulerMcu = (AL_TEncSchedulerMicroblaze*)pScheduler;
  (void)pSchedulerMcu;
  switch(info)
  {
  default: return;
  }

  return;
}

static const AL_IEncSchedulerVtable McuEncSchedulerVtable =
{
  API_Destroy,
  API_CreateChannel,
  API_DestroyChannel,
  API_EncodeOneFrame,
  API_PutStreamBuffer,
  API_GetRecPicture,
  API_ReleaseRecPicture,
  API_Get,
  API_Set,
};

AL_IEncScheduler* AL_EncSchedulerMcu_Create(AL_ICommunication* driver, AL_TLinuxDmaAllocator* pDmaAllocator)
{
  if(NULL == driver)
    return NULL;

  AL_TEncSchedulerMicroblaze* pScheduler = Rtos_Malloc(sizeof(*pScheduler));

  if(NULL == pScheduler)
    return NULL;

  pScheduler->vtable = &McuEncSchedulerVtable;
  pScheduler->driver = driver;
  pScheduler->allocator = pDmaAllocator;

  return (AL_IEncScheduler*)pScheduler;
}

#else

AL_IEncScheduler* AL_EncSchedulerMcu_Create(AL_ICommunication* driver, AL_TAllocator* pDmaAllocator)
{
  (void)driver, (void)pDmaAllocator;
  return NULL;
}

#endif
