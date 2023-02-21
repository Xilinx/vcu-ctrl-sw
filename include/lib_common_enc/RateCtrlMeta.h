// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

