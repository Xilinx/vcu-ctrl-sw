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
   \addtogroup lib_preprocess
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common_enc/Settings.h"

/*************************************************************************//*!
   \brief Fill QP part of the buffer pointed to by pQP with a QP for each
        Macroblock of the slice.
   \param[in]  eMode      Specifies the way QP values are computed. see EQpCtrlMode
   \param[in]  iSliceQP   Slice QP value (in range [0..51])
   \param[in]  iMinQP     Minimum allowed QP value (in range [0..50])
   \param[in]  iMaxQP     Maximum allowed QP value (in range [1..51]).
   \param[in]  iLCUWidth  Width in Lcu Unit of the picture
   \param[in]  iLCUHeight Height in Lcu Unit of the picture
   \param[in]  eProf      Profile used for the encoding
   \param[in]  iFrameID   Frame identifier
   \param[out] pQPs       Pointer to the buffer that receives the computed QPs
   \param[out] pSegs      Pointer to the buffer that receives the computed Segments
   \note iMinQp <= iMaxQP
   \return true on success, false on error
*****************************************************************************/
extern bool PreprocessQP(AL_EQpCtrlMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs, uint8_t* pSegs);

/****************************************************************************/

/*@}*/

