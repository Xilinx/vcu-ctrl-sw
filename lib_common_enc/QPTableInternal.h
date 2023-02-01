/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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
#include "lib_common/Error.h"
#include "lib_common_enc/QPTable.h"

/*************************************************************************//*!
   \brief Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uQpLCUGranularity QP block granularity in LCU unit
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_QPTable_GetFlexibleSize(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, uint8_t uQpLCUGranularity);

/*************************************************************************//*!
   \brief Checks that QP table is correctly filled
   \param[in] pQPTable Pointer to QP table
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] iQPTableDepth Depth of QP table
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] bDisIntra Disable intra prediction for inter slices
   \param[in] bRelative True if QPs are relative to SliceQP
   \return maximum size (in bytes) needed to store
*****************************************************************************/
AL_ERR AL_QPTable_CheckValidity(const uint8_t* pQPTable, AL_TDimension tDim, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bDisIntra, bool bRelative);

