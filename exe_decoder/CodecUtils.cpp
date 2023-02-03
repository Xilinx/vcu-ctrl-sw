/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

