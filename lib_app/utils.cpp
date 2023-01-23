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

#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <cstdarg>
#include <mutex>
#include "lib_app/utils.h"

using namespace std;

int g_Verbosity = 10;
static std::mutex s_LogMutex;

static void Message(EConColor Color, const char* sMsg, va_list args)
{
  std::lock_guard<std::mutex> guard(s_LogMutex);
  SetConsoleColor(Color);
  vprintf(sMsg, args);
  fflush(stdout);
  SetConsoleColor(CC_DEFAULT);
}

void LogError(const char* sMsg, ...)
{
  if(g_Verbosity < 1)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_RED, sMsg, args);
  va_end(args);
}

void LogWarning(const char* sMsg, ...)
{
  if(g_Verbosity < 3)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_YELLOW, sMsg, args);
  va_end(args);
}

void LogDimmedWarning(const char* sMsg, ...)
{
  if(g_Verbosity < 4)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_GREY, sMsg, args);
  va_end(args);
}

void LogInfo(const char* sMsg, ...)
{
  if(g_Verbosity < 5)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_DEFAULT, sMsg, args);
  va_end(args);
}

void LogInfo(EConColor Color, const char* sMsg, ...)
{
  if(g_Verbosity < 5)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(Color, sMsg, args);
  va_end(args);
}

void LogVerbose(const char* sMsg, ...)
{
  if(g_Verbosity < 7)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_DEFAULT, sMsg, args);
  va_end(args);
}

void LogVerbose(EConColor Color, const char* sMsg, ...)
{
  if(g_Verbosity < 7)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(Color, sMsg, args);
  va_end(args);
}

void LogDebug(const char* sMsg, ...)
{
  if(g_Verbosity < 20)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_DEFAULT, sMsg, args);
  va_end(args);
}

void OpenInput(std::ifstream& fp, std::string filename, bool binary)
{
  fp.open(filename, binary ? std::ios::binary : std::ios::in);
  fp.exceptions(ifstream::badbit);

  if(!fp.is_open())
    throw std::runtime_error("Can't open file for reading: '" + filename + "'");
}

void OpenOutput(std::ofstream& fp, std::string filename, bool binary)
{
  auto open_mode = binary ? std::ios::out | std::ios::binary : std::ios::out;

  fp.open(filename, open_mode);
  fp.exceptions(ofstream::badbit);

  if(!fp.is_open())
    throw std::runtime_error("Can't open file for writing: '" + filename + "'");
}

