/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_cuda_adb
 * @{
 */
/** @file
 */

#ifndef CUDA_HW_H_
#define CUDA_HW_H_

#include <ddi.h>

#include <stdint.h>

typedef struct cuda_regs {
	ioport8_t b;
	uint8_t pad0[0x1ff];

	ioport8_t a;
	uint8_t pad1[0x1ff];

	ioport8_t dirb;
	uint8_t pad2[0x1ff];

	ioport8_t dira;
	uint8_t pad3[0x1ff];

	ioport8_t t1cl;
	uint8_t pad4[0x1ff];

	ioport8_t t1ch;
	uint8_t pad5[0x1ff];

	ioport8_t t1ll;
	uint8_t pad6[0x1ff];

	ioport8_t t1lh;
	uint8_t pad7[0x1ff];

	ioport8_t t2cl;
	uint8_t pad8[0x1ff];

	ioport8_t t2ch;
	uint8_t pad9[0x1ff];

	ioport8_t sr;
	uint8_t pad10[0x1ff];

	ioport8_t acr;
	uint8_t pad11[0x1ff];

	ioport8_t pcr;
	uint8_t pad12[0x1ff];

	ioport8_t ifr;
	uint8_t pad13[0x1ff];

	ioport8_t ier;
	uint8_t pad14[0x1ff];

	ioport8_t anh;
	uint8_t pad15[0x1ff];
} cuda_regs_t;

/** B register fields */
enum {
	TREQ	= 0x08,
	TACK	= 0x10,
	TIP	= 0x20
};

/** IER register fields */
enum {
	IER_CLR	= 0x00,
	IER_SET	= 0x80,

	SR_INT	= 0x04,
	ALL_INT	= 0x7f
};

/** ACR register fields */
enum {
	SR_OUT	= 0x10
};

/** Packet types */
enum {
	PT_ADB	= 0x00,
	PT_CUDA	= 0x01
};

/** CUDA packet types */
enum {
	CPT_AUTOPOLL	= 0x01
};

enum {
	ADB_MAX_ADDR	= 16
};

#endif

/** @}
 */
