// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <climits>
#include <cstdint>
#include <string>
#include <set>
#include "lib_app/utils.hpp"

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_decode/DecSettings.h"
}

/******************************************************************************/
enum EDecErrorLevel
{
  DEC_WARNING,
  DEC_ERROR,
};

/******************************************************************************/
static int32_t const zDefaultInputBufferSize = 32 * 1024;
static const int32_t OUTPUT_BD_FIRST = 0;
static const int32_t OUTPUT_BD_ALLOC = -1;
static const int32_t OUTPUT_BD_STREAM = -2;
static const int32_t SEI_NOT_ASSOCIATED_WITH_FRAME = -1;
static uint32_t constexpr uDefaultNumBuffersHeldByNextComponent = 1; /* We need at least 1 buffer to copy the output on a file */

static const int32_t DEFAULT_DEC_APB_ID = 2;

enum class EInputContainer
{
  Elementary,
  Heif
};

/******************************************************************************/
struct Config
{
  Config();

  bool help = false;

  std::string sIn;
  std::string sMainOut = ""; // Output rec file
  std::string sCrc;

  AL_TDecSettings tDecSettings {};
  EInputContainer eContainer = EInputContainer::Elementary; /*!< Specify if stream is muxed (only HEIC-based containers(AVIF, HEIF) supported) */
  AL_TDecOutputSettings tUserOutputSettings {};
  bool bEnableCrop = false;

  AL_EDeviceType eDeviceType = AL_EDeviceType::AL_DEVICE_TYPE_BOARD;
  AL_ESchedulerType eSchedulerType = AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU;
  bool bSelectDeviceWithLowestAvailableResources = false;
  int32_t iOutputBitDepth = OUTPUT_BD_ALLOC;
  bool bEnableYUVOutput = true;
  TFourCC tOutputFourCC = FOURCC(NULL);
  int32_t iTraceIdx = -1;
  int32_t iTraceNumber = 0;
  bool bOutputTraceOnly = false;
  bool bForceCleanBuffers = false;
  uint32_t uInputBufferNum = 2;
  size_t zInputBufferSize = zDefaultInputBufferSize;
  AL_EIpCtrlMode ipCtrlMode = AL_EIpCtrlMode::AL_IPCTRL_MODE_STANDARD;
  std::string md5File = "";
  std::string apbFile = "";
  std::string sSplitSizesFile = "";
  AL_ETrackDmaMode eTrackDmaMode = AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE;
  int32_t iLoop = 1;
  bool bMultiChunk = false;
  bool bCertCRC = false;
  std::set<std::string> sDecDevicePath;
  AL_64S iTimeoutInSeconds = -1;
  AL_64S iMaxFrames = INT64_MAX;
  std::string seiFile = "";
  std::string hdrFile = "";
  bool bUsePreAlloc = false;

  bool UseBaseDecoder() const { return true; }

  EDecErrorLevel eExitCondition = DEC_ERROR;
};

/******************************************************************************/
Config ParseCommandLine(int32_t argc, char* argv[]);
AL_EFbStorageMode GetMainOutputStorageMode(AL_TDecOutputSettings tUserOutputSettings, AL_EFbStorageMode eOutstorageMode);
bool IsOutputStorageModeCompressed(AL_TDecOutputSettings tUserOutputSettings, bool bMainOutputCompressed);
