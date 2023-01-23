/******************************************************************************
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

/*************************************************************************//*!
   \brief Video Mode
*****************************************************************************/
typedef enum AL_e_VideoMode
{
  AL_VM_PROGRESSIVE, /*!< Progressive */
  AL_VM_INTERLACED_TOP, /*!< interlaced top filed first */
  AL_VM_INTERLACED_BOTTOM, /*!< interlaced bottom field first */
  AL_VM_MAX_ENUM,
}AL_EVideoMode;

static inline bool AL_IS_INTERLACED(AL_EVideoMode eVideoMode)
{
  (void)eVideoMode;
  bool bIsInterlaced = false;
  bIsInterlaced = (eVideoMode == AL_VM_INTERLACED_TOP) || (eVideoMode == AL_VM_INTERLACED_BOTTOM);
  return bIsInterlaced;
}

/*************************************************************************//*!
   \brief Sequence Mode
*****************************************************************************/
typedef enum AL_e_SequenceMode
{
  AL_SM_UNKNOWN, /*!< unknown */
  AL_SM_PROGRESSIVE, /*!< progressive */
  AL_SM_INTERLACED, /*!< interlaced */
  AL_SM_MAX_ENUM,
}AL_ESequenceMode;

