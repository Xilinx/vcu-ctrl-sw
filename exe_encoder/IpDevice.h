// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "exe_encoder/CfgParser.h"

extern "C"
{
#include "lib_encode/lib_encoder.h"
}

typedef struct AL_t_Allocator AL_TAllocator;
typedef struct AL_t_IpCtrl AL_TIpCtrl;
typedef struct AL_t_Timer AL_Timer;

/*****************************************************************************/
struct CIpDeviceParam
{
  int iDeviceType;
  int iSchedulerType;
  ConfigFile* pCfgFile;
  bool bTrackDma = false;
};

/*****************************************************************************/
static int constexpr NUM_SRC_SYNC_CHANNEL = 4;

class CIpDevice
{
public:
  CIpDevice() {};
  ~CIpDevice();

  void Configure(CIpDeviceParam& param);
  AL_IEncScheduler* GetScheduler();
  AL_TAllocator* GetAllocator();
  AL_Timer* GetTimer();

  CIpDevice(CIpDevice const &) = delete;
  CIpDevice & operator = (CIpDevice const &) = delete;

private:
  AL_IEncScheduler* m_pScheduler = nullptr;
  AL_TAllocator* m_pAllocator = nullptr;
  AL_Timer* m_pTimer = nullptr;

  void ConfigureMcu(CIpDeviceParam& param);
};

inline AL_IEncScheduler* CIpDevice::GetScheduler()
{
  return m_pScheduler;
}

inline AL_TAllocator* CIpDevice::GetAllocator()
{
  return m_pAllocator;
}

inline AL_Timer* CIpDevice::GetTimer()
{
  return m_pTimer;
}

