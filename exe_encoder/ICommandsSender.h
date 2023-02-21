// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

struct ICommandsSender
{
  virtual ~ICommandsSender() = default;
  virtual void notifySceneChange(int lookAhead) = 0;
  virtual void notifyIsLongTerm() = 0;
  virtual void notifyUseLongTerm() = 0;
  virtual void restartGop() = 0;
  virtual void restartGopRecoveryPoint() = 0;
  virtual void setGopLength(int gopLength) = 0;
  virtual void setNumB(int numB) = 0;
  virtual void setFreqIDR(int freqIDR) = 0;
  virtual void setFrameRate(int frameRate, int clockRatio) = 0;
  virtual void setBitRate(int bitRate) = 0;
  virtual void setMaxBitRate(int iTargetBitRate, int iMaxBitRate) = 0;
  virtual void setQP(int qp) = 0;
  virtual void setQPOffset(int iQpOffset) = 0;
  virtual void setQPBounds(int iMinQP, int iMaxQP) = 0;
  virtual void setQPBounds_I(int iMinQP_I, int iMaxQP_I) = 0;
  virtual void setQPBounds_P(int iMinQP_P, int iMaxQP_P) = 0;
  virtual void setQPBounds_B(int iMinQP_B, int iMaxQP_B) = 0;
  virtual void setQPIPDelta(int iQPDelta) = 0;
  virtual void setQPPBDelta(int iQPDelta) = 0;
  virtual void setDynamicInput(int iInputIdx) = 0;
  virtual void setLFBetaOffset(int iBetaOffset) = 0;
  virtual void setLFTcOffset(int iTcOffset) = 0;
  virtual void setCostMode(bool bCostMode) = 0;
  virtual void setMaxPictureSize(int iMaxPictureSize) = 0;
  virtual void setMaxPictureSize_I(int iMaxPictureSize_I) = 0;
  virtual void setMaxPictureSize_P(int iMaxPictureSize_P) = 0;
  virtual void setMaxPictureSize_B(int iMaxPictureSize_B) = 0;
  virtual void setQPChromaOffsets(int iQp1Offset, int iQp2Offset) = 0;
  virtual void setAutoQP(bool bUseAutoQP) = 0;
  virtual void setHDRIndex(int iHDRIdx) = 0;
};

