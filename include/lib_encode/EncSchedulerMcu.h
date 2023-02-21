// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
