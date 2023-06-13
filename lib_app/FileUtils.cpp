// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>

#if defined(__linux__)
#include <dirent.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#include "lib_app/FileUtils.h"

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
bool checkFolder(std::string folderPath)
{
  (void)folderPath;

  if(folderPath.compare("") != 0)
  {
#if defined(_WIN32)

    WIN32_FIND_DATA FindFolderData;
    HANDLE hFind;
    folderPath.resize(folderPath.find_last_not_of("\\/") + 1);

    hFind = FindFirstFile(folderPath.c_str(), &FindFolderData);

    if(hFind == INVALID_HANDLE_VALUE)
      return false;

    FindClose(hFind);
#endif

#if defined(__linux__)

    DIR* dir = opendir(folderPath.c_str());

    if(dir == NULL)
      return false;
    closedir(dir);
#endif
  }

  return true;
}

/****************************************************************************/
bool checkFileAvailability(std::string folderPath, std::regex file_regex)
{
  (void)file_regex;

  if(folderPath.compare("") == 0)
  {
    folderPath = ".";
  }

#if defined(_WIN32)

  WIN32_FIND_DATA FindFolderData;
  HANDLE hFind;
  FILE* fileStream;

  if(folderPath.compare(".") != 0)
    folderPath.resize(folderPath.find_last_not_of("\\/") + 1);

  hFind = FindFirstFile((folderPath + "\\*").c_str(), &FindFolderData);

  if(hFind == INVALID_HANDLE_VALUE)
    return false;

  do
  {
    std::string file(FindFolderData.cFileName);

    if(std::regex_match(file, file_regex))
    {
      std::string file_path = folderPath + "\\" + file;

      if(fopen_s(&fileStream, file_path.c_str(), "r") == 0)
      {
        fclose(fileStream);
        FindClose(hFind);
        return true;
      }
    }
  }
  while(FindNextFile(hFind, &FindFolderData));

  FindClose(hFind);

#endif

#if defined(__linux__)

  if(folderPath.back() != '/')
    folderPath = folderPath + "/";

  struct dirent* entry;
  DIR* dir = opendir(folderPath.c_str());

  while((entry = readdir(dir)) != NULL)
  {
    std::string file(entry->d_name);

    if(std::regex_match(file, file_regex))
    {
      std::string file_path = folderPath + file;

      auto pFile = fopen(file_path.c_str(), "r");

      if(pFile)
      {
        fclose(pFile);
        closedir(dir);
        return true;
      }
    }
  }

#endif

  return false;
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

