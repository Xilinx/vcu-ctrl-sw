// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Decoder_API
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/I_Communication.h"

typedef struct AL_IDecScheduler AL_IDecScheduler;

/*****************************************************************************
    \brief Interfaces with a scheduler that runs on different process.
    Its main usage is to interface with the MCU scheduler (microcontroller) when used with an hardware driver.
    It can also be used with a proxy driver to access a scheduler on another process.
   \param[in] driver Select which driver you want to use. This will dictate how the communication with the scheduler will be handled.
*****************************************************************************/
AL_IDecScheduler* AL_DecSchedulerMcu_Create(AL_ICommunication* driver);

/*!@}*/
