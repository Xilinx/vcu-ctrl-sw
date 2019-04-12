/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
   \addtogroup Encoder_Settings
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/VideoMode.h"
#include <assert.h>

/*************************************************************************//*!
   \brief Encoding parameters buffers info structure
*****************************************************************************/
typedef struct t_BufInfo
{
  uint32_t Flag;
  size_t Size;
  size_t Offset;
}AL_TBufInfo;

/*************************************************************************//*!
   \brief Lambda Control Mode
*****************************************************************************/
typedef enum e_LdaCtrlMode
{
  DEFAULT_LDA = 0x00, /*!< default behaviour */
  CUSTOM_LDA = 0x01, /*!< used for test purpose */
  DYNAMIC_LDA = 0x02, /*!< select lambda values according to the GOP pattern */
  AUTO_LDA = 0x03, /*!< automatically select betxeen DEFAULT_LDA and DYNAMIC_LDA */
  LOAD_LDA = 0x80, /*!< used for test purpose */
}AL_ELdaCtrlMode;

static inline bool AL_LdaIsSane(AL_ELdaCtrlMode lda)
{
  switch(lda)
  {
  case DEFAULT_LDA: // fallthrough
  case CUSTOM_LDA: // fallthrough
  case DYNAMIC_LDA: // fallthrough
  case AUTO_LDA: // fallthrough
  case LOAD_LDA: // fallthrough
    return true;
  default:
    return false;
  }
}

/*************************************************************************//*!
   \brief GDR Mode
*****************************************************************************/
typedef enum AL_e_GdrMode
{
  AL_GDR_OFF = 0x00,
  AL_GDR_ON = 0x02,
  AL_GDR_VERTICAL = AL_GDR_ON | 0x00,
  AL_GDR_HORIZONTAL = AL_GDR_ON | 0x01,
  AL_GDR_MAX_ENUM,
}AL_EGdrMode;


/*************************************************************************//*!
   \brief Picture format enum
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_PictFormat
{
  AL_400_8BITS = 0x0088,
  AL_420_8BITS = 0x0188,
  AL_422_8BITS = 0x0288,
  AL_444_8BITS = 0x0388,
  AL_400_10BITS = 0x00AA,
  AL_420_10BITS = 0x01AA,
  AL_422_10BITS = 0x02AA,
  AL_444_10BITS = 0x03AA,
} AL_EPicFormat;

#define AL_GET_BITDEPTH_LUMA(PicFmt) (uint8_t)((PicFmt) & 0x000F)
#define AL_GET_BITDEPTH_CHROMA(PicFmt) (uint8_t)(((PicFmt) >> 4) & 0x000F)
#define AL_GET_BITDEPTH(PicFmt) (AL_GET_BITDEPTH_LUMA(PicFmt) > AL_GET_BITDEPTH_CHROMA(PicFmt) ? AL_GET_BITDEPTH_LUMA(PicFmt) : AL_GET_BITDEPTH_CHROMA(PicFmt))
#define AL_GET_CHROMA_MODE(PicFmt) ((AL_EChromaMode)((PicFmt) >> 8))

#define AL_SET_BITDEPTH_LUMA(PicFmt, BitDepth) (PicFmt) = (AL_EPicFormat)(((PicFmt) & 0xFFF0) | (BitDepth))
#define AL_SET_BITDEPTH_CHROMA(PicFmt, BitDepth) (PicFmt) = (AL_EPicFormat)(((PicFmt) & 0xFF0F) | ((BitDepth) << 4))
#define AL_SET_BITDEPTH(PicFmt, BitDepth) (PicFmt) = (AL_EPicFormat)(((PicFmt) & 0xFF00) | (BitDepth) | ((BitDepth) << 4))
#define AL_SET_CHROMA_MODE(PicFmt, Mode) (PicFmt) = (AL_EPicFormat)(((PicFmt) & 0x00FF) | ((int)(Mode) << 8))

/*************************************************************************//*!
   \brief Encoding High level syntax enum
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_HlsFlag
{
  AL_SPS_LOG2_MAX_POC_MASK = 0x0000000F,
  AL_SPS_LOG2_MAX_FRAME_NUM_MASK = 0x000000F0,
  AL_SPS_LOG2_NUM_SHORT_TERM_RPS_MASK = 0x00003F00,
  AL_SPS_LOG2_NUM_LONG_TERM_RPS_MASK = 0x000FC000,
  AL_SPS_TEMPORAL_MVP_EN_FLAG = 0x00100000,

  AL_PPS_ENABLE_REORDERING = 0x00000001,
  AL_PPS_CABAC_INIT_PRES_FLAG = 0x00000002,
  AL_PPS_DBF_OVR_EN_FLAG = 0x00000004,
  AL_PPS_SLICE_SEG_EN_FLAG = 0x00000008,
  AL_PPS_NUM_ACT_REF_L0 = 0x000000F0,
  AL_PPS_NUM_ACT_REF_L1 = 0x00000F00,
  AL_PPS_OVERRIDE_LF = 0x00001000,
  AL_PPS_DISABLE_LF = 0x00002000,
} AL_EHlsFlag;

static inline uint32_t AL_GET_SPS_LOG2_MAX_POC(uint32_t uHlsParam)
{
  return (uHlsParam & AL_SPS_LOG2_MAX_POC_MASK) + 1;
}

static inline void AL_SET_SPS_LOG2_MAX_POC(uint32_t* pHlsParam, int iLog2MaxPoc)
{
  assert(pHlsParam);
  assert(iLog2MaxPoc <= 16);
  *pHlsParam = ((*pHlsParam & ~AL_SPS_LOG2_MAX_POC_MASK) | (iLog2MaxPoc - 1));
}

static inline uint32_t AL_GET_SPS_LOG2_MAX_FRAME_NUM(uint32_t uHlsParam)
{
  return (uHlsParam & AL_SPS_LOG2_MAX_FRAME_NUM_MASK) >> 4;
}

static inline void AL_SET_SPS_LOG2_MAX_FRAME_NUM(uint32_t* pHlsParam, int iLog2MaxFrameNum)
{
  assert(pHlsParam);
  assert(iLog2MaxFrameNum < 0xF);
  *pHlsParam = ((*pHlsParam & ~AL_SPS_LOG2_MAX_FRAME_NUM_MASK) | (iLog2MaxFrameNum << 4));
}

static inline uint32_t AL_GET_SPS_LOG2_NUM_SHORT_TERM_RPS(uint32_t uHlsParam)
{
  return (uHlsParam & AL_SPS_LOG2_NUM_SHORT_TERM_RPS_MASK) >> 8;
}

static inline void AL_SET_SPS_LOG2_NUM_SHORT_TERM_RPS(uint32_t* pHlsParam, int iLog2NumShortTermRps)
{
  assert(pHlsParam);
  assert(iLog2NumShortTermRps < 0x3F);
  *pHlsParam = ((*pHlsParam & ~AL_SPS_LOG2_NUM_SHORT_TERM_RPS_MASK) | (iLog2NumShortTermRps << 8));
}

static inline uint32_t AL_GET_SPS_LOG2_NUM_LONG_TERM_RPS(uint32_t uHlsParam)
{
  return (uHlsParam & AL_SPS_LOG2_NUM_LONG_TERM_RPS_MASK) >> 14;
}

static inline void AL_SET_SPS_LOG2_NUM_LONG_TERM_RPS(uint32_t* pHlsParam, int iLog2NumLongTermRps)
{
  assert(pHlsParam);
  assert(iLog2NumLongTermRps < 0xFC);
  *pHlsParam = ((*pHlsParam & ~AL_SPS_LOG2_NUM_LONG_TERM_RPS_MASK) | (iLog2NumLongTermRps << 14));
}

#define AL_GET_SPS_TEMPORAL_MVP_EN_FLAG(HlsParam) (((HlsParam) & AL_SPS_TEMPORAL_MVP_EN_FLAG) >> 20)

#define AL_GET_PPS_ENABLE_REORDERING(HlsParam) ((HlsParam) & AL_PPS_ENABLE_REORDERING)
#define AL_GET_PPS_CABAC_INIT_PRES_FLAG(HlsParam) (((HlsParam) & AL_PPS_CABAC_INIT_PRES_FLAG) >> 1)
#define AL_GET_PPS_DBF_OVR_EN_FLAG(HlsParam) (((HlsParam) & AL_PPS_DBF_OVR_EN_FLAG) >> 2)
#define AL_GET_PPS_SLICE_SEG_EN_FLAG(HlsParam) (((HlsParam) & AL_PPS_SLICE_SEG_EN_FLAG) >> 3)
#define AL_GET_PPS_OVERRIDE_LF(HlsParam) (((HlsParam) & AL_PPS_OVERRIDE_LF) >> 12)
#define AL_GET_PPS_DISABLE_LF(HlsParam) (((HlsParam) & AL_PPS_DISABLE_LF) >> 13)

static inline uint32_t AL_GET_PPS_NUM_ACT_REF_L0(uint32_t HlsParam)
{
  uint32_t uNumRefL0Minus1 = (HlsParam & AL_PPS_NUM_ACT_REF_L0) >> 4;

  if(uNumRefL0Minus1 > 0)
    --uNumRefL0Minus1;
  return uNumRefL0Minus1;
}

static inline uint32_t AL_GET_PPS_NUM_ACT_REF_L1(uint32_t HlsParam)
{
  uint32_t uNumRefL1Minus1 = (HlsParam & AL_PPS_NUM_ACT_REF_L1) >> 8;

  if(uNumRefL1Minus1 > 0)
    --uNumRefL1Minus1;
  return uNumRefL1Minus1;
}

static inline uint32_t AL_GetNumberOfRef(uint32_t HlsParam)
{
  /* this takes advantage of the fact that the number of L0 ref is always
   * bigger than the number of L1 refs */
  return (HlsParam & AL_PPS_NUM_ACT_REF_L0) >> 4;
}

#define AL_SET_PPS_NUM_ACT_REF_L0(HlsParam, Num) (HlsParam) = ((HlsParam) & ~AL_PPS_NUM_ACT_REF_L0) | ((Num) << 4)
#define AL_SET_PPS_NUM_ACT_REF_L1(HlsParam, Num) (HlsParam) = ((HlsParam) & ~AL_PPS_NUM_ACT_REF_L1) | ((Num) << 8)

/*************************************************************************//*!
   \brief Encoding option enum
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_ChEncOptions
{
  AL_OPT_QP_TAB_RELATIVE = 0x00000001,
  AL_OPT_FIX_PREDICTOR = 0x00000002,
  AL_OPT_CUSTOM_LDA = 0x00000004,
  AL_OPT_ENABLE_AUTO_QP = 0x00000008,
  AL_OPT_ADAPT_AUTO_QP = 0x00000010,
  AL_OPT_FORCE_REC = 0x00000040,
  AL_OPT_FORCE_MV_OUT = 0x00000080,
  AL_OPT_LOWLAT_SYNC = 0x00000100,
  AL_OPT_LOWLAT_INT = 0x00000200,
  AL_OPT_HIGH_FREQ = 0x00002000,
  AL_OPT_SCENE_CHANGE_DETECTION = 0x00004000,
  AL_OPT_FORCE_MV_CLIP = 0x00020000,
  AL_OPT_RDO_COST_MODE = 0x00040000,
} AL_EChEncOption;

/*************************************************************************//*!
   \brief Encoding tools enum
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_ChEncTools
{
  AL_OPT_WPP = 0x00000001,
  AL_OPT_TILE = 0x00000002,
  AL_OPT_LF = 0x00000004,
  AL_OPT_LF_X_SLICE = 0x00000008,
  AL_OPT_LF_X_TILE = 0x00000010,
  AL_OPT_SCL_LST = 0x00000020,
  AL_OPT_CONST_INTRA_PRED = 0x00000040,
  AL_OPT_TRANSFO_SKIP = 0x00000080,
} AL_EChEncTool;

/*************************************************************************//*!
   \brief Rate Control Mode
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_RateCtrlMode
{
  AL_RC_CONST_QP = 0x00,
  AL_RC_CBR = 0x01,
  AL_RC_VBR = 0x02,
  AL_RC_LOW_LATENCY = 0x03,
  AL_RC_CAPPED_VBR = 0x04,
  AL_RC_BYPASS = 0x3F,
  AL_RC_PLUGIN = 0x40,
  AL_TEST_RC_FILLER = 0xF1,
  AL_RC_MAX_ENUM,
} AL_ERateCtrlMode;

/*************************************************************************//*!
   \brief Rate Control Options
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_RateCtrlOption
{
  AL_RC_OPT_NONE = 0x00000000,
  AL_RC_OPT_SCN_CHG_RES = 0x00000001,
  AL_RC_OPT_DELAYED = 0x00000002,
  AL_RC_OPT_STATIC_SCENE = 0x00000004,
  AL_RC_OPT_ENABLE_SKIP = 0x00000008,
  AL_RC_OPT_MAX_ENUM,
} AL_ERateCtrlOption;

/*************************************************************************//*!
   \brief Rate Control parameters structure
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct __AL_ALIGNED__ (4) AL_t_RCParam
{
  AL_ERateCtrlMode eRCMode;
  uint32_t uInitialRemDelay;
  uint32_t uCPBSize;
  uint16_t uFrameRate;
  uint16_t uClkRatio;
  uint32_t uTargetBitRate;
  uint32_t uMaxBitRate;
  int16_t iInitialQP;
  int16_t iMinQP;
  int16_t iMaxQP;
  int16_t uIPDelta;
  int16_t uPBDelta;
  bool bUseGoldenRef;
  int16_t uPGoldenDelta;
  int16_t uGoldenRefFrequency;
  AL_ERateCtrlOption eOptions;
  uint32_t uNumPel;
  uint16_t uMaxPSNR;
  uint16_t uMaxPelVal;
  uint32_t uMaxPictureSize;
} AL_TRCParam;

#define AL_IS_HWRC_ENABLED(tRCParam) (((tRCParam).eRCMode == AL_RC_LOW_LATENCY) || (tRCParam).uMaxPictureSize)

#define MAX_LOW_DELAY_B_GOP_LENGTH 4
#define MIN_LOW_DELAY_B_GOP_LENGTH 1
/*************************************************************************//*!
   \brief GOP Control Mode
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_GopCtrlMode
{
  AL_GOP_MODE_DEFAULT = 0x00,
  AL_GOP_MODE_PYRAMIDAL = 0x01,
  AL_GOP_MODE_ADAPTIVE = 0x02,

  AL_GOP_MODE_BYPASS = 0x7F,

  AL_GOP_FLAG_LOW_DELAY = 0x80,
  AL_GOP_MODE_LOW_DELAY_P = AL_GOP_FLAG_LOW_DELAY | 0x00,
  AL_GOP_MODE_LOW_DELAY_B = AL_GOP_FLAG_LOW_DELAY | 0x01,
  AL_GOP_MODE_MAX_ENUM,
} AL_EGopCtrlMode;

/*************************************************************************//*!
   \brief Gop parameters structure
*****************************************************************************/
typedef struct AL_t_GopFrm
{
  uint8_t uType;
  uint8_t uTempId;
  uint8_t uIsRef;
  int16_t iPOC;
  int16_t iRefA;
  int16_t iRefB;
  uint8_t uSrcOrder; // filled by Gop Manager
  int8_t pDPB[5];
}AL_TGopFrm;

/*************************************************************************//*!
   \brief Gop parameters structure
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct AL_t_GopParam
{
  AL_EGopCtrlMode eMode;
  uint16_t uGopLength;
  uint8_t uNumB;
  uint8_t uFreqGoldenRef;
  uint32_t uFreqIDR;
  bool bEnableLT;
  uint32_t uFreqLT;
  AL_EGdrMode eGdrMode;
  int8_t tempDQP[4];
}AL_TGopParam;

/*************************************************************************//*!
   \brief First Pass infos parameters
*****************************************************************************/
typedef struct AL_t_LookAheadParam
{
  int32_t iSCPictureSize;
  int32_t iSCIPRatio;
  int16_t iComplexity;
  int16_t iTargetLevel;
}AL_TLookAheadParam;

/*************************************************************************//*!
   \brief Max burst size
*****************************************************************************/
typedef enum e_MaxBurstSize
{
  AL_BURST_256 = 0,
  AL_BURST_128 = 1,
  AL_BURST_64 = 2,
  AL_BURST_512 = 3,
}AL_EMaxBurstSize;

/*************************************************************************//*!
   \brief Source compression type
*****************************************************************************/
typedef enum e_SrcConvMode // [0] : CompMode | [3:1] : SourceFormat
{
  AL_SRC_NVX = 0x0,
  AL_SRC_TILE_64x4 = 0x4,
  AL_SRC_COMP_64x4 = 0x5,
  AL_SRC_TILE_32x4 = 0x6,
  AL_SRC_COMP_32x4 = 0x7,
  AL_SRC_MAX_ENUM,
}AL_ESrcMode;

#define MASK_SRC_COMP 0x01
#define MASK_SRC_FMT 0x0E

#define AL_GET_COMP_MODE(SrcConvFmt) ((SrcConvFmt) & MASK_SRC_COMP)
#define AL_GET_SRC_FMT(SrcConvFmt) (((SrcConvFmt) & MASK_SRC_FMT) >> 1)

#define AL_SET_COMP_MODE(SrcConvFmt, CompMode) (SrcConvFmt) = ((SrcConvFmt) & ~MASK_SRC_COMP) | ((CompMode) & MASK_SRC_COMP)
#define AL_SET_SRC_FMT(SrcConvFmt, SrcFmt) (SrcConvFmt) = ((SrcConvFmt) & ~MASK_SRC_FMT) | (((SrcFmt) << 1) & MASK_SRC_FMT)

#define AL_IS_L2P_DISABLED(iPrefetchLevel2) (iPrefetchLevel2 == 0)


/*************************************************************************//*!
   \brief AOM interpolation filter
*****************************************************************************/
typedef enum e_InterP_Filter
{
  AL_INTERP_REGULAR,
  AL_INTERP_SMOOTH,
  AL_INTERP_SHARP,
  AL_INTERP_BILINEAR,
  AL_INTERP_SWITCHABLE,
  AL_INTERP_MAX_ENUM, /* sentinel */
}AL_EInterPFilter;

/*************************************************************************//*!
   \brief Channel parameters structure
*****************************************************************************/
typedef AL_INTROSPECT (category = "debug") struct __AL_ALIGNED__ (4) AL_t_EncChanParam
{
  int iLayerID;

  /* Encoding resolution */
  uint16_t uWidth;
  uint16_t uHeight;


  AL_EVideoMode eVideoMode;
  /* Encoding picture format */
  AL_EPicFormat ePicFormat;
  AL_ESrcMode eSrcMode;
  /* Input picture bitdepth */
  uint8_t uSrcBitDepth;

  /* encoding profile/level */
  AL_EProfile eProfile;
  uint8_t uLevel;
  uint8_t uTier;

  uint32_t uSpsParam;
  uint32_t uPpsParam;

  /* Encoding tools parameters */
  AL_EChEncOption eEncOptions;
  AL_EChEncTool eEncTools;
  int8_t iBetaOffset;
  int8_t iTcOffset;

  int8_t iCbSliceQpOffset;
  int8_t iCrSliceQpOffset;
  int8_t iCbPicQpOffset;
  int8_t iCrPicQpOffset;


  uint8_t uCuQPDeltaDepth;
  uint8_t uCabacInitIdc;

  uint8_t uNumCore;
  uint16_t uSliceSize;
  uint16_t uNumSlices;

  /* L2 prefetch parameters */
  uint32_t uL2PrefetchMemOffset;
  uint32_t uL2PrefetchMemSize;
  uint16_t uClipHrzRange;
  uint16_t uClipVrtRange;


  /* MV range */
  int16_t pMeRange[2][2];  /*!< Allowed range for motion estimation */

  /* encoding block size */
  uint8_t uMaxCuSize;
  uint8_t uMinCuSize;
  uint8_t uMaxTuSize;
  uint8_t uMinTuSize;
  uint8_t uMaxTransfoDepthIntra;
  uint8_t uMaxTransfoDepthInter;

  // For AVC
  AL_EEntropyMode eEntropyMode;
  AL_EWPMode eWPMode;






  /* Gop & Rate control parameters */
  AL_TRCParam tRCParam;
  AL_TGopParam tGopParam;
  bool bSubframeLatency;
  AL_ELdaCtrlMode eLdaCtrlMode;
  int LdaFactors[6];

} AL_TEncChanParam;

/***************************************************************************/
#define ROUND_POWER_OF_TWO(value, n) (((value) + (1 << ((n) - 1))) >> (n))
#define ROUND_UP_POWER_OF_TWO(value, n) (((value) + (1 << (n)) - 1) >> (n))
#define AL_GetWidthInLCU(tChParam) (ROUND_UP_POWER_OF_TWO((tChParam).uWidth, (tChParam).uMaxCuSize))
#define AL_GetHeightInLCU(tChParam) (ROUND_UP_POWER_OF_TWO((tChParam).uHeight, (tChParam).uMaxCuSize))

#define AL_ENTCOMP(tChParam) false

#define AL_GetSrcWidth(tChParam) ((tChParam).uWidth)
#define AL_GetSrcHeight(tChParam) ((tChParam).uHeight)
#define AL_SetSrcWidth(tChParam, _uSrcWidth) ((tChParam)->uWidth = (_uSrcWidth))
#define AL_SetSrcHeight(tChParam, _uSrcHeight) ((tChParam)->uHeight = (_uSrcHeight))

/*@}*/

