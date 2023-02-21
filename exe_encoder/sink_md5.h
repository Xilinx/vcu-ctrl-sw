// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "sink.h"
#include "CfgParser.h"

std::unique_ptr<IFrameSink> createYuvMd5Calculator(std::string path, ConfigFile& cfg_);
std::unique_ptr<IFrameSink> createStreamMd5Calculator(std::string path);

