// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DPBConstraints.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_DefaultGopMngr(const AL_TGopParam* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode)
{
  int iNumRef = 0;

  if(pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_B)
  {
    iNumRef = 2; /* the refs of a B picture */

    /*
     * Add 1 in AVC for the current frame. If GOP length is 1, the current frame ref can be spared as
     * the ref to remove is always the oldest ref
     */
    if(eCodec == AL_CODEC_AVC && pGopParam->uGopLength > 1)
      iNumRef++;
  }
  /*
   * The structure of the default gop makes it that:
   *
   * When there is no reordering (no B frames) We only need one reference buffer
   * at a time
   *  ,-,,-,,-,
   * v  |v |v |
   * I  P  P  P ....
   *
   * When there are B frames, we need two reference buffer at a time
   *
   *   -
   * ,   `--
   * v   | v
   * I B B P B B ...
   * 0 2 3 1
   */
  else
  {
    iNumRef = 1;

    if(pGopParam->uNumB > 0 || (pGopParam->eMode & AL_GOP_FLAG_B_ONLY))
      ++iNumRef;
  }

  /*
   * We need an extra buffer to keep the long term picture while we deal
   * with the other pictures as normal
   */
  if(pGopParam->bEnableLT)
    ++iNumRef;

  // The current buffer is used as reference when dealing with the second field
  if(eCodec == AL_CODEC_AVC && eVideoMode != AL_VM_PROGRESSIVE)
    ++iNumRef;

  return iNumRef;
}

#define NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel) (((uNumBForPyrLevel) << 1) + 1)
/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrCustom(AL_TGopParam const* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode)
{
  int iNumRef;

  if(pGopParam->eMode & AL_GOP_FLAG_LOW_DELAY)
  {
    iNumRef = 1;

    if(pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_B)
    {
      iNumRef++;

      if(eCodec == AL_CODEC_AVC && pGopParam->uGopLength > 1)
        iNumRef++;
    }
  }
  else if(pGopParam->uNumB == 0)
  {
    iNumRef = 1;
  }
  else
  {
    uint8_t uPyrLevel = 0;
    uint8_t uNumBForPyrLevel = 1;
    uint8_t uNumBForNextPyrLevel = NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel);

    while(pGopParam->uNumB >= uNumBForNextPyrLevel)
    {
      uNumBForPyrLevel = uNumBForNextPyrLevel;
      uNumBForNextPyrLevel = NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel);
      uPyrLevel++;
    }

    iNumRef = uPyrLevel + 2; // Right B frames + 2 P frames

    /* Add 1 in AVC for the current frame */
    if(eCodec == AL_CODEC_AVC)
      iNumRef++;
  }

  if(pGopParam->bEnableLT)
    iNumRef++;

  // The current buffer is used as reference when dealing with the second field
  if(eCodec == AL_CODEC_AVC && eVideoMode != AL_VM_PROGRESSIVE)
    ++iNumRef;

  return iNumRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam)
{
  bool bIsLookAhead = false;
  AL_EGopMngrType eGopMngrType = AL_GetGopMngrType(pChParam->tGopParam.eMode, AL_GET_CODEC(pChParam->eProfile), bIsLookAhead);
  AL_ECodec eCodec = AL_GET_CODEC(pChParam->eProfile);
  uint8_t uDPBSize = 0;
  switch(eGopMngrType)
  {
  case AL_GOP_MNGR_DEFAULT:
    uDPBSize = AL_DPBConstraint_GetMaxRef_DefaultGopMngr(&pChParam->tGopParam, eCodec, pChParam->eVideoMode);
    break;
  case AL_GOP_MNGR_CUSTOM:
  case AL_GOP_MNGR_COMMON:
    uDPBSize = AL_DPBConstraint_GetMaxRef_GopMngrCustom(&pChParam->tGopParam, eCodec, pChParam->eVideoMode);
    break;
  case AL_GOP_MNGR_MAX_ENUM:
    break;
  }

  /* reconstructed buffer is an actual part of the dpb algorithm in hevc */
  if(eCodec == AL_CODEC_HEVC)
    uDPBSize++;

  return uDPBSize;
}

/****************************************************************************/
AL_EGopMngrType AL_GetGopMngrType(AL_EGopCtrlMode eMode, AL_ECodec eCodec, bool bIsLookAhead)
{
  (void)eCodec;
  (void)bIsLookAhead;

  if(eMode == AL_GOP_MODE_ADAPTIVE || (eMode & AL_GOP_FLAG_LOW_DELAY))
    return AL_GOP_MNGR_DEFAULT;
  else if(eMode & AL_GOP_FLAG_PYRAMIDAL)
    return AL_GOP_MNGR_CUSTOM;
  else if(eMode & AL_GOP_FLAG_DEFAULT)
  {
    return AL_GOP_MNGR_DEFAULT;
  }

  return AL_GOP_MNGR_MAX_ENUM;
}

