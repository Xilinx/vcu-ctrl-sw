// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#ifndef _AL_ENC_IOCTL_H_
#define _AL_ENC_IOCTL_H_
#include <linux/ioctl.h>
#include <linux/types.h>

#define AL_MCU_CONFIG_CHANNEL  _IOWR('q', 2, struct al5_channel_config)
#define AL_MCU_DESTROY_CHANNEL  _IO('q', 4)
#define AL_MCU_ENCODE_ONE_FRM _IOWR('q', 5, struct al5_encode_msg)
#define AL_MCU_WAIT_FOR_STATUS _IOWR('q', 6, struct al5_params)
#define GET_DMA_FD        _IOWR('q', 13, struct al5_dma_info)
#define MAIL_TESTS _IO('q', 7)
#define AL_MCU_SET_TIMER_BUFFER _IOWR('q', 16, struct al5_dma_info)

#define AL_MCU_PUT_STREAM_BUFFER _IOWR('q', 22, struct al5_buffer)

#define AL_MCU_GET_REC_PICTURE _IOWR('q', 23, struct al5_reconstructed_info)
#define AL_MCU_RELEASE_REC_PICTURE _IOWR('q', 24, __u32)

struct al5_reconstructed_info
{
	__u32 fd;
	__u32 pic_struct;
	__u32 poc;
	__u32 width;
	__u32 height;
};

struct al5_dma_info
{
	__u32 fd;
	__u32 size;
	/* this should disappear when the last use of phy addr is removed from
	 * userspace code */
	__u32 phy_addr;
};

struct al5_params
{
	__u32 size;
	__u32 opaque_params[128];
};

struct al5_channel_status
{
	__u32 error_code;
};

struct al5_channel_config
{
	struct al5_params param;
	struct al5_channel_status status;
	__u32 rc_plugin_fd;
};

struct al5_encode_msg {
	struct al5_params params;
	struct al5_params addresses;
};

struct al5_stream_buffer {
  __u64 stream_buffer_ptr;
  __u32 handle;
  __u32 offset;
  __u32 size;
};

struct al5_buffer {
  struct al5_stream_buffer stream_buffer;
  __u32 external_mv_handle;
};

#endif	/* _AL_ENC_IOCTL_H_ */
