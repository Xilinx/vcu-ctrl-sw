// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Encoder_API
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_fpga/DmaAllocLinux.h"
#include "lib_common/I_Communication.h"

typedef struct AL_IEncScheduler AL_IEncScheduler;

/*****************************************************************************
    \brief Interfaces with a scheduler that runs on different process.
    Its main usage is to interface with the MCU scheduler (microcontroller) when used with an hardware driver.
    It can also be used with a proxy driver to access a scheduler on another process.
   \param[in] pDriver Select which driver you want to use. This will dictate how the communication with the scheduler will be handled
   \param[in] pDmaAllocator a dma allocator that will be used to create work buffers and to map some of the buffer that are sent to the scheduler
   \return NULL on error. An AL_IEncScheduler pointer on success
*****************************************************************************/
AL_IEncScheduler* AL_EncSchedulerMcu_Create(AL_ICommunication* pDriver, AL_TLinuxDmaAllocator* pDmaAllocator);

AL_DEPRECATED("Use AL_EncSchedulerMcu_Create instead!")
static inline AL_IEncScheduler * AL_SchedulerMcu_Create(AL_ICommunication * pDriver, AL_TLinuxDmaAllocator * pDmaAllocator)
{
  return AL_EncSchedulerMcu_Create(pDriver, pDmaAllocator);
}

/*!@}*/
