/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "HDRParser.h"

#include "lib_app/JsonFile.h"

using namespace std;

HDRParser::HDRParser(const string& sHDRFile) :
  sHDRFile(sHDRFile)
{
}

bool HDRParser::ReadHDRSEIs(AL_THDRSEIs& tHDRSEIs, int iSEIsIndex)
{
  if(ReadJson(sHDRFile, tHDRSEIs, iSEIsIndex))
    return true;

  if(iSEIsIndex != 0)
    return false;

  return ReadLegacy(sHDRFile, tHDRSEIs);
}

bool HDRParser::ReadLegacy(const std::string& sHDRFile, AL_THDRSEIs& tHDRSEIs)
{
  std::ifstream ifs(sHDRFile);

  AL_HDRSEIs_Reset(&tHDRSEIs);

  if(!ifs.is_open())
    return false;

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

bool HDRParser::ReadJson(const std::string& sHDRFile, AL_THDRSEIs& tHDRSEIs, int iSEIsIndex)
{
  CJsonReader jsonreader(sHDRFile);
  TJsonValue tJsonRoot;

  AL_HDRSEIs_Reset(&tHDRSEIs);

  if(!jsonreader.Read(tJsonRoot))
    return false;

  TJsonValue* pSEIRoot = nullptr;
  switch(tJsonRoot.eType)
  {
  case TJsonValue::JSON_VALUE_OBJECT:

    if(iSEIsIndex != 0)
      return false;
    pSEIRoot = &tJsonRoot;
    break;
  case TJsonValue::JSON_VALUE_ARRAY:

    if(!tJsonRoot.GetValue(iSEIsIndex, TJsonValue::JSON_VALUE_OBJECT, pSEIRoot))
      return false;
    break;
  default:
    return false;
  }

  TJsonValue* pSEIObject = nullptr;

  if(pSEIRoot->GetValue("MasteringDisplayColorVolume", TJsonValue::JSON_VALUE_OBJECT, pSEIObject))
    tHDRSEIs.bHasMDCV = ReadMasteringDisplayColorVolume(pSEIObject, tHDRSEIs.tMDCV);

  if(pSEIRoot->GetValue("ContentLightLevel", TJsonValue::JSON_VALUE_OBJECT, pSEIObject))
    tHDRSEIs.bHasCLL = ReadContentLightLevel(pSEIObject, tHDRSEIs.tCLL);

  if(pSEIRoot->GetValue("AlternativeTransferCharacteristics", TJsonValue::JSON_VALUE_OBJECT, pSEIObject))
    tHDRSEIs.bHasATC = ReadAlternativeTransferCharacteristics(pSEIObject, tHDRSEIs.tATC);

  if(pSEIRoot->GetValue("DynamicMeta_ST2094_10", TJsonValue::JSON_VALUE_OBJECT, pSEIObject))
    tHDRSEIs.bHasST2094_10 = ReadST2094_10(pSEIObject, tHDRSEIs.tST2094_10);

  if(pSEIRoot->GetValue("DynamicMeta_ST2094_40", TJsonValue::JSON_VALUE_OBJECT, pSEIObject))
    tHDRSEIs.bHasST2094_40 = ReadST2094_40(pSEIObject, tHDRSEIs.tST2094_40);

  return true;
}

bool HDRParser::ReadMasteringDisplayColorVolume(TJsonValue* pSEIObject, AL_TMasteringDisplayColourVolume& tMDCV)
{
  TJsonValue* pDPObject = nullptr;

  if(!pSEIObject->GetValue("display_primaries", TJsonValue::JSON_VALUE_ARRAY, pDPObject))
    return false;

  TJsonValue* pCoord = nullptr;

  for(int i = 0; i < 3; i++)
  {
    if(!pDPObject->GetValue(i, TJsonValue::JSON_VALUE_ARRAY, pCoord) || pCoord->arrayValue.size() != 2)
      return false;
    tMDCV.display_primaries[i].x = pCoord->arrayValue[0].intValue;
    tMDCV.display_primaries[i].y = pCoord->arrayValue[1].intValue;
  }

  if(!pSEIObject->GetValue("white_point", TJsonValue::JSON_VALUE_ARRAY, pCoord) || pCoord->arrayValue.size() != 2)
    return false;
  tMDCV.white_point.x = pCoord->arrayValue[0].intValue;
  tMDCV.white_point.y = pCoord->arrayValue[1].intValue;

  int iVal = 0;

  if(!pSEIObject->GetValue("max_display_mastering_luminance", iVal))
    return false;
  tMDCV.max_display_mastering_luminance = iVal;

  if(!pSEIObject->GetValue("min_display_mastering_luminance", iVal))
    return false;
  tMDCV.min_display_mastering_luminance = iVal;

  return true;
}

bool HDRParser::ReadContentLightLevel(TJsonValue* pSEIObject, AL_TContentLightLevel& tCLL)
{
  int iVal = 0;

  if(!pSEIObject->GetValue("max_content_light_level", iVal))
    return false;
  tCLL.max_content_light_level = iVal;

  if(!pSEIObject->GetValue("max_pic_average_light_level", iVal))
    return false;
  tCLL.max_pic_average_light_level = iVal;

  return true;
}

bool HDRParser::ReadAlternativeTransferCharacteristics(TJsonValue* pSEIObject, AL_TAlternativeTransferCharacteristics& tATC)
{
  int iVal = 0;

  if(!pSEIObject->GetValue("preferred_transfer_characteristics", iVal))
    return false;
  tATC.preferred_transfer_characteristics = static_cast<AL_ETransferCharacteristics>(iVal);

  return true;
}

bool HDRParser::ReadST2094_10(TJsonValue* pSEIObject, AL_TDynamicMeta_ST2094_10& tST2094_10)
{
  int iVal = 0;

  if(!pSEIObject->GetValue("application_version", iVal))
    return false;
  tST2094_10.application_version = iVal;

  // -------Processing Windows -------
  {
    TJsonValue* pPWObject = nullptr;
    tST2094_10.processing_window_flag = pSEIObject->GetValue("processing_window", TJsonValue::JSON_VALUE_OBJECT, pPWObject);

    if(tST2094_10.processing_window_flag)
    {
      if(!pPWObject->GetValue("active_area_left_offset", iVal))
        return false;
      tST2094_10.processing_window.active_area_left_offset = iVal;

      if(!pPWObject->GetValue("active_area_right_offset", iVal))
        return false;
      tST2094_10.processing_window.active_area_right_offset = iVal;

      if(!pPWObject->GetValue("active_area_top_offset", iVal))
        return false;
      tST2094_10.processing_window.active_area_top_offset = iVal;

      if(!pPWObject->GetValue("active_area_bottom_offset", iVal))
        return false;
      tST2094_10.processing_window.active_area_bottom_offset = iVal;
    }
  }

  // -------Image Characteristics -------
  {
    TJsonValue* pICObject = nullptr;

    if(!pSEIObject->GetValue("image_characteristics", TJsonValue::JSON_VALUE_OBJECT, pICObject))
      return false;

    if(!pICObject->GetValue("min_pq", iVal))
      return false;
    tST2094_10.image_characteristics.min_pq = iVal;

    if(!pICObject->GetValue("max_pq", iVal))
      return false;
    tST2094_10.image_characteristics.max_pq = iVal;

    if(!pICObject->GetValue("avg_pq", iVal))
      return false;
    tST2094_10.image_characteristics.avg_pq = iVal;
  }

  // -------Manual Adjustment -------
  {
    tST2094_10.num_manual_adjustments = 0;

    TJsonValue* pMAArray = nullptr;

    if(!pSEIObject->GetValue("manual_adjustments", TJsonValue::JSON_VALUE_ARRAY, pMAArray))
      return true;

    size_t iNumMA = pMAArray->arrayValue.size();

    if(iNumMA > AL_MAX_MANUAL_ADJUSTMENT_ST2094_10)
      return false;

    for(size_t i = 0; i < iNumMA; i++, tST2094_10.num_manual_adjustments++)
    {
      TJsonValue* pMAObject = nullptr;

      if(!pMAArray->GetValue(i, TJsonValue::JSON_VALUE_OBJECT, pMAObject))
        return false;

      if(!pMAObject->GetValue("target_max_pq", iVal))
        return false;
      tST2094_10.manual_adjustments[i].target_max_pq = iVal;

      if(!pMAObject->GetValue("trim_slope", iVal))
        return false;
      tST2094_10.manual_adjustments[i].trim_slope = iVal;

      if(!pMAObject->GetValue("trim_offset", iVal))
        return false;
      tST2094_10.manual_adjustments[i].trim_offset = iVal;

      if(!pMAObject->GetValue("trim_power", iVal))
        return false;
      tST2094_10.manual_adjustments[i].trim_power = iVal;

      if(!pMAObject->GetValue("trim_chroma_weight", iVal))
        return false;
      tST2094_10.manual_adjustments[i].trim_chroma_weight = iVal;

      if(!pMAObject->GetValue("trim_saturation_gain", iVal))
        return false;
      tST2094_10.manual_adjustments[i].trim_saturation_gain = iVal;

      if(!pMAObject->GetValue("ms_weight", iVal))
        return false;
      tST2094_10.manual_adjustments[i].ms_weight = iVal;
    }
  }

  return true;
}

bool HDRParser::ReadST2094_40(TJsonValue* pSEIObject, AL_TDynamicMeta_ST2094_40& tST2094_40)
{
  int iVal = 0;

  if(!pSEIObject->GetValue("application_version", iVal))
    return false;
  tST2094_40.application_version = iVal;

  // -------Processing Windows -------
  {
    tST2094_40.num_windows = 0;

    TJsonValue* pPWArray = nullptr;

    if(!pSEIObject->GetValue("processing_windows", TJsonValue::JSON_VALUE_ARRAY, pPWArray))
      return false;

    size_t iNumWindow = pPWArray->arrayValue.size();

    if(iNumWindow < AL_MIN_WINDOW_ST2094_40 || iNumWindow > AL_MAX_WINDOW_ST2094_40)
      return false;

    bool bDefaultWindowParsed = false;

    for(size_t i = 0; i < iNumWindow; i++, tST2094_40.num_windows++)
    {
      TJsonValue* pPWObject = nullptr;

      if(!pPWArray->GetValue(i, TJsonValue::JSON_VALUE_OBJECT, pPWObject))
        return false;

      if(!ReadST2094_40ProcessingWindow(pPWObject, tST2094_40, bDefaultWindowParsed))
        return false;
    }
  }

  // ------- Targeted System Display -------
  {
    TJsonValue* pTSDObject = nullptr;

    if(!pSEIObject->GetValue("targeted_system_display", TJsonValue::JSON_VALUE_OBJECT, pTSDObject))
      return false;

    if(!pTSDObject->GetValue("maximum_luminance", iVal))
      return false;
    tST2094_40.targeted_system_display.maximum_luminance = iVal;

    AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance = tST2094_40.targeted_system_display.peak_luminance;
    tPeakLuminance.actual_peak_luminance_flag = false;

    TJsonValue* pPLObject = nullptr;

    if(pTSDObject->GetValue("peak_luminance", TJsonValue::JSON_VALUE_ARRAY, pPLObject))
      tPeakLuminance.actual_peak_luminance_flag = ReadST2094_40PeakLuminance(pPLObject, tPeakLuminance);
  }

  // ------- Mastering Display Luminance -------
  {
    AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance = tST2094_40.mastering_display_peak_luminance;
    tPeakLuminance.actual_peak_luminance_flag = false;

    TJsonValue* pPLObject = nullptr;

    if(pSEIObject->GetValue("mastering_display_peak_luminance", TJsonValue::JSON_VALUE_ARRAY, pPLObject))
      tPeakLuminance.actual_peak_luminance_flag = ReadST2094_40PeakLuminance(pPLObject, tPeakLuminance);
  }

  return true;
}

bool HDRParser::ReadST2094_40ProcessingWindow(TJsonValue* pPWObject, AL_TDynamicMeta_ST2094_40& tST2094_40, bool& bDefaultWindowParsed)
{
  int iVal = 0;

  TJsonValue* pWindowObject = nullptr;
  bool bDefaultWindow = !pPWObject->GetValue("window", TJsonValue::JSON_VALUE_OBJECT, pWindowObject);

  if(bDefaultWindow)
  {
    if(bDefaultWindowParsed)
      return false;
    bDefaultWindowParsed = true;
  }
  else
  {
    AL_TProcessingWindow_ST2094_40& tProcessingWindow = tST2094_40.processing_windows[tST2094_40.num_windows - (bDefaultWindowParsed ? 1 : 0)];

    TJsonValue* pCoord = nullptr;

    {
      TJsonValue* pBaseWindowObject = nullptr;

      if(!pWindowObject->GetValue("base_window", TJsonValue::JSON_VALUE_ARRAY, pBaseWindowObject) || pBaseWindowObject->arrayValue.size() != 2)
        return false;

      if(!pBaseWindowObject->GetValue(0, TJsonValue::JSON_VALUE_ARRAY, pCoord) || pCoord->arrayValue.size() != 2)
        return false;
      tProcessingWindow.base_processing_window.upper_left_corner_x = pCoord->arrayValue[0].intValue;
      tProcessingWindow.base_processing_window.upper_left_corner_y = pCoord->arrayValue[1].intValue;

      if(!pBaseWindowObject->GetValue(1, TJsonValue::JSON_VALUE_ARRAY, pCoord) || pCoord->arrayValue.size() != 2)
        return false;
      tProcessingWindow.base_processing_window.lower_right_corner_x = pCoord->arrayValue[0].intValue;
      tProcessingWindow.base_processing_window.lower_right_corner_y = pCoord->arrayValue[1].intValue;
    }

    if(!pWindowObject->GetValue("center_of_ellipse", TJsonValue::JSON_VALUE_ARRAY, pCoord) || pCoord->arrayValue.size() != 2)
      return false;
    tProcessingWindow.center_of_ellipse_x = pCoord->arrayValue[0].intValue;
    tProcessingWindow.center_of_ellipse_y = pCoord->arrayValue[1].intValue;

    if(!pWindowObject->GetValue("rotation_angle", iVal))
      return false;
    tProcessingWindow.rotation_angle = iVal;

    if(!pWindowObject->GetValue("semimajor_axis_internal_ellipse", iVal))
      return false;
    tProcessingWindow.semimajor_axis_internal_ellipse = iVal;

    if(!pWindowObject->GetValue("semimajor_axis_external_ellipse", iVal))
      return false;
    tProcessingWindow.semimajor_axis_external_ellipse = iVal;

    if(!pWindowObject->GetValue("semiminor_axis_external_ellipse", iVal))
      return false;
    tProcessingWindow.semiminor_axis_external_ellipse = iVal;

    if(!pWindowObject->GetValue("overlap_process_option", iVal))
      return false;
    tProcessingWindow.overlap_process_option = iVal;
  }

  {
    AL_TProcessingWindowTransform_ST2094_40& tTransfo = tST2094_40.processing_window_transforms[tST2094_40.num_windows + (bDefaultWindowParsed ? 0 : 1)];

    TJsonValue* pTransfoObject = nullptr;

    if(!pPWObject->GetValue("transform", TJsonValue::JSON_VALUE_OBJECT, pTransfoObject))
      return false;

    TJsonValue* pArray = nullptr;

    if(!pTransfoObject->GetValue("maxscl", TJsonValue::JSON_VALUE_ARRAY, pArray) || pArray->arrayValue.size() != 3)
      return false;

    for(int i = 0; i < 3; i++)
      tTransfo.maxscl[i] = pArray->arrayValue[i].intValue;

    if(!pTransfoObject->GetValue("average_maxrgb", iVal))
      return false;
    tTransfo.average_maxrgb = iVal;

    if(!pTransfoObject->GetValue("distribution_maxrgb_percentages", TJsonValue::JSON_VALUE_ARRAY, pArray) || pArray->arrayValue.size() > AL_MAX_MAXRGB_PERCENTILES_ST2094_40)
      return false;
    tTransfo.num_distribution_maxrgb_percentiles = pArray->arrayValue.size();

    for(int i = 0; i < tTransfo.num_distribution_maxrgb_percentiles; i++)
      tTransfo.distribution_maxrgb_percentages[i] = pArray->arrayValue[i].intValue;

    if(!pTransfoObject->GetValue("distribution_maxrgb_percentiles", TJsonValue::JSON_VALUE_ARRAY, pArray) || pArray->arrayValue.size() != tTransfo.num_distribution_maxrgb_percentiles)
      return false;

    for(int i = 0; i < tTransfo.num_distribution_maxrgb_percentiles; i++)
      tTransfo.distribution_maxrgb_percentiles[i] = pArray->arrayValue[i].intValue;

    if(!pTransfoObject->GetValue("fraction_bright_pixels", iVal))
      return false;
    tTransfo.fraction_bright_pixels = iVal;

    TJsonValue* pTMObject = nullptr;
    tTransfo.tone_mapping.tone_mapping_flag = pTransfoObject->GetValue("tone_mapping", TJsonValue::JSON_VALUE_OBJECT, pTMObject);

    if(tTransfo.tone_mapping.tone_mapping_flag)
    {
      if(!pTMObject->GetValue("knee_point", TJsonValue::JSON_VALUE_ARRAY, pArray) || pArray->arrayValue.size() != 2)
        return false;
      tTransfo.tone_mapping.knee_point_x = pArray->arrayValue[0].intValue;
      tTransfo.tone_mapping.knee_point_y = pArray->arrayValue[1].intValue;

      if(!pTMObject->GetValue("bezier_curve_anchors", TJsonValue::JSON_VALUE_ARRAY, pArray) || pArray->arrayValue.size() > AL_MAX_BEZIER_CURVE_ANCHORS_ST2094_40)
        return false;
      tTransfo.tone_mapping.num_bezier_curve_anchors = pArray->arrayValue.size();

      for(int i = 0; i < tTransfo.tone_mapping.num_bezier_curve_anchors; i++)
        tTransfo.tone_mapping.bezier_curve_anchors[i] = pArray->arrayValue[i].intValue;
    }

    tTransfo.color_saturation_mapping_flag = pTransfoObject->GetValue("color_saturation_weight", iVal);

    if(tTransfo.color_saturation_mapping_flag)
      tTransfo.color_saturation_weight = iVal;
  }

  return true;
}

bool HDRParser::ReadST2094_40PeakLuminance(TJsonValue* pPLObject, AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance)
{
  tPeakLuminance.num_rows_actual_peak_luminance = pPLObject->arrayValue.size();
  tPeakLuminance.num_cols_actual_peak_luminance = 0;

  if(tPeakLuminance.num_rows_actual_peak_luminance > AL_MAX_ROW_ACTUAL_PEAK_LUMINANCE_ST2094_40)
    return false;

  TJsonValue* pRow = nullptr;

  for(int i = 0; i < tPeakLuminance.num_rows_actual_peak_luminance; i++)
  {
    if(!pPLObject->GetValue(i, TJsonValue::JSON_VALUE_ARRAY, pRow))
      return false;

    if(tPeakLuminance.num_cols_actual_peak_luminance == 0)
    {
      tPeakLuminance.num_cols_actual_peak_luminance = pRow->arrayValue.size();

      if(tPeakLuminance.num_cols_actual_peak_luminance > AL_MAX_COL_ACTUAL_PEAK_LUMINANCE_ST2094_40)
        return false;
    }
    else if(tPeakLuminance.num_cols_actual_peak_luminance != pRow->arrayValue.size())
      return false;

    for(int j = 0; j < tPeakLuminance.num_cols_actual_peak_luminance; j++)
      tPeakLuminance.actual_peak_luminance[i][j] = pRow->arrayValue[j].intValue;
  }

  return true;
}
