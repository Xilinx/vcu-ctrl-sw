/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#include "lib_common/BufferMeta.h"
#include "lib_common/FourCC.h"
#include "lib_common/Planes.h"

/*************************************************************************//*!
   \brief Plane parameters
*****************************************************************************/
typedef struct AL_t_Plane
{
  int iChunkIdx;   /*!< Index of the chunk containing the plane */
  int iOffset;      /*!< Offset of the plane from beginning of the buffer chunk (in bytes) */
  int iPitch;       /*!< Pitch of the plane (in bytes) */
}AL_TPlane;

/*************************************************************************//*!
   \brief Useful information related to the framebuffers containing the picture
*****************************************************************************/
typedef struct AL_t_PixMapMetaData
{
  AL_TMetaData tMeta;
  AL_TDimension tDim; /*!< Dimension in pixel of the frame */
  AL_TPlane tPlanes[AL_PLANE_MAX_ENUM]; /*! < Array of color planes parameters */
  TFourCC tFourCC; /*!< FOURCC identifier */
}AL_TPixMapMetaData;

/*************************************************************************//*!
   \brief Create a pixmap metadata containing no planes
   \param[in] tFourCC FourCC of the framebuffer
   \return Returns NULL in case of failure. Returns a pointer to the metadata in
   case of success.
*****************************************************************************/
AL_TPixMapMetaData* AL_PixMapMetaData_CreateEmpty(TFourCC tFourCC);

/*************************************************************************//*!
   \brief Create a pixmap metadata for a semiplanar picture.
   \param[in] tDim Dimension of the the picture (width and height in pixels)
   \param[in] tYPlane Luma plane parameters (offset and pitch in bytes)
   \param[in] tUVPlane Chroma plane parameters (offset and pitch in bytes)
   \param[in] tFourCC FourCC of the framebuffer
   \return Returns NULL in case of failure. Returns a pointer to the metadata in
   case of success.
*****************************************************************************/
AL_TPixMapMetaData* AL_PixMapMetaData_Create(AL_TDimension tDim, AL_TPlane tYPlane, AL_TPlane tUVPlane, TFourCC tFourCC);

/*************************************************************************//*!
   \brief Create an identical copy of a pixmap metadata
   \param[in] pMeta A pointer the pixmap metadata
   \return Returns NULL in case of failure. Returns a pointer to the metadata
   copy in case of success.
*****************************************************************************/
AL_TPixMapMetaData* AL_PixMapMetaData_Clone(AL_TPixMapMetaData* pMeta);

/*************************************************************************//*!
   \brief Add a plane to a pixmap metadata
   \param[in] pMeta A pointer the pixmap metadata
   \param[in] tPlane plane parameters
   \param[in] ePlaneId plane type
   \return Returns true if plane sucessfully added, false otherwise
*****************************************************************************/
bool AL_PixMapMetaData_AddPlane(AL_TPixMapMetaData* pMeta, AL_TPlane tPlane, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Get the offset in bytes of a plane from the beginning of its buffer chunk
   \param[in] pMeta A pointer the pixmap metadata
   \param[in] ePlaneId plane type
   \return Returns the plane buffer offset
*****************************************************************************/
int AL_PixMapMetaData_GetOffset(AL_TPixMapMetaData* pMeta, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Get the size of the luma inside the picture
   \param[in] pMeta A pointer the pixmap metadata
   \return Returns size of the luma region
*****************************************************************************/
int AL_PixMapMetaData_GetLumaSize(AL_TPixMapMetaData* pMeta);

/*************************************************************************//*!
   \brief Get the size of the chroma inside the picture
   \param[in] pMeta A pointer the pixmap metadata
   \return Returns size of the chroma region
*****************************************************************************/
int AL_PixMapMetaData_GetChromaSize(AL_TPixMapMetaData* pMeta);

AL_DEPRECATED("Use AL_PixMapMetaData_GetOffset.")
int AL_PixMapMetaData_GetOffsetY(AL_TPixMapMetaData* pMeta);
AL_DEPRECATED("Use AL_PixMapMetaData_GetOffset.")
int AL_PixMapMetaData_GetOffsetUV(AL_TPixMapMetaData* pMeta);
AL_DEPRECATED("Renamed. Use AL_TPixMapMetaData.")
typedef AL_TPixMapMetaData AL_TSrcMetaData;

/*@}*/
