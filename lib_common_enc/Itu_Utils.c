// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "Itu_Utils.h"

int GetNumActiveRefIdx(AL_TEncChanParam const* pChParam)
{
  (void)pChParam;
  int32_t iNumRef = 1;

  bool bIncrNumRef = false;

  bIncrNumRef |= pChParam->tGopParam.bDoubleRef;

  if(bIncrNumRef)
    iNumRef++;
  return iNumRef;
}
