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

/**************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/BufConst.h"
#include "lib_common/Error.h"
#include "lib_common_enc/EncChanParam.h"
#include "lib_common/BufferAPI.h"
#include "lib_common_enc/RateCtrlStats.h"

/*************************************************************************//*!
   \brief Segmentation structure
*****************************************************************************/
#define MAX_SEGMENTS 8
typedef struct AL_t_Segmentation
{
  bool enable;
  bool update_map;
  bool temporal_update;
  bool update_data;
  bool abs_delta;
  int16_t feature_data[MAX_SEGMENTS];  // only store data for Q
}AL_TSegmentation;

/*************************************************************************//*!
   \brief Encoding tool enum
*****************************************************************************/
typedef enum AL_e_PicEncOption
{
  AL_OPT_USE_QP_TABLE = 0x0001,
  AL_OPT_FORCE_LOAD = 0x0002,
  AL_OPT_USE_L2 = 0x0004,
  AL_OPT_DISABLE_INTRA = 0x0008,
  AL_OPT_DEPENDENT_SLICES = 0x0010,
} AL_EPicEncOption __AL_ALIGNED__ (4);

typedef struct AL_t_EncInfo
{
  AL_EPicEncOption eEncOptions;
  uint8_t uPpsId;
  int16_t iPpsQP;

  AL_TLookAheadParam tLAParam;

  AL_64U UserParam;
  AL_64U SrcHandle;

  int8_t iQp1Offset;
  int8_t iQp2Offset;
}AL_TEncInfo;

typedef enum
{
  AL_NO_OPT = 0,
  AL_OPT_SCENE_CHANGE = 0x00001,
  AL_OPT_IS_LONG_TERM = 0x00002,
  AL_OPT_USE_LONG_TERM = 0x00004,
  AL_OPT_RESTART_GOP = 0x00008,
  AL_OPT_UPDATE_RC_GOP_PARAMS = 0x00010,
  AL_OPT_UPDATE_COST_MODE = 0x01000,
  AL_OPT_SET_QP = 0x00100,
  AL_OPT_SET_INPUT_RESOLUTION = 0x00200,
  AL_OPT_SET_LF_OFFSETS = 0x00400,
  AL_OPT_SET_AUTO_QP = 0x08000,
  AL_OPT_UPDATE_AUTO_QP_VALUES = 0x20000,
  AL_OPT_RECOVERY_POINT = 0x10000,
}AL_ERequestEncOption;

typedef struct
{
  AL_TDimension tInputResolution;
}AL_TDynResParams;

typedef struct
{
  AL_TRCParam rc;
  AL_TGopParam gop;
  int16_t iQPSet;
  int8_t iLFBetaOffset;
  int8_t iLFTcOffset;
  bool costMode;
  bool useAutoQP;
}AL_TEncSmartParams;

typedef struct AL_t_EncRequestInfo
{
  AL_ERequestEncOption eReqOptions;
  uint32_t uSceneChangeDelay;
  AL_TEncSmartParams smartParams;
  AL_TDynResParams dynResParams;
}AL_TEncRequestInfo;

/*************************************************************************//*!
   \brief Stream partition structure
*****************************************************************************/
typedef struct AL_t_StreamPart
{
  uint32_t uOffset;
  uint32_t uSize;
}AL_TStreamPart;

/*************************************************************************//*!
   \brief Picture status structure
*****************************************************************************/
typedef struct AL_t_EncPicStatus
{
  AL_64U UserParam;
  AL_64U SrcHandle;

  bool bSkip;
  bool bIsRef;
  uint32_t uInitialRemovalDelay;
  uint32_t uDpbOutputDelay;
  uint32_t uSize;
  uint32_t uFrmTagSize;
  int32_t iStuffing;
  int32_t iFiller;
  uint16_t uNumClmn;
  uint16_t uNumRow;
  int16_t iQP;
  uint8_t uNumRefIdxL0;
  uint8_t uNumRefIdxL1;

  uint32_t uStreamPartOffset;
  int32_t iNumParts;

  uint32_t uSumCplx;

  int32_t iTileWidth[AL_ENC_NUM_CORES * 1];
  int32_t iTileHeight[AL_MAX_ROWS_TILE];

  AL_ERR eErrorCode;

  AL_ESliceType eType;
  AL_EPicStruct ePicStruct;
  bool bIsIDR;
  bool bIsFirstSlice;
  bool bIsLastSlice;
  int16_t iPpsQP;
  int iRecoveryCnt;
  uint8_t uTempId;
  int32_t iPOC;

  uint16_t uEncWidth;
  uint16_t uEncHeight;

  uint8_t uCuQpDeltaDepth;
  uint8_t bDisLoopFilter;
  int8_t iBetaOffset;
  int8_t iTcOffset;
  bool bTemporalMVP;

  int32_t iPictureSize;
  int8_t iPercentIntra[5];

  AL_RateCtrl_Statistics tRateCtrlStats;

  uint16_t uGdrPos;
  AL_EGdrMode eGdrMode;

}AL_TEncPicStatus;

/*@}*/

/*************************************************************************//*!
   \brief Picture buffers structure
*****************************************************************************/
typedef struct AL_t_SrcInfo
{
  uint8_t uBitDepth;
  uint32_t uPitch;
  uint8_t uFormat;
}AL_TSrcInfo;

typedef struct AL_t_SrcAddrs
{
  AL_PADDR pY;
  AL_PADDR pC1;
}AL_TSrcAddrs;

typedef struct AL_t_EncPicBufAddrs
{
  AL_TSrcAddrs tSrcAddrs;
  AL_TSrcInfo tSrcInfo;
  AL_PADDR pEP2;
  AL_PTR64 pEP2_v;
}AL_TEncPicBufAddrs;

