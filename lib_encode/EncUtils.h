/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/PPS.h"
#include "lib_common/AUD.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/VPS.h"

#include "lib_common_enc/Settings.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common_enc/EncEPBuffer.h"

#define MAX_SPS_IDS 32
#define MAX_PPS_IDS (AL_MAX_NUM_B_PICT + 3 + 1)

typedef struct
{
  bool bHasBeenSent;
  int iRefCnt;
  AL_TDimension size;
}AL_TSpsCtx;

typedef struct
{
  bool bHasBeenSent;
  int iRefCnt;
  uint8_t uSpsId;
  int8_t iQpCrOffset;
  int8_t iQpCbOffset;
}AL_TPpsCtx;

typedef struct
{
  int iPrevSps;
  AL_TSpsCtx spsCtx[MAX_SPS_IDS];
  int iPrevPps;
  AL_TPpsCtx ppsCtx[MAX_PPS_IDS];
}AL_THeadersCtx;

typedef struct AL_t_EncPicStatus AL_TEncPicStatus;
typedef struct AL_t_HLSInfo AL_HLSInfo;

/****************************************************************************/
#define MAX_IDX_BIT_PER_PEL 28
static const int AL_BitPerPixelQP[2][MAX_IDX_BIT_PER_PEL + 1][2] = /* x1000 */
{
  // AVC
  {
    { 33, 40 },
    { 37, 39 },
    { 41, 38 },
    { 45, 37 },
    { 51, 36 },
    { 56, 35 },
    { 62, 34 },
    { 70, 33 },
    { 76, 32 },
    { 83, 31 },
    { 94, 30 },
    { 104, 29 },
    { 114, 28 },
    { 129, 27 },
    { 140, 26 },
    { 156, 25 },
    { 176, 24 },
    { 194, 23 },
    { 215, 22 },
    { 241, 21 },
    { 262, 20 },
    { 289, 19 },
    { 321, 18 },
    { 349, 17 },
    { 382, 16 },
    { 423, 15 },
    { 458, 14 },
    { 504, 13 },
    { 551, 12 },
  },
  // HEVC / AOM
  {
    { 3, 40 },
    { 8, 39 },
    { 11, 38 },
    { 14, 37 },
    { 18, 36 },
    { 23, 35 },
    { 28, 34 },
    { 32, 33 },
    { 40, 32 },
    { 48, 31 },
    { 56, 30 },
    { 64, 29 },
    { 104, 28 },
    { 114, 27 },
    { 129, 26 },
    { 140, 25 },
    { 156, 24 },
    { 176, 23 },
    { 194, 22 },
    { 215, 21 },
    { 241, 20 },
    { 262, 19 },
    { 289, 18 },
    { 321, 17 },
    { 349, 16 },
    { 382, 15 },
    { 423, 14 },
    { 458, 13 },
    { 504, 12 },
  }
};

/****************************************************************************/
typedef enum
{
  VIDEO_FORMAT_COMPONENT,
  VIDEO_FORMAT_PAL,
  VIDEO_FORMAT_NTSC,
  VIDEO_FORMAT_SECAM,
  VIDEO_FORMAT_MAC,
  VIDEO_FORMAT_UNSPECIFIED,
}EVUIVideoFormat;

bool isBaseLayer(int iLayer);
/****************************************************************************/
void AL_Decomposition(uint32_t* y, uint8_t* x);
void AL_Reduction(uint32_t* pN, uint32_t* pD);

/****************************************************************************/
void AL_UpdateAspectRatio(AL_TVuiParam* pVuiParam, uint32_t uWidth, uint32_t uHeight, AL_EAspectRatio eAspectRatio);
void AL_UpdateSarAspectRatio(AL_TVuiParam* pVuiParam, uint32_t uWidth, uint32_t uHeight, AL_EAspectRatio eAspectRatio);

bool AL_IsGdrEnabled(AL_TEncChanParam const* pChParam);

void AL_UpdateVuiTimingInfo(AL_TVuiParam* pVUI, int iLayerId, AL_TRCParam const* pRCParam, int iTimeScaleFactor);
int DeduceNumTemporalLayer(AL_TGopParam const* pGop);
bool HasCuQpDeltaDepthEnabled(AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam);

/****************************************************************************/
void AL_AVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, uint8_t chroma_format_idc, TBufferEP* pBufEP);

void AL_AVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, int iMaxRef, int iCpbSize);
void AL_AVC_UpdateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs);
bool AL_AVC_UpdateAUD(AL_TAud* pAud, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, int iLayerID);
void AL_AVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TSps const* pSPS);

bool AL_AVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs, bool bForcePpsIdToZero);

/****************************************************************************/
void AL_HEVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP);

void AL_HEVC_GenerateVPS(AL_TVps* pIVPS, AL_TEncSettings const* pSettings, int iMaxRef);
void AL_HEVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChanParam, int iMaxRef, int iCpbSize, int iLayerId);
void AL_HEVC_UpdateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs, int iLayerId);
bool AL_HEVC_UpdateAUD(AL_TAud* pAud, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, int iLayerID);
void AL_HEVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChanParam, int iMaxRef, int iLayerId);

bool AL_HEVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncPicStatus const* pPicStatus, AL_HLSInfo const* pHLSInfo, AL_THeadersCtx* pHdrs, int iLayerId);

uint8_t AL_GetSps(AL_THeadersCtx* pHdrs, uint16_t uWidth, uint16_t uHeight);
uint8_t AL_GetPps(AL_THeadersCtx* pHdrs, uint8_t uSpsId, int8_t iQpCbOffset, int8_t iQpCrOffset);
void AL_ReleaseSps(AL_THeadersCtx* pHdrs, uint8_t id);
void AL_ReleasePps(AL_THeadersCtx* pHdrs, uint8_t id);
bool AL_IsWriteSps(AL_THeadersCtx* pHdrs, uint8_t id);
bool AL_IsWritePps(AL_THeadersCtx* pHdrs, uint8_t id);
AL_TDimension AL_GetPpsDim(AL_THeadersCtx* pHdrs, uint8_t id);

