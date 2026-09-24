// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
  case AL_ERR_CHAN_CREATION_HW_CAPACITY_EXCEEDED: return "Channel creation failed, processing power of the hardware is insufficient";
  case AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE: return "Channel creation failed, processing power currently available is insufficient";
  case AL_ERR_CHAN_CREATION_LOAD_DISTRIBUTION: return "Channel creation failed, couldn't spread the load on cores";
  case AL_ERR_REQUEST_MALFORMED: return "Channel creation failed, request was malformed";
  case AL_ERR_CMD_NOT_ALLOWED: return "Command is not allowed";
  case AL_ERR_CHAN_CREATION_MIX_REALTIME: return "Cannot mix realtime and non-realtime channel";
  case AL_ERR_CANNOT_OPEN_FILE: return "Cannot Open file";
  case AL_ERR_ROI_DISABLE: return "Regions Of Interest (ROI) are disabled";
  case AL_ERR_QPLOAD_DATA: return "Error in QP file data.";
  case AL_ERR_QPLOAD_NOT_ENOUGH_DATA: return "Not enough data in the provided QP table.";
  case AL_ERR_QPLOAD_SEGMENT_CONFIG: return "Some segments are configured with an invalid QP in the provided QP table.";
  case AL_ERR_QPLOAD_QP_VALUE: return "Invalid QPs configured in the provided QP table.";
  case AL_ERR_QPLOAD_SEGMENT_INDEX: return "Invalid segment indexes found in the provided QP table.";
  case AL_ERR_QPLOAD_FORCE_FLAGS: return "Some coding units are configured with both force-intra and force-mv0 flags or with both force-intra and DisableIntra in the provided QP table.";
  case AL_ERR_QPLOAD_BLK_SIZE: return "Some LCU are configured with invalid min/max block sizes in the provided QP table.";
  case AL_ERR_INVALID_OR_UNSUPPORTED_FILE_FORMAT: return "Invalid or unsupported file format";
  case AL_ERR_REQUEST_INVALID_MIN_WIDTH: return "Minimal frame width not reached (2x LCU size)";
  case AL_ERR_REQUEST_INVALID_MAX_HEIGHT: return "Maximal frame height exceeded";
  case AL_ERR_INVALID_CMD_VALUE: return "Value associated with the command is invalid";

  case AL_ERR_CORRUPTED_STREAM: return "Stream is corrupted";
  case AL_ERR_CORRUPTED_QUANTIZATION_MATRIX: return "Quantization matrix is corrupted";
  case AL_ERR_BUFFER_TOO_SMALL: return "Buffer is too small";
  case AL_ERR_ALIGNMENT: return "Alignment constraint not respected";
  case AL_ERR_BUFFER_STACK_FULL: return "Too many buffers provided, stack is full";
  case AL_ERR_INVALID_NEURAL_VIDEO_PROCESSOR_LOADABLE: return "Loadable for Neural Video Processor is invalid. The loadable format might not be handled, the targeted hardware might be wrong, the buffer can corrupted...";
  case AL_ERR_AXI_BUS_ERROR: return "AXI bus error occurred so the current encoding/decoding operation on HW has been reset for all current channels";

  /* Warnings */
  case AL_WARN_CONCEAL_DETECT: return "Decoder had to conceal some errors in the stream";
  case AL_WARN_UNSUPPORTED_NAL: return "Some NALs have unsupported features, decoder discarded it";
  case AL_WARN_LCU_OVERFLOW: return "Warning some LCU exceed the maximum allowed bits";
  case AL_WARN_NUM_SLICES_ADJUSTED: return "Warning num slices have been adjusted";
  case AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps not compatible with channel settings, decoder discarded it";
  case AL_WARN_SEI_OVERFLOW: return "Metadata buffer is too small to contains all sei messages";
  case AL_WARN_RES_FOUND_CB: return "resolutionFound Callback returned with error";
  case AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps bitdepth not compatible with channel settings, decoder discarded it";
  case AL_WARN_SPS_LEVEL_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps level not compatible with channel settings, decoder discarded it";
  case AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps chroma mode not compatible with channel settings, decoder discarded it";
  case AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps sequence mode (progressive/interlaced) mode not compatible with channel settings, decoder discarded it";
  case AL_WARN_SPS_MIN_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps resolution not compatible with minimal resolution, decoder discarded it";
  case AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS: return "Sps resolution not compatible with channel settings, decoder discarded it";
  case AL_WARN_ASO_FMO_NOT_SUPPORTED: return "Arbitrary Slice Order (ASO) or Flexible Macroblock Reordering (FMO) features are not supported, decoder discarded it";
  case AL_WARN_INVALID_ACCESS_UNIT_STRUCTURE: return "The NALs present in the stream buffer don't form a valid Access Unit structure";
  case AL_WARN_HW_CONCEAL_DETECT: return "Decoder hardware had to conceal some errors in the stream";
  case AL_WARN_UNEXPECTED_SLICE_IN_INTRA_PROFILE: return "One or more P or B slices have been detected in Intra only profile, decoder skipped them";
  case AL_WARN_PPS_TILEMAP_NOT_COMPATIBLE_WITH_DECODING_MODE: return "one or more pictures have too many tile columns for the selected decoding mode (coupled or low latency)";

  /* Others */
  case AL_SUCCESS: return "Success";
  default: return "Unknown error";
  }
}
