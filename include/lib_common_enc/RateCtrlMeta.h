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

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common_enc/RateCtrlStats.h"

/*************************************************************************//*!
   \brief MetaData gathering encode-statistics useful for rate-control
   algorithms
*****************************************************************************/
typedef struct AL_t_RateCtrlMetaData
{
  AL_TMetaData tMeta;
  bool bFilled;
  AL_RateCtrl_Statistics tRateCtrlStats;
  AL_TBuffer* pMVBuf;
}AL_TRateCtrlMetaData;

/*************************************************************************//*!
   \brief Create a RateCtrl metadata.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create(AL_TAllocator* pAllocator, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec);

/*************************************************************************//*!
   \brief Create a RateCtrl metadata with buffer for motion vector. Caller must
   be sure pMVBuf size is correct.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create_WithBuffer(AL_TBuffer* pMVBuf);

/*@}*/

