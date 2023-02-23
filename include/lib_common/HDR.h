// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"
/*************************************************************************//*!
   \brief Colour Description identifier (See ISO/IEC 23091-4 or ITU-T H.273)
*****************************************************************************/
typedef enum e_ColourDescription
{
  AL_COLOUR_DESC_RESERVED,
  AL_COLOUR_DESC_UNSPECIFIED,
  AL_COLOUR_DESC_BT_470_NTSC,
  AL_COLOUR_DESC_BT_601_NTSC,
  AL_COLOUR_DESC_BT_601_PAL,
  AL_COLOUR_DESC_BT_709,
  AL_COLOUR_DESC_BT_2020,
  AL_COLOUR_DESC_SMPTE_240M,
  AL_COLOUR_DESC_SMPTE_ST_428,
  AL_COLOUR_DESC_SMPTE_RP_431,
  AL_COLOUR_DESC_SMPTE_EG_432,
  AL_COLOUR_DESC_EBU_3213,
  AL_COLOUR_DESC_GENERIC_FILM,
  AL_COLOUR_DESC_MAX_ENUM,
}AL_EColourDescription;

/************************************//*!
   \brief Transfer Function identifier
****************************************/
typedef enum e_TransferCharacteristics
{
  AL_TRANSFER_CHARAC_BT_709 = 1,
  AL_TRANSFER_CHARAC_UNSPECIFIED = 2,
  AL_TRANSFER_CHARAC_BT_470_SYSTEM_M = 4,
  AL_TRANSFER_CHARAC_BT_470_SYSTEM_B = 5,
  AL_TRANSFER_CHARAC_BT_601 = 6,
  AL_TRANSFER_CHARAC_SMPTE_240M = 7,
  AL_TRANSFER_CHARAC_LINEAR = 8,
  AL_TRANSFER_CHARAC_LOG = 9,
  AL_TRANSFER_CHARAC_LOG_EXTENDED = 10,
  AL_TRANSFER_CHARAC_IEC_61966_2_4 = 11,
  AL_TRANSFER_CHARAC_BT_1361 = 12,
  AL_TRANSFER_CHARAC_IEC_61966_2_1 = 13,
  AL_TRANSFER_CHARAC_BT_2020_10B = 14,
  AL_TRANSFER_CHARAC_BT_2020_12B = 15,
  AL_TRANSFER_CHARAC_BT_2100_PQ = 16,
  AL_TRANSFER_CHARAC_SMPTE_428 = 17,
  AL_TRANSFER_CHARAC_BT_2100_HLG = 18,
  AL_TRANSFER_CHARAC_MAX_ENUM,
}AL_ETransferCharacteristics;

/*******************************************************************************//*!
   \brief Matrix coefficient identifier used for luma/chroma computation from RGB
***********************************************************************************/
typedef enum e_ColourMatrixCoefficients
{
  AL_COLOUR_MAT_COEFF_GBR = 0,
  AL_COLOUR_MAT_COEFF_BT_709 = 1,
  AL_COLOUR_MAT_COEFF_UNSPECIFIED = 2,
  AL_COLOUR_MAT_COEFF_USFCC_CFR = 4,
  AL_COLOUR_MAT_COEFF_BT_601_625 = 5,
  AL_COLOUR_MAT_COEFF_BT_601_525 = 6,
  AL_COLOUR_MAT_COEFF_BT_SMPTE_240M = 7,
  AL_COLOUR_MAT_COEFF_BT_YCGCO = 8,
  AL_COLOUR_MAT_COEFF_BT_2100_YCBCR = 9,
  AL_COLOUR_MAT_COEFF_BT_2020_CLS = 10,
  AL_COLOUR_MAT_COEFF_SMPTE_2085 = 11,
  AL_COLOUR_MAT_COEFF_CHROMA_DERIVED_NCLS = 12,
  AL_COLOUR_MAT_COEFF_CHROMA_DERIVED_CLS = 13,
  AL_COLOUR_MAT_COEFF_BT_2100_ICTCP = 14,
  AL_COLOUR_MAT_COEFF_MAX_ENUM,
}AL_EColourMatrixCoefficients;

/*******************************************************************************//*!
   \brief Normalized x and y chromaticity coordinates
***********************************************************************************/
typedef struct AL_t_ChromaCoordinates
{
  uint16_t x;
  uint16_t y;
}AL_TChromaCoordinates;

/*************************************************************************//*!
   \brief Mimics structure for mastering display colour volume
*****************************************************************************/
typedef struct AL_t_MasteringDisplayColourVolume
{
  /* RGB chromaticity coordinates (CIE 1931 definition of x and y as specified in ISO 11664 - 1)
     in increments of 0.00002.
     Allowed values: [0;50000] */
  AL_TChromaCoordinates display_primaries[3];
  /* White point chromaticity coordinates (CIE 1931 definition of x and y as specified in ISO
     11664 - 1) in increments of 0.00002.
     Allowed values: [0;50000] */
  AL_TChromaCoordinates white_point;
  /* Maximum display luminance in units of 0.0001 cd/m2 */
  uint32_t max_display_mastering_luminance;
  /* Minimum display luminance in units of 0.0001 cd/m2 */
  uint32_t min_display_mastering_luminance;
}AL_TMasteringDisplayColourVolume;

/*************************************************************************//*!
   \brief Mimics structure for content light level information
*****************************************************************************/
typedef struct AL_t_ContentLightLevel
{
  uint16_t max_content_light_level;
  uint16_t max_pic_average_light_level;
}AL_TContentLightLevel;

/*************************************************************************//*!
   \brief Mimics structure for alternative transfer characteristic information
*****************************************************************************/
typedef struct AL_t_AlternativeTransferCharacteristics
{
  AL_ETransferCharacteristics preferred_transfer_characteristics;
}AL_TAlternativeTransferCharacteristics;

/*************************************************************************//*!
   \brief Mimics structure for dynamic metadata for color volume transform specified
   in SMPTE ST 2094-10 and carried under the ETSI TS 103 572 V1.1.1 specification
*****************************************************************************/
#define AL_MAX_MANUAL_ADJUSTMENT_ST2094_10 16

typedef struct AL_t_ProcessingWindow_ST2094_10
{
  uint16_t active_area_left_offset;
  uint16_t active_area_right_offset;
  uint16_t active_area_top_offset;
  uint16_t active_area_bottom_offset;
}AL_TProcessingWindow_ST2094_10;

typedef struct AL_t_TImageCharacteristics_ST2094_10
{
  uint16_t min_pq;
  uint16_t max_pq;
  uint16_t avg_pq;
}AL_TImageCharacteristics_ST2094_10;

typedef struct AL_t_ManualAdjustment_ST2094_10
{
  uint16_t target_max_pq;
  uint16_t trim_slope;
  uint16_t trim_offset;
  uint16_t trim_power;
  uint16_t trim_chroma_weight;
  uint16_t trim_saturation_gain;
  int16_t ms_weight;
}AL_TManualAdjustment_ST2094_10;

typedef struct AL_t_DynamicMeta_ST2094_10
{
  uint8_t application_version; /* = 0 */
  bool processing_window_flag;
  AL_TProcessingWindow_ST2094_10 processing_window;
  AL_TImageCharacteristics_ST2094_10 image_characteristics;
  uint8_t num_manual_adjustments;
  AL_TManualAdjustment_ST2094_10 manual_adjustments[AL_MAX_MANUAL_ADJUSTMENT_ST2094_10];
}AL_TDynamicMeta_ST2094_10;

/*************************************************************************//*!
   \brief Mimics structure for dynamic metadata for color volume transform specified
   in SMPTE ST 2094-40 and carried under CTA-861 interface
*****************************************************************************/
#define AL_MIN_WINDOW_ST2094_40 1
#define AL_MAX_WINDOW_ST2094_40 3
#define AL_MAX_MAXRGB_PERCENTILES_ST2094_40 15
#define AL_MAX_BEZIER_CURVE_ANCHORS_ST2094_40 15
#define AL_MAX_ROW_ACTUAL_PEAK_LUMINANCE_ST2094_40 25
#define AL_MAX_COL_ACTUAL_PEAK_LUMINANCE_ST2094_40 25

typedef struct AL_t_ProcessingWindow_ST2094_1
{
  uint16_t upper_left_corner_x;
  uint16_t upper_left_corner_y;
  uint16_t lower_right_corner_x;
  uint16_t lower_right_corner_y;
}AL_TProcessingWindow_ST2094_1;

typedef struct AL_t_ProcessingWindow_ST2094_40
{
  AL_TProcessingWindow_ST2094_1 base_processing_window;
  uint16_t center_of_ellipse_x;
  uint16_t center_of_ellipse_y;
  uint8_t rotation_angle;
  uint16_t semimajor_axis_internal_ellipse;
  uint16_t semimajor_axis_external_ellipse;
  uint16_t semiminor_axis_external_ellipse;
  uint8_t overlap_process_option;
}AL_TProcessingWindow_ST2094_40;

typedef struct AL_t_DisplayPeakLuminance_ST2094_40
{
  bool actual_peak_luminance_flag;
  uint8_t num_rows_actual_peak_luminance;
  uint8_t num_cols_actual_peak_luminance;
  uint8_t actual_peak_luminance[AL_MAX_ROW_ACTUAL_PEAK_LUMINANCE_ST2094_40][AL_MAX_COL_ACTUAL_PEAK_LUMINANCE_ST2094_40];
}AL_TDisplayPeakLuminance_ST2094_40;

typedef struct AL_t_TargetedSystemDisplay_ST2094_40
{
  uint32_t maximum_luminance;
  AL_TDisplayPeakLuminance_ST2094_40 peak_luminance;
}AL_TTargetedSystemDisplay_ST2094_40;

typedef struct AL_t_ToneMapping_ST2094_40
{
  bool tone_mapping_flag;
  uint16_t knee_point_x;
  uint16_t knee_point_y;
  uint8_t num_bezier_curve_anchors;
  uint16_t bezier_curve_anchors[AL_MAX_BEZIER_CURVE_ANCHORS_ST2094_40];
}AL_TToneMapping_ST2094_40;

typedef struct AL_t_ProcessingWindowTransform_ST2094_40
{
  uint32_t maxscl[3];
  uint32_t average_maxrgb;
  uint8_t num_distribution_maxrgb_percentiles;
  uint8_t distribution_maxrgb_percentages[AL_MAX_MAXRGB_PERCENTILES_ST2094_40];
  uint32_t distribution_maxrgb_percentiles[AL_MAX_MAXRGB_PERCENTILES_ST2094_40];
  uint8_t fraction_bright_pixels;
  AL_TToneMapping_ST2094_40 tone_mapping;
  bool color_saturation_mapping_flag;
  uint8_t color_saturation_weight;
}AL_TProcessingWindowTransform_ST2094_40;

typedef struct AL_t_DynamicMeta_ST2094_40
{
  uint8_t application_version;
  uint8_t num_windows;
  AL_TProcessingWindow_ST2094_40 processing_windows[AL_MAX_WINDOW_ST2094_40 - 1];
  AL_TTargetedSystemDisplay_ST2094_40 targeted_system_display;
  AL_TDisplayPeakLuminance_ST2094_40 mastering_display_peak_luminance;
  AL_TProcessingWindowTransform_ST2094_40 processing_window_transforms[AL_MAX_WINDOW_ST2094_40];
}AL_TDynamicMeta_ST2094_40;

/*************************************************************************//*!
   \brief Mimics structure containing HDR Related SEIs
*****************************************************************************/
typedef struct AL_t_HDRSEIs
{
  bool bHasMDCV;
  AL_TMasteringDisplayColourVolume tMDCV;

  bool bHasCLL;
  AL_TContentLightLevel tCLL;

  bool bHasATC;
  AL_TAlternativeTransferCharacteristics tATC;

  bool bHasST2094_10;
  AL_TDynamicMeta_ST2094_10 tST2094_10;

  bool bHasST2094_40;
  AL_TDynamicMeta_ST2094_40 tST2094_40;
}AL_THDRSEIs;

/*************************************************************************//*!
   \brief Initialize HDR SEIs structure
   \param[in] pHDRSEIs Pointer to the HDR SEIs
*****************************************************************************/
void AL_HDRSEIs_Reset(AL_THDRSEIs* pHDRSEIs);

/*************************************************************************//*!
   \brief Copy HDR SEIs
   \param[in] pHDRSEIsSrc Pointer to the source HDR SEIs
   \param[out] pHDRSEIsDst Pointer to the destination HDR SEIs
*****************************************************************************/
void AL_HDRSEIs_Copy(AL_THDRSEIs* pHDRSEIsSrc, AL_THDRSEIs* pHDRSEIsDst);

/*@}*/

