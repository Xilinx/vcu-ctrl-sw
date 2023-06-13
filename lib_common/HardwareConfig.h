// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "include/config.h"
#include "lib_common/PicFormat.h"

AL_EChromaMode AL_HWConfig_Enc_GetSupportedChromaMode(void);
int AL_HWConfig_Enc_GetSupportedBitDepth(void);
int AL_HWConfig_Enc_GetSupportedL2PBitDepth(void);

