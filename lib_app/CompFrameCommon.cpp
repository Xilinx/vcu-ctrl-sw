// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <stdexcept>
#include "lib_app/CompFrameCommon.hpp"

ETileMode EFbStorageModeToETileMode(AL_EFbStorageMode eFbStorageMode, uint8_t uBitDepth)
{
  (void)uBitDepth;
  switch(eFbStorageMode)
  {
  case AL_FB_TILE_32x4: return TILE_32x4_v1;
  case AL_FB_TILE_64x4: return TILE_64x4_v1;
  case AL_FB_RASTER: return RASTER;

  default: throw std::runtime_error("Unsupported eFbStorageMode");
  }

  throw std::runtime_error("Unsupported eFbStorageMode");
  return TILE_MAX_ENUM;
}

AL_EFbStorageMode ETileModeToEFbStorageMode(ETileMode eTileMode)
{
  switch(eTileMode)
  {
  case TILE_32x4_v1: return AL_FB_TILE_32x4;
  case TILE_64x4_v0:
  case TILE_64x4_v1: return AL_FB_TILE_64x4;
  case RASTER: return AL_FB_RASTER;

  default:;
  }

  throw std::runtime_error("Unsupported eTileMode");
  return AL_FB_MAX_ENUM;
}
