// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/PlaneUtils.hpp"

std::vector<AL_TPlaneDescription> getPlaneDescription(TFourCC tFourCC, int32_t iPitchY, int32_t iPitchC, int32_t iPitchMap, size_t sizes[], int& iTotalOffset)
{
  std::vector<AL_TPlaneDescription> outputPlaneDescription;
  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(tFourCC, &tPicFormat);

  int32_t iNbPlanes = AL_Plane_GetBufferPlanes(tPicFormat, usedPlanes);
  int32_t offset = 0;

  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    AL_EPlaneId ePlaneId = usedPlanes[iPlane];

    int32_t planeIdx = 0;
    int32_t pitch = iPitchY;
    switch(ePlaneId)
    {
    case AL_PLANE_U:
      planeIdx = 1;
      pitch = iPitchC;
      break;

    case AL_PLANE_V:
      planeIdx = 2;
      pitch = iPitchC;
      break;

    case AL_PLANE_UV:
      planeIdx = 1;
      pitch = iPitchC;
      break;

    case AL_PLANE_YUV:
      planeIdx = 0;
      pitch = iPitchY;
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
