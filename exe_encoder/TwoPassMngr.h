// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <fstream>
#include <vector>
#include <cstring>
#include <deque>
#include <memory>

extern "C"
{
#include <lib_common/BufferLookAheadMeta.h>
#include <lib_common/BufferAPI.h>
#include <lib_common_enc/Settings.h>
}

bool AL_TwoPassMngr_HasLookAhead(AL_TEncSettings const& settings);
void AL_TwoPassMngr_SetPass1Settings(AL_TEncSettings& settings);
void AL_TwoPassMngr_SetGlobalSettings(AL_TEncSettings& settings);
AL_TLookAheadMetaData* AL_TwoPassMngr_CreateAndAttachTwoPassMetaData(AL_TBuffer* Src);

/***************************************************************************/
/*Offline TwoPass structures and methods*/
/***************************************************************************/

/*
** Struct for TwoPass management
** Writes First Pass information on the logfile
** Reads and computes the logfile for the Second Pass
*/
struct TwoPassMngr
{
  TwoPassMngr(std::string p_FileName, int p_iPass, bool p_bEnabledFirstPassSceneChangeDetection, int p_iGopSize, int p_iCpbLevel, int p_iInitialLevel, int p_iFrameRate);
  ~TwoPassMngr();

  void AddFrame(AL_TLookAheadMetaData* pMetaData);
  void GetFrame(AL_TLookAheadMetaData* pMetaData);
  void Flush();

  int iPass;
  bool bEnableFirstPassSceneChangeDetection;

private:
  void OpenLog();
  void CloseLog();
  void EmptyLog();
  void FillLog();
  void AddNewFrame(int iPictureSize, int iPercentIntra);
  void ComputeTwoPass();
  void ComputeComplexity();
  bool HasPatternTwoFrames();

  std::string FileName;
  std::vector<AL_TLookAheadMetaData> tFrames;
  std::ofstream outputFile;
  std::ifstream inputFile;
  int iCurrentFrame = 0;

  int iGopSize;
  int iCpbLevel;
  int iInitialLevel;
  int iFrameRate;
};

/***************************************************************************/
/*LookAhead structures and methods*/
/***************************************************************************/

/*
** Struct for LookAhead management
** Keeps the src buffers between the two pass
** Compute lookahead metadata to improve second pass quality
*/
struct LookAheadMngr
{
  LookAheadMngr(int p_iLookAhead, bool p_bEnableFirstPassSceneChangeDetection);
  ~LookAheadMngr();

  uint16_t uLookAheadSize;
  bool bUseComplexity;
  bool bEnableFirstPassSceneChangeDetection;
  std::deque<AL_TBuffer*> m_fifo;

  void ProcessLookAheadParams();
  void ComputeComplexity();
  bool HasPatternTwoFrames();
  bool ComputeSceneChange(AL_TBuffer* pPrevSrc, AL_TBuffer* pCurrentSrc);
  bool ComputeSceneChange_LA1(AL_TBuffer* pCurrentSrc);
  int32_t ComputeIPRatio(AL_TBuffer* pCurrentSrc, AL_TBuffer* pNextSrc);
  int GetNextSceneChange();

private:
  int iComplexity;
  int iFrameCount;
  int iComplexityDiff;
  AL_TLookAheadMetaData tPrevMetaData;
};

