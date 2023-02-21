// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <thread>
#include <stdint.h>

inline uint64_t GetPerfTime()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

inline void Sleep(int ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
