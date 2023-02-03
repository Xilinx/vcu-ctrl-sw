/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/HDR.h"
#include "lib_common/BufferMeta.h"

/*************************************************************************//*!
   \brief Metadata containing HDR related settings
*****************************************************************************/
typedef struct AL_t_HDRMeta
{
  AL_TMetaData tMeta;
  AL_EColourDescription eColourDescription;
  AL_ETransferCharacteristics eTransferCharacteristics;
  AL_EColourMatrixCoefficients eColourMatrixCoeffs;
  AL_THDRSEIs tHDRSEIs;
}AL_THDRMetaData;

/*************************************************************************//*!
   \brief Creates a HDR Metadata
   \return Pointer to an HDR Metadata if success, NULL otherwise
*****************************************************************************/
AL_THDRMetaData* AL_HDRMetaData_Create(void);

/*************************************************************************//*!
   \brief Reset an HDR MetaData
   \param[in] pMeta Pointer to the HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Reset(AL_THDRMetaData* pMeta);

/*************************************************************************//*!
   \brief Copy HDR Info from one HDRMetaData to another
   \param[in] pMetaSrc Pointer to the source HDR Metadata
   \param[in] pMetaDst Pointer to the destination HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Copy(AL_THDRMetaData* pMetaSrc, AL_THDRMetaData* pMetaDst);

/*@}*/

