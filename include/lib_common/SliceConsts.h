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

/**************************************************************************//*!
   \addtogroup SliceConsts
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/****************************************************************************/
#define AL_CS_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)
#define AL_RExt_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)

#define AVC_PROFILE_IDC_CAVLC_444 44 // not supported
#define AVC_PROFILE_IDC_BASELINE 66
#define AVC_PROFILE_IDC_MAIN 77
#define AVC_PROFILE_IDC_EXTENDED 88 // not supported
#define AVC_PROFILE_IDC_HIGH 100
#define AVC_PROFILE_IDC_HIGH10 110
#define AVC_PROFILE_IDC_HIGH_422 122
#define AVC_PROFILE_IDC_HIGH_444_PRED 244 // not supported

#define HEVC_PROFILE_IDC_MAIN 1
#define HEVC_PROFILE_IDC_MAIN10 2
#define HEVC_PROFILE_IDC_MAIN_STILL 3
#define HEVC_PROFILE_IDC_RExt 4


/*************************************************************************//*!
   \brief Profiles identifier
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_Profile
{
  AL_PROFILE_AVC = 0x01000000,
  AL_PROFILE_AVC_CAVLC_444 = AL_PROFILE_AVC | AVC_PROFILE_IDC_CAVLC_444, // not supported
  AL_PROFILE_AVC_BASELINE = AL_PROFILE_AVC | AVC_PROFILE_IDC_BASELINE,
  AL_PROFILE_AVC_MAIN = AL_PROFILE_AVC | AVC_PROFILE_IDC_MAIN,
  AL_PROFILE_AVC_EXTENDED = AL_PROFILE_AVC | AVC_PROFILE_IDC_EXTENDED, // not supported
  AL_PROFILE_AVC_HIGH = AL_PROFILE_AVC | AVC_PROFILE_IDC_HIGH,
  AL_PROFILE_AVC_HIGH10 = AL_PROFILE_AVC | AVC_PROFILE_IDC_HIGH10,
  AL_PROFILE_AVC_HIGH_422 = AL_PROFILE_AVC | AVC_PROFILE_IDC_HIGH_422,
  AL_PROFILE_AVC_HIGH_444_PRED = AL_PROFILE_AVC | AVC_PROFILE_IDC_HIGH_444_PRED, // not supported

  AL_PROFILE_AVC_C_BASELINE = AL_PROFILE_AVC_BASELINE | AL_CS_FLAGS(0x0002),
  AL_PROFILE_AVC_PROG_HIGH = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x0010),
  AL_PROFILE_AVC_C_HIGH = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x0030),
  AL_PROFILE_AVC_HIGH10_INTRA = AL_PROFILE_AVC_HIGH10 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_422_INTRA = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_444_INTRA = AL_PROFILE_AVC_HIGH_444_PRED | AL_CS_FLAGS(0x0008), // not supported

  AL_PROFILE_HEVC = 0x02000000,
  AL_PROFILE_HEVC_MAIN = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN,
  AL_PROFILE_HEVC_MAIN10 = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN10,
  AL_PROFILE_HEVC_MAIN_STILL = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN_STILL,
  AL_PROFILE_HEVC_RExt = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_RExt,

  AL_PROFILE_HEVC_MONO = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xFC80),
  AL_PROFILE_HEVC_MONO10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xDC80),
  AL_PROFILE_HEVC_MONO12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9C80), // not supported
  AL_PROFILE_HEVC_MONO16 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x1C80), // not supported
  AL_PROFILE_HEVC_MAIN12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9880), // not supported
  AL_PROFILE_HEVC_MAIN_422 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xF080),
  AL_PROFILE_HEVC_MAIN_422_10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xD080),
  AL_PROFILE_HEVC_MAIN_422_12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9080), // not supported
  AL_PROFILE_HEVC_MAIN_444 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE080), // not supported
  AL_PROFILE_HEVC_MAIN_444_10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xC080), // not supported
  AL_PROFILE_HEVC_MAIN_444_12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x8080), // not supported
  AL_PROFILE_HEVC_MAIN_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xFA00),
  AL_PROFILE_HEVC_MAIN10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xDA00),
  AL_PROFILE_HEVC_MAIN12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9A00), // not supported
  AL_PROFILE_HEVC_MAIN_422_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xF200),
  AL_PROFILE_HEVC_MAIN_422_10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xD200),
  AL_PROFILE_HEVC_MAIN_422_12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9200), // not supported
  AL_PROFILE_HEVC_MAIN_444_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE200), // not supported
  AL_PROFILE_HEVC_MAIN_444_10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xC200), // not supported
  AL_PROFILE_HEVC_MAIN_444_12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x8200), // not supported
  AL_PROFILE_HEVC_MAIN_444_16_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x0200), // not supported
  AL_PROFILE_HEVC_MAIN_444_STILL = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE300), // not supported
  AL_PROFILE_HEVC_MAIN_444_16_STILL = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x0300), // not supported

} AL_EProfile;

/****************************************************************************/
#define AL_GET_PROFILE_CODEC(Prof) (Prof & 0xFF000000)
#define AL_GET_PROFILE_IDC(Prof) (Prof & 0x000000FF)
#define AL_GET_PROFILE_CODED_AND_IDC(Prof) (Prof & 0xFF0000FF)
#define AL_GET_RExt_FLAGS(Prof) ((Prof & 0x00FFFF00) >> 8)
#define AL_GET_CS_FLAGS(Prof) ((Prof & 0x00FFFF00) >> 8)
/****************************************************************************/
#define AL_IS_AVC(Prof) (AL_GET_PROFILE_CODEC(Prof) == AL_PROFILE_AVC)
#define AL_IS_HEVC(Prof) (AL_GET_PROFILE_CODEC(Prof) == AL_PROFILE_HEVC)

/****************************************************************************/
static AL_INLINE bool AL_IS_MONO_PROFILE(AL_EProfile eProf)
{
  bool bRes = ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt)
               || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH)
               || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH10)
               || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH_422)
               );
  return bRes;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_10BIT_PROFILE(AL_EProfile eProf)
{
  bool bRes = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProf) & 0x2000))
    || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_MAIN10)
    || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH10)
    || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH_422)
    );
  return bRes;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_420_PROFILE(AL_EProfile eProf)
{
  /* Only hevc mono doesn't support 420 */
  bool bIsHEVCMono = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProf) & 0x0400))
    );

  return !bIsHEVCMono;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_422_PROFILE(AL_EProfile eProf)
{
  bool bRes = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProf) & 0x0C00))
    || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_AVC_HIGH_422)
    );

  return bRes;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_STILL_PROFILE(AL_EProfile eProf)
{
  bool bRes = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProf) & 0x0100))
    || (AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_MAIN_STILL)
    );
  return bRes;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_INTRA_PROFILE(AL_EProfile eProf)
{
  bool bRes = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProf) & 0x0200))
    || (AL_IS_AVC(eProf) && (AL_GET_CS_FLAGS(eProf) & 0x0008))
    );
  return bRes;
}

/****************************************************************************/
static AL_INLINE bool AL_IS_LOW_BITRATE_PROFILE(AL_EProfile eProf)
{
  bool bRes = (
    ((AL_GET_PROFILE_CODED_AND_IDC(eProf) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProf) & 0x0080))
    );
  return bRes;
}

/****************************************************************************/
typedef enum AL_e_Codec
{
  AL_CODEC_AVC,
  AL_CODEC_HEVC,
  AL_CODEC_JPEG,
  AL_CODEC_VP9,
  AL_CODEC_AV1,
  AL_CODEC_INVALID, /* sentinel */
}AL_ECodec;

/*************************************************************************//*!
   \brief Scaling List identifier
*****************************************************************************/
typedef enum e_ScalingList
{
  AL_SCL_FLAT,
  AL_SCL_DEFAULT,
  AL_SCL_CUSTOM,
  AL_SCL_RANDOM,
  AL_SCL_MAX_ENUM, /* sentinel */
}AL_EScalingList;

/*************************************************************************//*!
   \brief Reference picture status
 ***************************************************************************/
typedef enum e_MarkingRef
{
  SHORT_TERM_REF = 0,
  LONG_TERM_REF = 1,
  UNUSED_FOR_REF = 2,
  NON_EXISTING_REF = 3,
}AL_EMarkingRef;

/*************************************************************************//*!
   \brief Identifies the slice coding type
*****************************************************************************/
typedef enum e_SliceType
{
  SLICE_SI = 4, /*!< AVC SI Slice */
  SLICE_SP = 3, /*!< AVC SP Slice */
  SLICE_GOLDEN = 3, /*!< Golden Slice */
  SLICE_I = 2,  /*!< I Slice (can contain I blocks) */
  SLICE_P = 1,  /*!< P Slice (can contain I and P blocks) */
  SLICE_B = 0,  /*!< B Slice (can contain I, P and B blocks) */
  SLICE_CONCEAL = 6, /*!< Conceal Slice (slice was concealed) */
  SLICE_SKIP = 7, /*!< Skip Slice */
  SLICE_REPEAT = 8, /*!< VP9 Repeat Slice (repeats the content of its reference) */
  SLICE_MAX_ENUM, /* sentinel */
}AL_ESliceType;

/*************************************************************************//*!
   \brief Indentifies pic_struct (subset of table D-2)
*****************************************************************************/
typedef enum e_PicStruct
{
  PS_FRM = 0,
  PS_TOP_FLD = 1,
  PS_BOT_FLD = 2,
  PS_TOP_BOT = 3,
  PS_BOT_TOP = 4,
  PS_TOP_BOT_TOP = 5,
  PS_BOT_TOP_BOT = 6,
  PS_FRM_x2 = 7,
  PS_FRM_x3 = 8,
  PS_TOP_FLD_WITH_PREV_BOT = 9,
  PS_BOT_FLD_WITH_PREV_TOP = 10,
  PS_TOP_FLD_WITH_NEXT_BOT = 11,
  PS_BOT_FLD_WITH_NEXT_TOP = 12,
  /* should always be last */
  PS_FRM_MAX_ENUM, /* sentinel */
}AL_EPicStruct;

/*************************************************************************//*!
   \brief chroma_format_idc
*****************************************************************************/
typedef enum e_ChromaMode
{
  CHROMA_MONO = 0, /*!< Monochrome */
  CHROMA_4_0_0 = 0, /*!< 4:0:0 = Monochrome */
  CHROMA_4_2_0 = 1, /*!< 4:2:0 chroma sampling */
  CHROMA_4_2_2 = 2, /*!< 4:2:2 chroma sampling : Not supported */
  CHROMA_4_4_4 = 3, /*!< 4:4:4 chroma sampling : Not supported */
  CHROMA_MAX_ENUM, /* sentinel */
}AL_EChromaMode;

/*************************************************************************//*!
   \brief identifies the entropy coding method
*****************************************************************************/
typedef enum e_EntropyMode
{
  AL_MODE_CAVLC,
  AL_MODE_CABAC,
  AL_MODE_MAX_ENUM, /* sentinel */
}AL_EEntropyMode;

/*************************************************************************//*!
   \brief identifies the deblocking filter mode
*****************************************************************************/
typedef enum e_FilterMode
{
  FILT_ENABLE,
  FILT_DISABLE,
  FILT_DIS_SLICE,
}AL_EFilterMode;

/*************************************************************************//*!
   \brief Weighted Pred Mode
*****************************************************************************/
typedef enum e_WPMode
{
  AL_WP_DEFAULT,
  AL_WP_EXPLICIT,
  AL_WP_IMPLICIT,
}AL_EWPMode;

/*************************************************************************//*!
   \brief Nal unit type enum
 *************************************************************************/
typedef enum e_NalUnitType
{
  AL_AVC_UNDEF_NUT = 0,
  AL_AVC_NUT_VCL_NON_IDR = 1,
  AL_AVC_NUT_VCL_IDR = 5,
  AL_AVC_NUT_PREFIX_SEI = 6,
  AL_AVC_NUT_SPS = 7,
  AL_AVC_NUT_PPS = 8,
  AL_AVC_NUT_AUD = 9,
  AL_AVC_NUT_EOS = 10,
  AL_AVC_NUT_EOB = 11,
  AL_AVC_NUT_FD = 12,
  AL_AVC_NUT_SPS_EXT = 13,
  AL_AVC_NUT_SUB_SPS = 15,
  AL_AVC_NUT_SUFFIX_SEI = 24, /* nal_unit_type : [24..31] -> Content Unspecified */
  AL_AVC_NUT_ERR = 32,
  AL_HEVC_NUT_TRAIL_N = 0,
  AL_HEVC_NUT_TRAIL_R = 1,
  AL_HEVC_NUT_TSA_N = 2,
  AL_HEVC_NUT_TSA_R = 3,
  AL_HEVC_NUT_STSA_N = 4,
  AL_HEVC_NUT_STSA_R = 5,
  AL_HEVC_NUT_RADL_N = 6,
  AL_HEVC_NUT_RADL_R = 7,
  AL_HEVC_NUT_RASL_N = 8,
  AL_HEVC_NUT_RASL_R = 9,
  AL_HEVC_NUT_RSV_VCL_N10 = 10,
  AL_HEVC_NUT_RSV_VCL_R11 = 11,
  AL_HEVC_NUT_RSV_VCL_N12 = 12,
  AL_HEVC_NUT_RSV_VCL_R13 = 13,
  AL_HEVC_NUT_RSV_VCL_N14 = 14,
  AL_HEVC_NUT_RSV_VCL_R15 = 15,
  AL_HEVC_NUT_BLA_W_LP = 16,
  AL_HEVC_NUT_BLA_W_RADL = 17,
  AL_HEVC_NUT_BLA_N_LP = 18,
  AL_HEVC_NUT_IDR_W_RADL = 19,
  AL_HEVC_NUT_IDR_N_LP = 20,
  AL_HEVC_NUT_CRA = 21,
  AL_HEVC_NUT_RSV_IRAP_VCL22 = 22,
  AL_HEVC_NUT_RSV_IRAP_VCL23 = 23,
  AL_HEVC_NUT_RSV_VCL24 = 24,
  AL_HEVC_NUT_RSV_VCL25 = 25,
  AL_HEVC_NUT_RSV_VCL26 = 26,
  AL_HEVC_NUT_RSV_VCL27 = 27,
  AL_HEVC_NUT_RSV_VCL28 = 28,
  AL_HEVC_NUT_RSV_VCL29 = 29,
  AL_HEVC_NUT_RSV_VCL30 = 30,
  AL_HEVC_NUT_RSV_VCL31 = 31,
  AL_HEVC_NUT_VPS = 32,
  AL_HEVC_NUT_SPS = 33,
  AL_HEVC_NUT_PPS = 34,
  AL_HEVC_NUT_AUD = 35,
  AL_HEVC_NUT_EOS = 36,
  AL_HEVC_NUT_EOB = 37,
  AL_HEVC_NUT_FD = 38,
  AL_HEVC_NUT_PREFIX_SEI = 39,
  AL_HEVC_NUT_SUFFIX_SEI = 40,
  AL_HEVC_NUT_ERR = 64,
}AL_ENut;

/*************************************************************************//*!
   \brief Color Space
*****************************************************************************/
typedef enum e_ColorSpace
{
  UNKNOWN = 0,
  BT_601 = 1,  // YUV
  BT_709 = 2,  // YUV
  SMPTE_170 = 3,  // YUV
  SMPTE_240 = 4,  // YUV
  RESERVED_1 = 5,
  RESERVED_2 = 6,
  SRGB = 7 // RGB
}AL_EColorSpace;

/*************************************************************************//*!
   \brief VP9 interpolation filter
*****************************************************************************/
typedef enum e_InterP_Filter
{
  AL_INTERP_REGULAR,
  AL_INTERP_SMOOTH,
  AL_INTERP_SHARP,
  AL_INTERP_BILINEAR,
  AL_INTERP_SWITCHABLE,
}AL_EInterPFilter;

/*************************************************************************//*!
   \brief Segmentation structure
*****************************************************************************/
#define MAX_SEGMENTS 8
typedef struct AL_t_Segmentation
{
  bool enable;
  bool update_map;
  bool update_data;
  bool abs_delta;
  int16_t feature_data[MAX_SEGMENTS];  // only store data for Q
}AL_TSegmentation;

/*************************************************************************//*!
   \brief Internal frame buffer storage mode
*****************************************************************************/
typedef enum AL_e_FbStorageMode
{
  AL_FB_RASTER = 0,
  AL_FB_TILE_32x4 = 2,
  AL_FB_TILE_64x4 = 3,
  AL_FB_MAX_ENUM, /* sentinel */
}AL_EFbStorageMode;

/*************************************************************************//*!
   \brief Struct for dimension
*****************************************************************************/
typedef struct
{
  int32_t iWidth;
  int32_t iHeight;
}AL_TDimension;

/****************************************************************************/
typedef enum e_SeiFlag
{
  // prefix
  SEI_NONE = 0x00000000, // no SEI
  SEI_BP = 0x00000001, // Buffering period
  SEI_PT = 0x00000002, // Picture Timing
  SEI_RP = 0x00000004, // Recovery Point
  // suffix
  SEI_EOF = 0x00001000, // End of frame
  SEI_ALL = 0x00FFFFFF, // All supported SEI
}AL_SeiFlag;

inline static bool isSuffix(AL_SeiFlag seiFlag)
{
  return seiFlag & 0xFFF000;
}

inline static bool isPrefix(AL_SeiFlag seiFlag)
{
  return seiFlag & 0x000FFF;
}

/*@}*/

