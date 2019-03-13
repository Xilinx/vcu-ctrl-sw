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
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/

#include "assert.h"
#include "Utils.h"
#include "lib_rtos/lib_rtos.h"

/***************************************************************************/
static const uint8_t tab_ceil_log2[] =
{
/*  0.. 7 */
  0, 0, 1, 2, 2, 3, 3, 3,
/*  8..15 */
  3, 4, 4, 4, 4, 4, 4, 4,
/* 16..23 */
  4, 5, 5, 5, 5, 5, 5, 5,
/* 24..31 */
  5, 5, 5, 5, 5, 5, 5, 5
};

/***************************************************************************/
int ceil_log2(uint16_t n)
{
  int v = 0;
  assert(n > 0);

  if(n < 32)
    return tab_ceil_log2[n];

  n--;

  // count the number of bit used to store the decremented n
  while(n != 0)
  {
    n >>= 1;
    ++v;
  }

  return v;
}

/***************************************************************************/
int floor_log2(uint16_t n)
{
  int s = -1;

  while(n != 0)
  {
    n >>= 1;
    ++s;
  }

  return s;
}

/*************************************************************************/
bool AL_AVC_IsIDR(AL_ENut eNUT)
{
  return eNUT == AL_AVC_NUT_VCL_IDR;
}

/*************************************************************************/
bool AL_AVC_IsVcl(AL_ENut eNUT)
{
  return eNUT == AL_AVC_NUT_VCL_IDR || eNUT == AL_AVC_NUT_VCL_NON_IDR;
}

/*************************************************************************/
bool AL_HEVC_IsSLNR(AL_ENut eNUT)
{
  switch(eNUT)
  {
  case AL_HEVC_NUT_TRAIL_N:
  case AL_HEVC_NUT_TSA_N:
  case AL_HEVC_NUT_STSA_N:
  case AL_HEVC_NUT_RADL_N:
  case AL_HEVC_NUT_RASL_N:
  case AL_HEVC_NUT_RSV_VCL_N10:
  case AL_HEVC_NUT_RSV_VCL_N12:
  case AL_HEVC_NUT_RSV_VCL_N14:
    return true;
    break;
  default:
    return false;
    break;
  }
}

/*************************************************************************/
bool AL_HEVC_IsRASL_RADL_SLNR(AL_ENut eNUT)
{
  switch(eNUT)
  {
  case AL_HEVC_NUT_TRAIL_N:
  case AL_HEVC_NUT_TSA_N:
  case AL_HEVC_NUT_STSA_N:
  case AL_HEVC_NUT_RADL_N:
  case AL_HEVC_NUT_RADL_R:
  case AL_HEVC_NUT_RASL_N:
  case AL_HEVC_NUT_RASL_R:
  case AL_HEVC_NUT_RSV_VCL_N10:
  case AL_HEVC_NUT_RSV_VCL_N12:
  case AL_HEVC_NUT_RSV_VCL_N14:
    return true;
    break;
  default:
    return false;
    break;
  }
}

/*************************************************************************/
bool AL_HEVC_IsBLA(AL_ENut eNUT)
{
  return eNUT == AL_HEVC_NUT_BLA_N_LP || eNUT == AL_HEVC_NUT_BLA_W_LP || eNUT == AL_HEVC_NUT_BLA_W_RADL;
}

/*************************************************************************/
bool AL_HEVC_IsCRA(AL_ENut eNUT)
{
  return eNUT == AL_HEVC_NUT_CRA;
}

/*************************************************************************/
bool AL_HEVC_IsIDR(AL_ENut eNUT)
{
  return eNUT == AL_HEVC_NUT_IDR_N_LP || eNUT == AL_HEVC_NUT_IDR_W_RADL;
}

/*************************************************************************/
bool AL_HEVC_IsRASL(AL_ENut eNUT)
{
  return eNUT == AL_HEVC_NUT_RASL_N || eNUT == AL_HEVC_NUT_RASL_R;
}

/*************************************************************************/
bool AL_HEVC_IsVcl(AL_ENut eNUT)
{
  return (eNUT >= AL_HEVC_NUT_TRAIL_N && eNUT <= AL_HEVC_NUT_RASL_R) ||
         (eNUT >= AL_HEVC_NUT_BLA_W_LP && eNUT <= AL_HEVC_NUT_CRA);
}

/***************************************************************************/
int AL_H273_ColourDescToColourPrimaries(AL_EColourDescription colourDesc)
{
  switch(colourDesc)
  {
  case COLOUR_DESC_RESERVED_0: return 0;
  case COLOUR_DESC_BT_709: return 1;
  case COLOUR_DESC_SRGB: return 1;
  case COLOUR_DESC_UNSPECIFIED: return 2;
  case COLOUR_DESC_RESERVED_3: return 3;
  case COLOUR_DESC_BT_470_NTSC: return 4;
  case COLOUR_DESC_BT_470_PAL: return 5;
  case COLOUR_DESC_BT_601: return 6;
  case COLOUR_DESC_SMPTE_170M: return 6;
  case COLOUR_DESC_SMPTE_240M: return 7;
  case COLOUR_DESC_GENERIC_FILM: return 8;
  case COLOUR_DESC_BT_2020: return 9;
  case COLOUR_DESC_MAX_ENUM: assert(0);
  }

  return 2;
}
