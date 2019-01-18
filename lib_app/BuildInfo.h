/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#pragma once
#include "lib_app/utils.h"
#include "functional"
#include <cstring>

struct BuildInfoDisplay
{
  BuildInfoDisplay(char const* svnRevision, char const* configureCmdline, char const* compilationFlags) : svnRevision{svnRevision}, configureCmdline{configureCmdline}, compilationFlags{compilationFlags}
  {
  }

  void operator () ()
  {
    displayTimeOfBuild();

    if(displayFeatures)
      displayFeatures();
    displayBuildOptions();
  }

  void displayTimeOfBuild()
  {
    Message("Compiled on %s at %s", __DATE__, __TIME__);

    if(strcmp(svnRevision, "0") && strcmp(svnRevision, ""))
      Message(" from SVN revision %s.\n", svnRevision);
  }

  void displayBuildOptions()
  {
    if(strcmp(configureCmdline, ""))
      Message(CC_DEFAULT, "\nUsing configuration:\n%s\n", configureCmdline);

#if HAS_COMPIL_FLAGS

    if(strcmp(compilationFlags, ""))
      Message(CC_DEFAULT, "\nUsing compilation options:\n%s\n", compilationFlags);
#endif

    Message(CC_DEFAULT, "\nUsing allegro library version: %d.%d.%d\n", AL_VERSION_MAJOR, AL_VERSION_MINOR, AL_VERSION_STEP);
  }

  char const* svnRevision;
  char const* configureCmdline;
  char const* compilationFlags;

  std::function<void(void)> displayFeatures {};
};

static inline void DisplayVersionInfo(char const* company, char const* productName, int versionMajor, int versionMinor, int versionPatch, char const* copyright, char const* comments)
{
  Message("%s - %s v%d.%d.%d - %s\n", company,
          productName,
          versionMajor,
          versionMinor,
          versionPatch,
          copyright);

  Message(CC_YELLOW, "%s\n\n", comments);
}

