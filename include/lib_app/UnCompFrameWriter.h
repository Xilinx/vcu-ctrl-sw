// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Sink.h"
#include "SinkBaseWriter.h"

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_common/BufferAPI.h"
}

/****************************************************************************/
struct UnCompFrameWriter final : IFrameSink, BaseFrameSink
{
  UnCompFrameWriter(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode, AL_EOutputType outputID);

  void ProcessFrame(AL_TBuffer* pBuf) override;

private:
  void DimInTileCalculusRaster();
};

