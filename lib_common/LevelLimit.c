// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "LevelLimit.h"

/*************************************************************************/
uint8_t AL_GetRequiredLevel(uint32_t uVal, const AL_TLevelLimit* pLevelLimits, int iNbLimits)
{
  for(int i = 0; i < iNbLimits; i++)
  {
    if(uVal <= pLevelLimits[i].uLimit)
      return pLevelLimits[i].uLevel;
  }

  return 255u;
}

