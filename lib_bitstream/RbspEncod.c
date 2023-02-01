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

/****************************************************************************
   -----------------------------------------------------------------------------
**************************************************************************//*!
   \addtogroup lib_bitstream
   @{
****************************************************************************/

#include "RbspEncod.h"
#include "lib_assert/al_assert.h"
#include "lib_common/SyntaxConversion.h"

/*****************************************************************************/
void AL_RbspEncoding_WriteAUD(AL_TBitStreamLite* pBS, AL_TAud const* pAud)
{
  // 1 - Write primary_pic_type.

  static int const SliceTypeToPrimaryPicType[] = { 2, 1, 0, 4, 3, 7, 1, 7, 7 };
  AL_BitStreamLite_PutU(pBS, 3, SliceTypeToPrimaryPicType[pAud->eType]);

  // 2 - Write rbsp_trailing_bits.

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
int AL_RbspEncoding_BeginSEI(AL_TBitStreamLite* pBS, uint8_t uPayloadType)
{
  AL_BitStreamLite_PutBits(pBS, 8, uPayloadType);
  int bookmarkSEI = AL_BitStreamLite_GetBitsCount(pBS);
  AL_Assert(bookmarkSEI % 8 == 0);

  AL_BitStreamLite_PutBits(pBS, 8, 0xFF);

  return bookmarkSEI;
}

static void PutUV(AL_TBitStreamLite* pBS, int32_t iValue)
{
  while(iValue > 255)
  {
    AL_BitStreamLite_PutU(pBS, 8, 0xFF);
    iValue -= 255;
  }

  AL_BitStreamLite_PutU(pBS, 8, iValue);
}

void AL_RbspEncoding_BeginSEI2(AL_TBitStreamLite* pBS, int iPayloadType, int iPayloadSize)
{
  /* See 7.3.5 Supplemental enhancement information message syntax */
  PutUV(pBS, iPayloadType);
  PutUV(pBS, iPayloadSize);
}

/******************************************************************************/
void AL_RbspEncoding_EndSEI(AL_TBitStreamLite* pBS, int bookmarkSEI)
{
  uint8_t* pSize = AL_BitStreamLite_GetData(pBS) + (bookmarkSEI / 8);
  AL_Assert(*pSize == 0xFF);
  int bits = AL_BitStreamLite_GetBitsCount(pBS) - bookmarkSEI;
  AL_Assert(bits % 8 == 0);
  *pSize = (bits / 8) - 1;
}

/******************************************************************************/
void AL_RbspEncoding_CloseSEI(AL_TBitStreamLite* pBS)
{
  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
void AL_RbspEncoding_WriteUserDataUnregistered(AL_TBitStreamLite* pBS, uint8_t uuid[16], int8_t numSlices)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 5);

  for(int i = 0; i < 16; i++)
    AL_BitStreamLite_PutU(pBS, 8, uuid[i]);

  AL_BitStreamLite_PutU(pBS, 8, numSlices);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
  AL_RbspEncoding_CloseSEI(pBS);
}

/******************************************************************************/
void AL_RbspEncoding_WriteMasteringDisplayColourVolume(AL_TBitStreamLite* pBS, AL_TMasteringDisplayColourVolume* pMDCV)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 137);

  for(int c = 0; c < 3; c++)
  {
    AL_BitStreamLite_PutU(pBS, 16, pMDCV->display_primaries[c].x);
    AL_BitStreamLite_PutU(pBS, 16, pMDCV->display_primaries[c].y);
  }

  AL_BitStreamLite_PutU(pBS, 16, pMDCV->white_point.x);
  AL_BitStreamLite_PutU(pBS, 16, pMDCV->white_point.y);

  AL_BitStreamLite_PutU(pBS, 32, pMDCV->max_display_mastering_luminance);
  AL_BitStreamLite_PutU(pBS, 32, pMDCV->min_display_mastering_luminance);

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
void AL_RbspEncoding_WriteContentLightLevel(AL_TBitStreamLite* pBS, AL_TContentLightLevel* pCLL)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 144);

  AL_BitStreamLite_PutU(pBS, 16, pCLL->max_content_light_level);
  AL_BitStreamLite_PutU(pBS, 16, pCLL->max_pic_average_light_level);

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
void AL_RbspEncoding_WriteAlternativeTransferCharacteristics(AL_TBitStreamLite* pBS, AL_TAlternativeTransferCharacteristics* pATC)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 147);

  AL_BitStreamLite_PutU(pBS, 8, AL_TransferCharacteristicsToVUIValue(pATC->preferred_transfer_characteristics));

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
void AL_RbspEncoding_WriteST2094_10(AL_TBitStreamLite* pBS, AL_TDynamicMeta_ST2094_10* pST2094_10)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 4);

  AL_BitStreamLite_PutU(pBS, 8, 0xB5);
  AL_BitStreamLite_PutU(pBS, 16, 0x3B);
  AL_BitStreamLite_PutU(pBS, 32, 0x00);
  AL_BitStreamLite_PutU(pBS, 8, 0x09);

  AL_BitStreamLite_PutUE(pBS, 1);
  AL_BitStreamLite_PutUE(pBS, pST2094_10->application_version);

  AL_BitStreamLite_PutBit(pBS, 1);

  uint32_t num_ext_blocks = pST2094_10->num_manual_adjustments + 1;

  if(pST2094_10->processing_window_flag)
    num_ext_blocks++;

  AL_BitStreamLite_PutUE(pBS, num_ext_blocks);

  AL_BitStreamLite_AlignWithBits(pBS, 0);

  // Image Characteristics
  AL_BitStreamLite_PutUE(pBS, 5);
  AL_BitStreamLite_PutU(pBS, 8, 1);
  AL_BitStreamLite_PutU(pBS, 12, pST2094_10->image_characteristics.min_pq);
  AL_BitStreamLite_PutU(pBS, 12, pST2094_10->image_characteristics.max_pq);
  AL_BitStreamLite_PutU(pBS, 12, pST2094_10->image_characteristics.avg_pq);
  AL_BitStreamLite_PutU(pBS, 4, 0);

  // Manual Adjustments
  for(int i = 0; i < pST2094_10->num_manual_adjustments; i++)
  {
    AL_BitStreamLite_PutUE(pBS, 11);
    AL_BitStreamLite_PutU(pBS, 8, 2);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].target_max_pq);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].trim_slope);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].trim_offset);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].trim_power);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].trim_chroma_weight);
    AL_BitStreamLite_PutU(pBS, 12, pST2094_10->manual_adjustments[i].trim_saturation_gain);
    AL_BitStreamLite_PutI(pBS, 13, pST2094_10->manual_adjustments[i].ms_weight);
    AL_BitStreamLite_PutU(pBS, 3, 0);
  }

  // Processing Window
  if(pST2094_10->processing_window_flag)
  {
    AL_BitStreamLite_PutUE(pBS, 7);
    AL_BitStreamLite_PutU(pBS, 8, 5);
    AL_BitStreamLite_PutU(pBS, 13, pST2094_10->processing_window.active_area_left_offset);
    AL_BitStreamLite_PutU(pBS, 13, pST2094_10->processing_window.active_area_right_offset);
    AL_BitStreamLite_PutU(pBS, 13, pST2094_10->processing_window.active_area_top_offset);
    AL_BitStreamLite_PutU(pBS, 13, pST2094_10->processing_window.active_area_bottom_offset);
    AL_BitStreamLite_PutU(pBS, 4, 0);
  }

  AL_BitStreamLite_AlignWithBits(pBS, 0);
  AL_BitStreamLite_PutU(pBS, 8, 0xFF);

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/******************************************************************************/
void WriteST2094_40_PeakLuminance(AL_TBitStreamLite* pBS, AL_TDisplayPeakLuminance_ST2094_40* pPeakLuminance)
{
  AL_BitStreamLite_PutBit(pBS, pPeakLuminance->actual_peak_luminance_flag);

  if(pPeakLuminance->actual_peak_luminance_flag)
  {
    AL_BitStreamLite_PutU(pBS, 5, pPeakLuminance->num_rows_actual_peak_luminance);
    AL_BitStreamLite_PutU(pBS, 5, pPeakLuminance->num_cols_actual_peak_luminance);

    for(int i = 0; i < pPeakLuminance->num_rows_actual_peak_luminance; i++)
      for(int j = 0; j < pPeakLuminance->num_cols_actual_peak_luminance; j++)
        AL_BitStreamLite_PutU(pBS, 4, pPeakLuminance->actual_peak_luminance[i][j]);
  }
}

void AL_RbspEncoding_WriteST2094_40(AL_TBitStreamLite* pBS, AL_TDynamicMeta_ST2094_40* pST2094_40)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 4);

  AL_BitStreamLite_PutU(pBS, 8, 0xB5);
  AL_BitStreamLite_PutU(pBS, 16, 0x3C);
  AL_BitStreamLite_PutU(pBS, 16, 0x01);

  AL_BitStreamLite_PutU(pBS, 8, 4);
  AL_BitStreamLite_PutU(pBS, 8, pST2094_40->application_version);

  AL_BitStreamLite_PutU(pBS, 2, pST2094_40->num_windows);

  for(int iWin = 0; iWin < pST2094_40->num_windows - 1; iWin++)
  {
    AL_TProcessingWindow_ST2094_40* pWin = &pST2094_40->processing_windows[iWin];
    AL_BitStreamLite_PutU(pBS, 16, pWin->base_processing_window.upper_left_corner_x);
    AL_BitStreamLite_PutU(pBS, 16, pWin->base_processing_window.upper_left_corner_y);
    AL_BitStreamLite_PutU(pBS, 16, pWin->base_processing_window.lower_right_corner_x);
    AL_BitStreamLite_PutU(pBS, 16, pWin->base_processing_window.lower_right_corner_y);
    AL_BitStreamLite_PutU(pBS, 16, pWin->center_of_ellipse_x);
    AL_BitStreamLite_PutU(pBS, 16, pWin->center_of_ellipse_y);
    AL_BitStreamLite_PutU(pBS, 8, pWin->rotation_angle);
    AL_BitStreamLite_PutU(pBS, 16, pWin->semimajor_axis_internal_ellipse);
    AL_BitStreamLite_PutU(pBS, 16, pWin->semimajor_axis_external_ellipse);
    AL_BitStreamLite_PutU(pBS, 16, pWin->semiminor_axis_external_ellipse);
    AL_BitStreamLite_PutU(pBS, 1, pWin->overlap_process_option);
  }

  AL_BitStreamLite_PutU(pBS, 27, pST2094_40->targeted_system_display.maximum_luminance);
  WriteST2094_40_PeakLuminance(pBS, &pST2094_40->targeted_system_display.peak_luminance);

  for(int iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    for(int i = 0; i < 3; i++)
      AL_BitStreamLite_PutU(pBS, 17, pWinTransfo->maxscl[i]);

    AL_BitStreamLite_PutU(pBS, 17, pWinTransfo->average_maxrgb);
    AL_BitStreamLite_PutU(pBS, 4, pWinTransfo->num_distribution_maxrgb_percentiles);

    for(int i = 0; i < pWinTransfo->num_distribution_maxrgb_percentiles; i++)
    {
      AL_BitStreamLite_PutU(pBS, 7, pWinTransfo->distribution_maxrgb_percentages[i]);
      AL_BitStreamLite_PutU(pBS, 17, pWinTransfo->distribution_maxrgb_percentiles[i]);
    }

    AL_BitStreamLite_PutU(pBS, 10, pWinTransfo->fraction_bright_pixels);
  }

  WriteST2094_40_PeakLuminance(pBS, &pST2094_40->mastering_display_peak_luminance);

  for(int iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    AL_TToneMapping_ST2094_40* pToneMapping = &pWinTransfo->tone_mapping;
    AL_BitStreamLite_PutBit(pBS, pToneMapping->tone_mapping_flag);

    if(pToneMapping->tone_mapping_flag)
    {
      AL_BitStreamLite_PutU(pBS, 12, pToneMapping->knee_point_x);
      AL_BitStreamLite_PutU(pBS, 12, pToneMapping->knee_point_y);
      AL_BitStreamLite_PutU(pBS, 4, pToneMapping->num_bezier_curve_anchors);

      for(int i = 0; i < pToneMapping->num_bezier_curve_anchors; i++)
        AL_BitStreamLite_PutU(pBS, 10, pToneMapping->bezier_curve_anchors[i]);
    }

    AL_BitStreamLite_PutBit(pBS, pWinTransfo->color_saturation_mapping_flag);

    if(pWinTransfo->color_saturation_mapping_flag)
      AL_BitStreamLite_PutU(pBS, 6, pWinTransfo->color_saturation_weight);
  }

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/*@}*/

