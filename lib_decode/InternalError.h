// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

typedef enum
{
  SUCCESS_ACCESS_UNIT,
  SUCCESS_NAL_UNIT,
  ERR_UNIT_NOT_FOUND,
  ERR_UNIT_INVALID_CHANNEL,
  ERR_UNIT_DYNAMIC_ALLOC,
  ERR_UNIT_FAILED,
  ERR_INVALID_ACCESS_UNIT,
  ERR_INVALID_NAL_UNIT,
}UNIT_ERROR;

