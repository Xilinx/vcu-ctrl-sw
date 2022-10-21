/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

/**************************************************************************//*!
   \addtogroup Errors

   This regroups all the errors and warnings that can be launched
   from the decoder or the encoder libraries.

   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"

#define AL_DEF_WARNING(N) ((AL_ERR)(0x00 + (N)))
#define AL_DEF_ERROR(N) ((AL_ERR)(0x80 + (N)))

enum
{
  /*! The decoder had to conceal some errors in the stream */
  AL_WARN_CONCEAL_DETECT = AL_DEF_WARNING(1),
  /*! Nal has parameters unsupported by decoder */
  AL_WARN_UNSUPPORTED_NAL = AL_DEF_WARNING(2),
  /*! Some LCU exceed the maximum allowed bits in the stream */
  AL_WARN_LCU_OVERFLOW = AL_DEF_WARNING(3),
  /*! Number of slices have been adjusted to be hardware compatible */
  AL_WARN_NUM_SLICES_ADJUSTED = AL_DEF_WARNING(4),
  /*! Sps not compatible with channel settings, decoder discards it */
  AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS = AL_DEF_WARNING(5),
  /*! Sei metadata buffer is too small to contains all sei messages */
  AL_WARN_SEI_OVERFLOW = AL_DEF_WARNING(6),
  /*! The resolutionFound Callback returns with error */
  AL_WARN_RES_FOUND_CB = AL_DEF_WARNING(7),
};

enum
{
  /*! The operation succeeded without encountering any error */
  AL_SUCCESS = ((AL_ERR)0x00000000),
  /*! Unknown error */
  AL_ERROR = AL_DEF_ERROR(0),
  /*! Couldn't allocate a resource because no memory was left
   * This can be dma memory, mcu specific memory if available or
   * simply virtual memory shortage */
  AL_ERR_NO_MEMORY = AL_DEF_ERROR(7),
  /*! The generated stream couldn't fit inside the allocated stream buffer */
  AL_ERR_STREAM_OVERFLOW = AL_DEF_ERROR(8),
  /*! If SliceSize mode is supported, the constraint couldn't be respected
   * as too many slices were required to respect it */
  AL_ERR_TOO_MANY_SLICES = AL_DEF_ERROR(9),
  /*! A timeout occurred while processing the request */
  AL_ERR_WATCHDOG_TIMEOUT = AL_DEF_ERROR(11),
  /*! The scheduler can't handle more channel (fixed limit of AL_SCHEDULER_MAX_CHANNEL) */
  AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE = AL_DEF_ERROR(13),
  /*! The processing power of the available cores is insufficient to handle this channel */
  AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE = AL_DEF_ERROR(14),
  /*! Couldn't spread the load on enough cores (a special case of ERROR_RESOURCE_UNAVAILABLE)
   * or the load can't be spread so much (each core has a requirement on the minimal number
   * of resources it can handle) */
  AL_ERR_CHAN_CREATION_LOAD_DISTRIBUTION = AL_DEF_ERROR(15),
  /*! Some parameters in the request have an invalid value */
  AL_ERR_REQUEST_MALFORMED = AL_DEF_ERROR(16),
  /*! The command is not allowed in some configuration */
  AL_ERR_CMD_NOT_ALLOWED = AL_DEF_ERROR(17),
  /*! The value associated with the command is invalid (in the current configuration) */
  AL_ERR_INVALID_CMD_VALUE = AL_DEF_ERROR(18),
  /*! Couldn't mix realtime and non-realtime channels at the sema time */
  AL_ERR_CHAN_CREATION_MIX_REALTIME = AL_DEF_ERROR(20),
  /*! The file open failed */
  AL_ERR_CANNOT_OPEN_FILE = AL_DEF_ERROR(21),
  /*! ROI disabled */
  AL_ERR_ROI_DISABLE = AL_DEF_ERROR(22),
  /*! There are some issues in the QP file */
  AL_ERR_QPLOAD_DATA = AL_DEF_ERROR(23),
  AL_ERR_QPLOAD_NOT_ENOUGH_DATA = AL_DEF_ERROR(24),
  AL_ERR_QPLOAD_SEGMENT_CONFIG = AL_DEF_ERROR(25),
  AL_ERR_QPLOAD_QP_VALUE = AL_DEF_ERROR(26),
  AL_ERR_QPLOAD_SEGMENT_INDEX = AL_DEF_ERROR(27),
  AL_ERR_QPLOAD_FORCE_FLAGS = AL_DEF_ERROR(28),
  AL_ERR_QPLOAD_BLK_SIZE = AL_DEF_ERROR(29),
  /*! invalid or unsupported file format */
  AL_ERR_INVALID_OR_UNSUPPORTED_FILE_FORMAT = AL_DEF_ERROR(30),
  /*! minimal frame width for HW is not encountered */
  AL_ERR_REQUEST_INVALID_MIN_WIDTH = AL_DEF_ERROR(31),
  /*! maximal frame height for HW is excedeed */
  AL_ERR_REQUEST_INVALID_MAX_HEIGHT = AL_DEF_ERROR(32),
  /*! HW capacity is excedeed */
  AL_ERR_CHAN_CREATION_HW_CAPACITY_EXCEEDED = AL_DEF_ERROR(33),
};

static inline bool AL_IS_ERROR_CODE(AL_ERR eErrorCode)
{
  return eErrorCode >= AL_ERROR;
}

static inline bool AL_IS_WARNING_CODE(AL_ERR eErrorCode)
{
  return (eErrorCode != AL_SUCCESS) && (eErrorCode < AL_ERROR);
}

static inline bool AL_IS_SUCCESS_CODE(AL_ERR eErrorCode)
{
  return eErrorCode == AL_SUCCESS;
}

/**************************************************************************//*!
   \brief Get a string corresponding to an error/warning code
   \param[in] eErrorCode The error code to get a string description from
   \return a string describing the error
******************************************************************************/
const char* AL_Codec_ErrorToString(AL_ERR eErrorCode);

/*@}*/

