/*
 * Copyright (c) 2010 Jiri Svoboda
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Gaisler GRLIB UART IP-Core driver.
 */

#ifndef KERN_GRLIB_UART_H_
#define KERN_GRLIB_UART_H_

#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>

typedef struct {
	unsigned int rcnt: 6;
	unsigned int tcnt: 6;
	unsigned int : 9;
	unsigned int rf: 1;
	unsigned int tf: 1;
	unsigned int rh: 1;
	unsigned int th: 1;
	unsigned int fe: 1;
	unsigned int pe: 1;
	unsigned int ov: 1;
	unsigned int br: 1;
	unsigned int te: 1;
	unsigned int ts: 1;
	unsigned int dr: 1;
} grlib_uart_status_t;

typedef struct {
	unsigned int fa: 1;
	unsigned int : 16;
	unsigned int si: 1;
	unsigned int di: 1;
	unsigned int bi: 1;
	unsigned int db: 1;
	unsigned int rf: 1;
	unsigned int tf: 1;
	unsigned int ec: 1;
	unsigned int lb: 1;
	unsigned int fl: 1;
	unsigned int pe: 1;
	unsigned int ps: 1;
	unsigned int ti: 1;
	unsigned int ri: 1;
	unsigned int te: 1;
	unsigned int re: 1;
} grlib_uart_control_t;

/** GRLIB UART registers */
typedef struct {
	uint32_t data;
	uint32_t status;
	uint32_t control;
	uint32_t scaler;
	uint32_t debug;
} grlib_uart_io_t;

typedef struct {
	grlib_uart_io_t *io;
	indev_t *indev;
	irq_t irq;
	parea_t parea;
} grlib_uart_t;

extern outdev_t *grlib_uart_init(uintptr_t, inr_t);
extern void grlib_uart_input_wire(grlib_uart_t *,
    indev_t *);

#endif

/** @}
 */
