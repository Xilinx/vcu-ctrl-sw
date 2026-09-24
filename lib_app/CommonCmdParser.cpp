// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <stdexcept>
#include <sstream>

#include "lib_app/CommonCmdParser.hpp"

/******************************************************************************/
AL_EFbStorageMode ParseFrameBufferFormat(const std::string& sBufFormat, bool& bBufComp)
{
  bBufComp = false;

  if(sBufFormat == "raster")
    return AL_FB_RASTER;

  throw std::runtime_error("Invalid buffer format");
}

/******************************************************************************/
std::string GetFrameBufferFormatOptDesc(bool bSecondOutput)
{
  std::string sFBufFormatOptDesc = "raster";

  if(!bSecondOutput)
  {

  }
  else
  {
  }

  return sFBufFormatOptDesc;
}

/******************************************************************************/
AL_TPosition ParsePosition(std::string const& s, int32_t iMultiple)
{
  AL_TPosition tPos = { 0, 0 };

  size_t separatorPos = s.find('x');

  if(separatorPos == std::string::npos)
    throw std::runtime_error("wrong position format");

  tPos.iX = atoi(s.substr(0, separatorPos - 0).c_str());
  tPos.iY = atoi(s.substr(separatorPos + 1).c_str());

  if(((tPos.iX % iMultiple) != 0) || ((tPos.iY % iMultiple) != 0))
    throw std::runtime_error("Position incorrect");

  return tPos;
}

/******************************************************************************/
AL_TDimension ParseDimension(std::string const& s, int32_t multiple)
{
  std::stringstream ss(s);
  ss.unsetf(std::ios::oct);
  ss.unsetf(std::ios::hex);

  char sep = 0;

  AL_TDimension d = { 0, 0 };
  ss >> d.iWidth;
  ss >> sep;
  ss >> d.iHeight;

  if(sep != 'x' || ss.fail() || ss.tellg() != std::streampos(-1))
    throw std::runtime_error("Wrong dimension format");

  if(((d.iWidth % multiple) != 0) || ((d.iHeight % multiple) != 0))
    throw std::runtime_error("Incorrect dimension");
  return d;
}
