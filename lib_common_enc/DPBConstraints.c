/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "DPBConstraints.h"
#include "lib_common/Utils.h"

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_DefaultGopMngr(const AL_TGopParam* pGopParam, AL_ECodec eCodec)
{
  /*
   * AVC worst case for the reference number in low_delay b
   * example with gop_length = 3
   *
   *   - - -
   * ,     | `
   * v     v |
   * I B B B B B B
   * g     g     g
   *
   * g: golden
   *
   * thus max_ref = gop_length + 1
   *
   * In AVC, because of the dpb algorithm, we keep the non-ref that are
   * between refs in the dpb as if they were refs.
   *
   */
  int iNumRef = 0;

  if(pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_B)
  {
    if(eCodec == AL_CODEC_AVC)
      iNumRef = Max(2, pGopParam->uGopLength + 1);
    else
    {
      /* 2 refs in hevc (the refs of a B picture) */
      iNumRef = 2;
    }
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

    if(pGopParam->uNumB > 0)
      ++iNumRef;
  }

  /*
   * We need an extra buffer to keep the long term picture while we deal
   * with the other pictures as normal
   */
  if(pGopParam->bEnableLT)
    ++iNumRef;

  return iNumRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrCustom(const AL_TGopParam* pGopParam)
{
  if(pGopParam->bEnableLT)
    return 6;
  return 5;
}


/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam)
{
  bool bIsAOM = false;

  AL_EGopMngrType eGopMngrType = AL_GetGopMngrType(pChParam->tGopParam.eMode, bIsAOM);
  AL_ECodec eCodec = AL_GetCodec(pChParam->eProfile);
  uint8_t uDPBSize = 0;
  switch(eGopMngrType)
  {
  case AL_GOP_MNGR_DEFAULT:
    uDPBSize = AL_DPBConstraint_GetMaxRef_DefaultGopMngr(&pChParam->tGopParam, eCodec);
    break;
  case AL_GOP_MNGR_CUSTOM:
    uDPBSize = AL_DPBConstraint_GetMaxRef_GopMngrCustom(&pChParam->tGopParam);
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
AL_EGopMngrType AL_GetGopMngrType(AL_EGopCtrlMode eMode, bool bIsAom)
{
  (void)bIsAom;
  switch(eMode)
  {
  case AL_GOP_MODE_ADAPTIVE: return AL_GOP_MNGR_DEFAULT;
  case AL_GOP_MODE_LOW_DELAY_P: return AL_GOP_MNGR_DEFAULT;
  case AL_GOP_MODE_LOW_DELAY_B: return AL_GOP_MNGR_DEFAULT;
  case AL_GOP_MODE_PYRAMIDAL: return AL_GOP_MNGR_CUSTOM;
  case AL_GOP_MODE_DEFAULT: return AL_GOP_MNGR_DEFAULT;
  default: return AL_GOP_MNGR_MAX_ENUM;
  }

  return false;
}

