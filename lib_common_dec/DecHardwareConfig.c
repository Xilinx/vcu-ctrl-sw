// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecHardwareConfig.h"

AL_EChromaMode AL_HWConfig_Dec_GetSupportedChromaMode(void)
{
  return AL_CHROMA_4_2_2;
}

int AL_HWConfig_Dec_GetSupportedBitDepth(void)
{
  return 10;
}
