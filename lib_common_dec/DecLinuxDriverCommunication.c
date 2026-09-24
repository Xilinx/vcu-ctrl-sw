// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/I_Communication.h"
#include "lib_rtos/types.h"

#if (__linux__)

#include "lib_rtos/lib_rtos.h"

#define GET_DMA_FD GET_DMA32_FD
#define al5_dma_info al5_dma32_info

#include "driver/microblaze/include/al_uapi_codec_ioctl.h"
#include "driver/microblaze/include/al_uapi_dec_ioctl.h"

#include <errno.h>
#include <string.h> // strlen

#define AL_POLL_MSG (0xFFFFFFFC)

typedef struct AL_TDecLinuxDriverCommunication
{
  AL_ICommunication tICommunication;
  char* pDevice;
}AL_TDecLinuxDriverCommunication;

static AL_ECommunicationError ErrnoToDriverError(int err)
{
  if(err == 0)
    return AL_COMMUNICATION_SUCCESS;

  if(err == ENOMEM)
    return AL_COMMUNICATION_ERROR_NO_MEMORY;

  if(err == EINVAL || err == EPERM)
    return AL_COMMUNICATION_ERROR_CHANNEL;

  return AL_COMMUNICATION_ERROR_UNKNOWN;
}

static AL_HANDLE Open(AL_ICommunication* pICommunication, AL_ECommunicationError* eError)
{
  AL_TDecLinuxDriverCommunication* pCommunication = (AL_TDecLinuxDriverCommunication*)pICommunication;
  int iErrno = 0;
  AL_HANDLE handle = Rtos_DriverOpen(pCommunication->pDevice, &iErrno);
  *eError = ErrnoToDriverError(iErrno);
  return handle;
}

static void Close(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine)
{
  AL_TDecLinuxDriverCommunication* pCommunication = (AL_TDecLinuxDriverCommunication*)pICommunication;
  (void)pCommunication;
  Rtos_DriverClose(hCommunicationLine);
}

static unsigned long ToIoctlRequest(AL_ECommunicationMessageID eMessage)
{
  switch(eMessage)
  {
  case AL_COMMUNICATION_MESSAGE_POLL: return AL_POLL_MSG;
  case AL_COMMUNICATION_MESSAGE_WAIT_FOR_STATUS: return AL_MCU_WAIT_FOR_STATUS;
  case AL_COMMUNICATION_MESSAGE_WAIT_FOR_START_CODE: return AL_MCU_WAIT_FOR_START_CODE;
  case AL_COMMUNICATION_MESSAGE_SEARCH_START_CODE: return AL_MCU_SEARCH_START_CODE;
  case AL_COMMUNICATION_MESSAGE_CONFIG_CHANNEL: return AL_MCU_CONFIG_CHANNEL;
  case AL_COMMUNICATION_MESSAGE_DESTROY_CHANNEL: return AL_MCU_DESTROY_CHANNEL;
  case AL_COMMUNICATION_MESSAGE_DECODE_ONE_FRAME: return AL_MCU_DECODE_ONE_FRM;
  case AL_COMMUNICATION_MESSAGE_DECODE_ONE_SLICE: return AL_MCU_DECODE_ONE_SLICE;
  case AL_COMMUNICATION_MESSAGE_DECODE_JPEG: return AL_MCU_DECODE_JPEG;
  case AL_COMMUNICATION_MESSAGE_GET: return AL_MCU_GET;
  case AL_COMMUNICATION_MESSAGE_SET: return AL_MCU_SET;
  default:
    Rtos_Log(AL_LOG_ERROR, "Unknown message: %s\n", AL_ECommunicationMessageID_ToString(eMessage));
    Rtos_Assert(false);
    return 0xDEADDEAD;
  }
}

static AL_ECommunicationError PostMessage(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine, AL_ECommunicationMessageID eMessageID, void* data, bool isBlocking)
{
  AL_TDecLinuxDriverCommunication* pCommunication = (AL_TDecLinuxDriverCommunication*)pICommunication;
  (void)pCommunication;

  while(true)
  {
    int32_t iRet;

    unsigned long uMessageID = ToIoctlRequest(eMessageID);

    if(uMessageID != AL_POLL_MSG)
      iRet = Rtos_DriverIoctl(hCommunicationLine, uMessageID, data);
    else
    {
      Rtos_PollCtx* ctx = (Rtos_PollCtx*)data;
      iRet = Rtos_DriverPoll(hCommunicationLine, ctx);

      if(iRet == 0)
        return AL_COMMUNICATION_TIMEOUT;
    }

    int errdrv = errno;

    if(iRet < 0)
    {
      /* posix -> EAGAIN == EWOULDBLOCK */
      if(((errdrv == EAGAIN) && isBlocking) || (errdrv == EINTR))
        continue;
      return ErrnoToDriverError(errdrv);
    }

    return AL_COMMUNICATION_SUCCESS;
  }
}

static void Destroy(AL_ICommunication* pICommunication)
{
  AL_TDecLinuxDriverCommunication* pCommunication = (AL_TDecLinuxDriverCommunication*)pICommunication;
  Rtos_Free(pCommunication->pDevice);
  Rtos_Free(pCommunication);
}

static AL_TCommunicationVTable const linuxDriverVtable =
{
  .pfnOpen = &Open,
  .pfnClose = &Close,
  .pfnPostMessage = &PostMessage,
  .pfnDestroy = &Destroy
};

AL_ICommunication* AL_CreateDecLinuxDriverCommunication(char const* pDevice)
{
  if(pDevice == NULL)
    return NULL;

  AL_TDecLinuxDriverCommunication* pCommunication = Rtos_Malloc(sizeof(*pCommunication));

  if(pCommunication == NULL)
    return NULL;

  pCommunication->pDevice = Rtos_Strndup(pDevice, strlen(pDevice));

  if(pCommunication->pDevice == NULL)
  {
    Rtos_Free(pCommunication);
    return NULL;
  }

  pCommunication->tICommunication.vtable = &linuxDriverVtable;

  return (AL_ICommunication*)pCommunication;
}

#else

AL_ICommunication* AL_CreateDecLinuxDriverCommunication(char const* pDevice)
{
  (void)pDevice;
  return NULL;
}

#endif
