/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_sun4v_con
 * @{
 */
/** @file
 */

#ifndef SUN4V_CON_H
#define SUN4V_CON_H

#include <async.h>
#include <ddf/driver.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdint.h>

#include "niagara_buf.h"

/** Sun4v console resources */
typedef struct {
	uintptr_t in_base;
	uintptr_t out_base;
} sun4v_con_res_t;

/** Sun4v console */
typedef struct {
	async_sess_t *client_sess;
	ddf_dev_t *dev;
	chardev_srvs_t cds;
	sun4v_con_res_t res;
	/** Virtual address of the shared input buffer */
	niagara_input_buffer_t *input_buffer;
	/** Virtual address of the shared input buffer */
	niagara_output_buffer_t *output_buffer;
} sun4v_con_t;

extern errno_t sun4v_con_add(sun4v_con_t *, sun4v_con_res_t *);
extern errno_t sun4v_con_remove(sun4v_con_t *);
extern errno_t sun4v_con_gone(sun4v_con_t *);

#endif

/** @}
 */
