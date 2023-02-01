/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

/****************************************************************************
 *****************************************************************************/
#pragma once

extern "C"
{
#include "lib_common/Error.h"
#include "lib_common_enc/Settings.h"
}

#include <string>
#include "ROIMngr.h"

typedef enum e_GenerateQpMode
{
  // exclusive modes
  AL_GENERATE_UNIFORM_QP = 0x00, /*!< default behaviour */
  AL_GENERATE_LOAD_QP = 0x01, /*!< used for test purpose */
  AL_GENERATE_ROI_QP = 0x02,
  AL_GENERATE_QP_TABLE_MASK = 0x03,

  // test modes
  AL_GENERATE_RANDOM_QP = 0x04, /*!< used for test purpose */
  AL_GENERATE_RANDOM_I_ONLY = 0x80, /*!< used for test purpose */
  AL_GENERATE_RANDOM_SKIP = 0x10, /*!< used for test purpose */
  AL_GENERATE_QP_TABLE_RANDOM_MASK = 0xFC,

  AL_GENERATE_QP_TABLE_MASK_EXT = AL_GENERATE_QP_TABLE_MASK | AL_GENERATE_QP_TABLE_RANDOM_MASK,

  // Auto QP
  AL_GENERATE_AUTO_QP = 0x400, /*!< compute Qp by MB on the fly */
  AL_GENERATE_ADAPTIVE_AUTO_QP = 0x800, /*!< Dynamically compute Qp by MB on the fly */
  AL_GENERATE_MASK_AUTO_QP = 0xC00,

  // QP table mode
  AL_GENERATE_RELATIVE_QP = 0x8000,
  AL_GENERATE_QP_MAX_ENUM, /* sentinel */
}AL_EGenerateQpMode;

static inline bool AL_IsAutoQP(AL_EGenerateQpMode eMode)
{
  return eMode & AL_GENERATE_MASK_AUTO_QP;
}

static inline bool AL_HasQpTable(AL_EGenerateQpMode eMode)
{
  return eMode & AL_GENERATE_QP_TABLE_MASK_EXT;
}

/*************************************************************************//*!
   \brief Fill QP part of the buffer pointed to by pQP with a QP for each
        Macroblock of the slice.
   \param[in]  eMode      Specifies the way QP values are computed. see AL_EGenerateQpMode
   \param[in]  iSliceQP   Slice QP value (in range [0..51])
   \param[in]  iMinQP     Minimum allowed QP value (in range [0..50])
   \param[in]  iMaxQP     Maximum allowed QP value (in range [1..51]).
   \param[in]  iLCUPicWidth  Width in Lcu Unit of the picture
   \param[in]  iLCUPicHeight Height in Lcu Unit of the picture
   \param[in]  eProf      Profile used for the encoding
   \param[in]  uLogMaxCuSize Log2 maximum CU size
   \param[in]  iQPTableDepth Ctb maximum depth of QP Table
   \param[in]  sQPTablesFolder In case QP are loaded from files, path to the folder
               containing the QP table files
   \param[in]  iFrameID   Frame identifier
   \param[out] pQPTable       Pointer to the buffer that receives the QP Table
   \note iMinQp <= iMaxQP
   \return 0 on success, 1 if file is not found, 2 if there is an error in the file
*****************************************************************************/
AL_ERR GenerateQPBuffer(AL_EGenerateQpMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, uint8_t uLogMaxCuSize, int iQPTableDepth, const std::string& sQPTablesFolder, int iFrameID, uint8_t* pQPTable);

/*************************************************************************//*!
   \brief Fill QP part of the buffer pointed to by pQP with a QP for each
        Macroblock of the slice with roi information
   \param[in]  pRoiCtx    Pointer to the roi object holding roi information
   \param[in]  sRoiFileName path and file name of the ROI description
   \param[in]  eMode      Specifies the way QP values are computed. see EQpCtrlMode
   \param[in]  iLCUPicWidth  Width in Lcu Unit of the picture
   \param[in]  iLCUPicHeight Height in Lcu Unit of the picture
   \param[in]  eProf      Profile used for the encoding
   \param[in]  uLogMaxCuSize Log2 maximum CU size
   \param[in]  iQPTableDepth Ctb maximum depth of QP Table
   \param[in]  iFrameID   Frame identifier
   \param[out] pQPs       Pointer to the buffer that receives the computed QPs
   \return true on success, false on error
*****************************************************************************/
AL_ERR GenerateROIBuffer(AL_TRoiMngrCtx* pRoiCtx, std::string const& sRoiFileName, int iLCUPicWidth, int iLCUPicHeight, AL_EProfile eProf, uint8_t uLogMaxCuSize, int iQPTableDepth, int iFrameID, uint8_t* pQPs);

