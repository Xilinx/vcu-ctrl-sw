/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include <stdio.h>
#include "lib_fpga/Board.h"
#include "lib_common/Allocator.h"

AL_TIpCtrl* AL_Board_Create(const char* deviceFile, uint32_t uIntReg, uint32_t uMskReg, uint32_t uIntMask)
{
  (void)deviceFile;
  (void)uIntReg;
  (void)uMskReg;
  (void)uIntMask;
  fprintf(stderr, "No support for FPGA board on this platform\n");
  return NULL;
}

AL_TAllocator* AL_DmaAlloc_Create(const char* deviceFile)
{
  (void)deviceFile;
  fprintf(stderr, "No support for FPGA board on this platform\n");
  return NULL;
}

