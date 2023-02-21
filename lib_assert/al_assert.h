// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#if __MICROBLAZE__

#include "lib_mcu/McuDebug.h"

#define AL_Assert(condition) \
  Mcu_Debug_Assert(condition, # condition)

#else

#include <assert.h>

#define AL_Assert(condition) \
  assert(condition)

#endif
