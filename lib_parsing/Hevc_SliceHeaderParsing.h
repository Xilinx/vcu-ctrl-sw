/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#pragma once

#include "Concealment.h"
#include "lib_common/SliceHeader.h"
#include "lib_common_dec/RbspParser.h"

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
