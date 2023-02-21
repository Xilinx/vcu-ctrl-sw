// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <list>
#include <iostream>
#include <string>
#include <vector>

#include "ICommandsSender.h"

struct CEncCmdMngr
{
  CEncCmdMngr(std::istream& CmdInput, int iLookAhead, uint32_t uFreqLT);

  void Process(ICommandsSender* sender, int iFrame);

private:
  std::istream& m_CmdInput;
  int const m_iLookAhead;
  uint32_t const m_uFreqLT;
  bool m_bHasLT;
  std::string m_sBufferedLine;

  struct TFrmCmd
  {
    int iFrame = 0;
    bool bSceneChange = false;
    bool bIsLongTerm = false;
    bool bUseLongTerm = false;
    bool bKeyFrame = false;
    bool bRecoveryPoint = false;
    bool bChangeGopLength = false;
    int iGopLength = 0;
    bool bChangeGopNumB = false;
    int iGopNumB = 0;
    bool bChangeFreqIDR = false;
    int iFreqIDR = 0;
    bool bChangeBitRate = false;
    int iBitRate = 0;
    bool bChangeMaxBitRate = false;
    int iTargetBitRate = 0;
    int iMaxBitRate = 0;
    bool bChangeFrameRate = false;
    int iFrameRate = 0;
    int iClkRatio = 0;
    bool bChangeQP = false;
    int iQP = 0;
    bool bChangeQPOffset = false;
    int iQPOffset = 0;
    bool bChangeQPBounds = false;
    int iMinQP = 0;
    int iMaxQP = 0;
    bool bChangeQPBounds_I = false;
    int iMinQP_I = 0;
    int iMaxQP_I = 0;
    bool bChangeQPBounds_P = false;
    int iMinQP_P = 0;
    int iMaxQP_P = 0;
    bool bChangeQPBounds_B = false;
    int iMinQP_B = 0;
    int iMaxQP_B = 0;
    bool bChangeIPDelta = false;
    int iIPDelta = 0;
    bool bChangePBDelta = false;
    int iPBDelta = 0;
    bool bChangeResolution = false;
    int iInputIdx;
    bool bSetLFBetaOffset = false;
    int iLFBetaOffset;
    bool bSetLFTcOffset = false;
    int iLFTcOffset;
    bool bSetCostMode = false;
    bool bCostMode;
    bool bMaxPictureSize = false;
    int iMaxPictureSize;
    bool bMaxPictureSize_I = false;
    int iMaxPictureSize_I;
    bool bMaxPictureSize_P = false;
    int iMaxPictureSize_P;
    bool bMaxPictureSize_B = false;
    int iMaxPictureSize_B;
    bool bChangeQPChromaOffsets = false;
    int iQp1Offset = 0;
    int iQp2Offset = 0;
    bool bSetAutoQP = false;
    bool bUseAutoQP = false;
    bool bChangeHDR = false;
    int iHDRIdx = 0;
  };

  std::list<TFrmCmd> m_Cmds;

  void Refill(int iCurFrame);
  bool ReadNextCmd(TFrmCmd& Cmd);
  bool ParseCmd(std::string sLine, TFrmCmd& Cmd, bool bSameFrame);
  bool GetNextLine(std::string& sNextLine);
};

