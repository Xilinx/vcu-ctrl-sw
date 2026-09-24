// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include <functional>
#include <set>
#include <string>
#include <array>
#include <memory>
#include <vector>
#include "lib_app/utils.hpp"
#include "IpDeviceCommon.hpp"

extern "C"
{
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/DecChanParam.h"
#include "lib_common/I_Communication.h"
}

typedef struct AL_IDecScheduler AL_IDecScheduler;

/*****************************************************************************/
class CIpDevice : public I_IpDevice
{
public:
  CIpDevice(CIpDeviceParam const& param, AL_EDeviceType eDeviceType, std::set<std::string> tDevices);
  ~CIpDevice();

  AL_EDeviceType GetDeviceType();
  void* GetScheduler() override;
  AL_TAllocator* GetAllocator() override;
  AL_ITimer* GetTimer() override;

  CIpDevice(CIpDevice const &) = delete;
  CIpDevice & operator = (CIpDevice const &) = delete;

  void SelectNextDevice();
  bool HandleDeviceFailure();
  bool IsDeviceFailed(std::string const& device);

private:
  std::set<std::string> const m_tDevices;
  std::vector<std::string> m_tSelectedDevices;
  AL_EDeviceType m_eDeviceType;
  AL_IDecScheduler* m_pScheduler = nullptr;
  std::shared_ptr<AL_TAllocator> m_pAllocator = nullptr;
  AL_ITimer* m_pTimer = nullptr;
  std::set<std::string> m_FailedDevices;
  bool m_bSelectDeviceWithLowestAvailableResources;
  uint32_t m_numDevices;
  std::shared_ptr<AL_ICommunication> m_pCommunication = nullptr;

  void ConfigureMcu(std::shared_ptr<AL_ICommunication> pCommunication, bool useProxy);
  std::string SelectMcuDevice(std::set<std::string> const& tDevices);
};

inline void* CIpDevice::GetScheduler(void)
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

