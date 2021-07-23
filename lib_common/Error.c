/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "lib_common/Error.h"

const char* AL_Codec_ErrorToString(AL_ERR eErrorCode)
{
  switch(eErrorCode)
  {
  /* Errors */
  case AL_ERR_NO_MEMORY: return "Memory shortage detected (DMA, embedded memory or virtual memory)";
  case AL_ERR_STREAM_OVERFLOW: return "Stream Error: Stream overflow";
  case AL_ERR_TOO_MANY_SLICES: return "Stream Error: Too many slices";
  case AL_ERR_WATCHDOG_TIMEOUT: return "Watchdog timeout";
  case AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE: return "Channel creation failed, no channel available";
  case AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE: return "Channel creation failed, processing power of the available cores insufficient";
  case AL_ERR_CHAN_CREATION_NOT_ENOUGH_CORES: return "Channel creation failed, couldn't spread the load on enough cores";
  case AL_ERR_REQUEST_MALFORMED: return "Channel creation failed, request was malformed";
  case AL_ERR_CMD_NOT_ALLOWED: return "Command is not allowed";
  case AL_ERR_INVALID_CMD_VALUE: return "Value associated with the command is invalid";

  /* Warnings */
  case AL_WARN_CONCEAL_DETECT: return "Decoder had to conceal some errors in the stream";
  case AL_WARN_LCU_OVERFLOW: return "Warning some LCU exceed the maximum allowed bits";
  case AL_WARN_NUM_SLICES_ADJUSTED: return "Warning num slices have been adjusted";
  case AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps not compatible with channel settings, decoder discarded it";
  case AL_WARN_SEI_OVERFLOW: return "Metadata buffer is too small to contains all sei messages";
  case AL_WARN_RES_FOUND_CB: return "resolutionFound Callback returned with error";

  /* Others */
  case AL_SUCCESS: return "Success";
  default: return "Unknown error";
  }
}
