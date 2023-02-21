// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

