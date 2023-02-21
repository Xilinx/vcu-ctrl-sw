// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_decode/I_Feeder.h"

AL_TFeeder* AL_SplitBufferFeeder_Create(AL_HANDLE hDec, int uMaxBufNum, AL_TBuffer* pEOSBuffer, bool bEOSParsingCB);

