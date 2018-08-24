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

#include "TwoPassMngr.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <iostream>

#define SEQUENCE_SIZE_MAX 1000
#define LOCAL_RANGE 5

using namespace std;

/***************************************************************************/
/*Shared Methods for LookAhead and offline Twopass*/
/***************************************************************************/
static bool SceneChangeDetected(AL_TLookAheadMetaData* pPrevMeta, AL_TLookAheadMetaData* pCurrentMeta)
{
  if(!pPrevMeta || !pCurrentMeta)
    return false;

  auto iPercent = 100 * pCurrentMeta->iPercentIntra;
  auto iIntraRatio = (pPrevMeta->iPercentIntra != 0) ? iPercent / pPrevMeta->iPercentIntra : iPercent;
  return (pCurrentMeta->iPercentSkip < 5) && ((pCurrentMeta->iPercentIntra == 100) || (pCurrentMeta->iPercentIntra >= 80 && iIntraRatio > 200));
}

/***************************************************************************/
static int32_t GetIPRatio(AL_TLookAheadMetaData* pCurrentMeta, AL_TLookAheadMetaData* pNextMeta)
{
  if(!pCurrentMeta || !pNextMeta || !pNextMeta->iPictureSize)
    return 1000;

  return max(static_cast<int64_t>(100), 1000 * static_cast<int64_t>(pCurrentMeta->iPictureSize) / pNextMeta->iPictureSize);
}

/***************************************************************************/
/*LookAhead Methods*/
/***************************************************************************/
bool AL_TwoPassMngr_HasLookAhead(AL_TEncSettings settings)
{
  return settings.LookAhead > 0;
}

/***************************************************************************/
void AL_TwoPassMngr_SetPass1Settings(AL_TEncSettings& settings)
{
  settings.NumLayer = 1;
  auto& channel = settings.tChParam[0];
  channel.tRCParam.eRCMode = AL_RC_CONST_QP;
  channel.tRCParam.iInitialQP = 20;
  channel.tGopParam.eMode = AL_GOP_MODE_LOW_DELAY_P;
  channel.tGopParam.uGopLength = 0;
  channel.tGopParam.uNumB = 0;
}

/***************************************************************************/
bool AL_TwoPassMngr_SceneChangeDetected(AL_TBuffer* pPrevSrc, AL_TBuffer* pCurrentSrc)
{
  if(!pPrevSrc || !pCurrentSrc)
    return false;

  auto pPreviousMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pPrevSrc, AL_META_TYPE_LOOKAHEAD));
  auto pCurrentMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pCurrentSrc, AL_META_TYPE_LOOKAHEAD));

  return SceneChangeDetected(pPreviousMeta, pCurrentMeta);
}

/***************************************************************************/
int32_t AL_TwoPassMngr_GetIPRatio(AL_TBuffer* pCurrentSrc, AL_TBuffer* pNextSrc)
{
  auto pCurrentMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pCurrentSrc, AL_META_TYPE_LOOKAHEAD));
  auto pNextMeta = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(pNextSrc, AL_META_TYPE_LOOKAHEAD));

  return GetIPRatio(pCurrentMeta, pNextMeta);
}

/***************************************************************************/
/*Offline TwoPass methods*/
/***************************************************************************/
TwoPassMngr::TwoPassMngr(string p_FileName, int p_iPass)
{
  FileName = p_FileName;
  iPass = p_iPass;
  iCurrentFrame = 0;
  tFrames.clear();
  OpenLog();
}

/***************************************************************************/
TwoPassMngr::~TwoPassMngr()
{
  tFrames.clear();
  CloseLog();
}

/***************************************************************************/
void TwoPassMngr::OpenLog()
{
  if(iPass == 1)
    outputFile.open(FileName);

  if(iPass == 2)
    inputFile.open(FileName);
}

/***************************************************************************/
void TwoPassMngr::CloseLog()
{
  inputFile.close();
  outputFile.close();
}

/***************************************************************************/
void TwoPassMngr::EmptyLog()
{
  if(!inputFile.is_open())
    throw runtime_error("Can't open TwoPass LogFile");

  tFrames.clear();

  char sLine[256];
  bool bFind = true;
  int i = 0;

  while(bFind && !inputFile.eof() && i < SEQUENCE_SIZE_MAX)
  {
    inputFile.getline(sLine, 256);

    auto str_PicSize = strtok(sLine, " ");
    auto str_PercentIntra = strtok(NULL, " ");
    auto str_PercentSkip = strtok(NULL, " ");

    bFind = (str_PicSize != NULL && str_PercentIntra != NULL && str_PercentSkip != NULL);

    if(!bFind)
      break;

    AddNewFrame(atoi(str_PicSize), atoi(str_PercentIntra), atoi(str_PercentSkip));
    i++;
  }

  ComputeTwoPass();
}

/***************************************************************************/
void TwoPassMngr::FillLog()
{
  if(!outputFile.is_open())
    throw runtime_error("Can't open TwoPass LogFile");

  for(auto frame: tFrames)
    outputFile << frame.iPictureSize << " " << static_cast<int>(frame.iPercentIntra) << " " << static_cast<int>(frame.iPercentSkip) << endl;

  tFrames.clear();
}

/***************************************************************************/
void TwoPassMngr::AddNewFrame(int iPictureSize, int iPercentIntra, int iPercentSkip)
{
  AL_TLookAheadMetaData tParams;
  tParams.iPictureSize = iPictureSize;
  tParams.iPercentIntra = iPercentIntra;
  tParams.iPercentSkip = iPercentSkip;
  tParams.iComplexity = 0;
  tParams.bNextSceneChange = false;
  tParams.iIPRatio = 1000;
  tFrames.push_back(tParams);
}

/***************************************************************************/
void TwoPassMngr::AddFrame(AL_TLookAheadMetaData* pMetaData)
{
  if(!pMetaData)
    throw runtime_error("[Pass 1] : No twopass Metadata during pass1, can't create logfile");

  tFrames.push_back(*pMetaData);

  if(static_cast<int>(tFrames.size()) >= SEQUENCE_SIZE_MAX)
    FillLog();
}

/***************************************************************************/
void TwoPassMngr::Flush()
{
  FillLog();
}

/***************************************************************************/
void TwoPassMngr::GetFrame(AL_TLookAheadMetaData* pMetaData)
{
  if(iCurrentFrame >= static_cast<int>(tFrames.size()))
  {
    EmptyLog();
    iCurrentFrame = 0;

    if(static_cast<int>(tFrames.size()) == 0)
      throw runtime_error("[Pass 2] : Not enough frames from pass 1 Logfile");
  }
  AL_LookAheadMetaData_Copy(&tFrames[iCurrentFrame], pMetaData);
  iCurrentFrame++;
}

/***************************************************************************/
AL_TLookAheadMetaData* TwoPassMngr::CreateAndAttachTwoPassMetaData(AL_TBuffer* Src)
{
  auto pPictureMetaTP = reinterpret_cast<AL_TLookAheadMetaData*>(AL_Buffer_GetMetaData(Src, AL_META_TYPE_LOOKAHEAD));

  if(!pPictureMetaTP)
  {
    pPictureMetaTP = AL_LookAheadMetaData_Create();
    bool success = AL_Buffer_AddMetaData(Src, reinterpret_cast<AL_TMetaData*>(pPictureMetaTP));
    assert(success);
  }
  AL_LookAheadMetaData_Reset(pPictureMetaTP);
  return pPictureMetaTP;
}

/***************************************************************************/
void TwoPassMngr::ComputeTwoPass()
{
  ComputeComplexity();
  auto iSequenceSize = static_cast<int>(tFrames.size());

  for(int i = 0; i < iSequenceSize - 1; i++)
    tFrames[i].bNextSceneChange = SceneChangeDetected(&tFrames[i], &tFrames[i + 1]);

  for(int i = 0; i < iSequenceSize - 1; i++)
  {
    tFrames[i].iIPRatio = GetIPRatio(&tFrames[i], &tFrames[i + 1]);

    for(int k = i + 2; k < min(iSequenceSize, i + 4) && !SceneChangeDetected(&tFrames[k - 1], &tFrames[k]); k++)
      tFrames[i].iIPRatio = min(tFrames[i].iIPRatio, GetIPRatio(&tFrames[i], &tFrames[k]));
  }
}

/***************************************************************************/
static int GetLocalComplexity(vector<AL_TLookAheadMetaData> tFrames, int iLocalIndex, int iIndexMax, size_t zPicSizeMoy)
{
  if(iIndexMax - iLocalIndex < LOCAL_RANGE)
    return 1000;

  size_t zSumLocal = 0;

  for(int k = 0; k < LOCAL_RANGE; k++)
    zSumLocal += tFrames[iLocalIndex + k].iPictureSize;

  return 1000 * (zSumLocal / LOCAL_RANGE) / zPicSizeMoy;
}

/***************************************************************************/
void TwoPassMngr::ComputeComplexity()
{
  auto iSequenceSize = static_cast<int>(tFrames.size());
  assert(iSequenceSize > 0);
  assert(iSequenceSize <= SEQUENCE_SIZE_MAX);

  size_t zSumPicSize = 0;

  for(auto frame: tFrames)
    zSumPicSize += frame.iPictureSize;

  auto zPicSizeMoy = zSumPicSize / iSequenceSize;

  int iComplexity = 1000;

  for(int k = 0; k < iSequenceSize; k++)
  {
    if(k % LOCAL_RANGE == 0)
      iComplexity = GetLocalComplexity(tFrames, k, iSequenceSize, zPicSizeMoy);
    tFrames[k].iComplexity = iComplexity;
  }
}

