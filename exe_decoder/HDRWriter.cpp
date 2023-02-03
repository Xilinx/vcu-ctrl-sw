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

#include "HDRWriter.h"
#include "lib_app/JsonFile.h"

using namespace std;

HDRWriter::HDRWriter(const string& sHDRFile) :
  jsonWriter(sHDRFile, true)
{
}

bool HDRWriter::WriteHDRSEIs(AL_EColourDescription eColourDesc, AL_ETransferCharacteristics eTransferCharacteristics, AL_EColourMatrixCoefficients eMatrixCoeffs, AL_THDRSEIs& tHDRSEIs)
{
  TJsonValue tSEIRoot;

  BuildJson(eColourDesc, eTransferCharacteristics, eMatrixCoeffs, tHDRSEIs, tSEIRoot);

  return jsonWriter.Write(tSEIRoot);
}

void HDRWriter::BuildJson(AL_EColourDescription eColourDesc, AL_ETransferCharacteristics eTransferCharacteristics, AL_EColourMatrixCoefficients eMatrixCoeffs, AL_THDRSEIs& tHDRSEIs, TJsonValue& tSEIRoot)
{
  tSEIRoot.eType = TJsonValue::JSON_VALUE_OBJECT;

  tSEIRoot.AddValue("colour_primaries", static_cast<int>(eColourDesc));
  tSEIRoot.AddValue("transfer_characteristics", static_cast<int>(eTransferCharacteristics));
  tSEIRoot.AddValue("matrix_coefficients", static_cast<int>(eMatrixCoeffs));

  BuildSEIs(tHDRSEIs, tSEIRoot);
}

void HDRWriter::BuildSEIs(AL_THDRSEIs& tHDRSEIs, TJsonValue& tSEIRoot)
{
  if(tHDRSEIs.bHasMDCV)
  {
    auto& tMDCVObject = tSEIRoot.AddValue("MasteringDisplayColorVolume", TJsonValue::JSON_VALUE_OBJECT);
    BuildMasteringDisplayColorVolume(tHDRSEIs.tMDCV, tMDCVObject);
  }

  if(tHDRSEIs.bHasCLL)
  {
    auto& tCLLObject = tSEIRoot.AddValue("ContentLightLevel", TJsonValue::JSON_VALUE_OBJECT);
    BuildContentLightLevel(tHDRSEIs.tCLL, tCLLObject);
  }

  if(tHDRSEIs.bHasATC)
  {
    auto& tATCObject = tSEIRoot.AddValue("AlternativeTransferCharacteristics", TJsonValue::JSON_VALUE_OBJECT);
    BuildAlternativeTransferCharacteristics(tHDRSEIs.tATC, tATCObject);
  }

  if(tHDRSEIs.bHasST2094_10)
  {
    auto& tST9094_10Object = tSEIRoot.AddValue("DynamicMeta_ST2094_10", TJsonValue::JSON_VALUE_OBJECT);
    BuildST2094_10(tHDRSEIs.tST2094_10, tST9094_10Object);
  }

  if(tHDRSEIs.bHasST2094_40)
  {
    auto& tST9094_40Object = tSEIRoot.AddValue("DynamicMeta_ST2094_40", TJsonValue::JSON_VALUE_OBJECT);
    BuildST2094_40(tHDRSEIs.tST2094_40, tST9094_40Object);
  }
}

void HDRWriter::BuildMasteringDisplayColorVolume(AL_TMasteringDisplayColourVolume& tMDCV, TJsonValue& tSEIObject)
{
  auto& tDPArray = tSEIObject.AddValue("display_primaries", TJsonValue::JSON_VALUE_ARRAY);

  for(int i = 0; i < 3; i++)
  {
    auto& tComponentArray = tDPArray.PushBackValue(TJsonValue::JSON_VALUE_ARRAY);
    tComponentArray.PushBackValue(static_cast<int>(tMDCV.display_primaries[i].x));
    tComponentArray.PushBackValue(static_cast<int>(tMDCV.display_primaries[i].y));
  }

  auto& tWPArray = tSEIObject.AddValue("white_point", TJsonValue::JSON_VALUE_ARRAY);
  tWPArray.PushBackValue(static_cast<int>(tMDCV.white_point.x));
  tWPArray.PushBackValue(static_cast<int>(tMDCV.white_point.y));

  tSEIObject.objectValue["max_display_mastering_luminance"] = TJsonValue(static_cast<int>(tMDCV.max_display_mastering_luminance));
  tSEIObject.objectValue["min_display_mastering_luminance"] = TJsonValue(static_cast<int>(tMDCV.min_display_mastering_luminance));
}

void HDRWriter::BuildContentLightLevel(AL_TContentLightLevel& tCLL, TJsonValue& tSEIObject)
{
  tSEIObject.AddValue("max_content_light_level", static_cast<int>(tCLL.max_content_light_level));
  tSEIObject.AddValue("max_pic_average_light_level", static_cast<int>(tCLL.max_pic_average_light_level));
}

void HDRWriter::BuildAlternativeTransferCharacteristics(AL_TAlternativeTransferCharacteristics& tATC, TJsonValue& tSEIObject)
{
  tSEIObject.AddValue("preferred_transfer_characteristics", static_cast<int>(tATC.preferred_transfer_characteristics));
}

void HDRWriter::BuildST2094_10(AL_TDynamicMeta_ST2094_10& tST2094_10, TJsonValue& tSEIObject)
{
  tSEIObject.AddValue("application_version", static_cast<int>(tST2094_10.application_version));

  // -------Processing Windows -------
  if(tST2094_10.processing_window_flag)
  {
    auto& tPWObject = tSEIObject.AddValue("processing_window", TJsonValue::JSON_VALUE_OBJECT);
    tPWObject.AddValue("active_area_left_offset", static_cast<int>(tST2094_10.processing_window.active_area_left_offset));
    tPWObject.AddValue("active_area_right_offset", static_cast<int>(tST2094_10.processing_window.active_area_right_offset));
    tPWObject.AddValue("active_area_top_offset", static_cast<int>(tST2094_10.processing_window.active_area_top_offset));
    tPWObject.AddValue("active_area_bottom_offset", static_cast<int>(tST2094_10.processing_window.active_area_bottom_offset));
  }

  // -------Image Characteristics -------
  {
    auto& tICObject = tSEIObject.AddValue("image_characteristics", TJsonValue::JSON_VALUE_OBJECT);
    tICObject.AddValue("min_pq", static_cast<int>(tST2094_10.image_characteristics.min_pq));
    tICObject.AddValue("max_pq", static_cast<int>(tST2094_10.image_characteristics.max_pq));
    tICObject.AddValue("avg_pq", static_cast<int>(tST2094_10.image_characteristics.avg_pq));
  }

  // -------Manual Adjustment -------
  if(tST2094_10.num_manual_adjustments != 0)
  {
    auto& tMAArray = tSEIObject.AddValue("manual_adjustments", TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < tST2094_10.num_manual_adjustments; i++)
    {
      auto& tMAObject = tMAArray.PushBackValue(TJsonValue::JSON_VALUE_OBJECT);
      tMAObject.AddValue("target_max_pq", tST2094_10.manual_adjustments[i].target_max_pq);
      tMAObject.AddValue("trim_slope", tST2094_10.manual_adjustments[i].trim_slope);
      tMAObject.AddValue("trim_offset", tST2094_10.manual_adjustments[i].trim_offset);
      tMAObject.AddValue("trim_power", tST2094_10.manual_adjustments[i].trim_power);
      tMAObject.AddValue("trim_chroma_weight", tST2094_10.manual_adjustments[i].trim_chroma_weight);
      tMAObject.AddValue("trim_saturation_gain", tST2094_10.manual_adjustments[i].trim_saturation_gain);
      tMAObject.AddValue("ms_weight", tST2094_10.manual_adjustments[i].ms_weight);
    }
  }
}

void HDRWriter::BuildST2094_40(AL_TDynamicMeta_ST2094_40& tST2094_40, TJsonValue& tSEIObject)
{
  tSEIObject.AddValue("application_version", tST2094_40.application_version);

  // -------Processing Windows -------
  {
    auto& tPWArray = tSEIObject.AddValue("processing_windows", TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < tST2094_40.num_windows; i++)
    {
      auto& tPWObject = tPWArray.PushBackValue(TJsonValue::JSON_VALUE_OBJECT);
      BuildST2094_40ProcessingWindow(tST2094_40, i, tPWObject);
    }
  }

  // ------- Targeted System Display -------
  {
    auto& tTSDObject = tSEIObject.AddValue("targeted_system_display", TJsonValue::JSON_VALUE_OBJECT);

    tTSDObject.AddValue("maximum_luminance", static_cast<int>(tST2094_40.targeted_system_display.maximum_luminance));
    BuildST2094_40PeakLuminance(tST2094_40.targeted_system_display.peak_luminance, tTSDObject, "peak_luminance");
  }

  // ------- Mastering Display Luminance -------
  BuildST2094_40PeakLuminance(tST2094_40.mastering_display_peak_luminance, tSEIObject, "mastering_display_peak_luminance");
}

void HDRWriter::BuildST2094_40ProcessingWindow(AL_TDynamicMeta_ST2094_40& tST2094_40, int iWindow, TJsonValue& tPWObject)
{
  if(iWindow != 0)
  {
    auto& tWindowObject = tPWObject.AddValue("window", TJsonValue::JSON_VALUE_OBJECT);

    AL_TProcessingWindow_ST2094_40& tProcessingWindow = tST2094_40.processing_windows[iWindow - 1];

    {
      auto& tBaseWindowObject = tWindowObject.AddValue("base_window", TJsonValue::JSON_VALUE_ARRAY);

      auto& tULCoord = tBaseWindowObject.PushBackValue(TJsonValue::JSON_VALUE_ARRAY);
      tULCoord.PushBackValue(static_cast<int>(tProcessingWindow.base_processing_window.upper_left_corner_x));
      tULCoord.PushBackValue(static_cast<int>(tProcessingWindow.base_processing_window.upper_left_corner_y));

      auto& tLRCoord = tBaseWindowObject.PushBackValue(TJsonValue::JSON_VALUE_ARRAY);
      tLRCoord.PushBackValue(static_cast<int>(tProcessingWindow.base_processing_window.lower_right_corner_x));
      tLRCoord.PushBackValue(static_cast<int>(tProcessingWindow.base_processing_window.lower_right_corner_y));
    }

    auto& tCOECoord = tWindowObject.AddValue("center_of_ellipse", TJsonValue::JSON_VALUE_ARRAY);
    tCOECoord.PushBackValue(static_cast<int>(tProcessingWindow.center_of_ellipse_x));
    tCOECoord.PushBackValue(static_cast<int>(tProcessingWindow.center_of_ellipse_y));

    tWindowObject.AddValue("rotation_angle", static_cast<int>(tProcessingWindow.rotation_angle));
    tWindowObject.AddValue("semimajor_axis_internal_ellipse", static_cast<int>(tProcessingWindow.semimajor_axis_internal_ellipse));
    tWindowObject.AddValue("semimajor_axis_external_ellipse", static_cast<int>(tProcessingWindow.semimajor_axis_external_ellipse));
    tWindowObject.AddValue("semiminor_axis_external_ellipse", static_cast<int>(tProcessingWindow.semiminor_axis_external_ellipse));
    tWindowObject.AddValue("overlap_process_option", static_cast<int>(tProcessingWindow.overlap_process_option));
  }

  {
    AL_TProcessingWindowTransform_ST2094_40& tTransfo = tST2094_40.processing_window_transforms[iWindow];

    auto& tTransfoObject = tPWObject.AddValue("transform", TJsonValue::JSON_VALUE_OBJECT);

    auto& tMSCLArray = tTransfoObject.AddValue("maxscl", TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < 3; i++)
      tMSCLArray.PushBackValue(static_cast<int>(tTransfo.maxscl[i]));

    tTransfoObject.AddValue("average_maxrgb", static_cast<int>(tTransfo.average_maxrgb));

    auto& tPercentagesArray = tTransfoObject.AddValue("distribution_maxrgb_percentages", TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < tTransfo.num_distribution_maxrgb_percentiles; i++)
      tPercentagesArray.PushBackValue(static_cast<int>(tTransfo.distribution_maxrgb_percentages[i]));

    auto& tPercentilesArray = tTransfoObject.AddValue("distribution_maxrgb_percentiles", TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < tTransfo.num_distribution_maxrgb_percentiles; i++)
      tPercentilesArray.PushBackValue(static_cast<int>(tTransfo.distribution_maxrgb_percentiles[i]));

    tTransfoObject.AddValue("fraction_bright_pixels", static_cast<int>(tTransfo.fraction_bright_pixels));

    if(tTransfo.tone_mapping.tone_mapping_flag)
    {
      auto& tTMObject = tTransfoObject.AddValue("tone_mapping", TJsonValue::JSON_VALUE_OBJECT);
      auto& tKPArray = tTMObject.AddValue("knee_point", TJsonValue::JSON_VALUE_ARRAY);
      tKPArray.PushBackValue(static_cast<int>(tTransfo.tone_mapping.knee_point_x));
      tKPArray.PushBackValue(static_cast<int>(tTransfo.tone_mapping.knee_point_y));

      auto& tBCAObject = tTMObject.AddValue("bezier_curve_anchors", TJsonValue::JSON_VALUE_ARRAY);

      for(int i = 0; i < tTransfo.tone_mapping.num_bezier_curve_anchors; i++)
        tBCAObject.PushBackValue(static_cast<int>(tTransfo.tone_mapping.bezier_curve_anchors[i]));
    }

    if(tTransfo.color_saturation_mapping_flag)
      tTransfoObject.AddValue("color_saturation_weight", static_cast<int>(tTransfo.color_saturation_weight));
  }
}

void HDRWriter::BuildST2094_40PeakLuminance(AL_TDisplayPeakLuminance_ST2094_40& tPeakLuminance, TJsonValue& tPLRootObject, std::string sKey)
{
  if(tPeakLuminance.actual_peak_luminance_flag)
  {
    TJsonValue& tPLObject = tPLRootObject.AddValue(sKey, TJsonValue::JSON_VALUE_ARRAY);

    for(int i = 0; i < tPeakLuminance.num_rows_actual_peak_luminance; i++)
    {
      auto& tRow = tPLObject.PushBackValue(TJsonValue::JSON_VALUE_ARRAY);

      for(int j = 0; j < tPeakLuminance.num_cols_actual_peak_luminance; j++)
        tRow.PushBackValue(static_cast<int>(tPeakLuminance.actual_peak_luminance[i][j]));
    }
  }
}
