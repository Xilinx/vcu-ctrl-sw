// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <map>
#include <stdexcept>

extern "C"
{
#include "lib_common/PicFormat.h"
#include "lib_common/FourCC.h"
#include "lib_common/AllocatorTracker.h"
}

static const std::map<std::string, AL_EChromaMode> HandledChromaMode =
{
  { "400", AL_CHROMA_4_0_0 },
  { "420", AL_CHROMA_4_2_0 },
  { "422", AL_CHROMA_4_2_2 },
  { "444", AL_CHROMA_4_4_4 },
};

static const std::map<std::string, AL_EFbStorageMode> HandledStorageModes =
{
  { "raster", AL_FB_RASTER },
  { "tile32x4", AL_FB_TILE_32x4 },
  { "tile64x4", AL_FB_TILE_64x4 },
};

/******************************************************************************/

template<typename T>
std::string GetHandledValuesList(const std::map<std::string, T> HandledValues)
{
  std::string sHandledValuesList;

  for(const auto& bdMode : HandledValues)
    sHandledValuesList += std::string("'") + bdMode.first + std::string("' ");

  return sHandledValuesList;
}

template<typename T>
T ParseHandledValue(std::string s, const std::map<std::string, T> HandledValues, T defaultValue)
{
  if(s.empty())
  {
    return defaultValue;
  }

  auto chosenValue = HandledValues.find(s);

  if(chosenValue != HandledValues.end())
    return chosenValue->second;

  throw std::runtime_error(std::string("Wrong value. Allowed values are: ") + GetHandledValuesList(HandledValues));
}

AL_EFbStorageMode ParseFrameBufferFormat(const std::string& sBufFormat, bool& bBufComp);
std::string GetFrameBufferFormatOptDesc(bool bSecondOutput = false);
AL_TPosition ParsePosition(std::string const& s, int32_t iMultiple);
AL_TDimension ParseDimension(std::string const& s, int32_t multiple);
