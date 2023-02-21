// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/HDR.h"

/***************************************************************************/
int AL_H273_ColourDescToColourPrimaries(AL_EColourDescription colourDesc);

/***************************************************************************/
AL_EColourDescription AL_H273_ColourPrimariesToColourDesc(int iColourPrimaries);

/***************************************************************************/
int AL_TransferCharacteristicsToVUIValue(AL_ETransferCharacteristics eTransferCharacteristics);

/***************************************************************************/
AL_ETransferCharacteristics AL_VUIValueToTransferCharacteristics(int iTransferCharacteristics);

/***************************************************************************/
int AL_ColourMatrixCoefficientsToVUIValue(AL_EColourMatrixCoefficients eColourMatrixCoef);

/***************************************************************************/
AL_EColourMatrixCoefficients AL_VUIValueToColourMatrixCoefficients(int iColourMatrixCoef);
