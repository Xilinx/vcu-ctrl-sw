/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "NalWriters.h"
#include "lib_common/Nuts.h"
#include "lib_common/SEI.h"
#include "lib_bitstream/RbspEncod.h"
#include "lib_assert/al_assert.h"

static void audWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  writer->WriteAUD(bitstream, (AL_ESliceType)(uintptr_t)param);
}

AL_TNalUnit AL_CreateNalUnit(void (* Write)(IRbspWriter*, AL_TBitStreamLite*, void const*, int), void const* param, int nut, int nalRefIdc, int layerId, int tempId)
{
  AL_TNalUnit nal = { 0 };
  nal.Write = Write;
  nal.param = param;
  nal.nut = nut;
  nal.nalRefIdc = nalRefIdc;
  nal.layerId = layerId;
  nal.tempId = tempId;
  return nal;
}

AL_TNalUnit AL_CreateAud(int nut, AL_ESliceType eSliceType, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&audWrite, (void*)(uintptr_t)eSliceType, nut, 0, 0, tempId);
  return nal;
}

static void spsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  writer->WriteSPS(bitstream, param, layerId);
}

AL_TNalUnit AL_CreateSps(int nut, AL_TSps* sps, int layerId, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&spsWrite, sps, nut, 1, layerId, tempId);
  return nal;
}

static void ppsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  writer->WritePPS(bitstream, param);
}

AL_TNalUnit AL_CreatePps(int nut, AL_TPps* pps, int layerId, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&ppsWrite, pps, nut, 1, layerId, tempId);
  return nal;
}

static void vpsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  writer->WriteVPS(bitstream, param);
}

AL_TNalUnit AL_CreateVps(AL_THevcVps* vps, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&vpsWrite, vps, AL_HEVC_NUT_VPS, 0, 0, tempId);
  return nal;
}

static void seiPrefixAPSWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  AL_TSeiPrefixAPSCtx* pCtx = (AL_TSeiPrefixAPSCtx*)param;
  writer->WriteSEI_ActiveParameterSets(bitstream, pCtx->vps, pCtx->sps);
}

AL_TNalUnit AL_CreateSeiPrefixAPS(AL_TSeiPrefixAPSCtx* ctx, int nut, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&seiPrefixAPSWrite, ctx, nut, 0, 0, tempId);
  return nal;
}

static void seiPrefixWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  AL_TSeiPrefixCtx* pCtx = (AL_TSeiPrefixCtx*)param;
  uint32_t uFlags = pCtx->uFlags;

  while(uFlags)
  {
    if(uFlags & AL_SEI_BP)
    {
      writer->WriteSEI_BufferingPeriod(bitstream, pCtx->sps, pCtx->cpbInitialRemovalDelay, 0);
      uFlags &= ~AL_SEI_BP;
    }
    else if(uFlags & AL_SEI_RP)
    {
      writer->WriteSEI_RecoveryPoint(bitstream, pCtx->pPicStatus->iRecoveryCnt);
      uFlags &= ~AL_SEI_RP;
    }
    else if(uFlags & AL_SEI_PT)
    {
      writer->WriteSEI_PictureTiming(bitstream, pCtx->sps,
                                     pCtx->cpbRemovalDelay,
                                     pCtx->pPicStatus->uDpbOutputDelay, pCtx->pPicStatus->ePicStruct);
      uFlags &= ~AL_SEI_PT;
    }
    else if(uFlags & AL_SEI_MDCV)
    {
      AL_Assert(pCtx->pHDRSEIs);
      writer->WriteSEI_MasteringDisplayColourVolume(bitstream, &pCtx->pHDRSEIs->tMDCV);
      uFlags &= ~AL_SEI_MDCV;
    }
    else if(uFlags & AL_SEI_CLL)
    {
      AL_Assert(pCtx->pHDRSEIs);
      writer->WriteSEI_ContentLightLevel(bitstream, &pCtx->pHDRSEIs->tCLL);
      uFlags &= ~AL_SEI_CLL;
    }
    else if(uFlags & AL_SEI_ST2094_10)
    {
      AL_Assert(pCtx->pHDRSEIs);
      writer->WriteSEI_ST2094_10(bitstream, &pCtx->pHDRSEIs->tST2094_10);
      uFlags &= ~AL_SEI_ST2094_10;
    }
    else if(uFlags & AL_SEI_ST2094_40)
    {
      AL_Assert(pCtx->pHDRSEIs);
      writer->WriteSEI_ST2094_40(bitstream, &pCtx->pHDRSEIs->tST2094_40);
      uFlags &= ~AL_SEI_ST2094_40;
    }

    if(!uFlags)
      AL_RbspEncoding_CloseSEI(bitstream);
  }
}

AL_TNalUnit AL_CreateSeiPrefix(AL_TSeiPrefixCtx* ctx, int nut, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&seiPrefixWrite, ctx, nut, 0, 0, tempId);
  return nal;
}

static void seiPrefixUDUWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)layerId;
  AL_TSeiPrefixUDUCtx* pCtx = (AL_TSeiPrefixUDUCtx*)param;
  writer->WriteSEI_UserDataUnregistered(bitstream, pCtx->uuid, pCtx->numSlices);
}

AL_TNalUnit AL_CreateSeiPrefixUDU(AL_TSeiPrefixUDUCtx* ctx, int nut, int tempId)
{
  AL_TNalUnit nal = AL_CreateNalUnit(&seiPrefixUDUWrite, ctx, nut, 0, 0, tempId);
  return nal;
}

static void seiExternalWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param, int layerId)
{
  (void)writer;
  (void)layerId;
  AL_TSeiExternalCtx* ctx = (AL_TSeiExternalCtx*)param;
  uint8_t* pPayload = ctx->pPayload;
  int iPayloadType = ctx->iPayloadType;
  int iPayloadSize = ctx->iPayloadSize;

  AL_RbspEncoding_BeginSEI2(bitstream, iPayloadType, iPayloadSize);
  uint8_t* curData = AL_BitStreamLite_GetCurData(bitstream);
  AL_BitStreamLite_SkipBits(bitstream, 8 * iPayloadSize);

  if(!bitstream->isOverflow)
    Rtos_Memcpy(curData, pPayload, iPayloadSize);
  AL_RbspEncoding_CloseSEI(bitstream);
}

AL_TNalUnit AL_CreateExternalSei(AL_TSeiExternalCtx* ctx, int nut, int tempId)
{
  return AL_CreateNalUnit(&seiExternalWrite, ctx, nut, 0, 0, tempId);
}

