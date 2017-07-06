/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include <assert.h>
#include <string.h>

#include "lib_rtos/lib_rtos.h"

#include "lib_common/Utils.h"

#include "common_syntax.h"
#include "PpsParsing.h"

/****************************************************************************
                A V C   P P S   P A R S I N G   F U N C T I O N
****************************************************************************/

/*************************************************************************//*!
   \brief The InitPPS function intialize a PPS structure
   \param[out] pPPS Pointer to the PPS structure that will be initialized
*****************************************************************************/
static void AL_AVC_sInitPPS(AL_TAvcPps* pPPS)
{
  Rtos_Memset(pPPS->UseDefaultScalingMatrix4x4Flag, 0, 6);
  Rtos_Memset(pPPS->UseDefaultScalingMatrix8x8Flag, 0, 2);
  pPPS->transform_8x8_mode_flag = 0;
  pPPS->pic_scaling_matrix_present_flag = 0;
  pPPS->bConceal = true;
}

/*************************************************************************/
AL_PARSE_RESULT AL_AVC_ParsePPS(AL_TAvcPps pPPSTable[], AL_TRbspParser* pRP, AL_TAvcSps pSPSTable[])
{
  uint16_t pps_id, QpBdOffset;
  AL_TAvcPps tempPPS;

  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 8); // Skip NUT

  pps_id = ue(pRP);

  COMPLY(pps_id < AL_AVC_MAX_PPS);

  AL_AVC_sInitPPS(&tempPPS);

  tempPPS.pic_parameter_set_id = pps_id;
  tempPPS.seq_parameter_set_id = ue(pRP);

  COMPLY(tempPPS.seq_parameter_set_id < AL_AVC_MAX_SPS);

  tempPPS.m_pSPS = &pSPSTable[tempPPS.seq_parameter_set_id];

  COMPLY(!tempPPS.m_pSPS->bConceal);

  tempPPS.entropy_coding_mode_flag = u(pRP, 1);
  tempPPS.bottom_field_pic_order_in_frame_present_flag = u(pRP, 1);
  tempPPS.num_slice_groups_minus1 = ue(pRP); // num_slice_groups_minus1

  if(tempPPS.num_slice_groups_minus1 > 0)
  {
    tempPPS.slice_group_map_type = ue(pRP);

    if(tempPPS.slice_group_map_type == 0)
    {
      int iGroup;

      for(iGroup = 0; iGroup <= tempPPS.num_slice_groups_minus1; iGroup++)
        tempPPS.run_length_minus1[iGroup] = ue(pRP);
    }
    else if(tempPPS.slice_group_map_type == 2)
    {
      int iGroup;

      for(iGroup = 0; iGroup <= tempPPS.num_slice_groups_minus1; iGroup++)
      {
        tempPPS.top_left[iGroup] = ue(pRP);
        tempPPS.bottom_right[iGroup] = ue(pRP);
      }
    }
    else if(tempPPS.slice_group_map_type == 3 ||
            tempPPS.slice_group_map_type == 4 ||
            tempPPS.slice_group_map_type == 5)
    {
      tempPPS.slice_group_change_direction_flag = u(pRP, 1);
      tempPPS.slice_group_change_rate_minus1 = ue(pRP);
    }
    else if(tempPPS.slice_group_map_type == 6)
    {
      int i;
      tempPPS.pic_size_in_map_units_minus1 = ue(pRP);

      for(i = 0; i <= tempPPS.pic_size_in_map_units_minus1; i++)
      {
        int slicegroupsize = tempPPS.pic_size_in_map_units_minus1 + 1;
        tempPPS.slice_group_id[i] = u(pRP, slicegroupsize);
      }
    }
  }

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  tempPPS.num_ref_idx_l0_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);
  tempPPS.num_ref_idx_l1_active_minus1 = Clip3(ue(pRP), 0, AL_AVC_MAX_REF_IDX);

  tempPPS.weighted_pred_flag = u(pRP, 1);
  tempPPS.weighted_bipred_idc = Clip3(u(pRP, 2), 0, AL_MAX_WP_IDC);

  QpBdOffset = 6 * tempPPS.m_pSPS->bit_depth_luma_minus8;
  tempPPS.pic_init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);
  tempPPS.pic_init_qs_minus26 = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);

  tempPPS.chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  tempPPS.second_chroma_qp_index_offset = tempPPS.chroma_qp_index_offset;

  tempPPS.deblocking_filter_control_present_flag = u(pRP, 1);
  tempPPS.constrained_intra_pred_flag = u(pRP, 1);
  skip(pRP, 1);
  tempPPS.redundant_pic_cnt_present_flag = 0;

  if(more_rbsp_data(pRP))
  {
    tempPPS.transform_8x8_mode_flag = u(pRP, 1);
    tempPPS.pic_scaling_matrix_present_flag = u(pRP, 1);

    if(tempPPS.pic_scaling_matrix_present_flag)
    {
      int i;

      for(i = 0; i < 6 + 2 * tempPPS.transform_8x8_mode_flag; i++)
      {
        if(i < 6)
          tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 0;
        else
          tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 0;

        tempPPS.pic_scaling_list_present_flag[i] = u(pRP, 1);

        if(tempPPS.pic_scaling_list_present_flag[i])
        {
          if(i < 6)
            avc_scaling_list_data(tempPPS.ScalingList4x4[i], pRP, 16, &tempPPS.UseDefaultScalingMatrix4x4Flag[i]);
          else
            avc_scaling_list_data(tempPPS.ScalingList8x8[i - 6], pRP, 64, &tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
        else
        {
          if(!tempPPS.m_pSPS->seq_scaling_matrix_present_flag)
          {
            if(i < 6)
            {
              if(i == 0 || i == 3)
                tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
              else
              {
                if(tempPPS.UseDefaultScalingMatrix4x4Flag[i - 1])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.ScalingList4x4[i - 1], 16);
              }
            }
            else
              tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
          }
          else
          {
            if(i < 6)
            {
              if(i == 0 || i == 3)
              {
                if(tempPPS.m_pSPS->UseDefaultScalingMatrix4x4Flag[i])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.m_pSPS->ScalingList4x4[i], 16);
              }
              else
              {
                if(tempPPS.UseDefaultScalingMatrix4x4Flag[i - 1])
                  tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
                else
                  Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.ScalingList4x4[i - 1], 16);
              }
            }
            else
            {
              if(tempPPS.m_pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
                tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
              else
                Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.m_pSPS->ScalingList8x8[i - 6], 64);
            }
          }
        }
      }
    }
    else if(tempPPS.m_pSPS)
    {
      int i;

      for(i = 0; i < 8; ++i)
      {
        if(i < 6)
        {
          if(tempPPS.m_pSPS->UseDefaultScalingMatrix4x4Flag[i])
            tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
          else
            Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.m_pSPS->ScalingList4x4[i], 16);
        }
        else
        {
          if(tempPPS.m_pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
            tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
          else
            Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.m_pSPS->ScalingList8x8[i - 6], 64);
        }
      }
    }

    tempPPS.second_chroma_qp_index_offset = Clip3(se(pRP), AL_MIN_INIT_QP, AL_MAX_INIT_QP);
  }
  else
  {
    int i;

    for(i = 0; i < 8; ++i)
    {
      if(i < 6)
      {
        if(tempPPS.m_pSPS->UseDefaultScalingMatrix4x4Flag[i])
          tempPPS.UseDefaultScalingMatrix4x4Flag[i] = 1;
        else
          Rtos_Memcpy(tempPPS.ScalingList4x4[i], tempPPS.m_pSPS->ScalingList4x4[i], 16);
      }
      else
      {
        if(tempPPS.m_pSPS->UseDefaultScalingMatrix8x8Flag[i - 6])
          tempPPS.UseDefaultScalingMatrix8x8Flag[i - 6] = 1;
        else
          Rtos_Memcpy(tempPPS.ScalingList8x8[i - 6], tempPPS.m_pSPS->ScalingList8x8[i - 6], 64);
      }
    }
  }

  /*dummy information to ensure there's no zero value in scaling list structure (div by zero prevention)*/
  if(!tempPPS.transform_8x8_mode_flag)
    tempPPS.UseDefaultScalingMatrix8x8Flag[0] =
      tempPPS.UseDefaultScalingMatrix8x8Flag[1] = 1;

  tempPPS.bConceal = rbsp_trailing_bits(pRP) ? false : true;

  COMPLY(tempPPS.num_slice_groups_minus1 == 0); // baseline profile only

  pPPSTable[pps_id] = tempPPS;
  return AL_OK;
}

/****************************************************************************
              H E V C   P P S   P A R S I N G   F U N C T I O N
****************************************************************************/

/*************************************************************************/
static void AL_HEVC_sInitPPS(AL_THevcPps* pPPS)
{
  Rtos_Memset(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, sizeof(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag));
  Rtos_Memset(pPPS->cb_qp_offset_list, 0, sizeof(pPPS->cb_qp_offset_list));
  Rtos_Memset(pPPS->cr_qp_offset_list, 0, sizeof(pPPS->cr_qp_offset_list));

  pPPS->loop_filter_across_tiles_enabled_flag = 1;
  pPPS->lists_modification_present_flag = 1;
  pPPS->num_tile_columns_minus1 = 0;
  pPPS->num_tile_rows_minus1 = 0;
  pPPS->deblocking_filter_override_enabled_flag = 0;
  pPPS->pps_deblocking_filter_disabled_flag = 0;
  pPPS->pps_beta_offset_div2 = 0;
  pPPS->pps_tc_offset_div2 = 0;

  pPPS->pps_extension_7bits = 0;
  pPPS->pps_range_extensions_flag = 0;
  pPPS->log2_transform_skip_block_size_minus2 = 0;
  pPPS->cross_component_prediction_enabled_flag = 0;
  pPPS->diff_cu_chroma_qp_offset_depth = 0;
  pPPS->chroma_qp_offset_list_enabled_flag = 0;
  pPPS->log2_sao_offset_scale_chroma = 0;
  pPPS->log2_sao_offset_scale_chroma = 0;
}

/*************************************************************************/
void AL_HEVC_ParsePPS(AL_THevcPps pPPSTable[], AL_TRbspParser* pRP, AL_THevcSps pSPSTable[], uint8_t* pPpsId)
{
  uint8_t i, j, pps_id, QpBdOffset;
  uint16_t uLCUWidth, uLCUHeight;
  AL_THevcPps* pPPS;

  // Parse bitstream
  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 16); // Skip NUT + temporal_id

  pps_id = ue(pRP);
  pPPS = &pPPSTable[pps_id];

  *pPpsId = pps_id;
  // default values
  AL_HEVC_sInitPPS(pPPS);
  pPPS->bConceal = true;

  if(pps_id >= AL_HEVC_MAX_PPS)
    return;

  pPPS->pps_pic_parameter_set_id = pps_id;
  pPPS->pps_seq_parameter_set_id = ue(pRP);

  if(pPPS->pps_seq_parameter_set_id >= AL_HEVC_MAX_SPS)
    return;

  pPPS->m_pSPS = &pSPSTable[pPPS->pps_seq_parameter_set_id];

  if(pPPS->m_pSPS->bConceal)
    return;

  uLCUWidth = pPPS->m_pSPS->PicWidthInCtbs;
  uLCUHeight = pPPS->m_pSPS->PicHeightInCtbs;

  pPPS->dependent_slice_segments_enabled_flag = u(pRP, 1);
  pPPS->output_flag_present_flag = u(pRP, 1);
  pPPS->num_extra_slice_header_bits = u(pRP, 3);
  pPPS->sign_data_hiding_flag = u(pRP, 1);
  pPPS->cabac_init_present_flag = u(pRP, 1);

  pPPS->num_ref_idx_l0_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
  pPPS->num_ref_idx_l1_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

  QpBdOffset = 6 * pPPS->m_pSPS->bit_depth_luma_minus8;
  pPPS->init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);

  pPPS->constrained_intra_pred_flag = u(pRP, 1);
  pPPS->transform_skip_enabled_flag = u(pRP, 1);

  pPPS->cu_qp_delta_enabled_flag = u(pRP, 1);

  if(pPPS->cu_qp_delta_enabled_flag)
    pPPS->diff_cu_qp_delta_depth = Clip3(ue(pRP), 0, pPPS->m_pSPS->log2_diff_max_min_luma_coding_block_size);

  pPPS->pps_cb_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_cr_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_slice_chroma_qp_offsets_present_flag = u(pRP, 1);

  pPPS->weighted_pred_flag = u(pRP, 1);
  pPPS->weighted_bipred_flag = u(pRP, 1);

  pPPS->transquant_bypass_enabled_flag = u(pRP, 1);
  pPPS->tiles_enabled_flag = u(pRP, 1);
  pPPS->entropy_coding_sync_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return;

  if(pPPS->tiles_enabled_flag)
  {
    pPPS->num_tile_columns_minus1 = ue(pRP);
    pPPS->num_tile_rows_minus1 = ue(pRP);
    pPPS->uniform_spacing_flag = u(pRP, 1);

    if(pPPS->num_tile_columns_minus1 >= uLCUWidth || pPPS->num_tile_rows_minus1 >= uLCUHeight ||
       pPPS->num_tile_columns_minus1 >= AL_MAX_COLUMNS_TILE || pPPS->num_tile_rows_minus1 >= AL_MAX_ROWS_TILE)
      return;

    if(!pPPS->uniform_spacing_flag)
    {
      uint16_t uClmnOffset = 0;
      uint16_t uLineOffset = 0;

      for(i = 0; i < pPPS->num_tile_columns_minus1; ++i)
      {
        pPPS->column_width[i] = ue(pRP) + 1;
        uClmnOffset += pPPS->column_width[i];
      }

      for(i = 0; i < pPPS->num_tile_rows_minus1; ++i)
      {
        pPPS->row_height[i] = ue(pRP) + 1;
        uLineOffset += pPPS->row_height[i];
      }

      if(uClmnOffset >= uLCUWidth || uLineOffset >= uLCUHeight)
        return;

      pPPS->column_width[pPPS->num_tile_columns_minus1] = uLCUWidth - uClmnOffset;
      pPPS->row_height[pPPS->num_tile_rows_minus1] = uLCUHeight - uLineOffset;
    }
    else /* tile of same size */
    {
      uint16_t num_clmn = pPPS->num_tile_columns_minus1 + 1;
      uint16_t num_line = pPPS->num_tile_rows_minus1 + 1;

      for(i = 0; i <= pPPS->num_tile_columns_minus1; ++i)
        pPPS->column_width[i] = (((i + 1) * uLCUWidth) / num_clmn) - ((i * uLCUWidth) / num_clmn);

      for(i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
        pPPS->row_height[i] = (((i + 1) * uLCUHeight) / num_line) - ((i * uLCUHeight) / num_line);
    }

    /* register tile topology within the frame */
    for(i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
    {
      for(j = 0; j <= pPPS->num_tile_columns_minus1; ++j)
      {
        uint8_t line = 0;
        uint8_t clmn = 0;
        uint8_t uClmn = 0;
        uint8_t uLine = 0;

        while(line < i)
          uLine += pPPS->row_height[line++];

        while(clmn < j)
          uClmn += pPPS->column_width[clmn++];

        pPPS->TileTopology[(i * (pPPS->num_tile_columns_minus1 + 1)) + j] = uLine * uLCUWidth + uClmn;
      }
    }

    pPPS->loop_filter_across_tiles_enabled_flag = u(pRP, 1);
  }

  // check if NAL isn't empty
  if(!more_rbsp_data(pRP))
    return;

  pPPS->loop_filter_across_slices_enabled_flag = u(pRP, 1);

  pPPS->deblocking_filter_control_present_flag = u(pRP, 1);

  if(pPPS->deblocking_filter_control_present_flag)
  {
    pPPS->deblocking_filter_override_enabled_flag = u(pRP, 1);
    pPPS->pps_deblocking_filter_disabled_flag = u(pRP, 1);

    if(!pPPS->pps_deblocking_filter_disabled_flag)
    {
      pPPS->pps_beta_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
      pPPS->pps_tc_offset_div2 = Clip3(se(pRP), AL_MIN_DBF_PARAM, AL_MAX_DBF_PARAM);
    }
  }

  pPPS->pps_scaling_list_data_present_flag = u(pRP, 1);

  if(pPPS->pps_scaling_list_data_present_flag)
    hevc_scaling_list_data(&pPPS->scaling_list_param, pRP);
  else // get scaling_list data from associated sps
    pPPS->scaling_list_param = pPPS->m_pSPS->scaling_list_param;

  pPPS->lists_modification_present_flag = u(pRP, 1);
  pPPS->log2_parallel_merge_level_minus2 = Clip3(ue(pRP), 0, pPPS->m_pSPS->Log2CtbSize - 2);

  pPPS->slice_segment_header_extension_present_flag = u(pRP, 1);
  pPPS->pps_extension_present_flag = u(pRP, 1);

  if(pPPS->pps_extension_present_flag)
  {
    pPPS->pps_range_extensions_flag = u(pRP, 1);
    pPPS->pps_extension_7bits = u(pRP, 7);
  }

  if(pPPS->pps_range_extensions_flag)
  {
    if(pPPS->transform_skip_enabled_flag)
      pPPS->log2_transform_skip_block_size_minus2 = ue(pRP);
    pPPS->cross_component_prediction_enabled_flag = u(pRP, 1);
    pPPS->chroma_qp_offset_list_enabled_flag = u(pRP, 1);

    if(pPPS->chroma_qp_offset_list_enabled_flag)
    {
      int i;
      pPPS->diff_cu_chroma_qp_offset_depth = ue(pRP);
      pPPS->chroma_qp_offset_list_len_minus1 = ue(pRP);

      for(i = 0; i <= pPPS->chroma_qp_offset_list_len_minus1; ++i)
      {
        pPPS->cb_qp_offset_list[i] = se(pRP);
        pPPS->cr_qp_offset_list[i] = se(pRP);
      }
    }
    pPPS->log2_sao_offset_scale_luma = ue(pRP);
    pPPS->log2_sao_offset_scale_chroma = ue(pRP);
  }

  if(pPPS->pps_extension_7bits) // pps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // pps_extension_data_flag
  }

  pPPS->bConceal = rbsp_trailing_bits(pRP) ? false : true;
}

/*@}*/

