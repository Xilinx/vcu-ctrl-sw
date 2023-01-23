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
   \addtogroup high_level_syntax High Level Syntax
   @{
   \file
******************************************************************************/
#pragma once

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

/****************************************************************************/
typedef enum AL_e_Codec
{
  /* assign hardware standard value */
  AL_CODEC_AVC = 0,
  AL_CODEC_HEVC = 1,
  AL_CODEC_AV1 = 2,
  AL_CODEC_VP9 = 3,
  AL_CODEC_JPEG = 4,
  AL_CODEC_VVC = 5,
  AL_CODEC_INVALID, /* sentinel */
}AL_ECodec;

/*************************************************************************//*!
   \brief Profiles identifier
*****************************************************************************/
typedef enum __AL_ALIGNED__ (4) AL_e_Profile
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
  AL_PROFILE_HEVC_MONO12 = AL_PROFILE_HEVC_RExt | AL_RExt_FLAGS(0x9C80), // not supported
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
} AL_EProfile;

/****************************************************************************/
#define AL_MAKE_PROFILE(Codec, Idc, Flags) ((AL_EProfile)(((Codec) << 24) | ((Flags) << 8) | (Idc)))

/****************************************************************************/
static inline AL_ECodec AL_GET_CODEC(AL_EProfile eProfile)
{
  return (AL_ECodec)((eProfile & 0xFF000000) >> 24);
}

/****************************************************************************/
static inline int AL_GET_PROFILE_IDC(AL_EProfile eProfile)
{
  return eProfile & 0x000000FF;
}

#define AL_GET_PROFILE_CODEC_AND_IDC(Prof) (Prof & 0xFF0000FF)
#define AL_GET_RExt_FLAGS(Prof) ((Prof & 0x00FFFF00) >> 8)
#define AL_GET_CS_FLAGS(Prof) ((Prof & 0x00FFFF00) >> 8)

/****************************************************************************/
static inline bool AL_IS_VP9(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bIsCodecVP9 = false;
  return bIsCodecVP9;
}

/****************************************************************************/
static inline bool AL_IS_AV1(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bIsCodecAV1 = false;
  return bIsCodecAV1;
}

/****************************************************************************/
static inline bool AL_IS_AOM(AL_EProfile eProfile)
{
  return AL_IS_AV1(eProfile) || AL_IS_VP9(eProfile);
}

/****************************************************************************/
static inline bool AL_IS_AOM_CODEC(AL_ECodec eCodec)
{
  return ((eCodec) == AL_CODEC_VP9) || ((eCodec) == AL_CODEC_AV1);
}

/****************************************************************************/
static inline bool AL_IS_JPEG(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bIsCodecJPEG = false;
  return bIsCodecJPEG;
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
  (void)eCodec;
  bool bIsAVC = false;
  bool bIsHEVC = false;
  bool bIsVVC = false;
  bIsAVC = ((eCodec) == AL_CODEC_AVC);
  bIsHEVC = ((eCodec) == AL_CODEC_HEVC);
  return bIsAVC || bIsHEVC || bIsVVC;
}

/****************************************************************************/
static inline bool AL_IS_JPEG_CODEC(AL_ECodec eCodec)
{
  (void)eCodec;
  bool bIsJpeg = false;
  return bIsJpeg;
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
  bRet |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN_STILL)
          || ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN10) && (AL_GET_RExt_FLAGS(eProfile) & 0x0100))
          || ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0100));

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
static inline bool AL_IS_MultiLayer(int iLayers)
{
  return iLayers > 1;
}

/****************************************************************************/
static inline bool AL_IS_10BIT_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH10)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_MAIN10)
          || ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x2000));
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_12BIT_PROFILE(AL_EProfile eProfile)
{
  bool bRes = false;
  bRes = (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED)
         || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x6000);
  (void)eProfile;
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_16BIT_PROFILE(AL_EProfile eProfile)
{
  bool bRes = false;

  (void)eProfile;
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_MONO_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH10)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt);
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_420_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = true;
  /* Only hevc mono doesn't support 420 */
  bool bIsHevcMono = ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0400));
  bRes &= !bIsHevcMono;
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_422_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_422)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRes |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x0C00));
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_444_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_HIGH_444_PRED)
          || (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA);
  bRes |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && !(AL_GET_RExt_FLAGS(eProfile) & 0x1C00));

  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_INTRA_PROFILE(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes |= (AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_AVC_CAVLC_444_INTRA)
          || (AL_IS_AVC(eProfile) && ((AL_GET_CS_FLAGS(eProfile) & 0x0008)));
  bRes |= ((AL_GET_PROFILE_CODEC_AND_IDC(eProfile) == AL_PROFILE_HEVC_RExt) && (AL_GET_RExt_FLAGS(eProfile) & 0x0200));
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_XAVC(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes = (
    (AL_IS_AVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x1000) != 0)
    );
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_CBG(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes = (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x2000) == 0)
    );
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_VBR(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes = (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x2000) != 0)
    );
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_MP4(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes = (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x4000) == 0)
    );
  return bRes;
}

/****************************************************************************/
static inline bool AL_IS_XAVC_MXF(AL_EProfile eProfile)
{
  (void)eProfile;
  bool bRes = false;
  bRes = (
    (AL_IS_XAVC(eProfile))
    && ((AL_GET_CS_FLAGS(eProfile) & 0x4000) != 0)
    );
  return bRes;
}

/*@}*/
