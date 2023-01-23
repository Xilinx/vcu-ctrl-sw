/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

