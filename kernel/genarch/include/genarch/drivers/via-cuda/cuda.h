/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_CUDA_H_
#define KERN_CUDA_H_

#include <ddi/irq.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <synch/spinlock.h>

typedef struct {
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
} cuda_t;

enum {
	CUDA_RCV_BUF_SIZE = 5
};

enum cuda_xfer_state {
	cx_listen,
	cx_receive,
	cx_rcv_end,
	cx_send_start,
	cx_send
};

typedef struct {
	irq_t irq;
	cuda_t *cuda;
	indev_t *kbrdin;
	uint8_t rcv_buf[CUDA_RCV_BUF_SIZE];
	uint8_t snd_buf[CUDA_RCV_BUF_SIZE];
	size_t bidx;
	size_t snd_bytes;
	enum cuda_xfer_state xstate;
	SPINLOCK_DECLARE(dev_lock);
} cuda_instance_t;

extern cuda_instance_t *cuda_init(cuda_t *, inr_t, cir_t, void *);
extern void cuda_wire(cuda_instance_t *, indev_t *);

#endif

/** @}
 */
