// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/HevcUtils.h"

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
