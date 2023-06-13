// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/HardwareConfig.h"

AL_EChromaMode AL_HWConfig_Enc_GetSupportedChromaMode(void)
{
  return AL_CHROMA_4_2_2;
}

int AL_HWConfig_Enc_GetSupportedBitDepth(void)
{
  return 10;
}

int AL_HWConfig_Enc_GetSupportedL2PBitDepth(void)
{
  return 10;
}

