// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_common/Planes.h"
}

using namespace std;

vector<AL_TPlaneDescription> getPlaneDescription(TFourCC tFourCC, int iPitch, int iPitchMap, size_t sizes[], int& iTotalOffset);
