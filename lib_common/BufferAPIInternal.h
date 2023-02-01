/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "lib_common/BufferAPI.h"

/*************************************************************************//*!
   \brief Gets the buffer memory physical address. If the buffer contains
   multiple chunks, returns the physical address of the first chunk.
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns the physical address of the memory chunk if succeeded,
   0 otherwise
*****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddress(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Gets the physical address of a memory chunk of an AL_TBuffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iChunkIdx Index of the chunk
   \return Returns the physical address of the memory chunk if succeeded,
   0 otherwise
*****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddressChunk(const AL_TBuffer* pBuf, int iChunkIdx);
