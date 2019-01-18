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
   \addtogroup Decoder_Settings
   @{
   \file
 *****************************************************************************/
#pragma once
/*************************************************************************//*!
   \brief Decoder DPB mode enum
*****************************************************************************/
typedef enum AL_e_DpbMode
{
  AL_DPB_NORMAL, /*< Follow DPB specification */
  AL_DPB_NO_REORDERING, /*< Assume there is no reordering in the stream */
  AL_DPB_MAX_ENUM, /* sentinel */
}AL_EDpbMode;

/*************************************************************************//*!
   \brief Retrieves the maximum DBP size respect to the level constraint
   \param[in] iLevel Level of the current H264 stream
   \param[in] iWidth Width of the current H264 stream
   \param[in] iHeight Height of the current H264 stream
   \return return the maximum size of the DBP allowed by the specified level
*****************************************************************************/
int AL_AVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight);

/*************************************************************************//*!
   \brief Retrieves the maximum DBP size respect to the level constraint
   \param[in] iLevel Level of the current HEVC stream (i.e. general_level_idc / 3)
   \param[in] iWidth Width of the current HEVC stream
   \param[in] iHeight Height of the current HEVC stream
   \return return the maximum size of the DBP allowed by the specified level
 ***************************************************************************/
int AL_HEVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight);

/*@}*/

