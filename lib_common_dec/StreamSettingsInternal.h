// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common_dec/StreamSettings.h"

static int const STREAM_SETTING_UNKNOWN = -1;

/******************************************************************************/
bool IsAllStreamSettingsSet(AL_TStreamSettings const* pStreamSettings);

/*****************************************************************************/
bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings const* pStreamSettings);

/*****************************************************************************/
bool CheckStreamSettings(AL_TStreamSettings const* pStreamSettings);
