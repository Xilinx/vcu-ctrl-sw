/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "HevcUtils.h"

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
  default:
    return false;
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
