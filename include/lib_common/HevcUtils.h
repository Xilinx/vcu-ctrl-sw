/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

