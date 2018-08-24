/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#pragma once
#include "lib_common_enc/EncChanParam.h"

static AL_TGopFrm PYRAMIDAL_GOP_3[] =
{
  {
    SLICE_P, 0, 1, 0, 4, -4, 0, 0, { -4, 0, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 1, 1, 2, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 1, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 3, -1, 1, 0, { -1, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_5[] =
{
  {
    SLICE_P, 0, 1, 0, 6, -6, 0, 0, { -6, 0, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 1, 1, 3, -3, 3, 0, { -3, 3, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 1, -1, 2, 0, { -1, 2, 5, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 2, -2, 1, 0, { -2, 1, 4, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 4, -1, 2, 0, { -1, 2, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 5, -2, 1, 0, { -2, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_7[] =
{
  {
    SLICE_P, 0, 1, 0, 8, -8, 0, 0, { -8, 0, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 1, 1, 4, -4, 4, 0, { -4, 4, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 1, 1, 2, -2, 2, 0, { -2, 2, 6, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 1, -1, 1, 0, { -1, 1, 3, 7, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 3, -1, 1, 0, { -1, 1, 5, 0, 0 }
  },
  {
    SLICE_B, 0, 1, 1, 6, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 5, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    SLICE_B, 0, 0, 2, 7, -1, 1, 0, { -1, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm* getPyramidalFrames(int numB)
{
  switch(numB)
  {
  case 7:
    return PYRAMIDAL_GOP_7;
  case 5:
    return PYRAMIDAL_GOP_5;
  case 3:
    return PYRAMIDAL_GOP_3;
  default:
    assert(false);
    return PYRAMIDAL_GOP_3;
  }
}

