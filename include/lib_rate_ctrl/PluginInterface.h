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

#include <stddef.h>
#include <stdint.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Allocator.h"
#include "lib_common/SliceConsts.h"
#include "lib_common_enc/RateCtrlStats.h"

/* all dependencies here will need to be shipped to the implementer of the plugin */

typedef struct Plugin_t_RCParam
{
  uint32_t uInitialRemDelay;
  uint32_t uCPBSize;
  uint16_t uFrameRate;
  uint16_t uClkRatio;
  uint32_t uTargetBitRate;
  uint32_t uMaxBitRate;
  int16_t iInitialQP;
  int16_t iMinQP;
  int16_t iMaxQP;
  int16_t uIPDelta;
  int16_t uPBDelta;
  uint32_t eOptions;
}Plugin_RCParam;

typedef struct Plugin_t_GopParam
{
  uint32_t uFreqIDR;
  uint16_t uGopLength;
  uint8_t uNumB;
}Plugin_GopParam;

typedef struct Plugin_t_PictureInfo
{
  uint32_t uSrcOrder; /*!< Source picture number in display order */
  uint32_t uFlags; /*! IsIDR, IsRef, SceneChange ... */
  int32_t iPOC;
  int32_t iFrameNum;
  AL_ESliceType eType;
  AL_EPicStruct ePicStruct;
}Plugin_PictureInfo;

typedef AL_RateCtrl_Statistics Plugin_Statistics;

typedef struct t_RC_Plugin_Vtable
{
  void (* setStreamInfo)(void* pHandle, int iWidth, int iHeight);
  void (* setRateControlParameters)(void* pHandle, Plugin_RCParam const* pRCParam, Plugin_GopParam const* pGopParam);
  void (* checkCompliance)(void* pHandle, Plugin_Statistics* pStatus, int iPictureSize, bool bCheckSkip, int* pFillOrSkip);
  void (* update)(void* pHandle, Plugin_PictureInfo const* pPicInfo, Plugin_Statistics const* pStatus, int iPictureSize, bool bSkipped, int iFillerSize);
  void (* choosePictureQP)(void* pHandle, Plugin_PictureInfo const* pPicInfo, int16_t* pQP);
  void (* getRemovalDelay)(void* pHandle, int* pDelay);
  void (* deinit)(void* pHandle);
}RC_Plugin_Vtable;

typedef struct t_Mcu_Export_Vtable
{
  void (* trace)(char*, size_t);
  void (* invalidateCache)(uint32_t memoryBaseAddr, uint32_t memorySize);
}Mcu_Export_Vtable;

// should be at 0x80080000 (extension start address)
void* RC_Plugin_Init(RC_Plugin_Vtable* pRcPlugin, Mcu_Export_Vtable* pDebug, AL_TAllocator* pAllocator, AL_VADDR pDmaContext, uint32_t zDmaSize) __attribute__((section(".text_plugin")));
