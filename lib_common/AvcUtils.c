// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/AvcUtils.h"

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
