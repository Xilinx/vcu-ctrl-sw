/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

