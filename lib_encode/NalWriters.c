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

#include "NalWriters.h"
#include "lib_bitstream/RbspEncod.h"

static void audWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  writer->WriteAUD(bitstream, (int)(uintptr_t)param);
}

AL_NalUnit AL_CreateNalUnit(void (* Write)(IRbspWriter*, AL_TBitStreamLite*, void const*), void* param, int nut, int idc)
{
  AL_NalUnit nal = { 0 };
  nal.Write = Write;
  nal.param = param;
  nal.nut = nut;
  nal.idc = idc;
  return nal;
}

AL_NalUnit AL_CreateAud(int nut, AL_ESliceType eSliceType)
{
  AL_NalUnit nal = AL_CreateNalUnit(&audWrite, (void*)(uintptr_t)eSliceType, nut, 0);
  return nal;
}

static void spsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  writer->WriteSPS(bitstream, param);
}

AL_NalUnit AL_CreateSps(int nut, AL_TSps* sps, int layer_id)
{
  int iID = (nut == AL_AVC_NUT_SPS) ? 1 : layer_id;
  AL_NalUnit nal = AL_CreateNalUnit(&spsWrite, sps, nut, iID);
  return nal;
}

static void ppsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  writer->WritePPS(bitstream, param);
}

AL_NalUnit AL_CreatePps(int nut, AL_TPps* pps, int layer_id)
{
  int iID = (nut == AL_AVC_NUT_PPS) ? 1 : layer_id;
  AL_NalUnit nal = AL_CreateNalUnit(&ppsWrite, pps, nut, iID);
  return nal;
}

static void vpsWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  writer->WriteVPS(bitstream, param);
}

AL_NalUnit AL_CreateVps(AL_THevcVps* vps)
{
  AL_NalUnit nal = AL_CreateNalUnit(&vpsWrite, vps, AL_HEVC_NUT_VPS, 0);
  return nal;
}

static void seiPrefixAPSWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  SeiPrefixAPSCtx* pCtx = (SeiPrefixAPSCtx*)param;
  writer->WriteSEI_ActiveParameterSets(bitstream, pCtx->vps, pCtx->sps);
}

AL_NalUnit AL_CreateSeiPrefixAPS(SeiPrefixAPSCtx* ctx, int nut)
{
  AL_NalUnit nal = AL_CreateNalUnit(&seiPrefixAPSWrite, ctx, nut, 0);
  return nal;
}

static void seiPrefixWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  SeiPrefixCtx* pCtx = (SeiPrefixCtx*)param;
  uint32_t uFlags = pCtx->uFlags;

  while(uFlags)
  {
    if(uFlags & SEI_BP)
    {
      writer->WriteSEI_BufferingPeriod(bitstream, pCtx->sps, pCtx->cpbInitialRemovalDelay, 0);
      uFlags &= ~SEI_BP;
    }
    else if(uFlags & SEI_RP)
    {
      writer->WriteSEI_RecoveryPoint(bitstream, pCtx->pPicStatus->iRecoveryCnt);
      uFlags &= ~SEI_RP;
    }
    else if(uFlags & SEI_PT)
    {
      writer->WriteSEI_PictureTiming(bitstream, pCtx->sps,
                                     pCtx->cpbRemovalDelay,
                                     pCtx->pPicStatus->uDpbOutputDelay, pCtx->pPicStatus->ePicStruct);
      uFlags &= ~SEI_PT;
    }

    if(!uFlags)
      AL_RbspEncoding_CloseSEI(bitstream);
  }
}

AL_NalUnit AL_CreateSeiPrefix(SeiPrefixCtx* ctx, int nut)
{
  AL_NalUnit nal = AL_CreateNalUnit(&seiPrefixWrite, ctx, nut, 0);
  return nal;
}

static void seiSuffixWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  SeiSuffixCtx* pCtx = (SeiSuffixCtx*)param;
  writer->WriteSEI_UserDataUnregistered(bitstream, pCtx->uuid);
}

AL_NalUnit AL_CreateSeiSuffix(SeiSuffixCtx* ctx, int nut)
{
  AL_NalUnit nal = AL_CreateNalUnit(&seiSuffixWrite, ctx, nut, 0);
  return nal;
}

static void seiExternalWrite(IRbspWriter* writer, AL_TBitStreamLite* bitstream, void const* param)
{
  (void)writer;
  SeiExternalCtx* ctx = (SeiExternalCtx*)param;
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

AL_NalUnit AL_CreateExternalSei(SeiExternalCtx* ctx, int nut)
{
  return AL_CreateNalUnit(&seiExternalWrite, ctx, nut, 0);
}

