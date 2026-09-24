// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DPBConstraints.h"

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_DefaultGop(const AL_TGopParam* pGopParam, AL_EProfile eProfile)
{

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

  uint8_t uNumRef = AL_IS_INTRA_PROFILE(eProfile) ? 0 : 1;

  if(pGopParam->uNumB > 0 || (pGopParam->eMode & AL_GOP_FLAG_B_ONLY))
    ++uNumRef;

  /*
  * We need an extra buffer to keep the long term picture while we deal
  * with the other pictures as normal
  */
  if(pGopParam->bEnableLT)
    ++uNumRef;

  if(pGopParam->bDoubleRef)
    ++uNumRef;

  return uNumRef;
}

#define NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel) (((uNumBForPyrLevel) << 1) + 1)
uint8_t AL_DPBConstraint_GetMaxReordering_PyramidalGop(const AL_TGopParam* pGopParam)
{
  /*
   * Max reordering required is dependent of the height of the pyramid.
   * This is the number of B-Reference-Levels + 1 for P.
   *
   * I               P
   *         B
   *     B       B
   *   b   b   b   b
   *   ^
   *   |
   * Reordering 3
   */

  uint8_t uNumReordering = 0;

  if(pGopParam->uNumB != 0)
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

    uNumReordering = uPyrLevel + 1;
  }

  return uNumReordering;
}

uint8_t AL_DPBConstraint_GetMaxRef_PyramidalGop(const AL_TGopParam* pGopParam, AL_EProfile eProfile)
{
  (void)eProfile;

  /*
   * Number of references required is the reordering + 1 for left I or P frame.
   * The higher number of references reached occurs when encoding the bottom-left
   * non-ref b.
   *
   * I               P
   *         B
   *     B       B
   *   b   b   b   b
   *   ^
   *   |
   * Worst-Case
   */

  uint8_t uNumRef = 1;

  if(pGopParam->uNumB != 0)
  {
    uint8_t const LEFT_I_OR_P_REFERENCE = 1;
    uNumRef = AL_DPBConstraint_GetMaxReordering_PyramidalGop(pGopParam) + LEFT_I_OR_P_REFERENCE;

    /* Add 1 in AVC for the current frame */
    if(AL_IS_AVC(eProfile))
      uNumRef++;
  }

  if(pGopParam->bEnableLT
     || pGopParam->bDoubleRef
     )
    uNumRef++;

  return uNumRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_LowDelayGop(const AL_TGopParam* pGopParam, AL_EProfile eProfile)
{
  (void)eProfile;

  /* Ref of a P picture */
  uint8_t uNumRef = 1;

  if(pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_B)
  {
    /* Refs of a B picture */
    uNumRef++;

    /*
     * Add 1 in AVC for the current frame. If GOP length is 1, the current frame ref can be spared as
     * the ref to remove is always the oldest ref
     */
    if(AL_IS_AVC(eProfile) && pGopParam->uGopLength > 1)
      uNumRef++;
  }

  if(pGopParam->bEnableLT)
    ++uNumRef;

  if(pGopParam->bDoubleRef)
    ++uNumRef;

  return uNumRef;
}

/****************************************************************************/
static uint8_t AL_DPBConstraint_AdjustMaxRefInterlaced(AL_EProfile eProfile, uint8_t uProgressiveMaxRef)
{
  uint8_t uInterleavedMaxRef = uProgressiveMaxRef;

  if(!AL_IS_AVC(eProfile))
  {
    // In AVC, top and bottom fields are handled as a single reference. For other codecs,
    // each field is handled independently like any other frame, so they are independent
    // in the DPB, meaning we must double the number of references.
    uInterleavedMaxRef *= 2;
  }

  // When the DPB has hit its maximum number of references, and a new reference comes in
  // to replace a previous reference, we need an additional slot to store the first field
  // as a reference when dealing with the second field.
  uInterleavedMaxRef += 1;

  return uInterleavedMaxRef;
}

/****************************************************************************/
static uint8_t AL_DPBConstraint_GetGopMaxRef(const AL_TGopParam* pGopParam, AL_EProfile eProfile, AL_EVideoMode eVideoMode)
{
  (void)eVideoMode;

  uint8_t uMaxRef = 0;

  if((pGopParam->eMode & AL_GOP_FLAG_DEFAULT) || (pGopParam->eMode == AL_GOP_MODE_ADAPTIVE))
    uMaxRef = AL_DPBConstraint_GetMaxRef_DefaultGop(pGopParam, eProfile);
  else if(pGopParam->eMode & AL_GOP_FLAG_PYRAMIDAL)
    uMaxRef = AL_DPBConstraint_GetMaxRef_PyramidalGop(pGopParam, eProfile);
  else if(pGopParam->eMode & AL_GOP_FLAG_LOW_DELAY)
    uMaxRef = AL_DPBConstraint_GetMaxRef_LowDelayGop(pGopParam, eProfile);
  else
  {
    Rtos_Assert(false);
  }

  if(eVideoMode != AL_VM_PROGRESSIVE)
    uMaxRef = AL_DPBConstraint_AdjustMaxRefInterlaced(eProfile, uMaxRef);

  return uMaxRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef(const AL_TEncChanParam* pChParam)
{
  if(AL_IS_INTRA_PROFILE(pChParam->eProfile))
    return 0;

  uint8_t uMaxRef = AL_DPBConstraint_GetGopMaxRef(&pChParam->tGopParam, pChParam->eProfile, pChParam->eVideoMode);

  return uMaxRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam)
{
  uint8_t uMaxDPBSize = AL_DPBConstraint_GetMaxRef(pChParam);
  AL_ECodec eCodec = AL_GET_CODEC(pChParam->eProfile);

  if(eCodec == AL_CODEC_HEVC || eCodec == AL_CODEC_VVC)
  {
    /* Reconstructed buffer is an actual part of the dpb algorithm in hevc & vvc*/
    uMaxDPBSize++;
  }

  return uMaxDPBSize;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxReordering(const AL_TEncChanParam* pChParam)
{
  if((pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY) || pChParam->tGopParam.uNumB == 0)
    return 0;

  uint8_t uMaxReordering = 1;

  if(pChParam->tGopParam.eMode & AL_GOP_FLAG_PYRAMIDAL)
    uMaxReordering = AL_DPBConstraint_GetMaxReordering_PyramidalGop(&pChParam->tGopParam);

  if(pChParam->eVideoMode != AL_VM_PROGRESSIVE && !AL_IS_AVC(pChParam->eProfile))
    uMaxReordering *= 2;

  return uMaxReordering;
}
