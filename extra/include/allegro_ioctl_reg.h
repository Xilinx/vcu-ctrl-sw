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

#pragma once

#include <linux/types.h>

#define AL_CMD_UNBLOCK_CHANNEL _IO('q', 1)

#define AL_CMD_IP_WRITE_REG         _IOWR('q', 10, struct al5_reg)
#define AL_CMD_IP_READ_REG          _IOWR('q', 11, struct al5_reg)
#define AL_CMD_IP_WAIT_IRQ          _IOWR('q', 12, __s32)
#define GET_DMA32_FD                _IOWR('q', 13, struct al5_dma32_info)
#define GET_DMA32_MMAP              _IOWR('q', 26, struct al5_dma32_info)
#define GET_DMA32_PHY               _IOWR('q', 18, struct al5_dma32_info)
#define GET_DMA64_FD                _IOWR('q', 113, struct al5_dma64_info)
#define GET_DMA64_MMAP              _IOWR('q', 126, struct al5_dma64_info)
#define GET_DMA64_PHY               _IOWR('q', 118, struct al5_dma64_info)


struct al5_reg {
	__u32 id;
	__u32 value;
};

struct al5_dma32_info {
	__u32 fd;
	__u32 size;
	__u32 phy_addr;
};

struct al5_dma64_info {
	__u32 fd;
	__u32 size;
	__u64 phy_addr;
};

#define GET_DMA_FD GET_DMA32_FD
#define GET_DMA_MMAP GET_DMA32_MMAP
#define GET_DMA_PHY GET_DMA32_PHY
#define al5_dma_info al5_dma32_info
