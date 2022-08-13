/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_msim_con
 * @{
 */
/** @file
 */

#ifndef MSIM_CON_H
#define MSIM_CON_H

#include <adt/circ_buf.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddi.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdint.h>

enum {
	msim_con_buf_size = 64
};

/** MSIM console resources */
typedef struct {
	uintptr_t base;
	int irq;
} msim_con_res_t;

/** MSIM console */
typedef struct {
	async_sess_t *client_sess;
	ddf_dev_t *dev;
	chardev_srvs_t cds;
	msim_con_res_t res;
	irq_pio_range_t irq_range[1];
	irq_code_t irq_code;
	circ_buf_t cbuf;
	uint8_t buf[msim_con_buf_size];
	fibril_mutex_t buf_lock;
	fibril_condvar_t buf_cv;
	ioport8_t *out_reg;
	cap_irq_handle_t irq_handle;
} msim_con_t;

extern errno_t msim_con_add(msim_con_t *, msim_con_res_t *);
extern errno_t msim_con_remove(msim_con_t *);
extern errno_t msim_con_gone(msim_con_t *);

#endif

/** @}
 */
