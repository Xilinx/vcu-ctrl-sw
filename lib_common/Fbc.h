// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"

bool HasSuperTile(AL_EFbStorageMode eMode);
int GetTileWidth(AL_EFbStorageMode eMode);
int GetTileHeight(AL_EFbStorageMode eMode);
int GetTileSize(AL_EFbStorageMode eMode, uint8_t uBitDepth);
