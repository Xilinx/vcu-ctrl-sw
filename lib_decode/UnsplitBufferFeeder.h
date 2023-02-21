// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "I_Feeder.h"
#include "DecoderFeeder.h"

AL_TFeeder* AL_UnsplitBufferFeeder_Create(AL_HANDLE hDec, int uMaxBufNum, AL_TAllocator* pAllocator, int iBufferStreamSize, AL_TBuffer* eosBuffer, bool bForceAccessUnitDestroy);

