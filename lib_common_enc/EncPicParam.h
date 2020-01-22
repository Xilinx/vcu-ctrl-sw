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

#pragma once

#include "lib_common_enc/EncPicInfo.h"
#include "PictureInfo.h"
#include "RefInfo.h"

/*************************************************************************//*!
   \brief Picture parameters structure
*****************************************************************************/
typedef struct AL_t_EncPicParam
{
  AL_TEncInfo tEncInfo;
  AL_TPictureInfo tPicInfo;
  AL_TRefInfo tRefInfo;

  uint8_t uNumPicTotalCurr;
  int32_t iLastIdrId;
}AL_TEncPicParam;

typedef struct
{
  AL_64U pStrmUserPtr;
  uint8_t* pStream_v;
  AL_PADDR pStream;
  int32_t iMaxSize;
  int32_t iOffset;
  int32_t iStreamPartOffset;
  uint8_t* pExternalMV_v;
}AL_EncStreamInfo;
/*************************************************************************//*!
   \brief Picture buffers structure
*****************************************************************************/
typedef struct AL_t_RecInfo
{
  bool bIs10bits;
  uint32_t uPitch;
}AL_TRecInfo;

typedef struct AL_t_EncPicBufAddrsFull
{
  AL_TEncPicBufAddrs tBasic;

  AL_PADDR pRefA_Y;
  AL_PADDR pRefA_UV;
  AL_PADDR pRefA_MapY;
  AL_PADDR pRefA_MapUV;
  AL_PADDR pRefB_Y;
  AL_PADDR pRefB_UV;
  AL_PADDR pRefB_MapY;
  AL_PADDR pRefB_MapUV;
  AL_PADDR pRec_Y;
  AL_PADDR pRec_UV;
  AL_PADDR pRec_MapY;
  AL_PADDR pRec_MapUV;
  AL_PADDR pColoc;
  AL_PADDR pMV;
  AL_PADDR pWPP;
  AL_PADDR pEP1;
  AL_PADDR pEP3;
  AL_PADDR pIntermMap;
  AL_PADDR pIntermData;
  AL_TRecInfo tRecInfo;
  AL_EncStreamInfo* pStreamInfo;
  void* pMV_v;
  void* pWPP_v;
  void* pEP3_v;
  void* pEP1_v;
}AL_TEncPicBufAddrsFull;

