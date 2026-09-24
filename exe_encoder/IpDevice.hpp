// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "exe_encoder/CfgParser.hpp"
#include "lib_app/utils.hpp"

#include <string>

extern "C"
{
#include "lib_encode/lib_encode.h"
#include "lib_log/I_Logger.h"
#include "lib_log/I_Timer.h"
#include "lib_common/I_Communication.h"
}

typedef struct AL_TAllocator AL_TAllocator;
typedef struct AL_TIpCtrl AL_TIpCtrl;

/*****************************************************************************/
struct CIpDeviceParam
{
  AL_EDeviceType eDeviceType;
  AL_ESchedulerType eSchedulerType;
  ConfigFile* pCfgFile;
  AL_ETrackDmaMode eTrackDmaMode = AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE;
};

/*****************************************************************************/
static int32_t constexpr NUM_SRC_SYNC_CHANNEL = 4;

class CIpDevice
{
public:
  CIpDevice() = default;
  ~CIpDevice();

  void Configure(CIpDeviceParam& param);
  AL_IEncScheduler* GetScheduler();
  AL_TAllocator* GetAllocator();
  AL_ITimer* GetTimer();

  CIpDevice(CIpDevice const &) = delete;
  CIpDevice & operator = (CIpDevice const &) = delete;

private:
  AL_IEncScheduler* m_pScheduler = nullptr;
  std::shared_ptr<AL_TAllocator> m_pAllocator = nullptr;
  AL_ITimer* m_pTimer = nullptr;

  std::shared_ptr<AL_ICommunication> m_pCommunication = nullptr;
  void ConfigureMcu(CIpDeviceParam& param);
};

inline AL_IEncScheduler* CIpDevice::GetScheduler(void)
{
  return m_pScheduler;
}

inline AL_TAllocator* CIpDevice::GetAllocator(void)
{
  return m_pAllocator.get();
}

inline AL_ITimer* CIpDevice::GetTimer(void)
{
  return m_pTimer;
}

