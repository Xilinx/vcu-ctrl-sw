// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <linux/types.h>
#include <linux/ioctl.h>
#include "al_uapi_struct.h"

#define AL_CMD_UNBLOCK_CHANNEL _IO('q', 1)

#define AL_CMD_IP_WRITE_REG _IOWR('q', 10, struct al5_reg)
#define AL_CMD_IP_READ_REG _IOWR('q', 11, struct al5_reg)
#define AL_CMD_IP_WAIT_IRQ _IOWR('q', 12, int32_t)
#define GET_DMA32_MMAP _IOWR('q', 26, struct al5_dma32_info)
#define GET_DMA32_FD _IOWR('q', 13, struct al5_dma32_info)
#define GET_DMA32_PHY _IOWR('q', 18, struct al5_dma32_info)
#define GET_DMA64_MMAP _IOWR('q', 126, struct al5_dma64_info)
#define GET_DMA64_FD _IOWR('q', 113, struct al5_dma64_info)
#define GET_DMA64_PHY _IOWR('q', 118, struct al5_dma64_info)
