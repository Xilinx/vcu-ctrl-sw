// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/console.h"

#include <unistd.h>
#include <cstdio>

static bool bUseColor = true;
static bool isAuto = true;

static int translateColor(EConColor col)
{
  switch(col)
  {
  case CC_BLACK: return 30;
  case CC_RED: return 91;
  case CC_GREEN: return 92;
  case CC_YELLOW: return 93;
  case CC_BLUE: return 94;
  case CC_MAGENTA: return 95;
  case CC_CYAN: return 96;
  case CC_GREY: return 97;
  case CC_DARK_RED: return 31;
  case CC_DARK_GREEN: return 32;
  case CC_DARK_YELLOW: return 33;
  case CC_DARK_BLUE: return 34;
  case CC_DARK_MAGENTA: return 35;
  case CC_DARK_CYAN: return 36;
  case CC_DARK_GREY: return 30;
  case CC_WHITE: return 37;
  case CC_DEFAULT:
  default:  return 39;
  }
}

void SetConsoleColor(EConColor eColor)
{
  if(isAuto)
    bUseColor = isatty(fileno(stdout));

  if(bUseColor)
    fprintf(stdout, "\033[%dm", translateColor(eColor));
}

void SetEnableColor(bool bColor)
{
  bUseColor = bColor;
  isAuto = false;
}

