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

#pragma once
#include "lib_app/utils.h"
#include "functional"
#include <cstring>

#ifndef DELIVERY_BUILD_NUMBER
#define DELIVERY_BUILD_NUMBER 0
#endif

#ifndef DELIVERY_SCM_REV
#define DELIVERY_SCM_REV "unknown"
#endif

#ifndef DELIVERY_DATE
#define DELIVERY_DATE "unknown"
#endif

#ifndef SCM_REV
#define SCM_REV ""
#endif

#ifndef SCM_BRANCH
#define SCM_BRANCH ""
#endif

struct BuildInfoDisplay
{
  BuildInfoDisplay(char const* scmRevision, char const* scmBranch, char const* configureCmdline, char const* compilationFlags, int deliveryBuildNumber = 0, char const* deliveryScmRevision = "unknown", char const* deliveryDate = "unknown") : scmRevision{scmRevision}, scmBranch{scmBranch}, configureCmdline{configureCmdline}, compilationFlags{compilationFlags}, deliveryBuildNumber{deliveryBuildNumber}, deliveryScmRevision{deliveryScmRevision}, deliveryDate{deliveryDate}
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
    LogInfo("Compiled on %s at %s", __DATE__, __TIME__);

    if(strcmp(scmRevision, "0") && strcmp(scmRevision, ""))
      LogInfo(" from GIT revision %s", scmRevision);

    if(strcmp(scmBranch, "unknown"))
      LogInfo(" (%s)", scmBranch);

    LogInfo(".\n");

    if(strcmp(deliveryDate, "unknown") && strcmp(deliveryDate, ""))
      LogInfo("\nDelivery created the %s", deliveryDate);

    if(deliveryBuildNumber != 0)
      LogInfo(". CI Build Number: %d", deliveryBuildNumber);

    if(strcmp(deliveryScmRevision, "unknown") && strcmp(deliveryScmRevision, ""))
      LogInfo(". GIT revision %s.\n", deliveryScmRevision);
  }

  void displayBuildOptions()
  {
    if(strcmp(configureCmdline, ""))
      LogInfo("\nUsing configuration:\n%s\n", configureCmdline);

#if HAS_COMPIL_FLAGS

    if(strcmp(compilationFlags, ""))
      LogInfo("\nUsing compilation options:\n%s\n", compilationFlags);
#endif

    LogInfo("\nUsing allegro library version: %d.%d.%d\n", AL_VERSION_MAJOR, AL_VERSION_MINOR, AL_VERSION_STEP);
  }

  char const* scmRevision;
  char const* scmBranch;
  char const* configureCmdline;
  char const* compilationFlags;
  int deliveryBuildNumber;
  char const* deliveryScmRevision;
  char const* deliveryDate;

  std::function<void(void)> displayFeatures {};
};

static inline void DisplayVersionInfo(char const* company, char const* productName, int versionMajor, int versionMinor, int versionPatch, char const* copyright, char const* comments)
{
  LogVerbose("%s - %s v%d.%d.%d - %s\n", company,
             productName,
             versionMajor,
             versionMinor,
             versionPatch,
             copyright);

  LogVerbose(CC_YELLOW, "%s\n\n", comments);
}

