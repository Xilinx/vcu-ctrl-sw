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

#include <string>
#include <regex>

/****************************************************************************/
void formatFolderPath(std::string& folderPath);

/****************************************************************************/
std::string combinePath(const std::string& folder, const std::string& filename);

/****************************************************************************/
std::string createFileNameWithID(const std::string& path, const std::string& motif, const std::string& extension, int iFrameID);

/****************************************************************************/
bool checkFolder(std::string folderPath);

/****************************************************************************/
bool checkFileAvailability(std::string folderPath, std::regex regex);

/****************************************************************************/
#define FROM_HEX_ERROR -1
int FromHex2(char a, char b);

/****************************************************************************/
int FromHex4(char a, char b, char c, char d);

