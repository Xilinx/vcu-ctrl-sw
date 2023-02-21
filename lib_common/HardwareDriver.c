// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <errno.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_common/IDriver.h"

static int Open(AL_TDriver* driver, const char* device)
{
  (void)driver;
  void* drv = Rtos_DriverOpen(device);

  if(drv == NULL)
    return -1;
  return (int)(uintptr_t)drv;
}

static void Close(AL_TDriver* driver, int fd)
{
  (void)driver;
  Rtos_DriverClose((void*)(intptr_t)fd);
}

static AL_EDriverError ErrnoToDriverError(int err)
{
  if(err == ENOMEM)
    return DRIVER_ERROR_NO_MEMORY;

  if(err == EINVAL || err == EPERM)
    return DRIVER_ERROR_CHANNEL;

  return DRIVER_ERROR_UNKNOWN;
}

static AL_EDriverError PostMessage(AL_TDriver* driver, int fd, long unsigned int messageId, void* data, bool isBlocking)
{
  (void)driver;

  while(true)
  {
    int iRet;

    if(messageId != AL_POLL_MSG)
      iRet = Rtos_DriverIoctl((void*)(intptr_t)fd, messageId, data);
    else
    {
      Rtos_PollCtx* ctx = (Rtos_PollCtx*)data;
      iRet = Rtos_DriverPoll((void*)(intptr_t)fd, ctx);

      if(iRet == 0)
        return DRIVER_TIMEOUT;
    }

    int errdrv = errno;

    if(iRet < 0)
    {
      /* posix -> EAGAIN == EWOULDBLOCK */
      if((errdrv == EAGAIN && isBlocking) || (errdrv == EINTR))
        continue;
      return ErrnoToDriverError(errdrv);
    }

    return DRIVER_SUCCESS;
  }
}

static AL_DriverVtable hardwareDriverVtable =
{
  Open,
  Close,
  PostMessage,
};

static AL_TDriver hardwareDriver =
{
  &hardwareDriverVtable
};

AL_TDriver* AL_GetHardwareDriver(void)
{
  return &hardwareDriver;
}

