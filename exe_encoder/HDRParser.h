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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup ExeEncoder
   @{
   \file
 *****************************************************************************/
#pragma once

#include <fstream>
#include <string>

extern "C"
{
#include "lib_common/HDR.h"
}

struct TJsonValue;

/*************************************************************************//*!
   \brief HDR SEI file parser
*****************************************************************************/
class HDRParser
{
public:
  explicit HDRParser(const std::string& sHDRFile);

  bool ReadHDRSEIs(AL_THDRSEIs& tHDRSEIs, int iSEIsIndex = 0);

private:
  const std::string sHDRFile;

  bool ReadLegacy(const std::string& sHDRFile, AL_THDRSEIs& tHDRSEIs);
  bool ReadJson(const std::string& sHDRFile, AL_THDRSEIs& tHDRSEIs, int iSEIsIndex);

  bool ReadMasteringDisplayColorVolume(TJsonValue* pSEIObject, AL_TMasteringDisplayColourVolume& tMDCV);
  bool ReadContentLightLevel(TJsonValue* pSEIObject, AL_TContentLightLevel& tCLL);
  bool ReadAlternativeTransferCharacteristics(TJsonValue* pSEIObject, AL_TAlternativeTransferCharacteristics& tATC);
  bool ReadST2094_10(TJsonValue* pSEIObject, AL_TDynamicMeta_ST2094_10& tST2094_10);
  bool ReadST2094_40(TJsonValue* pSEIObject, AL_TDynamicMeta_ST2094_40& tST2094_40);
  bool ReadST2094_40ProcessingWindow(TJsonValue* pPWObject, AL_TDynamicMeta_ST2094_40& tST2094_40, bool& bDefaultWindowParsed);
  bool ReadST2094_40PeakLuminance(TJsonValue* pPLObject, AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance);
};

