// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/plateform.h"

#define NOMINMAX
#include <windows.h>

void InitializePlateform()
{
  SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
}

