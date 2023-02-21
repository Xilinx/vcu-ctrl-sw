// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

enum EConColor
{
  CC_BLACK,
  CC_RED,
  CC_GREEN,
  CC_YELLOW,
  CC_BLUE,
  CC_MAGENTA,
  CC_CYAN,
  CC_GREY,
  CC_DARK_RED,
  CC_DARK_GREEN,
  CC_DARK_YELLOW,
  CC_DARK_BLUE,
  CC_DARK_MAGENTA,
  CC_DARK_CYAN,
  CC_DARK_GREY,
  CC_WHITE,
  CC_DEFAULT,
};

void SetEnableColor(bool bColor);
void SetConsoleColor(EConColor eColor);
