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

#pragma once

#define MAX_REF 16 // max number of frame buffer

#define AL_MAX_COLUMNS_TILE 20 // warning : true only for main profile (4K = 10; 8K = 20)
#define AL_MAX_ROWS_TILE 22  // warning : true only for main profile (4K = 11; 8K = 22)

#define AL_MAX_NUM_TILE 440

#define AL_MAX_CTU_ROWS 528 // max line number : sqrt(MaxLumaSample size * 8) / 32. (4K = 264; 8K = 528);

#define AL_MAX_NUM_WPP AL_MAX_CTU_ROWS

#define AL_MAX_ENTRY_POINT (((AL_MAX_NUM_TILE) > (AL_MAX_NUM_WPP)) ? (AL_MAX_NUM_TILE) : (AL_MAX_NUM_WPP))

#define AL_MAX_SLICES_IN_PICT 1 // TO BE SIZED
