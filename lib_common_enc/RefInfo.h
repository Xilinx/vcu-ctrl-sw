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

#include "lib_common/SliceConsts.h"
#include "lib_common/Utils.h"
#include "Reordering.h"

/*************************************************************************//*!
   \brief Reference Picture informations
*****************************************************************************/
typedef struct AL_t_RefParam
{
  uint32_t eType; // ESliceType
  AL_EMarkingRef eRefMode; /*!< Freame reference mode */
  int32_t iPOC; /*!< Picture Order Count */
  int32_t iRefAPOC;
  int32_t iRefBPOC;
}AL_TRefParam;

typedef struct AL_t_RefInfo
{
  AL_TRefParam RefA; /*!< Specifies the picture reference mode used for reference A */
  AL_TRefParam RefB; /*!< Specifies the picture reference mode used for reference B */
  AL_TRefParam RefColoc; /*!< Specifies the picture reference mode used for the colocated reference */
  AL_TRefParam RefF[3]; /*!< Specifies the picture reference mode used for the eventual following picture */

  uint8_t uNumRefIdxL0;
  uint8_t uNumRefIdxL1;

  uint8_t uRefIdxA;
  uint8_t uRefIdxB;

  bool bMergeLTL0; /*!< RefIdx0 L0 Picture Marking */
  bool bMergeLTL1; /*!< RefIdx0 L1 Picture Marking */

  int32_t iMMCORemovePicNum;
  bool bRmLT;

  bool bColocFromL0;
  bool bNoBackwardPredFlag;
  bool bRefColocL0Flag;
  bool bRefColocL0LTFlag;
  bool bRefColocL1LTFlag;
  AL_TReorder tReorder;
}AL_TRefInfo;

/*************************************************************************//*!
   \brief Available reference informations
*****************************************************************************/
typedef struct AL_t_RpsInfo
{
  // RPS
  int iNumShortTerm;                   /*!< Number of short term picture(s) in the DPB */
  int32_t pShortTermPOCs[AL_MAX_NUM_REF];  /*!< array of POC. on POC per the short term pictures in the DPB */

  int iNumFollow;                   /*!< Number of short term following picture(s) in the DPB */
  int32_t pFollowPOCs[AL_MAX_NUM_REF];  /*!< array of POC. on POC per the short term following pictures in the DPB */

  int iNumLongTerm;                    /*!< Number of long term picture(s) in the DPB */
  int32_t pLongTermPOCs[AL_MAX_NUM_REF];   /*!< array of POC. on POC per the long term pictures in the DPB */

}AL_TRpsInfo;

/*************************************************************************//*!
   \brief Available reference informations
*****************************************************************************/
typedef struct AL_t_AvailRef
{
  int iNumStRefPOCs; /*!< Number of Short Term Reference pictures available */
  int iNumLtRefPOCs; /*!< Number of Long Term Reference pictures available */
  int32_t pRefPOCs[2 * AL_MAX_NUM_REF]; /*!< POC list of the available reference picture(s) */
  AL_EMarkingRef eMarkingRef[2 * AL_MAX_NUM_REF]; /*!< Marking list of the available reference picture(s) */
  int32_t pType[2 * AL_MAX_NUM_REF]; /*!< Slice type of the available reference picture(s) */
}AL_TAvailRef;


