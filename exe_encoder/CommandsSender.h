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

#pragma once

#include <vector>

#include "ICommandsSender.h"
extern "C"
{
#include "lib_encode/lib_encoder.h"
}

struct CommandsSender : public ICommandsSender
{
public:
  explicit CommandsSender(AL_HEncoder hEnc) : hEnc{hEnc}
  {
    Reset();
  };
  ~CommandsSender() override = default;
  void notifySceneChange(int lookAhead) override;
  void notifyIsLongTerm() override;
  void notifyUseLongTerm() override;
  void restartGop() override;
  void restartGopRecoveryPoint() override;
  void setGopLength(int gopLength) override;
  void setNumB(int numB) override;
  void setFreqIDR(int freqIDR) override;
  void setFrameRate(int frameRate, int clockRatio) override;
  void setBitRate(int bitRate) override;
  void setMaxBitRate(int iTargetBitRate, int iMaxBitRate) override;
  void setQP(int qp) override;
  void setQPOffset(int iQpOffset) override;
  void setQPBounds(int iMinQP, int iMaxQP) override;
  void setQPBounds_I(int iMinQP_I, int iMaxQP_I) override;
  void setQPBounds_P(int iMinQP_P, int iMaxQP_P) override;
  void setQPBounds_B(int iMinQP_B, int iMaxQP_B) override;
  void setQPIPDelta(int iQPDelta) override;
  void setQPPBDelta(int iQPDelta) override;
  void setDynamicInput(int iInputIdx) override;
  void setLFBetaOffset(int iBetaOffset) override;
  void setLFTcOffset(int iTcOffset) override;
  void setCostMode(bool bCostMode) override;
  void setMaxPictureSize(int iMaxPictureSize) override;
  void setMaxPictureSize_I(int iMaxPictureSize_I) override;
  void setMaxPictureSize_P(int iMaxPictureSize_P) override;
  void setMaxPictureSize_B(int iMaxPictureSize_B) override;
  void setQPChromaOffsets(int iQp1Offset, int iQp2Offset) override;
  void setAutoQP(bool bUseAutoQP) override;
  void setHDRIndex(int iHDRIdx) override;

  void Reset();
  bool HasInputChanged(int& iInputIdx);
  bool HasHDRChanged(int& iHDRIdx);

private:
  AL_HEncoder hEnc;
  bool bInputChanged;
  int iInputIdx;
  bool bHDRChanged;
  int iHDRIdx;
};

