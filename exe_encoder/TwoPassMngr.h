/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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
** Writes First Pass informations on the logfile
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

