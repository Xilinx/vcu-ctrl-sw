// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_common
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufConst.h"
#include "lib_common/SEI.h"
#include "common_syntax_elements.h"

/*****************************************************************************
   \brief SEI payload types
*****************************************************************************/
typedef enum
{
  SEI_PTYPE_BUFFERING_PERIOD = 0,
  SEI_PTYPE_PIC_TIMING = 1,
  SEI_PTYPE_USER_DATA_REGISTERED = 4,
  SEI_PTYPE_USER_DATA_UNREGISTERED = 5,
  SEI_PTYPE_RECOVERY_POINT = 6,
  SEI_PTYPE_ACTIVE_PARAMETER_SETS = 129,
  SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME = 137,
  SEI_PTYPE_CONTENT_LIGHT_LEVEL = 144,
  SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS = 147
}AL_ESeiPayloadType;

/*****************************************************************************
   \brief User Data Registered SEI types
*****************************************************************************/
typedef enum
{
  AL_UDR_SEI_UNKNOWN,
  AL_UDR_SEI_ST2094_10,
  AL_UDR_SEI_ST2094_40,
}AL_EUserDataRegisterSEIType;

/*****************************************************************************
   \brief User Data Unregistered SEI types
*****************************************************************************/
typedef enum
{
  AL_UDU_SEI_UNKNOWN,
  AL_UDU_SEI_ALLEGRO_NUM_SLICES,
}AL_EUserDataUnregisterSEIType;

/*************************************************************************/
typedef struct AL_TRecoveryPoint
{
  int32_t recovery_cnt;
  bool exact_match;
  bool broken_link;
}AL_TRecoveryPoint;

/*****************************************************************************
   \brief Mimics structure for AVC SEI buffering period
*****************************************************************************/
typedef struct AL_TAvcBufPeriod
{
  uint8_t seq_parameter_set_id;
  uint32_t initial_cpb_removal_delay[32]; // E.2.2 : cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive
  uint32_t initial_cpb_removal_delay_offset[32];
}AL_TAvcBufPeriod;

/*****************************************************************************
   \brief Mimics structure for AVC picture timing
*****************************************************************************/
typedef struct AL_TAvcSeiClockTS
{
  uint8_t clock_time_stamp_flag;
  uint8_t ct_type;

  uint8_t nuit_field_based_flag;
  uint8_t counting_type;
  uint8_t full_time_stamp_flag;
  uint8_t discontinuity_flag;
  uint8_t cnt_dropped_flag;
  uint8_t n_frames;

  uint8_t seconds_value;
  uint8_t minutes_value;
  uint8_t hours_value;

  uint8_t seconds_flag;
  uint8_t minutes_flag;
  uint8_t hours_flag;

  int32_t time_offset;
}AL_TAvcSeiClockTS;

/*************************************************************************/
typedef struct AL_TAvcPicTiming
{
  uint32_t cpb_removal_delay;
  uint32_t dpb_output_delay;
  uint8_t pic_struct;
  AL_TAvcSeiClockTS clock_ts[4];
}AL_TAvcPicTiming;

/*****************************************************************************
   \brief AVC Supplemental enhancement information structure
*****************************************************************************/
typedef struct AL_TAvcSei
{
  AL_ESeiFlag present_flags;
  AL_TAvcPicTiming picture_timing;
  AL_TAvcBufPeriod buffering_period;
  AL_TRecoveryPoint recovery_point;
}AL_TAvcSei;

/*****************************************************************************
   \brief Mimics structure for HEVC SEI buffering period
*****************************************************************************/
typedef struct AL_THevcBufPeriod
{
  uint8_t bp_seq_parameter_set_id;
  uint8_t rap_cpb_params_present_flag;

  uint32_t cpb_delay_offset;
  uint32_t dpb_delay_offset;

  uint8_t concatenation_flag;
  uint32_t au_cpb_removal_delay_delta_minus1;

  uint32_t nal_initial_cpb_removal_delay[6]; // E.2.2 : cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive
  uint32_t nal_initial_cpb_removal_delay_offset[6];
  uint32_t nal_initial_alt_cpb_removal_delay[6];
  uint32_t nal_initial_alt_cpb_removal_delay_offset[6];

  uint32_t vcl_initial_cpb_removal_delay[6]; // E.2.2 : cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive
  uint32_t vcl_initial_cpb_removal_delay_offset[6];
  uint32_t vcl_initial_alt_cpb_removal_delay[6];
  uint32_t vcl_initial_alt_cpb_removal_delay_offset[6];
}AL_THevcBufPeriod;

/*****************************************************************************
   \brief Mimics structure for HEVC picture timing
*****************************************************************************/
typedef struct AL_THevcPicTiming
{
  uint8_t pic_struct;
  uint8_t source_scan_type;
  uint8_t duplicate_flag;

  uint32_t au_cpb_removal_delay_minus1;
  uint32_t pic_dpb_output_delay;
  uint32_t pic_dpb_output_du_delay;

  uint32_t num_decoding_units_minus1;
  uint8_t du_common_cpb_removal_delay_flag;
  uint32_t du_common_cpb_removal_delay_increment_minus1;
  uint32_t* num_nalus_in_du_minus1;
  uint32_t* du_cpb_removal_delay_increment_minus1;
}AL_THevcPicTiming;

/*****************************************************************************
   \brief HEVC Supplemental enhancement information structure
*****************************************************************************/
typedef struct AL_THevcSei
{
  AL_ESeiFlag present_flags;
  AL_THevcPicTiming picture_timing;
  AL_THevcBufPeriod buffering_period;
  AL_TRecoveryPoint recovery_point;
}AL_THevcSei;

/****************************************************************************/
#define UUID_SIZE 16
static uint8_t const ALLEGRO_NUM_SLICES_SEI_UUID[UUID_SIZE] =
{
  0xb1, 0xe1, 0x67, 0xa4, 0xd9, 0xca, 0x11, 0xe7,
  0xb1, 0x9b, 0x00, 0x50, 0xc2, 0x49, 0x00, 0x48
};

/*!@}*/
