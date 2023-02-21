// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/Nuts.h"

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
   \brief Size of the NAL Header
   respect to the AVC specification
 ***************************************************************************/
#define AL_AVC_NAL_HDR_SIZE 4

