// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
