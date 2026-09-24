// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_CODEC_IOCTL_H
#define AL_UAPI_CODEC_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>
#include "al_uapi_codec_struct.h"

#define GET_DMA32_FD _IOWR('q', 13, struct al5_dma32_info)
#define GET_DMA32_PHY _IOWR('q', 18, struct al5_dma32_info)

#endif /* AL_UAPI_CODEC_IOCTL_H */
