// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

AL_DRIVER AL_DevicePool_Open(char const* filename, int* iErr);
int32_t AL_DevicePool_Close(AL_DRIVER driver);
