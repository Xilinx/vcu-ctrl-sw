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

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "FileUtils.h"

static const char CurrentDirectory = '.';
static const char PathSeparator = '/';
static const char WinPathSeparator = '\\';

/****************************************************************************/
void formatFolderPath(std::string& folderPath)
{
  if(folderPath.empty())
  {
    folderPath += CurrentDirectory;
  }

  if(folderPath[folderPath.size() - 1] != PathSeparator && folderPath[folderPath.size() - 1] != WinPathSeparator)
  {
    folderPath += PathSeparator;
  }
}

/****************************************************************************/
std::string combinePath(const std::string& folder, const std::string& filename)
{
  std::string formatedFolderPath = folder;
  formatFolderPath(formatedFolderPath);
  return formatedFolderPath + filename;
}

/****************************************************************************/
std::string createFileNameWithID(const std::string& path, const std::string& motif, const std::string& extension, int iFrameID)
{
  std::ostringstream filename;
  filename << motif << "_" << iFrameID << extension;
  return combinePath(path, filename.str());
}

/****************************************************************************/
static int FromHex1(char a)
{
  int A = FROM_HEX_ERROR;

  if((a >= 'a') && (a <= 'f'))
    A = (a - 'a') + 10;
  else if((a >= 'A') && (a <= 'F'))
    A = (a - 'A') + 10;
  else if((a >= '0') && (a <= '9'))
    A = (a - '0');

  return A;
}

/****************************************************************************/
int FromHex2(char a, char b)
{
  int A = FromHex1(a);
  int B = FromHex1(b);

  if(A == FROM_HEX_ERROR || B == FROM_HEX_ERROR)
    return FROM_HEX_ERROR;

  return (A << 4) + B;
}

/****************************************************************************/
int FromHex4(char a, char b, char c, char d)
{
  int AB = FromHex2(a, b);
  int CD = FromHex2(c, d);

  if(AB == FROM_HEX_ERROR || CD == FROM_HEX_ERROR)
    return FROM_HEX_ERROR;

  return (AB << 8) + CD;
}

