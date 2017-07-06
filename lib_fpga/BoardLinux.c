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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************/
/*!
   \addtogroup lib_fpga
   @{
   \file BoardLinux.c
 *****************************************************************************/
#include <pthread.h>
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>

#include "lib_rtos/types.h"

#include "lib_common/versions.h"

#include "Board.h"
#include "lib_fpga/DmaAlloc.h"
#include "IpCtrlLinux.h"

#define METHOD_BEGIN(Type) \
  Type * this = (Type*)pThis; \
  (void)this

typedef struct
{
  TBoard vtable;
  AL_TIpCtrl* m_IpCtrl;
}TBoardLinux;

/******************************************************************************/
static void BoardLinux_Destroy(void* pThis)
{
  METHOD_BEGIN(TBoardLinux);

  LinuxIpCtrl_Destroy(this->m_IpCtrl);
  free(this);
}

/******************************************************************************/
static AL_TIpCtrl* BoardLinux_GetIpCtrl(void* pThis)
{
  METHOD_BEGIN(TBoardLinux);
  return this->m_IpCtrl;
}

/******************************************************************************/
static const TBoard BoardLinuxVtable =
{
  &BoardLinux_Destroy,
  &BoardLinux_GetIpCtrl,
};

/******************************************************************************/
TBoard* Board_Create(const char* deviceFile, uint32_t uIntReg, uint32_t uMskReg, uint32_t uIntMask)
{
  (void)uIntMask;
  (void)uIntReg;
  (void)uMskReg;
  TBoardLinux* board = calloc(1, sizeof(TBoardLinux));

  if(!board)
    return NULL;
  board->vtable = BoardLinuxVtable;

  board->m_IpCtrl = LinuxIpCtrl_Create(deviceFile);

  if(!board->m_IpCtrl)
  {
    free(board);
    return NULL;
  }

  return (TBoard*)board;
}

/*@}*/

