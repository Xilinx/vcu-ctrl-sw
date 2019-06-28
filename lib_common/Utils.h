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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file


 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"

#define NUMCORE_AUTO 0

/***************************************************************************/
static AL_INLINE size_t BitsToBytes(size_t zBits)
{
  return (zBits + 7) / 8;
}

/***************************************************************************/
static AL_INLINE int Clip3(int iVal, int iMin, int iMax)
{
  return ((iVal) < (iMin)) ? (iMin) : ((iVal) > (iMax)) ? (iMax) : (iVal);
}

/***************************************************************************/
static AL_INLINE int Max(int iVal1, int iVal2)
{
  return ((iVal1) < (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE uint32_t UnsignedMax(uint32_t iVal1, uint32_t iVal2)
{
  return ((iVal1) < (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE size_t UnsignedMin(size_t iVal1, size_t iVal2)
{
  return ((iVal1) > (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE int Min(int iVal1, int iVal2)
{
  return ((iVal1) > (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE int Abs(int iVal)
{
  return ((iVal) > 0) ? (iVal) : (-(iVal));
}

/***************************************************************************/
static AL_INLINE int Sign(int iVal)
{
  return ((iVal) > 0) ? (1) : (((iVal) < 0) ? (-1) : (0));
}

/***************************************************************************/
static AL_INLINE int RoundUp(int iVal, int iRnd)
{
  return iVal >= 0 ? ((iVal + iRnd - 1) / iRnd) * iRnd : (iVal / iRnd) * iRnd;
}

/***************************************************************************/
static AL_INLINE int RoundDown(int iVal, int iRnd)
{
  return iVal >= 0 ? (iVal / iRnd) * iRnd : ((iVal - iRnd + 1) / iRnd) * iRnd;
}

/***************************************************************************/
static AL_INLINE size_t UnsignedRoundUp(size_t zVal, size_t zRnd)
{
  return ((zVal + zRnd - 1) / zRnd) * zRnd;
}

/***************************************************************************/
static AL_INLINE size_t UnsignedRoundDown(size_t zVal, size_t zRnd)
{
  return (zVal / zRnd) * zRnd;
}

AL_INLINE static AL_ECodec AL_GetCodec(AL_EProfile eProf)
{

  if(AL_IS_AVC(eProf))
    return AL_CODEC_AVC;

  if(AL_IS_HEVC(eProf))
    return AL_CODEC_HEVC;
  return AL_CODEC_INVALID;
}

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to an IDR
   picture respect to the AVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's an IDR nal unit type
   false otherwise
 ***************************************************************************/
bool AL_AVC_IsIDR(AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to a Vcl NAL
   respect to the AVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's a VCL nal unit type
   false otherwise
 ***************************************************************************/
bool AL_AVC_IsVcl(AL_ENut eNUT);
/*************************************************************************//*!
    \brief This function checks if the current Nal Unit Type corresponds to an
           SLNR picture respect to the HEVC specification
    \param[in] eNUT Nal Unit Type
    \return true if it's an SLNR nal unit type
    false otherwise
 ***************************************************************************/
bool AL_HEVC_IsSLNR(AL_ENut eNUT);
/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to an RADL,
   RASL or SLNR picture respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's an RASL, RADL or SLNR nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsRASL_RADL_SLNR(AL_ENut eNUT);
/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to a BLA
   picture respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's a BLA nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsBLA(AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to a CRA
   picture respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's a CRA nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsCRA(AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to an IDR
   picture respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's an IDR nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsIDR(AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to an RASL
   picture respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's an RASL nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsRASL(AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function checks if the current Nal Unit Type corresponds to a Vcl NAL
   respect to the HEVC specification
   \param[in] eNUT Nal Unit Type
   \return true if it's a VCL nal unit type
   false otherwise
 ***************************************************************************/
bool AL_HEVC_IsVcl(AL_ENut eNUT);

/***************************************************************************/
int ceil_log2(uint16_t n);

/****************************************************************************/
int floor_log2(uint16_t n);

/****************************************************************************/
int AL_H273_ColourDescToColourPrimaries(AL_EColourDescription colourDesc);

/*************************************************************************//*!
   \brief Reference picture status
 ***************************************************************************/
typedef enum e_MarkingRef
{
  SHORT_TERM_REF,
  LONG_TERM_REF,
  UNUSED_FOR_REF,
  NON_EXISTING_REF,
  AL_MARKING_REF_MAX_ENUM, /* sentinel */
}AL_EMarkingRef;

/*@}*/

