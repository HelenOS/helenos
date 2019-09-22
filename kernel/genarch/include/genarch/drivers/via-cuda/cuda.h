/*
 * Copyright (c) 2006 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
