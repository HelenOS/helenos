/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_ski_con
 * @{
 */
/** @file
 */

#ifndef SKI_CON_H
#define SKI_CON_H

#include <adt/circ_buf.h>
#include <async.h>
#include <ddf/driver.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdint.h>

enum {
	ski_con_buf_size = 64
};

/** Ski console */
typedef struct {
	async_sess_t *client_sess;
	ddf_dev_t *dev;
	chardev_srvs_t cds;
	circ_buf_t cbuf;
	uint8_t buf[ski_con_buf_size];
	fibril_mutex_t buf_lock;
	fibril_condvar_t buf_cv;
	/** Memory area mapped to arbitrate with the kernel driver */
	void *mem_area;
} ski_con_t;

extern errno_t ski_con_add(ski_con_t *);
extern errno_t ski_con_remove(ski_con_t *);
extern errno_t ski_con_gone(ski_con_t *);

#endif

/** @}
 */
