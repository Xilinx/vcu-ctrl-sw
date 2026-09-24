// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_CODEC_STRUCT_H
#define AL_UAPI_CODEC_STRUCT_H

struct al5_dma32_info {
	int32_t fd;
	uint32_t size;
	uint32_t phy_addr;
};

#define OPAQUE_SIZE 256

struct al5_params {
	uint32_t size;
	uint32_t opaque[OPAQUE_SIZE];
};

struct al5_channel_status {
	uint32_t error_code;
};

#endif /* AL_UAPI_CODEC_STRUCT_H */
