// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <regex>
#include "lib_rtos/types.h"

/****************************************************************************/
std::string BaseName(const std::string& filename);

/****************************************************************************/
std::string DirName(const std::string& filename);

/****************************************************************************/
void FormatFolderPath(std::string& folderPath);

/****************************************************************************/
std::string CombinePath(std::string const& folder, std::string const& filename);

/****************************************************************************/
std::string CreateFileNameWithID(std::string const& path, std::string const& motif, std::string const& extension, AL_64S iFrameID);

/****************************************************************************/
bool FolderExists(std::string folderPath);

/****************************************************************************/
bool FileExists(std::string folderPath, std::regex const& regex);

/****************************************************************************/
bool GetFileSize(std::ifstream& fileStream, size_t& zSize);

/****************************************************************************/
bool GetFileSize(std::string const& filename, size_t& zSize);

/****************************************************************************/
#define FROM_HEX_ERROR (-1)
int32_t FromHex2(char a, char b);

/****************************************************************************/
int32_t FromHex4(char a, char b, char c, char d);
