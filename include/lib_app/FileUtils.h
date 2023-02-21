// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

