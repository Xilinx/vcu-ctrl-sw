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

#include "lib_bitstream/IRbspWriter.h"
#include "IP_Stream.h"

typedef struct t_NalUnit
{
  void (* Write)(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId);
  void const* param;
  int nut;
  int nalRefIdc;
  int layerId;
  int tempId;
  AL_TNalHeader header;
}AL_TNalUnit;

AL_TNalUnit AL_CreateAud(int nut, AL_TAud* aud, int tempId);
AL_TNalUnit AL_CreateSps(int nut, AL_TSps* sps, int layerId, int tempId);
AL_TNalUnit AL_CreatePps(int nut, AL_TPps* pps, int layerId, int tempId);
AL_TNalUnit AL_CreateVps(int nut, AL_TVps* vps, int tempId);

#include "lib_common_enc/EncPicInfo.h"
typedef struct t_SeiPrefixAPSCtx
{
  AL_TSps* sps;
  AL_THevcVps* vps;
}AL_TSeiPrefixAPSCtx;

AL_TNalUnit AL_CreateSeiPrefixAPS(AL_TSeiPrefixAPSCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixCtx
{
  AL_TSps* sps;
  int cpbInitialRemovalDelay;
  int cpbRemovalDelay;
  uint32_t uFlags;
  AL_TEncPicStatus const* pPicStatus;
  AL_THDRSEIs* pHDRSEIs;
}AL_TSeiPrefixCtx;

AL_TNalUnit AL_CreateSeiPrefix(AL_TSeiPrefixCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixUDUCtx
{
  uint8_t uuid[16];
  int8_t numSlices;
}AL_TSeiPrefixUDUCtx;

AL_TNalUnit AL_CreateSeiPrefixUDU(AL_TSeiPrefixUDUCtx* ctx, int nut, int tempId);

typedef struct t_SeiExternalCtx
{
  uint8_t* pPayload;
  int iPayloadType;
  int iPayloadSize;
}AL_TSeiExternalCtx;

AL_TNalUnit AL_CreateExternalSei(AL_TSeiExternalCtx* ctx, int nut, int tempId);

