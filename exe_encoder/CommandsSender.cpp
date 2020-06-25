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

#include "CommandsSender.h"

void CommandsSender::notifySceneChange(int lookAhead)
{
  AL_Encoder_NotifySceneChange(hEnc, lookAhead);
}

void CommandsSender::notifyIsLongTerm()
{
  AL_Encoder_NotifyIsLongTerm(hEnc);
}

void CommandsSender::notifyUseLongTerm()
{
  AL_Encoder_NotifyUseLongTerm(hEnc);
}

#include <iostream>

#define CHECK(statement) \
  if(!statement) \
    std::cerr << # statement << " failed with error : " << AL_Encoder_GetLastError(hEnc) << std::endl

void CommandsSender::restartGop()
{
  CHECK(AL_Encoder_RestartGop(hEnc));
}

void CommandsSender::setGopLength(int gopLength)
{
  CHECK(AL_Encoder_SetGopLength(hEnc, gopLength));
}

void CommandsSender::setNumB(int numB)
{
  CHECK(AL_Encoder_SetGopNumB(hEnc, numB));
}

void CommandsSender::setFreqIDR(int freqIDR)
{
  CHECK(AL_Encoder_SetFreqIDR(hEnc, freqIDR));
}

void CommandsSender::setFrameRate(int frameRate, int clockRatio)
{
  CHECK(AL_Encoder_SetFrameRate(hEnc, frameRate, clockRatio));
}

void CommandsSender::setBitRate(int bitRate)
{
  CHECK(AL_Encoder_SetBitRate(hEnc, bitRate));
}

void CommandsSender::setQP(int qp)
{
  CHECK(AL_Encoder_SetQP(hEnc, qp));
}

void CommandsSender::setQPBounds(int iMinQP, int iMaxQP)
{
  CHECK(AL_Encoder_SetQPBounds(hEnc, iMinQP, iMaxQP));
}

void CommandsSender::setQPIPDelta(int iQPDelta)
{
  CHECK(AL_Encoder_SetQPIPDelta(hEnc, iQPDelta));
}

void CommandsSender::setQPPBDelta(int iQPDelta)
{
  CHECK(AL_Encoder_SetQPPBDelta(hEnc, iQPDelta));
}

void CommandsSender::setDynamicInput(int iInputIdx)
{
  bInputChanged = true;
  this->iInputIdx = iInputIdx;
}

void CommandsSender::setLFBetaOffset(int iBetaOffset)
{
  CHECK(AL_Encoder_SetLoopFilterBetaOffset(hEnc, iBetaOffset));
}

void CommandsSender::setLFTcOffset(int iTcOffset)
{
  CHECK(AL_Encoder_SetLoopFilterTcOffset(hEnc, iTcOffset));
}

void CommandsSender::setHDRIndex(int iHDRIdx)
{
  bHDRChanged = true;
  this->iHDRIdx = iHDRIdx;
}

void CommandsSender::Reset()
{
  bInputChanged = false;
  bHDRChanged = false;
}

bool CommandsSender::HasInputChanged(int& iInputIdx)
{
  iInputIdx = this->iInputIdx;
  return bInputChanged;
}

bool CommandsSender::HasHDRChanged(int& iHDRIdx)
{
  iHDRIdx = this->iHDRIdx;
  return bHDRChanged;
}
