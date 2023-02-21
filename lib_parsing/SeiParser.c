// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "SeiParser.h"
#include "lib_common/SeiInternal.h"
#include "lib_common/SyntaxConversion.h"

#include "lib_common/HDR.h"

/*****************************************************************************/
void sei_get_uuid_iso_iec_11578(AL_TRbspParser* pRP, uint8_t* uuid)
{
  for(int i = 0; i < 16; i++)
    uuid[i] = getbyte(pRP);
}

/*****************************************************************************/
static bool SeiRecoveryPoint(AL_TRbspParser* pRP, AL_TRecoveryPoint* pRecoveryPoint)
{
  Rtos_Memset(pRecoveryPoint, 0, sizeof(*pRecoveryPoint));

  pRecoveryPoint->recovery_cnt = ue(pRP);
  pRecoveryPoint->exact_match = u(pRP, 1);
  pRecoveryPoint->broken_link = u(pRP, 1);

  /*changing_slice_group_idc = */ u(pRP, 2);

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool SeiMasteringDisplayColourVolume(AL_TMasteringDisplayColourVolume* pMDCV, AL_TRbspParser* pRP)
{
  Rtos_Memset(pMDCV, 0, sizeof(*pMDCV));

  for(int c = 0; c < 3; c++)
  {
    pMDCV->display_primaries[c].x = u(pRP, 16);
    pMDCV->display_primaries[c].y = u(pRP, 16);
  }

  pMDCV->white_point.x = u(pRP, 16);
  pMDCV->white_point.y = u(pRP, 16);

  pMDCV->max_display_mastering_luminance = u(pRP, 32);
  pMDCV->min_display_mastering_luminance = u(pRP, 32);

  if(byte_aligned(pRP))
    return true;

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool SeiContentLightLevel(AL_TContentLightLevel* pCLL, AL_TRbspParser* pRP)
{
  Rtos_Memset(pCLL, 0, sizeof(*pCLL));

  pCLL->max_content_light_level = u(pRP, 16);
  pCLL->max_pic_average_light_level = u(pRP, 16);

  if(byte_aligned(pRP))
    return true;

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool SeiAlternativeTransferCharacteristics(AL_TAlternativeTransferCharacteristics* pATC, AL_TRbspParser* pRP)
{
  pATC->preferred_transfer_characteristics = AL_VUIValueToTransferCharacteristics(u(pRP, 8));

  if(byte_aligned(pRP))
    return true;

  return byte_alignment(pRP);
}

/*****************************************************************************/
static AL_EUserDataRegisterSEIType SeiUserDataRegistered(AL_TRbspParser* pRP, uint32_t payload_size)
{
#define MIN_SEI_USER_DATA_SIZE 3

  if(u(pRP, 8) != 0xB5 || payload_size < MIN_SEI_USER_DATA_SIZE)
    return AL_UDR_SEI_UNKNOWN;

  uint16_t itu_t_t35_terminal_provider_code = u(pRP, 16);

  if(itu_t_t35_terminal_provider_code == 0x3B)
  {
    if(u(pRP, 32) == 0x00 && u(pRP, 8) == 0x09)
      return AL_UDR_SEI_ST2094_10;
  }
  else if(itu_t_t35_terminal_provider_code == 0x3C)
  {
    if(u(pRP, 16) == 0x01)
      return AL_UDR_SEI_ST2094_40;
  }

  return AL_UDR_SEI_UNKNOWN;
}

/*****************************************************************************/
static bool SeiSt2094_10(AL_TDynamicMeta_ST2094_10* pST2094_10, AL_TRbspParser* pRP)
{
  if(ue(pRP) != 1)
    return false;

  bool bImageCharacteristicsParsed = false;

  pST2094_10->application_version = ue(pRP);

  if(pST2094_10->application_version != 0)
    return false;

  bool metadata_refresh_flag = u(pRP, 1);

  if(metadata_refresh_flag)
  {
    pST2094_10->processing_window_flag = false;
    pST2094_10->num_manual_adjustments = 0;

    uint16_t num_ext_block = ue(pRP);

    if(num_ext_block > 0)
    {
      if(!simple_byte_alignment(pRP, 0))
        return false;

      for(int iBlock = 0; iBlock < num_ext_block; iBlock++)
      {
        uint16_t ext_block_length = ue(pRP);
        uint8_t ext_block_level = u(pRP, 8);

        uint16_t ext_block_len_bits = 8 * ext_block_length;
        uint16_t ext_block_use_bits = 0;
        switch(ext_block_level)
        {
        case 1:
        {
          if(bImageCharacteristicsParsed || ext_block_length < 5)
            return false;
          pST2094_10->image_characteristics.min_pq = u(pRP, 12);
          pST2094_10->image_characteristics.max_pq = u(pRP, 12);
          pST2094_10->image_characteristics.avg_pq = u(pRP, 12);
          ext_block_use_bits += 36;
          bImageCharacteristicsParsed = true;
          break;
        }
        case 2:
        {
          if(pST2094_10->num_manual_adjustments > 15 || ext_block_length < 11)
            return false;
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].target_max_pq = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].trim_slope = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].trim_offset = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].trim_power = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].trim_chroma_weight = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].trim_saturation_gain = u(pRP, 12);
          pST2094_10->manual_adjustments[pST2094_10->num_manual_adjustments].ms_weight = i(pRP, 13);
          ext_block_use_bits += 85;
          pST2094_10->num_manual_adjustments++;
          break;
        }
        case 5:
        {
          if(pST2094_10->processing_window_flag || ext_block_length < 7)
            return false;
          pST2094_10->processing_window.active_area_left_offset = u(pRP, 13);
          pST2094_10->processing_window.active_area_right_offset = u(pRP, 13);
          pST2094_10->processing_window.active_area_top_offset = u(pRP, 13);
          pST2094_10->processing_window.active_area_bottom_offset = u(pRP, 13);
          ext_block_use_bits += 52;
          pST2094_10->processing_window_flag = true;
          break;
        }
        default:
          break;
        }

        skip(pRP, ext_block_len_bits - ext_block_use_bits);
      }
    }
  }

  if(!simple_byte_alignment(pRP, 0))
    return false;

  // Reserved 0xFF bits
  skip(pRP, 8);

  return bImageCharacteristicsParsed;
}

/*****************************************************************************/
static void SeiSt2094_40_peakluminance(AL_TDisplayPeakLuminance_ST2094_40* pPeakLuminance, AL_TRbspParser* pRP)
{
  pPeakLuminance->actual_peak_luminance_flag = u(pRP, 1);

  if(pPeakLuminance->actual_peak_luminance_flag)
  {
    pPeakLuminance->num_rows_actual_peak_luminance = u(pRP, 5);
    pPeakLuminance->num_cols_actual_peak_luminance = u(pRP, 5);

    for(int i = 0; i < pPeakLuminance->num_rows_actual_peak_luminance; i++)
      for(int j = 0; j < pPeakLuminance->num_cols_actual_peak_luminance; j++)
        pPeakLuminance->actual_peak_luminance[i][j] = u(pRP, 4);
  }
}

/*****************************************************************************/
bool SeiSt2094_40(AL_TDynamicMeta_ST2094_40* pST2094_40, AL_TRbspParser* pRP)
{
  if(u(pRP, 8) != 4)
    return false;

  pST2094_40->application_version = u(pRP, 8);

  if(pST2094_40->application_version != 0)
    return false;

  pST2094_40->num_windows = u(pRP, 2);

  for(int iWin = 0; iWin < pST2094_40->num_windows - 1; iWin++)
  {
    AL_TProcessingWindow_ST2094_40* pWin = &pST2094_40->processing_windows[iWin];
    pWin->base_processing_window.upper_left_corner_x = u(pRP, 16);
    pWin->base_processing_window.upper_left_corner_y = u(pRP, 16);
    pWin->base_processing_window.lower_right_corner_x = u(pRP, 16);
    pWin->base_processing_window.lower_right_corner_y = u(pRP, 16);
    pWin->center_of_ellipse_x = u(pRP, 16);
    pWin->center_of_ellipse_y = u(pRP, 16);
    pWin->rotation_angle = u(pRP, 8);
    pWin->semimajor_axis_internal_ellipse = u(pRP, 16);
    pWin->semimajor_axis_external_ellipse = u(pRP, 16);
    pWin->semiminor_axis_external_ellipse = u(pRP, 16);
    pWin->overlap_process_option = u(pRP, 1);
  }

  pST2094_40->targeted_system_display.maximum_luminance = u(pRP, 27);
  SeiSt2094_40_peakluminance(&pST2094_40->targeted_system_display.peak_luminance, pRP);

  for(int iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    for(int i = 0; i < 3; i++)
      pWinTransfo->maxscl[i] = u(pRP, 17);

    pWinTransfo->average_maxrgb = u(pRP, 17);
    pWinTransfo->num_distribution_maxrgb_percentiles = u(pRP, 4);

    for(int i = 0; i < pWinTransfo->num_distribution_maxrgb_percentiles; i++)
    {
      pWinTransfo->distribution_maxrgb_percentages[i] = u(pRP, 7);
      pWinTransfo->distribution_maxrgb_percentiles[i] = u(pRP, 17);
    }

    pWinTransfo->fraction_bright_pixels = u(pRP, 10);
  }

  SeiSt2094_40_peakluminance(&pST2094_40->mastering_display_peak_luminance, pRP);

  for(int iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    AL_TToneMapping_ST2094_40* pToneMapping = &pWinTransfo->tone_mapping;
    pToneMapping->tone_mapping_flag = u(pRP, 1);

    if(pToneMapping->tone_mapping_flag)
    {
      pToneMapping->knee_point_x = u(pRP, 12);
      pToneMapping->knee_point_y = u(pRP, 12);
      pToneMapping->num_bezier_curve_anchors = u(pRP, 4);

      for(int i = 0; i < pToneMapping->num_bezier_curve_anchors; i++)
        pToneMapping->bezier_curve_anchors[i] = u(pRP, 10);
    }

    pWinTransfo->color_saturation_mapping_flag = u(pRP, 1);

    if(pWinTransfo->color_saturation_mapping_flag)
      pWinTransfo->color_saturation_weight = u(pRP, 6);
  }

  if(byte_aligned(pRP))
    return true;

  return byte_alignment(pRP);
}

/*****************************************************************************/
static bool ParseCommonSei(SeiParserParam* pParam, AL_TRbspParser* pRP, AL_ESeiPayloadType ePayloadType, uint32_t uPayloadSize, bool* bCanSendToUser)
{
  bool bParsingOk = true;
  *bCanSendToUser = true;
  switch(ePayloadType)
  {
  case SEI_PTYPE_RECOVERY_POINT: // picture_timing parsing
  {
    AL_TRecoveryPoint tRecoveryPoint;
    bParsingOk = SeiRecoveryPoint(pRP, &tRecoveryPoint);
    pParam->pIAup->iRecoveryCnt = tRecoveryPoint.recovery_cnt + 1; // +1 for non-zero value when AL_SEI_RP is present
    break;
  }
  case SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
  {
    bParsingOk = SeiMasteringDisplayColourVolume(&pParam->pIAup->tParsedHDRSEIs.tMDCV, pRP);
    pParam->pIAup->tParsedHDRSEIs.bHasMDCV = true;
    bCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_CONTENT_LIGHT_LEVEL:
  {
    bParsingOk = SeiContentLightLevel(&pParam->pIAup->tParsedHDRSEIs.tCLL, pRP);
    pParam->pIAup->tParsedHDRSEIs.bHasCLL = true;
    bCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
  {
    bParsingOk = SeiAlternativeTransferCharacteristics(&pParam->pIAup->tParsedHDRSEIs.tATC, pRP);
    pParam->pIAup->tParsedHDRSEIs.bHasATC = true;
    bCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_USER_DATA_REGISTERED:
  {
    AL_EUserDataRegisterSEIType eSEIType;
    bParsingOk = ((eSEIType = SeiUserDataRegistered(pRP, uPayloadSize)) != AL_UDR_SEI_UNKNOWN);

    if(!bParsingOk)
      break;
    switch(eSEIType)
    {
    case AL_UDR_SEI_ST2094_10:
    {
      bParsingOk = SeiSt2094_10(&pParam->pIAup->tParsedHDRSEIs.tST2094_10, pRP);
      pParam->pIAup->tParsedHDRSEIs.bHasST2094_10 = true;
      break;
    }
    case AL_UDR_SEI_ST2094_40:
    {
      bParsingOk = SeiSt2094_40(&pParam->pIAup->tParsedHDRSEIs.tST2094_40, pRP);
      pParam->pIAup->tParsedHDRSEIs.bHasST2094_40 = true;
      break;
    }
    default:
    {
      AL_Assert(0);
      break;
    }
    }

    bCanSendToUser = false;
    break;
  }
  default: // Payload not supported
  {
    skip(pRP, uPayloadSize << 3);
    break;
  }
  }

  return bParsingOk;
}

// For tomorow, remove PARSE_OR_SKIP
/*****************************************************************************/
bool ParseSeiHeader(AL_TRbspParser* pRP, SeiParserCB* pCB)
{
  do
  {
    uint32_t uPayloadType = 0;
    uint32_t uPayloadSize = 0;

    // Get payload type
    // ----------------
    if(!byte_aligned(pRP))
      return false;

    uint8_t byte = getbyte(pRP);

    while(byte == 0xff)
    {
      uPayloadType += 255;
      byte = getbyte(pRP);
    }

    uPayloadType += byte;
    AL_ESeiPayloadType ePayloadType = (AL_ESeiPayloadType)uPayloadType;

    // Get payload size
    // ----------------
    byte = getbyte(pRP);

    while(byte == 0xff)
    {
      uPayloadSize += 255;
      byte = getbyte(pRP);
    }

    uPayloadSize += byte;

    // Parse the payload
    // -----------------
    uint32_t uOffsetBefore = offset(pRP);
    bool bCanSendToUser = true;
    bool bParsed = false;
    bool bParsingOk = true;
    uint8_t* pPayloadData = get_raw_data(pRP);

    // Call codec specific SEI parsing
    if(pCB)
      bParsingOk = pCB->func(pCB->pParam, pRP, ePayloadType, uPayloadSize, &bCanSendToUser, &bParsed);

    // If the codec specific hasn't parsed, try the common SEI payload
    if(!bParsed)
      bParsingOk = ParseCommonSei(pCB->pParam, pRP, ePayloadType, uPayloadSize, &bCanSendToUser);

    // Try to catch up if the parsing was bad
    if(!bParsingOk)
    {
      uint32_t uReadSize = offset(pRP) - uOffsetBefore;

      if(uReadSize > uPayloadSize << 3)
        return false;

      skip(pRP, (uPayloadSize << 3) - uReadSize);
    }

    // Skip remaining payload
    // ----------------------
    uint32_t uOffsetAfter = offset(pRP);
    int32_t iRemainingPayload = (uPayloadSize << 3) - (int32_t)(uOffsetAfter - uOffsetBefore);

    if(iRemainingPayload > 0)
      skip(pRP, iRemainingPayload);

    // Attach sei to the SeiMetaData
    // -----------------------------
    if(bCanSendToUser && pCB->pParam->pMeta)
      if(!AL_SeiMetaData_AddPayload(pCB->pParam->pMeta, (AL_TSeiMessage) {pCB->pParam->bIsPrefix, ePayloadType, pPayloadData, uPayloadSize }))
        return false;

    // Send sei to the user
    // --------------------
    if(bCanSendToUser && pCB->pParam->cb->func)
      pCB->pParam->cb->func(pCB->pParam->bIsPrefix, ePayloadType, pPayloadData, uPayloadSize, pCB->pParam->cb->userParam);
  }
  while(more_rbsp_data(pRP));

  return true;
}
