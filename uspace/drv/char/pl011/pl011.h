/*
 * SPDX-FileCopyrightText: 2018 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
