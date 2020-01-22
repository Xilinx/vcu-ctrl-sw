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

#include "HDRParser.h"

using namespace std;

HDRParser::HDRParser(const string& sHDRFile) :
  ifs(sHDRFile)
{
}

bool HDRParser::ReadHDRSEIs(AL_THDRSEIs& tHDRSEIs)
{
  AL_HDRSEIs_Reset(&tHDRSEIs);

  while(!ifs.eof())
  {
    string sSEIID;
    ifs >> sSEIID;

    if(sSEIID == "MasteringDisplayColorVolume:")
    {
      for(int i = 0; i < 3; i++)
        ifs >> tHDRSEIs.tMDCV.display_primaries[i].x >> tHDRSEIs.tMDCV.display_primaries[i].y;

      ifs >> tHDRSEIs.tMDCV.white_point.x >> tHDRSEIs.tMDCV.white_point.y;
      ifs >> tHDRSEIs.tMDCV.max_display_mastering_luminance >> tHDRSEIs.tMDCV.min_display_mastering_luminance;

      if(ifs.fail())
        return false;

      tHDRSEIs.bHasMDCV = true;
    }
    else if(sSEIID == "ContentLightLevelInfo:")
    {
      ifs >> tHDRSEIs.tCLL.max_content_light_level >> tHDRSEIs.tCLL.max_pic_average_light_level;

      if(ifs.fail())
        return false;

      tHDRSEIs.bHasCLL = true;
    }
    else if(!sSEIID.empty())
      return false;
  }

  return true;
}

