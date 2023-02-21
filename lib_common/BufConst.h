// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#define MAX_REF 16 // max number of frame buffer

#define AL_MAX_COLUMNS_TILE 20 // warning : true only for main profile (4K = 10; 8K = 20)
#define AL_MAX_ROWS_TILE 22  // warning : true only for main profile (4K = 11; 8K = 22)

#define AL_MAX_NUM_TILE 440

#define AL_MAX_CTU_ROWS 528 // max line number : sqrt(MaxLumaSample size * 8) / 32. (4K = 264; 8K = 528);

#define AL_MAX_NUM_WPP AL_MAX_CTU_ROWS

#define AL_MAX_ENTRY_POINT (((AL_MAX_NUM_TILE) > (AL_MAX_NUM_WPP)) ? (AL_MAX_NUM_TILE) : (AL_MAX_NUM_WPP))

#define AL_MAX_SLICES_IN_PICT 1 // TO BE SIZED
