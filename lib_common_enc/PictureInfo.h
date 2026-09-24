// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_buf_mngt/FrmBuf.h"
#include "lib_common/Utils.h"
#include "lib_common_enc/EncChanParam.h"
#include "lib_common_enc/EncPicInfo.h"

/*****************************************************************************/
static const uint8_t PicStructToFieldNumber[] =
{
  2,
  1, 1, 2, 2, 3, 3,
  4, 6,
  1, 1, 1, 1,
};

/****************************************************************************/
static const uint32_t AL_PICT_INFO_IS_IDR = 0x00000001; /*!< The picture is an IDR */
static const uint32_t AL_PICT_INFO_IS_REF = 0x00000002; /*!< The picture is a reference */
static const uint32_t AL_PICT_INFO_SCN_CHG = 0x00000004; /*!< The picture is a scene change */
static const uint32_t AL_PICT_INFO_BEG_FRM = 0x00000008; /* internal */
static const uint32_t AL_PICT_INFO_END_FRM = 0x00000010; /* internal */
static const uint32_t AL_PICT_INFO_END_SRC = 0x00000020; /* internal */
static const uint32_t AL_PICT_INFO_USE_LT = 0x00000040; /*!< The picture uses a long term reference */
static const uint32_t AL_PICT_INFO_IS_GOLDREF = 0x0000080; /* internal */

static const uint32_t AL_PICT_INFO_NOT_SHOWABLE = 0x80000000; /*!< The picture isn't showable (VP9 / AV1) */

#define AL_IS_IDR(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_IS_IDR)
#define AL_IS_REF(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_IS_REF)
#define AL_SCN_CHG(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_SCN_CHG)
#define AL_END_SRC(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_END_SRC)
#define AL_USE_LT(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_USE_LT)
#define AL_IS_GOLDREF(PicInfo) ((PicInfo).uFlags & AL_PICT_INFO_IS_GOLDREF)
#define AL_IS_SHOWABLE(PicInfo) (!((PicInfo).uFlags & AL_PICT_INFO_NOT_SHOWABLE))

typedef struct
{
  uint16_t uGdrPos; /*!< Gradual Refresh position */
  AL_EGdrMode eGdrMode; /*!< Gradual Refresh Mode */
}AL_TPicInfoGdr;

/*****************************************************************************
   \brief Picture information
*****************************************************************************/
typedef struct AL_TPictureInfo
{
  AL_64U uSrcOrder; /*!< Source picture number in display order */
  uint32_t uFlags; /*!< Bitfield containing information about this picture (For example AL_PICT_INFO_IS_REF or AL_PICT_INFO_IS_IDR) \see include/lib_common_enc/PictureInfo.h for the full list */
  int32_t iPOC; /*!< Picture Order Count */
  int32_t iFrameNum; /*!< H264 frame_num field */
  AL_ESliceType eType; /*!< The type of the current slice (I, P, B, ...) */
  AL_EPicStruct ePicStruct; /*!< Identifies the pic_struct field */
  AL_EMarkingRef eMarking; /*!< Reference picture status */

  /*
    Hierarchical-Level: describes the level of the picture in the interprediction dependency
    hierarchy. It is used by the encoder-library to characterize the type/importance of pictures,
    but is not encoded in the bitstream. See Pyramidal-GOP mode for typical usage.
  */
  uint8_t uHierarchicalLevel;
  /*
    Temporal ID: it is similar to the Hierarchical-Level, but it is this time encoded in the bitstream.
    It allows a decoder to decode only a subset of the frames, as access-units with temporal-id N are
    guaranteed not to depend on access-units with temporal-id (N+1). Often, it is identical to the
    Hierarchical-Level. In some cases though, we cannot follow the Hierarchical-Level because of ITU
    specification constraints (ex.: interlaced).
  */
  uint8_t uTemporalId;

  bool bForceLT[2]; /*!< Specifies if a following reference picture need to be marked as long-term */

  int32_t iDpbOutputDelay;
  int8_t iRefPicSetIdx;
  int8_t iGopMngrQpOffset;
  int32_t iRecoveryCnt;
  int32_t iRefAQp;
  int32_t iRefBQp;

  bool bForceQp;
  int16_t iQpSet;
  bool bRateCtrlQpOffset;
  int8_t iRateCtrlQpOffset;

  AL_TLookAheadParam tLAParam;

  AL_TPicInfoGdr tGdrInfos;
}AL_TPictureInfo;
