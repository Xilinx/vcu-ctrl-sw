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

#pragma once

#define AL_MAX_SECTION (2 * (AL_MAX_ENC_SLICE + 2))

#include "lib_common/BufCommon.h"
#include "lib_common/StreamSection.h"
#include "lib_common/BufferAPI.h"

/*************************************************************************//*!
   \brief Adds a nex section to the HEVC Buffer
   \param[in] pStream  Pointer to the bitstream buffer
   \param[in] uOffset  Start offset of the section in number of bytes
   \param[in] uLength  Number of bytes of the section
   \param[in] uFlags Bit Field flags associated with the section
   \return the zero based index of the newly added section
*****************************************************************************/
uint16_t AddSection(AL_TBuffer* pStream, uint32_t uOffset, uint32_t uLength, uint32_t uFlags);

/*************************************************************************//*!
   \brief Allows to modify the information associated with the specified section
   \param[in] pStream Pointer to the bitstream buffer
   \param[in] uSectionID Section identifier (returned by AddSection)
   \param[in] uOffset Start offset of the section in number of bytes
   \param[in] uLength Number of bytes of the section
*****************************************************************************/
void ChangeSection(AL_TBuffer* pStream, uint16_t uSectionID, uint32_t uOffset, uint32_t uLength);

/*************************************************************************//*!
   \brief Updates a specific section's flag
   \param[in] pStream Pointer to the bitstream buffer
   \param[in] uSectionID Section identifier (returned by AddSection)
   \param[in] uFlags Bit Field flags associated with the section
*****************************************************************************/
void SetSectionFlags(AL_TBuffer* pStream, uint16_t uSectionID, uint32_t uFlags);

/*************************************************************************//*!
   \brief Removes all sections from HEVC Buffer
   \param[in] pStream Pointer to the bitstream buffer
*****************************************************************************/
void ClearAllSections(AL_TBuffer* pStream);

