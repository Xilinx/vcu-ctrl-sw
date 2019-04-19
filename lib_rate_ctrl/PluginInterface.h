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

#include <stddef.h>
#include <stdint.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_common/Allocator.h"
#include "lib_common/SliceConsts.h"

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

typedef struct Plugin_t_Statistics
{
  uint32_t uNumLCUs; /*!< Total number of encoded LCU */
  uint32_t uNumBytes; /*!< Number of bytes in the stream or Estimated Number of bytes in the stream (AVC multi-core only) */
  uint32_t uNumBins; /*!< Number of CABAC bin */
  uint32_t uNumIntra; /*!< Number of 8x8 blocks coded with intra mode */
  uint32_t uNumSkip; /*!< Number of 8x8 blocks coded with skip mode */
  uint32_t uNumCU8x8; /*!< Number of 8x8 CUs */
  uint32_t uNumCU16x16; /*!< Number of 16x16 CUs */
  uint32_t uNumCU32x32; /*!< Number of 32x32 CUs */
  int32_t uSumQP; /*!< Sum of QP value used to encode each block unit */
  int16_t uMinQP; /*!< Minimum QP value */
  int16_t uMaxQP; /*!< Maximum QP value */
}Plugin_Statistics;

typedef struct t_RC_Plugin_Vtable
{
  void (* setStreamInfo)(void* pHandle, int iWidth, int iHeight);
  void (* setRateControlParameters)(void* pHandle, Plugin_RCParam const* pRCParam, Plugin_GopParam const* pGopParam);
  void (* checkCompliance)(void* pHandle, Plugin_PictureInfo* pPicInfo, Plugin_Statistics* pStatus, int iPictureSize, int* pFillOrSkip);
  void (* update)(void* pHandle, Plugin_PictureInfo const* pPicInfo, Plugin_Statistics const* pStatus, int iPictureSize, bool bSkipped, int iFillerSize);
  void (* choosePictureQP)(void* pHandle, Plugin_PictureInfo const* pPicInfo, int16_t* pQP);
  void (* getRemovalDelay)(void* pHandle, int* pDelay);
  void (* deinit)(void* pHandle);
}RC_Plugin_Vtable;

typedef struct t_Mcu_DebugExport_Vtable
{
  void (* trace)(char*, size_t);
}Mcu_DebugExport_Vtable;

// should be at 0x80080000 (extension start address)
void* RC_Plugin_Init(RC_Plugin_Vtable* pRcPlugin, Mcu_DebugExport_Vtable* pDebug, AL_TAllocator* pAllocator) __attribute__((section(".text_plugin")));

