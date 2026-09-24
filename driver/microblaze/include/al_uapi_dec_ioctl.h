// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_DEC_IOCTL_H
#define AL_UAPI_DEC_IOCTL_H

#include <linux/ioctl.h>

#include "al_uapi_dec_struct.h"
#include "al_uapi_codec_ioctl.h"

#define AL_MCU_CONFIG_CHANNEL _IOWR('q', 2, struct al5_channel_config)
#define AL_MCU_DESTROY_CHANNEL _IO('q', 4)
#define AL_MCU_DECODE_ONE_FRM _IOWR('q', 5, struct al5_decode_msg)
#define AL_MCU_WAIT_FOR_STATUS _IOWR('q', 6, struct al5_params)
#define AL_MCU_SEARCH_START_CODE _IOWR('q', 8, struct al5_search_sc_msg)
#define AL_MCU_WAIT_FOR_START_CODE _IOWR('q', 9, struct al5_scstatus)
#define AL_MCU_DECODE_ONE_SLICE _IOWR('q', 18, struct al5_decode_msg)
#define AL_MCU_DECODE_JPEG _IOWR('q', 19, struct al5_decode_jpeg_msg)
#define AL_MCU_GET _IOWR('q', 20, struct al5_params)
#define AL_MCU_SET _IOWR('q', 21, struct al5_params)

#endif /* AL_UAPI_DEC_IOCTL_H */
