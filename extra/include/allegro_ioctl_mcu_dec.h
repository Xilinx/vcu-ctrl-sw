// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef _AL_DEC_IOCTL_H_
#define _AL_DEC_IOCTL_H_
#include <linux/ioctl.h>
#include <linux/types.h>

#define AL_MCU_CONFIG_CHANNEL  _IOWR('q', 2, struct al5_channel_config)
#define AL_MCU_DESTROY_CHANNEL  _IO('q', 4)
#define AL_MCU_DECODE_ONE_FRM _IOWR('q', 5, struct al5_decode_msg)
#define AL_MCU_WAIT_FOR_STATUS _IOWR('q', 6, struct al5_params)
#define MAIL_TESTS _IO('q', 7)
#define AL_MCU_SEARCH_START_CODE _IOWR('q', 8, struct al5_search_sc_msg)
#define AL_MCU_WAIT_FOR_START_CODE _IOWR('q', 9, struct al5_scstatus)
#define GET_DMA_FD        _IOWR('q', 13, struct al5_dma_info)
#define AL_MCU_DECODE_ONE_SLICE _IOWR('q', 18, struct al5_decode_msg)
#define AL_MCU_GET _IOWR('q', 20, struct al5_params)
#define AL_MCU_SET _IOWR('q', 21, struct al5_params)

struct al5_dma_info
{
	__u32 fd;
	__u32 size;
	/* this should disappear when the last use of phy addr is removed from
	 * userspace code */
	__u32 phy_addr;
};

struct al5_channel_status
{
  __u32 error_code;
};

#define OPAQUE_SIZE 128

struct al5_params
{
	__u32 size;
	__u32 opaque[OPAQUE_SIZE];
};

struct al5_channel_config
{
	struct al5_params param;
	struct al5_channel_status status;
};

struct al5_scstatus
{
	__u16 num_sc;
	__u32 num_bytes;
};

struct al5_decode_msg
{
	struct al5_params params;
	struct al5_params addresses;
	__u32 slice_param_v;
};

struct al5_search_sc_msg
{
	struct al5_params param;
	struct al5_params buffer_addrs;
};

#endif	/* _AL_DEC_IOCTL_H_ */
