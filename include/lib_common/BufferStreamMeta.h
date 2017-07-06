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

#include "lib_common/BufferMeta.h"
#include "lib_common/StreamSection.h"

#define AL_MAX_SECTION (2 * (AL_MAX_ENC_SLICE + 2))

/*************************************************************************//*!
   \brief MetaData for Stream buffer
*****************************************************************************/
typedef struct AL_t_StreamMetaData
{
  AL_TMetaData tMeta;
  uint32_t uMaxSize;
  uint32_t uOffset;
  uint32_t uAvailSize;

  uint16_t uMaxNumSection; /*!< Max Number of section */

  uint16_t uFirstSection; /*!< Number of section in the buffer  */
  uint16_t uNumSection; /*!< Number of section in the buffer  */
  uint16_t uNumMissingSection; /*!< Number of section missing in the buffer if the buffer is to small */

  AL_TStreamSection* pSections;  /*!< Array of sections */
}AL_TStreamMetaData;

AL_TStreamMetaData* AL_StreamMetaData_Create(uint16_t uMaxNumSection, uint32_t uMaxSize);
AL_TStreamMetaData* AL_StreamMetaData_Clone(AL_TStreamMetaData* pMeta);

