/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common_enc/Settings.h"
#include "lib_common_enc/EncSliceStatus.h"
#include "lib_common_enc/EncEPBuffer.h"
#include "lib_common/BufferSrcMeta.h"

#include "lib_common/PPS.h"

typedef struct AL_t_EncPicStatus AL_TEncPicStatus;
typedef struct AL_t_HLSInfo AL_HLSInfo;

/****************************************************************************/
static const int MAX_IDX_BIT_PER_PEL = 35;
static const int AL_BitPerPixelQP[36][2] = /* x1000 */
{
  // optimistic // average //pessimistic
  { 33, 40 }, // {  95, 37}, //{ 145, 37},
  { 37, 39 }, // {  95, 37}, //{ 145, 37},
  { 41, 38 }, // {  95, 37}, //{ 145, 37},
  { 45, 37 }, // {  95, 37}, //{ 145, 37},
  { 51, 36 }, // { 113, 36}, //{ 175, 36},
  { 56, 35 }, // { 131, 35}, //{ 206, 35},
  { 62, 34 }, // { 151, 34}, //{ 239, 34},
  { 70, 33 }, // { 172, 33}, //{ 274, 33},
  { 76, 32 }, // { 194, 32}, //{ 312, 32},
  { 83, 31 }, // { 217, 31}, //{ 351, 31},
  { 94, 30 }, // { 244, 30}, //{ 394, 30},
  { 104, 29 }, // { 272, 29}, //{ 439, 29},
  { 114, 28 }, // { 301, 28}, //{ 487, 28},
  { 129, 27 }, // { 334, 27}, //{ 539, 27},
  { 140, 26 }, // { 368, 26}, //{ 595, 26},
  { 156, 25 }, // { 406, 25}, //{ 656, 25},
  { 176, 24 }, // { 449, 24}, //{ 721, 24},
  { 194, 23 }, // { 493, 23}, //{ 792, 23},
  { 215, 22 }, // { 543, 22}, //{ 870, 22},
  { 241, 21 }, // { 598, 21}, //{ 954, 21},
  { 262, 20 }, // { 655, 20}, //{1047, 20},
  { 289, 19 }, // { 720, 19}, //{1150, 19},
  { 321, 18 }, // { 793, 18}, //{1265, 18},
  { 349, 17 }, // { 871, 17}, //{1392, 17},
  { 382, 16 }, // { 959, 16}, //{1535, 16},
  { 423, 15 }, // {1060, 15}, //{1697, 15},
  { 458, 14 }, // {1170, 14}, //{1882, 14},
  { 504, 13 }, // {1300, 13}, //{2095, 13},
  { 551, 12 }, // {1447, 12}, //{2343, 12},
};

/****************************************************************************/
void AL_HEVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP);
void AL_AVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP);

/****************************************************************************/
void AL_HEVC_GenerateVPS(AL_THevcVps* pVPS, AL_TEncSettings const* pSettings, int iMaxRef);

void AL_HEVC_GenerateSPS(AL_TSps* pSPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChanParam, int iMaxRef, int iCpbSize, int iLayerId);
void AL_AVC_GenerateSPS(AL_TSps* pSPS, AL_TEncSettings const* pSettings, int iMaxRef, int iCpbSize);

void AL_HEVC_UpdateSPS(AL_TSps* pISPS, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, int iLayerId);
void AL_AVC_UpdateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo);

void AL_HEVC_GeneratePPS(AL_TPps* pPPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChanParam, int iMaxRef, int iLayerId);
void AL_AVC_GeneratePPS(AL_TPps* pPPS, AL_TEncSettings const* pSettings, int iMaxRef, AL_TSps const* pSPS);

bool AL_HEVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, int iLayerId);
bool AL_AVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo);

