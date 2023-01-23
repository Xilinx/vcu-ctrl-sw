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

extern "C"
{
#include "lib_common/PicFormat.h"
}
#include <fstream>
#include <stdexcept>
#include "lib_app/MD5.h"

static constexpr uint8_t CurrentCompFileVersion = 3;

struct TCrop
{
  int iLeft;
  int iRight;
  int iTop;
  int iBottom;
};

enum ETileMode : uint8_t
{
  TILE_64x4_v0 = 0,
  TILE_64x4_v1 = 1,
  TILE_32x4_v1 = 2,
  RASTER = 5,
  TILE_MAX_ENUM, /* sentinel */
};

ETileMode EFbStorageModeToETileMode(AL_EFbStorageMode eFbStorageMode);
AL_EFbStorageMode ETileModeToEFbStorageMode(ETileMode eTileMode);

static constexpr int SUPER_TILE_WIDTH = 4;
static constexpr int MEGA_MAP_WIDTH = 16;
static constexpr int SUPER_TILE_SIZE = SUPER_TILE_WIDTH * SUPER_TILE_WIDTH;

/****************************************************************************/

struct IWriter
{
  virtual void WriteBytes(const char* data, int size) = 0;
  virtual std::streampos tellp() = 0;
  virtual std::ostream& seekp(std::streampos pos) = 0;
  virtual bool IsOpen() = 0;
  virtual ~IWriter() = default;
};

/****************************************************************************/

class FileWriter : public IWriter
{
public:
  FileWriter(std::ofstream& pFileStream) : m_FileStream(pFileStream) {}
  virtual ~FileWriter() = default;
  void WriteBytes(const char* data, int size) override
  {
    if(!m_FileStream.write(data, size))
    {
      throw std::runtime_error("Error writing in file.");
    }
  }

  std::streampos tellp() override { return m_FileStream.tellp(); }

  std::ostream& seekp(std::streampos pos) override { return m_FileStream.seekp(pos); }

  bool IsOpen() override { return m_FileStream.is_open(); }

private:
  std::ofstream& m_FileStream;
};

/****************************************************************************/

class Md5Writer : public IWriter
{
public:
  Md5Writer() {}
  virtual ~Md5Writer() = default;
  void WriteBytes(const char* pData, int pSize) override { m_CMD5.Update((uint8_t*)pData, pSize); }

  /* Dummy method - no practical purpose in this class */
  std::streampos tellp() override { return 0; }

  /* Dummy method - no practical purpose in this class */
  std::ostream& seekp(std::streampos pos) override { return m_dummy.seekp(pos); }

  std::string GetMd5() { return m_CMD5.GetMD5(); }

  virtual bool IsOpen() override { return true; }

private:
  std::ofstream m_dummy;
  CMD5 m_CMD5;
};
