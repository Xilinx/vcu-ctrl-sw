/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "FileUtils.h"

std::string createFileNameWithID(const std::string& path, const std::string& motif, const std::string& extension, int iFrameID)
{
  std::ostringstream filename;
  filename << path << motif << "_" << iFrameID << extension;
  return filename.str();
}

/****************************************************************************/
int FromHex2(char a, char b)
{
  int A = ((a >= 'a') && (a <= 'f')) ? (a - 'a') + 10 :
          ((a >= 'A') && (a <= 'F')) ? (a - 'A') + 10 :
          ((a >= '0') && (a <= '9')) ? (a - '0') : 0;

  int B = ((b >= 'a') && (b <= 'f')) ? (b - 'a') + 10 :
          ((b >= 'A') && (b <= 'F')) ? (b - 'A') + 10 :
          ((b >= '0') && (b <= '9')) ? (b - '0') : 0;

  return (A << 4) + B;
}

/****************************************************************************/
int FromHex4(char a, char b, char c, char d)
{
  return (FromHex2(a, b) << 8) + FromHex2(c, d);
}

