// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "Sections.h"
#include "IP_EncoderCtx.h"

AL_TNuts CreateHevcNuts(void);
void HEVC_GenerateSections(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iLayerID, int iPicID, bool bMustWritePPS, bool bMustWriteAUD);

