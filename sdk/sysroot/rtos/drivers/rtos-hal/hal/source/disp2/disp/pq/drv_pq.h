/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DRV_PQ_H__
#define __DRV_PQ_H__

#define PQ_REG_MASK 0xff000
#define DISP_MASK 0xf
#define CMD_MASK 0xf0

struct matrix4x4 {
	__s64 x00;
	__s64 x01;
	__s64 x02;
	__s64 x03;
	__s64 x10;
	__s64 x11;
	__s64 x12;
	__s64 x13;
	__s64 x20;
	__s64 x21;
	__s64 x22;
	__s64 x23;
	__s64 x30;
	__s64 x31;
	__s64 x32;
	__s64 x33;
};

enum choice{
	BT601_F2F,
	BT709_F2F,
	YCC,
	ENHANCE,
	NUM_SUM,
};

struct matrix_user {
	int cmd;
	int read;
	int choice;
	struct matrix4x4 matrix;
};

struct color_enhance {
	int cmd;
	int read;
	int contrast;
	int brightness;
	int saturation;
	int hue;
};

struct color_matrix {
	int cmd;
	int read;
};

enum pq_package_cmd {
    CMD_HEART_BEAT = 0,
    CMD_GET = 1,
    CMD_SET = 2,
    CMD_SAVE_CONFIG = 3,
    CMD_READ_CONFIG = 4,
};

enum {
    PQ_REPLY = 0,
    HARDWARE_INFO = 1,
    SINGLE_REG = 2,
    REG_SET = 3,
    GAMMA_LUT = 11,
    PEAK_SHARP = 12,
    LTI_SHARP = 13,
    PQD_ENHANCE = 14,
    COLOR_MATRIX_IN = 15,
    COLOR_MATRIX_OUT = 16,
    FCM_CTRL = 17,
    FCE_BE_CTRL = 18,
    CDC_CTRL = 19,
    DCI_CTRL = 20,
    DEBAND_CTRL = 21,
    DE35X_SHARP_CTRL = 22,
    TNR_CTRL = 23,
    GTM_CTRL = 24,
    SNR_CTRL = 25,
    DLC_CTRL = 28,
    FTD_CTRL = 29,
    CTC_CTRL = 30,
    CDC_FCM_CTRL = 191,
};

struct pq_private_data {
	bool enabled;
	s32 (*shadow_protect)(u32 sel, bool protect);
};

struct pq_reg {
	u32 offset;
	u32 value;
};

enum pq_block_flag {
	OP_NONE = 0,
	OP_PEAK = 0x1,
	OP_FTC = 0x2,
	OP_CE = 0x4,
	OP_BWS = 0x8,
	OP_LTI = 0x10,
	OP_FCC = 0x20,
	OP_GAMMA = 0x40,
};

void pq_set_reg(u32 sel, u32 off, u32 value);
void pq_get_reg(u32 sel, u32 offset, u32 *value);
void pq_set_matrix(struct matrix4x4 *conig, int choice, int out, int write);
void pq_set_enhance(struct color_enhance *pq_enh, int read);
void disp_pq_force_flush(int disp);
int pq_ioctl(unsigned int cmd, unsigned long arg);
#endif
