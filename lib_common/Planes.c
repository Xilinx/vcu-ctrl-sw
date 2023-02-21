// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/Planes.h"

bool AL_Plane_IsPixelPlane(AL_EPlaneId ePlaneId)
{
  return ePlaneId < (int)AL_PLANE_MAP_Y;
}

bool AL_Plane_IsMapPlane(AL_EPlaneId ePlaneId)
{
  return ePlaneId != AL_PLANE_MAX_ENUM && !AL_Plane_IsPixelPlane(ePlaneId);
}

static void AddPlane(AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES], int* iNbPlanes, AL_EPlaneId ePlaneType)
{
  usedPlanes[*iNbPlanes] = ePlaneType;
  (*iNbPlanes)++;
}

int AL_Plane_GetBufferPixelPlanes(AL_EChromaOrder eChromaOrder, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES])
{
  int iNbPlanes = 0;

  if(eChromaOrder == AL_C_ORDER_PACKED)
  {
    AddPlane(usedPlanes, &iNbPlanes, AL_PLANE_YUV);
    return iNbPlanes;
  }

  AddPlane(usedPlanes, &iNbPlanes, AL_PLANE_Y);

  if(eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    AddPlane(usedPlanes, &iNbPlanes, AL_PLANE_UV);
  else if(eChromaOrder != AL_C_ORDER_NO_CHROMA)
  {
    AddPlane(usedPlanes, &iNbPlanes, AL_PLANE_U);
    AddPlane(usedPlanes, &iNbPlanes, AL_PLANE_V);
  }

  return iNbPlanes;
}

static void AddBufferMapPlanes(AL_EChromaOrder eChromaOrder, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES], int* pNbPlanes)
{
  AddPlane(usedPlanes, pNbPlanes, AL_PLANE_MAP_Y);

  if(eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    AddPlane(usedPlanes, pNbPlanes, AL_PLANE_MAP_UV);
  else if(eChromaOrder != AL_C_ORDER_NO_CHROMA)
  {
    AddPlane(usedPlanes, pNbPlanes, AL_PLANE_MAP_U);
    AddPlane(usedPlanes, pNbPlanes, AL_PLANE_MAP_V);
  }
}

int AL_Plane_GetBufferMapPlanes(AL_EChromaOrder eChromaOrder, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES])
{
  int iNbPlanes = 0;
  AddBufferMapPlanes(eChromaOrder, usedPlanes, &iNbPlanes);
  return iNbPlanes;
}

int AL_Plane_GetBufferPlanes(AL_EChromaOrder eChromaOrder, bool bIsCompressed, AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES])
{
  int iNbPlanes = AL_Plane_GetBufferPixelPlanes(eChromaOrder, usedPlanes);

  if(bIsCompressed)
    AddBufferMapPlanes(eChromaOrder, usedPlanes, &iNbPlanes);
  return iNbPlanes;
}

bool AL_Plane_Exists(AL_EChromaOrder eChromaOrder, bool bIsCompressed, AL_EPlaneId ePlaneId)
{
  if(AL_Plane_IsMapPlane(ePlaneId) && !bIsCompressed)
    return false;
  switch(ePlaneId)
  {
  case AL_PLANE_Y:
  case AL_PLANE_MAP_Y:
    return true;
  case AL_PLANE_UV:
  case AL_PLANE_MAP_UV:
    return eChromaOrder == AL_C_ORDER_SEMIPLANAR;
  case AL_PLANE_U:
  case AL_PLANE_V:
  case AL_PLANE_MAP_U:
  case AL_PLANE_MAP_V:
    return eChromaOrder == AL_C_ORDER_U_V || eChromaOrder == AL_C_ORDER_V_U;
  case AL_PLANE_YUV:
    return (eChromaOrder == AL_C_ORDER_PACKED) && !bIsCompressed;
  default:
    return false;
  }

  return false;
}
