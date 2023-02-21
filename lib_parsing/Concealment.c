// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Concealment.h"

void AL_Conceal_Init(AL_TConceal* pConceal)
{
  pConceal->iFirstLCU = -1;
  pConceal->iActivePPS = -1;
  pConceal->bValidFrame = false;
  pConceal->iLastPPSId = -1;
  pConceal->bHasPPS = false;
}

