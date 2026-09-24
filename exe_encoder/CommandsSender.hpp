// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

#include "ICommandsSender.hpp"

extern "C"
{
#include "lib_encode/lib_encode.h"
}

struct CommandsSender : public ICommandsSender
{
public:
  explicit CommandsSender(AL_HEncoder hEnc) : hEnc{hEnc}
  {
    Reset();
  };
  ~CommandsSender() override = default;
  void notifySceneChange(int32_t lookAhead) override;
  void notifyIsLongTerm() override;
  void notifyUseLongTerm() override;
  void restartGop() override;
  void restartGopRecoveryPoint() override;
  void setGopLength(int32_t gopLength) override;
  void setNumB(int32_t numB) override;
  void setFreqIDR(int32_t freqIDR) override;
  void setFrameRate(int32_t frameRate, int32_t clockRatio) override;
  void setBitRate(int32_t bitRate) override;
  void setMaxBitRate(int32_t iTargetBitRate, int32_t iMaxBitRate) override;
  void setQP(int32_t qp) override;
  void setQPOffset(int32_t iQpOffset) override;
  void setQPBounds(int32_t iMinQP, int32_t iMaxQP) override;
  void setQPBounds_I(int32_t iMinQP_I, int32_t iMaxQP_I) override;
  void setQPBounds_P(int32_t iMinQP_P, int32_t iMaxQP_P) override;
  void setQPBounds_B(int32_t iMinQP_B, int32_t iMaxQP_B) override;
  void setQPIPDelta(int32_t iQPDelta) override;
  void setQPPBDelta(int32_t iQPDelta) override;
  void setDynamicInput(int32_t iInputIdx) override;
  void setLFMode(int32_t iMode) override;
  void setLFBetaOffset(int32_t iBetaOffset) override;
  void setLFTcOffset(int32_t iTcOffset) override;
  void setCostMode(bool bCostMode) override;
  void setMaxPictureSize(int32_t iMaxPictureSize) override;
  void setMaxPictureSize_I(int32_t iMaxPictureSize_I) override;
  void setMaxPictureSize_P(int32_t iMaxPictureSize_P) override;
  void setMaxPictureSize_B(int32_t iMaxPictureSize_B) override;
  void setQPChromaOffsets(int32_t iQp1Offset, int32_t iQp2Offset) override;
  void setAutoQP(bool bUseAutoQP) override;
  void setHDRIndex(int32_t iHDRIdx) override;

  void Reset();
  bool HasInputChanged(int& iInputIdx);
  bool HasHDRChanged(int& iHDRIdx);

private:
  AL_HEncoder hEnc;
  bool bInputChanged;
  int32_t iInputIdx;
  bool bHDRChanged;
  int32_t iHDRIdx;
};
