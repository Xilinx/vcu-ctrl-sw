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
   \defgroup Encoder_Settings Settings
   \ingroup Encoder

   @{
   \file
******************************************************************************/
#pragma once

#include "stdio.h"
#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/FourCC.h"
#include "EncChanParam.h"


/*************************************************************************//*!
   \brief Aspect Ratio identifer
*****************************************************************************/
typedef enum e_AspectRatio
{
  AL_ASPECT_RATIO_AUTO = 0x00,
  AL_ASPECT_RATIO_4_3 = 0x01,
  AL_ASPECT_RATIO_16_9 = 0x02,
  AL_ASPECT_RATIO_NONE = 0x03,
  AL_ASPECT_RATIO_MAX_ENUM,
}AL_EAspectRatio;

/*************************************************************************//*!
   \brief QP Control Mode
*****************************************************************************/
typedef enum e_QPCtrlMode
{
  // exclusive modes
  UNIFORM_QP = 0x00, /*!< default behaviour */
  CHOOSE_QP = 0x01, /*!< used for test purpose, need preprocessing */
  RAMP_QP = 0x02, /*!< used for test purpose */
  RANDOM_QP = 0x03, /*!< used for test purpose */
  LOAD_QP = 0x04, /*!< used for test purpose */
  BORDER_QP = 0x05, /*!< used for test purpose */
  ROI_QP = 0x06,
  MASK_QP_TABLE = 0x07,

  // additional modes
  RANDOM_SKIP = 0x20, /*!< used for test purpose */
  RANDOM_I_ONLY = 0x40, /*!< used for test purpose */

  BORDER_SKIP = 0x100,
  FULL_SKIP = 0x200,

  MASK_QP_TABLE_EXT = 0x367,

  // Auto QP
  AUTO_QP = 0x400, /*!< compute Qp by MB on the fly */
  ADAPTIVE_AUTO_QP = 0x800, /*!< Dynamically compute Qp by MB on the fly */
  MASK_AUTO_QP = 0xC00,

  // QP table mode
  RELATIVE_QP = 0x8000,
  QP_MAX_ENUM,
}AL_EQpCtrlMode;



/*************************************************************************//*!
   \brief Encoder Parameters
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct t_EncSettings
{
  // Stream
  AL_TEncChanParam tChParam[MAX_NUM_LAYER];
  bool bEnableAUD;
  bool bEnableFillerData;
  uint32_t uEnableSEI;

  AL_EAspectRatio eAspectRatio; /*!< specifies the display aspect ratio */
  AL_EColourDescription eColourDescription;
  AL_EScalingList eScalingList;
  bool bDependentSlice;

  bool bDisIntra;
  bool bForceLoad;
  int32_t iPrefetchLevel2;
  uint16_t uClipHrzRange;
  uint16_t uClipVrtRange;
  AL_EQpCtrlMode eQpCtrlMode;
  int NumView;
  int NumLayer;
  uint8_t ScalingList[4][6][64];
  uint8_t SclFlag[4][6];
  uint8_t DcCoeff[8];
  uint8_t DcCoeffFlag[8];
  bool bEnableWatchdog;
  int LookAhead;
  int TwoPass;
  bool bEnableFirstPassSceneChangeDetection;
}AL_TEncSettings;

/*************************************************************************//*!
   \brief Retrieves the default settings
   \param[out] pSettings Pointer to TEncSettings structure that receives
   default Settings.
*****************************************************************************/
void AL_Settings_SetDefaults(AL_TEncSettings* pSettings);

void AL_Settings_SetDefaultParam(AL_TEncSettings* pSettings);

void AL_Settings_SetDefaultRCParam(AL_TRCParam* pRCParam);


/*************************************************************************//*!
   \brief Checks that all encoding parameters are valids
   \param[in] pSettings Pointer to TEncSettings to be checked
   \param[in] pChParam Pointer to the channel parameters to be checked
   \param[in] pOut Optional standard stream on which verbose messages are
   written.
   \return If pSettings point to valid parameters the function returns false
   If pSettings point to invalid parameters the function returns
   the number of invalid parameters found (true);
   and this Settings can not be used with IP encoder.
*****************************************************************************/
int AL_Settings_CheckValidity(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, FILE* pOut);

/**************************************************************************//*!
   \brief Checks that encoding parameters are coherent between them.
   When incoherent parameter are found, the function automatically correct them.
   \param[in] pSettings Pointer to TEncSettings to be checked
   \param[in] pChParam Pointer to the channel parameters to be checked
   \param[in] tFourCC Encoding tFourCC format
   \param[in] pOut Optional standard stream on which verbose messages are
   written.
   \return 0 if no incoherency
           the number of incoherency if incoherency were found
   -1 if a fatal incoherency was found
   Since the function automatically apply correction,
   the Settings can be then used with IP encoder.
 *****************************************************************************/
int AL_Settings_CheckCoherency(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, TFourCC tFourCC, FILE* pOut);

/*@}*/

