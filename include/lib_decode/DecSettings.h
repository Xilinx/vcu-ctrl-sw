// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Decoder_API
   @{
   \file
 *****************************************************************************/

#pragma once

#include <stdio.h>

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common_dec/DecSynchro.h"
#include "lib_common_dec/StreamSettings.h"

/*************************************************************************//*!
   \brief Decoder Input Mode
   \ingroup Decoder_Settings
*****************************************************************************/
typedef enum
{
  AL_DEC_UNSPLIT_INPUT, /*!< The input is fed to the decoder without delimitations and the decoder find the decoding unit in the data by himself.*/
  AL_DEC_SPLIT_INPUT, /*!< The input is fed to the decoder with buffers containing one decoding unit each. */
}AL_EDecInputMode;

/*************************************************************************//*!
   \brief Decoder Settings
   \ingroup Decoder_Settings
*****************************************************************************/
typedef struct
{
  int iStackSize;       /*!< Size of the command stack handled by the decoder */
  int iStreamBufSize;   /*!< Size of the internal circular stream buffer (0 = default) */
  uint8_t uNumCore;     /*!< Number of core used for the decoding */
  bool bNonRealtime;    /*!< Specify is a non-realtime channel */
  uint32_t uFrameRate;  /*!< Frame rate value used if syntax element isn't present */
  uint32_t uClkRatio;   /*!< Clock ratio value used if syntax element isn't present */
  AL_ECodec eCodec;     /*!< Specify which codec is used */
  bool bParallelWPP;    /*!< Should wavefront parallelization processing be used (might be ignored if not available) */
  uint8_t uDDRWidth;    /*!< Width of the DDR uses by the decoder */
  bool bDisableCache;   /*!< Should the decoder disable the cache */
  bool bLowLat;         /*!< Should low latency decoding be used */
  bool bForceFrameRate; /*!< Should stream frame rate be ignored and replaced by user defined one */
  uint16_t uConcealMaxFps; /*!< Maximum frame rate to apply when stream headers are invalid or corrupted. 0 = no concealment */
  bool bFrameBufferCompression; /*!< Should internal frame buffer compression be used */
  AL_EFbStorageMode eFBStorageMode; /*!< Specifies the storage mode the decoder should use for the frame buffers*/
  AL_EDecUnit eDecUnit; /*!< Should subframe latency mode be used */
  AL_EDpbMode eDpbMode; /*!< Should low ref mode be used */
  AL_TStreamSettings tStream; /*!< Stream's settings. These need to be set if you want to preallocate the buffer. memset to 0 otherwise */
  AL_TPosition tOutputPosition; /*!< Position of the top left corner of the decoded frames in the Output buffers*/
  bool bUseIFramesAsSyncPoint; /*!< Allow decoder to sync on I frames if configurations' nals are presents */
  bool bUseEarlyCallback; /*< Lowlat phase 2. This only makes sense with special support for hw synchro */
  AL_EDecInputMode eInputMode; /* Send stream data by decoding unit or feed the library enough data and let it find the units. */
}AL_TDecSettings;

/*************************************************************************//*!
   \brief Retrieves the default settings
   \param[out] pSettings Pointer to TDecSettings structure that receives
   default Settings.
*****************************************************************************/
void AL_DecSettings_SetDefaults(AL_TDecSettings* pSettings);

/*************************************************************************//*!
   \brief Checks that all decoding parameters are valid
   \param[in] pSettings Pointer to TDecSettings to be checked
   \param[in] pOut Optional standard stream on which verbose messages are
   written.
   \return If pSettings point to valid parameters the function returns false
   If pSettings point to invalid parameters the function returns
   the number of invalid parameters found (true);
   and this Settings can not be used with IP encoder.
*****************************************************************************/
int AL_DecSettings_CheckValidity(AL_TDecSettings const* pSettings, FILE* pOut);

/**************************************************************************//*!
   \brief Checks that decoding parameters are coherent between them.
   When incoherent parameter are found, the function automatically correct them.
   \param[in] pSettings Pointer to TDecSettings to be checked
   \param[in] pOut Optional standard stream on which verbose messages are
   written.
   \return 0 if no incoherency
           the number of incoherency if incoherency were found
           -1 if a fatal incoherency was found
   Since the function automatically apply correction, the Settings can be then used
   with IP decoder.
 *****************************************************************************/
int AL_DecSettings_CheckCoherency(AL_TDecSettings* pSettings, FILE* pOut);
/*@}*/
