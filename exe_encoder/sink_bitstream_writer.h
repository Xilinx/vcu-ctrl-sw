// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include "lib_app/Sink.h"
#include "CfgParser.h"

std::unique_ptr<IFrameSink> createBitstreamWriter(std::string path, ConfigFile const& cfg);

