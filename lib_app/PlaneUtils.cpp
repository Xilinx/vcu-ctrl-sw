// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/PlaneUtils.h"

vector<AL_TPlaneDescription> getPlaneDescription(TFourCC tFourCC, int iPitch, int iPitchMap, size_t sizes[], int& iTotalOffset)
{
  vector<AL_TPlaneDescription> outputPlaneDescription;
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  AL_EChromaOrder eChromaOrderOutput = AL_GetChromaOrder(tFourCC);
  bool bComp = iPitchMap != 0 ? true : false;
  int iNbPlanes = AL_Plane_GetBufferPlanes(eChromaOrderOutput, bComp, usedPlanes);

  int offset = 0;

  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    AL_EPlaneId ePlaneId = usedPlanes[iPlane];

    int planeIdx = 0;
    int pitch = iPitch;
    switch(ePlaneId)
    {
    case AL_PLANE_U:
      planeIdx = 1;
      pitch = iPitch;
      break;

    case AL_PLANE_V:
      planeIdx = 2;
      pitch = iPitch;
      break;

    case AL_PLANE_UV:
      planeIdx = 1;
      pitch = iPitch;
      break;

    case AL_PLANE_MAP_Y:
      planeIdx = 3;
      pitch = iPitchMap;
      break;

    case AL_PLANE_MAP_U:
      planeIdx = 4;
      pitch = iPitchMap;
      break;

    case AL_PLANE_MAP_V:
      planeIdx = 5;
      pitch = iPitchMap;
      break;

    case AL_PLANE_MAP_UV:
      planeIdx = 4;
      pitch = iPitchMap;
      break;

    default:
      break;
    }

    outputPlaneDescription.push_back(AL_TPlaneDescription { ePlaneId, offset, pitch });
    offset += sizes[planeIdx];
  }

  iTotalOffset = offset;
  return outputPlaneDescription;
}
