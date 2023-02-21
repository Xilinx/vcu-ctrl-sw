// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/HDR.h"

void AL_HDRSEIs_Reset(AL_THDRSEIs* pHDRSEIs)
{
  if(pHDRSEIs == NULL)
    return;

  pHDRSEIs->bHasMDCV = false;
  pHDRSEIs->bHasCLL = false;
  pHDRSEIs->bHasATC = false;
  pHDRSEIs->bHasST2094_10 = false;
  pHDRSEIs->bHasST2094_40 = false;
}

void AL_HDRSEIs_Copy(AL_THDRSEIs* pHDRSEIsSrc, AL_THDRSEIs* pHDRSEIsDst)
{
  pHDRSEIsDst->bHasMDCV = pHDRSEIsSrc->bHasMDCV;

  if(pHDRSEIsDst->bHasMDCV)
    pHDRSEIsDst->tMDCV = pHDRSEIsSrc->tMDCV;

  pHDRSEIsDst->bHasCLL = pHDRSEIsSrc->bHasCLL;

  if(pHDRSEIsDst->bHasCLL)
    pHDRSEIsDst->tCLL = pHDRSEIsSrc->tCLL;

  pHDRSEIsDst->bHasATC = pHDRSEIsSrc->bHasATC;

  if(pHDRSEIsDst->bHasATC)
    pHDRSEIsDst->tATC = pHDRSEIsSrc->tATC;

  pHDRSEIsDst->bHasST2094_10 = pHDRSEIsSrc->bHasST2094_10;

  if(pHDRSEIsDst->bHasST2094_10)
    pHDRSEIsDst->tST2094_10 = pHDRSEIsSrc->tST2094_10;

  pHDRSEIsDst->bHasST2094_40 = pHDRSEIsSrc->bHasST2094_40;

  if(pHDRSEIsDst->bHasST2094_40)
    pHDRSEIsDst->tST2094_40 = pHDRSEIsSrc->tST2094_40;
}

