// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncSize.h"
#include "lib_common/StreamBufferPrivate.h"

/****************************************************************************/
int AL_GetCompDataSize(uint32_t uNumLCU, uint8_t uLog2MaxCUSize, uint8_t uBitDepth, AL_EChromaMode eChromaMode, bool bUseEnt)
{

  (void)uNumLCU, (void)uLog2MaxCUSize, (void)uBitDepth, (void)eChromaMode, (void)bUseEnt;
  // header + MVDs + residuals words size
  return uNumLCU * 1312;
}

/****************************************************************************/

