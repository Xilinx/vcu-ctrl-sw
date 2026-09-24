// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

struct al5_reg {
	uint32_t id;
	uint32_t value;
};

struct al5_dma32_info {
	int32_t fd;
	uint32_t size;
	uint32_t phy_addr;
};

struct al5_dma64_info {
	int32_t fd;
	uint32_t size;
	uint64_t phy_addr;
};
