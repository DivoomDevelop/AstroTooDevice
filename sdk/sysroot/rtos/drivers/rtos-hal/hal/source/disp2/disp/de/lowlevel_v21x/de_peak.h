/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2025 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_PEAK_H__
#define __DE_PEAK_H__

int de_peak_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data);
//int de_peak_pq_proc(unsigned int sel, unsigned int cmd, unsigned int subcmd, void *data);

#endif
