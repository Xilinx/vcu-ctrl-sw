// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "CodecUtils.h"
#include "resource.h"
#include "lib_app/utils.h"
#include "lib_app/BuildInfo.h"

/******************************************************************************/
void DisplayFrameStatus(int iFrameNum)
{
#if VERBOSE_MODE
  LogVerbose("\n\n> % 3d", iFrameNum);
#else
  LogVerbose("\r  Displayed picture #%-6d - ", iFrameNum);
#endif
}

#if !HAS_COMPIL_FLAGS
#define AL_COMPIL_FLAGS ""
#endif

void DisplayBuildInfo()
{
  BuildInfoDisplay displayBuildInfo {
    SCM_REV, SCM_BRANCH, AL_CONFIGURE_COMMANDLINE, AL_COMPIL_FLAGS, DELIVERY_BUILD_NUMBER, DELIVERY_SCM_REV, DELIVERY_DATE
  };
  displayBuildInfo();
}

/*****************************************************************************/
void DisplayVersionInfo()
{
  DisplayVersionInfo(AL_DECODER_COMPANY,
                     AL_DECODER_PRODUCT_NAME,
                     AL_DECODER_VERSION,
                     AL_DECODER_COPYRIGHT,
                     AL_DECODER_COMMENTS);
}

