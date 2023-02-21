// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

/*************************************************************************//*!
   \brief Core Information structure
*****************************************************************************/
typedef struct
{
  int iCoreFrequency;
  int iMaxVideoResourcePerCore;
  int iVideoResource[AL_DEC_NUM_CORES];
}AL_TISchedulerCore;

/****************************************************************************/
typedef enum
{
  AL_ISCHEDULER_CORE, /*!< reference: AL_TISchedulerCore */
  AL_ISCHEDULER_CHANNEL_TRACE_CALLBACK, /*!< reference: AL_TIDecSchedulerChannelTraceCallback */
  AL_ISCHEDULER_MAX_ENUM,
}AL_EIDecSchedulerInfo;

static inline char const* ToStringIDecSchedulerInfo(AL_EIDecSchedulerInfo eInfo)
{
  switch(eInfo)
  {
  case AL_ISCHEDULER_CORE: return "AL_ISCHEDULER_CORE";
  case AL_ISCHEDULER_CHANNEL_TRACE_CALLBACK: return "AL_ISCHEDULER_CHANNEL_TRACE_CALLBACK";
  case AL_ISCHEDULER_MAX_ENUM: return "AL_ISCHEDULER_MAX_ENUM";
  default: return "Unknown info";
  }

  return "Unknown info";
}
