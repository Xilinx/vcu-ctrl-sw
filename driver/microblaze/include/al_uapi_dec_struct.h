// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef AL_UAPI_DEC_STRUCT_H
#define AL_UAPI_DEC_STRUCT_H

#include "al_uapi_codec_struct.h"

struct al5_channel_config {
	struct al5_params param;
	struct al5_channel_status status;
};

struct al5_scstatus {
	uint16_t num_sc;
	uint32_t num_bytes;
};

struct al5_decode_msg {
	struct al5_params params;
	struct al5_params addresses;
	uint32_t slice_param_v;
};

struct al5_decode_jpeg_msg {
	struct al5_params param;
	struct al5_params buffer_addrs;
};

struct al5_search_sc_msg {
	struct al5_params param;
	struct al5_params buffer_addrs;
};

#endif /* AL_UAPI_DEC_STRUCT_H */
