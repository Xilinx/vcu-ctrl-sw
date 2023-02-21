// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Decoder_API
   @{
   \file
 *****************************************************************************/
#pragma once

typedef struct AL_i_DecScheduler AL_IDecScheduler;

#include "lib_common/HardwareDriver.h"

/*************************************************************************//*!
    \brief Interfaces with a scheduler that runs on different process.
    Its main usage is to interface with the MCU scheduler (microcontroller) when used with an hardware driver.
    It can also be used with a proxy driver to access a scheduler on another process.
   \param[in] driver Select which driver you want to use. This will dictate how the communication with the scheduler will be handled.
   \param[in] deviceFile The file that represents the device and that will be opened by the driver to communicate with the scheduler.
*****************************************************************************/
AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_TDriver* driver, char const* deviceFile);

/*@}*/
