/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

/**************************************************************************//*!
   \addtogroup Driver
   @{
   \file
 *****************************************************************************/
#pragma once

typedef enum
{
  DRIVER_SUCCESS,
  DRIVER_ERROR_UNKNOWN,
  DRIVER_ERROR_NO_MEMORY,
  DRIVER_ERROR_CHANNEL,
  DRIVER_TIMEOUT,
}AL_EDriverError;

#define AL_POLL_MSG 0xfffffffc

/*************************************************************************//*!
    \brief Interfaces with a device.
    The device can either be the interface of a kernel driver like al5e, al5r or al5d
    or it could also be a socket, this is implementation dependant.
    \see AL_GetHardwareDriver for the kernel driver implementation
*****************************************************************************/
typedef struct AL_t_driver AL_TDriver;
typedef struct
{
  int (* pfnOpen)(AL_TDriver* driver, const char* device);
  void (* pfnClose)(AL_TDriver* driver, int fd);
  AL_EDriverError (* pfnPostMessage)(AL_TDriver* driver, int fd, long unsigned int messageId, void* data, bool isBlocking);
}AL_DriverVtable;

struct AL_t_driver
{
  const AL_DriverVtable* vtable;
};

static inline
int AL_Driver_Open(AL_TDriver* driver, const char* device)
{
  return driver->vtable->pfnOpen(driver, device);
}

static inline
void AL_Driver_Close(AL_TDriver* driver, int fd)
{
  driver->vtable->pfnClose(driver, fd);
}

static inline
AL_EDriverError AL_Driver_PostMessage(AL_TDriver* driver, int fd, long unsigned int messageId, void* data)
{
  return driver->vtable->pfnPostMessage(driver, fd, messageId, data, true);
}

static inline
AL_EDriverError AL_Driver_PostMessage2(AL_TDriver* driver, int fd, long unsigned int messageId, void* data, bool isBlocking)
{
  return driver->vtable->pfnPostMessage(driver, fd, messageId, data, isBlocking);
}

/*@}*/
