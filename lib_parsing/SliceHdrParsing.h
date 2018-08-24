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

#pragma once

#include "lib_common/SliceHeader.h"

#include "lib_common_dec/RbspParser.h"

#include "lib_parsing/Concealment.h"

/*************************************************************************//*!
   \brief This function parses an Avc SliceHeader
   \param[out] pSlice    Pointer to the slice header structure that will be filled
   \param[in]  pRP       Pointer to NAL buffer
   \param[in]  pConceal  Pointer on a structure receiving concealment information
   \param[in]  pPPSTable Pointer to a table where are stored the available PPS
   \return return true if the current slice header is valid
   false otherwise
*****************************************************************************/
bool AL_AVC_ParseSliceHeader(AL_TAvcSliceHdr* pSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_TAvcPps pPPSTable[]);

/*************************************************************************//*!
   \brief This function parses an Hevc SliceHeader
   \param[out] pSlice    Pointer to the slice header structure that will be filled
   \param[in]  pIndSlice Pointer to the slice header structure holding the last independent slice header
   syntax elements
   \param[in]  pRP       Pointer to NAL buffer
   \param[in]  pConceal  Pointer on a structure receiving concealment information
   \param[in]  pPPSTable Pointer to a table where are stored the available PPS
   \return return true if the current slice header is valid
   false otherwise
*****************************************************************************/
bool AL_HEVC_ParseSliceHeader(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pIndSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_THevcPps pPPSTable[]);

/*@}*/

