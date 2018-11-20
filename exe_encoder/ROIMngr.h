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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup ExeEncoder
   @{
   \file
 *****************************************************************************/
#pragma once

extern "C"
{
#include <lib_common_enc/EncBuffers.h>
}

/*************************************************************************//*!
   \brief Types of ROI
*****************************************************************************/
typedef enum al_eRoiQuality
{
  AL_ROI_QUALITY_HIGH = -5,
  AL_ROI_QUALITY_MEDIUM = 0,
  AL_ROI_QUALITY_LOW = +5,
  AL_ROI_QUALITY_DONT_CARE = +31,
  AL_ROI_QUALITY_MAX_ENUM,
}AL_ERoiQuality;

/*************************************************************************//*!
   \brief Priority in case of intersecting ROIs
*****************************************************************************/
typedef enum al_eRoiOrder
{
  AL_ROI_INCOMING_ORDER,
  AL_ROI_QUALITY_ORDER,
  AL_ROI_MAX_ORDER,
}AL_ERoiOrder;

struct AL_TRoiNode;

/*************************************************************************//*!
   \brief ROI Manager context
*****************************************************************************/
struct AL_TRoiMngrCtx
{
  int8_t iMinQP;
  int8_t iMaxQP;

  int iPicWidth;
  int iPicHeight;

  int iLcuWidth;
  int iLcuHeight;
  int iNumLCUs;
  uint8_t uLcuSize;

  AL_ERoiQuality eBkgQuality;
  AL_ERoiOrder eOrder;

  AL_TRoiNode* pFirstNode;
  AL_TRoiNode* pLastNode;
};

/*************************************************************************//*!
   \brief Create a ROI Manager context
   \param[in] iPicWidth Source width
   \param[in] iPicHeight Source Height
   \param[in] eProf Codec profile
   \param[in] eBkgQuality Default quality applied to the background
   \param[in] eOrder ROI priority
   \returns Pointer to the ROI Manager context created
*****************************************************************************/
AL_TRoiMngrCtx* AL_RoiMngr_Create(int iPicWidth, int iPicHeight, AL_EProfile eProf, AL_ERoiQuality eBkgQuality, AL_ERoiOrder eOrder);

/*************************************************************************//*!
   \brief Destroy a ROI Manager context
   \param[in] pCtx Pointer to the ROI Manager context to destroy
*****************************************************************************/
void AL_RoiMngr_Destroy(AL_TRoiMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Clear all ROIs of a ROI Manager
   \param[in] pCtx Pointer to the ROI Manager context to clear
*****************************************************************************/
void AL_RoiMngr_Clear(AL_TRoiMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Add a ROI to a ROI Manager
   \param[in] pCtx Pointer to the ROI Manager context
   \param[in] iPosX Left position of the ROI
   \param[in] iPosY Top position of the ROI
   \param[in] iWidth Width of the ROI
   \param[in] iHeight Height of the ROI
   \param[in] eQuality Quality of the ROI
   \param[in] pCtx Pointer to the ROI Manager context
   \return True if the ROI was successfully added
*****************************************************************************/
bool AL_RoiMngr_AddROI(AL_TRoiMngrCtx* pCtx, int iPosX, int iPosY, int iWidth, int iHeight, AL_ERoiQuality eQuality);

/*************************************************************************//*!
   \brief Fill a QP table buffer according to the configuration of a ROI Manager
   \param[in] pCtx Pointer to the ROI Manager context
   \param[in] iNumQPPerLCU Number of QP values for a codec LCU
   \param[in] iNumBytesPerLCU Number of bytes storing the QPs for one LCU
   \param[out] pBuf The buffer of QPs to fill
*****************************************************************************/
void AL_RoiMngr_FillBuff(AL_TRoiMngrCtx* pCtx, int iNumQPPerLCU, int iNumBytesPerLCU, uint8_t* pBuf);

