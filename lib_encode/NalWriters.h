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

#pragma once

#include "lib_bitstream/IRbspWriter.h"
#include "lib_bitstream/SkippedPicture.h"
#include "IP_Stream.h"

typedef struct nalunit
{
  void (* Write)(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int iLayerId);
  void const* param;
  int nut;
  int idc;
  int iLayerId;
  int tempId;
  NalHeader header;
}AL_NalUnit;

AL_NalUnit AL_CreateAud(int nut, AL_ESliceType type, int tempId);
AL_NalUnit AL_CreateSps(int nut, AL_TSps* sps, int iLayerId, int tempId);
AL_NalUnit AL_CreatePps(int nut, AL_TPps* pps, int iLayerId, int tempId);
AL_NalUnit AL_CreateVps(AL_THevcVps* vps, int tempId);

#include "lib_common_enc/EncPicInfo.h"
typedef struct t_SeiPrefixAPSCtx
{
  AL_TSps* sps;
  AL_THevcVps* vps;
}SeiPrefixAPSCtx;

AL_NalUnit AL_CreateSeiPrefixAPS(SeiPrefixAPSCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixCtx
{
  AL_TSps* sps;
  int cpbInitialRemovalDelay;
  int cpbRemovalDelay;
  uint32_t uFlags;
  AL_TEncPicStatus const* pPicStatus;
}SeiPrefixCtx;

AL_NalUnit AL_CreateSeiPrefix(SeiPrefixCtx* ctx, int nut, int tempId);

typedef struct t_SeiPrefixUDUCtx
{
  uint8_t uuid[16];
  int8_t numSlices;
}SeiPrefixUDUCtx;

AL_NalUnit AL_CreateSeiPrefixUDU(SeiPrefixUDUCtx* ctx, int nut, int tempId);

typedef struct t_SeiExternalCtx
{
  uint8_t* pPayload;
  int iPayloadType;
  int iPayloadSize;
}SeiExternalCtx;

AL_NalUnit AL_CreateExternalSei(SeiExternalCtx* ctx, int nut, int tempId);

