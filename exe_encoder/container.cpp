// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include <string>
#include "lib_app/InputFiles.h"
#include "CodecUtils.h"

using namespace std;

void WriteContainerHeader(ofstream& fp, AL_TEncSettings const& Settings, TYUVFileInfo const& FileInfo, int numFrames)
{
  (void)fp;
  (void)Settings;
  (void)FileInfo;
  (void)numFrames;
}

