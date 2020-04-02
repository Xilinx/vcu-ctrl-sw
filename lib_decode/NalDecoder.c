/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "NalDecoder.h"
#include "NalUnitParserPrivate.h"

AL_TSeiMetaData* GetSeiMetaData(AL_TDecCtx* pCtx)
{
  if(!pCtx->pInputBuffer)
    return NULL;

  return (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pCtx->pInputBuffer, AL_META_TYPE_SEI);
}

bool HasOngoingFrame(AL_TDecCtx* pCtx)
{
  return pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid;
}

bool CheckAvailSpace(AL_TDecCtx* pCtx, AL_TSeiMetaData* pMeta)
{
  uint32_t uLengthNAL = GetNonVclSize(&pCtx->Stream);
  int Offset = (uintptr_t)AL_SeiMetaData_GetBuffer(pMeta) - (uintptr_t)pMeta->pBuf;
  return Offset + uLengthNAL <= pMeta->maxBufSize;
}

void AL_DecodeOneNal(AL_NonVclNuts nuts, AL_NalParser parser, AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut nut, bool bIsLastAUNal, int* iNumSlice)
{
  if(parser.isSliceData(nut))
  {
    parser.decodeSliceData(pAUP, pCtx, nut, bIsLastAUNal, iNumSlice);
    return;
  }

  if((nut == nuts.seiPrefix || (nut == nuts.seiSuffix && pCtx->bIsBuffersAllocated)) && parser.parseSei)
  {
    bool bIsPrefix = (nut == nuts.seiPrefix);
    AL_TSeiMetaData* pMeta = GetSeiMetaData(pCtx);

    if(pMeta && !CheckAvailSpace(pCtx, pMeta))
    {
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, pCtx->uCurPocLsb);
      return;
    }
    AL_TRbspParser rp = pMeta ? getParserOnNonVclNal(pCtx, AL_SeiMetaData_GetBuffer(pMeta))
                        : getParserOnNonVclNalInternalBuf(pCtx);

    if(!parser.parseSei(pAUP, &rp, bIsPrefix, &pCtx->parsedSeiCB, pMeta))
      AL_Default_Decoder_SetError(pCtx, AL_WARN_SEI_OVERFLOW, pCtx->uCurPocLsb);
  }

  if(nut == nuts.sps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    parser.parseSps(pAUP, &rp, pCtx);
  }

  if(nut == nuts.pps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    parser.parsePps(pAUP, &rp, pCtx);
  }

  if(nut == nuts.vps && parser.parseVps)
  {
    AL_TRbspParser rp = getParserOnNonVclNalInternalBuf(pCtx);
    parser.parseVps(pAUP, &rp);
  }

  if((nut == nuts.eos) || (nut == nuts.eob) || ((nut == nuts.fd) && pCtx->bSplitInput))
  {
    if(pCtx->bFirstIsValid && pCtx->bFirstSliceInFrameIsValid)
      parser.finishPendingRequest(pCtx);
    pCtx->bIsFirstPicture = true;
  }
}

