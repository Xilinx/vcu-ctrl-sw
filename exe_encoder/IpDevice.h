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

#pragma once

#include "lib_app/InputFiles.h"
#include "lib_app/utils.h"
#include "exe_encoder/CfgParser.h"

extern "C"
{
#include "lib_common_enc/Settings.h"
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
  int iVqDescr = 0;
};

/*****************************************************************************/
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

