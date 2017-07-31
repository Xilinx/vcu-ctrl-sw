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

#include "lib_rtos/lib_rtos.h"

#include "SeiParsing.h"

/****************************************************************************
                A V C   S E I   P A R S I N G   F U N C T I O N
****************************************************************************/

/*****************************************************************************/
static bool AL_AVC_sbuffering_period(AL_TRbspParser* pRP, AL_TAvcSps* pSpsTable, AL_TAvcBufPeriod* pBufPeriod, AL_TAvcSps** pActiveSps)
{
  AL_TAvcSps* pSPS = NULL;

  pBufPeriod->seq_parameter_set_id = ue(pRP);

  if(pBufPeriod->seq_parameter_set_id >= 32)
    return false;

  pSPS = &pSpsTable[pBufPeriod->seq_parameter_set_id];

  if(pSPS->bConceal)
    return false;

  *pActiveSps = pSPS;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    uint8_t i;
    uint8_t syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(i = 0; i <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      pBufPeriod->initial_cpb_removal_delay[i] = u(pRP, syntax_size);
      pBufPeriod->initial_cpb_removal_delay_offset[i] = u(pRP, syntax_size);
    }
  }

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    uint8_t i;
    uint8_t syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(i = 0; i <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++i)
    {
      pBufPeriod->initial_cpb_removal_delay[i] = u(pRP, syntax_size);
      pBufPeriod->initial_cpb_removal_delay_offset[i] = u(pRP, syntax_size);
    }
  }

  return byte_alignment(pRP);
}

/*****************************************************************************/
bool AL_AVC_spic_timing(AL_TRbspParser* pRP, AL_TAvcSps* pSPS, AL_TAvcPicTiming* pPicTiming)
{
  uint8_t time_offset_length = 0;

  if(pSPS == NULL || pSPS->bConceal)
    return false;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->cpb_removal_delay = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->dpb_output_delay = u(pRP, syntax_size);

    time_offset_length = pSPS->vui_param.hrd_param.time_offset_length;
  }
  else if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->cpb_removal_delay = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->dpb_output_delay = u(pRP, syntax_size);

    time_offset_length = pSPS->vui_param.hrd_param.time_offset_length;
  }

  if(pSPS->vui_param.pic_struct_present_flag)
  {
    uint8_t iter;
    const int NumClockTS[] =
    {
      1, 1, 1, 2, 2, 3, 3, 2, 3
    }; // Table D-1
    AL_TSeiClockTS* pClockTS;

    pPicTiming->pic_struct = u(pRP, 4);

    for(iter = 0; iter < NumClockTS[pPicTiming->pic_struct]; ++iter)
    {
      pClockTS = &pPicTiming->clock_ts[iter];

      pClockTS->clock_time_stamp_flag = u(pRP, 1);

      if(pClockTS->clock_time_stamp_flag)
      {
        pClockTS->ct_type = u(pRP, 2);
        pClockTS->nuit_field_based_flag = u(pRP, 1);
        pClockTS->counting_type = u(pRP, 5);
        pClockTS->full_time_stamp_flag = u(pRP, 1);
        pClockTS->discontinuity_flag = u(pRP, 1);
        pClockTS->cnt_dropped_flag = u(pRP, 1);
        pClockTS->n_frames = u(pRP, 8);

        if(pClockTS->full_time_stamp_flag)
        {
          pClockTS->seconds_value = u(pRP, 6);
          pClockTS->minutes_value = u(pRP, 6);
          pClockTS->hours_value = u(pRP, 5);
        }
        else
        {
          pClockTS->seconds_flag = u(pRP, 1);

          if(pClockTS->seconds_flag)
          {
            pClockTS->seconds_value = u(pRP, 6);  /* range 0..59 */
            pClockTS->minutes_flag = u(pRP, 1);

            if(pClockTS->minutes_flag)
            {
              pClockTS->minutes_value = u(pRP, 6); /* 0..59 */

              pClockTS->hours_flag = u(pRP, 1);

              if(pClockTS->hours_flag)
                pClockTS->hours_value = u(pRP, 5); /* 0..23 */
            }
          }
        }
      } // clock_timestamp_flag

      pClockTS->time_offset = i(pRP, time_offset_length);
    }
  }

  return byte_alignment(pRP);
}

/*****************************************************************************/
bool AL_AVC_ParseSEI(AL_TAvcSei* pSEI, AL_TRbspParser* pRP, AL_TAvcSps* pSpsTable, AL_TAvcSps** pActiveSps)
{
  pSEI->present_flags = 0;

  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 8); // Skip NUT

  while(more_rbsp_data(pRP))
  {
    uint32_t payload_type = 0;
    uint32_t payload_size = 0;

    // get payload type
    uint8_t byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_type += 255;
      byte = getbyte(pRP);
    }

    payload_type += byte;

    // get payload size
    byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_size += 255;
      byte = getbyte(pRP);
    }

    payload_size += byte;
    switch(payload_type)
    {
    case 0: // buffering_period parsing
    {
      uint32_t uOffset = offset(pRP);
      bool bRet = AL_AVC_sbuffering_period(pRP, pSpsTable, &pSEI->buffering_period, pActiveSps);

      if(!bRet)
      {
        uOffset = offset(pRP) - uOffset;
        skip(pRP, (payload_size << 3) - uOffset);
      }
      pSEI->present_flags |= SEI_BP;
      break;
    }

    case 1: // picture_timing parsing

      if(*pActiveSps)
      {
        bool bRet = AL_AVC_spic_timing(pRP, *pActiveSps, &pSEI->picture_timing);

        if(!bRet)
          skip(pRP, payload_size << 3);
        pSEI->present_flags |= SEI_PT;
      }
      else
        return false;
      break;

    case 5: // user_data_unregistered
    {
      skip(pRP, payload_size << 3); // skip data
    } break;

    default: // payload not supported
      skip(pRP, payload_size << 3); // skip data
      break;
    }
  }

  return true;
}

/****************************************************************************
              H E V C   S E I   P A R S I N G   F U N C T I O N
****************************************************************************/

/*************************************************************************//*!
   \brief The buffering_period function parse a buffering_period sei message
   \param[in]  pRP        Pointer to NAL parser
   \param[in]  pSpsTable  Pointer to the available sps table input
   and the current active sps output
   \param[out] pBufPeriod Pointer to the buffering period structure that will be filled
   \param[out] pActiveSps Pointer to the active SPS that will be filled
*****************************************************************************/
static void AL_HEVC_sbuffering_period(AL_TRbspParser* pRP, AL_THevcSps* pSpsTable, AL_THevcBufPeriod* pBufPeriod, AL_THevcSps** pActiveSps)
{
  uint8_t SchedSelIdx;
  uint8_t syntax_size;
  AL_THevcSps* pSPS = NULL;

  pBufPeriod->bp_seq_parameter_set_id = ue(pRP);
  assert(pBufPeriod->bp_seq_parameter_set_id < 32);

  pSPS = &pSpsTable[pBufPeriod->bp_seq_parameter_set_id];
  assert(pSPS);

  *pActiveSps = pSPS;

  if(!pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    pBufPeriod->rap_cpb_params_present_flag = u(pRP, 1);

  if(pBufPeriod->rap_cpb_params_present_flag)
  {
    syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pBufPeriod->cpb_delay_offset = u(pRP, syntax_size);
    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pBufPeriod->dpb_delay_offset = u(pRP, syntax_size);
  }

  pBufPeriod->concatenation_flag = u(pRP, 1);
  syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
  pBufPeriod->au_cpb_removal_delay_delta_minus1 = u(pRP, syntax_size);

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
  {
    syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(SchedSelIdx = 0; SchedSelIdx <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++SchedSelIdx)
    {
      pBufPeriod->nal_initial_cpb_removal_delay[SchedSelIdx] = u(pRP, syntax_size);
      pBufPeriod->nal_initial_cpb_removal_delay_offset[SchedSelIdx] = u(pRP, syntax_size);

      if(pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag || pBufPeriod->rap_cpb_params_present_flag)
      {
        pBufPeriod->nal_initial_alt_cpb_removal_delay[SchedSelIdx] = u(pRP, syntax_size);
        pBufPeriod->nal_initial_alt_cpb_removal_delay_offset[SchedSelIdx] = u(pRP, syntax_size);
      }
    }
  }

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
  {
    syntax_size = pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 + 1;

    for(SchedSelIdx = 0; SchedSelIdx <= pSPS->vui_param.hrd_param.cpb_cnt_minus1[0]; ++SchedSelIdx)
    {
      pBufPeriod->vcl_initial_cpb_removal_delay[SchedSelIdx] = u(pRP, syntax_size);
      pBufPeriod->vcl_initial_cpb_removal_delay_offset[SchedSelIdx] = u(pRP, syntax_size);

      if(pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag || pBufPeriod->rap_cpb_params_present_flag)
      {
        pBufPeriod->vcl_initial_alt_cpb_removal_delay[SchedSelIdx] = u(pRP, syntax_size);
        pBufPeriod->vcl_initial_alt_cpb_removal_delay_offset[SchedSelIdx] = u(pRP, syntax_size);
      }
    }
  }
}

/*************************************************************************//*!
   \brief The pic_timing function parse a pic_timing sei message
   \param[in]  pRP        Pointer to NAL parser
   \param[in]  pSPS       Pointer to the current active sps
   \param[out] pPicTiming Pointer to the pic timing structure that will be filled
*****************************************************************************/
static void AL_HEVC_spic_timing(AL_TRbspParser* pRP, AL_THevcSps* pSPS, AL_THevcPicTiming* pPicTiming)
{
  uint8_t i;
  uint8_t syntax_size;

  if(pSPS->vui_param.frame_field_info_present_flag)
  {
    pPicTiming->pic_struct = u(pRP, 4);
    pPicTiming->source_scan_type = u(pRP, 2);
    pPicTiming->duplicate_flag = u(pRP, 1);
  }

  if(pSPS->vui_param.vui_hrd_parameters_present_flag)
  {
    syntax_size = pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1;
    pPicTiming->au_cpb_removal_delay_minus1 = u(pRP, syntax_size);
    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->pic_dpb_output_delay = u(pRP, syntax_size);

    if(pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    {
      pPicTiming->pic_dpb_output_du_delay = u(pRP, syntax_size);

      if(pSPS->vui_param.hrd_param.sub_pic_cpb_params_in_pic_timing_sei_flag)
      {
        pPicTiming->num_decoding_units_minus1 = ue(pRP);
        pPicTiming->du_common_cpb_removal_delay_flag = u(pRP, 1);

        syntax_size = pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1;

        if(pPicTiming->du_common_cpb_removal_delay_flag)
          pPicTiming->du_common_cpb_removal_delay_increment_minus1 = u(pRP, syntax_size);

        for(i = 0; i <= pPicTiming->num_decoding_units_minus1; ++i)
        {
          pPicTiming->num_nalus_in_du_minus1[i] = ue(pRP);

          if(!pPicTiming->du_common_cpb_removal_delay_flag && i < pPicTiming->num_decoding_units_minus1)
            pPicTiming->du_cpb_removal_delay_increment_minus1[i] = u(pRP, syntax_size);
        }
      }
    }
  }
}

/*************************************************************************//*!
   \brief The InitSEI function intializes a SEI structure
   \param[out] pSEI Pointer to the SEI structure that will be initialized
*****************************************************************************/
void AL_HEVC_InitSEI(AL_THevcSei* pSEI)
{
  pSEI->picture_timing.num_nalus_in_du_minus1 = Rtos_Malloc(32768 * sizeof(uint32_t));
  pSEI->picture_timing.du_cpb_removal_delay_increment_minus1 = Rtos_Malloc(32768 * sizeof(uint32_t));

  pSEI->buffering_period.cpb_delay_offset = 0;
  pSEI->buffering_period.dpb_delay_offset = 0;
}

/*************************************************************************//*!
   \brief The DeinitSEI function releases memory allocation operated by the SEI structure
   \param[out] pSEI Pointer to the SEI structure that will be freed
*****************************************************************************/
void AL_HEVC_DeinitSEI(AL_THevcSei* pSEI)
{
  if(pSEI->picture_timing.num_nalus_in_du_minus1)
  {
    Rtos_Free(pSEI->picture_timing.num_nalus_in_du_minus1);
    pSEI->picture_timing.num_nalus_in_du_minus1 = NULL;
  }

  if(pSEI->picture_timing.du_cpb_removal_delay_increment_minus1)
  {
    Rtos_Free(pSEI->picture_timing.du_cpb_removal_delay_increment_minus1);
    pSEI->picture_timing.du_cpb_removal_delay_increment_minus1 = NULL;
  }
}

/*************************************************************************//*!
   \brief The HEVC_ParseSEI function parse a SEI NAL
   \param[out] pSEI Pointer to the sei message structure that will be filled
   \param[in]  pRP  Pointer to NAL parser
   \param[in]  pSpsTable pointer to the available sps table input
   \param[out] pActiveSps pointer to the current active sps output
   \return     true when all sei messages had been parsed
            false otherwise.
*****************************************************************************/
bool AL_HEVC_ParseSEI(AL_THevcSei* pSEI, AL_TRbspParser* pRP, AL_THevcSps* pSpsTable, AL_THevcSps** pActiveSps)
{
  while(u(pRP, 8) == 0x00)
    ; // Skip all 0x00s and one 0x01

  u(pRP, 16); // Skip NUT + temporal_id

  while(more_rbsp_data(pRP))
  {
    uint32_t payload_type = 0;
    uint32_t payload_size = 0;

    // get payload type
    uint8_t byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_type += 255;
      byte = getbyte(pRP);
    }

    payload_type += byte;

    // get payload size
    byte = getbyte(pRP);

    while(byte == 0xff)
    {
      payload_size += 255;
      byte = getbyte(pRP);
    }

    payload_size += byte;
    switch(payload_type)
    {
    case 0: // buffering_period parsing
    {
      uint32_t uCur = offset(pRP);
      uint32_t uEnd = uCur + (payload_size << 3);

      AL_HEVC_sbuffering_period(pRP, pSpsTable, &pSEI->buffering_period, pActiveSps);

      if(!byte_alignment(pRP))
        return false;
      uCur = offset(pRP);

      if(uCur < uEnd)
        skip(pRP, uEnd - uCur);
      break;
    }

    case 1: // picture_timing parsing

      if(*pActiveSps)
      {
        uint32_t uCur = offset(pRP);
        uint32_t uEnd = uCur + (payload_size << 3);

        AL_HEVC_spic_timing(pRP, *pActiveSps, &pSEI->picture_timing);

        if(!byte_alignment(pRP))
          return false;

        uCur = offset(pRP);

        if(uCur < uEnd)
          skip(pRP, uEnd - uCur);
      }
      else
        return false;
      break;

    default: // payload not supported
      skip(pRP, payload_size << 3); // skip data
      break;
    }
  }

  return true;
}

/******************************************************************************/

/*@}*/

