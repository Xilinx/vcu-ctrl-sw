/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "CommandsSender.h"

using namespace std;

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

void CommandsSender::restartGopRecoveryPoint()
{
  CHECK(AL_Encoder_RestartGopRecoveryPoint(hEnc));
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

void CommandsSender::setMaxBitRate(int iTargetBitRate, int iMaxBitRate)
{
  CHECK(AL_Encoder_SetMaxBitRate(hEnc, iTargetBitRate, iMaxBitRate));
}

void CommandsSender::setQP(int qp)
{
  CHECK(AL_Encoder_SetQP(hEnc, qp));
}

void CommandsSender::setQPOffset(int iQpOffset)
{
  CHECK(AL_Encoder_SetQPOffset(hEnc, iQpOffset));
}

void CommandsSender::setQPBounds(int iMinQP, int iMaxQP)
{
  CHECK(AL_Encoder_SetQPBounds(hEnc, iMinQP, iMaxQP));
}

void CommandsSender::setQPBounds_I(int iMinQP_I, int iMaxQP_I)
{
  CHECK(AL_Encoder_SetQPBoundsPerFrameType(hEnc, iMinQP_I, iMaxQP_I, AL_SLICE_I));
}

void CommandsSender::setQPBounds_P(int iMinQP_P, int iMaxQP_P)
{
  CHECK(AL_Encoder_SetQPBoundsPerFrameType(hEnc, iMinQP_P, iMaxQP_P, AL_SLICE_P));
}

void CommandsSender::setQPBounds_B(int iMinQP_B, int iMaxQP_B)
{
  CHECK(AL_Encoder_SetQPBoundsPerFrameType(hEnc, iMinQP_B, iMaxQP_B, AL_SLICE_B));
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

void CommandsSender::setCostMode(bool bCostMode)
{
  CHECK(AL_Encoder_SetCostMode(hEnc, bCostMode));
}

void CommandsSender::setMaxPictureSize(int iMaxPictureSize)
{
  CHECK(AL_Encoder_SetMaxPictureSize(hEnc, iMaxPictureSize));
}

void CommandsSender::setMaxPictureSize_I(int iMaxPictureSize)
{
  CHECK(AL_Encoder_SetMaxPictureSizePerFrameType(hEnc, iMaxPictureSize, AL_SLICE_I));
}

void CommandsSender::setMaxPictureSize_P(int iMaxPictureSize)
{
  CHECK(AL_Encoder_SetMaxPictureSizePerFrameType(hEnc, iMaxPictureSize, AL_SLICE_P));
}

void CommandsSender::setMaxPictureSize_B(int iMaxPictureSize)
{
  CHECK(AL_Encoder_SetMaxPictureSizePerFrameType(hEnc, iMaxPictureSize, AL_SLICE_B));
}

void CommandsSender::setQPChromaOffsets(int iQp1Offset, int iQp2Offset)
{
  CHECK(AL_Encoder_SetQPChromaOffsets(hEnc, iQp1Offset, iQp2Offset));
}

void CommandsSender::setAutoQP(bool bUseAutoQP)
{
  CHECK(AL_Encoder_SetAutoQP(hEnc, bUseAutoQP));
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
