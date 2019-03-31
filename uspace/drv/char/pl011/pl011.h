/*
 * Copyright (c) 2018 Petr Pavlu
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

/** @addtogroup uspace_drv_pl011
 * @{
 */
/** @file
 */

#ifndef PL011_CON_H
#define PL011_CON_H

#include <adt/circ_buf.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddi.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdint.h>

enum {
	pl011_buf_size = 64
};

/** PL011 device resources. */
typedef struct {
	uintptr_t base;
	int irq;
} pl011_res_t;

/** PL011 device. */
typedef struct {
	ddf_dev_t *dev;
	chardev_srvs_t cds;
	pl011_res_t res;
	irq_code_t irq_code;
	circ_buf_t cbuf;
	uint8_t buf[pl011_buf_size];
	fibril_mutex_t buf_lock;
	fibril_condvar_t buf_cv;
	void *regs;
	cap_irq_handle_t irq_handle;
} pl011_t;

extern errno_t pl011_add(pl011_t *, pl011_res_t *);
extern errno_t pl011_remove(pl011_t *);
extern errno_t pl011_gone(pl011_t *);

#endif

/** @}
 */
