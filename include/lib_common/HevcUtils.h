// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/Nuts.h"

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

/*************************************************************************//*!
   \brief Size of the NAL Header
   respect to the HEVC specification
 ***************************************************************************/
#define AL_HEVC_NAL_HDR_SIZE 5

