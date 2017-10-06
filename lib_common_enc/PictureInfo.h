/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/SliceConsts.h"

/****************************************************************************/
static const int PicStructInfo[3] =
{
  2, // 0:frame
  4, // 1:frame doubling
  6  // 2:frame tripling
};

/*************************************************************************//*!
   \brief Video Mode
*****************************************************************************/
typedef enum AL_e_VideoMode
{ // Warning obf reserved !
  PROGRESSIVE = 0, /*!< Progressive */
}AL_EVideoMode;

static const uint32_t AL_PICT_INFO_IS_IDR = 0x00000001;
static const uint32_t AL_PICT_INFO_IS_REF = 0x00000002;
static const uint32_t AL_PICT_INFO_SCN_CHG = 0x00000004;
static const uint32_t AL_PICT_INFO_BEG_FRM = 0x00000008;
static const uint32_t AL_PICT_INFO_END_FRM = 0x00000010;
static const uint32_t AL_PICT_INFO_END_SRC = 0x00000020;
static const uint32_t AL_PICT_INFO_USE_LT = 0x00000040;

#define AL_IS_IDR(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_IS_IDR)
#define AL_IS_REF(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_IS_REF)
#define AL_SCN_CHG(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_SCN_CHG)
#define AL_USE_LT(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_USE_LT)

/*************************************************************************//*!
   \brief Picture informations
*****************************************************************************/
typedef struct AL_t_PictureInfo
{
  uint32_t uSrcOrder; /*!< Source picture number in display order */
  uint32_t uFlags; /*! IsIDR, IsRef, SceneChange ... */
  int32_t iPOC;
  int32_t iFrameNum;
  AL_ESliceType eType;
  AL_EPicStruct ePicStruct;
  AL_EMarkingRef eMarking;

  bool bForceLT[2]; /*!< Specifies if a following reference picture need to be markes as long-term */

  int32_t iDpbOutputDelay;
  uint8_t uRefPicSetIdx;
  int8_t iQpOffset;

  uint16_t uGdrPos; /*!< Gradual Refresh position */
  AL_EGdrMode eGdrMode; /*!< Gradual Refresh Mode */
  bool bIsGoldenRef;
}AL_TPictureInfo;

/*@}*/

