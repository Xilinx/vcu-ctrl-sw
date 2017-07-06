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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/****************************************************************************/
#define AL_CS_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)
#define AL_RExt_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)

/*************************************************************************//*!
   \brief HEVC Profile identifier
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_Profile
{
  AL_PROFILE_AVC = 0x01000000,
  AL_PROFILE_AVC_CAVLC_444 = AL_PROFILE_AVC | 44, // not supported
  AL_PROFILE_AVC_BASELINE = AL_PROFILE_AVC | 66,
  AL_PROFILE_AVC_MAIN = AL_PROFILE_AVC | 77,
  AL_PROFILE_AVC_EXTENDED = AL_PROFILE_AVC | 88, // not supported
  AL_PROFILE_AVC_HIGH = AL_PROFILE_AVC | 100,
  AL_PROFILE_AVC_HIGH10 = AL_PROFILE_AVC | 110,
  AL_PROFILE_AVC_HIGH_422 = AL_PROFILE_AVC | 122,
  AL_PROFILE_AVC_HIGH_444_PRED = AL_PROFILE_AVC | 244, // not supported

  AL_PROFILE_AVC_C_BASELINE = AL_PROFILE_AVC_BASELINE | AL_CS_FLAGS(0x0002),
  AL_PROFILE_AVC_PROG_HIGH = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x0010),
  AL_PROFILE_AVC_C_HIGH = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x0030),
  AL_PROFILE_AVC_HIGH10_INTRA = AL_PROFILE_AVC_HIGH10 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_422_INTRA = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_444_INTRA = AL_PROFILE_AVC_HIGH_444_PRED | AL_CS_FLAGS(0x0008), // not supported

  AL_PROFILE_HEVC = 0x02000000,
  AL_PROFILE_HEVC_MAIN = AL_PROFILE_HEVC | 1,
  AL_PROFILE_HEVC_MAIN10 = AL_PROFILE_HEVC | 2,
  AL_PROFILE_HEVC_MAIN_STILL = AL_PROFILE_HEVC | 3,
  AL_PROFILE_HEVC_RExt = AL_PROFILE_HEVC | 4,

  AL_PROFILE_HEVC_MONO = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xFC80),
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
#define AL_GET_PROFILE_IDC(Prof) ((Prof) & 0x000000FF)
#define AL_GET_RExt_FLAGS(Prof) (((Prof) & 0x00FFFF00) >> 8)
#define AL_GET_CS_FLAGS(Prof) (((Prof) & 0x00FFFF00) >> 8)
#define AL_IS_STILL_PROFILE(Prof) (((Prof) & 0x00000100) || ((Prof) == AL_PROFILE_HEVC_MAIN_STILL))
#define AL_IS_INTRA_PROFILE(Prof) (((((Prof) & 0xFF0000FF) == AL_PROFILE_HEVC_RExt) && ((Prof) & 0x00020000)) || ((Prof) == AL_PROFILE_AVC_HIGH10_INTRA) || ((Prof) == AL_PROFILE_AVC_HIGH_422_INTRA) || ((Prof) == AL_PROFILE_AVC_HIGH_444_INTRA))
#define AL_IS_422_PROFILE(Prof) (((((Prof) & 0xFF0000FF) == AL_PROFILE_HEVC_RExt) && !((Prof) & 0x000C0000)) || ((Prof) == AL_PROFILE_AVC_HIGH_422) || ((Prof) == AL_PROFILE_AVC_HIGH_422_INTRA))
#define AL_IS_10BIT_PROFILE(Prof) (((((Prof) & 0xFF0000FF) == AL_PROFILE_HEVC_RExt) && !((Prof) & 0x00200000)) || ((Prof) == AL_PROFILE_HEVC_MAIN10) || ((Prof) == AL_PROFILE_AVC_HIGH10) || ((Prof) == AL_PROFILE_AVC_HIGH_422) || ((Prof) == AL_PROFILE_AVC_HIGH_422_INTRA))
#define AL_IS_LOW_BITRATE_PROFILE(Prof) ((Prof) & 0x00008000)

/****************************************************************************/
#define AL_IS_AVC(Prof) (((Prof) & 0xFF000000) == AL_PROFILE_AVC)
#define AL_IS_HEVC(Prof) (((Prof) & 0xFF000000) == AL_PROFILE_HEVC)

/****************************************************************************/

/*************************************************************************//*!
   \brief GDR Mode
*****************************************************************************/
typedef enum AL_e_GdrMode
{
  AL_GDR_OFF = 0x00,
  AL_GDR_VERTICAL = 0x02,
  AL_GDR_HORIZONTAL = 0x03,
}AL_EGdrMode;

/****************************************************************************/
typedef enum AL_e_Codec
{
  AL_CODEC_AVC = 0x00,
  AL_CODEC_HEVC = 0x01,
  AL_CODEC_JPEG = 0x02,
  AL_CODEC_VP9 = 0x03,
  AL_CODEC_INVALID = 0xFF
}AL_ECodec;

/*************************************************************************//*!
   \brief Scaling List identifier
*****************************************************************************/
typedef enum e_ScalingList
{
  FLAT = 0,
  DEFAULT = 1,
  CUSTOM = 2,
  RANDOM_SCL = 3
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
  SLICE_SI = 4, /*!<AVC SI Slice */
  SLICE_SP = 3, /*!<AVC SP Slice */
  SLICE_I = 2,  /*!< I Slice */
  SLICE_P = 1,  /*!< P Slice */
  SLICE_B = 0,  /*!< B Slice */

  SLICE_CONCEAL = 6, /*< Conceal Slice */

  SLICE_SKIP = 7, /*< slice skip */
  SLICE_REPEAT = 8,
  /* should always be last */
  SLICE_MAX_ENUM
}AL_ESliceType;

/*************************************************************************//*!
   \brief Indentifies pic_struct (subset of table D-1)
*****************************************************************************/
typedef enum e_PicStruct
{
  PS_FRM = 0,
  PS_FRM_x2 = 7,
  PS_FRM_x3 = 8,
  /* should always be last */
  PS_FRM_MAX_ENUM
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
  AL_MODE_CAVLC = 0,
  AL_MODE_CABAC = 1,
}AL_EEntropyMode;

/*************************************************************************//*!
   \brief identifies the deblocking filter mode
*****************************************************************************/
typedef enum e_FilterMode
{
  FILT_ENABLE = 0,
  FILT_DISABLE = 1,
  FILT_DIS_SLICE = 2,
}AL_EFilterMode;

/*************************************************************************//*!
   \brief Weighted Pred Mode
*****************************************************************************/
typedef enum e_WPMode
{
  AL_WP_DEFAULT = 0,
  AL_WP_EXPLICIT = 1,
  AL_WP_IMPLICIT = 2,
}AL_EWPMode;

/*************************************************************************//*!
   \brief Nal unit type enum
 *************************************************************************/
typedef enum e_NalUnitType
{
  AL_AVC_UNDEF_NUT = 0,
  AL_AVC_NUT_VCL_NON_IDR = 1,
  AL_AVC_NUT_VCL_IDR = 5,
  AL_AVC_NUT_SEI = 6,
  AL_AVC_NUT_SPS = 7,
  AL_AVC_NUT_PPS = 8,
  AL_AVC_NUT_AUD = 9,
  AL_AVC_NUT_EOS = 10,
  AL_AVC_NUT_EOB = 11,
  AL_AVC_NUT_FD = 12,
  AL_AVC_NUT_SPS_EXT = 13,
  AL_AVC_NUT_SUB_SPS = 15,
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
  AL_INTERP_REGULAR = 0,
  AL_INTERP_SMOOTH = 1,
  AL_INTERP_SHARP = 2,
  AL_INTERP_BILINEAR = 3,
  AL_INTERP_SWITCHABLE = 4,
}AL_EInterPFilter;

// ----------- Inter Pred  -------
typedef enum
{
  VP9_INTER_SINGLE = 0,
  VP9_INTER_COMPOUND = 1,
  VP9_INTER_SELECT = 2,
  VP9_INTER_MODES
}VP9_INTERPRED_MODE;

/*************************************************************************//*!
   \brief VP9 Intra Mode
*****************************************************************************/
typedef enum
{
  VP9_INTRA_DC,
  VP9_INTRA_V,
  VP9_INTRA_H,
  VP9_INTRA_D45,
  VP9_INTRA_D135,
  VP9_INTRA_D117,
  VP9_INTRA_D153,
  VP9_INTRA_D207,
  VP9_INTRA_D63,
  VP9_INTRA_TM,
  AL_VP9_INTRA_MODES
}AL_VP9_INTRA_MODE;

/*************************************************************************//*!
   \brief VP9 Partition Mode
*****************************************************************************/

typedef enum
{
  PARTITION_NONE,
  PARTITION_HORZ,
  PARTITION_VERT,
  PARTITION_SPLIT,
  AL_PARTITION_TYPES,
  PARTITION_INVALID = AL_PARTITION_TYPES
}AL_PARTITION_TYPE;

/************************************************************************/
typedef enum
{
  MV_JOINT_ZERO = 0, /* Zero vector */
  MV_JOINT_HNZVZ = 1, /* Vert zero, hor nonzero */
  MV_JOINT_HZVNZ = 2, /* Hor zero, vert nonzero */
  MV_JOINT_HNZVNZ = 3, /* Both components nonzero */
}AL_VP9_MV_JOINT_TYPE;

/************************************************************************/
typedef enum
{
  MV_CLASS_0 = 0, /* (0, 2] integer pel */
  MV_CLASS_1 = 1, /* (2, 4] integer pel */
  MV_CLASS_2 = 2, /* (4, 8] integer pel */
  MV_CLASS_3 = 3, /* (8, 16] integer pel */
  MV_CLASS_4 = 4, /* (16, 32] integer pel */
  MV_CLASS_5 = 5, /* (32, 64] integer pel */
  MV_CLASS_6 = 6, /* (64, 128]  integer pel */
  MV_CLASS_7 = 7, /* (128, 256] integer pel */
  MV_CLASS_8 = 8, /* (256, 512] integer pel */
  MV_CLASS_9 = 9, /* (512, 1024] integer pel */
  MV_CLASS_10 = 10, /* (1024,2048] integer pel */
}AL_MV_CLASS_TYPE;

/************************************************************************/
typedef enum
{
  VP9_INTER_NEAREST_MV = 0,
  VP9_INTER_NEAR_MV = 1,
  VP9_INTER_ZERO_MV = 2,
  VP9_INTER_NEW_MV = 3,
  AL_VP9_MVPRED_MODES
}AL_VP9_MVPRED_MODE;

// block transform size
typedef enum
{
  ONLY_4x4 = 0,
  ALLOW_8x8 = 1,
  ALLOW_16x16 = 2,
  ALLOW_32x32 = 3,
  TX_MODE_SELECT = 4
}VP9_TX_MODE;

static const uint8_t VP9_DEFAULT_REF_FRAME_SIGN_BIAS[] =
{
  0, 0, 0, 1
}; // ref frames : LAST / ALTREF

#define MAX_PROB 255
#define MAX_SEGMENTS 8
#define SEG_TREE_PROBS (MAX_SEGMENTS - 1)

// Segment level features.
typedef enum
{
  SEG_LVL_ALT_Q = 0, // Use alternate Quantizer ....
  SEG_LVL_ALT_LF = 1, // Use alternate loop filter value...
  SEG_LVL_REF_FRAME = 2, // Optional Segment reference frame
  SEG_LVL_SKIP = 3, // Optional Segment (0,0) + skip mode
  SEG_LVL_MAX = 4 // Number of features supported
}SEG_LVL_FEATURES;

/*************************************************************************//*!
   \brief Segentation structure
*****************************************************************************/
typedef struct AL_t_Segmentation
{
  bool enable;
  bool update_map;
  bool update_data;
  bool abs_delta;
  int16_t feature_data[MAX_SEGMENTS];  // only store data for Q
}AL_TSegmentation;

static const uint8_t seg_feature_data_signed[SEG_LVL_MAX] =
{
  1, 1, 0, 0
};
static const uint8_t seg_feature_data_numbits[SEG_LVL_MAX] =
{
  8, 6, 2, 1
};
static const uint8_t seg_tree_probs_value[SEG_TREE_PROBS] =
{
  205, 176, 192, 186, 154, 171, 255
}; // TODO adjust probs.

/*@}*/

/*************************************************************************//*!
   \brief Internal frame buffer storage mode
*****************************************************************************/
typedef enum AL_e_FbStorageMode
{
  AL_FB_RASTER = 0,
  AL_FB_TILE_32x4 = 2,
  AL_FB_TILE_64x4 = 3
}AL_EFbStorageMode;

