// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "sink.h"
#include "CfgParser.h"

std::unique_ptr<IFrameSink> createFrameWriter(std::string path, ConfigFile& cfg_, int iLayerID_);

