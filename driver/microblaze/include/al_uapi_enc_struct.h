// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_ENC_STRUCT_H
#define AL_UAPI_ENC_STRUCT_H

#include "al_uapi_codec_struct.h"

struct al5_reconstructed_info {
	int32_t fd;
	uint32_t pic_struct;
	uint32_t poc;
	uint32_t width;
	uint32_t height;
};

struct al5_config_channel {
	struct al5_params param;
	struct al5_channel_status status;
	int32_t rc_plugin_fd;
};

struct al5_encode_msg {
	struct al5_params params;
	struct al5_params addresses;
};

struct al5_stream_buffer {
	uint64_t stream_buffer_ptr;
	int32_t handle;
	uint32_t offset;
	uint32_t size;
};

struct al5_buffer {
	struct al5_stream_buffer stream_buffer;
	int32_t external_mv_handle;
};

#endif /* AL_UAPI_ENC_STRUCT_H */
