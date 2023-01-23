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

/*************************************************************************//*!
   \addtogroup Decoder
   @{
   \file
*****************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecSynchro.h"

/*************************************************************************//*!
   \brief Channel Parameter structure
*****************************************************************************/
typedef struct AL_t_DecChannelParam
{
  int32_t iWidth; /*< width in pixels */
  int32_t iHeight; /*< height in pixels */
  uint8_t uLog2MaxCuSize;
  uint32_t uFrameRate;
  uint32_t uClkRatio;
  uint32_t uMaxLatency;
  uint8_t uNumCore;
  bool bNonRealtime;
  uint8_t uDDRWidth;
  bool bLowLat;
  bool bParallelWPP;
  bool bDisableCache;
  bool bFrameBufferCompression;
  bool bUseEarlyCallback; /*< LLP2: this only makes sense with special support for hw synchro */
  AL_EFbStorageMode eFBStorageMode;
  AL_ECodec eCodec;
  AL_EChromaMode eMaxChromaMode;
  int32_t iMaxSlices;
  int32_t iMaxTiles;
  AL_EDecUnit eDecUnit;
}AL_TDecChanParam;

/*@}*/

