// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

