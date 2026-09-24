// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/DecOutputSettings.h"

void SetDefaultDecOutputSettings(AL_TDecOutputSettings* pDecOutputSettings)
{
  pDecOutputSettings->tPicFormat = GetDefaultPicFormat();
}
