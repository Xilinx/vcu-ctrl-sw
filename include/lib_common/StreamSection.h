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

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 **************************************************************************/
#pragma once

typedef enum
{
  AL_SECTION_NO_FLAG = 0, /*< no flag */
  AL_SECTION_SEI_PREFIX_FLAG = 0x1, /*< this section data is from a SEI prefix */
  AL_SECTION_SYNC_FLAG = 0x2, /*< this section data is from an IDR */
  AL_SECTION_END_FRAME_FLAG = 0x4, /*< this section denotes the end of a frame */
  AL_SECTION_CONFIG_FLAG = 0x8, /*< section data is an sps, pps, vps, aud */
  AL_SECTION_FILLER_FLAG = 0x10, /*< section data contains filler data */
  AL_SECTION_APP_FILLER_FLAG = 0x20, /*< section data contains uninitialized filler data that should be filled by the application layer */
}AL_ESectionFlags;

/*************************************************************************//*!
   \brief Stream section. Act as a kind of scatter gather list containing the
   stream parts inside a buffer.
*****************************************************************************/
typedef struct
{
  uint32_t uOffset; /*!< Start offset of the section (in bytes from the beginning of the buffer) */
  uint32_t uLength; /*!< Length in bytes of the section */
  AL_ESectionFlags eFlags; /*!< Flags associated with the section; see macro AL_SECTION_xxxxx_FLAG */
}AL_TStreamSection;

/*@}*/

