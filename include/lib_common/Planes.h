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

/**************************************************************************//*!
   \addtogroup Planes

   Defines types and functions associated to the planes of a frame buffer

   @{
   \file
******************************************************************************/
#pragma once

#include "lib_common/PicFormat.h"

/*************************************************************************//*!
   \brief Types of planes in a frame buffers
*****************************************************************************/
// Add new plane at the bottom of the appropriate list

#define AL_PLANE_PIXEL_LIST \
  AL_PLANE_Y, \
  AL_PLANE_U, \
  AL_PLANE_V, \
  AL_PLANE_UV, \
  AL_PLANE_YUV \

#define AL_PLANE_MAP_LIST \
  AL_PLANE_MAP_Y, \
  AL_PLANE_MAP_U, \
  AL_PLANE_MAP_V, \
  AL_PLANE_MAP_UV \

typedef enum AL_e_PlaneId
{
  AL_PLANE_PIXEL_LIST,
  AL_PLANE_MAP_LIST,
  AL_PLANE_MAX_ENUM, /* sentinel */
}AL_EPlaneId;

#undef AL_PLANE_PIXEL_LIST
#undef AL_PLANE_MAP_LIST

#define AL_MAX_BUFFER_PLANES 6

/*************************************************************************//*!
   \brief Plane parameters
*****************************************************************************/
typedef struct AL_t_PlaneDescription
{
  AL_EPlaneId ePlaneId; /*!< Type of plane */
  int iOffset;          /*!< Offset of the plane from beginning of the buffer (in bytes) */
  int iPitch;           /*!< Pitch of the plane (in bytes) */
}AL_TPlaneDescription;

/*************************************************************************//*!
   \brief Check if a plane contains pixel data
   \param[in] ePlaneId The plane type
   \return Returns true if the plane contains pixel data, false otherwise
*****************************************************************************/
bool AL_Plane_IsPixelPlane(AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Check if a plane contains map data
   \param[in] ePlaneId The plane type
   \return Returns true if the plane contains map data, false otherwise
*****************************************************************************/
bool AL_Plane_IsMapPlane(AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Get the list of pixel planes contained in a frame buffer
   \param[in] eChromaOrder Chroma order of the frame buffer
   \param[out] usedPlanes Filled with the list of pixel plane ids contained in
              the frame buffer
   \return Returns the number of pixel planes contained in the frame buffer
*****************************************************************************/
int AL_Plane_GetBufferPixelPlanes(AL_EChromaOrder eChromaOrder, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES]);

/*************************************************************************//*!
   \brief Get the list of map planes contained in a frame buffer
   \param[in] eChromaOrder Chroma order of the frame buffer
   \param[out] usedPlanes Filled with the list of map plane ids contained in
   the frame buffer
   \return Returns the number of map planes contained in the frame buffer
*****************************************************************************/
int AL_Plane_GetBufferMapPlanes(AL_EChromaOrder eChromaOrder, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES]);

/*************************************************************************//*!
   \brief Get the list of planes contained in a frame buffer
   \param[in] eChromaOrder Chroma order of the frame buffer
   \param[in] bIsCompressed True if the frame buffer is compressed
   \param[out] usedPlanes Filled with the list of plane ids contained in the frame
              buffer
   \return Returns the number of planes contained in the frame buffer
*****************************************************************************/
int AL_Plane_GetBufferPlanes(AL_EChromaOrder eChromaOrder, bool bIsCompressed, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES]);

/*************************************************************************//*!
   \brief Check that a plane is contained in a frame buffer
   \param[in] eChromaOrder Chroma order of the frame buffer
   \param[in] bIsCompressed True if the frame buffer is compressed
   \param[out] ePlaneId The type of plane
   \return Returns true if the plane is contained in the buffer, false otherwise
*****************************************************************************/
bool AL_Plane_Exists(AL_EChromaOrder eChromaOrder, bool bIsCompressed, AL_EPlaneId ePlaneId);

/*@}*/
