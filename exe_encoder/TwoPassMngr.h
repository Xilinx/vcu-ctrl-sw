/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#pragma once

#include <fstream>
#include <vector>
#include <cstring>

extern "C"
{
#include <lib_common/BufferLookAheadMeta.h>
#include <lib_common/BufferAPI.h>
#include <lib_common_enc/Settings.h>
}

bool AL_TwoPassMngr_HasLookAhead(AL_TEncSettings settings);
void AL_TwoPassMngr_SetPass1Settings(AL_TEncSettings& settings);
bool AL_TwoPassMngr_SceneChangeDetected(AL_TBuffer* pPrevSrc, AL_TBuffer* pCurrentSrc);

/*************************************************************************//*!
   \brief Computes and returns the IPRatio between the two frames
   \param[in] pCurrentSrc       Pointer to the I frame
   \param[in] pNextSrc       Pointer to the P frame
   \return the computed IPRatio
*****************************************************************************/
int32_t AL_TwoPassMngr_GetIPRatio(AL_TBuffer* pCurrentSrc, AL_TBuffer* pNextSrc);

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
  TwoPassMngr(std::string p_FileName, int p_iPass);
  ~TwoPassMngr();

  void OpenLog();
  void CloseLog();
  void EmptyLog();
  void FillLog();
  void AddNewFrame(int iPictureSize, int iPercentIntra, int iPercentSkip);
  void AddFrame(AL_TLookAheadMetaData* pMetaData);
  void GetFrame(AL_TLookAheadMetaData* pMetaData);
  void Flush();
  AL_TLookAheadMetaData* CreateAndAttachTwoPassMetaData(AL_TBuffer* Src);
  void ComputeTwoPass();
  void ComputeComplexity();

  int iPass;
  std::string FileName;
  std::vector<AL_TLookAheadMetaData> tFrames;
  int iCurrentFrame;
  std::ofstream outputFile;
  std::ifstream inputFile;
};

