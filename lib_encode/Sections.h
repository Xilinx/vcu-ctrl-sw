/******************************************************************************
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

#pragma once
#include "IP_Stream.h"
#include "lib_bitstream/IRbspWriter.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SEI.h"
#include "lib_common/AUD.h"
#include "lib_common/HDR.h"

typedef struct t_nuts
{
  AL_TNalHeader (* GetNalHeader)(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId);
  int spsNut;
  int ppsNut;
  int vpsNut;
  int audNut;
  int fdNut;
  int seiPrefixNut;
  int seiSuffixNut;
  int phNut;
  int apsNut;
}AL_TNuts;

typedef struct
{
  int initialCpbRemovalDelay;
  int cpbRemovalDelay;
  AL_THDRSEIs* pHDRSEIs;
}AL_TSeiData;

typedef struct
{
  AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned;
  AL_TVps* vps;
  AL_TSps* sps;
  AL_TPps* pps;
  AL_TAud* aud;

  bool bMustWriteAud;
  AL_EFillerCtrlMode fillerCtrlMode;
  bool bMustWritePPS;
  bool bMustWriteDynHDR;
  AL_ESeiFlag seiFlags;
  AL_TSeiData seiData;
}AL_TNalsData;

void GenerateSections(IRbspWriter* writer, AL_TNuts Nuts, AL_TNalsData const* pNalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayerID, int iNumSlices, bool bSubframeLatency, bool bForceSEIRecoveryPointOnIDR);
int AL_WriteSeiSection(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, bool isPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, int iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);
bool AL_CreateNuts(AL_TNuts* nuts, AL_EProfile eProfile);

