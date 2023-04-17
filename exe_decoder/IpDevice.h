// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include <functional>
#include "lib_app/utils.h"

extern "C"
{
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecChanParam.h"
}

typedef struct AL_t_Allocator AL_TAllocator;
typedef struct AL_i_DecScheduler AL_IDecScheduler;
typedef struct AL_t_IpCtrl AL_TIpCtrl;
typedef struct AL_t_Timer AL_Timer;
typedef struct AL_t_driver AL_TDriver;

/*****************************************************************************/
struct CIpDeviceParam
{
  int iDeviceType;
  int iSchedulerType;
  bool bTrackDma = false;
  uint8_t uNumCore = 0;
  int iHangers = 0;
  AL_EIpCtrlMode ipCtrlMode;
  std::string apbFile;
};

/*****************************************************************************/
class CIpDevice
{
public:
  CIpDevice() {};
  ~CIpDevice();

  void Configure(CIpDeviceParam& param);
  AL_IDecScheduler* GetScheduler();
  AL_TAllocator* GetAllocator();
  AL_Timer* GetTimer();

  CIpDevice(CIpDevice const &) = delete;
  CIpDevice & operator = (CIpDevice const &) = delete;

private:
  AL_IDecScheduler* m_pScheduler = nullptr;
  AL_TAllocator* m_pAllocator = nullptr;
  AL_Timer* m_pTimer = nullptr;

  void ConfigureMcu(AL_TDriver* driver, bool useProxy);
};

inline AL_IDecScheduler* CIpDevice::GetScheduler()
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

