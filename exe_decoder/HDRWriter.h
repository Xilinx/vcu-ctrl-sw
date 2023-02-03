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
   \addtogroup ExeDecoder
   @{
   \file
 *****************************************************************************/
#pragma once

#include <fstream>
#include <string>
#include "lib_app/JsonFile.h"

extern "C"
{
#include "lib_common/HDR.h"
}

/*************************************************************************//*!
   \brief HDR SEI file writer
*****************************************************************************/
class HDRWriter
{
public:
  explicit HDRWriter(const std::string& sHDRFile);

  bool WriteHDRSEIs(AL_EColourDescription eColourDesc, AL_ETransferCharacteristics eTransferCharacteristics, AL_EColourMatrixCoefficients eMatrixCoeffs, AL_THDRSEIs& tHDRSEIs);

private:
  CJsonWriter jsonWriter;

  void BuildJson(AL_EColourDescription eColourDesc, AL_ETransferCharacteristics eTransferCharacteristics, AL_EColourMatrixCoefficients eMatrixCoeffs, AL_THDRSEIs& tHDRSEIs, TJsonValue& tSEIRoot);
  void BuildSEIs(AL_THDRSEIs& tHDRSEIs, TJsonValue& tSEIRoot);
  void BuildMasteringDisplayColorVolume(AL_TMasteringDisplayColourVolume& tMDCV, TJsonValue& tSEIObject);
  void BuildContentLightLevel(AL_TContentLightLevel& tCLL, TJsonValue& tSEIObject);
  void BuildAlternativeTransferCharacteristics(AL_TAlternativeTransferCharacteristics& tATC, TJsonValue& tSEIObject);
  void BuildST2094_10(AL_TDynamicMeta_ST2094_10& tST2094_10, TJsonValue& tSEIObject);
  void BuildST2094_40(AL_TDynamicMeta_ST2094_40& tST2094_40, TJsonValue& tSEIObject);
  void BuildST2094_40ProcessingWindow(AL_TDynamicMeta_ST2094_40& tST2094_40, int iWindow, TJsonValue& tPWObject);
  void BuildST2094_40PeakLuminance(AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance, TJsonValue& tPLRootObject, std::string sKey);
};
