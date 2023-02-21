// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_decode/DecSettings.h"

/******************************************************************************/
bool IsAllStreamSettingsSet(AL_TStreamSettings const* pStreamSettings);

/*****************************************************************************/
bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings const* pStreamSettings);

/*****************************************************************************/
int GetAlignedStreamBufferSize(int iStreamBufferSize);
