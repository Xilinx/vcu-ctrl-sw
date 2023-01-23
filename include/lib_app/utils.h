/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#pragma once
#include <fstream>
#include <memory>
#include "lib_app/console.h" // EConColor

extern int g_Verbosity;

void LogError(const char* sMsg, ...);
void LogWarning(const char* sMsg, ...);
void LogDimmedWarning(const char* sMsg, ...);
void LogInfo(const char* sMsg, ...);
void LogInfo(EConColor Color, const char* sMsg, ...);
void LogVerbose(const char* sMsg, ...);
void LogDebug(const char* sMsg, ...);
void LogVerbose(EConColor Color, const char* sMsg, ...);

void OpenInput(std::ifstream& fp, std::string filename, bool binary = true);
void OpenOutput(std::ofstream& fp, std::string filename, bool binary = true);

/*****************************************************************************/

template<typename Lambda>
class ScopeExitClass
{
public:
  ScopeExitClass(Lambda fn) : m_fn(fn)
  {
  }

  ~ScopeExitClass()
  {
    m_fn();
  }

private:
  Lambda m_fn;
};

template<typename Lambda>
ScopeExitClass<Lambda> scopeExit(Lambda fn)
{
  return ScopeExitClass<Lambda>(fn);
}

#if __cplusplus < 201402 && !defined(_MSC_VER) // has c++14 ? (Visual 2015 defines __cplusplus as "199711L" ...)
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args && ... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}

#endif

enum AL_EIpCtrlMode
{
  AL_IPCTRL_MODE_STANDARD,
  AL_IPCTRL_MODE_TIMERS,
  AL_IPCTRL_MODE_LOGS,
  AL_IPCTRL_MODE_TRACE, // codec-agnostic raw register r/w and irq dump
};

enum AL_ESchedulerType
{
  AL_SCHEDULER_TYPE_CPU,
  AL_SCHEDULER_TYPE_MCU,
};

enum AL_EDeviceType
{
  AL_DEVICE_TYPE_AUTO,
  AL_DEVICE_TYPE_BOARD,
  AL_DEVICE_TYPE_REFSW,
};
