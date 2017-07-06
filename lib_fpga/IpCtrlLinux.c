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

#include <pthread.h>
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>

#include "IpCtrlLinux.h"
#include "lib_rtos/types.h"
#include "DevicePool.h"
#include "allegro_ioctl_reg.h"

static void* WaitInterruptThread(void* p);
#define NUM_INTERRUPT 20

typedef struct
{
  const AL_IpCtrlVtable* vtable;
  int fd;
  pthread_t thread;
  pthread_mutex_t mutex;
  struct
  {
    AL_PFN_IpCtrl_CallBack pfnCB;
    void* pUserData;
  }m_pInterrupt[NUM_INTERRUPT];
}LinuxIpCtrl;

#define METHOD_BEGIN(Type) \
  Type* this = (Type*)pThis; \
  (void)this

static void* WaitInterruptThread(void* pThis)
{
  METHOD_BEGIN(LinuxIpCtrl);

  for(;;)
  {
    __s32 uNumInt = -1;
    int ret = ioctl(this->fd, AL_CMD_IP_WAIT_IRQ, &uNumInt);

    if(ret == -1)
    {
      if(errno != EINTR)
        perror("IOCTL0 failed with ");
      goto fail;
    }

    /* got IRQ, Callback */
    if(uNumInt < 0 || uNumInt >= NUM_INTERRUPT)
    {
      fprintf(stderr, "Got %d, No interrupt to handle\n", uNumInt);
      goto fail;
    }

    pthread_mutex_lock(&this->mutex);

    if(!this->m_pInterrupt[uNumInt].pfnCB)
      fprintf(stderr, "Interrupt %d doesn't have an handler, signaling it was caught and returning\n", uNumInt);

    if(this->m_pInterrupt[uNumInt].pfnCB)
      this->m_pInterrupt[uNumInt].pfnCB(this->m_pInterrupt[uNumInt].pUserData);

    pthread_mutex_unlock(&this->mutex);
  }

  fail:
  return NULL;
}

/******************************************************************************/
static uint32_t LinuxIpCtrl_ReadRegister(AL_TIpCtrl* pThis, uint32_t uReg)
{
  METHOD_BEGIN(LinuxIpCtrl);

  struct al5_reg reg;
  reg.id = uReg;
  reg.value = 0;
  int ret = ioctl(this->fd, AL_CMD_IP_READ_REG, &reg);

  if(ret < 0)
    perror("IOCTL failed with ");
  return reg.value;
}

/******************************************************************************/
static void LinuxIpCtrl_WriteRegister(AL_TIpCtrl* pThis, uint32_t uReg, uint32_t uVal)
{
  METHOD_BEGIN(LinuxIpCtrl);

  struct al5_reg reg;
  reg.id = uReg;
  reg.value = uVal;
  int ret = ioctl(this->fd, AL_CMD_IP_WRITE_REG, &reg);

  if(ret < 0)
    perror("IOCTL failed with ");
}

/******************************************************************************/
static void LinuxIpCtrl_RegisterCallBack(AL_TIpCtrl* pThis, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uNumInt)
{
  METHOD_BEGIN(LinuxIpCtrl);

  pthread_mutex_lock(&this->mutex);
  this->m_pInterrupt[uNumInt].pfnCB = pfnCallBack;
  this->m_pInterrupt[uNumInt].pUserData = pUserData;
  pthread_mutex_unlock(&this->mutex);
}


static const AL_IpCtrlVtable LinuxIpControlVtable =
{
  &LinuxIpCtrl_ReadRegister,
  &LinuxIpCtrl_WriteRegister,
  &LinuxIpCtrl_RegisterCallBack,
};

AL_TIpCtrl* LinuxIpCtrl_Create(const char* deviceFile)
{
  LinuxIpCtrl* ipCtrl = calloc(1, sizeof(LinuxIpCtrl));

  if(!ipCtrl)
    return NULL;
  ipCtrl->vtable = &LinuxIpControlVtable;

  ipCtrl->fd = AL_DevicePool_Open(deviceFile);

  if(ipCtrl->fd < 0)
    goto fail_devicepool;

  if(pthread_mutex_init(&ipCtrl->mutex, NULL) != 0)
    goto fail_mutex;

  if(pthread_create(&ipCtrl->thread, NULL, &WaitInterruptThread, ipCtrl) < 0)
    goto fail_thread;

  return (AL_TIpCtrl*)ipCtrl;

  fail_thread:
  pthread_mutex_destroy(&ipCtrl->mutex);
  fail_mutex:
  AL_DevicePool_Close(ipCtrl->fd);
  fail_devicepool:
  free(ipCtrl);
  return NULL;
}

void LinuxIpCtrl_Destroy(void* pThis)
{
  METHOD_BEGIN(LinuxIpCtrl);

  ioctl(this->fd, AL_CMD_UNBLOCK_CHANNEL, NULL);

  pthread_join(this->thread, NULL);
  AL_DevicePool_Close(this->fd);
}

