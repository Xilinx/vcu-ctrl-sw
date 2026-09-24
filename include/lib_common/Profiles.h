// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup high_level_syntax High Level Syntax
   !@{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/assert.h"
#include "lib_rtos/types.h"

/****************************************************************************/
#define AL_CS_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)
#define AVC_PROFILE_IDC_CAVLC_444_INTRA 44
#define AVC_PROFILE_IDC_BASELINE 66
#define AVC_PROFILE_IDC_MAIN 77
#define AVC_PROFILE_IDC_EXTENDED 88 // not supported
#define AVC_PROFILE_IDC_HIGH 100
#define AVC_PROFILE_IDC_HIGH10 110
#define AVC_PROFILE_IDC_HIGH_422 122
#define AVC_PROFILE_IDC_HIGH_444_PRED 244

#define AL_RExt_FLAGS(Flags) (((Flags) & 0xFFFF) << 8)
#define HEVC_PROFILE_IDC_MAIN 1
#define HEVC_PROFILE_IDC_MAIN10 2
#define HEVC_PROFILE_IDC_MAIN_STILL 3
#define HEVC_PROFILE_IDC_RExt 4

/*****************************************************************************
   \brief Codec identifier
*****************************************************************************/
typedef enum AL_ECodec
{
  /* assign hardware standard value */
  AL_CODEC_AVC = 0,
  AL_CODEC_HEVC = 1,
  AL_CODEC_AV1 = 2,
  AL_CODEC_VP9 = 3,
  AL_CODEC_JPEG = 4,
  AL_CODEC_VVC = 5,
  AL_CODEC_MPEG2 = 6,
  AL_CODEC_AVC_I = 7,
  AL_CODEC_LCEVC = 8,
  AL_CODEC_AV2 = 9,
  AL_CODEC_JPEG_XS = 10,
  AL_CODEC_INVALID, /* sentinel */
}AL_ECodec;

/******************************************************************************
   \brief Get a string corresponding to a codec
   \param[in] eCodec The codec to get a string description from
   \return a string describing the codec
******************************************************************************/
char const* AL_CodecToString(AL_ECodec eCodec);

/*****************************************************************************
   \brief Profiles identifier
*****************************************************************************/
typedef enum AL_EProfile
{
  AL_PROFILE_AVC = (AL_CODEC_AVC << 24),
  AL_PROFILE_AVC_CAVLC_444_INTRA = AL_PROFILE_AVC | AVC_PROFILE_IDC_CAVLC_444_INTRA, // not supported
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
  AL_PROFILE_AVC_HIGH_INTRA = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x0008), // allow as part of High 10 Intra Profile see NOTE1 in A.2.8
  AL_PROFILE_AVC_HIGH10_INTRA = AL_PROFILE_AVC_HIGH10 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_422_INTRA = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x0008),
  AL_PROFILE_AVC_HIGH_444_INTRA = AL_PROFILE_AVC_HIGH_444_PRED | AL_CS_FLAGS(0x0008), // not supported

  AL_PROFILE_XAVC_HIGH10_INTRA_CBG = AL_PROFILE_AVC_HIGH10 | AL_CS_FLAGS(0x1008),
  AL_PROFILE_XAVC_HIGH10_INTRA_VBR = AL_PROFILE_AVC_HIGH10 | AL_CS_FLAGS(0x3008),
  AL_PROFILE_XAVC_HIGH_422_INTRA_CBG = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x1008),
  AL_PROFILE_XAVC_HIGH_422_INTRA_VBR = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x3008),
  AL_PROFILE_XAVC_LONG_GOP_MAIN_MP4 = AL_PROFILE_AVC_MAIN | AL_CS_FLAGS(0x1000),
  AL_PROFILE_XAVC_LONG_GOP_HIGH_MP4 = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x1000),
  AL_PROFILE_XAVC_LONG_GOP_HIGH_MXF = AL_PROFILE_AVC_HIGH | AL_CS_FLAGS(0x5000),
  AL_PROFILE_XAVC_LONG_GOP_HIGH_422_MXF = AL_PROFILE_AVC_HIGH_422 | AL_CS_FLAGS(0x5000),

  AL_PROFILE_HEVC = (AL_CODEC_HEVC << 24),
  AL_PROFILE_HEVC_MAIN = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN,
  AL_PROFILE_HEVC_MAIN10 = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN10,
  AL_PROFILE_HEVC_MAIN_STILL = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_MAIN_STILL,
  AL_PROFILE_HEVC_RExt = AL_PROFILE_HEVC | HEVC_PROFILE_IDC_RExt,
  AL_PROFILE_HEVC_MONO = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xFC80),
  AL_PROFILE_HEVC_MONO10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xDC80),
  AL_PROFILE_HEVC_MONO12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9C80),
  AL_PROFILE_HEVC_MONO16 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x1C80), // not supported
  AL_PROFILE_HEVC_MAIN12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9880),
  AL_PROFILE_HEVC_MAIN_422 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xF080),
  AL_PROFILE_HEVC_MAIN_422_10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xD080),
  AL_PROFILE_HEVC_MAIN_422_12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9080),
  AL_PROFILE_HEVC_MAIN_444 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE080),
  AL_PROFILE_HEVC_MAIN_444_10 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xC080),
  AL_PROFILE_HEVC_MAIN_444_12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x8080), // not supported
  AL_PROFILE_HEVC_MAIN_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xFA00),
  AL_PROFILE_HEVC_MAIN10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xDA00),
  AL_PROFILE_HEVC_MAIN12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9A00), // not supported
  AL_PROFILE_HEVC_MAIN_422_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xF200),
  AL_PROFILE_HEVC_MAIN_422_10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xD200),
  AL_PROFILE_HEVC_MAIN_422_12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9200), // not supported
  AL_PROFILE_HEVC_MAIN_444_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE200),
  AL_PROFILE_HEVC_MAIN_444_10_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xC200),
  AL_PROFILE_HEVC_MAIN_444_12_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x8200), // not supported
  AL_PROFILE_HEVC_MAIN_444_16_INTRA = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x0200), // not supported
  AL_PROFILE_HEVC_MAIN_444_STILL = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0xE300),
  AL_PROFILE_HEVC_MAIN_444_16_STILL = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x0300), // not supported

  AL_PROFILE_UNKNOWN = ~0,
}AL_EProfile;

/****************************************************************************/
#define AL_MAKE_PROFILE(Codec, Idc, Flags) ((AL_EProfile)(((Codec) << 24) | ((Flags) << 8) | (Idc)))

/****************************************************************************/
static inline AL_ECodec AL_GET_CODEC(AL_EProfile eProfile)
{
  return (AL_ECodec)((eProfile & 0xFF000000) >> 24);
}

/****************************************************************************/
static inline int32_t AL_GET_PROFILE_IDC(AL_EProfile eProfile)
{
  return eProfile & 0x000000FF;
}

#define AL_GET_PROFILE_CODEC_AND_IDC(Prof) ((Prof) & 0xFF0000FF)
#define AL_GET_RExt_FLAGS(Prof) (((Prof) & 0x00FFFF00) >> 8)
#define AL_GET_CS_FLAGS(Prof) (((Prof) & 0x00FFFF00) >> 8)

/****************************************************************************/
static inline bool AL_HAS_LEVEL(AL_EProfile eProfile)
{
  AL_ECodec const eCodec = AL_GET_CODEC(eProfile);
  switch(eCodec)
  {
  case AL_CODEC_AVC:
  case AL_CODEC_HEVC:
  case AL_CODEC_VVC:
  case AL_CODEC_AV1:
  case AL_CODEC_AV2:
  case AL_CODEC_LCEVC:
    return true;
  default:
    return false;
  }
}

/****************************************************************************/
static inline bool AL_IS_VP9(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_VP9;
}

/****************************************************************************/
static inline bool AL_IS_AV1(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_AV1;
}

/****************************************************************************/
static inline bool AL_IS_AV2(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_AV2;
}

/****************************************************************************/
static inline bool AL_IS_AOM(AL_EProfile eProfile)
{
  return AL_IS_AV1(eProfile) || AL_IS_VP9(eProfile) || AL_IS_AV2(eProfile);
}

/****************************************************************************/
static inline bool AL_IS_AOM_CODEC(AL_ECodec eCodec)
{
  return ((eCodec) == AL_CODEC_VP9) || ((eCodec) == AL_CODEC_AV1) || ((eCodec) == AL_CODEC_AV2);
}

/****************************************************************************/
static inline bool AL_IS_D105_CODEC(AL_ECodec eCodec)
{
  return ((eCodec) == AL_CODEC_MPEG2) || ((eCodec) == AL_CODEC_AVC_I);
}

/****************************************************************************/
static inline bool AL_IS_LIB_MVD_CODEC(AL_ECodec eCodec)
{
  return AL_IS_AOM_CODEC(eCodec) || AL_IS_D105_CODEC(eCodec);
}

/****************************************************************************/
static inline bool AL_IS_JPEG(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_JPEG;
}

/****************************************************************************/
static inline bool AL_IS_AVC(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_AVC;
}

/****************************************************************************/
static inline bool AL_IS_HEVC(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_HEVC;
}

/****************************************************************************/
static inline bool AL_IS_VVC(AL_EProfile eProfile)
{
  return AL_GET_CODEC(eProfile) == AL_CODEC_VVC;
}

/****************************************************************************/
static inline bool AL_IS_ITU_CODEC(AL_ECodec eCodec)
{
  bool const bIsAVC = eCodec == AL_CODEC_AVC;
  bool const bIsHEVC = eCodec == AL_CODEC_HEVC;
  bool const bIsVVC = eCodec == AL_CODEC_VVC;
  return bIsAVC || bIsHEVC || bIsVVC;
}

/****************************************************************************/
static inline bool AL_IS_ITU(AL_EProfile eProfile)
{
  return AL_IS_ITU_CODEC(AL_GET_CODEC(eProfile));
}

/****************************************************************************/
static inline bool AL_IS_JPEG_CODEC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_JPEG;
}

/****************************************************************************/
static inline bool AL_IS_JPEG_XS_CODEC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_JPEG_XS;
}

/****************************************************************************/
static inline bool AL_IS_JPEG_OR_JPEG_XS_CODEC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_JPEG || eCodec == AL_CODEC_JPEG_XS;
}

/****************************************************************************/
static inline bool AL_IS_LOW_BITRATE_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0080));
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_STILL_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN_STILL);
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN10) && (AL_GET_RExt_FLAGS(eProfile) & 0x0100));
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0100));

  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_SHVC(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_MultiLayer(int32_t iLayers)
{
  return iLayers > 1;
}

/****************************************************************************/
static inline bool AL_IS_10BIT_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH10);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN10);
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x2000));
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_12BIT_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x6000);
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_16BIT_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_MONO_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH10);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt);
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_400_PROFILE(AL_EProfile eProfile)
{
  return AL_IS_MONO_PROFILE(eProfile);
}

/****************************************************************************/
static inline bool AL_IS_420_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = true;
  /* Only hevc mono doesn't support 420 */
  bool bIsHevcMono = ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0400));
  bRet &= !bIsHevcMono;
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_422_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x0C00));
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_444_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED);
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x1C00));

  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_INTRA_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRet |= (AL_IS_AVC(eProfile) && (AL_GET_PROFILE_IDC(eProfile) >= AVC_PROFILE_IDC_HIGH) && ((AL_GET_CS_FLAGS(eProfile) & 0x0008)));
  bRet |= (AL_IS_HEVC(eProfile) && (AL_GET_PROFILE_IDC(eProfile) >= HEVC_PROFILE_IDC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0200));
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_XAVC(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (
    (AL_IS_AVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x1000) != 0)
    );
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_CBG(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x2000) == 0)
    );
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_VBR(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x2000) != 0)
    );
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_MP4(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x4000) == 0)
    );
  return bRet;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_MXF(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRet = false;
  bRet |= (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x4000) != 0)
    );
  return bRet;
}

/*!@}*/
