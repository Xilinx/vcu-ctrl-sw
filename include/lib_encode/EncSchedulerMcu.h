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
   \addtogroup Encoder_API
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_fpga/DmaAllocLinux.h"

typedef struct AL_t_driver AL_TDriver;
typedef struct AL_i_EncScheduler AL_IEncScheduler;

/*************************************************************************//*!
    \brief Interfaces with a scheduler that runs on different process.
    Its main usage is to interface with the MCU scheduler (microcontroller) when used with an hardware driver.
    It can also be used with a proxy driver to access a scheduler on another process.
   \param[in] driver Select which driver you want to use. This will dictate how the communication with the scheduler will be handled.
   \param[in] pDmaAllocator a dma allocator that will be used to create work buffers and to map some of the buffer that are sent to the scheduler.
   \param[in] deviceFile The file that represents the device and that will be opened by the driver to communicate with the scheduler.
*****************************************************************************/
AL_IEncScheduler* AL_SchedulerMcu_Create(AL_TDriver* driver, AL_TLinuxDmaAllocator* pDmaAllocator, char const* deviceFile);

/*@}*/
