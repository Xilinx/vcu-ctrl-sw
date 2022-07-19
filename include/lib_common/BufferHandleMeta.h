/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"

typedef struct AL_t_internalHandleMetaData AL_TInternalHandleMetaData;

typedef struct
{
  AL_TMetaData tMeta;
  AL_TInternalHandleMetaData* pInternal;
}AL_THandleMetaData;

/*************************************************************************//*!
   \brief Create a Handle metadata.
*****************************************************************************/
AL_THandleMetaData* AL_HandleMetaData_Create(int iMaxHandles, int iHandleSizeInBytes);
AL_THandleMetaData* AL_HandleMetaData_Clone(AL_THandleMetaData* pMeta);
bool AL_HandleMetaData_AddHandle(AL_THandleMetaData* pMeta, AL_HANDLE pHandle);
void AL_HandleMetaData_ResetHandles(AL_THandleMetaData* pMeta);
AL_HANDLE AL_HandleMetaData_GetHandle(AL_THandleMetaData* pMeta, int iNumHandle);
int AL_HandleMetaData_GetNumHandles(AL_THandleMetaData* pMeta);

/*@}*/

