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

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/SliceConsts.h"

/*************************************************************************//*!
   \brief Scene change enum
*****************************************************************************/
typedef enum AL_e_SceneChangeType
{
  AL_SC_NONE = 0x00000000,    /* No scene change detected */
  AL_SC_CURRENT = 0x00000001, /* Scene change in current frame (for LA1) */
  AL_SC_NEXT = 0x00000002,    /* Scene change in next frame */
  AL_SC_MAX_ENUM,
}AL_ESceneChangeType;

/*************************************************************************//*!
   \brief Structure used in LookAhead and Twopass, to transmits frame
    informations between the two pass
*****************************************************************************/
typedef struct AL_t_LookAheadMetaData
{
  AL_TMetaData tMeta;
  int32_t iPictureSize;   /*< current frame size */
  int8_t iPercentIntra[5];   /*< current frame Percent Intra Ratio */
  AL_ESceneChangeType eSceneChange;  /*< scene change description  */
  int32_t iIPRatio;       /*< current frame IPRatio for scene change*/
  int32_t iComplexity;    /*< current frame complexity */
  int32_t iTargetLevel;   /*< current frame target CPB level */
}AL_TLookAheadMetaData;

/*************************************************************************//*!
   \brief Create a look ahead metadata.
   The parameters are initialized to an invalid value (-1) by default.
*****************************************************************************/
AL_TLookAheadMetaData* AL_LookAheadMetaData_Create(void);
AL_TLookAheadMetaData* AL_LookAheadMetaData_Clone(AL_TLookAheadMetaData* pMeta);
void AL_LookAheadMetaData_Copy(AL_TLookAheadMetaData* pMetaSrc, AL_TLookAheadMetaData* pMetaDest);
void AL_LookAheadMetaData_Reset(AL_TLookAheadMetaData* pMeta);

/*@}*/

