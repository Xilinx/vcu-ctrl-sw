// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "SeiParser.h"
#include "lib_common/SyntaxConversion.h"

#include "lib_common/HDR.h"

/*****************************************************************************/
void AL_SeiParser_Init(AL_TSeiParserCtx* pCtx, AL_TAup* pOutputAup, AL_TSeiMetaData* pOutputMeta, AL_CB_ParsedSei* pSeiParsedCallback)
{
  pCtx->pOutputAup = pOutputAup;
  pCtx->pOutputMeta = pOutputMeta;
  pCtx->pSeiParsedCallback = pSeiParsedCallback;
  pCtx->pfnCustomSeiParsing = NULL;
}

/*****************************************************************************/
void AL_SeiParser_AddCustomSeiParsing(AL_TSeiParserCtx* pCtx, AL_PFN_ParseOneSei pfnCustomSeiParsing)
{
  pCtx->pfnCustomSeiParsing = pfnCustomSeiParsing;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiRecoveryPoint(AL_TRbspParser* pRP, AL_TRecoveryPoint* pRecoveryPoint)
{
  Rtos_Memset(pRecoveryPoint, 0, sizeof(*pRecoveryPoint));

  pRecoveryPoint->recovery_cnt = ue(pRP);
  pRecoveryPoint->exact_match = u(pRP, 1);
  pRecoveryPoint->broken_link = u(pRP, 1);

  /*changing_slice_group_idc = */ u(pRP, 2);
  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
typedef enum
{
  AL_UDR_SEI_COUNTRY_CODE_UK = 0xB4,
  AL_UDR_SEI_COUNTRY_CODE_USA = 0xB5,
}AL_EUserDataRegisterSEICountryCode;

/*****************************************************************************/
static AL_ESeiParseResult SeiMasteringDisplayColourVolume(AL_TMasteringDisplayColourVolume* pMDCV, AL_TRbspParser* pRP)
{
  Rtos_Memset(pMDCV, 0, sizeof(*pMDCV));

  for(int32_t c = 0; c < 3; c++)
  {
    pMDCV->display_primaries[c].x = u(pRP, 16);
    pMDCV->display_primaries[c].y = u(pRP, 16);
  }

  pMDCV->white_point.x = u(pRP, 16);
  pMDCV->white_point.y = u(pRP, 16);

  pMDCV->max_display_mastering_luminance = u(pRP, 32);
  pMDCV->min_display_mastering_luminance = u(pRP, 32);

  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiContentLightLevel(AL_TContentLightLevel* pCLL, AL_TRbspParser* pRP)
{
  Rtos_Memset(pCLL, 0, sizeof(*pCLL));
  pCLL->max_content_light_level = u(pRP, 16);
  pCLL->max_pic_average_light_level = u(pRP, 16);
  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiAlternativeTransferCharacteristics(AL_TAlternativeTransferCharacteristics* pATC, AL_TRbspParser* pRP)
{
  pATC->preferred_transfer_characteristics = AL_VUIValueToTransferCharacteristics(u(pRP, 8));
  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiSt2094_10(AL_TDynamicMeta_ST2094_10* pST2094_10, AL_TRbspParser* pRP)
{
  if(ue(pRP) != 1)
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  bool bImageCharacteristicsParsed = false;

  pST2094_10->application_version = ue(pRP);

  if(pST2094_10->application_version != 0)
    return AL_SEI_PARSE_RESULT_UNKNOWN_SEI;

  bool metadata_refresh_flag = u(pRP, 1);

  if(metadata_refresh_flag)
  {
    pST2094_10->processing_window_flag = false;
    pST2094_10->num_manual_adjustments = 0;

    uint16_t num_ext_block = ue(pRP);

    if(num_ext_block > 0)
    {
      if(!simple_byte_alignment(pRP, 0))
        return AL_SEI_PARSE_RESULT_PARSING_ERROR;

      for(int32_t iBlock = 0; iBlock < num_ext_block; iBlock++)
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
            return AL_SEI_PARSE_RESULT_PARSING_ERROR;
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
            return AL_SEI_PARSE_RESULT_PARSING_ERROR;
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
            return AL_SEI_PARSE_RESULT_PARSING_ERROR;
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
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  // Reserved 0xFF bits
  skip(pRP, 8);

  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static void SeiSt2094_40_peakluminance(AL_TDisplayPeakLuminance_ST2094_40* pPeakLuminance, AL_TRbspParser* pRP)
{
  pPeakLuminance->actual_peak_luminance_flag = u(pRP, 1);

  if(pPeakLuminance->actual_peak_luminance_flag)
  {
    pPeakLuminance->num_rows_actual_peak_luminance = u(pRP, 5);
    pPeakLuminance->num_cols_actual_peak_luminance = u(pRP, 5);

    for(int32_t i = 0; i < pPeakLuminance->num_rows_actual_peak_luminance; i++)
      for(int32_t j = 0; j < pPeakLuminance->num_cols_actual_peak_luminance; j++)
        pPeakLuminance->actual_peak_luminance[i][j] = u(pRP, 4);
  }
}

/*****************************************************************************/
static AL_ESeiParseResult SeiSt2094_40(AL_TDynamicMeta_ST2094_40* pST2094_40, AL_TRbspParser* pRP)
{
  if(u(pRP, 8) != 4)
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  pST2094_40->application_version = u(pRP, 8);

  if(pST2094_40->application_version != 0)
    return AL_SEI_PARSE_RESULT_UNKNOWN_SEI;

  pST2094_40->num_windows = u(pRP, 2);

  for(int32_t iWin = 0; iWin < pST2094_40->num_windows - 1; iWin++)
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

  for(int32_t iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    for(int32_t i = 0; i < 3; i++)
      pWinTransfo->maxscl[i] = u(pRP, 17);

    pWinTransfo->average_maxrgb = u(pRP, 17);
    pWinTransfo->num_distribution_maxrgb_percentiles = u(pRP, 4);

    for(int32_t i = 0; i < pWinTransfo->num_distribution_maxrgb_percentiles; i++)
    {
      pWinTransfo->distribution_maxrgb_percentages[i] = u(pRP, 7);
      pWinTransfo->distribution_maxrgb_percentiles[i] = u(pRP, 17);
    }

    pWinTransfo->fraction_bright_pixels = u(pRP, 10);
  }

  SeiSt2094_40_peakluminance(&pST2094_40->mastering_display_peak_luminance, pRP);

  for(int32_t iWin = 0; iWin < pST2094_40->num_windows; iWin++)
  {
    AL_TProcessingWindowTransform_ST2094_40* pWinTransfo = &pST2094_40->processing_window_transforms[iWin];

    AL_TToneMapping_ST2094_40* pToneMapping = &pWinTransfo->tone_mapping;
    pToneMapping->tone_mapping_flag = u(pRP, 1);

    if(pToneMapping->tone_mapping_flag)
    {
      pToneMapping->knee_point_x = u(pRP, 12);
      pToneMapping->knee_point_y = u(pRP, 12);
      pToneMapping->num_bezier_curve_anchors = u(pRP, 4);

      for(int32_t i = 0; i < pToneMapping->num_bezier_curve_anchors; i++)
        pToneMapping->bezier_curve_anchors[i] = u(pRP, 10);
    }

    pWinTransfo->color_saturation_mapping_flag = u(pRP, 1);

    if(pWinTransfo->color_saturation_mapping_flag)
      pWinTransfo->color_saturation_weight = u(pRP, 6);
  }

  return AL_SEI_PARSE_RESULT_PARSED;
}

/*****************************************************************************/
static bool ReadSeiUserDataRegisteredType(AL_TRbspParser* pRP, uint32_t uPayloadSize, AL_EUserDataRegisterSEIType* pSeiType)
{
#define MIN_SEI_USER_DATA_REGISTERED_SIZE 3

  *pSeiType = AL_UDR_SEI_UNKNOWN;

  if(uPayloadSize < MIN_SEI_USER_DATA_REGISTERED_SIZE)
    return false;

  uint8_t itu_t_t35_country_code = u(pRP, 8);
  uint16_t itu_t_t35_terminal_provider_code = u(pRP, 16);

  if(itu_t_t35_country_code == AL_UDR_SEI_COUNTRY_CODE_USA)
  {
    if(itu_t_t35_terminal_provider_code == 0x3B)
    {
      if(u(pRP, 32) == 0x00 && u(pRP, 8) == 0x09)
        *pSeiType = AL_UDR_SEI_ST2094_10;
    }
    else if(itu_t_t35_terminal_provider_code == 0x3C)
    {
      if(u(pRP, 16) == 0x01)
        *pSeiType = AL_UDR_SEI_ST2094_40;
    }
  }

  return true;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiUserDataRegistered(AL_TRbspParser* pRP, uint32_t uPayloadSize, AL_TAup* pOutputAup, bool* pCanSendToUser)
{
  (void)pOutputAup;

  AL_EUserDataRegisterSEIType eSeiType;

  if(!ReadSeiUserDataRegisteredType(pRP, uPayloadSize, &eSeiType))
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  AL_ESeiParseResult eParseResult = AL_SEI_PARSE_RESULT_UNKNOWN_SEI;
  *pCanSendToUser = true;
  switch(eSeiType)
  {
  case AL_UDR_SEI_ST2094_10:
  {
    eParseResult = SeiSt2094_10(&pOutputAup->tParsedHDRSEIs.tST2094_10, pRP);
    pOutputAup->tParsedHDRSEIs.bHasST2094_10 = eParseResult == AL_SEI_PARSE_RESULT_PARSED;
    *pCanSendToUser = false;
    break;
  }
  case AL_UDR_SEI_ST2094_40:
  {
    eParseResult = SeiSt2094_40(&pOutputAup->tParsedHDRSEIs.tST2094_40, pRP);
    pOutputAup->tParsedHDRSEIs.bHasST2094_40 = eParseResult == AL_SEI_PARSE_RESULT_PARSED;
    *pCanSendToUser = false;
    break;
  }
  default:
  {
    break;
  }
  }

  return eParseResult;
}

/*****************************************************************************/
static bool ReadSeiUserDataUnregisteredType(AL_TRbspParser* pRP, uint32_t uPayloadSize, AL_EUserDataUnregisterSEIType* pSeiType)
{
#define MIN_SEI_USER_DATA_UNREGISTERED_SIZE UUID_SIZE

  *pSeiType = AL_UDU_SEI_UNKNOWN;

  if(uPayloadSize < MIN_SEI_USER_DATA_UNREGISTERED_SIZE)
    return false;

  uint8_t uuid[UUID_SIZE];

  for(uint8_t uByte = 0; uByte < UUID_SIZE; uByte++)
    uuid[uByte] = u(pRP, 8);

  if(Rtos_Memcmp(uuid, ALLEGRO_NUM_SLICES_SEI_UUID, UUID_SIZE) == 0)
    *pSeiType = AL_UDU_SEI_ALLEGRO_NUM_SLICES;

  return true;
}

/*****************************************************************************/
static AL_ESeiParseResult SeiUserDataUnregistered(AL_TRbspParser* pRP, uint32_t uPayloadSize, AL_TAup* pOutputAup, bool* pCanSendToUser)
{
  (void)pOutputAup;

  AL_EUserDataUnregisterSEIType eSeiType;

  if(!ReadSeiUserDataUnregisteredType(pRP, uPayloadSize, &eSeiType))
    return AL_SEI_PARSE_RESULT_PARSING_ERROR;

  AL_ESeiParseResult eParseResult = AL_SEI_PARSE_RESULT_UNKNOWN_SEI;
  *pCanSendToUser = true;
  switch(eSeiType)
  {
  case AL_UDU_SEI_ALLEGRO_NUM_SLICES:
  {
    /*
      The SeiParser is called after we detect a full frame (in frame-latency mode), ie access unit,
      to parse all SEIs we associated to the frame. Yet, this SEI is specific, as it is used to
      help the decoder to detect we have a full frame before we receive the first NAL of the next
      frame (goal is to reduce latency). Thus, it must be parsed before standard SEIs, and we just
      skip this SEI here. Also, dont sent to user, its internal usage only.
    */
    *pCanSendToUser = false;
    break;
  }
  default:
  {
    break;
  }
  }

  return eParseResult;
}

/*****************************************************************************/
static AL_ESeiParseResult ParseCommonSei(AL_TRbspParser* pRP, AL_ESeiPayloadType ePayloadType, uint32_t uPayloadSize, AL_TAup* pOutputAup, bool* pCanSendToUser)
{
  AL_ESeiParseResult eParseResult = AL_SEI_PARSE_RESULT_UNKNOWN_SEI;
  *pCanSendToUser = true;
  switch(ePayloadType)
  {
  case SEI_PTYPE_RECOVERY_POINT:
  {
    AL_TRecoveryPoint tRecoveryPoint;
    eParseResult = SeiRecoveryPoint(pRP, &tRecoveryPoint);
    pOutputAup->iRecoveryCnt = tRecoveryPoint.recovery_cnt + 1; // +1 for non-zero value when AL_SEI_RP is present
    break;
  }
  case SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
  {
    eParseResult = SeiMasteringDisplayColourVolume(&pOutputAup->tParsedHDRSEIs.tMDCV, pRP);
    pOutputAup->tParsedHDRSEIs.bHasMDCV = true;
    *pCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_CONTENT_LIGHT_LEVEL:
  {
    eParseResult = SeiContentLightLevel(&pOutputAup->tParsedHDRSEIs.tCLL, pRP);
    pOutputAup->tParsedHDRSEIs.bHasCLL = true;
    *pCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
  {
    eParseResult = SeiAlternativeTransferCharacteristics(&pOutputAup->tParsedHDRSEIs.tATC, pRP);
    pOutputAup->tParsedHDRSEIs.bHasATC = true;
    *pCanSendToUser = false;
    break;
  }
  case SEI_PTYPE_USER_DATA_REGISTERED:
  {
    eParseResult = SeiUserDataRegistered(pRP, uPayloadSize, pOutputAup, pCanSendToUser);
    break;
  }
  case SEI_PTYPE_USER_DATA_UNREGISTERED:
  {
    eParseResult = SeiUserDataUnregistered(pRP, uPayloadSize, pOutputAup, pCanSendToUser);
    break;
  }
  default:
  {
    break;
  }
  }

  return eParseResult;
}

/*****************************************************************************/
static bool GetPayloadType(AL_TRbspParser* pRP, AL_ESeiPayloadType* pPayloadType)
{
  uint32_t uPayloadType = 0;

  if(!byte_aligned(pRP))
    return false;

  uint8_t byte = getbyte(pRP);

  while(byte == 0xff)
  {
    uPayloadType += 255;
    byte = getbyte(pRP);
  }

  uPayloadType += byte;
  *pPayloadType = (AL_ESeiPayloadType)uPayloadType;

  return true;
}

/*****************************************************************************/
static uint32_t GetPayloadSize(AL_TRbspParser* pRP)
{
  uint32_t uPayloadSize = 0;

  uint8_t byte = getbyte(pRP);

  while(byte == 0xff)
  {
    uPayloadSize += 255;
    byte = getbyte(pRP);
  }

  uPayloadSize += byte;

  return uPayloadSize;
}

/*****************************************************************************/
bool AL_SeiParser_Parse(AL_TSeiParserCtx* pCtx, AL_TRbspParser* pRP, bool bIsPrefix)
{
  AL_ESeiPayloadType ePayloadType;

  // Parse payload header
  // -----------------
  if(!GetPayloadType(pRP, &ePayloadType))
    return false;

  uint32_t uPayloadSize = GetPayloadSize(pRP);

  // Parse payload content
  // -----------------
  uint32_t uOffsetBefore = offset(pRP);
  uint8_t* pPayloadData = get_raw_data(pRP);

  AL_ESeiParseResult eParseResult = AL_SEI_PARSE_RESULT_UNKNOWN_SEI;
  bool bCanSendToUser = true;

  if(pCtx->pfnCustomSeiParsing)
    eParseResult = pCtx->pfnCustomSeiParsing(pRP, ePayloadType, uPayloadSize, pCtx->pOutputAup, &bCanSendToUser);

  if(eParseResult == AL_SEI_PARSE_RESULT_UNKNOWN_SEI)
    eParseResult = ParseCommonSei(pRP, ePayloadType, uPayloadSize, pCtx->pOutputAup, &bCanSendToUser);

  uint32_t const uOffsetAfter = offset(pRP);
  uint32_t const uReadSize = uOffsetAfter - uOffsetBefore;
  int32_t iRemainingPayload = (uPayloadSize << 3) - (int32_t)(uReadSize);

  // Try to catch up if the parsing was bad
  if((eParseResult == AL_SEI_PARSE_RESULT_PARSING_ERROR) && (iRemainingPayload < 0))
    return false;

  // Skip remaining payload
  // ----------------------
  if(iRemainingPayload > 0)
    skip(pRP, iRemainingPayload);

  if(!byte_aligned(pRP))
    byte_alignment(pRP);

  // Attach sei to the SeiMetaData
  // -----------------------------
  if(bCanSendToUser && pCtx->pOutputMeta)
    if(!AL_SeiMetaData_AddPayload(pCtx->pOutputMeta, (AL_TSeiMessage) {bIsPrefix, ePayloadType, pPayloadData, uPayloadSize }))
      return false;

  // Send sei to the user
  // --------------------
  if(bCanSendToUser && (pCtx->pSeiParsedCallback != NULL) && (pCtx->pSeiParsedCallback->func != NULL))
    pCtx->pSeiParsedCallback->func(bIsPrefix, ePayloadType, pPayloadData, uPayloadSize, pCtx->pSeiParsedCallback->userParam);

  return true;
}
