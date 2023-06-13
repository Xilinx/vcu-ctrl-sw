// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

extern "C"
{
#include "lib_common/PicFormat.h"
}
#include <fstream>
#include <stdexcept>

static constexpr uint8_t CurrentCompFileVersion = 3;

enum ETileMode : uint8_t
{
  TILE_64x4_v0 = 0,
  TILE_64x4_v1 = 1,
  TILE_32x4_v1 = 2,
  RASTER = 5,
  TILE_MAX_ENUM,
};

static constexpr int SUPER_TILE_WIDTH = 4;
static constexpr int MEGA_MAP_WIDTH = 16;
static constexpr int SUPER_TILE_SIZE = SUPER_TILE_WIDTH * SUPER_TILE_WIDTH;

ETileMode EFbStorageModeToETileMode(AL_EFbStorageMode eFbStorageMode);
AL_EFbStorageMode ETileModeToEFbStorageMode(ETileMode eTileMode);

