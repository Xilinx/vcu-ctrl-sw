/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
    AL_SLICE_P, 0, 1, 4, -4, 0, 0, { -4, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 2, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 1, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 3, -1, 1, 0, { -1, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_5[] =
{
  {
    AL_SLICE_P, 0, 1, 6, -6, 0, 0, { -6, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 3, -3, 3, 0, { -3, 3, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 1, -1, 2, 0, { -1, 2, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 2, -2, 1, 0, { -2, 1, 4, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 4, -1, 2, 0, { -1, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 5, -2, 1, 0, { -2, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_7[] =
{
  {
    AL_SLICE_P, 0, 1, 8, -8, 0, 0, { -8, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 4, -4, 4, 0, { -4, 4, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 2, -2, 2, 0, { -2, 2, 6, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 1, -1, 1, 0, { -1, 1, 3, 7, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 3, -1, 1, 0, { -1, 1, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 6, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 5, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 7, -1, 1, 0, { -1, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_15[] =
{
  {
    AL_SLICE_P, 0, 1, 16, -16, 0, 0, { -16, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 8, -8, 8, 0, { -8, 8, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 4, -4, 4, 0, { -4, 4, 12, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 2, -2, 2, 0, { -2, 2, 6, 14, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 1, -1, 1, 0, { -1, 1, 3, 7, 15 }
  },
  {
    AL_SLICE_B, 4, 0, 3, -1, 1, 0, { -1, 1, 5, 13, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 6, -2, 2, 0, { -2, 2, 10, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 5, -1, 1, 0, { -1, 1, 3, 11, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 7, -1, 1, 0, { -1, 1, 9, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 12, -4, 4, 0, { -4, 4, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 10, -2, 2, 0, { -2, 2, 6, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 9, -1, 1, 0, { -1, 1, 3, 7, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 11, -1, 1, 0, { -1, 1, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 14, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 13, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 15, -1, 1, 0, { -1, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_3_AVC[] =
{
  {
    AL_SLICE_P, 0, 1, 4, -4, 0, 0, { -4, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 2, -2, 2, 0, { -2, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 1, -1, 1, 0, { -1, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 3, -3, 1, 0, { -3, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_5_AVC[] =
{
  {
    AL_SLICE_P, 0, 1, 6, -6, 0, 0, { -6, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 3, -3, 3, 0, { -3, 3, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 1, -1, 2, 0, { -1, 2, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 2, -2, 1, 0, { -2, 1, 4, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 4, -4, 2, 0, { -4, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 0, 5, -5, 1, 0, { -5, 1, 0, 0, 0 }
  }
};
static AL_TGopFrm PYRAMIDAL_GOP_7_AVC[] =
{
  {
    AL_SLICE_P, 0, 1, 8, -8, 0, 0, { -8, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 4, -4, 4, 0, { -4, 4, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 2, -2, 2, 0, { -2, 2, 6, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 1, -1, 1, 0, { -1, 1, 3, 7, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 3, -3, 1, 0, { -3, 1, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 6, -6, 2, 0, { -6, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 5, -5, 1, 0, { -5, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 0, 7, -7, 1, 0, { -7, 1, 0, 0, 0 }
  }
};

static AL_TGopFrm PYRAMIDAL_GOP_15_AVC[] =
{
  {
    AL_SLICE_P, 0, 1, 16, -16, 0, 0, { -16, 0, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 1, 1, 8, -8, 8, 0, { -8, 8, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 4, -4, 4, 0, { -4, 4, 12, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 2, -2, 2, 0, { -2, 2, 6, 14, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 1, -1, 1, 0, { -1, 1, 3, 7, 15 }
  },
  {
    AL_SLICE_B, 4, 0, 3, -3, 1, 0, { -3, 1, 5, 13, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 6, -6, 2, 0, { -6, 2, 10, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 5, -5, 1, 0, { -5, 1, 3, 11, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 7, -7, 1, 0, { -7, 1, 9, 0, 0 }
  },
  {
    AL_SLICE_B, 2, 1, 12, -12, 4, 0, { -12, 4, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 10, -10, 2, 0, { -10, 2, 6, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 9, -9, 1, 0, { -9, 1, 3, 7, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 11, -11, 1, 0, { -11, 1, 5, 0, 0 }
  },
  {
    AL_SLICE_B, 3, 1, 14, -14, 2, 0, { -14, 2, 0, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 13, -13, 1, 0, { -13, 1, 3, 0, 0 }
  },
  {
    AL_SLICE_B, 4, 0, 15, -15, 1, 0, { -15, 1, 0, 0, 0 }
  }
};

static inline AL_TGopFrm* getPyramidalFrames(int numB)
{
  switch(numB)
  {
  case 15:
    return PYRAMIDAL_GOP_15;
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

static inline AL_TGopFrm* getPyramidalFrames_AVC(int numB)
{
  switch(numB)
  {
  case 15:
    return PYRAMIDAL_GOP_15_AVC;
  case 7:
    return PYRAMIDAL_GOP_7_AVC;
  case 5:
    return PYRAMIDAL_GOP_5_AVC;
  case 3:
    return PYRAMIDAL_GOP_3_AVC;
  default:
    assert(false);
    return PYRAMIDAL_GOP_3_AVC;
  }
}

