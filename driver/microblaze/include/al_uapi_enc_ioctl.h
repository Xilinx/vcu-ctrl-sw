// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_ENC_IOCTL_H
#define AL_UAPI_ENC_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#include "al_uapi_enc_struct.h"
#include "al_uapi_codec_ioctl.h"

#define AL_MCU_CONFIG_CHANNEL _IOWR('q', 2, struct al5_config_channel)
#define AL_MCU_DESTROY_CHANNEL _IO('q', 4)
#define AL_MCU_ENCODE_ONE_FRM _IOWR('q', 5, struct al5_encode_msg)
#define AL_MCU_WAIT_FOR_STATUS _IOWR('q', 6, struct al5_params)
#define AL_MCU_PUT_STREAM_BUFFER _IOWR('q', 22, struct al5_buffer)
#define AL_MCU_GET_REC_PICTURE _IOWR('q', 23, struct al5_reconstructed_info)
#define AL_MCU_RELEASE_REC_PICTURE _IOWR('q', 24, int32_t)
#define AL_MCU_GET _IOWR('q', 27, struct al5_params)
#define AL_MCU_SET _IOWR('q', 28, struct al5_params)

#endif /* AL_UAPI_ENC_IOCTL_H */
