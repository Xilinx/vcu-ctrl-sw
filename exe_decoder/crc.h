// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <fstream>

extern "C"
{
#include "lib_common/BufferAPI.h"
}

void Compute_CRC(AL_TBuffer* pYUV, int iBdInY, int iBdInC, std::ofstream& ofCrcFile);

