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

/**************************************************************************//*!
   \addtogroup FourCC
   @{
   \file
 *****************************************************************************/
#pragma once
#include "lib_common/FourCC.h"

/*************************************************************************//*!
   \brief Returns the FOURCC identifier of the decoded frame buffer generated
   by the decoder according to the chroma mode, the bitdepth of the stream
   and the storage mode of the ip
   \param[in] picFmt source picture format
   \return return the corresponding TFourCC format
*****************************************************************************/
TFourCC AL_GetDecFourCC(AL_TPicFormat const picFmt);

/*************************************************************************//*!
   \brief Returns the TPicFormat of the decoded framebuffer generated by the decoder
   according to the chroma mode, the bitdepth of the stream, the storage and
   compression mode of the ip
   \param[in] eChromaMode decoded picture chroma mode
   \param[in] uBitDepth decoded picture bit depth
   \param[in] eStorageMode decoded picture storage mode
   \param[in] bIsCompressed true if decoded picture is compressed, false otherwise
   \return Returns the corresponding TPicFormat
*****************************************************************************/
AL_TPicFormat AL_GetDecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_EFbStorageMode eStorageMode, bool bIsCompressed);

