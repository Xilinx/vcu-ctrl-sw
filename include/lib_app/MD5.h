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

#include "lib_rtos/types.h"
#include "lib_app/utils.h"
#include <string>
#include <fstream>
#include <iostream>

class CMD5
{
public:
  CMD5();

  void Update(uint8_t* pBuffer, uint32_t uSize);
  std::string GetMD5();

protected:
  void UpdateBlock(uint32_t* pBlock);

  union
  {
    uint32_t m_pHash32[4];
    uint8_t m_pHash8[16];
  };

  AL_64U m_uNumBytes;

  uint8_t m_pBound[64]; // 512 bits
  uint32_t m_uBound;
};

struct Md5Calculator
{
  Md5Calculator(const std::string& path)
  {
    if(!path.empty())
    {
      if(path == "stdout")
        m_pMd5Out = &std::cout;
      else
      {
        OpenOutput(m_Md5File, path);
        m_pMd5Out = &m_Md5File;
      }
    }
  }

  bool IsFileOpen()
  {
    return m_Md5File.is_open();
  }

  void Md5Output()
  {
    auto const sMD5 = m_MD5.GetMD5();
    * m_pMd5Out << sMD5 << std::endl;
  }

  CMD5& GetCMD5()
  {
    return m_MD5;
  }

protected:
  std::ofstream m_Md5File;
  std::ostream* m_pMd5Out;
  CMD5 m_MD5;
};
