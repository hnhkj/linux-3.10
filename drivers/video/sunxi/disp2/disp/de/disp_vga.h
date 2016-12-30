/*
 *
 * Copyright (c) 2016 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef __DISP_VGA_H__
#define __DISP_VGA_H__

#include "disp_private.h"
#include "disp_display.h"

struct disp_vga_private_data {
	bool enabled;
	bool suspended;

	enum disp_tv_mode vga_mode;
	struct disp_tv_func tv_func;
	struct disp_video_timings *video_info;

	struct disp_clk_info lcd_clk;
	struct clk *clk;
	struct clk *clk_parent;

	s32 frame_per_sec;
	u32 usec_per_line;
	u32 judge_line;

	u32 irq_no;
	spinlock_t data_lock;
	struct mutex mlock;
};

s32 disp_init_vga(void);

#endif
