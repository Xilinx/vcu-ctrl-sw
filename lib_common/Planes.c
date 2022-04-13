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
