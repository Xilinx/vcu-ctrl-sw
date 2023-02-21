// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
