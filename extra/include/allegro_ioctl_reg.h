/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include <linux/types.h>

#define AL_CMD_UNBLOCK_CHANNEL _IO('q', 1)

#define AL_CMD_IP_WRITE_REG         _IOWR('q', 10, struct al5_reg)
#define AL_CMD_IP_READ_REG          _IOWR('q', 11, struct al5_reg)
#define AL_CMD_IP_WAIT_IRQ          _IOWR('q', 12, __s32)
#define GET_DMA_FD                  _IOWR('q', 13, struct al5_dma_info)
#define GET_DMA_MMAP                _IOWR('q', 26, struct al5_dma_info)
#define GET_DMA_PHY                 _IOWR('q', 18, struct al5_dma_info)



struct al5_reg {
	__u32 id;
	__u32 value;
};

struct al5_dma_info {
	__u32 fd;
	__u32 size;
	__u32 phy_addr;
};

