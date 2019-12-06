/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
